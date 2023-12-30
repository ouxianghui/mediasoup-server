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
#include <absl/types/optional.h>
#include "interface/i_consumer_controller.h"
#include "FBS/notification.h"
#include "FBS/consumer.h"

namespace srv {

    class Channel;

    class ConsumerController : public IConsumerController, public std::enable_shared_from_this<ConsumerController>
    {
    public:
        ConsumerController(const ConsumerInternal& internal,
                           const ConsumerData& data,
                           const std::shared_ptr<Channel>& channel,
                           const nlohmann::json& appData,
                           bool paused,
                           bool producerPaused,
                           const ConsumerScore& score,
                           const ConsumerLayers& preferredLayers);
        
        virtual ~ConsumerController();
        
        void init() override;
        
        void destroy() override;
        
        const std::string& id() override { return _internal.consumerId; }
        
        const std::string& producerId() override { return _data.producerId; }

        const std::string& kind() override { return _data.kind; }

        const RtpParameters& rtpParameters() override { return _data.rtpParameters; }

        const std::string& type() override { return _data.type; }

        const ConsumerScore& score() override { return _score; }

        const ConsumerLayers& preferredLayers() override { return _preferredLayers; }

        const ConsumerLayers& currentLayers() override { return _currentLayers; }
        
        void setAppData(const nlohmann::json& data) override { _appData = data; }
        
        const nlohmann::json& appData() override { return _appData; }
        
        void close() override;
        
        bool closed() override { return _closed; }

        void onTransportClosed() override;
        
        std::shared_ptr<ConsumerDump> dump() override;
        
        std::vector<std::shared_ptr<ConsumerStat>> getStats() override;
        
        void pause() override;
        
        void resume() override;

        bool paused() override { return _paused; }

        bool producerPaused() override { return _producerPaused; }

        void setPreferredLayers(const ConsumerLayers& layers) override;
        
        void setPriority(int32_t priority) override;

        void unsetPriority() override;

        int32_t priority() override { return _priority; }

        void requestKeyFrame() override;

        // types = 'rtp' | 'keyframe' | 'nack' | 'pli' | 'fir';
        void enableTraceEvent(const std::vector<std::string>& types) override;
        
    private:
        void handleWorkerNotifications();

        void onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data);
        
    private:
        // Internal data.
        ConsumerInternal _internal;

        // Consumer data.
        ConsumerData _data;

        // Channel instance.
        std::weak_ptr<Channel> _channel;

        // Closed flag.
        std::atomic_bool _closed { false };

        // Custom app data.
        nlohmann::json _appData;

        // Paused flag.
        std::atomic_bool _paused { false };

        // Associated Producer paused flag.
        std::atomic_bool _producerPaused { false };

        // Current priority.
        int32_t _priority = 1;

        // Current score.
        ConsumerScore _score;

        // Preferred layers.
        ConsumerLayers _preferredLayers;

        // Curent layers.
        ConsumerLayers _currentLayers;
    };

    std::shared_ptr<ConsumerDump> parseConsumerDumpResponse(const FBS::Consumer::DumpResponse* response);

    std::shared_ptr<BaseConsumerDump> parseBaseConsumerDump(const FBS::Consumer::BaseConsumerDump* data);

    std::shared_ptr<SimpleConsumerDump> parseSimpleConsumerDump(const FBS::Consumer::ConsumerDump* consumerDump);

    std::shared_ptr<SimulcastConsumerDump> parseSimulcastConsumerDump(const FBS::Consumer::ConsumerDump* consumerDump);

    std::shared_ptr<SvcConsumerDump> parseSvcConsumerDump(const FBS::Consumer::ConsumerDump* consumerDump);

    std::shared_ptr<PipeConsumerDump> parsePipeConsumerDump(const FBS::Consumer::ConsumerDump* consumerDump);

    std::shared_ptr<ConsumerTraceEventData> parseTraceEventData(const FBS::Consumer::TraceNotification* trace);

    std::shared_ptr<ConsumerLayers>  parseConsumerLayers(const FBS::Consumer::ConsumerLayers* data);

    std::vector<std::shared_ptr<ConsumerStat>> parseConsumerStats(const FBS::Consumer::GetStatsResponse* binary);

}
