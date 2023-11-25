/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "transport_controller.h"
#include "srv_logger.h"
#include "types.h"
#include "uuid.h"
#include "channel.h"
#include "payload_channel.h"
#include "ortc.h"

namespace srv {

    TransportController::TransportController(const std::shared_ptr<TransportConstructorOptions>& options)
    : _internal(options->internal)
    , _data(options->data)
    , _channel(options->channel)
    , _payloadChannel(options->payloadChannel)
    , _appData(options->appData)
    , _getRouterRtpCapabilities(options->getRouterRtpCapabilities)
    , _getProducerController(options->getProducerController)
    , _getDataProducerController(options->getDataProducerController)
    {
        SRV_LOGD("TransportController()");
    }

    TransportController::~TransportController()
    {
        SRV_LOGD("~TransportController()");
    }

    void TransportController::close()
    {
        if (_closed) {
            return;
        }
        
        SRV_LOGD("close()");

        _closed = true;

        // Remove notification subscriptions.
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        channel->notificationSignal.disconnect(shared_from_this());
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->notificationSignal.disconnect(shared_from_this());
        
        nlohmann::json reqData;
        reqData["transportId"] = _internal.transportId;
        
        channel->request("router.closeTransport", _internal.routerId, reqData.dump());
        
        std::unordered_map<std::string, std::shared_ptr<ProducerController>> producerControllers;
        {
            std::lock_guard<std::mutex> lock(_producersMutex);
            producerControllers = _producerControllers;
        }
        for (const auto& item : producerControllers) {
            auto ctrl = item.second;
            ctrl->onTransportClosed();
            this->producerCloseSignal(ctrl);
        }
        //_producerControllers.clear();
        
        std::unordered_map<std::string, std::shared_ptr<ConsumerController>> consumerControllers;
        {
            std::lock_guard<std::mutex> lock(_consumersMutex);
            consumerControllers = _consumerControllers;
        }
        for (const auto& item : consumerControllers) {
            auto ctrl = item.second;
            ctrl->onTransportClosed();
        }
        //_consumerControllers.clear();
        
        std::unordered_map<std::string, std::shared_ptr<DataProducerController>> dataProducerControllers;
        {
            std::lock_guard<std::mutex> lock(_dataProducersMutex);
            dataProducerControllers = _dataProducerControllers;
        }
        for (const auto& item : dataProducerControllers) {
            auto ctrl = item.second;
            ctrl->onTransportClosed();
            this->dataProducerCloseSignal(ctrl);
        }
        //_dataProducerControllers.clear();
        
        std::unordered_map<std::string, std::shared_ptr<DataConsumerController>> dataConsumerControllers;
        {
            std::lock_guard<std::mutex> lock(_dataConsumersMutex);
            dataConsumerControllers = _dataConsumerControllers;
        }
        for (const auto& item : dataConsumerControllers) {
            auto ctrl = item.second;
            ctrl->onTransportClosed();
        }
        //_dataConsumerControllers.clear();
        
        this->closeSignal(id());
    }

    void TransportController::onRouterClosed()
    {
        if (_closed) {
            return;
        }
        
        SRV_LOGD("routerClosed()");

        _closed = true;

        // Remove notification subscriptions.
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        channel->notificationSignal.disconnect(shared_from_this());
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->notificationSignal.disconnect(shared_from_this());
        
        clearControllers();
        
        this->routerCloseSignal();
        
        this->closeSignal(id());
    }

    void TransportController::onListenServerClosed()
    {
        if (_closed) {
            return;
        }
        
        SRV_LOGD("onListenServerClosed()");

        _closed = true;

        // Remove notification subscriptions.
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        channel->notificationSignal.disconnect(shared_from_this());
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->notificationSignal.disconnect(shared_from_this());
        
        clearControllers();
        
        this->listenServerCloseSignal();
        
        this->closeSignal(id());
    }

