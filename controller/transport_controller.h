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
#include <string>
#include <unordered_map>
#include <assert.h>
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "types.h"
#include "rtp_parameters.h"
#include "sctp_parameters.h"
#include "srtp_parameters.h"
#include "producer_controller.h"
#include "consumer_controller.h"
#include "data_producer_controller.h"
#include "data_consumer_controller.h"
#include "FBS/transport.h"
#include "FBS/sctpAssociation.h"
#include "flatbuffers/flatbuffers.h"

namespace srv {

    struct TransportListenIp
    {
        /**
         * Listening IPv4 or IPv6.
         */
        std::string ip;

        /**
         * Announced IPv4 or IPv6 (useful when running mediasoup behind NAT with
         * private IP).
         */
        std::string announcedIp;
    };

    void to_json(nlohmann::json& j, const TransportListenIp& st);
    void from_json(const nlohmann::json& j, TransportListenIp& st);

    struct TransportListenInfo
    {
        /**
         * Network protocol.
         */
        std::string protocol;

        /**
         * Listening IPv4 or IPv6.
         */
        std::string ip;

        /**
         * Announced IPv4 or IPv6 (useful when running mediasoup behind NAT with
         * private IP).
         */
        std::string announcedIp;

        /**
         * Listening port.
         */
        uint16_t port;

        /**
         * Send buffer size (bytes).
         */
        uint32_t sendBufferSize = 0;

        /**
         * Recv buffer size (bytes).
         */
        uint32_t recvBufferSize = 0;
    };

    void to_json(nlohmann::json& j, const TransportListenInfo& st);
    void from_json(const nlohmann::json& j, TransportListenInfo& st);

    /**
     * Transport protocol.
     */
    // export type TransportProtocol = 'udp' | 'tcp';

    struct TransportTuple
    {
        std::string localIp;
        int32_t localPort;
        std::string remoteIp;
        int32_t remotePort;
        
        // Options: 'udp' | 'tcp'
        std::string protocol;
    };

    class TransportTraceInfo
    {
    public:
        virtual ~TransportTraceInfo() = default;
    };

    class ProbationTraceInfo : public TransportTraceInfo
    {
    public:
    };

    class BweTraceInfo : public TransportTraceInfo
    {
    public:
        std::string bweType;
        uint32_t desiredBitrate;
        uint32_t effectiveDesiredBitrate;
        uint32_t minBitrate;
        uint32_t maxBitrate;
        uint32_t startBitrate;
        uint32_t maxPaddingBitrate;
        uint32_t availableBitrate;
    };

    /**
     * Valid types for 'trace' event.
     */
    // export type TransportTraceEventType = 'probation' | 'bwe';

    /**
     * 'trace' event data.
     */
    struct TransportTraceEventData
    {
        /**
         * Trace type.
         * Options: 'probation' | 'bwe'
         */
        std::string type;

        /**
         * Event timestamp.
         */
        uint64_t timestamp;

        /**
         * Event direction.
         * Options: 'in' | 'out'
         */
        std::string direction;

        /**
         * Per type information.
         */
        std::shared_ptr<TransportTraceInfo> info;
    };

    void to_json(nlohmann::json& j, const TransportTraceEventData& st);
    void from_json(const nlohmann::json& j, TransportTraceEventData& st);

    struct RtpListenerDump
    {
        // Table of SSRC / Producer pairs.
        std::unordered_map<uint32_t, std::string> ssrcTable;
        //  Table of MID / Producer pairs.
        std::unordered_map<std::string, std::string> midTable;
        //  Table of RID / Producer pairs.
        std::unordered_map<std::string, std::string> ridTable;
        
    };

    struct SctpListenerDump
    {
        std::unordered_map<uint16_t, std::string> streamIdTable;
    };

    struct RecvRtpHeaderExtensions
    {
        uint8_t mid;
        uint8_t rid;
        uint8_t rrid;
        uint32_t absSendTime;
        uint8_t transportWideCc01;
    };

    struct BaseTransportDump
    {
        std::string id;
        bool direct;
        std::vector<std::string> producerIds;
        std::vector<std::string> consumerIds;
        std::vector<std::pair<uint32_t, std::string>> mapSsrcConsumerId;
        std::vector<std::pair<uint32_t, std::string>> mapRtxSsrcConsumerId;
        RecvRtpHeaderExtensions recvRtpHeaderExtensions;
        RtpListenerDump rtpListener;
        std::size_t maxMessageSize;
        std::vector<std::string> dataProducerIds;
        std::vector<std::string> dataConsumerIds;
        SctpParameters sctpParameters;
        std::string sctpState;
        SctpListenerDump sctpListener;
        std::vector<std::string> traceEventTypes;
    };

    struct BaseTransportStats
    {
        std::string transportId;
        uint64_t timestamp;
        std::string sctpState;
        std::size_t bytesReceived;
        uint32_t recvBitrate;
        std::size_t bytesSent;
        uint32_t sendBitrate;
        std::size_t rtpBytesReceived;
        uint32_t rtpRecvBitrate;
        std::size_t rtpBytesSent;
        uint32_t rtpSendBitrate;
        std::size_t rtxBytesReceived;
        uint32_t rtxRecvBitrate;
        std::size_t rtxBytesSent;
        uint32_t rtxSendBitrate;
        std::size_t probationBytesSent;
        uint32_t probationSendBitrate;
        uint32_t availableOutgoingBitrate;
        uint32_t availableIncomingBitrate;
        uint32_t maxIncomingBitrate;
    };

