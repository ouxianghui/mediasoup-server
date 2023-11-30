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
#include "FBS/notification.h"
#include "FBS/dataConsumer.h"

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
         * Whether the data consumer must start in paused mode. Default false.
         */
        bool paused = false;

        /**
         * Subchannels this data consumer initially subscribes to.
         * Only used in case this data consumer receives messages from a local data
         * producer that specifies subchannel(s) when calling send().
         */
        std::vector<uint16_t> subchannels;

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    struct DataConsumerStat
    {
        std::string type;
        uint64_t timestamp;
        std::string label;
        std::string protocol;
        int64_t messagesSent;
        int64_t bytesSent;
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
        
        uint32_t bufferedAmountLowThreshold;
    };

    struct DataConsumerDump : DataConsumerData
    {
        std::string id;
        bool paused;
        bool dataProducerPaused;
        std::vector<uint16_t> subchannels;
    };

    class Channel;

    class DataConsumerController : public std::enable_shared_from_this<DataConsumerController>
    {
    public:
        DataConsumerController(const DataConsumerInternal& internal,
                               const DataConsumerData& data,
                               const std::shared_ptr<Channel>& channel,
                               bool paused,
                               bool dataProducerPaused,
                               const std::vector<uint16_t>& subchannels,
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
        
        bool paused() { return _paused; }
        
        bool dataProducerPaused() { return _dataProducerPaused; }
        
        const std::vector<uint16_t>& subchannels() { return _subchannels; }

        void setAppData(const nlohmann::json& data) { _appData = data; }
        
        const nlohmann::json& appData() { return _appData; }
        
        void close();
        
        void onTransportClosed();
        
        std::shared_ptr<DataConsumerDump> dump();
        
        std::vector<std::shared_ptr<DataConsumerStat>> getStats();
        
        void pause();
        
        void resume();
        
        void setBufferedAmountLowThreshold(uint32_t threshold);
        
        void setSubchannels(const std::vector<uint16_t>& subchannels);

        void send(const std::vector<uint8_t>& data, bool isBinary = false);
        
        uint32_t getBufferedAmount();
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data);
        
    public:
        sigslot::signal<> transportCloseSignal;
        
        sigslot::signal<> dataProducerCloseSignal;
        
        sigslot::signal<> dataProducerPauseSignal;
        
        sigslot::signal<> dataProducerResumeSignal;
        
        // data, ppid
        sigslot::signal<const std::vector<uint8_t>&, int32_t> messageSignal;
        
        sigslot::signal<> sctpSendBufferFullSignal;
        
        sigslot::signal<int32_t> bufferedAmountLowSignal;
        
        sigslot::signal<> closeSignal;
        
        sigslot::signal<> pauseSignal;
        
        sigslot::signal<> resumeSignal;
        
    private:
        // Internal data.
        DataConsumerInternal _internal;

        // Consumer data.
        DataConsumerData _data;

        // Channel instance.
        std::weak_ptr<Channel> _channel;

        // Closed flag.
        std::atomic_bool _closed { false };
        
        // Paused flag.
        bool _paused = false;

        // Associated DataProducer paused flag.
        bool _dataProducerPaused = false;

        // Subchannels subscribed to.
        std::vector<uint16_t> _subchannels;

        // Custom app data.
        nlohmann::json _appData;
        
    };

    FBS::DataProducer::Type dataConsumerTypeToFbs(const std::string& type);

    std::string dataConsumerTypeFromFbs(FBS::DataProducer::Type type);

    std::shared_ptr<DataConsumerDump> parseDataConsumerDumpResponse(const FBS::DataConsumer::DumpResponse* data);

    std::shared_ptr<DataConsumerStat> parseDataConsumerStats(const FBS::DataConsumer::GetStatsResponse* binary);

}
