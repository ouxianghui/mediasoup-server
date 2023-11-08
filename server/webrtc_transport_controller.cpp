/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "webrtc_transport_controller.h"
#include "srv_logger.h"
#include "types.h"
#include "uuid.h"
#include "channel.h"
#include "payload_channel.h"

namespace srv {

    WebRtcTransportController::WebRtcTransportController(const std::shared_ptr<WebRtcTransportConstructorOptions>& options)
    : TransportController(options)
    {
        SRV_LOGD("WebRtcTransportController()");
        
        const auto& data = options->data;
        
        _data["iceRole"] = data["iceRole"];
        _data["iceParameters"] = data["iceParameters"];
        _data["iceCandidates"] = data["iceCandidates"];
        _data["iceState"] = data["iceState"];
        if (data.contains("iceSelectedTuple")) {
            _data["iceSelectedTuple"] = data["iceSelectedTuple"];
        }
        _data["dtlsParameters"] = data["dtlsParameters"];
        _data["dtlsState"] = data["dtlsState"];
        if (data.contains("dtlsRemoteCert")) {
            _data["dtlsRemoteCert"] = data["dtlsRemoteCert"];
        }
        _data["sctpParameters"] = data["sctpParameters"];
        _data["sctpState"] = data["sctpState"];
    }

    WebRtcTransportController::~WebRtcTransportController()
    {
        SRV_LOGD("~WebRtcTransportController()");
    }

    void WebRtcTransportController::init()
    {
        SRV_LOGD("init()");
        handleWorkerNotifications();
    }

    void WebRtcTransportController::destroy()
    {
        SRV_LOGD("destroy()");
    }

    void WebRtcTransportController::cleanData()
    {
        this->_data["iceState"] = "closed";
        this->_data["iceSelectedTuple"] = "";
        this->_data["dtlsState"] = "closed";

        if (this->_data["sctpState"]) {
            this->_data["sctpState"] = "closed";
        }
    }

    void WebRtcTransportController::close()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");
        
        cleanData();

