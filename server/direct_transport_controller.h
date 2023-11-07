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

namespace srv {

    struct DirectTransportOptions
    {
        /**
         * Maximum allowed size for direct messages sent from DataProducers.
         * Default 262144.
         */
        int32_t maxMessageSize = 262144;

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    struct DirectTransportStat
    {
        // Common to all Transports.
        std::string type;
        std::string transportId;
        int64_t timestamp;
        
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
    };

    void to_json(nlohmann::json& j, const DirectTransportStat& st);
    void from_json(const nlohmann::json& j, DirectTransportStat& st);

    struct DirectTransportConstructorOptions : TransportConstructorOptions {};

    class DirectTransportController : public TransportController
    {
    public:
        DirectTransportController(const std::shared_ptr<DirectTransportConstructorOptions>& options);
        
        ~DirectTransportController();
        
        void init();
        
        void destroy();
        
        void close();

        void onRouterClosed();

        nlohmann::json getStats() override;
        
        void connect(const nlohmann::json& data) override;

        void setMaxIncomingBitrate(int32_t bitrate) override;
        
        void setMaxOutgoingBitrate(int32_t bitrate) override;
        
        void setMinOutgoingBitrate(int32_t bitrate) override;
        
        void sendRtcp(const uint8_t* payload, size_t payloadLen);
        
    public:
        sigslot::signal<const uint8_t*, size_t> _rtcpSignal;
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, const std::string& event, const std::string& data);
        
        void onPayloadChannel(const std::string& targetId, const std::string& event, const std::string& data, const uint8_t* payload, size_t payloadLen);
    };

}
