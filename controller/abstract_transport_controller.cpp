/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "abstract_transport_controller.h"
#include "srv_logger.h"
#include "types.h"
#include "uuid.h"
#include "channel.h"
#include "ortc.h"
#include "FBS/router.h"
#include "FBS/request.h"
#include "FBS/response.h"
#include "FBS/transport.h"
#include "producer_controller.h"
#include "consumer_controller.h"
#include "data_consumer_controller.h"
#include "data_producer_controller.h"
#include "message_builder.h"

namespace srv {

    AbstractTransportController::AbstractTransportController(const std::shared_ptr<TransportConstructorOptions>& options)
    : _internal(options->internal)
    , _data(options->data)
    , _channel(options->channel)
    , _appData(options->appData)
    , _getRouterRtpCapabilities(options->getRouterRtpCapabilities)
    , _getProducerController(options->getProducerController)
    , _getDataProducerController(options->getDataProducerController)
    {
        SRV_LOGD("AbstractTransportController()");
    }

    AbstractTransportController::~AbstractTransportController()
    {
        SRV_LOGD("~AbstractTransportController()");
    }

    void AbstractTransportController::close()
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
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqOffset = FBS::Router::CreateCloseTransportRequestDirect(builder, _internal.transportId.c_str());
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.routerId, FBS::Request::Method::ROUTER_CLOSE_TRANSPORT, FBS::Request::Body::Router_CloseTransportRequest, reqOffset);
        
        channel->request(reqId, reqData);
        
        std::threadsafe_unordered_map<std::string, std::shared_ptr<IProducerController>> producerControllers = _producerControllers.value();
        producerControllers.for_each([this](const auto& item){
            auto ctrl = item.second;
            ctrl->onTransportClosed();
            this->producerCloseSignal(ctrl);
        });
        
        std::threadsafe_unordered_map<std::string, std::shared_ptr<IConsumerController>> consumerControllers = _consumerControllers.value();
        consumerControllers.for_each([](const auto& item){
            auto ctrl = item.second;
            ctrl->onTransportClosed();
        });
        
        std::threadsafe_unordered_map<std::string, std::shared_ptr<IDataProducerController>> dataProducerControllers = _dataProducerControllers.value();
        dataProducerControllers.for_each([this](const auto& item){
            auto ctrl = item.second;
            ctrl->onTransportClosed();
            this->dataProducerCloseSignal(ctrl);
        });
        
        std::threadsafe_unordered_map<std::string, std::shared_ptr<IDataConsumerController>> dataConsumerControllers = _dataConsumerControllers.value();
        dataConsumerControllers.for_each([](const auto& item){
            auto ctrl = item.second;
            ctrl->onTransportClosed();
        });
        
        this->closeSignal(id());
    }

    void AbstractTransportController::onRouterClosed()
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
        
        clearControllers();
        
        this->routerCloseSignal();
        
        this->closeSignal(id());
    }

    void AbstractTransportController::onWebRtcServerClosed()
    {
        if (_closed) {
            return;
        }
        
        SRV_LOGD("onWebRtcServerClosed()");

        _closed = true;

        // Remove notification subscriptions.
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        channel->notificationSignal.disconnect(shared_from_this());
        
        clearControllers();
        
        this->webRtcServerCloseSignal();
        
        this->closeSignal(id());
    }

    std::shared_ptr<BaseTransportDump> AbstractTransportController::dump()
    {
        assert(0);
        return nullptr;
    }

    std::shared_ptr<BaseTransportStats> AbstractTransportController::getStats()
    {
        assert(0);
        return nullptr;
    }

    void AbstractTransportController::connect(const std::shared_ptr<ConnectParams>& params)
    {
        assert(0);
    }

    void AbstractTransportController::setMaxIncomingBitrate(int32_t bitrate)
    {
        SRV_LOGD("setMaxIncomingBitrate() [bitrate:%d]", bitrate);

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqOffset = FBS::Transport::CreateSetMaxIncomingBitrateRequest(builder, bitrate);
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::TRANSPORT_SET_MAX_INCOMING_BITRATE,
                                                     FBS::Request::Body::Transport_SetMaxIncomingBitrateRequest,
                                                     reqOffset);
                         
        channel->request(reqId, reqData);
    }

    void AbstractTransportController::setMaxOutgoingBitrate(int32_t bitrate)
    {
        SRV_LOGD("setMaxOutgoingBitrate() [bitrate:%d]", bitrate);

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqOffset = FBS::Transport::CreateSetMaxOutgoingBitrateRequest(builder, bitrate);
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::TRANSPORT_SET_MAX_OUTGOING_BITRATE,
                                                     FBS::Request::Body::Transport_SetMaxOutgoingBitrateRequest,
                                                     reqOffset);
        
        channel->request(reqId, reqData);
    }

    void AbstractTransportController::setMinOutgoingBitrate(int32_t bitrate)
    {
        SRV_LOGD("setMinOutgoingBitrate() [bitrate:%d]", bitrate);

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqOffset = FBS::Transport::CreateSetMinOutgoingBitrateRequest(builder, bitrate);
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::TRANSPORT_SET_MIN_OUTGOING_BITRATE,
                                                     FBS::Request::Body::Transport_SetMinOutgoingBitrateRequest,
                                                     reqOffset);
        
        channel->request(reqId, reqData);
    }

    void AbstractTransportController::enableTraceEvent(const std::vector<std::string>& types)
    {
        SRV_LOGD("enableTraceEvent()");

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }

        std::vector<FBS::Transport::TraceEventType> events;
        
        for (const auto& type : types) {
            events.emplace_back(transportTraceEventTypeToFbs(type));
        }
        
        flatbuffers::FlatBufferBuilder builder;
            
        auto reqOffset = FBS::Transport::CreateEnableTraceEventRequestDirect(builder, &events);
        
        auto reqId = channel->genRequestId();
            
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::TRANSPORT_ENABLE_TRACE_EVENT,
                                                     FBS::Request::Body::Transport_EnableTraceEventRequest,
                                                     reqOffset);
            
        channel->request(reqId, reqData);
    }

    std::shared_ptr<IProducerController> AbstractTransportController::produce(const std::shared_ptr<ProducerOptions>& options)
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
        
        if (_producerControllers.contains(id)) {
            SRV_LOGE("a Producer with same id '%s' already exists", id.c_str());
            return producerController;
        }
        else if (kind != "audio" && kind != "video") {
            SRV_LOGE("invalid kind: '%s'", kind.c_str());
            return producerController;
        }
        
        nlohmann::json parameters = options->rtpParameters;
        
        // This may throw.
        ortc::validateRtpParameters(parameters);
        
        // TODO: check
        options->rtpParameters = parameters;
        
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
        
        nlohmann::json jsonRtpMapping;
        jsonRtpMapping["rtpMapping"] = rtpMapping;
        
        // This may throw.
        auto consumableRtpParameters = ortc::getConsumableRtpParameters(kind, rtpParameters, routerRtpCapabilities, rtpMapping);

        // TODO: convert json to RtpMappingFbs
        RtpMappingFbs rtpMappingFbs;
        convert(jsonRtpMapping, rtpMappingFbs);
        
        auto producerId = id.empty() ? uuid::uuidv4() : id;
        
        auto channel = _channel.lock();
        if (!channel) {
            return producerController;
        }
        
        flatbuffers::FlatBufferBuilder builder;
            
        auto reqOffset = createProduceRequest(builder, producerId, kind, rtpParameters, rtpMappingFbs, keyFrameRequestDelay, paused);

        auto reqId = channel->genRequestId();
            
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::TRANSPORT_PRODUCE,
                                                     FBS::Request::Body::Transport_ProduceRequest,
                                                     reqOffset);
        
        auto respData = channel->request(reqId, reqData);
            
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto stats = response->body_as_Transport_ProduceResponse();
        
        ProducerData producerData;
        producerData.type = producerTypeFromFbs(stats->type());
        producerData.kind = kind;
        producerData.rtpParameters = rtpParameters;
        // TODO: verify convert
        producerData.consumableRtpParameters = consumableRtpParameters;
        
        ProducerInternal producerInternal;
        producerInternal.producerId = producerId;
        producerInternal.transportId = _internal.transportId;
        
        producerController = std::make_shared<ProducerController>(producerInternal,
                                                                  producerData,
                                                                  _channel.lock(),
                                                                  appData,
                                                                  paused);
        producerController->init();
        _producerControllers.emplace(std::make_pair(producerController->id(), producerController));

        producerController->closeSignal.connect([id = producerController->id(), wself = std::weak_ptr<AbstractTransportController>(shared_from_this())]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (self->_producerControllers.contains(id)) {
                auto ctrl = self->_producerControllers[id];
                self->producerCloseSignal(ctrl);
                self->_producerControllers.erase(id);
            }
        });

        this->newProducerSignal(producerController);

        return producerController;
    }

    std::shared_ptr<IConsumerController> AbstractTransportController::consume(const std::shared_ptr<ConsumerOptions>& options)
    {
        SRV_LOGD("consume()");
        
        std::shared_ptr<ConsumerController> consumerController;
        
        const std::string& producerId = options->producerId;
        RtpCapabilities rtpCapabilities = options->rtpCapabilities;
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

        nlohmann::json capablities = rtpCapabilities;
        // This may throw.
        ortc::validateRtpCapabilities(capablities);
        
        // TODO:
        rtpCapabilities = capablities;

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
        
        flatbuffers::FlatBufferBuilder builder;
            
        auto reqOffset = createConsumeRequest(builder, producerController, consumerId, rtpParameters, paused, preferredLayers, ignoreDtx, pipe);
        
        auto reqId = channel->genRequestId();
            
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::TRANSPORT_CONSUME,
                                                     FBS::Request::Body::Transport_ConsumeRequest,
                                                     reqOffset);
        
        auto respData = channel->request(reqId, reqData);
            
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto stats = response->body_as_Transport_ConsumeResponse();
        
        bool paused_ = stats->paused();
        bool producerPaused_ = stats->producerPaused();
        
        ConsumerScore score_;
        score_.score = stats->score()->score();
        score_.producerScore = stats->score()->producerScore();
        for (const auto& item : *stats->score()->producerScores()) {
            score_.producerScores.emplace_back(item);
        }
        
        ConsumerLayers preferredLayers_;
        if (auto layers = stats->preferredLayers()) {
            preferredLayers_.spatialLayer = layers->spatialLayer();
            preferredLayers_.temporalLayer = layers->temporalLayer().value_or(0);
        }
        
        ConsumerInternal internal;
        internal.transportId = _internal.transportId;
        internal.consumerId = consumerId;
        
        ConsumerData data;
        data.producerId = producerId;
        data.kind = producerController->kind();
        data.rtpParameters = rtpParameters;
        data.type = pipe ? "pipe" : producerController->type();
        
        consumerController = std::make_shared<ConsumerController>(internal,
                                                                  data,
                                                                  _channel.lock(),
                                                                  appData,
                                                                  paused_,
                                                                  producerPaused_,
                                                                  score_,
                                                                  preferredLayers_);
        consumerController->init();
        _consumerControllers.emplace(std::make_pair(consumerController->id(), consumerController));
        
        auto removeLambda = [id = consumerController->id(), wself = std::weak_ptr<AbstractTransportController>(shared_from_this())]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (self->_consumerControllers.contains(id)) {
                self->_consumerControllers.erase(id);
            }
        };
        
        consumerController->closeSignal.connect(removeLambda);
        consumerController->producerCloseSignal.connect(removeLambda);
        
        this->newConsumerSignal(consumerController);

        return consumerController;
    }

    std::shared_ptr<IDataProducerController> AbstractTransportController::produceData(const std::shared_ptr<DataProducerOptions>& options)
    {
        SRV_LOGD("produceData()");
        
        std::shared_ptr<DataProducerController> dataProducerController;
        
        if (!options) {
            return dataProducerController;
        }
        
        const std::string& id = options->id;
        SctpStreamParameters sctpStreamParameters = options->sctpStreamParameters;
        nlohmann::json jsctpStreamParameters = sctpStreamParameters;
        SRV_LOGD("sctpStreamParameters: %s", jsctpStreamParameters.dump().c_str());
        
        const std::string& label = options->label;
        const std::string& protocol = options->protocol;
        const bool paused = options->paused;
        const nlohmann::json& appData = options->appData;

        if (_dataProducerControllers.contains(id)) {
            SRV_LOGE("a DataProducer with same id = %s already exists", id.c_str());
            return dataProducerController;
        }
        
        std::string type;
        std::string constructorName = typeid(*this).name();
        if (constructorName.find("DirectTransport") == std::string::npos) {
            type = "sctp";

            nlohmann::json parameters = sctpStreamParameters;
            if (sctpStreamParameters.maxPacketLifeTime == 0) {
                parameters.erase("maxPacketLifeTime");
            }
            
            if (sctpStreamParameters.maxRetransmits == 0) {
                parameters.erase("maxRetransmits");
            }
            
            // This may throw.
            ortc::validateSctpStreamParameters(parameters);
            
            sctpStreamParameters = parameters;
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
        
        flatbuffers::FlatBufferBuilder builder;
            
        auto reqOffset = createProduceDataRequest(builder, dataProducerId, type, sctpStreamParameters, label, protocol, paused);
        
        auto reqId = channel->genRequestId();
            
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::TRANSPORT_PRODUCE_DATA,
                                                     FBS::Request::Body::Transport_ProduceDataRequest,
                                                     reqOffset);
        
        auto respData = channel->request(reqId, reqData);
            
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto dump = response->body_as_DataProducer_DumpResponse();
        
        DataProducerInternal internal;
        internal.transportId = _internal.transportId;
        internal.dataProducerId = dataProducerId;
        
        DataProducerData dataProducerData;
        dataProducerData.type = dataProducerTypeFromFbs(dump->type());
        dataProducerData.sctpStreamParameters = *parseSctpStreamParameters(dump->sctpStreamParameters());
        dataProducerData.label = dump->label()->str();
        dataProducerData.protocol = dump->protocol()->str();
        
        dataProducerController = std::make_shared<DataProducerController>(internal,
                                                                          dataProducerData,
                                                                          _channel.lock(),
                                                                          paused,
                                                                          appData);
        dataProducerController->init();
        _dataProducerControllers.emplace(std::make_pair(dataProducerController->id(), dataProducerController));
        
        dataProducerController->closeSignal.connect([id = dataProducerController->id(), wself = std::weak_ptr<AbstractTransportController>(shared_from_this())]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (self->_dataProducerControllers.contains(id)) {
                auto ctrl = self->_dataProducerControllers[id];
                self->_dataProducerControllers.erase(id);
                self->dataProducerCloseSignal(ctrl);
            }
        });
        
        this->newDataProducerSignal(dataProducerController);

        return dataProducerController;
    }

    std::shared_ptr<IDataConsumerController> AbstractTransportController::consumeData(const std::shared_ptr<DataConsumerOptions>& options)
    {
        SRV_LOGD("consumeData()");
        
        std::shared_ptr<DataConsumerController> dataConsumerController;
        
        const std::string& dataProducerId = options->dataProducerId;
        const bool ordered = options->ordered;
        const int64_t maxPacketLifeTime = options->maxPacketLifeTime;
        const int32_t maxRetransmits = options->maxRetransmits;
        const bool paused = options->paused;
        const auto& subchannels = options->subchannels;
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
        
        flatbuffers::FlatBufferBuilder builder;
            
        auto reqOffset = createConsumeDataRequest(builder, dataConsumerId, dataProducerId, type, sctpStreamParameters, label, protocol, paused, subchannels);

        auto reqId = channel->genRequestId();
            
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::TRANSPORT_CONSUME_DATA,
                                                     FBS::Request::Body::Transport_ConsumeDataRequest,
                                                     reqOffset);
        
        auto respData = channel->request(reqId, reqData);
            
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto dump = response->body_as_DataConsumer_DumpResponse();
        
        auto dataConsumerDump = parseDataConsumerDumpResponse(dump);
        
        DataConsumerData dataConsumerData;
        dataConsumerData.dataProducerId = dataConsumerDump->dataProducerId;
        dataConsumerData.type = dataConsumerDump->type;
        dataConsumerData.sctpStreamParameters = dataConsumerDump->sctpStreamParameters;
        dataConsumerData.label = dataConsumerDump->label;
        dataConsumerData.protocol = dataConsumerDump->protocol;
        dataConsumerData.bufferedAmountLowThreshold = dataConsumerDump->bufferedAmountLowThreshold;
        
        dataConsumerController = std::make_shared<DataConsumerController>(internal,
                                                                          dataConsumerData,
                                                                          _channel.lock(),
                                                                          paused,
                                                                          dataConsumerDump->dataProducerPaused,
                                                                          subchannels,
                                                                          appData);
        dataConsumerController->init();
        _dataConsumerControllers.emplace(std::make_pair(dataConsumerController->id(), dataConsumerController));
        
        auto removeLambda = [id = dataConsumerController->id(), wself = std::weak_ptr<AbstractTransportController>(shared_from_this()), sctpStreamId]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (self->_dataConsumerControllers.contains(id)) {
                self->_dataConsumerControllers.erase(id);
            }
            if (self->_sctpStreamIds.size() != 0) {
                self->_sctpStreamIds[sctpStreamId] = 0;
            }
        };
        
        dataConsumerController->closeSignal.connect(removeLambda);
        dataConsumerController->dataProducerCloseSignal.connect(removeLambda);
        
        this->newDataConsumerSignal(dataConsumerController);

        return dataConsumerController;
    }

    int32_t AbstractTransportController::getNextSctpStreamId()
    {
        if (!this->_data || this->_data->sctpParameters.MIS == 0) {
            SRV_LOGD("TransportData is null");
            return -1;
        }

        const int numStreams = this->_data->sctpParameters.MIS;

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

    void AbstractTransportController::clearControllers()
    {
        std::threadsafe_unordered_map<std::string, std::shared_ptr<IProducerController>> producerControllers = _producerControllers.value();
        producerControllers.for_each([](const auto& item){
            auto ctrl = item.second;
            ctrl->onTransportClosed();
        });
        
        std::threadsafe_unordered_map<std::string, std::shared_ptr<IConsumerController>> consumerControllers = _consumerControllers.value();
        consumerControllers.for_each([](const auto& item){
            auto ctrl = item.second;
            ctrl->onTransportClosed();
        });
        
        std::threadsafe_unordered_map<std::string, std::shared_ptr<IDataProducerController>> dataProducerControllers = _dataProducerControllers.value();
        dataProducerControllers.for_each([](const auto& item){
            auto ctrl = item.second;
            ctrl->onTransportClosed();
        });
        
        std::threadsafe_unordered_map<std::string, std::shared_ptr<IDataConsumerController>> dataConsumerControllers = _dataConsumerControllers.value();
        dataConsumerControllers.for_each([](const auto& item){
            auto ctrl = item.second;
            ctrl->onTransportClosed();
        });
    }
}

