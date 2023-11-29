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
#include "FBS/dataProducer.h"

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
         * Whether the data producer must start in paused mode. Default false.
         */
        bool paused = false;

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    struct DataProducerStat
    {
        std::string type;
        uint64_t timestamp;
        std::string label;
        std::string protocol;
        uint64_t messagesReceived;
        uint64_t bytesReceived;
    };

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

    struct DataProducerDump : DataProducerData
    {
        std::string id;
        bool paused;
    };

    class Channel;

    class DataProducerController : public std::enable_shared_from_this<DataProducerController>
    {
    public:
        DataProducerController(const DataProducerInternal& internal,
                               const DataProducerData& data,
                               const std::shared_ptr<Channel>& channel,
                               bool paused,
                               const nlohmann::json& appData);
        
        virtual ~DataProducerController();
        
        void init();
        
        void destroy();
        
        const std::string& id() { return _internal.dataProducerId; }

        bool closed() { return _closed; }
        
        bool paused() { return _paused; }

        const std::string& type() { return _data.type; }

        const SctpStreamParameters& sctpStreamParameters() { return _data.sctpStreamParameters; }

        const std::string& label() { return _data.label; }

        const std::string& protocol() { return _data.protocol; }

        void setAppData(const nlohmann::json& data) { _appData = data; }
        
        const nlohmann::json& appData() { return _appData; }
        
        void pause();
        
        void resume();
        
        void close();
        
        void onTransportClosed();
        
        std::shared_ptr<DataProducerDump> dump();
        
        std::vector<std::shared_ptr<DataProducerStat>> getStats();

        void send(const std::vector<uint8_t>& data, const std::vector<uint16_t>& subchannels, uint16_t requiredSubchannel, bool isBinary = false);
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, const std::string& event, const std::string& data) {}
        
    public:
        sigslot::signal<> transportCloseSignal;
        
        sigslot::signal<> closeSignal;
        
        sigslot::signal<> pauseSignal;
        
        sigslot::signal<> resumeSignal;
        
    private:
        // Internal data.
        DataProducerInternal _internal;

        // Consumer data.
        DataProducerData _data;

        // Channel instance.
        std::weak_ptr<Channel> _channel;

        // Closed flag.
        std::atomic_bool _closed { false };
        
        std::atomic_bool _paused = { false };

        // Custom app data.
        nlohmann::json _appData;
    };

    FBS::DataProducer::Type dataProducerTypeToFbs(const std::string& type);

    std::string dataProducerTypeFromFbs(FBS::DataProducer::Type type);

    std::shared_ptr<DataProducerDump> parseDataProducerDumpResponse(const FBS::DataProducer::DumpResponse* data);

    std::shared_ptr<DataProducerStat> parseDataProducerStats(const FBS::DataProducer::GetStatsResponse* binary);


}
