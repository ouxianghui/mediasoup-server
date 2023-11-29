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

    std::shared_ptr<BaseTransportDump> DirectTransportController::dump()
    {
        auto channel = _channel.lock();
        if (!channel) {
            return nullptr;
        }
        
        auto respData = channel->request(FBS::Request::Method::TRANSPORT_DUMP, _internal.transportId);
        
        auto message = FBS::Message::GetMessage(respData.data());
        auto response = message->data_as_Response();
        auto dumpResponse = response->body_as_DirectTransport_DumpResponse();
        
        return parseDirectTransportDumpResponse(dumpResponse);
    }

    std::shared_ptr<BaseTransportStats> DirectTransportController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nullptr;
        }
        
        auto respData = channel->request(FBS::Request::Method::TRANSPORT_GET_STATS, _internal.transportId);
        
        auto message = FBS::Message::GetMessage(respData.data());
        auto response = message->data_as_Response();
        auto getStatsResponse = response->body_as_DirectTransport_GetStatsResponse();
        
        return parseGetStatsResponse(getStatsResponse);
    }

    void DirectTransportController::connect(const std::shared_ptr<ConnectData>& data)
    {
        SRV_LOGD("connect()");
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

    void DirectTransportController::sendRtcp(const std::vector<uint8_t>& data)
    {
        SRV_LOGD("sendRtcp()");
        
        if (data.empty()) {
            SRV_LOGE("rtcpPacket must be a Buffer");
            return;
        }

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }

        auto dataOffset = FBS::Transport::CreateSendRtcpNotificationDirect(channel->builder(), &data);
        
        channel->notify(FBS::Notification::Event::TRANSPORT_SEND_RTCP, FBS::Notification::Body::Transport_SendRtcpNotification, dataOffset, _internal.transportId);
    }

    void DirectTransportController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto self = std::dynamic_pointer_cast<DirectTransportController>(TransportController::shared_from_this());
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        channel->notificationSignal.connect(&DirectTransportController::onChannel, self);
    }

    void DirectTransportController::onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data)
    {
        SRV_LOGD("onChannel()");
        
        if (targetId != _internal.transportId) {
            return;
        }
        
        if (event == FBS::Notification::Event::TRANSPORT_TRACE) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_Transport_TraceNotification()) {
                auto eventData = *parseTransportTraceEventData(nf);
                this->traceSignal(eventData);
            }
        }
        else if (event == FBS::Notification::Event::DIRECTTRANSPORT_RTCP) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_DirectTransport_RtcpNotification()) {
                std::vector<uint8_t> rtcpData(nf->data()->begin(), nf->data()->end());
                this->rtcpSignal(rtcpData);
            }
        }
        else {
            SRV_LOGD("ignoring unknown event %u", (uint8_t)event);
        }
    }
}

namespace srv
{
    std::shared_ptr<DirectTransportDump> parseDirectTransportDumpResponse(const FBS::DirectTransport::DumpResponse* binary)
    {
        auto dump = std::make_shared<DirectTransportDump>();
        
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
        
        return dump;
    }

    std::shared_ptr<DirectTransportStat> parseGetStatsResponse(const FBS::DirectTransport::GetStatsResponse* binary)
    {
        auto stats = std::make_shared<DirectTransportStat>();
        
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
        
        return stats;
    }
}

//namespace srv
//{
//    void to_json(nlohmann::json& j, const DirectTransportStat& st)
//    {
//        j["type"] = st.type;
//        j["transportId"] = st.transportId;
//        j["timestamp"] = st.timestamp;
//
//        j["bytesReceived"] = st.bytesReceived;
//        j["recvBitrate"] = st.recvBitrate;
//        j["bytesSent"] = st.bytesSent;
//        j["sendBitrate"] = st.sendBitrate;
//        j["rtpBytesReceived"] = st.rtpBytesReceived;
//        j["rtpRecvBitrate"] = st.rtpRecvBitrate;
//
//        j["rtpBytesSent"] = st.rtpBytesSent;
//        j["rtpSendBitrate"] = st.rtpSendBitrate;
//        j["rtxBytesReceived"] = st.rtxBytesReceived;
//        j["rtxRecvBitrate"] = st.rtxRecvBitrate;
//        j["rtxBytesSent"] = st.rtxBytesSent;
//
//        j["rtxSendBitrate"] = st.rtxSendBitrate;
//        j["probationBytesSent"] = st.probationBytesSent;
//        j["probationSendBitrate"] = st.probationSendBitrate;
//        j["availableOutgoingBitrate"] = st.availableOutgoingBitrate;
//        j["availableIncomingBitrate"] = st.availableIncomingBitrate;
//        j["maxIncomingBitrate"] = st.maxIncomingBitrate;
//    }
//
//    void from_json(const nlohmann::json& j, DirectTransportStat& st)
//    {
//        if (j.contains("type")) {
//            j.at("type").get_to(st.type);
//        }
//        if (j.contains("transportId")) {
//            j.at("transportId").get_to(st.transportId);
//        }
//        if (j.contains("timestamp")) {
//            j.at("timestamp").get_to(st.timestamp);
//        }
//
//        if (j.contains("bytesReceived")) {
//            j.at("bytesReceived").get_to(st.bytesReceived);
//        }
//        if (j.contains("recvBitrate")) {
//            j.at("recvBitrate").get_to(st.recvBitrate);
//        }
//        if (j.contains("bytesSent")) {
//            j.at("bytesSent").get_to(st.bytesSent);
//        }
//        if (j.contains("sendBitrate")) {
//            j.at("sendBitrate").get_to(st.sendBitrate);
//        }
//        if (j.contains("rtpBytesReceived")) {
//            j.at("rtpBytesReceived").get_to(st.rtpBytesReceived);
//        }
//        if (j.contains("rtpRecvBitrate")) {
//            j.at("rtpRecvBitrate").get_to(st.rtpRecvBitrate);
//        }
//        if (j.contains("rtpBytesSent")) {
//            j.at("rtpBytesSent").get_to(st.rtpBytesSent);
//        }
//        if (j.contains("rtpSendBitrate")) {
//            j.at("rtpSendBitrate").get_to(st.rtpSendBitrate);
//        }
//        if (j.contains("rtxBytesReceived")) {
//            j.at("rtxBytesReceived").get_to(st.rtxBytesReceived);
//        }
//        if (j.contains("rtxRecvBitrate")) {
//            j.at("rtxRecvBitrate").get_to(st.rtxRecvBitrate);
//        }
//        if (j.contains("rtxBytesSent")) {
//            j.at("rtxBytesSent").get_to(st.rtxBytesSent);
//        }
//        if (j.contains("rtxSendBitrate")) {
//            j.at("rtxSendBitrate").get_to(st.rtxSendBitrate);
//        }
//        if (j.contains("probationBytesSent")) {
//            j.at("probationBytesSent").get_to(st.probationBytesSent);
//        }
//        if (j.contains("probationSendBitrate")) {
//            j.at("probationSendBitrate").get_to(st.probationSendBitrate);
//        }
//        if (j.contains("availableOutgoingBitrate")) {
//            j.at("availableOutgoingBitrate").get_to(st.availableOutgoingBitrate);
//        }
//        if (j.contains("availableIncomingBitrate")) {
//            j.at("availableIncomingBitrate").get_to(st.availableIncomingBitrate);
//        }
//        if (j.contains("maxIncomingBitrate")) {
//            j.at("maxIncomingBitrate").get_to(st.maxIncomingBitrate);
//        }
//    }
//}