namespace srv {

    FBS::Transport::TraceEventType transportTraceEventTypeToFbs(const std::string& eventType)
    {
        if (eventType == "probation") {
            return FBS::Transport::TraceEventType::PROBATION;
        }
        else if (eventType == "bwe") {
            return FBS::Transport::TraceEventType::BWE;
        }
        else {
            SRV_LOGE("invalid TransportTraceEventType: %s}", eventType.c_str());
            return FBS::Transport::TraceEventType::MIN;
        }
    }

    std::string transportTraceEventTypeFromFbs(FBS::Transport::TraceEventType eventType)
    {
        switch (eventType)
        {
            case FBS::Transport::TraceEventType::PROBATION:
                return "probation";
            case FBS::Transport::TraceEventType::BWE:
                return "bwe";
            default:
                return "";
        }
    }

    std::string parseSctpState(FBS::SctpAssociation::SctpState fbsSctpState)
    {
        switch (fbsSctpState)
        {
            case FBS::SctpAssociation::SctpState::NEW:
                return "new";
            case FBS::SctpAssociation::SctpState::CONNECTING:
                return "connecting";
            case FBS::SctpAssociation::SctpState::CONNECTED:
                return "connected";
            case FBS::SctpAssociation::SctpState::FAILED:
                return "failed";
            case FBS::SctpAssociation::SctpState::CLOSED:
                return "closed";
            default:
                SRV_LOGE("invalid SctpState: %u", (uint8_t)fbsSctpState);
                return "";
        }
    }

