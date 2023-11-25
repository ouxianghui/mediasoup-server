/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "consumer_controller.h"
#include "srv_logger.h"
#include "channel.h"

#include "FBS/common.h"
#include "FBS/request.h"
#include "FBS/transport.h"
#include "FBS/rtpStream.h"
#include "FBS/rtxStream.h"
#include "FBS/rtpParameters.h"

namespace {
    FBS::Consumer::TraceEventType consumerTraceEventTypeToFbs(const std::string& eventType)
    {
        if ("keyframe" == eventType) {
            return FBS::Consumer::TraceEventType::KEYFRAME;
        }
        else if ("fir" == eventType) {
            return FBS::Consumer::TraceEventType::FIR;
        }
        else if ("nack" == eventType) {
            return FBS::Consumer::TraceEventType::NACK;
        } else if ("pli" == eventType) {
            return FBS::Consumer::TraceEventType::PLI;
        } else if("rtp"== eventType) {
            return FBS::Consumer::TraceEventType::RTP;
        }
        else {
            assert(0);
            return FBS::Consumer::TraceEventType::MAX;
        }
    }

    std::string consumerTraceEventTypeFromFbs(FBS::Consumer::TraceEventType traceType)
    {
        switch (traceType) {
            case FBS::Consumer::TraceEventType::KEYFRAME:
                return "keyframe";
            case FBS::Consumer::TraceEventType::FIR:
                return "fir";
            case FBS::Consumer::TraceEventType::NACK:
                return "nack";
            case FBS::Consumer::TraceEventType::PLI:
                return "pli";
            case FBS::Consumer::TraceEventType::RTP:
                return "rtp";
            default:
                assert(0);
                return "";
        }
    }
}

namespace srv {

ConsumerController::ConsumerController(const ConsumerInternal& internal,
                                       const ConsumerData& data,
                                       const std::shared_ptr<Channel>& channel,
                                       const nlohmann::json& appData,
                                       bool paused,
                                       bool producerPaused,
                                       const ConsumerScore& score,
                                       const ConsumerLayers& preferredLayers)
    : _internal(internal)
    , _data(data)
    , _channel(channel)
    , _appData(appData)
    , _paused(paused)
    , _producerPaused(producerPaused)
    , _score(score)
    , _preferredLayers(preferredLayers)
    {
        SRV_LOGD("ConsumerController()");
    }

    ConsumerController::~ConsumerController()
    {
        SRV_LOGD("~ConsumerController()");
    }

    void ConsumerController::init()
    {
        SRV_LOGD("init()");
        handleWorkerNotifications();
    }

    void ConsumerController::destroy()
    {
        SRV_LOGD("destroy()");
    }

    void ConsumerController::close()
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
        
        auto requestOffset = FBS::Transport::CreateCloseConsumerRequestDirect(channel->builder(), _internal.consumerId.c_str());

        channel->request(FBS::Request::Method::TRANSPORT_CLOSE_CONSUMER, FBS::Request::Body::Transport_CloseConsumerRequest, requestOffset, _internal.transportId);
        
