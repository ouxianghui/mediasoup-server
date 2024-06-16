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
#include "message_builder.h"

namespace srv {

    WebRtcTransportController::WebRtcTransportController(const std::shared_ptr<WebRtcTransportConstructorOptions>& options)
    : AbstractTransportController(options)
    {
        SRV_LOGD("WebRtcTransportController()");
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
        transportData()->iceState = "closed";
        transportData()->iceSelectedTuple = {};
        transportData()->dtlsState = "closed";
        transportData()->sctpState = "closed";
    }

    void WebRtcTransportController::close()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");
        
        cleanData();

        AbstractTransportController::close();
    }

    void WebRtcTransportController::onWebRtcServerClosed()
    {
        SRV_LOGD("onWebRtcServerClosed()");
        
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");

        cleanData();

        AbstractTransportController::onWebRtcServerClosed();
    }

    void WebRtcTransportController::onRouterClosed()
    {
        SRV_LOGD("onRouterClosed()");
        
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");

        cleanData();
        
        AbstractTransportController::onRouterClosed();
    }

    std::shared_ptr<BaseTransportDump> WebRtcTransportController::dump()
    {
        SRV_LOGD("dump()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nullptr;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::TRANSPORT_DUMP);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto dumpResponse = response->body_as_WebRtcTransport_DumpResponse();
        
        return parseWebRtcTransportDumpResponse(dumpResponse);
    }

    std::shared_ptr<BaseTransportStats> WebRtcTransportController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nullptr;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::TRANSPORT_GET_STATS);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto getStatsResponse = response->body_as_WebRtcTransport_GetStatsResponse();
        
        return parseGetStatsResponse(getStatsResponse);
    }

    void WebRtcTransportController::connect(const std::shared_ptr<ConnectParams>& params)
    {
        SRV_LOGD("connect()");
        
        if (!params) {
            SRV_LOGE("params is null");
            return;
        }
            
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
    
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqOffset = createConnectRequest(builder, params->dtlsParameters);
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::WEBRTCTRANSPORT_CONNECT,
                                                     FBS::Request::Body::WebRtcTransport_ConnectRequest,
                                                     reqOffset);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto connectResponse = response->body_as_WebRtcTransport_ConnectResponse();
        
        this->transportData()->dtlsParameters.role = dtlsRoleFromFbs(connectResponse->dtlsLocalRole());
    }

    std::shared_ptr<IceParameters> WebRtcTransportController::restartIce()
    {
        SRV_LOGD("restartIce()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nullptr;
        }

        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::TRANSPORT_RESTART_ICE);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto restartIceResponse = response->body_as_Transport_RestartIceResponse();
        
        auto iceParameters = std::make_shared<IceParameters>();
        
        iceParameters->usernameFragment = restartIceResponse->usernameFragment()->str();
        iceParameters->password = restartIceResponse->password()->str();
        iceParameters->iceLite = restartIceResponse->iceLite();
        
        return iceParameters;
    }

    void WebRtcTransportController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        auto self = std::dynamic_pointer_cast<WebRtcTransportController>(AbstractTransportController::shared_from_this());
        channel->notificationSignal.connect(&WebRtcTransportController::onChannel, self);
    }

    void WebRtcTransportController::onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data)
    {
        //SRV_LOGD("onChannel()");
        
        if (targetId != _internal.transportId) {
            return;
        }
        
        if (event == FBS::Notification::Event::WEBRTCTRANSPORT_ICE_STATE_CHANGE) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_WebRtcTransport_IceStateChangeNotification()) {
                transportData()->iceState = iceStateFromFbs(nf->iceState());
                this->iceStateChangeSignal(transportData()->iceState);
            }
        }
        else if (event == FBS::Notification::Event::WEBRTCTRANSPORT_ICE_SELECTED_TUPLE_CHANGE) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_WebRtcTransport_IceSelectedTupleChangeNotification()) {
                transportData()->iceSelectedTuple = *parseTuple(nf->tuple());
                this->iceSelectedTupleChangeSignal(transportData()->iceSelectedTuple);
            }
        }
        else if (event == FBS::Notification::Event::WEBRTCTRANSPORT_DTLS_STATE_CHANGE) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_WebRtcTransport_DtlsStateChangeNotification()) {
                transportData()->dtlsState = dtlsStateFromFbs(nf->dtlsState());
                if (nf->dtlsState() == FBS::WebRtcTransport::DtlsState::CONNECTED) {
                    // TODO: assign dtlsRemoteCert
                }
                this->dtlsStateChangeSignal(transportData()->dtlsState);
            }
        }
        else if (event == FBS::Notification::Event::TRANSPORT_SCTP_STATE_CHANGE) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_Transport_SctpStateChangeNotification()) {
                transportData()->sctpState = parseSctpState(nf->sctpState());
                this->sctpStateChangeSignal(transportData()->sctpState);
            }
        }
        else if (event == FBS::Notification::Event::TRANSPORT_TRACE) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_Transport_TraceNotification()) {
                auto eventData = parseTransportTraceEventData(nf);
                this->traceSignal(*eventData);
            }
        }
        else {
            SRV_LOGD("ignoring unknown event %u", (uint8_t)event);
        }
    }
}