    std::string parseProtocol(FBS::Transport::Protocol protocol)
    {
        switch (protocol)
        {
            case FBS::Transport::Protocol::UDP:
                return "udp";

            case FBS::Transport::Protocol::TCP:
                return "tcp";
            default:
                SRV_LOGE("invalid protocol: %u", (uint8_t)protocol);
                return "";
        }
    }

    FBS::Transport::Protocol serializeProtocol(const std::string& protocol)
    {
        if (protocol == "udp") {
            return FBS::Transport::Protocol::UDP;
        }
        if (protocol == "tcp") {
            return FBS::Transport::Protocol::TCP;
        }
        else {
            SRV_LOGE("invalid protocol: %s}", protocol.c_str());
            return FBS::Transport::Protocol::MIN;
        }
    }

    std::shared_ptr<TransportTuple> parseTuple(const FBS::Transport::Tuple* binary)
    {
        auto tuple = std::make_shared<TransportTuple>();
        
        tuple->localAddress = binary->localAddress()->str();
        tuple->localPort = binary->localPort();
        tuple->remoteIp = binary->remoteIp()->str();
        tuple->remotePort = binary->remotePort();
        tuple->protocol = parseProtocol(binary->protocol());
        
        return tuple;
    }

    std::shared_ptr<BaseTransportDump> parseBaseTransportDump(const FBS::Transport::Dump* binary)
    {
        auto dump = std::make_shared<BaseTransportDump>();
        
        dump->id = binary->id()->str();
        dump->direct = binary->direct();
        
        for (const auto& id : *binary->producerIds()) {
            dump->producerIds.emplace_back(id->str());
        }
        
        for (const auto& id : *binary->consumerIds()) {
            dump->consumerIds.emplace_back(id->str());
        }
        
        for (const auto& id : *binary->mapSsrcConsumerId()) {
            dump->mapSsrcConsumerId.emplace_back(std::pair<uint32_t, std::string>(id->key(), id->value()->str()));
        }
        
        for (const auto& id : *binary->mapRtxSsrcConsumerId()) {
            dump->mapRtxSsrcConsumerId.emplace_back(std::pair<uint32_t, std::string>(id->key(), id->value()->str()));
        }
        
        dump->recvRtpHeaderExtensions = *parseRecvRtpHeaderExtensions(binary->recvRtpHeaderExtensions());
        dump->rtpListener = *parseRtpListenerDump(binary->rtpListener());
        dump->maxMessageSize = binary->maxMessageSize();
        
        for (const auto& id : *binary->dataProducerIds()) {
            dump->dataProducerIds.emplace_back(id->str());
        }
        
        for (const auto& id : *binary->dataConsumerIds()) {
            dump->dataConsumerIds.emplace_back(id->str());
        }
        
        if (auto parameters = binary->sctpParameters()) {
            auto sctpParametersDump = parseSctpParametersDump(parameters);
            dump->sctpParameters.port = sctpParametersDump->port;
            dump->sctpParameters.OS = sctpParametersDump->OS;
            dump->sctpParameters.MIS = sctpParametersDump->MIS;
            dump->sctpParameters.maxMessageSize = sctpParametersDump->maxMessageSize;
        }
        
        if (const auto& state = binary->sctpState()) {
            dump->sctpState = parseSctpState(state.value_or(FBS::SctpAssociation::SctpState::CLOSED));
        }
        
        if (const auto& listener = binary->sctpListener()) {
            dump->sctpListener = *parseSctpListenerDump(listener);
        }
        
        for (const auto& type : *binary->traceEventTypes()) {
            std::string t = type == FBS::Transport::TraceEventType::PROBATION ? "probation" : "bwe";
            dump->traceEventTypes.emplace_back(t);
        }
        
        return dump;
    }

