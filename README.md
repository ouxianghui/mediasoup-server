# SFU
基于mediasoup的SFU: 

1.用C++重写了js层

2.支持单进程和多进程模式

3.应用层使用oatpp

4.支持最新的mediasoup，应用层与worker通讯采用flatbuffers

5.应用层与worker通讯支持pipe和直接回调

6.支持linux部署脚本

7.超简洁、超轻量

8.易拓展、好维护

# Build & Run

1.macOS

  a.run builddeps.sh
  
  b.run genxcode.sh

  c.open project with xcode

2.Ubuntu

  a.run build.sh

  b.run ./build/RELEASE/sfu

  c.deploy
  
    1) cd install
    
    2) sudo ./install.sh
    
    3) sudo service sfu start|stop|restart|status
    
    4) sudo ./unstall.sh
