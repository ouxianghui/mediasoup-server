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
#include "payload_channel.h"

namespace srv {

    ProducerController::ProducerController(const ProducerInternal& internal,
                                           const ProducerData& data,
                                           const std::shared_ptr<Channel>& channel,
                                           const std::shared_ptr<PayloadChannel>& payloadChannel,
                                           const nlohmann::json& appData,
                                           bool paused)
    : _internal(internal)
    , _data(data)
    , _channel(channel)
    , _payloadChannel(payloadChannel)
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
        channel->_notificationSignal.disconnect(shared_from_this());
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->_notificationSignal.disconnect(shared_from_this());
        
        nlohmann::json reqData;
        reqData["producerId"] = _internal.producerId;
        
        channel->request("transport.closeProducer", _internal.transportId, reqData.dump());
        
        _closeSignal();
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
        channel->_notificationSignal.disconnect(shared_from_this());
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->_notificationSignal.disconnect(shared_from_this());
        
        _transportCloseSignal();
        
        _closeSignal();
    }

    nlohmann::json ProducerController::dump()
    {
        SRV_LOGD("dump()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nlohmann::json();
        }
        
        return channel->request("producer.dump", _internal.producerId, "{}");
    }

    nlohmann::json ProducerController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return std::vector<ProducerStat>();
        }
        
        return channel->request("producer.getStats", _internal.producerId, "{}");
    }

    void ProducerController::pause()
    {
        SRV_LOGD("pause()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        bool wasPaused = _paused;

        channel->request("producer.pause", _internal.producerId, "{}");

        _paused = true;

        // Emit observer event.
        if (!wasPaused) {
            _pauseSignal();
        }
    }

    void ProducerController::resume()
    {
        SRV_LOGD("resume()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        bool wasPaused = _paused;

        channel->request("producer.resume", _internal.producerId, "{}");

        _paused = false;

        // Emit observer event.
        if (wasPaused) {
            _resumeSignal();
        }
    }

    // types = 'rtp' | 'keyframe' | 'nack' | 'pli' | 'fir';
    void ProducerController::enableTraceEvent(const std::vector<std::string>& types)
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
        
        nlohmann::json data = channel->request("producer.enableTraceEvent", _internal.producerId, reqData.dump());
    }

    void ProducerController::send(const uint8_t* payload, size_t payloadLen)
    {
        if (payloadLen <= 0) {
            return;
        }

        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        
        payloadChannel->notify("roducer.send", _internal.producerId, "", payload, payloadLen);
    }

    void ProducerController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        channel->_notificationSignal.connect(&ProducerController::onChannel, shared_from_this());
    }

    void ProducerController::onChannel(const std::string& targetId, const std::string& event, const std::string& data)
    {
        if (targetId != _internal.producerId) {
            return;
        }
        
        if (event == "score") {
            auto js = nlohmann::json::parse(data);
            if (js.is_array()) {
                std::vector<ProducerScore> scores;
                for (const auto& item : js["score"]) {
                    ProducerScore score = item;
                    scores.emplace_back(score);
                }
                {
                    std::lock_guard<std::mutex> lock(_scoreMutex);
                    _score = scores;
                }
                _scoreSignal(scores);
            }
        }
        else if (event == "videoorientationchange") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                ProducerVideoOrientation orientation = js;
                _videoOrientationChangeSignal(orientation);
            }
        }
        else if (event == "trace") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                ProducerTraceEventData eventData = js;
                _traceSignal(eventData);
            }
        }
        else {
            SRV_LOGD("ignoring unknown event %s", event.c_str());
        }
    }

}

namespace srv
{
    void to_json(nlohmann::json& j, const ProducerTraceEventData& st)
    {
        j["type"] = st.type;
        j["timestamp"] = st.timestamp;
        j["direction"] = st.direction;
        j["info"] = st.info;
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
        if (j.contains("info")) {
            j.at("info").get_to(st.info);
        }
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
