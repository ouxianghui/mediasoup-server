/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "producer_controller.h"
#include "srv_logger.h"
#include "channel.h"
#include "ortc.h"
#include "message_builder.h"

namespace srv {

    ProducerController::ProducerController(const ProducerInternal& internal,
                                           const ProducerData& data,
                                           const std::shared_ptr<Channel>& channel,
                                           const nlohmann::json& appData,
                                           bool paused)
    : _internal(internal)
    , _data(data)
    , _channel(channel)
    , _appData(appData)
    , _paused(paused)
    {
        SRV_LOGD("ProducerController()");
    }

    ProducerController::~ProducerController()
    {
        SRV_LOGD("~ProducerController()");
    }

    void ProducerController::init()
    {
        SRV_LOGD("init()");
        handleWorkerNotifications();
    }

    void ProducerController::destroy()
    {
        SRV_LOGD("destroy()");
    }

    void ProducerController::close()
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
        
        auto reqOffset = FBS::Transport::CreateCloseProducerRequestDirect(builder, _internal.producerId.c_str());
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::TRANSPORT_CLOSE_PRODUCER,
                                                     FBS::Request::Body::Transport_CloseProducerRequest,
                                                     reqOffset);
        
        channel->request(reqId, reqData);
        
        this->closeSignal();
    }

    void ProducerController::onTransportClosed()
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

    std::shared_ptr<ProducerDump> ProducerController::dump()
    {
        SRV_LOGD("dump()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nullptr;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.producerId, FBS::Request::Method::PRODUCER_DUMP);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        return parseProducerDump(response->body_as_Producer_DumpResponse());
    }

    std::vector<std::shared_ptr<ProducerStat>> ProducerController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return {};
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.producerId, FBS::Request::Method::PRODUCER_GET_STATS);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        return parseProducerStats(response->body_as_Producer_GetStatsResponse());
    }

    void ProducerController::pause()
    {
        SRV_LOGD("pause()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }

        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.producerId, FBS::Request::Method::PRODUCER_PAUSE);
        
        channel->request(reqId, reqData);
        
        bool wasPaused = _paused;
        
        _paused = true;

        // Emit observer event.
        if (!wasPaused) {
            this->pauseSignal();
        }
    }

    void ProducerController::resume()
    {
        SRV_LOGD("resume()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }

        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.producerId, FBS::Request::Method::PRODUCER_RESUME);
        
        channel->request(reqId, reqData);
        
        bool wasPaused = _paused;
        
        _paused = false;

        // Emit observer event.
        if (wasPaused) {
            this->resumeSignal();
        }
    }

    // types = 'rtp' | 'keyframe' | 'nack' | 'pli' | 'fir' | 'sr';
    void ProducerController::enableTraceEvent(const std::vector<std::string>& types)
    {
        SRV_LOGD("enableTraceEvent()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }

        std::vector<FBS::Producer::TraceEventType> eventTypes;
        for (const auto& type : types) {
            eventTypes.emplace_back(producerTraceEventTypeToFbs(type));
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqOffset = FBS::Producer::CreateEnableTraceEventRequestDirect(builder, &eventTypes);
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId, 
                                                     _internal.producerId,
                                                     FBS::Request::Method::PRODUCER_ENABLE_TRACE_EVENT,
                                                     FBS::Request::Body::Producer_EnableTraceEventRequest,
                                                     reqOffset);
        
        channel->request(reqId, reqData);
    }

    void ProducerController::send(const std::vector<uint8_t>& data)
    {
        if (data.size() <= 0) {
            return;
        }

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto nfOffset = FBS::Producer::CreateSendNotificationDirect(builder, &data);
        
        auto nfData = MessageBuilder::createNotification(builder,
                                                         _internal.producerId,
                                                         FBS::Notification::Event::PRODUCER_SEND,
                                                         FBS::Notification::Body::Producer_SendNotification,
                                                         nfOffset);
        
        channel->notify(nfData);
    }

    void ProducerController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        channel->notificationSignal.connect(&ProducerController::onChannel, shared_from_this());
    }

    void ProducerController::onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data)
    {
        if (targetId != _internal.producerId) {
            return;
        }
        
        if (event == FBS::Notification::Event::PRODUCER_SCORE) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_Producer_ScoreNotification()) {
                _score.clear();
                for (const auto& item : *nf->scores()) {
                    ProducerScore producerScore;
                    producerScore.score = item->score();
                    producerScore.ssrc = item->ssrc();
                    if (auto rid = item->rid()) {
                        producerScore.rid = rid->str();
                    }
                    _score.push_back(producerScore);
                }
                this->scoreSignal(_score.value());
            }
        }
        else if (event == FBS::Notification::Event::PRODUCER_VIDEO_ORIENTATION_CHANGE) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_Producer_VideoOrientationChangeNotification()) {
                ProducerVideoOrientation orientation;
                orientation.camera = nf->camera();
                orientation.flip = nf->flip();
                orientation.rotation = nf->rotation();
                this->videoOrientationChangeSignal(orientation);
            }
        }
        else if (event == FBS::Notification::Event::PRODUCER_TRACE) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_Producer_TraceNotification()) {
                ProducerTraceEventData eventData = *parseTraceEventData(nf);
                this->traceSignal(eventData);
            }
        }
        else {
            SRV_LOGD("ignoring unknown event %u", (uint8_t)event);
        }
    }
}

