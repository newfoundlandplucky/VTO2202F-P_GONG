// This program monitors two Dahua VTO2202F-P VTO units and closes a corresponding doorbell relay when either VTO is
// ringing the VTH.
//
// Example tcpdump packet capture ( these packets drive the state machine ):
//
// 10:58:00.987049 08:ed:ed:e6:bc:84 (oui Unknown) > 01:00:5e:00:02:0e (oui Unknown),
//      ethertype IPv4 (0x0800), length 60: (tos 0x0, ttl 1, id 54610, offset 0, flags [DF], proto UDP (17), length 44)
//      192.168.1.110.20001 > 224.0.2.14.30000: [udp sum ok] UDP, length 16
//
// Example logging output from this program:
//
//  Wed Sep  2 18:43:56 2020 [00,00]: 192.168.1.110: 80 60 7b 3e 47 28 84 55 ff ff f3 0f 68 ce 31 b2

#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#define DAHUA_PORT      30000
#define PRESS_BUTTON    "1"
#define RELEASE_BUTTON  "0"

typedef struct _vto {
    char      alias[32];
    char      ip[INET_ADDRSTRLEN];
    char      relay_pin[8];
    int       call_in_progress;
    pthread_t thread;
} vto;

typedef struct _application {
    char     *version;
    int       debug;
    char      interface[INET_ADDRSTRLEN];
    char      multicast[INET_ADDRSTRLEN];
    int       relay_timeout;
    int       cooldown_timeout;
    vto       main;
    vto       sub;
} application;

static application context = {
    .version = "0.5",
    .multicast = "224.0.2.14",
    .relay_timeout = 2,
    .cooldown_timeout = 10,
    .main = { .alias = "MainVTO", .relay_pin = "PG6" },
    .sub = { .alias = "SubVTO", .relay_pin = "PG11" } };

static void log_fatal( const char *format, ... )
{
    char message[BUFSIZ];
    va_list args;

    va_start( args, format );
    memset( message, '\0', sizeof( message ) );
    vsnprintf( message, sizeof( message ) - 1, format, args );
    if( errno )
        perror( message );
    else
        fprintf( stderr, "%s\n", message );

    exit( EXIT_FAILURE );
}

static void log_debug( const char* format, ... )
{
    if( context.debug )
    {
        time_t now;
        char *date;
        va_list args;

        va_start( args, format );
        time( &now );
        date = ctime( &now );
        date[ strlen( date ) - 1 ] = '\0';
        fprintf( stderr, "%s: ", date );
        vfprintf( stderr, format, args );
        fprintf( stderr, "\n" );
    }
}

static const char *bytes_to_hex( const void *ascii, size_t length, const void *data, size_t size )
{
    char *output = (char *) ascii, *input = (char *) data;

    if( size * 3 > length )
        log_fatal( "Datagram of %d bytes can't be formatted to fit into a %d byte ascii buffer.", size, length );
    for( int i = 0; i < size; i++, input++ )
        output += sprintf( output, i == 0 ? "%02x" : " %02x", *input );

    return ascii;
}

#define USAGE_D_DEBUG     "-d: Show debug messages. No parameter. [DEFAULT OFF]"
#define USAGE_I_INTERFACE "-i: Interface to monitor (e.g. 192.168.1.111)"
#define USAGE_G_MULTICAST "-g: VTO multicast ip address [DEFAULT 224.0.2.14]"
#define USAGE_B_BUTTON    "-b: Button press time in seconds [DEFAULT 2]"
#define USAGE_C_COOLDOWN  "-c: Cooldown between button presses in seconds [DEFAULT: 8]"
#define USAGE_M_MAINIP    "-m: Main VTO ip address (e.g. 192.168.1.108)"
#define USAGE_1_MAINRELAY "-1: Main VTO relay name [DEFAULT PG6]"
#define USAGE_S_SUBIP     "-s: Sub VTO ip address (e.g., 192.168.1.110)"
#define USAGE_2_SUBRELAY  "-2: Sub VTO relay name [DEFAULT PG11]"