    std::shared_ptr<BaseTransportStats> parseBaseTransportStats(const FBS::Transport::Stats* binary)
    {
        auto stats = std::make_shared<BaseTransportStats>();
        
        stats->transportId = binary->transportId()->str();
        stats->timestamp = binary->timestamp();
        stats->sctpState = parseSctpState(binary->sctpState().value_or(FBS::SctpAssociation::SctpState::CLOSED));
        stats->bytesReceived = binary->bytesReceived();
        stats->recvBitrate = binary->recvBitrate();
        stats->bytesSent = binary->bytesSent();
        stats->sendBitrate = binary->sendBitrate();
        stats->rtpBytesReceived = binary->rtpBytesReceived();
        stats->rtpRecvBitrate = binary->rtpRecvBitrate();
        stats->rtpBytesSent = binary->rtpBytesSent();
        stats->rtpSendBitrate = binary->rtpSendBitrate();
        stats->rtxBytesReceived = binary->rtxBytesReceived();
        stats->rtxRecvBitrate = binary->rtxRecvBitrate();
        stats->rtxBytesSent = binary->rtxBytesSent();
        stats->rtxSendBitrate = binary->rtxSendBitrate();
        stats->probationBytesSent = binary->probationBytesSent();
        stats->probationSendBitrate = binary->probationSendBitrate();
        stats->availableOutgoingBitrate = binary->availableOutgoingBitrate().value_or(0);
        stats->availableIncomingBitrate = binary->availableIncomingBitrate().value_or(0);
        stats->maxIncomingBitrate = binary->maxIncomingBitrate().value_or(0);
        
        return stats;
    }

