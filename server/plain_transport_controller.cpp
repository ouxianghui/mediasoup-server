/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "plain_transport_controller.h"
#include "srv_logger.h"
#include "types.h"
#include "uuid.h"
#include "channel.h"
#include "payload_channel.h"

namespace srv {

    PlainTransportController::PlainTransportController(const std::shared_ptr<PlainTransportConstructorOptions>& options)
    : TransportController(options)
    {
        SRV_LOGD("PlainTransportController()");
        
        const auto& data = options->data;
        
        _data["rtcpMux"] = data["rtcpMux"];
        _data["comedia"] = data["comedia"];
        _data["tuple"] = data["tuple"];
        _data["rtcpTuple"] = data["rtcpTuple"];
        _data["sctpParameters"] = data["sctpParameters"];
        _data["sctpState"] = data["sctpState"];
        _data["srtpParameters"] = data["srtpParameters"];
    }

    PlainTransportController::~PlainTransportController()
    {
        SRV_LOGD("~PlainTransportController()");
    }

    void PlainTransportController::init()
    {
        SRV_LOGD("init()");
        handleWorkerNotifications();
    }

    void PlainTransportController::destroy()
    {
        SRV_LOGD("destroy()");
    }

    void PlainTransportController::close()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");
        
        this->_data["sctpState"] = "closed";

        TransportController::close();
    }

    void PlainTransportController::onRouterClosed()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");
        
        this->_data["sctpState"] = "closed";

        TransportController::onRouterClosed();
    }

    nlohmann::json PlainTransportController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nlohmann::json();
        }
        
        return channel->request("transport.getStats", _internal.transportId, "{}");
    }

    void PlainTransportController::connect(const nlohmann::json& reqData)
    {
        SRV_LOGD("connect()");
        
        //const reqData = { ip, port, rtcpPort, srtpParameters };

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        auto data = channel->request("transport.connect", _internal.transportId, reqData.dump());

        // Update data.
        if (data.contains("tuple")) {
            this->_data["tuple"] = data["tuple"];
        }
 
        if (data.contains("rtcpTuple")) {
            this->_data["rtcpTuple"] = data["rtcpTuple"];
        }
        
        this->_data["srtpParameters"] = data["srtpParameters"];
    }

    void PlainTransportController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        auto self = std::dynamic_pointer_cast<PlainTransportController>(TransportController::shared_from_this());
        channel->_notificationSignal.connect(&PlainTransportController::onChannel, self);
    }

    void PlainTransportController::onChannel(const std::string& targetId, const std::string& event, const std::string& data)
    {
        SRV_LOGD("onChannel()");
        
        if (targetId != _internal.transportId) {
            return;
        }
        
        if (event == "tuple") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                auto tuple = js["tuple"];
                this->_data["tuple"] = tuple;
                _tupleSignal(tuple);
            }
        }
        else if (event == "rtcptuple") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                auto rtcpTuple = js["rtcpTuple"];
                this->_data["rtcpTuple"] = rtcpTuple;
                _rtcpTupleSignal(rtcpTuple);
            }
        }
        else if (event == "sctpstatechange") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                auto sctpState = js["sctpState"];
                this->_data["sctpState"] = sctpState;
                _sctpStateChangeSignal(sctpState);
            }
        }
        else if (event == "trace") {
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

}

namespace srv
{
    void to_json(nlohmann::json& j, const PlainTransportStat& st)
    {
        j["type"] = st.type;
        j["transportId"] = st.transportId;
        j["timestamp"] = st.timestamp;
        j["sctpState"] = st.sctpState;

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

        j["rtcpMux"] = st.rtcpMux;
        j["comedia"] = st.comedia;
        j["tuple"] = st.tuple;
        j["rtcpTuple"] = st.rtcpTuple;
    }

    void from_json(const nlohmann::json& j, PlainTransportStat& st)
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
        if (j.contains("sctpState")) {
            j.at("sctpState").get_to(st.sctpState);
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
        
        if (j.contains("rtcpMux")) {
            j.at("rtcpMux").get_to(st.rtcpMux);
        }
        if (j.contains("comedia")) {
            j.at("comedia").get_to(st.comedia);
        }
        if (j.contains("tuple")) {
            j.at("tuple").get_to(st.tuple);
        }
        if (j.contains("rtcpTuple")) {
            j.at("rtcpTuple").get_to(st.rtcpTuple);
        }
    }

    void to_json(nlohmann::json& j, const PlainTransportOptions& st)
    {
        j["listenIps"] = st.listenIps;
        j["port"] = st.port;
        j["rtcpMux"] = st.rtcpMux;
        j["comedia"] = st.comedia;

        j["enableSctp"] = st.enableSctp;
        j["numSctpStreams"] = st.numSctpStreams;
        j["maxSctpMessageSize"] = st.maxSctpMessageSize;
        j["sctpSendBufferSize"] = st.sctpSendBufferSize;

        j["enableSrtp"] = st.enableSrtp;
        j["srtpCryptoSuite"] = st.srtpCryptoSuite;
        j["appData"] = st.appData;
    }

    void from_json(const nlohmann::json& j, PlainTransportOptions& st)
    {
        if (j.contains("listenIps")) {
            j.at("listenIps").get_to(st.listenIps);
        }
        if (j.contains("port")) {
            j.at("port").get_to(st.port);
        }
        if (j.contains("rtcpMux")) {
            j.at("rtcpMux").get_to(st.rtcpMux);
        }
        if (j.contains("comedia")) {
            j.at("comedia").get_to(st.comedia);
        }
        if (j.contains("enableSctp")) {
            j.at("enableSctp").get_to(st.enableSctp);
        }
        if (j.contains("numSctpStreams")) {
            j.at("numSctpStreams").get_to(st.numSctpStreams);
        }
        if (j.contains("maxSctpMessageSize")) {
            j.at("maxSctpMessageSize").get_to(st.maxSctpMessageSize);
        }
        if (j.contains("maxSctpMessageSize")) {
            j.at("maxSctpMessageSize").get_to(st.maxSctpMessageSize);
        }
        if (j.contains("enableSrtp")) {
            j.at("enableSrtp").get_to(st.enableSrtp);
        }
        if (j.contains("srtpCryptoSuite")) {
            j.at("srtpCryptoSuite").get_to(st.srtpCryptoSuite);
        }
        if (j.contains("appData")) {
            j.at("appData").get_to(st.appData);
        }
    }

}
