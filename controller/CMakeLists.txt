cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME "controller")
project(${PROJECT_NAME})

# C++ standard requirements.
set(CMAKE_CXX_STANDAR 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -g -gdwarf-2 -Wall -Wendif-labels -W -Wno-unused-parameter -Wall -Wextra -Wno-unused-parameter -std=c++17")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIC")
ENDIF()
message(STATUS "CMAKE_CXX_FLAGS : " ${CMAKE_CXX_FLAGS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(headers
  interface/i_consumer_controller.h 
  interface/i_data_consumer_controller.h 
  interface/i_data_producer_controller.h 
  interface/i_producer_controller.h 
  interface/i_transport_controller.h 
  interface/i_router_controller.h 
  interface/i_rtp_observer_controller.h 
  interface/i_webrtc_server_controller.h 
  interface/i_worker_controller.h 
  config.h
  rtc_export.h
  common_types.h
  types.h
  utils.h
  uuid.h
  h264_profile_level_id.h
  ortc.h
  engine.h
  unix_stream_socket_handle.h
  srv_logger.h
  channel_socket.h
  channel.h
  worker_controller.h
  rtp_stream.h
  parameters.h
  srtp_parameters.h
  sctp_parameters.h
  scalability_mode.h
  rtp_parameters.h
  supported_rtp_capabilities.h
  rtp_observer_controller.h
  audio_level_observer_controller.h
  active_speaker_observer_controller.h
  router_controller.h
  webrtc_server_controller.h
  abstract_transport_controller.h
  direct_transport_controller.h
  plain_transport_controller.h
  pipe_transport_controller.h
  webrtc_transport_controller.h
  consumer_controller.h
  data_consumer_controller.h
  producer_controller.h
  data_producer_controller.h
)

set(sources
  types.cpp
  config.cpp
  utils.cpp
  uuid.cpp
  h264_profile_level_id.cc
  ortc.cpp
  engine.cpp
  unix_stream_socket_handle.cpp
  channel_socket.cpp
  channel.cpp
  worker_controller.cpp
  rtp_stream.cpp
  parameters.cpp
  srtp_parameters.cpp
  sctp_parameters.cpp
  scalability_mode.cpp
  rtp_parameters.cpp
  supported_rtp_capabilities.cpp
  rtp_observer_controller.cpp
  audio_level_observer_controller.cpp
  active_speaker_observer_controller.cpp
  router_controller.cpp
  webrtc_server_controller.cpp
  abstract_transport_controller.cpp
  direct_transport_controller.cpp
  plain_transport_controller.cpp
  pipe_transport_controller.cpp
  webrtc_transport_controller.cpp
  consumer_controller.cpp
  data_consumer_controller.cpp
  producer_controller.cpp
  data_producer_controller.cpp
)

add_library(controller
  STATIC
  ${headers}
  ${sources}
)

message(STATUS "CMAKE_CURRENT_SOURCE_DIR : " ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "PROJECT_INCLUDE_DIR : " ${PROJECT_INCLUDE_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE
  ${WORKER_INCLUDE_DIR}
  ${LIB_WEBRTC_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../deps/json/single_include
  ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libwebrtc/deps/abseil-cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../deps/asio/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../release/include/fbs
  ${PROJECT_INCLUDE_DIR}
)

target_include_directories(${PROJECT_NAME}
          INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

  #target_link_libraries(${PROJECT_NAME}
  #    worker
  #    libwebrtc)  

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
