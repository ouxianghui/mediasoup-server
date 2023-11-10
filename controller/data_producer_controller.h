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

    struct DataProducerOptions
    {
        /**
         * DataProducer id (just for Router.pipeToRouter() method).
         */
        std::string id;

        /**
         * SCTP parameters defining how the endpoint is sending the data.
         * Just if messages are sent over SCTP.
         */
        SctpStreamParameters sctpStreamParameters;

        /**
         * A label which can be used to distinguish this DataChannel from others.
         */
        std::string label;

        /**
         * Name of the sub-protocol used by this DataChannel.
         */
        std::string protocol;

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    struct DataProducerStat
    {
        std::string type;
        int64_t timestamp;
        std::string label;
        std::string protocol;
        int32_t messagesReceived;
        int32_t bytesReceived;
    };

    void to_json(nlohmann::json& j, const DataProducerStat& st);
    void from_json(const nlohmann::json& j, DataProducerStat& st);

    struct DataProducerInternal
    {
        std::string transportId;
        std::string dataProducerId;
    };

    struct DataProducerData
    {
        // Options: 'sctp' | 'direct'
        std::string type;
        SctpStreamParameters sctpStreamParameters;
        std::string label;
        std::string protocol;
    };

    class Channel;
    class PayloadChannel;

    class DataProducerController : public std::enable_shared_from_this<DataProducerController>
    {
    public:
        DataProducerController(const DataProducerInternal& internal,
                               const DataProducerData& data,
                               const std::shared_ptr<Channel>& channel,
                               std::shared_ptr<PayloadChannel> payloadChannel,
                               const nlohmann::json& appData);
        
        virtual ~DataProducerController();
        
        void init();
        
        void destroy();
        
        const std::string& id() { return _internal.dataProducerId; }

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

        void send(const uint8_t* payload, size_t payloadLen, bool isBinary = false);
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, const std::string& event, const std::string& data);
        
        void onPayloadChannel(const std::string& targetId, const std::string& event, const std::string& data, const uint8_t* payload, size_t payloadLen);
        
    public:
        sigslot::signal<> transportCloseSignal;
        
        sigslot::signal<> closeSignal;
        
    private:
        // Internal data.
        DataProducerInternal _internal;

        // Consumer data.
        DataProducerData _data;

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
