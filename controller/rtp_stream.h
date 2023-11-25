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

namespace srv {

    struct BaseRtpStreamStats
    {
        uint32_t timestamp;
        uint32_t ssrc;
        uint32_t rtxSsrc;
        std::string rid;
        std::string kind;
        std::string mimeType;
        uint32_t packetsLost;
        uint32_t fractionLost;
        uint32_t packetsDiscarded;
        uint32_t packetsRetransmitted;
        uint32_t packetsRepaired;
        uint32_t nackCount;
        uint32_t nackPacketCount;
        uint32_t pliCount;
        uint32_t firCount;
        uint32_t score;
        uint32_t roundTripTime;
        uint32_t rtxPacketsDiscarded;
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
        uint32_t packetCount;
        uint32_t byteCount;
        uint32_t bitrate;
        BitrateByLayer bitrateByLayer;
    };

    struct RtpStreamSendStats : BaseRtpStreamStats
    {
        std::string type;
        uint32_t packetCount;
        uint32_t byteCount;
        uint32_t bitrate;
    };

}
