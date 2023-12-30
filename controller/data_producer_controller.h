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
#include "interface/i_data_producer_controller.h"
#include "sctp_parameters.h"
#include "FBS/dataProducer.h"

namespace srv {

    class Channel;

    class DataProducerController : public IDataProducerController, public std::enable_shared_from_this<DataProducerController>
    {
    public:
        DataProducerController(const DataProducerInternal& internal,
                               const DataProducerData& data,
                               const std::shared_ptr<Channel>& channel,
                               bool paused,
                               const nlohmann::json& appData);
        
        virtual ~DataProducerController();
        
        void init() override;
        
        void destroy() override;
        
        const std::string& id() override { return _internal.dataProducerId; }

        const std::string& type() override { return _data.type; }

        const SctpStreamParameters& sctpStreamParameters() override { return _data.sctpStreamParameters; }

        const std::string& label() override { return _data.label; }

        const std::string& protocol() override { return _data.protocol; }

        void setAppData(const nlohmann::json& data) override { _appData = data; }
        
        const nlohmann::json& appData() override { return _appData; }
        
        void pause() override;
        
        void resume() override;    
            
        bool paused() override { return _paused; }
        
        void close() override;
        
        bool closed() override { return _closed; }

        void onTransportClosed() override;
        
        std::shared_ptr<DataProducerDump> dump() override;
        
        std::vector<std::shared_ptr<DataProducerStat>> getStats() override;

        void send(const std::vector<uint8_t>& data, const std::vector<uint16_t>& subchannels, uint16_t requiredSubchannel, bool isBinary = false) override;
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, const std::string& event, const std::string& data) {}
        
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