    std::shared_ptr<TransportTraceEventData> parseTransportTraceEventData(const FBS::Transport::TraceNotification* trace)
    {
        auto eventData = std::make_shared<TransportTraceEventData>();
        
        eventData->timestamp = trace->timestamp();
        eventData->direction = trace->direction() == FBS::Common::TraceDirection::DIRECTION_IN ? "in" : "out";
        
        switch (trace->type()) {
            case FBS::Transport::TraceEventType::BWE: {
                eventData->type = "bwe";
                if (auto info = trace->info_as_BweTraceInfo()) {
                    eventData->info = parseBweTraceInfo(info);
                }
                break;
            }
            case FBS::Transport::TraceEventType::PROBATION: {
                eventData->type = "probation";
                break;
            }
            default:
                break;
        }
        
        return eventData;
    }

    std::shared_ptr<RecvRtpHeaderExtensions> parseRecvRtpHeaderExtensions(const FBS::Transport::RecvRtpHeaderExtensions* binary)
    {
        auto extensions = std::make_shared<RecvRtpHeaderExtensions>();
        
        if (binary->mid()) {
            extensions->mid = binary->mid().value();
        }
        
        if (binary->rid()) {
            extensions->rid = binary->rid().value();
        }
        
        if (binary->rrid()) {
            extensions->rrid = binary->rrid().value();
        }
        
        if (binary->absSendTime()) {
            extensions->absSendTime = binary->absSendTime().value();
        }
        
        if (binary->transportWideCc01()) {
            extensions->transportWideCc01 = binary->transportWideCc01().value();
        }
        
        return extensions;
    }

