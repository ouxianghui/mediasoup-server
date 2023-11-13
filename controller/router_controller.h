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
#include "rtp_parameters.h"
#include "transport_controller.h"
#include "sctp_parameters.h"

namespace srv {
 
    class RouterController;
    class ConsumerController;
    class DataConsumerController;
    class ProducerController;
    class DataProducerController;
    class RtpObserverController;
    class Channel;
    class PayloadChannel;
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
    class PipeTransportController;

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

    struct PipeToRouterOptions
    {
        /**
         * The id of the Producer to consume.
         */
        std::string producerId;

        /**
         * The id of the DataProducer to consume.
         */
        std::string dataProducerId;

        /**
         * Target Router instance.
         */
        std::shared_ptr<RouterController> router;

        /**
         * IP used in the PipeTransport pair. Default '127.0.0.1'.
         * value: TransportListenIp | string;
         */
        std::string listenIp;

        /**
         * Create a SCTP association. Default true.
         */
        bool enableSctp;

        /**
         * SCTP streams number.
         */
        NumSctpStreams numSctpStreams;

        /**
         * Enable RTX and NACK for RTP retransmission.
         */
        bool enableRtx;

        /**
         * Enable SRTP.
         */
        bool enableSrtp;
    };

    struct PipeToRouterResult
    {
        /**
         * The Consumer created in the current Router.
         */
        std::shared_ptr<ConsumerController> pipeConsumerController;

        /**
         * The Producer created in the target Router.
         */
        std::shared_ptr<ProducerController> pipeProducerController;

        /**
         * The DataConsumer created in the current Router.
         */
        std::shared_ptr<DataConsumerController> pipeDataConsumerController;

        /**
         * The DataProducer created in the target Router.
         */
        std::shared_ptr<DataProducerController> pipeDataProducerController;
    };

    using PipeTransportControllerPair = std::unordered_map<std::string, std::shared_ptr<PipeTransportController>>;

    struct RouterInternal
    {
        std::string routerId;
    };

    class RouterController : public std::enable_shared_from_this<RouterController> {
    public:
        RouterController(const RouterInternal& internal,
                         const RouterData& data,
                         const std::shared_ptr<Channel>& channel,
                         std::shared_ptr<PayloadChannel> payloadChannel,
                         const nlohmann::json& appData);
        
        ~RouterController();
        
        void init();
        
        void destroy();
        
        const std::string& id() { return _internal.routerId; }
        
        bool closed() { return _closed; }
        
        const RtpCapabilities& rtpCapabilities() { return _data.rtpCapabilities; }
        
        void setAppData(const nlohmann::json& data) { _appData = data; }
        
        const nlohmann::json& appData() { return _appData; }
        
        nlohmann::json dump();
        
        void close();
        
        void onWorkerClosed();
        
        bool canConsume(const std::string& producerId, const RtpCapabilities& rtpCapabilities);
            
        std::shared_ptr<WebRtcTransportController> createWebRtcTransportController(const std::shared_ptr<WebRtcTransportOptions>& options);
        
        std::shared_ptr<PlainTransportController> createPlainTransportController(const std::shared_ptr<PlainTransportOptions>& options);
        
        std::shared_ptr<DirectTransportController> createDirectTransportController(const std::shared_ptr<DirectTransportOptions>& options);
        
        std::shared_ptr<PipeTransportController> createPipeTransportController(const std::shared_ptr<PipeTransportOptions>& options);
        
        std::shared_ptr<ActiveSpeakerObserverController> createActiveSpeakerObserverController(const std::shared_ptr<ActiveSpeakerObserverOptions>& options);
        
        std::shared_ptr<AudioLevelObserverController> createAudioLevelObserverController(const std::shared_ptr<AudioLevelObserverOptions>& options);
        
        std::shared_ptr<PipeToRouterResult> pipeToRouter(const std::shared_ptr<PipeToRouterOptions>& options);
        
    public:
        // signals
        sigslot::signal<std::shared_ptr<RouterController>> closeSignal;
        
        sigslot::signal<> workerCloseSignal;
        
        sigslot::signal<std::shared_ptr<TransportController>> newTransportSignal;
        
        sigslot::signal<std::shared_ptr<RtpObserverController>> newRtpObserverSignal;
        
    private:
        void clear();
        
        std::shared_ptr<ProducerController> getProducerController(const std::string& producerId);

        std::shared_ptr<DataProducerController> getDataProducerController(const std::string& dataProducerId);
        
        void connectSignals(const std::shared_ptr<TransportController>& transportController);
        
        // key: router.id
        void addPipeTransportPair(const std::string& key, PipeTransportControllerPair& pair);
        
    private:
        // Internal data.
        RouterInternal _internal;

        // Router data.
        RouterData _data;

        // Channel instance.
        std::weak_ptr<Channel> _channel;

        // PayloadChannel instance.
        std::weak_ptr<PayloadChannel> _payloadChannel;

        // Closed flag.
        std::atomic_bool _closed { false };

        // Custom app data.
        nlohmann::json _appData;

        std::mutex _transportsMutex;
        // Transports map.
        std::unordered_map<std::string, std::shared_ptr<TransportController>> _transportControllers;

        std::mutex _producersMutex;
        // Producers map.
        std::unordered_map<std::string, std::shared_ptr<ProducerController>> _producerControllers;

        std::mutex _rtpObserversMutex;
        // RtpObservers map.
        std::unordered_map<std::string, std::shared_ptr<RtpObserverController>> _rtpObserverControllers;

        std::mutex _dataProducersMutex;
        // DataProducers map.
        std::unordered_map<std::string, std::shared_ptr<DataProducerController>> _dataProducerControllers;

        std::function<std::shared_ptr<ProducerController>(const std::string&)> _getProducerController;
        
        std::function<std::shared_ptr<DataProducerController>(const std::string&)> _getDataProducerController;
        
        std::function<RtpCapabilities()> _getRouterRtpCapabilities;
        
        // Map of PipeTransport pair indexed by the id of the Router in which pipeToRouter() was called.
        std::unordered_map<std::string, PipeTransportControllerPair> _routerPipeTransportPairMap;
    };

}
