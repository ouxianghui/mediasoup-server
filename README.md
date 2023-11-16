# SFU

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
