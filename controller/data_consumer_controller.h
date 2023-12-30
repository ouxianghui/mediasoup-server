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
#include "interface/i_data_consumer_controller.h"
#include "FBS/notification.h"
#include "FBS/dataConsumer.h"

namespace srv {

    class Channel;

    class DataConsumerController : public IDataConsumerController, public std::enable_shared_from_this<DataConsumerController>
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
        
        void init() override;
        
        void destroy() override;
        
        const std::string& id() override { return _internal.dataConsumerId; }

        const std::string& dataProducerId() override { return _data.dataProducerId; }

        const std::string& type() override { return _data.type; }

        const SctpStreamParameters& sctpStreamParameters() override { return _data.sctpStreamParameters; }

        const std::string& label() override { return _data.label; }

        const std::string& protocol() override { return _data.protocol; }
        
        const std::vector<uint16_t>& subchannels() override { return _subchannels; }

        void setAppData(const nlohmann::json& data) override { _appData = data; }
        
        const nlohmann::json& appData() override { return _appData; }
        
        void close() override;
        
        bool closed() override { return _closed; }

        void onTransportClosed() override;
        
        std::shared_ptr<DataConsumerDump> dump() override;
        
        std::vector<std::shared_ptr<DataConsumerStat>> getStats() override;
        
        void addSubchannel(uint16_t subchannel) override;
        
        void removeSubchannel(uint16_t subchannel) override;
        
        void pause() override;
        
        void resume() override;

        bool paused() override { return _paused; }
        
        bool dataProducerPaused() override { return _dataProducerPaused; }
        
        void setBufferedAmountLowThreshold(uint32_t threshold) override;
        
        void setSubchannels(const std::vector<uint16_t>& subchannels) override;

        void send(const std::vector<uint8_t>& data, bool isBinary = false) override;
        
        uint32_t getBufferedAmount() override;
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data);
        
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