    std::shared_ptr<BweTraceInfo> parseBweTraceInfo(const FBS::Transport::BweTraceInfo* binary)
    {
        auto info = std::make_shared<BweTraceInfo>();
        
        info->bweType = binary->bweType() == FBS::Transport::BweType::TRANSPORT_CC ? "transport-cc'" : "remb";
        info->desiredBitrate = binary->desiredBitrate();
        info->effectiveDesiredBitrate = binary->effectiveDesiredBitrate();
        info->minBitrate = binary->minBitrate();
        info->maxBitrate = binary->maxBitrate();
        info->startBitrate = binary->startBitrate();
        info->maxPaddingBitrate = binary->maxPaddingBitrate();
        info->availableBitrate = binary->availableBitrate();
        
        return info;
    }

    flatbuffers::Offset<FBS::Transport::ConsumeRequest> createConsumeRequest(flatbuffers::FlatBufferBuilder& builder,
                                                                             const std::shared_ptr<IProducerController>& producer,
                                                                             const std::string& consumerId,
                                                                             const RtpParameters& rtpParameters,
                                                                             bool paused,
                                                                             const ConsumerLayers& preferredLayers,
                                                                             bool ignoreDtx,
                                                                             bool pipe)
    {
        auto rtpParametersOffset = rtpParameters.serialize(builder);
        
        std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpEncodingParameters>> consumableRtpEncodings;
        for (const auto& encoding : producer->consumableRtpParameters().encodings) {
            consumableRtpEncodings.emplace_back(encoding.serialize(builder));
        }
        
        auto preferredLayersOffset = FBS::Consumer::CreateConsumerLayers(builder, preferredLayers.spatialLayer, preferredLayers.temporalLayer);
        
        FBS::RtpParameters::MediaKind kind = producer->kind() == "audio" ? FBS::RtpParameters::MediaKind::AUDIO : FBS::RtpParameters::MediaKind::VIDEO;
        
        FBS::RtpParameters::Type type = producerTypeToFbs(producer->type());
        
        auto reqOffset = FBS::Transport::CreateConsumeRequestDirect(builder,
                                                                    consumerId.c_str(),
                                                                    producer->id().c_str(),
                                                                    kind,
                                                                    rtpParametersOffset,
                                                                    type,
                                                                    &consumableRtpEncodings,
                                                                    paused,
                                                                    preferredLayersOffset,
                                                                    ignoreDtx
                                                                    );
        return reqOffset;
    }

    flatbuffers::Offset<FBS::Transport::ProduceRequest> createProduceRequest(flatbuffers::FlatBufferBuilder& builder,
                                                                             const std::string& producerId,
                                                                             const std::string& kind,
                                                                             const RtpParameters& rtpParameters,
                                                                             const RtpMappingFbs& rtpMapping,
                                                                             uint32_t keyFrameRequestDelay,
                                                                             bool paused)
    {
        auto rtpParametersOffset = rtpParameters.serialize(builder);
        
        FBS::RtpParameters::MediaKind mediaKind = kind == "audio" ? FBS::RtpParameters::MediaKind::AUDIO : FBS::RtpParameters::MediaKind::VIDEO;
        
        auto rtpMappingOffset = rtpMapping.serialize(builder);
        
        auto reqOffset = FBS::Transport::CreateProduceRequestDirect(builder,
                                                                    producerId.c_str(),
                                                                    mediaKind,
                                                                    rtpParametersOffset,
                                                                    rtpMappingOffset,
                                                                    keyFrameRequestDelay,
                                                                    paused
                                                                    );
        return reqOffset;
    }

