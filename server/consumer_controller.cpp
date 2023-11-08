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
#include "payload_channel.h"

namespace srv {

ConsumerController::ConsumerController(const ConsumerInternal& internal,
                                       const ConsumerData& data,
                                       const std::shared_ptr<Channel>& channel,
                                       std::shared_ptr<PayloadChannel> payloadChannel,
                                       const nlohmann::json& appData,
                                       bool paused,
                                       bool producerPaused,
                                       const ConsumerScore& score,
                                       const ConsumerLayers& preferredLayers)
    : _internal(internal)
    , _data(data)
    , _channel(channel)
    , _payloadChannel(payloadChannel)
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
        channel->_notificationSignal.disconnect(shared_from_this());
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->_notificationSignal.disconnect(shared_from_this());
        
        nlohmann::json reqData;
        reqData["consumerId"] = _internal.consumerId;
        
        channel->request("transport.closeConsumer", _internal.transportId, reqData.dump());
        
        _closeSignal();
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
        channel->_notificationSignal.disconnect(shared_from_this());
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->_notificationSignal.disconnect(shared_from_this());
        
        _transportCloseSignal();
        
        _closeSignal();
    }

    nlohmann::json ConsumerController::dump()
    {
        SRV_LOGD("dump()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nlohmann::json();
        }
        
        return channel->request("consumer.dump", _internal.consumerId, "{}");
    }

    nlohmann::json ConsumerController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return std::vector<ConsumerStat>();
        }
        
        return channel->request("consumer.getStats", _internal.consumerId, "{}");
    }

    void ConsumerController::pause()
    {
        SRV_LOGD("pause()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        auto wasPaused = _paused || _producerPaused;

        channel->request("consumer.pause", _internal.consumerId, "{}");

        _paused = true;

        // Emit observer event.
        if (!wasPaused) {
            _pauseSignal();
        }
    }

    void ConsumerController::resume()
    {
        SRV_LOGD("resume()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        auto wasPaused = _paused || _producerPaused;

        channel->request("consumer.resume", _internal.consumerId, "{}");

        _paused = false;

        // Emit observer event.
        if (wasPaused && !_producerPaused) {
            _resumeSignal();
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
        
        nlohmann::json data = channel->request("consumer.setPreferredLayers", _internal.consumerId, reqData.dump());
        
        if (data.is_object() && data.find("spatialLayer") != data.end() && data.find("temporalLayer") != data.end()) {
            _preferredLayers = data;
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
        channel->_notificationSignal.connect(&ConsumerController::onChannel, shared_from_this());
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->_notificationSignal.connect(&ConsumerController::onPayloadChannel, shared_from_this());
    }

    void ConsumerController::onChannel(const std::string& targetId, const std::string& event, const std::string& data)
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
            channel->_notificationSignal.disconnect(shared_from_this());
            
            auto payloadChannel = _payloadChannel.lock();
            if (!payloadChannel) {
                return;
            }
            payloadChannel->_notificationSignal.disconnect(shared_from_this());
            
            _producerCloseSignal();
            
            _closeSignal();
        }
        else if (event == "producerpause") {
            if (_producerPaused) {
                return;
            }
            
            auto wasPaused = _paused || _producerPaused;
            
            _producerPaused = true;
            
            _producerPauseSignal();
            
            if (!wasPaused) {
                _pauseSignal();
            }
        }
        else if (event == "producerresume") {
            if (!_producerPaused) {
                return;
            }
            
            auto wasPaused = _paused || _producerPaused;
            
            _producerPaused = false;
            
            _producerResumeSignal();
            
            if (wasPaused && !_paused) {
                _resumeSignal();
            }
        }
        else if (event == "score") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                ConsumerScore score = js;
                _scoreSignal(score);
            }
        }
        else if (event == "layerschange") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                ConsumerLayers layers = js;
                _currentLayers = layers;
                _layersChangeSignal(layers);
            }
        }
        else if (event == "trace") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                ConsumerTraceEventData eventData = js;
                _traceSignal(eventData);
            }
        }
        else {
            SRV_LOGD("ignoring unknown event %s", event.c_str());
        }
    }

    void ConsumerController::onPayloadChannel(const std::string& targetId, const std::string& event, const std::string& data, const uint8_t* payload, size_t payloadLen)
    {
        if (_closed) {
            return;
        }
        
        if (targetId != _internal.consumerId) {
            return;
        }
        
        if (event == "rtp") {
            _rtpSignal(payload, payloadLen);
        }
        else {
            SRV_LOGD("ignoring unknown event %s", event.c_str());
        }
    }
}


namespace srv
{
    void to_json(nlohmann::json& j, const ConsumerLayers& st)
    {
        j["spatialLayer"] = st.spatialLayer;
        j["temporalLayer"] = st.temporalLayer;
    }

    void from_json(const nlohmann::json& j, ConsumerLayers& st)
    {
        if (j.contains("spatialLayer")) {
            j.at("spatialLayer").get_to(st.spatialLayer);
        }
        if (j.contains("temporalLayer")) {
            j.at("temporalLayer").get_to(st.temporalLayer);
        }
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
        j["info"] = st.info;
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
        if (j.contains("info")) {
            j.at("info").get_to(st.info);
        }
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
