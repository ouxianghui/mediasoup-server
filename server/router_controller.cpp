/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "router_controller.h"
#include "srv_logger.h"
#include "types.h"
#include "uuid.h"
#include "ortc.h"
#include "channel.h"
#include "payload_channel.h"
#include "rtp_observer_controller.h"
#include "webrtc_server_controller.h"
#include "webrtc_transport_controller.h"
#include "plain_transport_controller.h"
#include "direct_transport_controller.h"
#include "pipe_transport_controller.h"
#include "audio_level_observer_controller.h"
#include "active_speaker_observer_controller.h"

namespace srv {

    RouterController::RouterController(const RouterInternal& internal,
                                       const RouterData& data,
                                       const std::shared_ptr<Channel>& channel,
                                       std::shared_ptr<PayloadChannel> payloadChannel,
                                       const nlohmann::json& appData)
    : _internal(internal)
    , _data(data)
    , _channel(channel)
    , _payloadChannel(payloadChannel)
    , _appData(appData)
    {
        SRV_LOGD("RouterController()");
        
        _getProducerController = [&](const std::string& producerId) {
            return this->getProducerController(producerId);
        };
        
        _getDataProducerController = [&](const std::string& dataProducerId) {
            return this->getDataProducerController(dataProducerId);
        };
        
        _getRouterRtpCapabilities = [&]() {
            return this->rtpCapabilities();
        };
        
    }

    RouterController::~RouterController()
    {
        SRV_LOGD("~RouterController()");
    }

    void RouterController::init()
    {
        SRV_LOGD("init()");
    }

    void RouterController::destroy()
    {
        SRV_LOGD("destroy()");
    }

    std::shared_ptr<ProducerController> RouterController::getProducerController(const std::string& producerId)
    {
        std::shared_ptr<ProducerController> controller;
        {
            std::lock_guard<std::mutex> lock(_producersMutex);
            if (_producerControllers.find(producerId) != _producerControllers.end()) {
                controller = _producerControllers[producerId];
            }
        }
        return controller;
    }

    std::shared_ptr<DataProducerController> RouterController::getDataProducerController(const std::string& dataProducerId)
    {
        std::shared_ptr<DataProducerController> controller;
        {
            std::lock_guard<std::mutex> lock(_dataProducersMutex);
            if (_dataProducerControllers.find(dataProducerId) != _dataProducerControllers.end()) {
                controller = _dataProducerControllers[dataProducerId];
            }
        }
        return controller;
    }

    void RouterController::clear()
    {
        std::unordered_map<std::string, std::shared_ptr<TransportController>> transportControllers;
        {
            std::lock_guard<std::mutex> lock(_transportsMutex);
            transportControllers = _transportControllers;
        }

        // Close every Transport.
        for (const auto& item : transportControllers) {
            item.second->onRouterClosed();
        }
        
        {
            std::lock_guard<std::mutex> lock(_producersMutex);
            // Clear the Producers map.
            _producerControllers.clear();
        }

        std::unordered_map<std::string, std::shared_ptr<RtpObserverController>> rtpObserverControllers;
        {
            std::lock_guard<std::mutex> lock(_rtpObserversMutex);
            rtpObserverControllers = _rtpObserverControllers;
            //_rtpObserverControllers.clear();
        }

        // Close every RtpObserver.
        for (const auto& item : rtpObserverControllers) {
            item.second->onRouterClosed();
        }
        
        {
            std::lock_guard<std::mutex> lock(_dataProducersMutex);
            // Clear the DataProducers map.
            _dataProducerControllers.clear();
        }
    }

    void RouterController::close()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");

        _closed = true;

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        nlohmann::json reqData;
        reqData["routerId"] = _internal.routerId;

        channel->request("worker.closeRouter", "", reqData.dump());

        clear();

