/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "direct_transport_controller.h"
#include "srv_logger.h"
#include "types.h"
#include "uuid.h"
#include "ortc.h"
#include "channel.h"
#include "payload_channel.h"

namespace srv {

    DirectTransportController::DirectTransportController(const std::shared_ptr<DirectTransportConstructorOptions>& options)
    : TransportController(options)
    {
        SRV_LOGD("DirectTransportController()");
    }

    DirectTransportController::~DirectTransportController()
    {
        SRV_LOGD("~DirectTransportController()");
    }

    void DirectTransportController::init()
    {
        SRV_LOGD("init()");
        handleWorkerNotifications();
    }

    void DirectTransportController::destroy()
    {
        SRV_LOGD("destroy()");
    }

    void DirectTransportController::close()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");
        
        TransportController::close();
    }

    void DirectTransportController::onRouterClosed()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("onRouterClosed()");

        TransportController::onRouterClosed();
    }

    nlohmann::json DirectTransportController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nlohmann::json();
        }
        
        return channel->request("transport.getStats", _internal.transportId, "{}");
    }

    void DirectTransportController::connect(const nlohmann::json& data)
    {
        SRV_LOGD("connect()");
        return;
    }

    void DirectTransportController::setMaxIncomingBitrate(int32_t bitrate)
    {
        SRV_LOGE("setMaxIncomingBitrate() not implemented in DirectTransport");
        return;
    }

    void DirectTransportController::setMaxOutgoingBitrate(int32_t bitrate)
    {
        SRV_LOGE("setMaxOutgoingBitrate() not implemented in DirectTransport");
        return;
    }

    void DirectTransportController::setMinOutgoingBitrate(int32_t bitrate)
    {
        SRV_LOGE("setMinOutgoingBitrate() not implemented in DirectTransport");
        return;
    }

    void DirectTransportController::sendRtcp(const uint8_t* payload, size_t payloadLen)
    {
        SRV_LOGD("sendRtcp()");
        
        if (!payload || payloadLen <= 0) {
            SRV_LOGE("rtcpPacket must be a Buffer");
            return;
        }

        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        
        payloadChannel->notify("transport.sendRtcp", _internal.transportId, "", payload, payloadLen);
    }

    void DirectTransportController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto self = std::dynamic_pointer_cast<DirectTransportController>(TransportController::shared_from_this());
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        channel->_notificationSignal.connect(&DirectTransportController::onChannel, self);
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->_notificationSignal.connect(&DirectTransportController::onPayloadChannel, self);
    }

    void DirectTransportController::onChannel(const std::string& targetId, const std::string& event, const std::string& data)
    {
        SRV_LOGD("onChannel()");
        
        if (targetId != _internal.transportId) {
            return;
        }
        
        if (event == "trace") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                TransportTraceEventData eventData = js;
                _traceSignal(eventData);
            }
        }
        else {
            SRV_LOGD("ignoring unknown event %s", event.c_str());
        }
    }

    void DirectTransportController::onPayloadChannel(const std::string& targetId, const std::string& event, const std::string& data, const uint8_t* payload, size_t payloadLen)
    {
        SRV_LOGD("onPayloadChannel()");
        
        if (_closed) {
            return;
        }

        if (targetId != _internal.transportId) {
            return;
        }

        if (event == "rtcp") {
            _rtcpSignal(payload, payloadLen);
        }
        else {
            SRV_LOGD("ignoring unknown event %s", event.c_str());
        }
    }

}

namespace srv
{
    void to_json(nlohmann::json& j, const DirectTransportStat& st)
    {
        j["type"] = st.type;
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

    void from_json(const nlohmann::json& j, DirectTransportStat& st)
    {
        if (j.contains("type")) {
            j.at("type").get_to(st.type);
        }
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
