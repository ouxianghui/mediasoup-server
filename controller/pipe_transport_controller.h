/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "types.h"
#include "transport_controller.h"
#include "sctp_parameters.h"
#include "srtp_parameters.h"
#include "rtp_parameters.h"

namespace srv {

    struct PipeTransportOptions
    {
        /**
         * Listening IP address.
         */
        nlohmann::json listenIp;

        /**
         * Fixed port to listen on instead of selecting automatically from Worker's port
         * range.
         */
        int32_t port = 0;

        /**
         * Create a SCTP association. Default false.
         */
        bool enableSctp = false;

        /**
         * SCTP streams number.
         */
        NumSctpStreams numSctpStreams;

        /**
         * Maximum allowed size for SCTP messages sent by DataProducers.
         * Default 268435456.
         */
        int32_t maxSctpMessageSize = 268435456;

        /**
         * Maximum SCTP send buffer used by DataConsumers.
         * Default 268435456.
         */
        int32_t sctpSendBufferSize = 268435456;

        /**
         * Enable RTX and NACK for RTP retransmission. Useful if both Routers are
         * located in different hosts and there is packet lost in the link. For this
         * to work, both PipeTransports must enable this setting. Default false.
         */
        bool enableRtx = false;

        /**
         * Enable SRTP. Useful to protect the RTP and RTCP traffic if both Routers
         * are located in different hosts. For this to work, connect() must be called
         * with remote SRTP parameters. Default false.
         */
        bool enableSrtp = false;

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    struct PipeTransportStat
    {
        // Common to all Transports.
        std::string type;
        std::string transportId;
        int64_t timestamp;
        
        std::string sctpState;
        
        int32_t bytesReceived;
        int32_t recvBitrate;
        int32_t bytesSent;
        int32_t sendBitrate;
        int32_t rtpBytesReceived;
        int32_t rtpRecvBitrate;
        int32_t rtpBytesSent;
        int32_t rtpSendBitrate;
        int32_t rtxBytesReceived;
        int32_t rtxRecvBitrate;
        int32_t rtxBytesSent;
        int32_t rtxSendBitrate;
        int32_t probationBytesSent;
        int32_t probationSendBitrate;
        int32_t availableOutgoingBitrate;
        int32_t availableIncomingBitrate;
        int32_t maxIncomingBitrate;
        
        // PipeTransport specific.
        TransportTuple tuple;
    };

    void to_json(nlohmann::json& j, const PipeTransportStat& st);
    void from_json(const nlohmann::json& j, PipeTransportStat& st);

    struct PipeTransportConstructorOptions : TransportConstructorOptions {};

    class PipeTransportController : public TransportController
    {
    public:
        PipeTransportController(const std::shared_ptr<PipeTransportConstructorOptions>& options);
        
        ~PipeTransportController();
        
        void init();
        
        void destroy();
        
        TransportTuple tuple() { return _data["tuple"]; }

        SctpParameters sctpParameters() { return _data["sctpParameters"]; }

        std::string sctpState() { return _data["sctpState"]; }

        SrtpParameters srtpParameters() { return _data["srtpParameters"]; }
        
        void close();
        
        void onRouterClosed();
        
        nlohmann::json getStats() override;
        
        void connect(const nlohmann::json& data) override;
        
        std::shared_ptr<ConsumerController> consume(const std::shared_ptr<ConsumerOptions>& options) override;
        
    public:
        sigslot::signal<const std::string&> sctpStateChangeSignal;
        
    private:
        void removeConsumerController(const std::string& id);
        
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, const std::string& event, const std::string& data);
    };

}
