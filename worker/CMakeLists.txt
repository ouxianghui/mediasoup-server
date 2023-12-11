cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME "worker")
project(${PROJECT_NAME})

include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)

IF(NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
ENDIF()

message("\n=========== libwebrtc Build Configuration ===========\n")
message(STATUS "MAIN_DIR : " ${MAIN_DIR})
message(STATUS "PROJECT_INCLUDE_DIR : " ${PROJECT_INCLUDE_DIR})
message(STATUS "PROJECT_LIB_DIR : " ${PROJECT_LIB_DIR})
message(STATUS "LIB_WEBRTC_INCLUDE_DIR : " ${LIB_WEBRTC_INCLUDE_DIR})
message(STATUS "CMAKE_INSTALL_PREFIX : " ${CMAKE_INSTALL_PREFIX})

message("")

add_definitions(-DMS_LIBURING_SUPPORTED)

# C++ standard requirements.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(mediasoup_worker_headers
"include/Channel/ChannelNotification.hpp"
"include/Channel/ChannelNotifier.hpp"
"include/Channel/ChannelRequest.hpp"
"include/Channel/ChannelSocket.hpp"
"include/ChannelMessageRegistrator.hpp"
"include/DepLibSRTP.hpp"
"include/DepLibUring.hpp"
"include/DepLibUV.hpp"
"include/DepLibWebRTC.hpp"
"include/DepOpenSSL.hpp"
"include/DepUsrSCTP.hpp"
"include/LogLevel.hpp"
"include/Logger.hpp"
"include/MediaSoupErrors.hpp"
"include/RTC/ActiveSpeakerObserver.hpp"
"include/RTC/AudioLevelObserver.hpp"
"include/RTC/BweType.hpp"
"include/RTC/Codecs/H264.hpp"
"include/RTC/Codecs/H264_SVC.hpp"
"include/RTC/Codecs/Opus.hpp"
"include/RTC/Codecs/PayloadDescriptorHandler.hpp"
"include/RTC/Codecs/Tools.hpp"
"include/RTC/Codecs/VP8.hpp"
"include/RTC/Codecs/VP9.hpp"
"include/RTC/Consumer.hpp"
"include/RTC/DataConsumer.hpp"
"include/RTC/DataProducer.hpp"
"include/RTC/DirectTransport.hpp"
"include/RTC/DtlsTransport.hpp"
"include/RTC/IceCandidate.hpp"
"include/RTC/IceServer.hpp"
"include/RTC/KeyFrameRequestManager.hpp"
"include/RTC/NackGenerator.hpp"
"include/RTC/Parameters.hpp"
"include/RTC/PipeConsumer.hpp"
"include/RTC/PipeTransport.hpp"
"include/RTC/PlainTransport.hpp"
"include/RTC/PortManager.hpp"
"include/RTC/Producer.hpp"
"include/RTC/RTCP/Bye.hpp"
"include/RTC/RTCP/CompoundPacket.hpp"
"include/RTC/RTCP/Feedback.hpp"
"include/RTC/RTCP/FeedbackItem.hpp"
"include/RTC/RTCP/FeedbackPs.hpp"
"include/RTC/RTCP/FeedbackPsAfb.hpp"
"include/RTC/RTCP/FeedbackPsFir.hpp"
"include/RTC/RTCP/FeedbackPsLei.hpp"
"include/RTC/RTCP/FeedbackPsPli.hpp"
"include/RTC/RTCP/FeedbackPsRemb.hpp"
"include/RTC/RTCP/FeedbackPsRpsi.hpp"
"include/RTC/RTCP/FeedbackPsSli.hpp"
"include/RTC/RTCP/FeedbackPsTst.hpp"
"include/RTC/RTCP/FeedbackPsVbcm.hpp"
"include/RTC/RTCP/FeedbackRtp.hpp"
"include/RTC/RTCP/FeedbackRtpEcn.hpp"
"include/RTC/RTCP/FeedbackRtpNack.hpp"
"include/RTC/RTCP/FeedbackRtpSrReq.hpp"
"include/RTC/RTCP/FeedbackRtpTllei.hpp"
"include/RTC/RTCP/FeedbackRtpTmmb.hpp"
"include/RTC/RTCP/FeedbackRtpTransport.hpp"
"include/RTC/RTCP/Packet.hpp"
"include/RTC/RTCP/ReceiverReport.hpp"
"include/RTC/RTCP/Sdes.hpp"
"include/RTC/RTCP/SenderReport.hpp"
"include/RTC/RTCP/XR.hpp"
"include/RTC/RTCP/XrDelaySinceLastRr.hpp"
"include/RTC/RTCP/XrReceiverReferenceTime.hpp"
"include/RTC/RateCalculator.hpp"
"include/RTC/Router.hpp"
"include/RTC/RtcLogger.hpp"
"include/RTC/RtpDictionaries.hpp"
"include/RTC/RtpHeaderExtensionIds.hpp"
"include/RTC/RtpListener.hpp"
"include/RTC/RtpObserver.hpp"
"include/RTC/RtpPacket.hpp"
"include/RTC/RtpProbationGenerator.hpp"
"include/RTC/RtpRetransmissionBuffer.hpp"
"include/RTC/RtpStream.hpp"
"include/RTC/RtpStreamRecv.hpp"
"include/RTC/RtpStreamSend.hpp"
"include/RTC/RtxStream.hpp"
"include/RTC/SctpAssociation.hpp"
"include/RTC/SctpDictionaries.hpp"
"include/RTC/SctpListener.hpp"
"include/RTC/SenderBandwidthEstimator.hpp"
"include/RTC/SeqManager.hpp"
"include/RTC/Shared.hpp"
"include/RTC/SimpleConsumer.hpp"
"include/RTC/SimulcastConsumer.hpp"
"include/RTC/SrtpSession.hpp"
"include/RTC/StunPacket.hpp"
"include/RTC/SvcConsumer.hpp"
"include/RTC/TcpConnection.hpp"
"include/RTC/TcpServer.hpp"
"include/RTC/Transport.hpp"
"include/RTC/TransportCongestionControlClient.hpp"
"include/RTC/TransportCongestionControlServer.hpp"
"include/RTC/TransportTuple.hpp"
"include/RTC/TrendCalculator.hpp"
"include/RTC/UdpSocket.hpp"
"include/RTC/WebRtcServer.hpp"
"include/RTC/WebRtcTransport.hpp"
"include/Settings.hpp"
"include/Utils.hpp"
"include/Worker.hpp"
"include/common.hpp"
"include/handles/SignalHandle.hpp"
"include/handles/TcpConnectionHandle.hpp"
"include/handles/TcpServerHandle.hpp"
"include/handles/TimerHandle.hpp"
"include/handles/UdpSocketHandle.hpp"
"include/handles/UnixStreamSocketHandle.hpp"
"include/lib.hpp"
)

