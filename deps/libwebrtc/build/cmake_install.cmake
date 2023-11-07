# Install script for directory: /Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/Users/jackie.ou/Desktop/Research/mediasoup-server/release")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/build/liblibwebrtc.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/liblibwebrtc.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/liblibwebrtc.a")
    execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/liblibwebrtc.a")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc/system_wrappers/source" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc/system_wrappers/source" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc/rtc_base" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc/rtc_base" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc/call" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc/call" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc/api" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc/api" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc/api/transport" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc/api/transport" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc/api/units" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc/api/units" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc/modules/include" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc/modules/include" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc/modules/pacing" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc/modules/pacing" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc/modules/remote_bitrate_estimator" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc/modules/remote_bitrate_estimator" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc/modules/remote_bitrate_estimator/include" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc/modules/remote_bitrate_estimator/include" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc/modules/rtp_rtcp/include" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc/modules/rtp_rtcp/include" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc/modules/rtp_rtcp/rtp_packet" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_packet" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc/modules/bitrate_controller" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc/modules/bitrate_controller" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc/modules/congestion_controller/goog_cc" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc/modules/congestion_controller/goog_cc" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libwebrtc/modules/congestion_controller/rtp" TYPE DIRECTORY FILES "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/libwebrtc/modules/congestion_controller/rtp" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/Users/jackie.ou/Desktop/Research/mediasoup-server/deps/libwebrtc/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