    nlohmann::json TransportController::dump()
    {
        SRV_LOGD("dump()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nlohmann::json();
        }
        
        return channel->request("transport.dump", _internal.transportId, "{}");
    }

    nlohmann::json TransportController::getStats()
    {
        assert(0);
        return nlohmann::json();
    }

    void TransportController::connect(const nlohmann::json& data)
    {
        assert(0);
    }

    void TransportController::setMaxIncomingBitrate(int32_t bitrate)
    {
        SRV_LOGD("setMaxIncomingBitrate() [bitrate:%d]", bitrate);

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        nlohmann::json reqData;
        reqData["bitrate"] = bitrate;

        channel->request("transport.setMaxIncomingBitrate", _internal.transportId, reqData.dump());
    }

    void TransportController::setMaxOutgoingBitrate(int32_t bitrate)
    {
        SRV_LOGD("setMaxOutgoingBitrate() [bitrate:%d]", bitrate);

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        nlohmann::json reqData;
        reqData["bitrate"] = bitrate;

        channel->request("transport.setMaxOutgoingBitrate", _internal.transportId, reqData.dump());
    }

    void TransportController::setMinOutgoingBitrate(int32_t bitrate)
    {
        SRV_LOGD("setMinOutgoingBitrate() [bitrate:%d]", bitrate);

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        nlohmann::json reqData;
        reqData["bitrate"] = bitrate;

        channel->request("transport.setMinOutgoingBitrate", _internal.transportId, reqData.dump());
    }

    void TransportController::enableTraceEvent(const std::vector<std::string>& types)
    {
        SRV_LOGD("enableTraceEvent()");

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        nlohmann::json items = nlohmann::json::array();
        for (const auto& type : types) {
            items.emplace_back(type);
        }
        nlohmann::json reqData;
        reqData["types"] = items;
        
        
        SRV_LOGD("enableTraceEvent(): %s", reqData.dump().c_str());

        channel->request("transport.enableTraceEvent", _internal.transportId, reqData.dump());
    }