        this->closeSignal();
    }

    void ConsumerController::onTransportClosed()
    {
        if (_closed) {
            return;
        }
        
        SRV_LOGD("onTransportClosed()");
        
        _closed = true;

        // Remove notification subscriptions.
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        channel->notificationSignal.disconnect(shared_from_this());
        
        this->transportCloseSignal();
        
        this->closeSignal();
    }

    std::shared_ptr<ConsumerDump> ConsumerController::dump()
    {
        SRV_LOGD("dump()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nullptr;
        }
        
        flatbuffers::Offset<void> bodyOffset;
        auto data = channel->request(FBS::Request::Method::CONSUMER_DUMP, FBS::Request::Body::NONE, bodyOffset, _internal.consumerId);
        
        auto message = FBS::Message::GetMessage(data.data());
        
        auto response = message->data_as_Response();
        
        auto dumpResponse = response->body_as_Consumer_DumpResponse();
        
        return parseConsumerDumpResponse(dumpResponse);
    }

    std::vector<std::shared_ptr<ConsumerStat>> ConsumerController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return {};
        }
        
        //auto data = channel->request("consumer.getStats", _internal.consumerId, "{}");
        flatbuffers::Offset<void> bodyOffset;
        auto data = channel->request(FBS::Request::Method::CONSUMER_GET_STATS, FBS::Request::Body::NONE, bodyOffset, _internal.consumerId);
        
        auto message = FBS::Message::GetMessage(data.data());
        
        auto response = message->data_as_Response();
        
        auto dumpResponse = response->body_as_Consumer_GetStatsResponse();
        
        return parseConsumerStats(dumpResponse);
    }

    void ConsumerController::pause()
    {
        SRV_LOGD("pause()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }

        flatbuffers::Offset<void> bodyOffset;
        channel->request(FBS::Request::Method::CONSUMER_PAUSE, FBS::Request::Body::NONE, bodyOffset, _internal.consumerId);
        
        bool wasPaused = _paused;

        _paused = true;

        // Emit observer event.
        if (!wasPaused && !_producerPaused) {
            this->pauseSignal();
        }
    }

    void ConsumerController::resume()
    {
        SRV_LOGD("resume()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }

        flatbuffers::Offset<void> bodyOffset;
        channel->request(FBS::Request::Method::CONSUMER_RESUME, FBS::Request::Body::NONE, bodyOffset, _internal.consumerId);
        
        bool wasPaused = _paused;

        _paused = false;

        // Emit observer event.
        if (wasPaused && !_producerPaused) {
            this->resumeSignal();
        }
    }

    void ConsumerController::setPreferredLayers(const ConsumerLayers& layers)
    {
        SRV_LOGD("setPreferredLayers()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        nlohmann::json reqData;
        reqData["spatialLayer"] = layers.spatialLayer;
        reqData["temporalLayer"] = layers.temporalLayer;
        
        auto preferredLayersOffset = FBS::Consumer::CreateConsumerLayers(channel->builder(), layers.spatialLayer, layers.temporalLayer);
        
        auto bodyOffset = FBS::Consumer::CreateSetPreferredLayersRequest(channel->builder(), preferredLayersOffset);
        
        auto data = channel->request(FBS::Request::Method::CONSUMER_SET_PREFERRED_LAYERS, FBS::Request::Body::NONE, bodyOffset, _internal.consumerId);
        
        auto message = FBS::Message::GetMessage(data.data());
        
        auto response = message->data_as_Response();
        
        auto setPreferredLayersResponse = response->body_as_Consumer_SetPreferredLayersResponse();
        auto preferredLayers = parseConsumerLayers(setPreferredLayersResponse->preferredLayers());
        if (preferredLayers) {
            _preferredLayers.spatialLayer = preferredLayers->spatialLayer;
            _preferredLayers.temporalLayer = preferredLayers->temporalLayer;
        }
        else {
            _preferredLayers.spatialLayer = 0;
            _preferredLayers.temporalLayer = 0;
        }
    }

    void ConsumerController::setPriority(int32_t priority)
    {
        SRV_LOGD("setPriority()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        nlohmann::json reqData;
        reqData["priority"] = priority;
        
        nlohmann::json data = channel->request("consumer.setPriority", _internal.consumerId, reqData.dump());
        
        if (data.is_object() && data.find("priority") != data.end()) {
            _priority = data["priority"];
        }
        else {
            _priority = 1;
        }
    }

    void ConsumerController::unsetPriority()
    {
        SRV_LOGD("unsetPriority()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        nlohmann::json reqData;
        reqData["priority"] = 1;
        
        nlohmann::json data = channel->request("consumer.setPriority", _internal.consumerId, reqData.dump());
        
        if (data.is_object() && data.find("priority") != data.end()) {
            _priority = data["priority"];
        }
        else {
            _priority = 1;
        }
    }

    void ConsumerController::requestKeyFrame()
    {
        SRV_LOGD("requestKeyFrame()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        channel->request("consumer.requestKeyFrame", _internal.consumerId, "{}");
    }

    // types = 'rtp' | 'keyframe' | 'nack' | 'pli' | 'fir';
    void ConsumerController::enableTraceEvent(const std::vector<std::string>& types)
    {
        SRV_LOGD("enableTraceEvent()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        nlohmann::json reqData = nlohmann::json::array();
        for (const auto& type : types) {
            reqData.emplace_back(type);
        }
        
        nlohmann::json data = channel->request("consumer.enableTraceEvent", _internal.consumerId, reqData.dump());
    }

    void ConsumerController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        channel->notificationSignal.connect(&ConsumerController::onChannel, shared_from_this());
    }

    void ConsumerController::onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data)
    {
        if (targetId != _internal.consumerId) {
            return;
        }
        
        if (event == "producerclose") {
            if (_closed) {
                return;
            }
            _closed = true;
            
            auto channel = _channel.lock();
            if (!channel) {
                return;
            }
            channel->notificationSignal.disconnect(shared_from_this());
            
            this->producerCloseSignal();
            
            this->closeSignal();
        }
        else if (event == "producerpause") {
            if (_producerPaused) {
                return;
            }
            
            _producerPaused = true;
            
            this->producerPauseSignal();
            
            if (!_paused) {
                this->pauseSignal();
            }
        }
        else if (event == "producerresume") {
            if (!_producerPaused) {
                return;
            }
            
            _producerPaused = false;
            
            this->producerResumeSignal();
            
            if (!_paused) {
                this->resumeSignal();
            }
        }
        else if (event == "score") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                ConsumerScore score = js;
                this->scoreSignal(score);
            }
        }
        else if (event == "layerschange") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                ConsumerLayers layers = js;
                _currentLayers = layers;
                this->layersChangeSignal(layers);
            }
        }
        else if (event == "trace") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                ConsumerTraceEventData eventData = js;
                this->traceSignal(eventData);
            }
        }
        else {
            SRV_LOGD("ignoring unknown event %s", event.c_str());
        }
    }

    std::shared_ptr<ConsumerDump> ConsumerController::parseConsumerDumpResponse(const FBS::Consumer::DumpResponse* response)
    {
        std::shared_ptr<BaseConsumerDump> consumerDump;
        
        auto type = response->data()->base()->type();
        switch(type) {
            case FBS::RtpParameters::Type::SIMPLE:
                consumerDump = parseSimpleConsumerDump(response->data());
                break;
            case FBS::RtpParameters::Type::SIMULCAST:
                consumerDump = parseSimulcastConsumerDump(response->data());
                break;
            case FBS::RtpParameters::Type::SVC:
                consumerDump = parseSvcConsumerDump(response->data());
                break;
            case FBS::RtpParameters::Type::PIPE:
                consumerDump = parsePipeConsumerDump(response->data());
                break;
            default:
                break;
        }
        
        return consumerDump;
    }

    std::shared_ptr<BaseConsumerDump> ConsumerController::parseBaseConsumerDump(const FBS::Consumer::BaseConsumerDump* baseConsumerDump)
    {
        auto dump = std::make_shared<BaseConsumerDump>();
        dump->id = baseConsumerDump->id()->str();
        dump->producerId = baseConsumerDump->producerId()->str();
        dump->kind = baseConsumerDump->kind() == FBS::RtpParameters::MediaKind::VIDEO ? "video" : "audio";
        
        // RtpParameters rtpParameters;
        {
            dump->rtpParameters.mid = baseConsumerDump->rtpParameters()->mid()->str();

            const auto* codecsFBS = baseConsumerDump->rtpParameters()->codecs();
            for (const auto& codec : *codecsFBS) {
                RtpCodecParameters parameters;
                parameters.mimeType = codec->mimeType()->str();

                parameters.payloadType = codec->payloadType();

                parameters.clockRate = codec->clockRate();

                parameters.channels = codec->channels();

                parameters.parameters.Set(codec->parameters());

                for (const auto& feedback : *codec->rtcpFeedback()) {
                    RtcpFeedback rtcpFeedback;
                    rtcpFeedback.type = feedback->type();
                    rtcpFeedback.parameter = feedback->parameter()->str();
                    parameters.rtcpFeedback.emplace_back(feedback);
                }
                
                dump->rtpParameters->codecs.emplace_back(parameters);
            }

            for (const auto& headExtension : *baseConsumerDump->rtpParameters()->headerExtensions()) {
                RtpHeaderExtensionParameters parameters;
                parameters.uri = headExtension->uri()->str();
                parameters.id = headExtension->id()->str();
                parameters.encrypt = headExtension->encrypt();
                parameters.parameters.Set(headExtension->parameters());
                dump->rtpParameters.headerExtensions.emplace_back(parameters);
            }
            
            for (const auto& encoding : *baseConsumerDump->rtpParameters()->encodings()) {
                RtpEncodingParameters parameters;
                parameters.ssrc = encoding->ssrc();
                parameters.rid = encoding->rid()->str();
                parameters.codecPayloadType = encoding->codecPayloadType().value_or(0);
                parameters.rtx.ssrc = encoding->rtx()->ssrc();
                parameters.dtx = encoding->dtx();
                parameters.scalabilityMode = encoding->scalabilityMode()->str();
                parameters.maxBitrate = encoding->maxBitrate().value_or(0);
                dump->rtpParameters.encodings.emplace_back(parameters);
            }

            dump->rtpParameters.rtcp.cname = baseConsumerDump->rtpParameters()->rtcp()->cname()->str();
            dump->rtpParameters.rtcp.reducedSize = baseConsumerDump->rtpParameters()->rtcp()->reducedSize();
        }

        // std::vector<RtpEncodingParameters> consumableRtpEncodings;
        {
            for (const auto& encoding : *baseConsumerDump->consumableRtpEncodings())) {
                RtpEncodingParameters parameters;
                parameters.ssrc = encoding->ssrc();
                parameters.rid = encoding->rid()->str();
                parameters.codecPayloadType = encoding->codecPayloadType().value_or(0);
                parameters.rtx.ssrc = encoding->rtx()->ssrc();
                parameters.dtx = encoding->dtx();
                parameters.scalabilityMode = encoding->scalabilityMode()->str();
                parameters.maxBitrate = encoding->maxBitrate().value_or(0);
                dump->consumableRtpEncodings.emplace_back(parameters);
            }
        }
        
        // std::vector<uint8_t> supportedCodecPayloadTypes;
        {
            for (const auto& type : *baseConsumerDump->supportedCodecPayloadTypes())) {
                dump->supportedCodecPayloadTypes.emplace_back(type);
            }
        }
        
        // std::vector<std::string> traceEventTypes;
        {
            for (const auto& type : *baseConsumerDump->traceEventTypes())) {
                auto eventType = consumerTraceEventTypeFromFbs(*type);
                dump->traceEventTypes.emplace_back(eventType);
            }
        }
        
        // bool paused;
        dump->paused = baseConsumerDump->paused();
        dump->producerPaused = baseConsumerDump->producerPaused();
        dump->priority = baseConsumerDump->priority();
    }

    std::shared_ptr<SimpleConsumerDump> ConsumerController::parseSimpleConsumerDump(const FBS::Consumer::ConsumerDump* consumerDump)
    {
        auto base = parseBaseConsumerDump(consumerDump->base());
        
        auto dump = std::make_shared<SimpleConsumerDump>();
        dump->type = "simple";
    
        dump->id = base->id;
        dump->producerId = base->producerId;
        dump->kind = base->kind;
        dump->rtpParameters = base->rtpParameters;
        dump->consumableRtpEncodings = base->consumableRtpEncodings;
        dump->supportedCodecPayloadTypes = base->supportedCodecPayloadTypes;
        dump->traceEventTypes = base->traceEventTypes;
        dump->paused = base->paused;
        dump->producerPaused = base->producerPaused;
        dump->priority = base->priority;
        
        dump->rtpStream = parseRtpStream(consumerDump->rtpStreams());
        
        return dump;
    }

    std::shared_ptr<SimulcastConsumerDump> ConsumerController::parseSimulcastConsumerDump(const FBS::Consumer::ConsumerDump* consumerDump)
    {
        auto base = parseBaseConsumerDump(consumerDump->base());
        
        auto dump = std::make_shared<SimulcastConsumerDump>();
        dump->type = "simulcast";
        dump->id = base->id;
        dump->producerId = base->producerId;
        dump->kind = base->kind;
        dump->rtpParameters = base->rtpParameters;
        dump->consumableRtpEncodings = base->consumableRtpEncodings;
        dump->supportedCodecPayloadTypes = base->supportedCodecPayloadTypes;
        dump->traceEventTypes = base->traceEventTypes;
        dump->paused = base->paused;
        dump->producerPaused = base->producerPaused;
        dump->priority = base->priority;
        
        dump->rtpStream = parseRtpStream(consumerDump->rtpStreams());
        
        dump->preferredSpatialLayer = consumerDump->preferredSpatialLayer().value_or(0);
        dump->targetSpatialLayer = consumerDump->targetSpatialLayer().value_or(0);
        dump->currentSpatialLayer = consumerDump->currentSpatialLayer().value_or(0);
        dump->preferredTemporalLayer = consumerDump->preferredTemporalLayer().value_or(0);
        dump->targetTemporalLayer = consumerDump->targetTemporalLayer().value_or(0);
        dump->currentTemporalLayer = consumerDump->currentTemporalLayer().value_or(0);
        
        return dump;
    }

    std::shared_ptr<SvcConsumerDump> ConsumerController::parseSvcConsumerDump(const FBS::Consumer::ConsumerDump* consumerDump)
    {
        auto dump = parseSvcConsumerDump(consumerDump);
        
        dump->type = "svc";
        
        return dump;
    }

    std::shared_ptr<PipeConsumerDump> ConsumerController::parsePipeConsumerDump(const FBS::Consumer::ConsumerDump* consumerDump)
    {
        auto base = parseBaseConsumerDump(consumerDump->base());
        
        auto dump = std::make_shared<SimpleConsumerDump>();
        dump->type = "pipe";
    
        dump->id = base->id;
        dump->producerId = base->producerId;
        dump->kind = base->kind;
        dump->rtpParameters = base->rtpParameters;
        dump->consumableRtpEncodings = base->consumableRtpEncodings;
        dump->supportedCodecPayloadTypes = base->supportedCodecPayloadTypes;
        dump->traceEventTypes = base->traceEventTypes;
        dump->paused = base->paused;
        dump->producerPaused = base->producerPaused;
        dump->priority = base->priority;
        
        dump->rtpStream = parseRtpStream(consumerDump->rtpStreams());
        
        return dump;
    }

    std::shared_ptr<ConsumerTraceEventData> ConsumerController::parseTraceEventData(const FBS::Consumer::TraceNotification* trace)
    {
        auto eventData = std::make_shared<ConsumerTraceEventData>();
        eventData->type = consumerTraceEventTypeFromFbs(trace->type());
        eventData->direction = trace->direction() == FBS::Common::TraceDirection::DIRECTION_IN ? "in" : "out";
        eventData->timestamp = trace->timestamp();

        if (trace->info_type() == FBS::Consumer::TraceInfo::KeyFrameTraceInfo) {
            auto traceInfo = std::make_shared<KeyFrameTraceInfo>();
            auto traceInfoFBS = trace->info_as_KeyFrameTraceInfo();
            traceInfo->isRtx = traceInfoFBS->isRtx();
            traceInfo->rtpPacket = traceInfoFBS->rtpPacket();
            eventData->info = traceInfo;
        }
        else if (trace->info_type() == FBS::Consumer::TraceInfo::FirTraceInfo) {
            auto traceInfo = std::make_shared<FirTraceInfo>();
            auto traceInfoFBS = trace->info_as_FirTraceInfo();
            traceInfo->ssrc = traceInfoFBS->ssrc();
            eventData->info = traceInfo;
        }
        else if (trace->info_type() == FBS::Consumer::TraceInfo::PliTraceInfo) {
            auto traceInfo = std::make_shared<PliTraceInfo>();
            auto traceInfoFBS = trace->info_as_PliTraceInfo();
            traceInfo->ssrc = traceInfoFBS->ssrc();
            eventData->info = traceInfo;
        }
        else if ()trace->info_type() == FBS::Consumer::TraceInfo::RtpTraceInfo {
            auto traceInfo = std::make_shared<KeyFrameTraceInfo>();
            auto traceInfoFBS = trace->info_as_KeyFrameTraceInfo();
            traceInfo->isRtx = traceInfoFBS->isRtx();
            traceInfo->rtpPacket = traceInfoFBS->rtpPacket();
            eventData->info = traceInfo;
        }
        
        return eventData;
    }

    std::shared_ptr<ConsumerLayers> ConsumerController::parseConsumerLayers(const FBS::Consumer::ConsumerLayers* data)
    {
        auto layers = std::make_shared<ConsumerLayers>();
        
        layers->spatialLayer = data->spatialLayer();
        layers->temporalLayer = data->temporalLayer();
        
        return layers;
    }

    std::shared_ptr<RtpStreamParameters> ConsumerController::parseRtpStreamParameters(const FBS::RtpStream::Params* data)
    {
        auto parameters = std::make_shared<RtpStreamParameters>();
        
        parameters->encodingIdx = data->encodingIdx();
        parameters->ssrc = data->ssrc();
        parameters->payloadType = data->payloadType();
        parameters->mimeType = data->mimeType()->str();
        parameters->clockRate = data->clockRate();
        parameters->rid = data->rid()->str();
        parameters->cname = data->cname()->str();
        parameters->rtxSsrc = data->rtxSsrc();
        parameters->rtxPayloadType = data->rtxPayloadType();
        parameters->useNack = data->useNack();
        parameters->usePli = data->usePli();
        parameters->useFir = data->useFir();
        parameters->useInBandFec = data->useInBandFec();
        parameters->useDtx = data->useDtx();
        parameters->spatialLayers = data->spatialLayers();
        parameters->temporalLayers = data->temporalLayers();
        
        return parameters;
    }

    std::shared_ptr<RtxStreamParameters> ConsumerController::parseRtxStreamParameters(const FBS::RtxStream::Params* data)
    {
        auto parameters = std::make_shared<RtxStreamParameters>();
        
        parameters->ssrc = data->ssrc();
        parameters->payloadType = data->payloadType();
        parameters->mimeType = data->mimeType()->str();
        parameters->clockRate = data->clockRate();
        parameters->rrid = data->rrid()->str();
        parameters->cname = data->cname()->str();
        
        return parameters;
    }

    std::shared_ptr<RtxStreamDump> ConsumerController::parseRtxStream(const FBS::RtxStream::RtxDump* data)
    {
        auto dump = std::make_shared<RtxStreamDump>();
        
        dump->params = parseRtxStreamParameters(data->params());
                                                
        return dump;
    }

    std::shared_ptr<RtpStreamDump> ConsumerController::parseRtpStream(const FBS::RtpStream::Dump* data)
    {
        auto dump = std::make_shared<RtpStreamDump>();
            
        dump->params = parseRtxStreamParameters(data->params());
        
        std::shared_ptr<RtxStreamDump> rtxStream;
        if (data->rtxStream()) {
            rtxStream = parseRtxStream(data->rtxStream());
        }
        
        dump->rtxStream = rtxStream;
        
        dump->score = data->score();
        
        return dump;
    }

    std::vector<std::shared_ptr<ConsumerStat>> ConsumerController::parseConsumerStats(const FBS::Consumer::GetStatsResponse* binary)
    {
        std::vector<std::shared_ptr<ConsumerStat>> result;
        
        auto stats = binary->stats();
        
        for (const auto& st : *stats) {
            auto consumerStat = std::make_shared<ConsumerStat>();
            
            auto sendStats = st->data_as_SendStats();
            // TODO:
            consumerStat->type = "";
            consumerStat->packetCount = sendStats->packetCount();
            consumerStat->byteCount = sendStats->byteCount();
            consumerStat->bitrate = = sendStats->bitrate();
            
            auto baseStats = st->data_as_BaseStats();
            // base
            consumerStat->timestamp = baseStats->timestamp();;
            consumerStat->ssrc = baseStats->ssrc();
            consumerStat->rtxSsrc = baseStats->rtxSsrc().value_or(0);
            consumerStat->rid = baseStats->rid()->str();
            consumerStat->kind = baseStats->kind() == FBS::RtpParameters::MediaKind::AUDIO ? "audio" : "video";
            consumerStat->mimeType = baseStats->mimeType()->str();
            consumerStat->packetsLost = baseStats->packetsLost();
            consumerStat->fractionLost = baseStats->fractionLost();
            consumerStat->packetsDiscarded = baseStats->packetsDiscarded();
            consumerStat->packetsRetransmitted = baseStats->packetsRetransmitted();
            consumerStat->packetsRepaired = baseStats->packetsRepaired();
            consumerStat->nackCount = baseStats->nackCount();
            consumerStat->nackPacketCount = baseStats->nackPacketCount();
            consumerStat->pliCount = baseStats->pliCount();
            consumerStat->firCount = baseStats->firCount();
            consumerStat->score = baseStats->score();
            consumerStat->roundTripTime =baseStats->roundTripTime();
            consumerStat->rtxPacketsDiscarded = baseStats->rtxPacketsDiscarded();
            
            result->emplace_back(consumerStat);
        }
    }

}