namespace srv {

    std::string producerTypeFromFbs(FBS::RtpParameters::Type type)
    {
        switch (type)
        {
            case FBS::RtpParameters::Type::SIMPLE:
                return "simple";
            case FBS::RtpParameters::Type::SIMULCAST:
                return "simulcast";
            case FBS::RtpParameters::Type::SVC:
                return "svc";
            default:
                SRV_LOGE("invalid FbsRtpParameters.Type: %u", (uint8_t)type);
                return "";
        }
    }

    FBS::RtpParameters::Type producerTypeToFbs(const std::string& type)
    {
        if (type == "simple") {
            return FBS::RtpParameters::Type::SIMPLE;
        }
        else if (type == "simulcast") {
            return FBS::RtpParameters::Type::SIMULCAST;
        }
        else if (type == "svc") {
            return FBS::RtpParameters::Type::SVC;
        }
        
        return FBS::RtpParameters::Type::MIN;
    }

    FBS::Producer::TraceEventType producerTraceEventTypeToFbs(const std::string& eventType)
    {
        if (eventType == "keyframe") {
            return FBS::Producer::TraceEventType::KEYFRAME;
        }
        else if (eventType == "fir") {
            return FBS::Producer::TraceEventType::FIR;
        }
        else if (eventType == "nack") {
            return FBS::Producer::TraceEventType::NACK;
        }
        else if (eventType == "pli") {
            return FBS::Producer::TraceEventType::PLI;
        }
        else if (eventType == "rtp") {
            return FBS::Producer::TraceEventType::RTP;
        }
        else if (eventType == "sr") {
            return FBS::Producer::TraceEventType::SR;
        }
        else {
            SRV_LOGE("invalid ProducerTraceEventType: %s", eventType.c_str());
            return FBS::Producer::TraceEventType::MIN;
        }
    }

    std::string producerTraceEventTypeFromFbs(FBS::Producer::TraceEventType eventType)
    {
        switch (eventType)
        {
            case FBS::Producer::TraceEventType::KEYFRAME:
                return "keyframe";
            case FBS::Producer::TraceEventType::FIR:
                return "fir";
            case FBS::Producer::TraceEventType::NACK:
                return "nack";
            case FBS::Producer::TraceEventType::PLI:
                return "pli";
            case FBS::Producer::TraceEventType::RTP:
                return "rtp";
            case FBS::Producer::TraceEventType::SR:
                return "sr";
            default:
                SRV_LOGE("invalid FBS::Producer::TraceEventType: %u", (uint8_t)eventType);
                return "";
        }
    }

