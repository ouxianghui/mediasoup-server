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
#include "message_builder.h"

namespace srv {

    DirectTransportController::DirectTransportController(const std::shared_ptr<DirectTransportConstructorOptions>& options)
    : AbstractTransportController(options)
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
        
        AbstractTransportController::close();
    }

    void DirectTransportController::onRouterClosed()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("onRouterClosed()");

        AbstractTransportController::onRouterClosed();
    }

    std::shared_ptr<BaseTransportDump> DirectTransportController::dump()
    {
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
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.transportId, FBS::Request::Method::TRANSPORT_GET_STATS);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto getStatsResponse = response->body_as_DirectTransport_GetStatsResponse();
        
        return parseGetStatsResponse(getStatsResponse);
    }

    void DirectTransportController::connect(const std::shared_ptr<ConnectParams>& params)
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

        flatbuffers::FlatBufferBuilder builder;
        
        auto dataOffset = FBS::Transport::CreateSendRtcpNotificationDirect(builder, &data);
        
        auto nfData = MessageBuilder::createNotification(builder,
                                                       _internal.transportId,
                                                       FBS::Notification::Event::TRANSPORT_SEND_RTCP,
                                                       FBS::Notification::Body::Transport_SendRtcpNotification,
                                                       dataOffset);
                                        
        channel->notify(nfData);
    }

    void DirectTransportController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto self = std::dynamic_pointer_cast<DirectTransportController>(AbstractTransportController::shared_from_this());
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        channel->notificationSignal.connect(&DirectTransportController::onChannel, self);
    }

    void DirectTransportController::onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data)
    {
        //SRV_LOGD("onChannel()");
        
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
