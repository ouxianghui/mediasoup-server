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
#include "message_builder.h"

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
        }
        else if ("pli" == eventType) {
            return FBS::Consumer::TraceEventType::PLI;
        }
        else if("rtp"== eventType) {
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
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqOffset = FBS::Transport::CreateCloseConsumerRequestDirect(builder, _internal.consumerId.c_str());

        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.transportId, FBS::Request::Method::TRANSPORT_CLOSE_CONSUMER, FBS::Request::Body::Transport_CloseConsumerRequest, reqOffset);
        
        channel->request(reqId, reqData);
        
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
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.consumerId, FBS::Request::Method::CONSUMER_DUMP);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
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
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.consumerId, FBS::Request::Method::CONSUMER_GET_STATS);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(reqData.data());
        
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

        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.consumerId, FBS::Request::Method::CONSUMER_PAUSE);
        
        channel->request(reqId, reqData);
        
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

        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.consumerId, FBS::Request::Method::CONSUMER_RESUME);
        
        channel->request(reqId, reqData);
        
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
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto preferredLayersOffset = FBS::Consumer::CreateConsumerLayers(builder, layers.spatialLayer, layers.temporalLayer);
        
        auto bodyOffset = FBS::Consumer::CreateSetPreferredLayersRequest(builder, preferredLayersOffset);
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.consumerId, FBS::Request::Method::CONSUMER_SET_PREFERRED_LAYERS, FBS::Request::Body::Consumer_SetPreferredLayersRequest, bodyOffset);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
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
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqOffset = FBS::Consumer::CreateSetPriorityRequest(builder, priority);
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.consumerId, FBS::Request::Method::CONSUMER_SET_PRIORITY, FBS::Request::Body::Consumer_SetPriorityRequest, reqOffset);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto setPriorityResponse = response->body_as_Consumer_SetPriorityResponse();
        
        if (setPriorityResponse) {
            _priority = setPriorityResponse->priority();
        }
        else {
            _priority = 1;
        }
    }

    void ConsumerController::unsetPriority()
    {
        SRV_LOGD("unsetPriority()");
        
        setPriority(1);
    }

    void ConsumerController::requestKeyFrame()
    {
        SRV_LOGD("requestKeyFrame()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.consumerId, FBS::Request::Method::CONSUMER_REQUEST_KEY_FRAME);
        
        channel->request(reqId, reqData);
    }

    // types = 'rtp' | 'keyframe' | 'nack' | 'pli' | 'fir';
    void ConsumerController::enableTraceEvent(const std::vector<std::string>& types)
    {
        SRV_LOGD("enableTraceEvent()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        std::vector<FBS::Consumer::TraceEventType> events;
        for (const auto& type : types) {
            events.emplace_back(consumerTraceEventTypeToFbs(type));
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqOffset = FBS::Consumer::CreateEnableTraceEventRequestDirect(builder, &events);
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.consumerId, FBS::Request::Method::CONSUMER_ENABLE_TRACE_EVENT, FBS::Request::Body::Consumer_EnableTraceEventRequest, reqOffset);
        
        channel->request(reqId, reqData);
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
        
        if (event == FBS::Notification::Event::CONSUMER_PRODUCER_CLOSE) {
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
        else if (event == FBS::Notification::Event::CONSUMER_PRODUCER_PAUSE) {
            if (_producerPaused) {
                return;
            }
            
            _producerPaused = true;
            
            this->producerPauseSignal();
            
            if (!_paused) {
                this->pauseSignal();
            }
        }
        else if (event == FBS::Notification::Event::CONSUMER_PRODUCER_RESUME) {
            if (!_producerPaused) {
                return;
            }
            
            _producerPaused = false;
            
            this->producerResumeSignal();
            
            if (!_paused) {
                this->resumeSignal();
            }
        }
        else if (event == FBS::Notification::Event::CONSUMER_SCORE) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_Consumer_ScoreNotification()) {
                ConsumerScore score;
                score.score = nf->score()->score();
                score.producerScore = nf->score()->producerScore();
                for (const auto& s : *nf->score()->producerScores()) {
                    score.producerScores.emplace_back(s);
                }
                this->scoreSignal(score);
            }
        }
        else if (event == FBS::Notification::Event::CONSUMER_LAYERS_CHANGE) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_Consumer_LayersChangeNotification()) {
                if (const auto& l = nf->layers())  {
                    ConsumerLayers layers;
                    layers.spatialLayer =  l->spatialLayer();
                    layers.temporalLayer = l->temporalLayer().value_or(0);
                    _currentLayers = layers;
                    this->layersChangeSignal(layers);
                }
            }
        }
        else if (event == FBS::Notification::Event::CONSUMER_TRACE) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_Consumer_TraceNotification()) {
                ConsumerTraceEventData eventData = *parseTraceEventData(nf);
                this->traceSignal(eventData);
            }
        }
        else if (event == FBS::Notification::Event::CONSUMER_RTP) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_Consumer_RtpNotification()) {
                auto rtpData = nf->data();
                std::vector<uint8_t> vec(rtpData->begin(), rtpData->end());
                this->rtpSignal(vec);
            }
        }
        else {
            SRV_LOGD("ignoring unknown event %u", (uint8_t)event);
        }
    }
}