    void to_json(nlohmann::json& j, const BaseTransportStats& st);
    void from_json(const nlohmann::json& j, BaseTransportStats& st);

    /**
     * The hash function algorithm (as defined in the "Hash function Textual Names"
     * registry initially specified in RFC 4572 Section 8) and its corresponding
     * certificate fingerprint value (in lowercase hex string as expressed utilizing
     * the syntax of "fingerprint" in RFC 4572 Section 5).
     */
    struct DtlsFingerprint
    {
        std::string algorithm;
        std::string value;
    };

    void to_json(nlohmann::json& j, const DtlsFingerprint& st);
    void from_json(const nlohmann::json& j, DtlsFingerprint& st);
    
    struct DtlsParameters
    {
        // DtlsRole, Options: 'auto' | 'client' | 'server'
        std::string role;
        
        std::vector<DtlsFingerprint> fingerprints;
    };

    void to_json(nlohmann::json& j, const DtlsParameters& st);
    void from_json(const nlohmann::json& j, DtlsParameters& st);

    struct ConnectParams
    {
        std::string ip;
        uint16_t port;
        uint16_t rtcpPort;
        SrtpParameters srtpParameters;
        DtlsParameters dtlsParameters;
    };

    // export type SctpState = 'new' | 'connecting' | 'connected' | 'failed' | 'closed';

    struct TransportInternal
    {
        std::string routerId;
        std::string transportId;
    };

    class TransportData {
    public:
        virtual ~TransportData() = default;
        SctpParameters sctpParameters;
    };

    class Channel;

    struct TransportConstructorOptions
    {
        TransportInternal internal;
        std::shared_ptr<TransportData> data;
        std::shared_ptr<Channel> channel;
        nlohmann::json appData;
        std::function<RtpCapabilities()> getRouterRtpCapabilities;
        std::function<std::shared_ptr<ProducerController>(const std::string&)> getProducerController;
        std::function<std::shared_ptr<DataProducerController>(const std::string&)> getDataProducerController;
    };

    class TransportController : public std::enable_shared_from_this<TransportController>
    {
    public:
        TransportController(const std::shared_ptr<TransportConstructorOptions>& options);
        
        virtual ~TransportController();
        
        const std::string& id() { return _internal.transportId; }
        
        bool closed() { return _closed; }
        
        void setAppData(const nlohmann::json& data) { _appData = data; }
        
        const nlohmann::json& appData() { return _appData; }
        
        void close();
        
        void onRouterClosed();
        
        void onListenServerClosed();
        
        virtual std::shared_ptr<BaseTransportDump> dump();
        
        virtual std::shared_ptr<BaseTransportStats> getStats();
        
        virtual void connect(const std::shared_ptr<ConnectParams>& data);
        
        virtual void setMaxIncomingBitrate(int32_t bitrate);

        virtual void setMaxOutgoingBitrate(int32_t bitrate);

        virtual void setMinOutgoingBitrate(int32_t bitrate);
        
        void enableTraceEvent(const std::vector<std::string>& types);
        
        virtual std::shared_ptr<ProducerController> produce(const std::shared_ptr<ProducerOptions>& options);
        
        virtual std::shared_ptr<ConsumerController> consume(const std::shared_ptr<ConsumerOptions>& options);
        
        virtual std::shared_ptr<DataProducerController> produceData(const std::shared_ptr<DataProducerOptions>& options);
        
        virtual std::shared_ptr<DataConsumerController> consumeData(const std::shared_ptr<DataConsumerOptions>& options);

    protected:
        int32_t getNextSctpStreamId();
        
        void clearControllers();
        
    public:
        sigslot::signal<> routerCloseSignal;
        
        sigslot::signal<> listenServerCloseSignal;
        
        sigslot::signal<const std::string&> closeSignal;
        
        sigslot::signal<std::shared_ptr<ProducerController>> producerCloseSignal;
        
        sigslot::signal<std::shared_ptr<DataProducerController>> dataProducerCloseSignal;
        
        sigslot::signal<const TransportTraceEventData&> traceSignal;
        
        sigslot::signal<std::shared_ptr<ProducerController>> newProducerSignal;
        
        sigslot::signal<std::shared_ptr<ConsumerController>> newConsumerSignal;
        
        sigslot::signal<std::shared_ptr<DataProducerController>> newDataProducerSignal;
        
        sigslot::signal<std::shared_ptr<DataConsumerController>> newDataConsumerSignal;
        
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
        std::function<std::shared_ptr<ProducerController>(const std::string&)> _getProducerController;
        
        // Method to retrieve a DataProducer.
        std::function<std::shared_ptr<DataProducerController>(const std::string&)> _getDataProducerController;

        std::mutex _producersMutex;
        // Producers map.
        std::unordered_map<std::string, std::shared_ptr<ProducerController>> _producerControllers;

        std::mutex _consumersMutex;
        // Consumers map.
        std::unordered_map<std::string, std::shared_ptr<ConsumerController>> _consumerControllers;

        std::mutex _dataProducersMutex;
        // DataProducers map.
        std::unordered_map<std::string, std::shared_ptr<DataProducerController>> _dataProducerControllers;

        std::mutex _dataConsumersMutex;
        // DataConsumers map.
        std::unordered_map<std::string, std::shared_ptr<DataConsumerController>> _dataConsumerControllers;

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
                                                                             const std::shared_ptr<ProducerController>& producer,
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
