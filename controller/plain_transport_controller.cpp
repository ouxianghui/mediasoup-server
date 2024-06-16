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
#include "message_builder.h"

namespace srv {

    PlainTransportController::PlainTransportController(const std::shared_ptr<PlainTransportConstructorOptions>& options)
    : AbstractTransportController(options)
    {
        SRV_LOGD("PlainTransportController()");
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
        
        transportData()->sctpState = "closed";

        AbstractTransportController::close();
    }

    void PlainTransportController::onRouterClosed()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");
        
        transportData()->sctpState = "closed";

        AbstractTransportController::onRouterClosed();
    }

    std::shared_ptr<BaseTransportDump> PlainTransportController::dump()
    {
        SRV_LOGD("dump()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nullptr;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.transportId, FBS::Request::Method::TRANSPORT_DUMP);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto dumpResponse = response->body_as_PlainTransport_DumpResponse();
        
        return parsePlainTransportDumpResponse(dumpResponse);
    }

    std::shared_ptr<BaseTransportStats> PlainTransportController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nullptr;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.transportId, FBS::Request::Method::TRANSPORT_GET_STATS);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto getStatsResponse = response->body_as_PlainTransport_GetStatsResponse();
        
        return parseGetStatsResponse(getStatsResponse);
    }

    void PlainTransportController::connect(const std::shared_ptr<ConnectParams>& params)
    {
        SRV_LOGD("connect()");
        
        assert(params);

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqOffset = FBS::PlainTransport::CreateConnectRequestDirect(builder,
                                                                         params->ip.c_str(),
                                                                         params->port,
                                                                         params->rtcpPort,
                                                                         params->srtpParameters.serialize(builder)
                                                                         );
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.transportId, FBS::Request::Method::PLAINTRANSPORT_CONNECT, FBS::Request::Body::PlainTransport_ConnectRequest, reqOffset);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto connectResponse = response->body_as_PlainTransport_ConnectResponse();
        transportData()->tuple = *parseTuple(connectResponse->tuple());
        transportData()->rtcpTuple = *parseTuple(connectResponse->rtcpTuple());
        transportData()->srtpParameters = *parseSrtpParameters(connectResponse->srtpParameters());
    }

    void PlainTransportController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        auto self = std::dynamic_pointer_cast<PlainTransportController>(AbstractTransportController::shared_from_this());
        channel->notificationSignal.connect(&PlainTransportController::onChannel, self);
    }

    void PlainTransportController::onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data)
    {
        //SRV_LOGD("onChannel()");
        
        if (targetId != _internal.transportId) {
            return;
        }
        
        if (event == FBS::Notification::Event::PLAINTRANSPORT_TUPLE) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_PlainTransport_TupleNotification()) {
                transportData()->tuple = *parseTuple(nf->tuple());
                this->tupleSignal(transportData()->tuple);
            }
        }
        else if (event == FBS::Notification::Event::PLAINTRANSPORT_RTCP_TUPLE) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_PlainTransport_RtcpTupleNotification()) {
                transportData()->rtcpTuple = *parseTuple(nf->tuple());
                this->rtcpTupleSignal(transportData()->rtcpTuple);
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
                auto eventData = *parseTransportTraceEventData(nf);
                this->traceSignal(eventData);
            }
        }
        else {
            SRV_LOGD("ignoring unknown event %u", (uint8_t)event);
        }
    }
}

namespace srv
{
    std::shared_ptr<PlainTransportDump> parsePlainTransportDumpResponse(const FBS::PlainTransport::DumpResponse* binary)
    {
        auto dump = std::make_shared<PlainTransportDump>();
        
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
        
        dump->rtcpMux = binary->rtcpMux();
        dump->comedia = binary->comedia();
        
        if (auto tuple = binary->tuple()) {
            dump->tuple = *parseTuple(tuple);
        }
        dump->rtcpTuple = *parseTuple(binary->rtcpTuple());
        
        if (auto params = binary->srtpParameters()) {
            dump->srtpParameters = *parseSrtpParameters(params);
        }
        
        return dump;
    }

    std::shared_ptr<PlainTransportStat> parseGetStatsResponse(const FBS::PlainTransport::GetStatsResponse* binary)
    {
        auto stats = std::make_shared<PlainTransportStat>();
        
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
        stats->rtcpMux = binary->rtcpMux();
        stats->comedia = binary->comedia();
        stats->tuple = *parseTuple(binary->tuple());
        stats->rtcpTuple = *parseTuple(binary->rtcpTuple());
        
        return stats;
    }

    flatbuffers::Offset<FBS::PlainTransport::ConnectRequest> createConnectRequest(flatbuffers::FlatBufferBuilder& builder,
                                                                                  const std::string& ip,
                                                                                  uint16_t port,
                                                                                  uint16_t rtcpPort,
                                                                                  const SrtpParameters& srtpParameters)
    {
        return FBS::PlainTransport::CreateConnectRequestDirect(builder,
                                                               ip.c_str(),
                                                               port,
                                                               rtcpPort,
                                                               srtpParameters.serialize(builder)
                                                               );
    }
}

namespace srv
{
    void to_json(nlohmann::json& j, const PlainTransportOptions& st)
    {
        j["listenInfo"] = st.listenInfo;
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
        if (j.contains("listenInfo")) {
            j.at("listenInfo").get_to(st.listenInfo);
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