namespace srv {

    std::shared_ptr<ConsumerDump> parseConsumerDumpResponse(const FBS::Consumer::DumpResponse* response)
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

    std::shared_ptr<BaseConsumerDump> parseBaseConsumerDump(const FBS::Consumer::BaseConsumerDump* baseConsumerDump)
    {
        auto dump = std::make_shared<BaseConsumerDump>();
        dump->id = baseConsumerDump->id()->str();
        dump->producerId = baseConsumerDump->producerId()->str();
        dump->kind = baseConsumerDump->kind() == FBS::RtpParameters::MediaKind::VIDEO ? "video" : "audio";
        
        // RtpParameters rtpParameters;
        {
            dump->rtpParameters = *parseRtpParameters(baseConsumerDump->rtpParameters());
        }

        // std::vector<RtpEncodingParameters> consumableRtpEncodings;
        {
            for (const auto& encoding : *baseConsumerDump->consumableRtpEncodings()) {
                RtpEncodingParameters parameters = *parseRtpEncodingParameters(encoding);
                dump->consumableRtpEncodings.emplace_back(parameters);
            }
        }
        
        // std::vector<uint8_t> supportedCodecPayloadTypes;
        {
            for (const auto& type : *baseConsumerDump->supportedCodecPayloadTypes()) {
                dump->supportedCodecPayloadTypes.emplace_back(type);
            }
        }
        
        // std::vector<std::string> traceEventTypes;
        {
            for (const auto& type : *baseConsumerDump->traceEventTypes()) {
                auto eventType = consumerTraceEventTypeFromFbs(type);
                dump->traceEventTypes.emplace_back(eventType);
            }
        }
        
        // bool paused;
        dump->paused = baseConsumerDump->paused();
        dump->producerPaused = baseConsumerDump->producerPaused();
        dump->priority = baseConsumerDump->priority();
        
        return dump;
    }

    std::shared_ptr<SimpleConsumerDump> parseSimpleConsumerDump(const FBS::Consumer::ConsumerDump* consumerDump)
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
        dump->rtpStream = *parseRtpStream(*consumerDump->rtpStreams()->begin());
        
