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
#include "threadsafe_vector.hpp"
#include "interface/i_producer_controller.h"
#include "FBS/notification.h"
#include "FBS/producer.h"
#include "ortc.h"

namespace srv {

    class Channel;

    class ProducerController : public IProducerController, public std::enable_shared_from_this<ProducerController>
    {
    public:
        ProducerController(const ProducerInternal& internal,
                           const ProducerData& data,
                           const std::shared_ptr<Channel>& channel,
                           const nlohmann::json& appData,
                           bool paused);
        
        virtual ~ProducerController();
        
        void init() override;
        
        void destroy() override;
        
        const std::string& id() override { return _internal.producerId; }

        const std::string& kind() override { return _data.kind; }

        const RtpParameters& rtpParameters() override { return _data.rtpParameters; }

        const std::string& type() override { return _data.type; }

        const RtpParameters& consumableRtpParameters() override { return _data.consumableRtpParameters; }

        const std::threadsafe_vector<ProducerScore>& score() override {
            return _score;
        }
        
        void setAppData(const nlohmann::json& data) override { _appData = data; }
        
        const nlohmann::json& appData() override { return _appData; }
        
        void close() override;
        
        bool closed() override { return _closed; }

        void onTransportClosed() override;
        
        std::shared_ptr<ProducerDump> dump() override;
        
        std::vector<std::shared_ptr<ProducerStat>> getStats() override;
        
        void pause() override;
        
        void resume() override;

        bool paused() override { return _paused; }

        // types = 'rtp' | 'keyframe' | 'nack' | 'pli' | 'fir';
        void enableTraceEvent(const std::vector<std::string>& types) override;
        
        void send(const std::vector<uint8_t>& data) override;
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data);
        
    private:
        // Internal data.
        ProducerInternal _internal;

        // Consumer data.
        ProducerData _data;

        // Channel instance.
        std::weak_ptr<Channel> _channel;

        // Closed flag.
        std::atomic_bool _closed { false };

        // Custom app data.
        nlohmann::json _appData;

        // Paused flag.
        std::atomic_bool _paused { false };
        
        std::threadsafe_vector<ProducerScore> _score;
        
    };

    std::string producerTypeFromFbs(FBS::RtpParameters::Type type);

    FBS::RtpParameters::Type producerTypeToFbs(const std::string& type);

    FBS::Producer::TraceEventType producerTraceEventTypeToFbs(const std::string& eventType);

    std::string producerTraceEventTypeFromFbs(FBS::Producer::TraceEventType eventType);

    std::shared_ptr<ProducerDump> parseProducerDump(const FBS::Producer::DumpResponse* data);

    std::vector<std::shared_ptr<ProducerStat>> parseProducerStats(const FBS::Producer::GetStatsResponse* binary);

    std::shared_ptr<ProducerScore> parseProducerScore(const FBS::Producer::Score* binary);

    std::shared_ptr<ProducerTraceEventData> parseTraceEventData(const FBS::Producer::TraceNotification* trace);

}
