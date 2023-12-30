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
#include "threadsafe_unordered_map.hpp"
#include "interface/i_transport_controller.h"
#include "FBS/transport.h"
#include "FBS/sctpAssociation.h"
#include "flatbuffers/flatbuffers.h"
#include "ortc.h"

namespace srv {

    struct ConsumerLayers;

    class AbstractTransportController : public ITransportController, public std::enable_shared_from_this<AbstractTransportController>
    {
    public:
        AbstractTransportController(const std::shared_ptr<TransportConstructorOptions>& options);
        
        ~AbstractTransportController();

        const std::string& id() override { return _internal.transportId; }
        
        void setAppData(const nlohmann::json& data) override { _appData = data; }
        
        const nlohmann::json& appData() override { return _appData; }
        
        void close() override;
        
        bool closed() override { return _closed; }
        
        void onRouterClosed() override;
        
        void onWebRtcServerClosed() override;
        
        std::shared_ptr<BaseTransportDump> dump() override;
        
        std::shared_ptr<BaseTransportStats> getStats() override;
        
        void connect(const std::shared_ptr<ConnectParams>& params) override;
    
        void setMaxIncomingBitrate(int32_t bitrate) override;

        void setMaxOutgoingBitrate(int32_t bitrate) override;

        void setMinOutgoingBitrate(int32_t bitrate) override;
        
        void enableTraceEvent(const std::vector<std::string>& types) override;
        
        std::shared_ptr<IProducerController> produce(const std::shared_ptr<ProducerOptions>& options) override;
    
        std::shared_ptr<IConsumerController> consume(const std::shared_ptr<ConsumerOptions>& options) override;
    
        std::shared_ptr<IDataProducerController> produceData(const std::shared_ptr<DataProducerOptions>& options) override;
    
        std::shared_ptr<IDataConsumerController> consumeData(const std::shared_ptr<DataConsumerOptions>& options) override;

    protected:
        int32_t getNextSctpStreamId();
        
        void clearControllers();
  
    protected:
        // Internal data.
        TransportInternal _internal;

        // Consumer data.
        std::shared_ptr<TransportData> _data;

        // Channel instance.
        std::weak_ptr<Channel> _channel;

        // Closed flag.
        std::atomic_bool _closed { false };

        // Custom app data.
        nlohmann::json _appData;
        
        // Method to retrieve Router RTP capabilities.
        std::function<RtpCapabilities()> _getRouterRtpCapabilities;
        
        // Method to retrieve a Producer.
        std::function<std::shared_ptr<IProducerController>(const std::string&)> _getProducerController;
        
        // Method to retrieve a DataProducer.
        std::function<std::shared_ptr<IDataProducerController>(const std::string&)> _getDataProducerController;

        // Producers map.
        std::threadsafe_unordered_map<std::string, std::shared_ptr<IProducerController>> _producerControllers;

        // Consumers map.
        std::threadsafe_unordered_map<std::string, std::shared_ptr<IConsumerController>> _consumerControllers;

        // DataProducers map.
        std::threadsafe_unordered_map<std::string, std::shared_ptr<IDataProducerController>> _dataProducerControllers;

        // DataConsumers map.
        std::threadsafe_unordered_map<std::string, std::shared_ptr<IDataConsumerController>> _dataConsumerControllers;

        // RTCP CNAME for Producers.
        std::string _cnameForProducers;

        // Next MID for Consumers. It's converted into string when used.
        int32_t _nextMidForConsumers = 0;

        // Buffer with available SCTP stream ids.
        std::vector<int32_t> _sctpStreamIds;

        // Next SCTP stream id.
        int32_t _nextSctpStreamId = 0;
    };

    FBS::Transport::TraceEventType transportTraceEventTypeToFbs(const std::string& eventType);

    std::string transportTraceEventTypeFromFbs(FBS::Transport::TraceEventType eventType);

    std::string parseSctpState(FBS::SctpAssociation::SctpState fbsSctpState);

    std::string parseProtocol(FBS::Transport::Protocol protocol);

    FBS::Transport::Protocol serializeProtocol(const std::string& protocol);

    std::shared_ptr<TransportTuple> parseTuple(const FBS::Transport::Tuple* binary);

    std::shared_ptr<BaseTransportDump> parseBaseTransportDump(const FBS::Transport::Dump* binary);

    std::shared_ptr<BaseTransportStats> parseBaseTransportStats(const FBS::Transport::Stats* binary);

    std::shared_ptr<TransportTraceEventData> parseTransportTraceEventData(const FBS::Transport::TraceNotification* trace);

    std::shared_ptr<RecvRtpHeaderExtensions> parseRecvRtpHeaderExtensions(const FBS::Transport::RecvRtpHeaderExtensions* binary);

    std::shared_ptr<BweTraceInfo> parseBweTraceInfo(const FBS::Transport::BweTraceInfo* binary);

    flatbuffers::Offset<FBS::Transport::ConsumeRequest> createConsumeRequest(flatbuffers::FlatBufferBuilder& builder,
                                                                             const std::shared_ptr<IProducerController>& producer,
                                                                             const std::string& consumerId,
                                                                             const RtpParameters& rtpParameters,
                                                                             bool paused,
                                                                             const ConsumerLayers& preferredLayers,
                                                                             bool ignoreDtx,
                                                                             bool pipe);

    flatbuffers::Offset<FBS::Transport::ProduceRequest> createProduceRequest(flatbuffers::FlatBufferBuilder& builder,
                                                                             const std::string& producerId,
                                                                             const std::string& kind,
                                                                             const RtpParameters& rtpParameters,
                                                                             const RtpMappingFbs& rtpMapping,
                                                                             uint32_t keyFrameRequestDelay,
                                                                             bool paused);

    flatbuffers::Offset<FBS::Transport::ConsumeDataRequest> createConsumeDataRequest(flatbuffers::FlatBufferBuilder& builder,
                                                                                     const std::string& dataConsumerId,
                                                                                     const std::string& dataProducerId,
                                                                                     const std::string& type,
                                                                                     const SctpStreamParameters& sctpStreamParameters,
                                                                                     const std::string& label,
                                                                                     const std::string& protocol,
                                                                                     bool paused,
                                                                                     const std::vector<uint16_t>& subchannels);

    flatbuffers::Offset<FBS::Transport::ProduceDataRequest> createProduceDataRequest(flatbuffers::FlatBufferBuilder& builder,
                                                                                     const std::string& dataProducerId,
                                                                                     const std::string& type,
                                                                                     const SctpStreamParameters& sctpStreamParameters,
                                                                                     const std::string& label,
                                                                                     const std::string& protocol,
                                                                                     bool paused);

    std::shared_ptr<RtpListenerDump> parseRtpListenerDump(const FBS::Transport::RtpListener* binary);

    std::shared_ptr<SctpListenerDump> parseSctpListenerDump(const FBS::Transport::SctpListener* binary);

}