        return dump;
    }

    std::shared_ptr<SimulcastConsumerDump> parseSimulcastConsumerDump(const FBS::Consumer::ConsumerDump* consumerDump)
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
        dump->rtpStream = *parseRtpStream(*consumerDump->rtpStreams()->begin());
        dump->preferredSpatialLayer = consumerDump->preferredSpatialLayer().value_or(0);
        dump->targetSpatialLayer = consumerDump->targetSpatialLayer().value_or(0);
        dump->currentSpatialLayer = consumerDump->currentSpatialLayer().value_or(0);
        dump->preferredTemporalLayer = consumerDump->preferredTemporalLayer().value_or(0);
        dump->targetTemporalLayer = consumerDump->targetTemporalLayer().value_or(0);
        dump->currentTemporalLayer = consumerDump->currentTemporalLayer().value_or(0);
        
        return dump;
    }

    std::shared_ptr<SvcConsumerDump> parseSvcConsumerDump(const FBS::Consumer::ConsumerDump* consumerDump)
    {
        auto dump = parseSimulcastConsumerDump(consumerDump);
        
        dump->type = "svc";
        
        return dump;
    }

    std::shared_ptr<PipeConsumerDump> parsePipeConsumerDump(const FBS::Consumer::ConsumerDump* consumerDump)
    {
        auto base = parseBaseConsumerDump(consumerDump->base());
        
        auto dump = std::make_shared<PipeConsumerDump>();
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
        
        for (const auto& stream : *consumerDump->rtpStreams()) {
            auto s = parseRtpStream(stream);
            dump->rtpStreams.emplace_back(*s);
        }
        
        return dump;
    }

    std::shared_ptr<ConsumerTraceEventData> parseTraceEventData(const FBS::Consumer::TraceNotification* trace)
    {
        auto eventData = std::make_shared<ConsumerTraceEventData>();
        eventData->type = consumerTraceEventTypeFromFbs(trace->type());
        eventData->direction = trace->direction() == FBS::Common::TraceDirection::DIRECTION_IN ? "in" : "out";
        eventData->timestamp = trace->timestamp();

        if (trace->info_type() == FBS::Consumer::TraceInfo::KeyFrameTraceInfo) {
            auto traceInfo = std::make_shared<KeyFrameTraceInfo>();
            auto traceInfoFbs = trace->info_as_KeyFrameTraceInfo();
            traceInfo->isRtx = traceInfoFbs->isRtx();
            traceInfo->rtpPacket = traceInfoFbs->rtpPacket();
            eventData->info = traceInfo;
        }
        else if (trace->info_type() == FBS::Consumer::TraceInfo::FirTraceInfo) {
            auto traceInfo = std::make_shared<FirTraceInfo>();
            auto traceInfoFbs = trace->info_as_FirTraceInfo();
            traceInfo->ssrc = traceInfoFbs->ssrc();
            eventData->info = traceInfo;
        }
        else if (trace->info_type() == FBS::Consumer::TraceInfo::PliTraceInfo) {
            auto traceInfo = std::make_shared<PliTraceInfo>();
            auto traceInfoFbs = trace->info_as_PliTraceInfo();
            traceInfo->ssrc = traceInfoFbs->ssrc();
            eventData->info = traceInfo;
        }
        else if (trace->info_type() == FBS::Consumer::TraceInfo::RtpTraceInfo) {
            auto traceInfo = std::make_shared<KeyFrameTraceInfo>();
            auto traceInfoFbs = trace->info_as_KeyFrameTraceInfo();
            traceInfo->isRtx = traceInfoFbs->isRtx();
            traceInfo->rtpPacket = traceInfoFbs->rtpPacket();
            eventData->info = traceInfo;
        }
        
        return eventData;
    }

    std::shared_ptr<ConsumerLayers> parseConsumerLayers(const FBS::Consumer::ConsumerLayers* data)
    {
        auto layers = std::make_shared<ConsumerLayers>();
        
        layers->spatialLayer = data->spatialLayer();
        layers->temporalLayer = data->temporalLayer().value_or(0);
        
        return layers;
    }

    std::vector<std::shared_ptr<ConsumerStat>> parseConsumerStats(const FBS::Consumer::GetStatsResponse* binary)
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
            consumerStat->bitrate = sendStats->bitrate();
            
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
            consumerStat->roundTripTime = baseStats->roundTripTime();
            consumerStat->rtxPacketsDiscarded = baseStats->rtxPacketsDiscarded();
            
            result.emplace_back(consumerStat);
        }
        
        return result;
    }

    void to_json(nlohmann::json& j, const ConsumerScore& st)
    {
        j["score"] = st.score;
        j["producerScore"] = st.producerScore;
        j["producerScores"] = st.producerScores;
    }

    void from_json(const nlohmann::json& j, ConsumerScore& st)
    {
        if (j.contains("score")) {
            j.at("score").get_to(st.score);
        }
        if (j.contains("producerScore")) {
            j.at("producerScore").get_to(st.producerScore);
        }
        if (j.contains("producerScores")) {
            j.at("producerScores").get_to(st.producerScores);
        }
    }

    void to_json(nlohmann::json& j, const ConsumerTraceEventData& st)
    {
        j["type"] = st.type;
        j["timestamp"] = st.timestamp;
        j["direction"] = st.direction;
        //j["info"] = st.info;
    }

    void from_json(const nlohmann::json& j, ConsumerTraceEventData& st)
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

    void to_json(nlohmann::json& j, const ConsumerStat& st)
        {
            j["type"] = st.type;
            j["timestamp"] = st.timestamp;
            j["ssrc"] = st.ssrc;
            j["rtxSsrc"] = st.rtxSsrc;
            j["kind"] = st.kind;

            j["mimeType"] = st.mimeType;
            j["packetsLost"] = st.packetsLost;
            j["fractionLost"] = st.fractionLost;
            j["packetsDiscarded"] = st.packetsDiscarded;
            j["packetsRetransmitted"] = st.packetsRetransmitted;

            j["packetsRepaired"] = st.packetsRepaired;
            j["nackCount"] = st.nackCount;
            j["nackPacketCount"] = st.nackPacketCount;
            j["pliCount"] = st.pliCount;
            j["firCount"] = st.firCount;

            j["score"] = st.score;
            j["packetCount"] = st.packetCount;
            j["byteCount"] = st.byteCount;
            j["bitrate"] = st.bitrate;
            j["roundTripTime"] = st.roundTripTime;
        }

        void from_json(const nlohmann::json& j, ConsumerStat& st)
        {
            if (j.contains("type")) {
                j.at("type").get_to(st.type);
            }
            if (j.contains("timestamp")) {
                j.at("timestamp").get_to(st.timestamp);
            }
            if (j.contains("ssrc")) {
                j.at("ssrc").get_to(st.ssrc);
            }
            if (j.contains("rtxSsrc")) {
                j.at("rtxSsrc").get_to(st.rtxSsrc);
            }
            if (j.contains("kind")) {
                j.at("kind").get_to(st.kind);
            }
            
            if (j.contains("mimeType")) {
                j.at("mimeType").get_to(st.mimeType);
            }
            if (j.contains("packetsLost")) {
                j.at("packetsLost").get_to(st.packetsLost);
            }
            if (j.contains("fractionLost")) {
                j.at("fractionLost").get_to(st.fractionLost);
            }
            if (j.contains("packetsDiscarded")) {
                j.at("packetsDiscarded").get_to(st.packetsDiscarded);
            }
            if (j.contains("packetsRetransmitted")) {
                j.at("packetsRetransmitted").get_to(st.packetsRetransmitted);
            }
            
            if (j.contains("packetsRepaired")) {
                j.at("packetsRepaired").get_to(st.packetsRepaired);
            }
            if (j.contains("nackCount")) {
                j.at("nackCount").get_to(st.nackCount);
            }
            if (j.contains("nackPacketCount")) {
                j.at("nackPacketCount").get_to(st.nackPacketCount);
            }
            if (j.contains("pliCount")) {
                j.at("pliCount").get_to(st.pliCount);
            }
            if (j.contains("firCount")) {
                j.at("firCount").get_to(st.firCount);
            }
            
            if (j.contains("score")) {
                j.at("score").get_to(st.score);
            }
            if (j.contains("packetCount")) {
                j.at("packetCount").get_to(st.packetCount);
            }
            if (j.contains("byteCount")) {
                j.at("byteCount").get_to(st.byteCount);
            }
            if (j.contains("bitrate")) {
                j.at("bitrate").get_to(st.bitrate);
            }
            if (j.contains("roundTripTime")) {
                j.at("roundTripTime").get_to(st.roundTripTime);
            }
        }

}