namespace srv {

    std::string iceStateFromFbs(FBS::WebRtcTransport::IceState iceState)
    {
        switch (iceState)
        {
            case FBS::WebRtcTransport::IceState::NEW:
                return "new";
            case FBS::WebRtcTransport::IceState::CONNECTED:
                return "connected";
            case FBS::WebRtcTransport::IceState::COMPLETED:
                return "completed";
            case FBS::WebRtcTransport::IceState::DISCONNECTED:
                return "disconnected";
            default:
                return "";
        }
    }

    std::string iceRoleFromFbs(FBS::WebRtcTransport::IceRole role)
    {
        switch (role)
        {
            case FBS::WebRtcTransport::IceRole::CONTROLLED:
                return "controlled";
            case FBS::WebRtcTransport::IceRole::CONTROLLING:
                return "controlling";
            default:
                return "";
        }
    }

    std::string iceCandidateTypeFromFbs(FBS::WebRtcTransport::IceCandidateType type)
    {
        switch (type)
        {
            case FBS::WebRtcTransport::IceCandidateType::HOST:
                return "host";
            default:
                return "";
        }
    }

    std::string iceCandidateTcpTypeFromFbs(FBS::WebRtcTransport::IceCandidateTcpType type)
    {
        switch (type)
        {
            case FBS::WebRtcTransport::IceCandidateTcpType::PASSIVE:
                return "passive";
            default:
                return "";
        }
    }

    std::string dtlsStateFromFbs(FBS::WebRtcTransport::DtlsState fbsDtlsState)
    {
        switch (fbsDtlsState)
        {
            case FBS::WebRtcTransport::DtlsState::NEW:
                return "new";
            case FBS::WebRtcTransport::DtlsState::CONNECTING:
                return "connecting";
            case FBS::WebRtcTransport::DtlsState::CONNECTED:
                return "connected";
            case FBS::WebRtcTransport::DtlsState::FAILED:
                return "failed";
            case FBS::WebRtcTransport::DtlsState::CLOSED:
                return "closed";
            default:
                return "";
        }
    }

    std::string dtlsRoleFromFbs(FBS::WebRtcTransport::DtlsRole role)
    {
        switch (role)
        {
            case FBS::WebRtcTransport::DtlsRole::AUTO:
                return "auto";
            case FBS::WebRtcTransport::DtlsRole::CLIENT:
                return "client";
            case FBS::WebRtcTransport::DtlsRole::SERVER:
                return "server";
        }
    }

    std::string fingerprintAlgorithmsFromFbs(FBS::WebRtcTransport::FingerprintAlgorithm algorithm)
    {
        switch (algorithm)
        {
            case FBS::WebRtcTransport::FingerprintAlgorithm::SHA1:
                return "sha-1";
            case FBS::WebRtcTransport::FingerprintAlgorithm::SHA224:
                return "sha-224";
            case FBS::WebRtcTransport::FingerprintAlgorithm::SHA256:
                return "sha-256";
            case FBS::WebRtcTransport::FingerprintAlgorithm::SHA384:
                return "sha-384";
            case FBS::WebRtcTransport::FingerprintAlgorithm::SHA512:
                return "sha-512";
            default:
                return "";
        }
    }

    FBS::WebRtcTransport::FingerprintAlgorithm fingerprintAlgorithmToFbs(const std::string& algorithm)
    {
        if ("sha-1" == algorithm) {
            return FBS::WebRtcTransport::FingerprintAlgorithm::SHA1;
        }
        else if ("sha-224" == algorithm) {
            return FBS::WebRtcTransport::FingerprintAlgorithm::SHA224;
        }
        else if ("sha-256" == algorithm) {
            return FBS::WebRtcTransport::FingerprintAlgorithm::SHA256;
        }
        else if ("sha-384" == algorithm) {
            return FBS::WebRtcTransport::FingerprintAlgorithm::SHA384;
        }
        else if ("sha-512" == algorithm) {
            return FBS::WebRtcTransport::FingerprintAlgorithm::SHA512;
        }

        SRV_LOGE("invalid FingerprintAlgorithm: %s", algorithm.c_str());
        
        return FBS::WebRtcTransport::FingerprintAlgorithm::MIN;
    }

