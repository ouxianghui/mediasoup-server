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
#include "producer_controller.h"
#include "consumer_controller.h"
#include "data_producer_controller.h"
#include "data_consumer_controller.h"

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

    void to_json(nlohmann::json& j, const TransportTuple& st);
    void from_json(const nlohmann::json& j, TransportTuple& st);

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
        int64_t timestamp;

        /**
         * Event direction.
         * Options: 'in' | 'out'
         */
        std::string direction;

        /**
         * Per type information.
         */
        nlohmann::json info;
    };

    void to_json(nlohmann::json& j, const TransportTraceEventData& st);
    void from_json(const nlohmann::json& j, TransportTraceEventData& st);

    // export type SctpState = 'new' | 'connecting' | 'connected' | 'failed' | 'closed';

    struct TransportInternal
    {
        std::string routerId;
        std::string transportId;
    };

    class Channel;
    class PayloadChannel;

    struct TransportConstructorOptions
    {
        TransportInternal internal;
        nlohmann::json data;
        std::shared_ptr<Channel> channel;
        std::shared_ptr<PayloadChannel> payloadChannel;
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
        
        nlohmann::json dump();
        
        virtual nlohmann::json getStats();
        
        virtual void connect(const nlohmann::json& data);
        
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
        sigslot::signal<> _routerCloseSignal;
        sigslot::signal<> _listenServerCloseSignal;
        sigslot::signal<const std::string&> _closeSignal;
        
        sigslot::signal<std::shared_ptr<ProducerController>> _producerCloseSignal;
        sigslot::signal<std::shared_ptr<DataProducerController>> _dataProducerCloseSignal;
        
        sigslot::signal<const TransportTraceEventData&> _traceSignal;
        
        sigslot::signal<std::shared_ptr<ProducerController>> _newProducerSignal;
        sigslot::signal<std::shared_ptr<ConsumerController>> _newConsumerSignal;
        
        sigslot::signal<std::shared_ptr<DataProducerController>> _newDataProducerSignal;
        sigslot::signal<std::shared_ptr<DataConsumerController>> _newDataConsumerSignal;
        
    protected:
        // Internal data.
        TransportInternal _internal;

        // Consumer data.
        nlohmann::json _data;

        // Channel instance.
        std::weak_ptr<Channel> _channel;

        // PayloadChannel instance.
        std::weak_ptr<PayloadChannel> _payloadChannel;

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

}