        _closeSignal(shared_from_this());
    }

    void RouterController::onWorkerClosed()
    {
        
        if (_closed) {
            return;
        }

        SRV_LOGD("onWorkerClosed()");

        _closed = true;

        clear();

        _workerCloseSignal();
        
        _closeSignal(shared_from_this());
    }
        
    nlohmann::json RouterController::dump()
    {
        SRV_LOGD("dump()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nlohmann::json();
        }
        
        return channel->request("router.dump", _internal.routerId, "{}");
    }

    std::shared_ptr<WebRtcTransportController> RouterController::createWebRtcTransportController(const std::shared_ptr<WebRtcTransportOptions>& options)
    {
        SRV_LOGD("createWebRtcTransportController()");
        
        std::shared_ptr<WebRtcTransportController> transportController;
        
        if (!options) {
            SRV_LOGE("options must be a valid pointer");
            return transportController;
        }
        
        auto channel = _channel.lock();
        if (!channel) {
            SRV_LOGE("channel must be a valid pointer");
            return transportController;
        }
        
        const nlohmann::json& listenIps = options->listenIps;
        const int32_t port = options->port;
        const bool enableUdp = options->enableUdp;
        const bool enableTcp = options->enableTcp;
        const bool preferUdp = options->preferUdp;
        const bool preferTcp = options->preferTcp;
        const int32_t initialAvailableOutgoingBitrate = options->initialAvailableOutgoingBitrate;
        const bool enableSctp = options->enableSctp;
        const NumSctpStreams& numSctpStreams = options->numSctpStreams;
        const int32_t maxSctpMessageSize = options->maxSctpMessageSize;
        const int32_t sctpSendBufferSize = options->sctpSendBufferSize;
        const nlohmann::json& appData = options->appData;
        const auto& webRtcServer = options->webRtcServer;
        
        if (!webRtcServer && !listenIps.is_array()) {
            SRV_LOGE("missing webRtcServer and listenIps (one of them is mandatory)");
            return transportController;
        }

        TransportInternal internal;
        internal.routerId = _internal.routerId;
        internal.transportId = uuid::uuidv4();
        
        nlohmann::json reqData;
        reqData["transportId"] = internal.transportId;
        reqData["webRtcServerId"] =  webRtcServer ? webRtcServer->id() : "";
        reqData["listenIps"] = listenIps;
        reqData["port"] = port;
        reqData["enableUdp"] = enableUdp;
        reqData["enableTcp"] = enableTcp;
        reqData["preferUdp"] = preferUdp;
        reqData["preferTcp"] = preferTcp;
        reqData["initialAvailableOutgoingBitrate"] = initialAvailableOutgoingBitrate;
        reqData["enableSctp"] = enableSctp;
        reqData["numSctpStreams"] = numSctpStreams;
        reqData["maxSctpMessageSize"] = maxSctpMessageSize;
        reqData["sctpSendBufferSize"] = sctpSendBufferSize;
        reqData["isDataChannel"] = true;
        
        nlohmann::json jsData;
        if (webRtcServer) {
            jsData = channel->request("router.createWebRtcTransportWithServer", _internal.routerId, reqData.dump());
        }
        else {
            jsData = channel->request("router.createWebRtcTransport", _internal.routerId, reqData.dump());
        }
        
        auto wtcOptions = std::make_shared<WebRtcTransportConstructorOptions>();
        wtcOptions->internal = internal;
        wtcOptions->data = jsData;
        wtcOptions->channel = _channel.lock();
        wtcOptions->payloadChannel = _payloadChannel.lock();
        wtcOptions->appData = appData;
        wtcOptions->getRouterRtpCapabilities = _getRouterRtpCapabilities;
        wtcOptions->getProducerController = _getProducerController;
        wtcOptions->getDataProducerController = _getDataProducerController;
        
        {
            std::lock_guard<std::mutex> lock(_transportsMutex);
            transportController = std::make_shared<WebRtcTransportController>(wtcOptions);
            transportController->init();
            _transportControllers[internal.transportId] = transportController;
        }
        
        connectSignals(transportController);
        
        _newTransportSignal(transportController);
        
        // TODO: check thread
        if (webRtcServer) {
            webRtcServer->handleWebRtcTransport(transportController);
        }

        return transportController;
    }

    std::shared_ptr<PlainTransportController> RouterController::createPlainTransportController(const std::shared_ptr<PlainTransportOptions>& options)
    {
        SRV_LOGD("createPlainTransportController()");
        
        std::shared_ptr<PlainTransportController> transportController;
        
        if (!options) {
            SRV_LOGE("options must be a valid pointer");
            return transportController;
        }
        
        auto channel = _channel.lock();
        if (!channel) {
            SRV_LOGE("channel must be a valid pointer");
            return transportController;
        }
        
        const nlohmann::json& listenIps = options->listenIps;
        const int32_t port = options->port;
        const bool rtcpMux = options->rtcpMux;
        const bool comedia = options->comedia;
        const bool enableSctp = options->enableSctp;
        const bool enableSrtp = options->enableSrtp;
        const NumSctpStreams& numSctpStreams = options->numSctpStreams;
        const std::string& srtpCryptoSuite = options->srtpCryptoSuite;
        const int32_t maxSctpMessageSize = options->maxSctpMessageSize;
        const int32_t sctpSendBufferSize = options->sctpSendBufferSize;
        const nlohmann::json& appData = options->appData;


        TransportInternal internal;
        internal.routerId = _internal.routerId;
        internal.transportId = uuid::uuidv4();
        
        nlohmann::json reqData;
        reqData["transportId"] = internal.transportId;
        reqData["listenIps"] = listenIps;
        reqData["port"] = port;
        reqData["rtcpMux"] = rtcpMux;
        reqData["comedia"] = comedia;
        reqData["enableSctp"] = enableSctp;
        reqData["numSctpStreams"] = numSctpStreams;
        reqData["maxSctpMessageSize"] = maxSctpMessageSize;
        reqData["sctpSendBufferSize"] = sctpSendBufferSize;
        reqData["isDataChannel"] = true;
        reqData["enableSrtp"] = enableSrtp;
        reqData["srtpCryptoSuite"] = srtpCryptoSuite;
        
        nlohmann::json jsData = channel->request("router.createPlainTransport", _internal.routerId, reqData.dump());
        
        auto ptcOptions = std::make_shared<PlainTransportConstructorOptions>();
        ptcOptions->internal = internal;
        ptcOptions->data = jsData;
        ptcOptions->channel = _channel.lock();
        ptcOptions->payloadChannel = _payloadChannel.lock();
        ptcOptions->appData = appData;
        ptcOptions->getRouterRtpCapabilities = _getRouterRtpCapabilities;
        ptcOptions->getProducerController = _getProducerController;
        ptcOptions->getDataProducerController = _getDataProducerController;
        
        {
            std::lock_guard<std::mutex> lock(_transportsMutex);
            transportController = std::make_shared<PlainTransportController>(ptcOptions);
            transportController->init();
            _transportControllers[internal.transportId] = transportController;
        }
        
        connectSignals(transportController);
        
        _newTransportSignal(transportController);

        return transportController;
    }

    std::shared_ptr<DirectTransportController> RouterController::createDirectTransportController(const std::shared_ptr<DirectTransportOptions>& options)
    {
        SRV_LOGD("createDirectTransportController()");
        
        std::shared_ptr<DirectTransportController> transportController;
        
        if (!options) {
            SRV_LOGE("options must be a valid pointer");
            return transportController;
        }
        
        auto channel = _channel.lock();
        if (!channel) {
            SRV_LOGE("channel must be a valid pointer");
            return transportController;
        }
        
        const int32_t maxMessageSize = options->maxMessageSize;
        const nlohmann::json& appData = options->appData;


        TransportInternal internal;
        internal.routerId = _internal.routerId;
        internal.transportId = uuid::uuidv4();
        
        nlohmann::json reqData;
        reqData["transportId"] = internal.transportId;
        reqData["direct"] = true;
        reqData["maxMessageSize"] = maxMessageSize;
        
        nlohmann::json jsData = channel->request("router.createDirectTransport", _internal.routerId, reqData.dump());
        
        auto dtcOptions = std::make_shared<DirectTransportConstructorOptions>();
        dtcOptions->internal = internal;
        dtcOptions->data = jsData;
        dtcOptions->channel = _channel.lock();
        dtcOptions->payloadChannel = _payloadChannel.lock();
        dtcOptions->appData = appData;
        dtcOptions->getRouterRtpCapabilities = _getRouterRtpCapabilities;
        dtcOptions->getProducerController = _getProducerController;
        dtcOptions->getDataProducerController = _getDataProducerController;
        
        {
            std::lock_guard<std::mutex> lock(_transportsMutex);
            transportController = std::make_shared<DirectTransportController>(dtcOptions);
            transportController->init();
            _transportControllers[internal.transportId] = transportController;
        }
        
        connectSignals(transportController);
        
        _newTransportSignal(transportController);

        return transportController;
    }

    std::shared_ptr<PipeTransportController> RouterController::createPipeTransportController(const std::shared_ptr<PipeTransportOptions>& options)
    {
        SRV_LOGD("createPipeTransportController()");
        std::shared_ptr<PipeTransportController> pipeTransportController;
        
        return pipeTransportController;
    }

    std::shared_ptr<ActiveSpeakerObserverController> RouterController::createActiveSpeakerObserverController(const std::shared_ptr<ActiveSpeakerObserverOptions>& options)
    {
        SRV_LOGD("createActiveSpeakerObserverController()");
        
        std::shared_ptr<ActiveSpeakerObserverController> rtpObserverController;
        
        if (!options) {
            SRV_LOGE("options must be a valid pointer");
            return rtpObserverController;
        }
        
        auto channel = _channel.lock();
        if (!channel) {
            SRV_LOGE("channel must be a valid pointer");
            return rtpObserverController;
        }
        
        const int interval = options->interval;
        const nlohmann::json& appData = options->appData;
        
        RtpObserverObserverInternal internal;
        internal.routerId = _internal.routerId;
        internal.rtpObserverId = uuid::uuidv4();

        nlohmann::json reqData;
        reqData["rtpObserverId"] = internal.rtpObserverId;
        reqData["interval"] = interval;
        
        channel->request("router.createActiveSpeakerObserver", _internal.routerId, reqData.dump());
        
        auto roocOptions = std::make_shared<RtpObserverObserverConstructorOptions>();
        roocOptions->internal = internal;
        roocOptions->channel = _channel.lock();
        roocOptions->payloadChannel = _payloadChannel.lock();
        roocOptions->appData = appData;
        roocOptions->getProducerController = _getProducerController;
        
        {
            std::lock_guard<std::mutex> lock(_rtpObserversMutex);
            rtpObserverController = std::make_shared<ActiveSpeakerObserverController>(roocOptions);
            rtpObserverController->init();
            
            _rtpObserverControllers[internal.rtpObserverId] = rtpObserverController;
        }
        
        rtpObserverController->_closeSignal.connect([id = rtpObserverController->id(), wself = std::weak_ptr<RouterController>(shared_from_this())]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            std::lock_guard<std::mutex> lock(self->_rtpObserversMutex);
            if (self->_rtpObserverControllers.find(id) != self->_rtpObserverControllers.end()) {
                self->_rtpObserverControllers.erase(id);
            }
        });
        
        _newRtpObserverSignal(rtpObserverController);
        
        return rtpObserverController;
    }

    std::shared_ptr<AudioLevelObserverController> RouterController::createAudioLevelObserverController(const std::shared_ptr<AudioLevelObserverOptions>& options)
    {
        SRV_LOGD("createAudioLevelObserverController()");
        
        std::shared_ptr<AudioLevelObserverController> rtpObserverController;
        
        if (!options) {
            SRV_LOGE("options must be a valid pointer");
            return rtpObserverController;
        }
        
        auto channel = _channel.lock();
        if (!channel) {
            SRV_LOGE("channel must be a valid pointer");
            return rtpObserverController;
        }
        
        const int maxEntries = options->maxEntries;
        const int threshold = options->threshold;
        const int interval = options->interval;
        const nlohmann::json& appData = options->appData;
        
        RtpObserverObserverInternal internal;
        internal.routerId = _internal.routerId;
        internal.rtpObserverId = uuid::uuidv4();

        nlohmann::json reqData;
        reqData["rtpObserverId"] = internal.rtpObserverId;
        reqData["maxEntries"] = maxEntries;
        reqData["threshold"] = threshold;
        reqData["interval"] = interval;

        channel->request("router.createAudioLevelObserver", _internal.routerId, reqData.dump());
    
        auto alocOptions = std::make_shared<AudioLevelObserverConstructorOptions>();
        alocOptions->internal = internal;
        alocOptions->channel = _channel.lock();
        alocOptions->payloadChannel = _payloadChannel.lock();
        alocOptions->appData = appData;
        alocOptions->getProducerController = _getProducerController;
        
        {
            std::lock_guard<std::mutex> lock(_rtpObserversMutex);
            rtpObserverController = std::make_shared<AudioLevelObserverController>(alocOptions);
            rtpObserverController->init();
            _rtpObserverControllers[internal.rtpObserverId] = rtpObserverController;
        }
        
        rtpObserverController->_closeSignal.connect([id = rtpObserverController->id(), wself = std::weak_ptr<RouterController>(shared_from_this())]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            std::lock_guard<std::mutex> lock(self->_rtpObserversMutex);
            if (self->_rtpObserverControllers.find(id) != self->_rtpObserverControllers.end()) {
                self->_rtpObserverControllers.erase(id);
            }
        });
        
        _newRtpObserverSignal(rtpObserverController);
        
        return rtpObserverController;
    }

    bool RouterController::canConsume(const std::string& producerId, const RtpCapabilities& rtpCapabilities)
    {
        SRV_LOGD("canConsume()");
        
        if (_producerControllers.find(producerId) == _producerControllers.end()) {
            return false;
        }
        
        auto producerController = _producerControllers[producerId];
        
        if (!producerController) {
            SRV_LOGE("canConsume() | Producer with id '%s' not found", producerId.c_str());
            return false;
        }

        try {
            return ortc::canConsume(producerController->consumableRtpParameters(), rtpCapabilities);
        }
        catch (...) {
            SRV_LOGE("canConsume() | unexpected error");
            return false;
        }
    }

    void RouterController::connectSignals(const std::shared_ptr<TransportController>& transportController)
    {
        transportController->_closeSignal.connect([wself = std::weak_ptr<RouterController>(shared_from_this())](const std::string& transportId) {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            //std::lock_guard<std::mutex> lock(self->_transportsMutex);
            if (self->_transportControllers.find(transportId) != self->_transportControllers.end()) {
                self->_transportControllers.erase(transportId);
            }
        });
        
        transportController->_listenServerCloseSignal.connect([id = transportController->id(), wself = std::weak_ptr<RouterController>(shared_from_this())]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            //std::lock_guard<std::mutex> lock(self->_transportsMutex);
            if (self->_transportControllers.find(id) != self->_transportControllers.end()) {
                self->_transportControllers.erase(id);
            }
        });
        
        transportController->_newProducerSignal.connect([wself = std::weak_ptr<RouterController>(shared_from_this())](std::shared_ptr<ProducerController> producerController) {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (!producerController) {
                return;
            }
            //std::lock_guard<std::mutex> lock(self->_producersMutex);
            if (self->_producerControllers.find(producerController->id()) == self->_producerControllers.end()) {
                self->_producerControllers[producerController->id()] = producerController;
            }
        });
        
        transportController->_producerCloseSignal.connect([wself = std::weak_ptr<RouterController>(shared_from_this())](std::shared_ptr<ProducerController> producerController) {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (!producerController) {
                return;
            }
            //std::lock_guard<std::mutex> lock(self->_producersMutex);
            if (self->_producerControllers.find(producerController->id()) != self->_producerControllers.end()) {
                self->_producerControllers.erase(producerController->id());
            }
        });
        
        transportController->_newDataProducerSignal.connect([wself = std::weak_ptr<RouterController>(shared_from_this())](std::shared_ptr<DataProducerController> dataProducerController) {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (!dataProducerController) {
                return;
            }
            //std::lock_guard<std::mutex> lock(self->_dataProducersMutex);
            if (self->_dataProducerControllers.find(dataProducerController->id()) == self->_dataProducerControllers.end()) {
                self->_dataProducerControllers[dataProducerController->id()] = dataProducerController;
            }
        });
        
        transportController->_dataProducerCloseSignal.connect([wself = std::weak_ptr<RouterController>(shared_from_this())](std::shared_ptr<DataProducerController> dataProducerController) {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (!dataProducerController) {
                return;
            }
            //std::lock_guard<std::mutex> lock(self->_dataProducersMutex);
            if (self->_dataProducerControllers.find(dataProducerController->id()) != self->_dataProducerControllers.end()) {
                self->_dataProducerControllers.erase(dataProducerController->id());;
            }
        });
    }

}

namespace srv
{
    void to_json(nlohmann::json& j, const RouterOptions& st)
    {
        j["mediaCodecs"] = st.mediaCodecs;
        j["appData"] = st.appData;
    }

    void from_json(const nlohmann::json& j, RouterOptions& st)
    {
        if (j.contains("mediaCodecs")) {
            j.at("mediaCodecs").get_to(st.mediaCodecs);
        }
        if (j.contains("appData")) {
            j.at("appData").get_to(st.appData);
        }
    }
}