    FBS::WebRtcTransport::DtlsRole dtlsRoleToFbs(const std::string& role)
    {
        if ("auto" == role) {
            return FBS::WebRtcTransport::DtlsRole::AUTO;
        }
        if ("client" == role) {
            return FBS::WebRtcTransport::DtlsRole::CLIENT;
        }
        if ("server" == role) {
            return FBS::WebRtcTransport::DtlsRole::SERVER;
        }

        SRV_LOGE("invalid DtlsRole: %s", role.c_str());
        
        return FBS::WebRtcTransport::DtlsRole::MIN;
    }

    std::shared_ptr<WebRtcTransportDump> parseWebRtcTransportDumpResponse(const FBS::WebRtcTransport::DumpResponse* binary)
    {
        auto dump = std::make_shared<WebRtcTransportDump>();
        
        auto baseDump = parseBaseTransportDump(binary->base());
        
        dump->id = baseDump->id;
        dump->direct = baseDump->direct;
        dump->producerIds = baseDump->producerIds;
        dump->consumerIds = baseDump->consumerIds;
        dump->mapSsrcConsumerId = baseDump->mapSsrcConsumerId;
        dump->mapRtxSsrcConsumerId = baseDump->mapRtxSsrcConsumerId;
        dump->recvRtpHeaderExtensions = baseDump->recvRtpHeaderExtensions;
        dump->rtpListener = baseDump->rtpListener;
        dump->maxMessageSize = baseDump->maxMessageSize;
        dump->dataProducerIds = baseDump->dataProducerIds;
        dump->dataConsumerIds = baseDump->dataConsumerIds;
        dump->sctpParameters = baseDump->sctpParameters;
        dump->sctpState = baseDump->sctpState;
        dump->sctpListener = baseDump->sctpListener;
        dump->traceEventTypes = baseDump->traceEventTypes;
        
        
        dump->iceRole = iceRoleFromFbs(binary->iceRole());
        dump->iceParameters = *parseIceParameters(binary->iceParameters());
        
        for (const auto& item : *binary->iceCandidates()) {
            dump->iceCandidates.emplace_back(*parseIceCandidate(item));
        }
        
        dump->iceState = iceStateFromFbs(binary->iceState());
        //dump->iceSelectedTuple = *parseTuple(binary->iceSelectedTuple());
        dump->dtlsParameters = *parseDtlsParameters(binary->dtlsParameters());
        dump->dtlsState = dtlsStateFromFbs(binary->dtlsState());
        
        // TODO: missing?
        //dump->dtlsRemoteCert = binary->dtlsRemoteCert()->str();
        
        return dump;
    }

    flatbuffers::Offset<FBS::WebRtcTransport::ConnectRequest> createConnectRequest(flatbuffers::FlatBufferBuilder& builder, const DtlsParameters& dtlsParameters)
    {
        auto paramsOffset = serializeDtlsParameters(builder, dtlsParameters);
        
        return FBS::WebRtcTransport::CreateConnectRequest(builder, paramsOffset);
    }

    std::shared_ptr<WebRtcTransportStat> parseGetStatsResponse(const FBS::WebRtcTransport::GetStatsResponse* binary)
    {
        auto stats = std::make_shared<WebRtcTransportStat>();

        auto baseStats = parseBaseTransportStats(binary->base());
        
        stats->transportId = baseStats->transportId;
        stats->timestamp = baseStats->timestamp;
        stats->sctpState = baseStats->sctpState;
        stats->bytesReceived = baseStats->bytesReceived;
        stats->recvBitrate = baseStats->recvBitrate;
        stats->bytesSent = baseStats->bytesSent;
        stats->sendBitrate = baseStats->sendBitrate;
        stats->rtpBytesReceived = baseStats->rtpBytesReceived;
        stats->rtpRecvBitrate = baseStats->rtpRecvBitrate;
        stats->rtpBytesSent = baseStats->rtpBytesSent;
        stats->rtpSendBitrate = baseStats->rtpSendBitrate;
        stats->rtxBytesReceived = baseStats->rtxBytesReceived;
        stats->rtxRecvBitrate = baseStats->rtxRecvBitrate;
        stats->rtxBytesSent = baseStats->rtxBytesSent;
        stats->rtxSendBitrate = baseStats->rtxSendBitrate;
        stats->probationBytesSent = baseStats->probationBytesSent;
        stats->probationSendBitrate = baseStats->probationSendBitrate;
        stats->availableOutgoingBitrate = baseStats->availableOutgoingBitrate;
        stats->availableIncomingBitrate = baseStats->availableIncomingBitrate;
        stats->maxIncomingBitrate = baseStats->maxIncomingBitrate;
        
        // TODO: type?
        stats->type = "";
        stats->iceRole = iceRoleFromFbs(binary->iceRole());
        stats->iceState = iceStateFromFbs(binary->iceState());
        stats->iceSelectedTuple = *parseTuple(binary->iceSelectedTuple());
        stats->dtlsState = dtlsStateFromFbs(binary->dtlsState());
        
        return stats;
    }