    std::shared_ptr<ProducerController> TransportController::produce(const std::shared_ptr<ProducerOptions>& options)
    {
        SRV_LOGD("produce()");
        
        std::shared_ptr<ProducerController> producerController;
        
        if (!options) {
            return producerController;
        }
        
        const std::string& id = options->id;
        const std::string& kind = options->kind;
        RtpParameters& rtpParameters = options->rtpParameters;
        const bool& paused = options->paused;
        const int& keyFrameRequestDelay = options->keyFrameRequestDelay;
        const nlohmann::json& appData = options->appData;
        
        if (_producerControllers.find(id) != _producerControllers.end()) {
            SRV_LOGE("a Producer with same id '%s' already exists", id.c_str());
            return producerController;
        }
        else if (kind != "audio" && kind != "video") {
            SRV_LOGE("invalid kind: '%s'", kind.c_str());
            return producerController;
        }
        
        // This may throw.
        nlohmann::json jrtpParameters = options->rtpParameters;
        ortc::validateRtpParameters(jrtpParameters);
        
        // If missing or empty encodings, add one.
        // if (
        //     !rtpParameters.encodings ||
        //     !Array.isArray(rtpParameters.encodings) ||
        //     rtpParameters.encodings.length === 0
        // )
        // {
        //     rtpParameters.encodings = [ {} ];
        // }

        // Don't do this in PipeTransports since there we must keep CNAME value in
        // each Producer.
        std::string constructorName = typeid(*this).name();
        if (constructorName.find("PipeTransport") == std::string::npos) {
            // If CNAME is given and we don't have yet a CNAME for Producers in this
            // Transport, take it.
            if (_cnameForProducers.empty() && !rtpParameters.rtcp.cname.empty()) {
                _cnameForProducers = rtpParameters.rtcp.cname;
            }
            // Otherwise if we don't have yet a CNAME for Producers and the RTP parameters
            // do not include CNAME, create a random one.
            else if (_cnameForProducers.empty()) {
                _cnameForProducers = uuid::uuidv4_prefix8();
            }

            // Override Producer's CNAME.
            rtpParameters.rtcp.cname = _cnameForProducers;
        }

        auto routerRtpCapabilities = _getRouterRtpCapabilities();

        // This may throw.
        auto rtpMapping = ortc::getProducerRtpParametersMapping(rtpParameters, routerRtpCapabilities);

        // This may throw.
        auto consumableRtpParameters = ortc::getConsumableRtpParameters(kind, rtpParameters, routerRtpCapabilities, rtpMapping);

        auto producerId = id.empty() ? uuid::uuidv4() : id;
        
        nlohmann::json reqData;
            reqData["producerId"] = producerId;
            reqData["kind"] = kind;
            reqData["rtpParameters"] = rtpParameters;
            reqData["rtpMapping"] = rtpMapping;
            reqData["keyFrameRequestDelay"] = keyFrameRequestDelay;
            reqData["paused"] = paused;
        
        auto channel = _channel.lock();
        if (!channel) {
            return producerController;
        }
        
        nlohmann::json status = channel->request("transport.produce", _internal.transportId, reqData.dump());
        
        ProducerData producerData;
        producerData.type = status["type"];
        producerData.kind = kind;
        producerData.rtpParameters = rtpParameters;
        // TODO: verify convert
        producerData.consumableRtpParameters = consumableRtpParameters;
        
        ProducerInternal producerInternal;
        producerInternal.producerId = producerId;
        producerInternal.transportId = _internal.transportId;
        
        {
            std::lock_guard<std::mutex> lock(_producersMutex);
            producerController = std::make_shared<ProducerController>(producerInternal,
                                                                      producerData,
                                                                      _channel.lock(),
                                                                      _payloadChannel.lock(),
                                                                      appData,
                                                                      paused);
            producerController->init();
            _producerControllers[producerController->id()] = producerController;
        }

        producerController->closeSignal.connect([id = producerController->id(), wself = std::weak_ptr<TransportController>(shared_from_this())]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            std::lock_guard<std::mutex> lock(self->_producersMutex);
            if (self->_producerControllers.find(id) != self->_producerControllers.end()) {
                auto ctrl = self->_producerControllers[id];
                self->producerCloseSignal(ctrl);
                self->_producerControllers.erase(id);
            }
        });

        this->newProducerSignal(producerController);

