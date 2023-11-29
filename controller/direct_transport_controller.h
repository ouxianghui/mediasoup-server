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

    struct DirectTransportDump : BaseTransportDump {};

    struct DirectTransportStat : BaseTransportStats
    {
        std::string type;
    };

    struct DirectTransportData : TransportData
    {
        //sctpParameters?: SctpParameters;
    };

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

        std::shared_ptr<BaseTransportDump> dump() override;
        
        std::shared_ptr<BaseTransportStats> getStats() override;
        
        void connect(const std::shared_ptr<ConnectData>& data) override;

        void setMaxIncomingBitrate(int32_t bitrate) override;
        
        void setMaxOutgoingBitrate(int32_t bitrate) override;
        
        void setMinOutgoingBitrate(int32_t bitrate) override;
        
        void sendRtcp(const std::vector<uint8_t>& data);
        
    public:
        sigslot::signal<const std::vector<uint8_t>&> rtcpSignal;
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data);
    };

    std::shared_ptr<DirectTransportDump> parseDirectTransportDumpResponse(const FBS::DirectTransport::DumpResponse* binary);

    std::shared_ptr<DirectTransportStat> parseGetStatsResponse(const FBS::DirectTransport::GetStatsResponse* binary);
}