set(mediasoup_worker_sources
"src/lib.cpp"
"src/DepLibSRTP.cpp"
"src/DepLibUV.cpp"
"src/DepLibUring.cpp"
"src/DepLibWebRTC.cpp"
"src/DepOpenSSL.cpp"
"src/DepUsrSCTP.cpp"
"src/Logger.cpp"
"src/MediaSoupErrors.cpp"
"src/Settings.cpp"
"src/Worker.cpp"
"src/ChannelMessageRegistrator.cpp"
"src/Utils/Crypto.cpp"
"src/Utils/File.cpp"
"src/Utils/IP.cpp"
"src/Utils/String.cpp"
"src/handles/SignalHandle.cpp"
"src/handles/TcpConnectionHandle.cpp"
"src/handles/TcpServerHandle.cpp"
"src/handles/TimerHandle.cpp"
"src/handles/UdpSocketHandle.cpp"
"src/handles/UnixStreamSocketHandle.cpp"
"src/Channel/ChannelNotifier.cpp"
"src/Channel/ChannelRequest.cpp"
"src/Channel/ChannelNotification.cpp"
"src/Channel/ChannelSocket.cpp"
"src/RTC/ActiveSpeakerObserver.cpp"
"src/RTC/AudioLevelObserver.cpp"
"src/RTC/Consumer.cpp"
"src/RTC/DataConsumer.cpp"
"src/RTC/DataProducer.cpp"
"src/RTC/DirectTransport.cpp"
"src/RTC/DtlsTransport.cpp"
"src/RTC/IceCandidate.cpp"
"src/RTC/IceServer.cpp"
"src/RTC/KeyFrameRequestManager.cpp"
"src/RTC/NackGenerator.cpp"
"src/RTC/PipeConsumer.cpp"
"src/RTC/PipeTransport.cpp"
"src/RTC/PlainTransport.cpp"
"src/RTC/PortManager.cpp"
"src/RTC/Producer.cpp"
"src/RTC/RateCalculator.cpp"
"src/RTC/Router.cpp"
"src/RTC/RtcLogger.cpp"
"src/RTC/RtpListener.cpp"
"src/RTC/RtpObserver.cpp"
"src/RTC/RtpPacket.cpp"
"src/RTC/RtpProbationGenerator.cpp"
"src/RTC/RtpRetransmissionBuffer.cpp"
"src/RTC/RtpStream.cpp"
"src/RTC/RtpStreamRecv.cpp"
"src/RTC/RtpStreamSend.cpp"
"src/RTC/RtxStream.cpp"
"src/RTC/SctpAssociation.cpp"
"src/RTC/SctpListener.cpp"
"src/RTC/SenderBandwidthEstimator.cpp"
"src/RTC/SeqManager.cpp"
"src/RTC/Shared.cpp"
"src/RTC/SimpleConsumer.cpp"
"src/RTC/SimulcastConsumer.cpp"
"src/RTC/SrtpSession.cpp"
"src/RTC/StunPacket.cpp"
"src/RTC/SvcConsumer.cpp"
"src/RTC/TcpConnection.cpp"
"src/RTC/TcpServer.cpp"
"src/RTC/Transport.cpp"
"src/RTC/TransportCongestionControlClient.cpp"
"src/RTC/TransportCongestionControlServer.cpp"
"src/RTC/TransportTuple.cpp"
"src/RTC/TrendCalculator.cpp"
"src/RTC/UdpSocket.cpp"
"src/RTC/WebRtcServer.cpp"
"src/RTC/WebRtcTransport.cpp"
"src/RTC/Codecs/H264.cpp"
"src/RTC/Codecs/H264_SVC.cpp"
"src/RTC/Codecs/VP8.cpp"
"src/RTC/Codecs/VP9.cpp"
"src/RTC/Codecs/Opus.cpp"
"src/RTC/RtpDictionaries/Parameters.cpp"
"src/RTC/RtpDictionaries/RtcpFeedback.cpp"
"src/RTC/RtpDictionaries/RtcpParameters.cpp"
"src/RTC/RtpDictionaries/RtpCodecMimeType.cpp"
"src/RTC/RtpDictionaries/RtpCodecParameters.cpp"
"src/RTC/RtpDictionaries/RtpEncodingParameters.cpp"
"src/RTC/RtpDictionaries/RtpHeaderExtensionParameters.cpp"
"src/RTC/RtpDictionaries/RtpHeaderExtensionUri.cpp"
"src/RTC/RtpDictionaries/RtpParameters.cpp"
"src/RTC/RtpDictionaries/RtpRtxParameters.cpp"
"src/RTC/SctpDictionaries/SctpStreamParameters.cpp"
"src/RTC/RTCP/Packet.cpp"
"src/RTC/RTCP/CompoundPacket.cpp"
"src/RTC/RTCP/SenderReport.cpp"
"src/RTC/RTCP/ReceiverReport.cpp"
"src/RTC/RTCP/Sdes.cpp"
"src/RTC/RTCP/Bye.cpp"
"src/RTC/RTCP/Feedback.cpp"
"src/RTC/RTCP/FeedbackPs.cpp"
"src/RTC/RTCP/FeedbackRtp.cpp"
"src/RTC/RTCP/FeedbackRtpNack.cpp"
"src/RTC/RTCP/FeedbackRtpTmmb.cpp"
"src/RTC/RTCP/FeedbackRtpSrReq.cpp"
"src/RTC/RTCP/FeedbackRtpTllei.cpp"
"src/RTC/RTCP/FeedbackRtpEcn.cpp"
"src/RTC/RTCP/FeedbackRtpTransport.cpp"
"src/RTC/RTCP/FeedbackPsPli.cpp"
"src/RTC/RTCP/FeedbackPsSli.cpp"
"src/RTC/RTCP/FeedbackPsRpsi.cpp"
"src/RTC/RTCP/FeedbackPsFir.cpp"
"src/RTC/RTCP/FeedbackPsTst.cpp"
"src/RTC/RTCP/FeedbackPsVbcm.cpp"
"src/RTC/RTCP/FeedbackPsLei.cpp"
"src/RTC/RTCP/FeedbackPsAfb.cpp"
"src/RTC/RTCP/FeedbackPsRemb.cpp"
"src/RTC/RTCP/XR.cpp"
"src/RTC/RTCP/XrDelaySinceLastRr.cpp"
"src/RTC/RTCP/XrReceiverReferenceTime.cpp"
)

