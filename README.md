# SFU

Mediasoup based SFU:

1. Rewrote the JavaScript layer in C++
2. Supports single process and multi process modes
3. Use Oatpp at the application layer
4. Support the latest mediasoup and use flatbuffers for communication between the application layer and workers
5. Application layer and worker communication support pipe and direct callback
6. Support Linux deployment scripts
7. Simple and lightweight
8. High scalability and maintainability

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
