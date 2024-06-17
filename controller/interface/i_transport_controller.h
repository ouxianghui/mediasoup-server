/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <vector>
#include <atomic>
#include <string>
#include <unordered_map>
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "types.h"
#include "rtp_parameters.h"
#include "sctp_parameters.h"
#include "srtp_parameters.h"

namespace srv {
 
    /**
     * Port range..
     */
    struct TransportPortRange
    {
        /**
         * Lowest port in the range.
         */
        uint16_t min;
        /**
         * Highest port in the range.
         */
        uint16_t max;
    };

    void to_json(nlohmann::json& j, const TransportPortRange& st);
    void from_json(const nlohmann::json& j, TransportPortRange& st);

    //struct TransportListenIp
    //{
    //    /**
    //     * Listening IPv4 or IPv6.
    //     */
    //    std::string ip;

    //    /**
    //     * Announced IPv4 or IPv6 (useful when running mediasoup behind NAT with
    //     * private IP).
    //     */
    //    std::string announcedIp;
    //};

    //void to_json(nlohmann::json& j, const TransportListenIp& st);
    //void from_json(const nlohmann::json& j, TransportListenIp& st);

    struct TransportSocketFlags
    {
        /**
        * Disable dual-stack support so only IPv6 is used (only if ip is IPv6).
        */
        bool ipv6Only = false;
        /**
        * Make different transports bind to the same ip and port (only for UDP).
        * Useful for multicast scenarios with plain transport. Use with caution.
        */
        bool udpReusePort = false;
    };

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
         * @deprecated Use |announcedAddress| instead.
         *
         * Announced IPv4, IPv6 or hostname (useful when running mediasoup behind NAT
         * with private IP).
         */
        std::string announcedIp;

        /**
         * Announced IPv4, IPv6 or hostname (useful when running mediasoup behind NAT
         * with private IP).
         */
        std::string announcedAddress;

        /**
         * Listening port.
         */
        uint16_t port;
        
        /**
         * Listening port range. If given then |port| will be ignored.
         */
        TransportPortRange portRange;
        
        /**
        * Socket flags.
        */
        TransportSocketFlags flags;

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
        std::string localAddress;
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
    class IProducerController;
    class IDataProducerController;
    struct TransportConstructorOptions
    {
        TransportInternal internal;
        std::shared_ptr<TransportData> data;
        std::shared_ptr<Channel> channel;
        nlohmann::json appData;
        std::function<RtpCapabilities()> getRouterRtpCapabilities;
        std::function<std::shared_ptr<IProducerController>(const std::string&)> getProducerController;
        std::function<std::shared_ptr<IDataProducerController>(const std::string&)> getDataProducerController;
    };

    struct ConsumerOptions;
    struct ProducerOptions;
    struct DataProducerOptions;
    struct DataConsumerOptions;

    class IConsumerController;
    class IDataConsumerController;

    class ITransportController
    {
    public:
        virtual ~ITransportController() = default;
        
        virtual void init() = 0;
        
        virtual void destroy() = 0;
        
        virtual const std::string& id() = 0;
        
        virtual void setAppData(const nlohmann::json& data) = 0;
        
        virtual const nlohmann::json& appData() = 0;
        
        virtual void close() = 0;
        
        virtual bool closed() = 0;
        
        virtual std::shared_ptr<BaseTransportDump> dump() = 0;
        
        virtual std::shared_ptr<BaseTransportStats> getStats() = 0;
        
        virtual void connect(const std::shared_ptr<ConnectParams>& params) = 0;
        
        virtual void setMaxIncomingBitrate(int32_t bitrate) = 0;
        
        virtual void setMaxOutgoingBitrate(int32_t bitrate) = 0;
        
        virtual void setMinOutgoingBitrate(int32_t bitrate) = 0;
        
        virtual void enableTraceEvent(const std::vector<std::string>& types) = 0;
        
        virtual void onRouterClosed() = 0;
        
        virtual void onWebRtcServerClosed() = 0;
        
        virtual std::shared_ptr<IProducerController> produce(const std::shared_ptr<ProducerOptions>& options) = 0;
        
        virtual std::shared_ptr<IConsumerController> consume(const std::shared_ptr<ConsumerOptions>& options) = 0;
        
        virtual std::shared_ptr<IDataProducerController> produceData(const std::shared_ptr<DataProducerOptions>& options) = 0;
        
        virtual std::shared_ptr<IDataConsumerController> consumeData(const std::shared_ptr<DataConsumerOptions>& options) = 0;
        
    public:
        sigslot::signal<> routerCloseSignal;
        
        sigslot::signal<> webRtcServerCloseSignal;
        
        sigslot::signal<const std::string&> closeSignal;
        
        sigslot::signal<std::shared_ptr<IProducerController>> producerCloseSignal;
        
        sigslot::signal<std::shared_ptr<IDataProducerController>> dataProducerCloseSignal;
        
        sigslot::signal<const TransportTraceEventData&> traceSignal;
        
        sigslot::signal<std::shared_ptr<IProducerController>> newProducerSignal;
        
        sigslot::signal<std::shared_ptr<IConsumerController>> newConsumerSignal;
        
        sigslot::signal<std::shared_ptr<IDataProducerController>> newDataProducerSignal;
        
        sigslot::signal<std::shared_ptr<IDataConsumerController>> newDataConsumerSignal;
    };
}
