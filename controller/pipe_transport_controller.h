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
#include "abstract_transport_controller.h"
#include "sctp_parameters.h"
#include "srtp_parameters.h"
#include "rtp_parameters.h"
#include "FBS/notification.h"

namespace srv {

    struct PipeTransportOptions
    {
        /**
         * Listening info.
         */
        TransportListenInfo listenInfo;
        
        ///**
        // * Listening IP address.
        // */
        //TransportListenIp listenIp;
        
        /**
         * Fixed port to listen on instead of selecting automatically from Worker's port
         * range.
         */
        uint16_t port;

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

    struct PipeTransportDump : BaseTransportDump
    {
        TransportTuple tuple;
        bool rtx;
        SrtpParameters srtpParameters;
    };

    struct PipeTransportStat : BaseTransportStats
    {
        std::string type;
        TransportTuple tuple;
    };

    class PipeTransportData : public TransportData
    {
    public:
        TransportTuple tuple;
        //SctpParameters sctpParameters;
        std::string sctpState;
        bool rtx;
        SrtpParameters srtpParameters;
    };

    struct PipeTransportConstructorOptions : TransportConstructorOptions {};

    struct ConsumerOptions;
    class IProducerController;
    class IConsumerController;

    class PipeTransportController : public AbstractTransportController
    {
    public:
        PipeTransportController(const std::shared_ptr<PipeTransportConstructorOptions>& options);
        
        ~PipeTransportController();
        
        void init() override;
        
        void destroy() override;
        
        TransportTuple tuple() { return transportData()->tuple; }

        SctpParameters sctpParameters() { return transportData()->sctpParameters; }

        std::string sctpState() { return transportData()->sctpState; }

        SrtpParameters srtpParameters() { return transportData()->srtpParameters; }
        
        void close() override;
        
        void onRouterClosed() override;
    
        std::shared_ptr<BaseTransportStats> getStats() override;
        
        void connect(const std::shared_ptr<ConnectParams>& params) override;
        
        std::shared_ptr<IConsumerController> consume(const std::shared_ptr<ConsumerOptions>& options) override;
        
    public:
        sigslot::signal<const std::string&> sctpStateChangeSignal;
        
    private:
        void removeConsumerController(const std::string& id);
        
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data);
        
        std::shared_ptr<PipeTransportData> transportData() { return std::dynamic_pointer_cast<PipeTransportData>(this->_data); }
    };

    std::shared_ptr<PipeTransportDump> parsePipeTransportDumpResponse(const FBS::PipeTransport::DumpResponse* binary);

    std::shared_ptr<PipeTransportStat> parseGetStatsResponse(const FBS::PipeTransport::GetStatsResponse* binary);

    flatbuffers::Offset<FBS::Transport::ConsumeRequest> createConsumeRequest(flatbuffers::FlatBufferBuilder& builder,
                                                                             const std::string& consumerId,
                                                                             std::shared_ptr<IProducerController> producer,
                                                                             const RtpParameters& rtpParameters);

}