    std::shared_ptr<IceCandidate> parseIceCandidate(const FBS::WebRtcTransport::IceCandidate* binary)
    {
        auto candidate = std::make_shared<IceCandidate>();
        
        candidate->foundation = binary->foundation()->str();
        candidate->priority = binary->priority();
        candidate->address = binary->address()->str();
        candidate->protocol = parseProtocol(binary->protocol());
        candidate->port = binary->port();
        candidate->type = iceCandidateTypeFromFbs(binary->type());
        if (binary->tcpType().has_value()) {
            candidate->tcpType = iceCandidateTcpTypeFromFbs(binary->tcpType().value());
        }
        
        return candidate;
    }

    std::shared_ptr<IceParameters> parseIceParameters(const FBS::WebRtcTransport::IceParameters* binary)
    {
        auto parameters = std::make_shared<IceParameters>();
        
        parameters->usernameFragment = binary->usernameFragment()->str();
        parameters->password = binary->password()->str();
        parameters->iceLite = binary->iceLite();
        
        return parameters;
    }

    std::shared_ptr<DtlsParameters> parseDtlsParameters(const FBS::WebRtcTransport::DtlsParameters* binary)
    {
        auto parameters = std::make_shared<DtlsParameters>();
        
        parameters->role = dtlsRoleFromFbs(binary->role());
        
        for (const auto& item : *binary->fingerprints()) {
            DtlsFingerprint fingerprint;
            fingerprint.algorithm = fingerprintAlgorithmsFromFbs(item->algorithm());
            fingerprint.value = item->value()->str();
            parameters->fingerprints.emplace_back(fingerprint);
        }
        
        return parameters;
    }

    flatbuffers::Offset<FBS::WebRtcTransport::DtlsParameters> serializeDtlsParameters(flatbuffers::FlatBufferBuilder& builder, const DtlsParameters& dtlsParameters)
    {
        std::vector<flatbuffers::Offset<FBS::WebRtcTransport::Fingerprint>> fingerprints;
        
        for (const auto& item : dtlsParameters.fingerprints) {
            auto algorithm = fingerprintAlgorithmToFbs(item.algorithm);
            SRV_LOGD("dtlsParameters.fingerprints, algorithm: %u, value: %s", (uint8_t)algorithm, item.value.c_str());
            auto ft = FBS::WebRtcTransport::CreateFingerprintDirect(builder, algorithm, item.value.c_str());
            fingerprints.emplace_back(ft);
        }
        
        FBS::WebRtcTransport::DtlsRole role = dtlsRoleToFbs(dtlsParameters.role);
        
        return FBS::WebRtcTransport::CreateDtlsParametersDirect(builder, &fingerprints, role);
    }
}

namespace srv
{

    void to_json(nlohmann::json& j, const WebRtcTransportOptions& st)
    {
        j["listenInfos"] = st.listenInfos;
        j["port"] = st.port;
        j["enableUdp"] = st.enableUdp;
        j["enableTcp"] = st.enableTcp;
        j["preferUdp"] = st.preferUdp;
        j["preferTcp"] = st.preferTcp;
        j["iceConsentTimeout"] = st.iceConsentTimeout;
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
        if (j.contains("listenInfos")) {
            j.at("listenInfos").get_to(st.listenInfos);
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
        if (j.contains("iceConsentTimeout")) {
            j.at("iceConsentTimeout").get_to(st.iceConsentTimeout);
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
        j["address"] = st.address;
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
        if (j.contains("address")) {
            j.at("address").get_to(st.address);
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

}