#define USAGE_HELP        "Usage: %s " \
                          "[-d] -i <ip> -g <ip> [-b <integer>] [-c <integer>] -m <ip> [-1 <pin>] [-s <ip>] [-2 <pin>]\n" \
                          "\t" USAGE_D_DEBUG "\n" \
                          "\t" USAGE_I_INTERFACE "\n" \
                          "\t" USAGE_G_MULTICAST "\n" \
                          "\t" USAGE_B_BUTTON "\n" \
                          "\t" USAGE_C_COOLDOWN "\n" \
                          "\t" USAGE_M_MAINIP "\n" \
                          "\t" USAGE_1_MAINRELAY "\n" \
                          "\t" USAGE_S_SUBIP "\n" \
                          "\t" USAGE_2_SUBRELAY "\n" \
                          "\tSend SIGINT to end program\n"

static void configure( int argc, char *argv[] )
{
    int c = 0;
    opterr = 0;
    while( ( c = getopt( argc, argv, "di:g:b:c:m:1:s:2:" ) ) != -1 )
    {
        struct in_addr ip;
        switch( c )
        {
            case 'd':
                context.debug = 1;
                break;
            case 'i':
                strncpy( context.interface, optarg, sizeof( context.interface ) );
                if( inet_aton( context.interface, &ip ) == 0 )
                    log_fatal( "Error converting %s into an interface address.", context.interface );
                break;
            case 'g':
                strncpy( context.multicast, optarg, sizeof( context.multicast ) );
                if( inet_aton( context.multicast, &ip ) == 0 )
                    log_fatal( "Error converting %s into a multicast address.", context.multicast );
                break;
            case 'b':
                context.relay_timeout = atoi( optarg );
                break;
            case 'c':
                context.cooldown_timeout = atoi( optarg );
                break;
            case 'm':
                strncpy( context.main.ip, optarg, sizeof( context.main.ip ) );
                if( inet_aton( context.main.ip,  &ip ) == 0 )
                    log_fatal( "Error converting %s into a VTO address.", context.main.ip );
                break;
            case '1':
                strncpy( context.main.relay_pin, optarg, sizeof( context.main.relay_pin ) );
                break;
            case 's':
                strncpy( context.sub.ip , optarg, sizeof( context.sub.ip ) );
                if( inet_aton( context.sub.ip,  &ip ) == 0 )
                    log_fatal( "Error converting %s into a VTO address.", context.sub.ip );
                break;
            case '2':
                strncpy( context.sub.relay_pin, optarg, sizeof( context.sub.relay_pin ) );
                break;
            case '?':
                if( optopt == 'i' || optopt == 'g' || optopt == 'b' || optopt == 'c' ||
                    optopt == 'm' || optopt == '1' || optopt == 's' || optopt == '2' )
                    log_fatal( "Option -%c requires an argument.\n", optopt );
                else if( isprint( optopt ) )
                    log_fatal(  "Unknown option -%c.\n", optopt );
                else
                    log_fatal( "Unknown option character \\x%x.\n", optopt );
                break;
            default:
                log_fatal( USAGE_HELP, argv[0] );
        }
    }
    if( strlen( context.interface ) == 0 )
        log_fatal( "Missing " USAGE_I_INTERFACE "\n\n" USAGE_HELP, argv[0] );
    if( strlen( context.main.ip ) == 0 )
       log_fatal( "Missing " USAGE_M_MAINIP "\n\n" USAGE_HELP, argv[0] );
    log_debug( "Running: %s%s -i %s -g %s -b %d -c %d -m %s -1 %s -s %s -2 %s",
                argv[0], context.debug ? " -d" : "",
                context.interface, context.multicast, context.relay_timeout, context.cooldown_timeout,
                context.main.ip, context.main.relay_pin,
                context.sub.ip, context.sub.relay_pin );
}