        TransportController::close();
    }

    void WebRtcTransportController::onListenServerClosed()
    {
        SRV_LOGD("onListenServerClosed()");
        
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");

        cleanData();

        TransportController::onListenServerClosed();
    }

    void WebRtcTransportController::onRouterClosed()
    {
        SRV_LOGD("onRouterClosed()");
        
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");

        cleanData();
        
        TransportController::onRouterClosed();
    }

    nlohmann::json WebRtcTransportController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nlohmann::json();
        }
        
        return channel->request("transport.getStats", _internal.transportId, "{}");
    }

    void WebRtcTransportController::connect(const nlohmann::json& reqData)
    {
        SRV_LOGD("connect()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        // nlohmann::json reqData;
        // reqData["dtlsParameters"] = dtlsParameters;

        auto data = channel->request("transport.connect", _internal.transportId, reqData.dump());

        // Update data.
        this->_data["dtlsParameters"]["role"] = data["dtlsLocalRole"];
    }

    IceParameters WebRtcTransportController::restartIce()
    {
        SRV_LOGD("restartIce()");
        
        IceParameters iceParameters;
        
        auto channel = _channel.lock();
        if (!channel) {
            return iceParameters;
        }
        
        auto data = channel->request("transport.restartIce", _internal.transportId, "{}");
        
        iceParameters = data;

        this->_data["iceParameters"] = data["iceParameters"];

        return iceParameters;
        
    }

    void WebRtcTransportController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        auto self = std::dynamic_pointer_cast<WebRtcTransportController>(TransportController::shared_from_this());
        channel->_notificationSignal.connect(&WebRtcTransportController::onChannel, self);
    }

    void WebRtcTransportController::onChannel(const std::string& targetId, const std::string& event, const std::string& data)
    {
        SRV_LOGD("onChannel()");
        
        if (targetId != _internal.transportId) {
            return;
        }
        
        if (event == "icestatechange") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                std::string iceState = js["iceState"];
                this->_data["iceState"] = iceState;
                _iceStateChangeSignal(iceState);
            }
        }
        else if (event == "iceselectedtuplechange") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                auto iceSelectedTuple = js["iceSelectedTuple"];
                this->_data["iceSelectedTuple"] = iceSelectedTuple;
                TransportTuple tuple = js;
                _iceSelectedTupleChangeSignal(tuple);
            }
        }
        else if (event == "dtlsstatechange") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                auto dtlsState = js["dtlsState"];

                this->_data["dtlsState"] = dtlsState;

                if (dtlsState == "connected") {
                    this->_data["dtlsRemoteCert"] = js["dtlsRemoteCert"];
                }
                _dtlsStateChangeSignal(dtlsState);
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
    void to_json(nlohmann::json& j, const WebRtcTransportOptions& st)
    {
        j["listenIps"] = st.listenIps;
        j["port"] = st.port;
        j["enableUdp"] = st.enableUdp;
        j["enableTcp"] = st.enableTcp;
        j["preferUdp"] = st.preferUdp;
        j["preferTcp"] = st.preferTcp;
        j["initialAvailableOutgoingBitrate"] = st.initialAvailableOutgoingBitrate;
        j["minimumAvailableOutgoingBitrate"] = st.minimumAvailableOutgoingBitrate;
        j["enableSctp"] = st.enableSctp;
        j["numSctpStreams"] = st.numSctpStreams;
        j["maxSctpMessageSize"] = st.maxSctpMessageSize;
        j["maxIncomingBitrate"] = st.maxIncomingBitrate;
        j["sctpSendBufferSize"] = st.sctpSendBufferSize;
        j["appData"] = st.appData;
    }

    void from_json(const nlohmann::json& j, WebRtcTransportOptions& st)
    {
        if (j.contains("listenIps")) {
            j.at("listenIps").get_to(st.listenIps);
        }
        if (j.contains("port")) {
            j.at("port").get_to(st.port);
        }
        if (j.contains("enableUdp")) {
            j.at("enableUdp").get_to(st.enableUdp);
        }
        if (j.contains("enableTcp")) {
            j.at("enableTcp").get_to(st.enableTcp);
        }
        if (j.contains("preferUdp")) {
            j.at("preferUdp").get_to(st.preferUdp);
        }
        if (j.contains("preferTcp")) {
            j.at("preferTcp").get_to(st.preferTcp);
        }
        if (j.contains("initialAvailableOutgoingBitrate")) {
            j.at("initialAvailableOutgoingBitrate").get_to(st.initialAvailableOutgoingBitrate);
        }
        if (j.contains("minimumAvailableOutgoingBitrate")) {
            j.at("minimumAvailableOutgoingBitrate").get_to(st.minimumAvailableOutgoingBitrate);
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
        if (j.contains("maxIncomingBitrate")) {
            j.at("maxIncomingBitrate").get_to(st.maxIncomingBitrate);
        }
        if (j.contains("sctpSendBufferSize")) {
            j.at("sctpSendBufferSize").get_to(st.sctpSendBufferSize);
        }
        if (j.contains("appData")) {
            j.at("appData").get_to(st.appData);
        }
    }

    void to_json(nlohmann::json& j, const WebRtcTransportStat& st)
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
        
        j["iceRole"] = st.iceRole;
        j["iceState"] = st.iceState;
        j["iceSelectedTuple"] = st.iceSelectedTuple;
        j["dtlsState"] = st.dtlsState;
    }

    void from_json(const nlohmann::json& j, WebRtcTransportStat& st)
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
        
        if (j.contains("iceRole")) {
            j.at("iceRole").get_to(st.iceRole);
        }
        if (j.contains("iceState")) {
            j.at("iceState").get_to(st.iceState);
        }
        if (j.contains("iceSelectedTuple")) {
            j.at("iceSelectedTuple").get_to(st.iceSelectedTuple);
        }
        if (j.contains("dtlsState")) {
            j.at("dtlsState").get_to(st.dtlsState);
        }
    }

    void to_json(nlohmann::json& j, const IceParameters& st)
    {
        j["usernameFragment"] = st.usernameFragment;
        j["password"] = st.password;
        j["iceLite"] = st.iceLite;
    }

    void from_json(const nlohmann::json& j, IceParameters& st)
    {
        if (j.contains("usernameFragment")) {
            j.at("usernameFragment").get_to(st.usernameFragment);
        }
        if (j.contains("password")) {
            j.at("password").get_to(st.password);
        }
        if (j.contains("iceLite")) {
            j.at("iceLite").get_to(st.iceLite);
        }
    }

    void to_json(nlohmann::json& j, const IceCandidate& st)
    {
        j["foundation"] = st.foundation;
        j["priority"] = st.priority;
        j["ip"] = st.ip;
        j["protocol"] = st.protocol;
        j["port"] = st.port;
        j["type"] = st.type;
        j["tcpType"] = st.tcpType;
    }

    void from_json(const nlohmann::json& j, IceCandidate& st)
    {
        if (j.contains("foundation")) {
            j.at("foundation").get_to(st.foundation);
        }
        if (j.contains("priority")) {
            j.at("priority").get_to(st.priority);
        }
        if (j.contains("ip")) {
            j.at("ip").get_to(st.ip);
        }
        if (j.contains("protocol")) {
            j.at("protocol").get_to(st.protocol);
        }
        if (j.contains("port")) {
            j.at("port").get_to(st.port);
        }
        if (j.contains("type")) {
            j.at("type").get_to(st.type);
        }
        if (j.contains("tcpType")) {
            j.at("tcpType").get_to(st.tcpType);
        }
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
}
