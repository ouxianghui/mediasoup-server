/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <string>
#include "FBS/rtpStream.h"

namespace srv {

    struct BaseRtpStreamStats
    {
        uint64_t timestamp;
        uint32_t ssrc;
        uint32_t rtxSsrc;
        std::string rid;
        std::string kind;
        std::string mimeType;
        uint64_t packetsLost;
        uint32_t fractionLost;
        uint64_t packetsDiscarded;
        uint64_t packetsRetransmitted;
        uint64_t packetsRepaired;
        uint64_t nackCount;
        uint64_t nackPacketCount;
        uint64_t pliCount;
        uint64_t firCount;
        uint8_t score;
        uint64_t roundTripTime;
        uint64_t rtxPacketsDiscarded;
    };

    struct BitrateByLayer
    {
        std::string layer;
        uint32_t bitrate;
    };

    struct RtpStreamRecvStats : BaseRtpStreamStats
    {
        std::string type;
        uint32_t jitter;
        uint64_t packetCount;
        uint64_t byteCount;
        uint64_t bitrate;
        // TODO: using map
        std::vector<std::shared_ptr<BitrateByLayer>> bitrateByLayer;
    };

    struct RtpStreamSendStats : BaseRtpStreamStats
    {
        std::string type;
        uint64_t packetCount;
        uint64_t byteCount;
        uint64_t bitrate;
    };

    struct RtxStreamParameters
    {
        uint32_t ssrc;
        uint8_t payloadType;
        std::string mimeType;
        uint32_t clockRate;
        std::string rrid;
        std::string cname;
    };

    struct RtxStreamDump
    {
        RtxStreamParameters params;
    };

    struct RtpStreamParameters
    {
        size_t encodingIdx;
        uint32_t ssrc;
        uint8_t payloadType;
        std::string mimeType;
        uint32_t clockRate;
        std::string rid;
        std::string cname;
        uint32_t rtxSsrc;
        uint8_t rtxPayloadType;
        bool useNack;
        bool usePli;
        bool useFir;
        bool useInBandFec;
        bool useDtx;
        uint8_t spatialLayers;
        uint8_t temporalLayers;
    };

    struct RtpStreamDump
    {
        RtpStreamParameters params;
        uint8_t score;
        RtxStreamDump rtxStream;
    };

    std::shared_ptr<RtpStreamParameters> parseRtpStreamParameters(const FBS::RtpStream::Params* data);

    std::shared_ptr<RtxStreamParameters> parseRtxStreamParameters(const FBS::RtxStream::Params* data);

    std::shared_ptr<RtxStreamDump> parseRtxStream(const FBS::RtxStream::RtxDump* data);

    std::shared_ptr<RtpStreamDump> parseRtpStream(const FBS::RtpStream::Dump* data);

    std::vector<std::shared_ptr<BitrateByLayer>> parseBitrateByLayer(const FBS::RtpStream::RecvStats* binary);

}