static int open_network_socket( void )
{
    // Create a socket on which to receive multicast datagram.
    int descriptor = socket( AF_INET, SOCK_DGRAM, 0 );
    if( descriptor < 0 )
        log_fatal( "Error opening socket." );
    log_debug( "Opening datagram socket ... OK." );

    // Enable SO_REUSEADDR to allow multiple instances of this application to receive copies of the
    // multicast datagrams.
    int reuse = 1;
    if( setsockopt( descriptor, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof( reuse ) ) < 0 )
        log_fatal( "Setting SO_REUSEADDR error.");
    log_debug( "Setting SO_REUSEADDR ... OK." );

    // Bind to the proper port number with the IP address specified as INADDR_ANY.
    struct sockaddr_in network;
    memset( &network, '\0', sizeof( network ) );
    network.sin_family = AF_INET;
    network.sin_port = htons( DAHUA_PORT );
    network.sin_addr.s_addr = INADDR_ANY;
    if( bind( descriptor, (struct sockaddr *) &network, sizeof( network ) ) )
        log_fatal( "Binding datagram socket error." );
    log_debug( "Binding datagram socket ... OK." );

    // Note that this IP_ADD_MEMBERSHIP option must be called for each local interface over which the multicast
    // datagrams are to be received.
    struct ip_mreq group;
    memset( &group, '\0', sizeof( group ) );
    group.imr_interface.s_addr = inet_addr( context.interface );
    group.imr_multiaddr.s_addr = inet_addr( context.multicast );
    if( setsockopt( descriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof( group ) ) < 0 )
        log_fatal( "Adding multicast group error. Interface %s multicast %s.\n", context.interface, context.multicast );
    log_debug( "Adding multicast group ... OK." );

    const int on = 1, off = 0;
    setsockopt( descriptor, IPPROTO_IP, IP_PKTINFO, &on, sizeof( on ) );
    log_debug( "Enabling packet info ... OK." );

    struct timeval timeout = { 1, 500 };
    if( setsockopt( descriptor, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof( timeout ) ) < 0 )
        log_fatal( "Set socket timeout error %ld.%ld.", timeout.tv_sec, timeout.tv_usec );
    log_debug( "Set socket timeout to %ld.%ld", timeout.tv_sec, timeout.tv_usec );

    return descriptor;
}

// To convert a hardware pin name (PXn) to a linux port number, use the following formula:
//      X * 32 + n
// where "X" is the group's ASCII value minus 65 (so A = 0, B = 1, C = 2, ...) and "n" is the
// decadic integer immediately following.
//
// For example, PG7 is 6 * 32 + 7 = 199.
//
// After the export command, a new directory representing the port will appear in the filesystem
// ( e.g. /sys/class/gpio/gpio199 ). Inside you'll find the files you probably know how to use
// including direction, and value, ...
 
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"

static void set_button_value( vto *v, const char *value )
    // pin name is P<group><number> where <group> is an ascii letter and <number> is an integer
    // composed of one or more consecutive ascii digits.
{
    char pin[BUFSIZ];
    memset( pin, '\0', sizeof( pin ) );
    snprintf( pin, sizeof( pin ) - 1, "%d", ( v->relay_pin[1] - 'A' ) * 32 + atoi( v->relay_pin + 2 ) );

    char hw[BUFSIZ];
    memset( hw, '\0', sizeof( hw ) );
    snprintf( hw, sizeof( hw ) - 1, "/sys/class/gpio/gpio%s", pin );

    struct stat state;
    int xp = stat( hw, &state );
    if( xp == -1 )
    {
        int xfd = open( "/sys/class/gpio/export", O_WRONLY );
        if( xfd == -1 )
            log_fatal( "Failed to open /sys/class/gpio/export for %s-%s. Need to be root", v->alias, v->relay_pin );
        if( write( xfd, pin, strlen( pin ) ) == -1 )
            log_fatal( "Failed to write %s to /sys/class/gpio/export for %s-%s", pin, v->alias, v->relay_pin );
        close( xfd );

        // Set GPIO pin to output.
        char dfn[BUFSIZ];
        memset( dfn, '\0', sizeof( dfn ) );
        snprintf( dfn, sizeof( dfn ) - 1, "%s/direction", hw );

        int dfd = open( dfn, O_WRONLY );
        if( dfd == -1 )
            log_fatal( "Failed to open %s for pin %s-%s", dfn, v->alias, v->relay_pin );
        if( write( dfd, "out", strlen( "out" ) ) == -1 )
            log_fatal( "Failed to write %s direction for pin %s-%s", dfn, v->alias, v->relay_pin );
        close( dfd );
    }

    // Write value to GPIO.
    char vfn[BUFSIZ];
    memset( vfn, '\0', sizeof( vfn ) );
    snprintf( vfn, sizeof( vfn ) - 1, "%s/value", hw );

    int vfd = open( vfn, O_WRONLY );
    if( vfd == -1 )
        log_fatal( "Failed to open %s for pin %s-%s", vfn, v->alias, v->relay_pin );
    if( write( vfd, value, strlen( value ) ) == -1 )
        log_fatal( "Failed to write %s to %s for pin %s-%s. Pin direction changed?", value, vfn, v->alias, v->relay_pin );
    close( vfd );

    log_debug( "%s-%s button %s.", v->alias, v->relay_pin, strcmp( value, "0" ) ? "pressed" : "released" );
}