    std::shared_ptr<ProducerDump> parseProducerDump(const FBS::Producer::DumpResponse* data)
    {
        auto dump = std::make_shared<ProducerDump>();

        dump->id = data->id()->str();
        dump->kind = data->kind() == FBS::RtpParameters::MediaKind::AUDIO ? "audio" : "video";
        dump->type = producerTypeFromFbs(data->type());
        dump->rtpParameters = *parseRtpParameters(data->rtpParameters());

        for (const auto& codec : *data->rtpMapping()->codecs()) {
            dump->rtpMapping.codecs.emplace(std::pair<uint8_t, uint8_t>(codec->payloadType(), codec->mappedPayloadType()));
        }
        
        for (const auto& encoding : *data->rtpMapping()->encodings()) {
            srv::RtpEncodingMapping rtpEncodingMapping;
            rtpEncodingMapping.ssrc = encoding->ssrc().value_or(0);
            rtpEncodingMapping.rid = encoding->rid()->str();
            rtpEncodingMapping.mappedSsrc = encoding->mappedSsrc();
            dump->rtpMapping.encodings.emplace_back(rtpEncodingMapping);
        }
        
        for (const auto& rtpStream : *data->rtpStreams()) {
            dump->rtpStreams.emplace_back(*parseRtpStream(rtpStream));
        }
        
        for (const auto& eventType : *data->traceEventTypes()) {
            dump->traceEventTypes.emplace_back(producerTraceEventTypeFromFbs(eventType));
        }
        
        dump->paused = data->paused();
        
        return dump;
    }

    std::vector<std::shared_ptr<ProducerStat>> parseProducerStats(const FBS::Producer::GetStatsResponse* binary)
    {
        std::vector<std::shared_ptr<ProducerStat>> result;
        
        auto stats = binary->stats();
        
        for (const auto& st : *stats) {
            auto producerStat = std::make_shared<ProducerStat>();
            
            auto recvStats = st->data_as_RecvStats();
            // TODO:
            producerStat->type = "";
            producerStat->packetCount = recvStats->packetCount();
            producerStat->byteCount = recvStats->byteCount();
            producerStat->bitrate = recvStats->bitrate();
            producerStat->jitter = recvStats->jitter();
            producerStat->bitrateByLayer = parseBitrateByLayer(recvStats);
            
            auto baseStats = st->data_as_BaseStats();
            // base
            producerStat->timestamp = baseStats->timestamp();;
            producerStat->ssrc = baseStats->ssrc();
            producerStat->rtxSsrc = baseStats->rtxSsrc().value_or(0);
            producerStat->rid = baseStats->rid()->str();
            producerStat->kind = baseStats->kind() == FBS::RtpParameters::MediaKind::AUDIO ? "audio" : "video";
            producerStat->mimeType = baseStats->mimeType()->str();
            producerStat->packetsLost = baseStats->packetsLost();
            producerStat->fractionLost = baseStats->fractionLost();
            producerStat->packetsDiscarded = baseStats->packetsDiscarded();
            producerStat->packetsRetransmitted = baseStats->packetsRetransmitted();
            producerStat->packetsRepaired = baseStats->packetsRepaired();
            producerStat->nackCount = baseStats->nackCount();
            producerStat->nackPacketCount = baseStats->nackPacketCount();
            producerStat->pliCount = baseStats->pliCount();
            producerStat->firCount = baseStats->firCount();
            producerStat->score = baseStats->score();
            producerStat->roundTripTime = baseStats->roundTripTime();
            producerStat->rtxPacketsDiscarded = baseStats->rtxPacketsDiscarded();
            
            result.emplace_back(producerStat);
        }

        return result;
    }

