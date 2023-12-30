cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME "sfu")

project(${PROJECT_NAME})
message("\n=========== ${PROJECT_NAME} Build Configuration ===========\n")
message(STATUS "PROJECT_INCLUDE_DIR : " ${PROJECT_INCLUDE_DIR})
message(STATUS "PROJECT_LIB_DIR : " ${PROJECT_LIB_DIR})

# C++ standard requirements.
set(CMAKE_CXX_STANDAR 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
ENDif ()

IF (WIN32)

ELSEIF (APPLE)

ELSEIF (UNIX)
  message(STATUS "This is BINARY dir " ${PROJECT_BINARY_DIR})
  message(STATUS "This is SOURCE dir " ${PROJECT_SOURCE_DIR})
  message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
  message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
ENDIF ()

include_directories(src)

set(sources
    src/app.cpp
    src/app_component.hpp
    src/controller/rooms_controller.hpp
    src/controller/statistics_controller.hpp
    src/rooms/peer.cpp
    src/rooms/peer.hpp
    src/rooms/room.cpp
    src/rooms/room.hpp
    src/rooms/lobby.cpp
    src/rooms/lobby.hpp
    src/rooms/video_sharing_controller.hpp
    src/rooms/video_sharing_controller.cpp
    src/rooms/producer_video_quality_controller.hpp
    src/rooms/producer_video_quality_controller.cpp
    src/utils/statistics.cpp
    src/utils/statistics.hpp
    src/utils/message.cpp
    src/utils/message.hpp
    src/dto/dtos.hpp
    src/dto/config.hpp
)

add_executable(${PROJECT_NAME} ${sources} )

if (IS_BIG_ENDIAN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "MS_BIG_ENDIAN")
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE "MS_LITTLE_ENDIAN")
endif ()


if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_compile_definitions(${PROJECT_NAME} 
        PRIVATE 
        _POSIX_C_SOURCE=200112
        _GNU_SOURCE
    )
ELSEif (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--export-dynamic")
ELSEif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_include_directories(${PROJECT_NAME}
    PRIVATE
    ../deps/getopt/src
)
ENDif (CMAKE_SYSTEM_NAME MATCHES "Linux")

if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -g -gdwarf-2 -Wall -Wendif-labels -W -Wno-unused-parameter -Wall -Wextra -Wno-unused-parameter -std=c++17")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIC")
ENDif ()
message(STATUS "CMAKE_CXX_FLAGS : " ${CMAKE_CXX_FLAGS})

if (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set_property(TARGET ${PROJECT_NAME}
        PROPERTY XCODE_ATTRIBUTE_WARNING_CFLAGS
        -std=c++17
        PROPERTY XCODE_ATTRIBUTE_OTHER_CPLUSPLUSFLAGS
    )
ENDif ()

# Public (interface) headers from dependencies.
target_include_directories(${PROJECT_NAME} PUBLIC
  ${PROJECT_INCLUDE_DIR}
  ${WORKER_INCLUDE_DIR}
  ${CONTROLLER_INCLUDE_DIR}
  ${LIB_WEBRTC_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../deps/json/single_include
  ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libwebrtc/deps/abseil-cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libsdptransform
  ${CMAKE_CURRENT_SOURCE_DIR}/../deps/asio/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../release/include/fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/../deps/stl
)

if (APPLE) 
target_link_libraries(${PROJECT_NAME}
    libwebrtc.a
    libworker.a
    libcontroller.a
    # ${PROJECT_LIB_DIR}/liburing.a
    ${PROJECT_LIB_DIR}/liboatpp-websocket.a
    ${PROJECT_LIB_DIR}/liboatpp.a
    ${PROJECT_LIB_DIR}/liboatpp-openssl.a
    ${PROJECT_LIB_DIR}/libuv.a
    ${PROJECT_LIB_DIR}/libssl.a
    ${PROJECT_LIB_DIR}/libcrypto.a
    ${PROJECT_LIB_DIR}/libsrtp2.a
    ${PROJECT_LIB_DIR}/libusrsctp.a
    ${PROJECT_LIB_DIR}/libabsl_strings.a
    ${PROJECT_LIB_DIR}/libabsl_bad_optional_access.a
    ${PROJECT_LIB_DIR}/libabsl_bad_variant_access.a
    ${PROJECT_LIB_DIR}/libabsl_hashtablez_sampler.a
    ${PROJECT_LIB_DIR}/libabsl_raw_hash_set.a
    ${PROJECT_LIB_DIR}/libabsl_hash.a
    ${PROJECT_LIB_DIR}/libabsl_city.a
    ${PROJECT_LIB_DIR}/libabsl_low_level_hash.a
    ${PROJECT_LIB_DIR}/libabsl_throw_delegate.a
    ${PROJECT_LIB_DIR}/libabsl_strings_internal.a
    ${PROJECT_LIB_DIR}/libabsl_raw_logging_internal.a
    ${PROJECT_LIB_DIR}/libabsl_string_view.a
    # ${PROJECT_LIB_DIR}/libsdptransform.a
    dl
)
else ()
target_link_libraries(${PROJECT_NAME}
    -Wl,--start-group
    ${CMAKE_CURRENT_SOURCE_DIR}/../build/${CMAKE_BUILD_TYPE}/libwebrtc.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../build/${CMAKE_BUILD_TYPE}/libworker.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../build/${CMAKE_BUILD_TYPE}/libcontroller.a
    ${PROJECT_LIB_DIR}/liburing.a
    ${PROJECT_LIB_DIR}/liboatpp-websocket.a
    ${PROJECT_LIB_DIR}/liboatpp.a
    ${PROJECT_LIB_DIR}/liboatpp-openssl.a
    ${PROJECT_LIB_DIR}/libuv.a
    ${PROJECT_LIB_DIR}/libssl.a
    ${PROJECT_LIB_DIR}/libcrypto.a
    ${PROJECT_LIB_DIR}/libsrtp2.a
    ${PROJECT_LIB_DIR}/libusrsctp.a
    ${PROJECT_LIB_DIR}/libabsl_strings.a
    ${PROJECT_LIB_DIR}/libabsl_bad_optional_access.a
    ${PROJECT_LIB_DIR}/libabsl_bad_variant_access.a
    ${PROJECT_LIB_DIR}/libabsl_hashtablez_sampler.a
    ${PROJECT_LIB_DIR}/libabsl_raw_hash_set.a
    ${PROJECT_LIB_DIR}/libabsl_hash.a
    ${PROJECT_LIB_DIR}/libabsl_city.a
    ${PROJECT_LIB_DIR}/libabsl_low_level_hash.a
    ${PROJECT_LIB_DIR}/libabsl_throw_delegate.a
    ${PROJECT_LIB_DIR}/libabsl_strings_internal.a
    ${PROJECT_LIB_DIR}/libabsl_raw_logging_internal.a
    ${PROJECT_LIB_DIR}/libabsl_string_view.a
    # ${PROJECT_LIB_DIR}/libsdptransform.a
    dl
    -Wl,--end-group
)
endif ()

#install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../install/bin)