add_library(${PROJECT_NAME}
    ${mediasoup_worker_sources}
    ${mediasoup_worker_headers}
)

if(IS_BIG_ENDIAN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "MS_BIG_ENDIAN")
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE "MS_LITTLE_ENDIAN")
endif()

IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_compile_definitions(${PROJECT_NAME} 
        PRIVATE 
        _POSIX_C_SOURCE=200112
        _GNU_SOURCE
    )
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--export-dynamic")
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_include_directories(${PROJECT_NAME}
    PRIVATE
    ../deps/getopt/src
)
ELSE()

ENDIF(CMAKE_SYSTEM_NAME MATCHES "Linux")

IF(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -g -gdwarf-2 -Wall -Wendif-labels -W -Wno-unused-parameter -Wall -Wextra -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIC")
ENDIF()

message(STATUS "CMAKE_CXX_FLAGS : " ${CMAKE_CXX_FLAGS})

IF(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set_property(TARGET ${PROJECT_NAME}
        PROPERTY XCODE_ATTRIBUTE_WARNING_CFLAGS
        -std=c++17
        PROPERTY XCODE_ATTRIBUTE_OTHER_CPLUSPLUSFLAGS
    )
ENDIF()

target_include_directories(${PROJECT_NAME}
    PRIVATE
    include
    ../deps/json/single_include/nlohmann
    ../release/include/fbs
    ../release/include/srtp2
)

# Public (interface) headers from dependencies.
target_include_directories(${PROJECT_NAME} PUBLIC
	${PROJECT_INCLUDE_DIR}
  ${LIB_WEBRTC_INCLUDE_DIR}
  ${LIB_WEBRTC_INCLUDE_DIR}/libwebrtc
  ${LIB_WEBRTC_INCLUDE_DIR}/../abseil-cpp
)

LINK_DIRECTORIES(
	${PROJECT_LIB_DIR}
)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
#install(FILES ${mediasoup_worker_headers} DESTINATION include/worker)