    flatbuffers::Offset<FBS::Transport::ConsumeDataRequest> createConsumeDataRequest(flatbuffers::FlatBufferBuilder& builder,
                                                                                     const std::string& dataConsumerId,
                                                                                     const std::string& dataProducerId,
                                                                                     const std::string& type,
                                                                                     const SctpStreamParameters& sctpStreamParameters,
                                                                                     const std::string& label,
                                                                                     const std::string& protocol,
                                                                                     bool paused,
                                                                                     const std::vector<uint16_t>& subchannels)
    {
        FBS::DataProducer::Type typeFbs = dataConsumerTypeToFbs(type);
        
        auto sctpStreamParametersOffset = sctpStreamParameters.serialize(builder);
        
        auto reqOffset = FBS::Transport::CreateConsumeDataRequestDirect(builder,
                                                                        dataConsumerId.c_str(),
                                                                        dataProducerId.c_str(),
                                                                        typeFbs,
                                                                        sctpStreamParametersOffset,
                                                                        label.c_str(),
                                                                        protocol.c_str(),
                                                                        paused,
                                                                        &subchannels
                                                                        );
        
        return reqOffset;
    }

    flatbuffers::Offset<FBS::Transport::ProduceDataRequest> createProduceDataRequest(flatbuffers::FlatBufferBuilder& builder,
                                                                                     const std::string& dataProducerId,
                                                                                     const std::string& type,
                                                                                     const SctpStreamParameters& sctpStreamParameters,
                                                                                     const std::string& label,
                                                                                     const std::string& protocol,
                                                                                     bool paused)
    {
        FBS::DataProducer::Type typeFbs = dataProducerTypeToFbs(type);
        
        auto sctpStreamParametersOffset = sctpStreamParameters.serialize(builder);
        
        auto reqOffset = FBS::Transport::CreateProduceDataRequestDirect(builder,
                                                                        dataProducerId.c_str(),
                                                                        typeFbs,
                                                                        sctpStreamParametersOffset,
                                                                        label.c_str(),
                                                                        protocol.c_str(),
                                                                        paused
                                                                        );
        return reqOffset;
    }

    std::shared_ptr<RtpListenerDump> parseRtpListenerDump(const FBS::Transport::RtpListener* binary)
    {
        auto dump = std::make_shared<RtpListenerDump>();

        for (const auto& item : *binary->ssrcTable()) {
            dump->ssrcTable.emplace(std::pair<uint16_t, std::string>(item->key(), item->value()->str()));
        }
        
        for (const auto& item : *binary->midTable()) {
            dump->midTable.emplace(std::pair<std::string, std::string>(item->key()->str(), item->value()->str()));
        }
        
        for (const auto& item : *binary->ridTable()) {
            dump->ridTable.emplace(std::pair<std::string, std::string>(item->key()->str(), item->value()->str()));
        }
        
        return dump;
    }

    std::shared_ptr<SctpListenerDump> parseSctpListenerDump(const FBS::Transport::SctpListener* binary)
    {
        auto dump = std::make_shared<SctpListenerDump>();
        
        for (const auto& item : *binary->streamIdTable()) {
            dump->streamIdTable.emplace(std::pair<uint16_t, std::string>(item->key(), item->value()->str()));
        }
        
        return dump;
    }

    void to_json(nlohmann::json& j, const TransportPortRange& st)
    {
        j["min"] = st.min;
        j["max"] = st.max;
    }

    void from_json(const nlohmann::json& j, TransportPortRange& st)
    {
        if (j.contains("min")) {
            j.at("min").get_to(st.min);
        }
        if (j.contains("max")) {
            j.at("max").get_to(st.max);
        }
    }

    //void to_json(nlohmann::json& j, const TransportListenIp& st)
    //{
    //    j["ip"] = st.ip;
    //    j["announcedIp"] = st.announcedIp;
    //}

    //void from_json(const nlohmann::json& j, TransportListenIp& st)
    //{
    //    if (j.contains("ip")) {
    //        j.at("ip").get_to(st.ip);
    //    }
    //    if (j.contains("announcedIp")) {
    //        j.at("announcedIp").get_to(st.announcedIp);
    //    }
    //}

    void to_json(nlohmann::json& j, const TransportListenInfo& st)
    {
        j["protocol"] = st.protocol;
        j["ip"] = st.ip;
        j["announcedIp"] = st.announcedIp;
        j["announcedAddress"] = st.announcedAddress;
        j["portRange"] = st.portRange;
        j["port"] = st.port;
        j["sendBufferSize"] = st.sendBufferSize;
        j["recvBufferSize"] = st.recvBufferSize;
    }

    void from_json(const nlohmann::json& j, TransportListenInfo& st)
    {
        if (j.contains("protocol")) {
            j.at("protocol").get_to(st.protocol);
        }
        if (j.contains("ip")) {
            j.at("ip").get_to(st.ip);
        }
        if (j.contains("announcedIp")) {
            j.at("announcedIp").get_to(st.announcedIp);
        }
        if (j.contains("announcedAddress")) {
            j.at("announcedAddress").get_to(st.announcedAddress);
        }
        if (j.contains("port")) {
            j.at("port").get_to(st.port);
        }
        if (j.contains("portRange")) {
            j.at("portRange").get_to(st.portRange);
        }
        if (j.contains("sendBufferSize")) {
            j.at("sendBufferSize").get_to(st.sendBufferSize);
        }
        if (j.contains("recvBufferSize")) {
            j.at("recvBufferSize").get_to(st.recvBufferSize);
        }
    }