    std::shared_ptr<ProducerScore> parseProducerScore(const FBS::Producer::Score* binary)
    {
        auto producerScore = std::make_shared<ProducerScore>();
        
        producerScore->ssrc = binary->ssrc();
        producerScore->rid = binary->rid()->str();
        producerScore->score = binary->ssrc();
        
        return producerScore;
    }

    std::shared_ptr<ProducerTraceEventData> parseTraceEventData(const FBS::Producer::TraceNotification* trace)
    {
        auto eventData = std::make_shared<ProducerTraceEventData>();
        eventData->type = producerTraceEventTypeFromFbs(trace->type());
        eventData->direction = trace->direction() == FBS::Common::TraceDirection::DIRECTION_IN ? "in" : "out";
        eventData->timestamp = trace->timestamp();

        if (trace->info_type() == FBS::Producer::TraceInfo::KeyFrameTraceInfo) {
            auto traceInfo = std::make_shared<KeyFrameTraceInfo>();
            auto traceInfoFbs = trace->info_as_KeyFrameTraceInfo();
            traceInfo->isRtx = traceInfoFbs->isRtx();
            traceInfo->rtpPacket = traceInfoFbs->rtpPacket();
            eventData->info = traceInfo;
        }
        else if (trace->info_type() == FBS::Producer::TraceInfo::FirTraceInfo) {
            auto traceInfo = std::make_shared<FirTraceInfo>();
            auto traceInfoFbs = trace->info_as_FirTraceInfo();
            traceInfo->ssrc = traceInfoFbs->ssrc();
            eventData->info = traceInfo;
        }
        else if (trace->info_type() == FBS::Producer::TraceInfo::PliTraceInfo) {
            auto traceInfo = std::make_shared<PliTraceInfo>();
            auto traceInfoFbs = trace->info_as_PliTraceInfo();
            traceInfo->ssrc = traceInfoFbs->ssrc();
            eventData->info = traceInfo;
        }
        else if (trace->info_type() == FBS::Producer::TraceInfo::RtpTraceInfo) {
            auto traceInfo = std::make_shared<KeyFrameTraceInfo>();
            auto traceInfoFbs = trace->info_as_KeyFrameTraceInfo();
            traceInfo->isRtx = traceInfoFbs->isRtx();
            traceInfo->rtpPacket = traceInfoFbs->rtpPacket();
            eventData->info = traceInfo;
        }
        
        return eventData;
    }

    void to_json(nlohmann::json& j, const ProducerScore& st)
    {
        j["ssrc"] = st.ssrc;
        j["rid"] = st.rid;
        j["score"] = st.score;
    }

    void from_json(const nlohmann::json& j, ProducerScore& st)
    {
        if (j.contains("ssrc")) {
            j.at("ssrc").get_to(st.ssrc);
        }
        if (j.contains("rid")) {
            j.at("rid").get_to(st.rid);
        }
        if (j.contains("score")) {
            j.at("score").get_to(st.score);
        }
    }

    void to_json(nlohmann::json& j, const ProducerVideoOrientation& st)
    {
        j["camera"] = st.camera;
        j["flip"] = st.flip;
        j["rotation"] = st.rotation;
    }

    void from_json(const nlohmann::json& j, ProducerVideoOrientation& st)
    {
        if (j.contains("camera")) {
            j.at("camera").get_to(st.camera);
        }
        if (j.contains("flip")) {
            j.at("flip").get_to(st.flip);
        }
        if (j.contains("rotation")) {
            j.at("rotation").get_to(st.rotation);
        }
    }

    void to_json(nlohmann::json& j, const ProducerTraceEventData& st)
    {
        j["type"] = st.type;
        j["timestamp"] = st.timestamp;
        j["direction"] = st.direction;
        //j["info"] = st.info;
    }

    void from_json(const nlohmann::json& j, ProducerTraceEventData& st)
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

    void to_json(nlohmann::json& j, const ProducerStat& st)
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

    void from_json(const nlohmann::json& j, ProducerStat& st)
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
