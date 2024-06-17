# SFU
基于mediasoup的SFU。
1.重写了js层
2.支持单进程和多进程模式
3.应用层使用oatpp
4.支持最新的mediasoup，worker通讯采用flatbuffers
5.支持linux部署脚本

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