    void to_json(nlohmann::json& j, const TransportTraceEventData& st)
    {
        j["type"] = st.type;
        j["timestamp"] = st.timestamp;
        j["direction"] = st.direction;
        //j["info"] = st.info;
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
        //if (j.contains("info")) {
        //    j.at("info").get_to(st.info);
        //}
    }

    void to_json(nlohmann::json& j, const DtlsFingerprint& st)
    {
        j["algorithm"] = st.algorithm;
        j["value"] = st.value;
    }

    void from_json(const nlohmann::json& j, DtlsFingerprint& st)
    {
        if (j.contains("algorithm")) {
            j.at("algorithm").get_to(st.algorithm);
        }
        if (j.contains("value")) {
            j.at("value").get_to(st.value);
        }
    }

    void to_json(nlohmann::json& j, const DtlsParameters& st)
    {
        j["role"] = st.role;
        j["fingerprints"] = st.fingerprints;
    }

    void from_json(const nlohmann::json& j, DtlsParameters& st)
    {
        if (j.contains("role")) {
            j.at("role").get_to(st.role);
        }
        if (j.contains("fingerprints")) {
            j.at("fingerprints").get_to(st.fingerprints);
        }
    }

    void to_json(nlohmann::json& j, const BaseTransportStats& st)
     {
         j["transportId"] = st.transportId;
         j["timestamp"] = st.timestamp;

         j["bytesReceived"] = st.bytesReceived;
         j["recvBitrate"] = st.recvBitrate;
         j["bytesSent"] = st.bytesSent;
         j["sendBitrate"] = st.sendBitrate;
         j["rtpBytesReceived"] = st.rtpBytesReceived;
         j["rtpRecvBitrate"] = st.rtpRecvBitrate;

         j["rtpBytesSent"] = st.rtpBytesSent;
         j["rtpSendBitrate"] = st.rtpSendBitrate;
         j["rtxBytesReceived"] = st.rtxBytesReceived;
         j["rtxRecvBitrate"] = st.rtxRecvBitrate;
         j["rtxBytesSent"] = st.rtxBytesSent;

         j["rtxSendBitrate"] = st.rtxSendBitrate;
         j["probationBytesSent"] = st.probationBytesSent;
         j["probationSendBitrate"] = st.probationSendBitrate;
         j["availableOutgoingBitrate"] = st.availableOutgoingBitrate;
         j["availableIncomingBitrate"] = st.availableIncomingBitrate;
         j["maxIncomingBitrate"] = st.maxIncomingBitrate;
     }

    void from_json(const nlohmann::json& j, BaseTransportStats& st)
     {
         if (j.contains("transportId")) {
             j.at("transportId").get_to(st.transportId);
         }
         if (j.contains("timestamp")) {
             j.at("timestamp").get_to(st.timestamp);
         }
         
         if (j.contains("bytesReceived")) {
             j.at("bytesReceived").get_to(st.bytesReceived);
         }
         if (j.contains("recvBitrate")) {
             j.at("recvBitrate").get_to(st.recvBitrate);
         }
         if (j.contains("bytesSent")) {
             j.at("bytesSent").get_to(st.bytesSent);
         }
         if (j.contains("sendBitrate")) {
             j.at("sendBitrate").get_to(st.sendBitrate);
         }
         if (j.contains("rtpBytesReceived")) {
             j.at("rtpBytesReceived").get_to(st.rtpBytesReceived);
         }
         if (j.contains("rtpRecvBitrate")) {
             j.at("rtpRecvBitrate").get_to(st.rtpRecvBitrate);
         }
         if (j.contains("rtpBytesSent")) {
             j.at("rtpBytesSent").get_to(st.rtpBytesSent);
         }
         if (j.contains("rtpSendBitrate")) {
             j.at("rtpSendBitrate").get_to(st.rtpSendBitrate);
         }
         if (j.contains("rtxBytesReceived")) {
             j.at("rtxBytesReceived").get_to(st.rtxBytesReceived);
         }
         if (j.contains("rtxRecvBitrate")) {
             j.at("rtxRecvBitrate").get_to(st.rtxRecvBitrate);
         }
         if (j.contains("rtxBytesSent")) {
             j.at("rtxBytesSent").get_to(st.rtxBytesSent);
         }
         if (j.contains("rtxSendBitrate")) {
             j.at("rtxSendBitrate").get_to(st.rtxSendBitrate);
         }
         if (j.contains("probationBytesSent")) {
             j.at("probationBytesSent").get_to(st.probationBytesSent);
         }
         if (j.contains("probationSendBitrate")) {
             j.at("probationSendBitrate").get_to(st.probationSendBitrate);
         }
         if (j.contains("availableOutgoingBitrate")) {
             j.at("availableOutgoingBitrate").get_to(st.availableOutgoingBitrate);
         }
         if (j.contains("availableIncomingBitrate")) {
             j.at("availableIncomingBitrate").get_to(st.availableIncomingBitrate);
         }
         if (j.contains("maxIncomingBitrate")) {
             j.at("maxIncomingBitrate").get_to(st.maxIncomingBitrate);
         }
     }

}
