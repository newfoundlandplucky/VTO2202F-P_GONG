# VTO2202F-P_GONG
Monitor up to two Dahua VTO2202F-P VTO doorbell units and close a corresponding doorbell relay when either VTO is calling

![Deployment on NanoPI NEO Core2](https://github.com/newfoundlandplucky/VTO2202F-P_GONG/blob/master/documentation/Delpoyment.jpg?raw=true)

**Tested using the following environment**
         SBC: NanoPI NEO Core2 "Armbian 20.08 Focal"
              Linux Elvis 5.7.15-sunxi64 #20.08 SMP Mon Aug 17 10:32:57 CEST 2020 aarch64 aarch64 aarch64 GNU/Linux
    VTO Main: VTO2202F 4.410.0000000.4.R
     VTO Sub: VTO2202F 4.400.0000000.2.R
         VTH: VTH2421F-P 4.400.0000000.7.R

**Vendors and Forums**
* IP Cam Talk: https://www.ipcamtalk.com/threads/review-ip-villa-outdoor-doorbell-station-indoor-monitor-kit.49853/
* BEC Technology Co.: https://ipcamtalk.com/forums/empiretech-andy.68/
* Friendly Elec: https://www.friendlyarm.com/

**Sample Output**
```
elvis@NanoPI:~/gong$ sudo ./gong.sh
[sudo] password for elvis:
Thu Sep  3 14:33:16 2020: Running: ./gong -d -i 192.168.1.26 -g 224.0.2.14 -b 2 -c 10 -m 192.168.1.108 -1 PG6 -s 192.168.1.110 -2 PG11
Thu Sep  3 14:33:16 2020: Opening datagram socket ... OK.
Thu Sep  3 14:33:16 2020: Setting SO_REUSEADDR ... OK.
Thu Sep  3 14:33:16 2020: Binding datagram socket ... OK.
Thu Sep  3 14:33:16 2020: Adding multicast group ... OK.
Thu Sep  3 14:33:16 2020: Enabling packet info ... OK.
Thu Sep  3 14:33:16 2020: Set socket timeout to 1.500
Thu Sep  3 14:34:09 2020: VTO 192.168.1.108 80 60 38 83 ff ff d7 eb ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:09 2020: MainVTO-PG6 button pressed.
Thu Sep  3 14:34:11 2020: VTO 192.168.1.108 80 60 39 ea 00 02 97 0b ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:11 2020: MainVTO-PG6 button released.
Thu Sep  3 14:34:13 2020: VTO 192.168.1.108 80 60 3b 74 00 05 56 2b ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:15 2020: VTO 192.168.1.108 80 60 3c f3 00 08 15 4b ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:17 2020: VTO 192.168.1.108 80 60 3e 73 00 0a d4 6b ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:19 2020: MainVTO-PG6 button pressed.
Thu Sep  3 14:34:19 2020: VTO 192.168.1.108 80 60 3f f2 00 0d 93 8b ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:21 2020: MainVTO-PG6 button released.
Thu Sep  3 14:34:21 2020: VTO 192.168.1.108 80 60 41 72 00 10 52 ab ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:23 2020: VTO 192.168.1.108 80 60 42 f0 00 13 11 cb ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:25 2020: VTO 192.168.1.108 80 60 44 6e 00 15 d0 eb ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:27 2020: VTO 192.168.1.108 80 60 45 f0 00 18 90 0b ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:29 2020: MainVTO-PG6 button pressed.
Thu Sep  3 14:34:29 2020: VTO 192.168.1.108 80 60 47 6c 00 1b 4f 2b ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:31 2020: MainVTO-PG6 button released.
Thu Sep  3 14:34:31 2020: VTO 192.168.1.108 80 60 48 e8 00 1e 0e 4b ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:33 2020: VTO 192.168.1.108 80 60 4a 6c 00 20 cd 6b ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:35 2020: VTO 192.168.1.108 80 60 4b ec 00 23 8c 8b ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:37 2020: VTO 192.168.1.108 80 60 4d 66 00 26 4b ab ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:39 2020: MainVTO-PG6 button pressed.
Thu Sep  3 14:34:39 2020: VTO 192.168.1.108 80 60 4e e8 00 29 05 2b ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:41 2020: MainVTO-PG6 button released.
Thu Sep  3 14:34:41 2020: VTO 192.168.1.108 80 60 50 62 00 2b c4 4b ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:43 2020: VTO 192.168.1.108 80 60 51 de 00 2e 83 6b ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:45 2020: VTO 192.168.1.108 80 60 53 5a 00 31 42 8b ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:47 2020: VTO 192.168.1.108 80 60 54 d9 00 34 01 ab ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:49 2020: MainVTO-PG6 button pressed.
Thu Sep  3 14:34:49 2020: VTO 192.168.1.108 80 60 56 53 00 36 c0 cb ff ff e9 5e 68 ce 31 b2
Thu Sep  3 14:34:51 2020: MainVTO-PG6 button released.
Thu Sep  3 14:34:51 2020: MainVTO-PG6 is no longer calling
```

**Benchtop Evaluation**

![Benchtop Evaluation](https://github.com/newfoundlandplucky/VTO2202F-P_GONG/blob/master/documentation/BenchTest.jpg?raw=true)