        return producerController;
    }

    std::shared_ptr<ConsumerController> TransportController::consume(const std::shared_ptr<ConsumerOptions>& options)
    {
        SRV_LOGD("consume()");
        
        std::shared_ptr<ConsumerController> consumerController;
        
        const std::string& producerId = options->producerId;
        const RtpCapabilities& rtpCapabilities = options->rtpCapabilities;
        const bool paused = options->paused;
        const std::string& mid = options->mid;
        const ConsumerLayers& preferredLayers = options->preferredLayers;
        const bool enableRtx = options->enableRtx;
        const bool ignoreDtx = options->ignoreDtx;
        const bool pipe = options->pipe;
        const nlohmann::json& appData = options->appData;
        
        if (producerId.empty()) {
            SRV_LOGE("missing producerId");
            return consumerController;
        }

        // This may throw.
        nlohmann::json jRtpCapabilities = rtpCapabilities;
        ortc::validateRtpCapabilities(jRtpCapabilities);

        auto producerController = _getProducerController(producerId);
        if (!producerController) {
            SRV_LOGE("Producer with id '%s' not found", producerId.c_str());
            return consumerController;
        }

        // If enableRtx is not given, set it to true if video and false if audio.
        // if (enableRtx === undefined) {
        //     enableRtx = producer.kind === 'video';
        // }
        
        // This may throw.
        auto rtpParameters = ortc::getConsumerRtpParameters(producerController->consumableRtpParameters(), rtpCapabilities, pipe, enableRtx);

        // Set MID.
        if (!pipe) {
            if (!mid.empty()) {
                rtpParameters.mid = mid;
            }
            else {
                rtpParameters.mid = std::to_string(this->_nextMidForConsumers++);

                // We use up to 8 bytes for MID (string).
                if (_nextMidForConsumers == 100000000) {
                    SRV_LOGE("consume() | reaching max MID value _nextMidForConsumers = %d", this->_nextMidForConsumers);
                    _nextMidForConsumers = 0;
                }
            }
        }

        auto channel = _channel.lock();
        if (!channel) {
            return consumerController;
        }
        
        auto consumerId = uuid::uuidv4();
        
        nlohmann::json reqData;
        reqData["consumerId"] = consumerId;
        reqData["producerId"] = producerId;
        reqData["kind"] = producerController->kind();
        reqData["rtpParameters"] = rtpParameters;
        reqData["type"] = pipe ? "pipe" : producerController->type();
        reqData["consumableRtpEncodings"] = producerController->consumableRtpParameters().encodings;
        reqData["paused"] = paused;
        reqData["preferredLayers"] = preferredLayers;
        reqData["ignoreDtx"] = ignoreDtx;

        nlohmann::json status = channel->request("transport.consume", _internal.transportId, reqData.dump());
        
        bool paused_ = status["paused"];
        bool producerPaused_ = status["producerPaused"];
        ConsumerScore score_ = status["score"];
        ConsumerLayers preferredLayers_ = status["preferredLayers"];
        
        ConsumerInternal internal;
        internal.transportId = _internal.transportId;
        internal.consumerId = consumerId;
        
        ConsumerData data;
        data.producerId = producerId;
        data.kind = producerController->kind();
        data.rtpParameters = rtpParameters;
        data.type = pipe ? "pipe" : producerController->type();
        
        {
            std::lock_guard<std::mutex> lock(_consumersMutex);
            consumerController = std::make_shared<ConsumerController>(internal,
                                                                      data,
                                                                      _channel.lock(),
                                                                      _payloadChannel.lock(),
                                                                      appData,
                                                                      paused_,
                                                                      producerPaused_,
                                                                      score_,
                                                                      preferredLayers_);
            consumerController->init();
            _consumerControllers[consumerController->id()] = consumerController;
            
            auto removeLambda = [id = consumerController->id(), wself = std::weak_ptr<TransportController>(shared_from_this())]() {
                auto self = wself.lock();
                if (!self) {
                    return;
                }
                std::lock_guard<std::mutex> lock(self->_consumersMutex);
                if (self->_consumerControllers.find(id) != self->_consumerControllers.end()) {
                    self->_consumerControllers.erase(id);
                }
            };
            
            consumerController->closeSignal.connect(removeLambda);
            consumerController->producerCloseSignal.connect(removeLambda);
            
            this->newConsumerSignal(consumerController);
        }

        return consumerController;
    }

    std::shared_ptr<DataProducerController> TransportController::produceData(const std::shared_ptr<DataProducerOptions>& options)
    {
        SRV_LOGD("produceData()");
        
        std::shared_ptr<DataProducerController> dataProducerController;
        
        if (!options) {
            return dataProducerController;
        }
        
        const std::string& id = options->id;
        const SctpStreamParameters& sctpStreamParameters = options->sctpStreamParameters;
        const std::string& label = options->label;
        const std::string& protocol = options->protocol;
        const nlohmann::json& appData = options->appData;

        if (_dataProducerControllers.find(id) != _dataProducerControllers.end()) {
            SRV_LOGE("a DataProducer with same id = %s already exists", id.c_str());
            return dataProducerController;
        }
        
        std::string type;
        std::string constructorName = typeid(*this).name();
        if (constructorName.find("DirectTransport") == std::string::npos) {
            type = "sctp";

            // This may throw.
            nlohmann::json jsctpStreamParameters = sctpStreamParameters;
            ortc::validateSctpStreamParameters(jsctpStreamParameters);
        }
        // If this is a DirectTransport, sctpStreamParameters must not be given.
        else {
            type = "direct";
            SRV_LOGW("produceData() | sctpStreamParameters are ignored when producing data on a DirectTransport");
        }

        auto channel = _channel.lock();
        if (!channel) {
            return dataProducerController;
        }
        
        auto dataProducerId = id.empty() ? uuid::uuidv4() : id;
        
        nlohmann::json reqData;
        reqData["dataProducerId"] = dataProducerId;
        reqData["type"] = type;
        reqData["sctpStreamParameters"] = sctpStreamParameters;
        reqData["label"] = label;
        reqData["protocol"] = protocol;
        
        auto data = channel->request("transport.produceData", _internal.transportId, reqData.dump());
        
        DataProducerInternal internal;
        internal.transportId = _internal.transportId;
        internal.dataProducerId = dataProducerId;
        
        DataProducerData dataProducerData;
        dataProducerData.type = data["type"];
        dataProducerData.sctpStreamParameters = data["sctpStreamParameters"];
        dataProducerData.label = data["label"];
        dataProducerData.protocol = data["protocol"];
        
        {
            std::lock_guard<std::mutex> lock(_dataProducersMutex);
            dataProducerController = std::make_shared<DataProducerController>(internal,
                                                                              dataProducerData,
                                                                              _channel.lock(),
                                                                              _payloadChannel.lock(),
                                                                              appData);
            dataProducerController->init();
            _dataProducerControllers[dataProducerController->id()] = dataProducerController;
            
            dataProducerController->closeSignal.connect([id = dataProducerController->id(), wself = std::weak_ptr<TransportController>(shared_from_this())]() {
                auto self = wself.lock();
                if (!self) {
                    return;
                }
                std::lock_guard<std::mutex> lock(self->_dataProducersMutex);
                if (self->_dataProducerControllers.find(id) != self->_dataProducerControllers.end()) {
                    auto ctrl = self->_dataProducerControllers[id];
                    self->_dataProducerControllers.erase(id);
                    self->dataProducerCloseSignal(ctrl);
                }
            });
            
            this->newDataProducerSignal(dataProducerController);
        }

        return dataProducerController;
    }

    std::shared_ptr<DataConsumerController> TransportController::consumeData(const std::shared_ptr<DataConsumerOptions>& options)
    {
        SRV_LOGD("consumeData()");
        
        std::shared_ptr<DataConsumerController> dataConsumerController;
        
        const std::string& dataProducerId = options->dataProducerId;
        const bool ordered = options->ordered;
        const int64_t maxPacketLifeTime = options->maxPacketLifeTime;
        const int32_t maxRetransmits = options->maxRetransmits;
        const nlohmann::json& appData = options->appData;
     
         if (dataProducerId.empty()) {
             SRV_LOGE("missing producerId");
             return dataConsumerController;
         }

         auto dataProducerController = _getDataProducerController(dataProducerId);

         if (!dataProducerController) {
             SRV_LOGE("dataProducer with id %s not found", dataProducerId.c_str());
             return dataConsumerController;
         }
             
         std::string type;
         SctpStreamParameters sctpStreamParameters;
         int sctpStreamId = 0;
        
        // If this is not a DirectTransport, use sctpStreamParameters from the
        // DataProducer (if type 'sctp') unless they are given in method parameters.
        std::string constructorName = typeid(*this).name();
        if (constructorName.find("DirectTransport") == std::string::npos) {
            type = "sctp";
            
            sctpStreamParameters = dataProducerController->sctpStreamParameters();

            // Override if given.
            sctpStreamParameters.ordered = ordered;

            sctpStreamParameters.maxPacketLifeTime = maxPacketLifeTime;

            sctpStreamParameters.maxRetransmits = maxRetransmits;

            // This may throw.
            sctpStreamId = getNextSctpStreamId();

            this->_sctpStreamIds[sctpStreamId] = 1;
            sctpStreamParameters.streamId = sctpStreamId;
             
        }
        // If this is a DirectTransport, sctpStreamParameters must not be used.
        else {
            type = "direct";

            SRV_LOGW("consumeData() | ordered, maxPacketLifeTime and maxRetransmits are ignored when consuming data on a DirectTransport");
        }
        
        auto channel = _channel.lock();
        if (!channel) {
            return dataConsumerController;
        }
        
        auto label = dataProducerController->label();
        auto protocol = dataProducerController->protocol();
        
        auto dataConsumerId = uuid::uuidv4();
        
        DataConsumerInternal internal;
        internal.transportId = _internal.transportId;
        internal.dataConsumerId = dataConsumerId;
        
        nlohmann::json reqData;
        reqData["dataConsumerId"] = dataConsumerId;
        reqData["dataProducerId"] = dataProducerId;
        reqData["type"] = type;
        reqData["sctpStreamParameters"] = sctpStreamParameters;
        reqData["label"] = label;
        reqData["protocol"] = protocol;

        auto data = channel->request("transport.consumeData", _internal.transportId, reqData.dump());

        DataConsumerData dataConsumerData = data;
        
        {
            std::lock_guard<std::mutex> lock(_dataConsumersMutex);
            dataConsumerController = std::make_shared<DataConsumerController>(internal,
                                                                              dataConsumerData,
                                                                              _channel.lock(),
                                                                              _payloadChannel.lock(),
                                                                              appData);
            dataConsumerController->init();
            _dataConsumerControllers[dataConsumerController->id()] = dataConsumerController;
            
            auto removeLambda = [id = dataConsumerController->id(), wself = std::weak_ptr<TransportController>(shared_from_this()), sctpStreamId]() {
                auto self = wself.lock();
                if (!self) {
                    return;
                }
                std::lock_guard<std::mutex> lock(self->_dataConsumersMutex);
                if (self->_dataConsumerControllers.find(id) != self->_dataConsumerControllers.end()) {
                    self->_dataConsumerControllers.erase(id);
                }
                if (self->_sctpStreamIds.size() != 0) {
                    self->_sctpStreamIds[sctpStreamId] = 0;
                }
            };
            
            dataConsumerController->closeSignal.connect(removeLambda);
            dataConsumerController->dataProducerCloseSignal.connect(removeLambda);
        }
        
        this->newDataConsumerSignal(dataConsumerController);

        return dataConsumerController;
    }

    int32_t TransportController::getNextSctpStreamId()
    {
        if (!this->_data["sctpParameters"] || this->_data["sctpParameters"]["MIS"] == 0) {
            SRV_LOGD("TransportData is null");
            return -1;
        }

        const int numStreams = this->_data["sctpParameters"]["MIS"];

        if (_sctpStreamIds.empty()) {
            for (int i = 0; i < numStreams; ++i) {
                _sctpStreamIds.push_back(0);
            }
        }

        int32_t sctpStreamId = 0;

        for (int32_t idx = 0; idx < (int32_t)_sctpStreamIds.size(); ++idx) {
            sctpStreamId = (_nextSctpStreamId + idx) % _sctpStreamIds.size();

            if (!_sctpStreamIds[sctpStreamId]) {
                _nextSctpStreamId = sctpStreamId + 1;
                return sctpStreamId;
            }
        }
        
        return 0;
    }

    void TransportController::clearControllers()
    {
        std::unordered_map<std::string, std::shared_ptr<ProducerController>> producerControllers;
        {
            std::lock_guard<std::mutex> lock(_producersMutex);
            producerControllers = _producerControllers;
            //_producerControllers.clear();
        }
        for (const auto& item : producerControllers) {
            auto ctrl = item.second;
            ctrl->onTransportClosed();
        }
        
        std::unordered_map<std::string, std::shared_ptr<ConsumerController>> consumerControllers;
        {
            std::lock_guard<std::mutex> lock(_consumersMutex);
            consumerControllers = _consumerControllers;
            //_consumerControllers.clear();
        }
        for (const auto& item : consumerControllers) {
            auto ctrl = item.second;
            ctrl->onTransportClosed();
        }
        
        std::unordered_map<std::string, std::shared_ptr<DataProducerController>> dataProducerControllers;
        {
            std::lock_guard<std::mutex> lock(_dataProducersMutex);
            dataProducerControllers = _dataProducerControllers;
            //_dataProducerControllers.clear();
        }
        for (const auto& item : dataProducerControllers) {
            auto ctrl = item.second;
            ctrl->onTransportClosed();
        }
        
        std::unordered_map<std::string, std::shared_ptr<DataConsumerController>> dataConsumerControllers;
        {
            std::lock_guard<std::mutex> lock(_dataConsumersMutex);
            dataConsumerControllers = _dataConsumerControllers;
            //_dataConsumerControllers.clear();
        }
        for (const auto& item : dataConsumerControllers) {
            auto ctrl = item.second;
            ctrl->onTransportClosed();
        }
    }
}


