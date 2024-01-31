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
#include "threadsafe_unordered_map.hpp"
#include "interface/i_router_controller.h"
#include "FBS/router.h"

namespace srv {
 
    class RouterController;
    class ConsumerController;
    class DataConsumerController;
    class ProducerController;
    class DataProducerController;
    class RtpObserverController;
    class Channel;
    struct WebRtcTransportOptions;
    class WebRtcTransportController;
    struct PlainTransportOptions;
    class PlainTransportController;
    struct DirectTransportOptions;
    class DirectTransportController;
    struct PipeTransportOptions;
    class PipeTransportController;
    struct ActiveSpeakerObserverOptions;
    class ActiveSpeakerObserverController;
    struct AudioLevelObserverOptions;
    class AudioLevelObserverController;

    struct RouterData
    {
        RtpCapabilities rtpCapabilities;
    };

    struct RouterOptions
    {
        /**
         * Router media codecs.
         */
        std::vector<RtpCodecCapability> mediaCodecs;

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    void to_json(nlohmann::json& j, const RouterOptions& st);
    void from_json(const nlohmann::json& j, RouterOptions& st);

    struct RouterInternal
    {
        std::string routerId;
    };

    class RouterController : public IRouterController, public std::enable_shared_from_this<RouterController> {
    public:
        RouterController(const RouterInternal& internal,
                         const RouterData& data,
                         const std::shared_ptr<Channel>& channel,
                         const nlohmann::json& appData);
        
        ~RouterController();
        
        void init() override;
        
        void destroy() override;
        
        const std::string& id() override { return _internal.routerId; }
        
        const RtpCapabilities& rtpCapabilities() override { return _data.rtpCapabilities; }
        
        void setAppData(const nlohmann::json& data) override { _appData = data; }
        
        const nlohmann::json& appData() override { return _appData; }
        
        std::shared_ptr<RouterDump> dump() override;
        
        void close() override;
        
        bool closed() override { return _closed; }

        void onWorkerClosed() override;
        
        bool canConsume(const std::string& producerId, const RtpCapabilities& rtpCapabilities) override;
            
        std::shared_ptr<ITransportController> createWebRtcTransportController(const std::shared_ptr<WebRtcTransportOptions>& options) override;
        
        std::shared_ptr<ITransportController> createPlainTransportController(const std::shared_ptr<PlainTransportOptions>& options) override;
        
        std::shared_ptr<ITransportController> createDirectTransportController(const std::shared_ptr<DirectTransportOptions>& options) override;
        
        std::shared_ptr<ITransportController> createPipeTransportController(const std::shared_ptr<PipeTransportOptions>& options) override;
        
        std::shared_ptr<IRtpObserverController> createActiveSpeakerObserverController(const std::shared_ptr<ActiveSpeakerObserverOptions>& options) override;
        
        std::shared_ptr<IRtpObserverController> createAudioLevelObserverController(const std::shared_ptr<AudioLevelObserverOptions>& options) override;
        
        std::shared_ptr<PipeToRouterResult> pipeToRouter(const std::shared_ptr<PipeToRouterOptions>& options) override;
        
    private:
        void clear();
        
        std::shared_ptr<IProducerController> getProducerController(const std::string& producerId);

        std::shared_ptr<IDataProducerController> getDataProducerController(const std::string& dataProducerId);
        
        void connectSignals(const std::shared_ptr<ITransportController>& transportController);
        
        // key: router.id
        void addPipeTransportPair(const std::string& key, PipeTransportControllerPair& pair) override;
        
    private:
        // Internal data.
        RouterInternal _internal;

        // Router data.
        RouterData _data;

        // Channel instance.
        std::weak_ptr<Channel> _channel;

        // Closed flag.
        std::atomic_bool _closed { false };

        // Custom app data.
        nlohmann::json _appData;

        // Transports map.
        std::threadsafe_unordered_map<std::string, std::shared_ptr<ITransportController>> _transportControllers;

        // Producers map.
        std::threadsafe_unordered_map<std::string, std::shared_ptr<IProducerController>> _producerControllers;

        // RtpObservers map.
        std::threadsafe_unordered_map<std::string, std::shared_ptr<IRtpObserverController>> _rtpObserverControllers;

        // DataProducers map.
        std::threadsafe_unordered_map<std::string, std::shared_ptr<IDataProducerController>> _dataProducerControllers;

        std::function<std::shared_ptr<IProducerController>(const std::string&)> _getProducerController;
        
        std::function<std::shared_ptr<IDataProducerController>(const std::string&)> _getDataProducerController;
        
        std::function<RtpCapabilities()> _getRouterRtpCapabilities;
        
        // Map of PipeTransport pair indexed by the id of the Router in which pipeToRouter() was called.
        std::threadsafe_unordered_map<std::string, PipeTransportControllerPair> _routerPipeTransportPairMap;
    };

    std::shared_ptr<RouterDump> parseRouterDumpResponse(const FBS::Router::DumpResponse* binary);
}
