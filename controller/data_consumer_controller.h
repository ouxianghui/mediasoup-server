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
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "types.h"
#include "sctp_parameters.h"

namespace srv {

    struct DataConsumerOptions
    {
        /**
         * The id of the DataProducer to consume.
         */
        std::string dataProducerId;

        /**
         * Just if consuming over SCTP.
         * Whether data messages must be received in order. If true the messages will
         * be sent reliably. Defaults to the value in the DataProducer if it has type
         * 'sctp' or to true if it has type 'direct'.
         */
        bool ordered;

        /**
         * Just if consuming over SCTP.
         * When ordered is false indicates the time (in milliseconds) after which a
         * SCTP packet will stop being retransmitted. Defaults to the value in the
         * DataProducer if it has type 'sctp' or unset if it has type 'direct'.
         */
        int64_t maxPacketLifeTime;

        /**
         * Just if consuming over SCTP.
         * When ordered is false indicates the maximum number of times a packet will
         * be retransmitted. Defaults to the value in the DataProducer if it has type
         * 'sctp' or unset if it has type 'direct'.
         */
        int32_t maxRetransmits;

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    struct DataConsumerStat
    {
        std::string type;
        int64_t timestamp;
        std::string label;
        std::string protocol;
        int32_t messagesSent;
        int32_t bytesSent;
        int32_t bufferedAmount;
    };

    void to_json(nlohmann::json& j, const DataConsumerStat& st);
    void from_json(const nlohmann::json& j, DataConsumerStat& st);

    struct DataConsumerInternal
    {
        std::string transportId;
        std::string dataConsumerId;
    };

    struct DataConsumerData
    {
        std::string dataProducerId;
        
        // Options: 'sctp' | 'direct'
        std::string type;
        SctpStreamParameters sctpStreamParameters;
        std::string label;
        std::string protocol;
    };

    void to_json(nlohmann::json& j, const DataConsumerData& st);
    void from_json(const nlohmann::json& j, DataConsumerData& st);

    class Channel;
    class PayloadChannel;

    class DataConsumerController : public std::enable_shared_from_this<DataConsumerController>
    {
    public:
        DataConsumerController(const DataConsumerInternal& internal,
                               const DataConsumerData& data,
                               const std::shared_ptr<Channel>& channel,
                               std::shared_ptr<PayloadChannel> payloadChannel,
                               const nlohmann::json& appData);

        virtual ~DataConsumerController();
        
        void init();
        
        void destroy();
        
        const std::string& id() { return _internal.dataConsumerId; }

        const std::string& dataProducerId() { return _data.dataProducerId; }
        
        bool closed() { return _closed; }

        const std::string& type() { return _data.type; }

        const SctpStreamParameters& sctpStreamParameters() { return _data.sctpStreamParameters; }

        const std::string& label() { return _data.label; }

        const std::string& protocol() { return _data.protocol; }

        void setAppData(const nlohmann::json& data) { _appData = data; }
        
        const nlohmann::json& appData() { return _appData; }
        
        void close();
        
        void onTransportClosed();
        
        nlohmann::json dump();
        
        nlohmann::json getStats();
        
        void setBufferedAmountLowThreshold(int32_t threshold);

        void send(const uint8_t* payload, size_t payloadLen, bool isBinary = false);
        
        int32_t getBufferedAmount();
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, const std::string& event, const std::string& data);
        
        void onPayloadChannel(const std::string& targetId, const std::string& event, const std::string& data, const uint8_t* payload, size_t payloadLen);
        
    public:
        sigslot::signal<> transportCloseSignal;
        
        sigslot::signal<> dataProducerCloseSignal;
        
        sigslot::signal<const uint8_t*, size_t, int32_t> messageSignal;
        
        sigslot::signal<> sctpSendBufferFullSignal;
        
        sigslot::signal<int32_t> bufferedAmountLowSignal;
        
        sigslot::signal<> closeSignal;
        
    private:
        // Internal data.
        DataConsumerInternal _internal;

        // Consumer data.
        DataConsumerData _data;

        // Channel instance.
        std::weak_ptr<Channel> _channel;

        // PayloadChannel instance.
        std::weak_ptr<PayloadChannel> _payloadChannel;

        // Closed flag.
        std::atomic_bool _closed { false };

        // Custom app data.
        nlohmann::json _appData;
        
    };

}