namespace srv
{
    void to_json(nlohmann::json& j, const TransportTuple& st)
    {
        j["localIp"] = st.localIp;
        j["localPort"] = st.localPort;
        j["remoteIp"] = st.remoteIp;
        j["remotePort"] = st.remotePort;
        j["protocol"] = st.protocol;
    }

    void from_json(const nlohmann::json& j, TransportTuple& st)
    {
        if (j.contains("localIp")) {
            j.at("localIp").get_to(st.localIp);
        }
        if (j.contains("localPort")) {
            j.at("localPort").get_to(st.localPort);
        }
        if (j.contains("remoteIp")) {
            j.at("remoteIp").get_to(st.remoteIp);
        }
        if (j.contains("remotePort")) {
            j.at("remotePort").get_to(st.remotePort);
        }
        if (j.contains("protocol")) {
            j.at("protocol").get_to(st.protocol);
        }
    }

    void to_json(nlohmann::json& j, const TransportTraceEventData& st)
    {
        j["type"] = st.type;
        j["timestamp"] = st.timestamp;
        j["direction"] = st.direction;
        j["info"] = st.info;
    }

    void from_json(const nlohmann::json& j, TransportTraceEventData& st)
    {
        if (j.contains("type")) {
            j.at("type").get_to(st.type);
        }
        if (j.contains("timestamp")) {
            j.at("timestamp").get_to(st.timestamp);
        }
        if (j.contains("direction")) {
            j.at("direction").get_to(st.direction);
        }
        if (j.contains("info")) {
            j.at("info").get_to(st.info);
        }
    }

    void to_json(nlohmann::json& j, const DataConsumerData& st)
    {
        j["dataProducerId"] = st.dataProducerId;
        j["type"] = st.type;
        j["sctpStreamParameters"] = st.sctpStreamParameters;
        j["label"] = st.label;
        j["protocol"] = st.protocol;
    }

    void from_json(const nlohmann::json& j, DataConsumerData& st)
    {
        if (j.contains("dataProducerId")) {
            j.at("dataProducerId").get_to(st.dataProducerId);
        }
        if (j.contains("type")) {
            j.at("type").get_to(st.type);
        }
        if (j.contains("sctpStreamParameters")) {
            j.at("sctpStreamParameters").get_to(st.sctpStreamParameters);
        }
        if (j.contains("label")) {
            j.at("label").get_to(st.label);
        }
        if (j.contains("protocol")) {
            j.at("protocol").get_to(st.protocol);
        }
    }
}