#pragma GCC diagnostic pop

// VTO sends a 16 byte UDP message, approximately every couple of seconds, when soliciting an answer from the VTH.
// The VTH sends these messages for a few minutes. The messages drive a state machine that presses the doorbell
// button for <relay_timer default 2> seconds and also waits for <cooldown_timer default 10> seconds before allowing
// the relay to chime again. By default this will cause the doorbell to chime about 5 times over the entire call cycle.

void *vto_is_calling( void *args )
{
    vto *v = (vto *) args;

    while( v->call_in_progress )
    {
        set_button_value( v, PRESS_BUTTON );
        if( v->call_in_progress )
            sleep( context.relay_timeout );
       set_button_value( v, RELEASE_BUTTON );
        if( v->call_in_progress )
            sleep( context.cooldown_timeout - context.relay_timeout );
    }

    return args;
}

static void vto_is_not_calling( vto *v )
{
    if( v->call_in_progress )
    {
        v->call_in_progress = 0;
        if( pthread_join( v->thread, NULL  ) != 0 )
            log_fatal( "Failed to get returm value from thread for %s-%s", v->alias, v->relay_pin );
        log_debug( "%s-%s is no longer calling", v->alias, v->relay_pin );
    }
}

static void interrupt_handler( int signum )
{
    vto_is_not_calling( &context.main );
    vto_is_not_calling( &context.sub );
    log_debug( "Caught signal %d ...", signum );

    exit( EXIT_SUCCESS );
}

int main( int argc, char *argv[] )
{
    configure( argc, argv );
    int descriptor = open_network_socket( );

    vto_is_not_calling( &context.main );
    vto_is_not_calling( &context.sub );

    signal( SIGINT, interrupt_handler );

    while( 1 )
    {
        char buffer[BUFSIZ];
        memset( buffer, '\0', sizeof( buffer ) );
        struct sockaddr address_buffer;
        socklen_t address_size = sizeof( address_buffer );
        ssize_t size = recvfrom( descriptor, buffer, sizeof( buffer ), 0, &address_buffer, &address_size );

        struct sockaddr_in *in = (struct sockaddr_in *) &address_buffer;
        char *ip_address = inet_ntoa( in->sin_addr );

        if( size == -1 && ( errno == EAGAIN || size == EWOULDBLOCK ) )
        {
            // Only time out when both main vto and sub vto are quiet. As a result one or the other vto may
            // take slightly longer to timeout because it has to wait for the other to finish call handling.
            vto_is_not_calling( &context.main );
            vto_is_not_calling( &context.sub );
        }
        else if( size < 0 )
        {
            log_fatal( "Read data from socket error." );
        }
        else if( size == 16 && buffer[0] == 0x80 && ( buffer[1] == 0x60 || buffer[1] == 0xe0 ) )
        {
            char hex[BUFSIZ];
            memset( hex, '\0', sizeof( hex ) );
            log_debug( "VTO %s %s", ip_address, bytes_to_hex( hex, sizeof( hex ), buffer, size ) );

            if( strcmp( ip_address, context.main.ip ) == 0 && context.main.call_in_progress  == 0 )
            {
                context.main.call_in_progress = 1;
                pthread_create( &context.main.thread, NULL, vto_is_calling, &context.main );
            }
            else if( strcmp( ip_address, context.sub.ip ) == 0 && context.sub.call_in_progress  == 0 )
            {
                context.sub.call_in_progress = 1;
                pthread_create( &context.sub.thread, NULL, vto_is_calling, &context.sub );
            }
        }
    }
}
