/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "pipe_transport_controller.h"
#include <regex>
#include <algorithm>
#include "srv_logger.h"
#include "types.h"
#include "uuid.h"
#include "ortc.h"
#include "channel.h"
#include "utils.h"
#include "consumer_controller.h"
#include "producer_controller.h"
#include "message_builder.h"

namespace srv {

    PipeTransportController::PipeTransportController(const std::shared_ptr<PipeTransportConstructorOptions>& options)
    : AbstractTransportController(options)
    {
        SRV_LOGD("PipeTransportController()");
    }

    PipeTransportController::~PipeTransportController()
    {
        SRV_LOGD("~PipeTransportController()");
    }

    void PipeTransportController::init()
    {
        SRV_LOGD("init()");
        handleWorkerNotifications();
    }

    void PipeTransportController::destroy()
    {
        SRV_LOGD("destroy()");
    }

    void PipeTransportController::close()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");
        
        transportData()->sctpState = "closed";

        AbstractTransportController::close();
    }

    void PipeTransportController::onRouterClosed()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("onRouterClosed()");
        
        transportData()->sctpState = "closed";

        AbstractTransportController::onRouterClosed();
    }

    std::shared_ptr<BaseTransportStats> PipeTransportController::getStats()
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
        
        auto getStatsResponse = response->body_as_PipeTransport_GetStatsResponse();
        
        return parseGetStatsResponse(getStatsResponse);
    }

    void PipeTransportController::connect(const std::shared_ptr<ConnectParams>& params)
    {
        SRV_LOGD("connect()");
        
        assert(params);

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;

        auto reqOffset = FBS::PipeTransport::CreateConnectRequestDirect(builder,
                                                                        params->ip.c_str(),
                                                                        params->port,
                                                                        params->srtpParameters.serialize(builder)
                                                                        );
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::PIPETRANSPORT_CONNECT,
                                                     FBS::Request::Body::PipeTransport_ConnectRequest,
                                                     reqOffset);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto connectResponse = response->body_as_PipeTransport_ConnectResponse();
        
        transportData()->tuple = *parseTuple(connectResponse->tuple());
    }

    std::shared_ptr<IConsumerController> PipeTransportController::consume(const std::shared_ptr<ConsumerOptions>& options)
    {
        SRV_LOGD("consume()");
        
        std::shared_ptr<IConsumerController> consumerController;
        
        auto channel = _channel.lock();
        if (!channel) {
            return consumerController;
        }
        
        const std::string& producerId = options->producerId;
        const bool pipe = true;
        const nlohmann::json& appData = options->appData;
        
        if (producerId.empty()) {
            SRV_LOGE("missing producerId");
            return consumerController;
        }

        auto producerController = _getProducerController(producerId);
        if (!producerController) {
            SRV_LOGE("Producer with id '%s' not found", producerId.c_str());
            return consumerController;
        }
        
        auto consumableRtpParameters = producerController->consumableRtpParameters();
        bool enableRtx = transportData()->rtx;
        
        // This may throw.
        auto rtpParameters = ortc::getPipeConsumerRtpParameters(consumableRtpParameters, enableRtx);
        
        auto consumerId = uuid::uuidv4();

        flatbuffers::FlatBufferBuilder builder;
        
        auto reqOffset = createConsumeRequest(builder,
                                              consumerId,
                                              producerController,
                                              rtpParameters
                                              );
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::TRANSPORT_CONSUME,
                                                     FBS::Request::Body::Transport_ConsumeRequest,
                                                     reqOffset);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto consumeResponse = response->body_as_Transport_ConsumeResponse();
        
        bool paused_ = consumeResponse->paused();
        bool producerPaused_ = consumeResponse->producerPaused();
        
        ConsumerInternal internal;
        internal.transportId = _internal.transportId;
        internal.consumerId = consumerId;
        
        ConsumerData data;
        data.producerId = producerId;
        data.kind = producerController->kind();
        data.rtpParameters = rtpParameters;
        data.type = pipe ? "pipe" : producerController->type();
        
        consumerController = std::make_shared<ConsumerController>(internal,
                                                                  data,
                                                                  _channel.lock(),
                                                                  appData,
                                                                  paused_,
                                                                  producerPaused_,
                                                                  ConsumerScore(),
                                                                  ConsumerLayers());
        consumerController->init();
        
        _consumerControllers.emplace(std::make_pair(consumerController->id(), consumerController));
        
        auto wself = std::weak_ptr<ITransportController>(AbstractTransportController::shared_from_this());
        auto removeLambda = [id = consumerController->id(), wself]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (auto ptc = std::dynamic_pointer_cast<PipeTransportController>(self)) {
                ptc->removeConsumerController(id);
            }
            else {
                assert(0);
            }
        };
        
        consumerController->closeSignal.connect(removeLambda);
        consumerController->producerCloseSignal.connect(removeLambda);
        
        this->newConsumerSignal(consumerController);

        return consumerController;
    }

    void PipeTransportController::removeConsumerController(const std::string& id)
    {
        if (_consumerControllers.contains(id)) {
            _consumerControllers.erase(id);
        }
    }

    void PipeTransportController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        auto self = std::dynamic_pointer_cast<PipeTransportController>(AbstractTransportController::shared_from_this());
        channel->notificationSignal.connect(&PipeTransportController::onChannel, self);
    }

    void PipeTransportController::onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data)
    {
        //SRV_LOGD("onChannel()");
        
        if (targetId != _internal.transportId) {
            return;
        }
        
        if (event == FBS::Notification::Event::TRANSPORT_SCTP_STATE_CHANGE) {
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

namespace srv {

    std::shared_ptr<PipeTransportDump> parsePipeTransportDumpResponse(const FBS::PipeTransport::DumpResponse* binary)
    {
        auto dump = std::make_shared<PipeTransportDump>();
        
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
        
        dump->tuple = *parseTuple(binary->tuple());
        dump->rtx = binary->rtx();
        
        if (auto params = binary->srtpParameters()) {
            dump->srtpParameters = *parseSrtpParameters(params);
        }
        
        return dump;
    }

    std::shared_ptr<PipeTransportStat> parseGetStatsResponse(const FBS::PipeTransport::GetStatsResponse* binary)
    {
        auto stats = std::make_shared<PipeTransportStat>();
        
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
        
        stats->type = "";
        stats->tuple = *parseTuple(binary->tuple());
        
        return stats;
    }

    flatbuffers::Offset<FBS::Transport::ConsumeRequest> createConsumeRequest(flatbuffers::FlatBufferBuilder& builder,
                                                                             const std::string& consumerId,
                                                                             std::shared_ptr<IProducerController> producer,
                                                                             const RtpParameters& rtpParameters)
    {
        auto rtpParametersOffset = rtpParameters.serialize(builder);
        
        std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpEncodingParameters>> consumableRtpEncodings;
        for (const auto& encoding : producer->consumableRtpParameters().encodings) {
            consumableRtpEncodings.emplace_back(encoding.serialize(builder));
        }
        
        return FBS::Transport::CreateConsumeRequestDirect(builder,
                                                          consumerId.c_str(),
                                                          producer->id().c_str(),
                                                          producer->kind() == "audio" ? FBS::RtpParameters::MediaKind::AUDIO : FBS::RtpParameters::MediaKind::VIDEO,
                                                          rtpParametersOffset,
                                                          FBS::RtpParameters::Type::PIPE,
                                                          &consumableRtpEncodings
                                                          );
    }
}
