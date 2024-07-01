/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "router_controller.h"
#include "srv_logger.h"
#include "types.h"
#include "uuid.h"
#include "ortc.h"
#include "channel.h"
#include "rtp_observer_controller.h"
#include "webrtc_server_controller.h"
#include "webrtc_transport_controller.h"
#include "plain_transport_controller.h"
#include "direct_transport_controller.h"
#include "pipe_transport_controller.h"
#include "audio_level_observer_controller.h"
#include "active_speaker_observer_controller.h"
#include "consumer_controller.h"
#include "producer_controller.h"
#include "data_consumer_controller.h"
#include "data_producer_controller.h"
#include "FBS/worker.h"
#include "message_builder.h"

namespace srv {

    RouterController::RouterController(const RouterInternal& internal,
                                       const RouterData& data,
                                       const std::shared_ptr<Channel>& channel,
                                       const nlohmann::json& appData)
    : _internal(internal)
    , _data(data)
    , _channel(channel)
    , _appData(appData)
    {
        SRV_LOGD("RouterController()");
        
        _getProducerController = [&](const std::string& producerId) {
            return this->getProducerController(producerId);
        };
        
        _getDataProducerController = [&](const std::string& dataProducerId) {
            return this->getDataProducerController(dataProducerId);
        };
        
        _getRouterRtpCapabilities = [&]() {
            return this->rtpCapabilities();
        };
        
    }

    RouterController::~RouterController()
    {
        SRV_LOGD("~RouterController()");
    }

    void RouterController::init()
    {
        SRV_LOGD("init()");
    }

    void RouterController::destroy()
    {
        SRV_LOGD("destroy()");
    }

    std::shared_ptr<IProducerController> RouterController::getProducerController(const std::string& producerId)
    {
        std::shared_ptr<IProducerController> controller;
        
        if (_producerControllers.contains(producerId)) {
            controller = _producerControllers[producerId];
        }
        
        return controller;
    }

    std::shared_ptr<IDataProducerController> RouterController::getDataProducerController(const std::string& dataProducerId)
    {
        std::shared_ptr<IDataProducerController> controller;

        if (_dataProducerControllers.contains(dataProducerId)) {
            controller = _dataProducerControllers[dataProducerId];
        }

        return controller;
    }

    void RouterController::clear()
    {
        std::threadsafe_unordered_map<std::string, std::shared_ptr<ITransportController>> transportControllers = _transportControllers.value();
        
        // Close every Transport.
        transportControllers.for_each([](const auto& item) {
            item.second->onRouterClosed();
        });
        
        _producerControllers.clear();

        std::threadsafe_unordered_map<std::string, std::shared_ptr<IRtpObserverController>> rtpObserverControllers = _rtpObserverControllers.value();

        // Close every RtpObserver.
        rtpObserverControllers.for_each([](const auto& item) {
            item.second->onRouterClosed();
        });
        
        // Clear the DataProducers map.
        _dataProducerControllers.clear();
    }

    void RouterController::close()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");

        _closed = true;

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqOffset = FBS::Worker::CreateCloseRouterRequestDirect(builder, _internal.routerId.c_str());
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, "", FBS::Request::Method::WORKER_CLOSE_ROUTER, FBS::Request::Body::Worker_CloseRouterRequest, reqOffset);

        channel->request(reqId, reqData);
        
        clear();

        this->closeSignal(shared_from_this());
    }

    void RouterController::onWorkerClosed()
    {
        
        if (_closed) {
            return;
        }

        SRV_LOGD("onWorkerClosed()");

        _closed = true;

        clear();

        this->workerCloseSignal();
        
        this->closeSignal(shared_from_this());
    }
        
    std::shared_ptr<RouterDump> RouterController::dump()
    {
        SRV_LOGD("dump()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nullptr;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.routerId, FBS::Request::Method::ROUTER_DUMP);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto dumpResponse = response->body_as_Router_DumpResponse();
        
        return parseRouterDumpResponse(dumpResponse);
    }

    std::shared_ptr<ITransportController> RouterController::createWebRtcTransportController(const std::shared_ptr<WebRtcTransportOptions>& options)
    {
        SRV_LOGD("createWebRtcTransportController()");
        
        std::shared_ptr<WebRtcTransportController> transportController;
        
        if (!options) {
            SRV_LOGE("options must be a valid pointer");
            return transportController;
        }
        
        auto channel = _channel.lock();
        if (!channel) {
            SRV_LOGE("channel must be a valid pointer");
            return transportController;
        }
        
        auto listenInfos = options->listenInfos;
        bool enableUdp = options->enableUdp;
        bool enableTcp = options->enableTcp;
        const bool preferUdp = options->preferUdp;
        const bool preferTcp = options->preferTcp;
        const int32_t initialAvailableOutgoingBitrate = options->initialAvailableOutgoingBitrate;
        const bool enableSctp = options->enableSctp;
        const NumSctpStreams& numSctpStreams = options->numSctpStreams;
        const int32_t maxSctpMessageSize = options->maxSctpMessageSize;
        const int32_t sctpSendBufferSize = options->sctpSendBufferSize;
        const nlohmann::json& appData = options->appData;
        const auto& webRtcServer = options->webRtcServer;
        uint8_t iceConsentTimeout = options->iceConsentTimeout;
        
        if (!webRtcServer && listenInfos.empty()) {
            SRV_LOGE("missing webRtcServer, listenInfos (one of them is mandatory)");
            return transportController;
        }
        else if (webRtcServer && !listenInfos.empty())
        {
            SRV_LOGE("only one of webRtcServer, listenInfos must be given");
            //return transportController;
        }
        
        TransportInternal internal;
        internal.routerId = _internal.routerId;
        internal.transportId = uuid::uuidv4();
        
        //// If webRtcServer is given, then do not force default values for enableUdp
        //// and enableTcp. Otherwise set them if unset.
        //if (webRtcServer) {
        //    enableUdp = true;
        //    enableTcp = true;
        //}
        //else {
        //    enableUdp = true;
        //    enableTcp = false;
        //}
        //
        //// Convert deprecated TransportListenIps to TransportListenInfos.
        //if (!listenIps.empty()) {
        //    // Normalize IP strings to TransportListenInfo objects.
        //    std::vector<TransportListenInfo> listenInfosTmp;
        //    for (const auto& listenIp : listenIps) {
        //        TransportListenInfo info;
        //        info.ip = listenIp.ip;
        //        info.announcedIp = listenIp.announcedIp;
        //        listenInfosTmp.emplace_back(info);
        //    }
        //
        //    listenInfos.clear();
        //
        //    std::vector<std::string> orderedProtocols;
        //
        //    if (enableUdp && (preferUdp || !enableTcp || !preferTcp)) {
        //        //if (enableUdp && (!enableTcp || preferUdp)) {
        //        orderedProtocols.emplace_back("udp");
        //        if (enableTcp) {
        //            orderedProtocols.emplace_back("tcp");
        //        }
        //    }
        //    else if (enableTcp && ((preferTcp && !preferUdp) || !enableUdp)) {
        //    //else if (enableTcp && (!enableUdp || (preferTcp && !preferUdp))) {
        //        orderedProtocols.emplace_back("tcp");
        //        if (enableUdp) {
        //            orderedProtocols.emplace_back("udp");
        //        }
        //    }
        //    for (const auto& listenIp : listenInfosTmp) {
        //        for (const auto& protocol : orderedProtocols) {
        //            TransportListenInfo info;
        //            info.protocol = protocol;
        //            info.ip = listenIp.ip;
        //            info.announcedAddress = listenIp.announcedIp;
        //            info.port = port;
        //            listenInfos.emplace_back(info);
        //        }
        //    }
        //}
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        flatbuffers::Offset<void> listenInfoOffset;
        
        if (webRtcServer) {
            listenInfoOffset = FBS::WebRtcTransport::CreateListenServerDirect(builder, webRtcServer->id().c_str()).Union();
        }
        else {
            std::vector<flatbuffers::Offset<FBS::Transport::ListenInfo>> listenInfos_;
            for (const auto& item : listenInfos) {
                auto portRange = FBS::Transport::CreatePortRange(builder, item.portRange.min, item.portRange.max);
                auto socketFlags = FBS::Transport::CreateSocketFlags(builder, item.flags.ipv6Only, item.flags.udpReusePort);
                auto ip = !item.announcedAddress.empty() ? item.announcedAddress : item.announcedIp;
                auto infoOffset = FBS::Transport::CreateListenInfoDirect(builder,
                                                                         item.protocol == "udp" ? FBS::Transport::Protocol::UDP : FBS::Transport::Protocol::TCP,
                                                                         item.ip.c_str(),
                                                                         ip.c_str(),
                                                                         item.port,
                                                                         portRange,
                                                                         socketFlags,
                                                                         item.sendBufferSize,
                                                                         item.recvBufferSize
                                                                         );
                listenInfos_.emplace_back(infoOffset);
            }
            listenInfoOffset = FBS::WebRtcTransport::CreateListenIndividualDirect(builder, &listenInfos_).Union();
        }
        
        auto numSctpStreamsOffset = FBS::SctpParameters::CreateNumSctpStreams(builder, numSctpStreams.OS, numSctpStreams.MIS);
        bool isDataChannel = true;
        auto baseTransportOptionsOffset = FBS::Transport::CreateOptions(builder,
                                                                        false,
                                                                        flatbuffers::nullopt,
                                                                        initialAvailableOutgoingBitrate,
                                                                        enableSctp,
                                                                        numSctpStreamsOffset,
                                                                        maxSctpMessageSize,
                                                                        sctpSendBufferSize,
                                                                        isDataChannel
                                                                        );
        
        auto webRtcTransportOptionsOffset = FBS::WebRtcTransport::CreateWebRtcTransportOptions(builder,
                                                                                               baseTransportOptionsOffset,
                                                                                               webRtcServer ? FBS::WebRtcTransport::Listen::ListenServer : FBS::WebRtcTransport::Listen::ListenIndividual,
                                                                                               listenInfoOffset,
                                                                                               enableUdp,
                                                                                               enableTcp,
                                                                                               preferUdp,
                                                                                               preferTcp,
                                                                                               iceConsentTimeout
                                                                                               );
        
        auto reqOffset = FBS::Router::CreateCreateWebRtcTransportRequestDirect(builder, internal.transportId.c_str(), webRtcTransportOptionsOffset);
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.routerId,
                                                     webRtcServer ? FBS::Request::Method::ROUTER_CREATE_WEBRTCTRANSPORT_WITH_SERVER : FBS::Request::Method::ROUTER_CREATE_WEBRTCTRANSPORT,
                                                     FBS::Request::Body::Router_CreateWebRtcTransportRequest,
                                                     reqOffset);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto dumpResponse = response->body_as_WebRtcTransport_DumpResponse();
        
        auto dump = parseWebRtcTransportDumpResponse(dumpResponse);
        
        auto webRtcTransportData = std::make_shared<WebRtcTransportData>();
        webRtcTransportData->iceRole = dump->iceRole;
        webRtcTransportData->iceParameters = dump->iceParameters;
        webRtcTransportData->iceCandidates = dump->iceCandidates;
        webRtcTransportData->iceState = dump->iceState;
        webRtcTransportData->iceSelectedTuple = dump->iceSelectedTuple;
        webRtcTransportData->dtlsParameters = dump->dtlsParameters;
        webRtcTransportData->dtlsState = dump->dtlsState;
        webRtcTransportData->dtlsRemoteCert = dump->dtlsRemoteCert;
        webRtcTransportData->sctpParameters = dump->sctpParameters;
        webRtcTransportData->sctpState = dump->sctpState;
        
        auto wtcOptions = std::make_shared<WebRtcTransportConstructorOptions>();
        wtcOptions->internal = internal;
        wtcOptions->data = webRtcTransportData;
        wtcOptions->channel = _channel.lock();
        wtcOptions->appData = appData;
        wtcOptions->getRouterRtpCapabilities = _getRouterRtpCapabilities;
        wtcOptions->getProducerController = _getProducerController;
        wtcOptions->getDataProducerController = _getDataProducerController;
        
        transportController = std::make_shared<WebRtcTransportController>(wtcOptions);
        transportController->init();
        _transportControllers.emplace(std::make_pair(internal.transportId, transportController));
        
        connectSignals(transportController);
        
        this->newTransportSignal(transportController);
        
        // TODO: check thread
        if (webRtcServer) {
            webRtcServer->handleWebRtcTransport(transportController);
        }

        return transportController;
    }

    std::shared_ptr<ITransportController> RouterController::createPlainTransportController(const std::shared_ptr<PlainTransportOptions>& options)
    {
        SRV_LOGD("createPlainTransportController()");
        
        std::shared_ptr<PlainTransportController> transportController;
        
        if (!options) {
            SRV_LOGE("options must be a valid pointer");
            return transportController;
        }
        
        auto channel = _channel.lock();
        if (!channel) {
            SRV_LOGE("channel must be a valid pointer");
            return transportController;
        }
        
        //const auto& listenIp = options->listenIp;
        auto listenInfo = options->listenInfo;
        auto rtcpListenInfo = options->rtcpListenInfo;
        //const uint16_t port = options->port;
        const bool rtcpMux = options->rtcpMux;
        const bool comedia = options->comedia;
        const bool enableSctp = options->enableSctp;
        const bool enableSrtp = options->enableSrtp;
        const NumSctpStreams& numSctpStreams = options->numSctpStreams;
        const std::string& srtpCryptoSuite = options->srtpCryptoSuite;
        const int32_t maxSctpMessageSize = options->maxSctpMessageSize;
        const int32_t sctpSendBufferSize = options->sctpSendBufferSize;
        const nlohmann::json& appData = options->appData;

        //if (listenInfo.ip.empty() && listenIp.ip.empty()) {
        if (listenInfo.ip.empty()) {
            SRV_LOGE("missing listenInfo is mandatory");
            return nullptr;
        }
        
        // If rtcpMux is enabled, ignore rtcpListenInfo.
        if (rtcpMux) {
            rtcpListenInfo.ip.clear();
            rtcpListenInfo.announcedIp.clear();
            rtcpListenInfo.port = -1;
        }
        
        //// Convert deprecated TransportListenIps to TransportListenInfos.
        //if (!listenIp.ip.empty()) {
        //    // Normalize IP string to TransportListenInfo object.
        //    listenInfo.protocol = "udp";
        //    listenInfo.ip = listenIp.ip;
        //    listenInfo.announcedAddress = listenIp.announcedIp;
        //    listenInfo.port = port;
        //}
        
        TransportInternal internal;
        internal.routerId = _internal.routerId;
        internal.transportId = uuid::uuidv4();
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto numSctpStreamsOffset = FBS::SctpParameters::CreateNumSctpStreams(builder, numSctpStreams.OS, numSctpStreams.MIS);
        bool isDataChannel = false;
        auto baseTransportOptionsOffset = FBS::Transport::CreateOptions(builder,
                                                                        false,
                                                                        flatbuffers::nullopt,
                                                                        flatbuffers::nullopt,
                                                                        enableSctp,
                                                                        numSctpStreamsOffset,
                                                                        maxSctpMessageSize,
                                                                        sctpSendBufferSize,
                                                                        isDataChannel
                                                                        );
        
        auto listnInfoPortRange = FBS::Transport::CreatePortRange(builder, listenInfo.portRange.min, listenInfo.portRange.max);
        
        auto listenInfoSocketFlags = FBS::Transport::CreateSocketFlags(builder, listenInfo.flags.ipv6Only, listenInfo.flags.udpReusePort);
        
        auto listenInfoOffset = FBS::Transport::CreateListenInfoDirect(builder,
                                                                       listenInfo.protocol == "udp" ? FBS::Transport::Protocol::UDP : FBS::Transport::Protocol::TCP,
                                                                       listenInfo.ip.c_str(),
                                                                       !listenInfo.announcedAddress.empty() ? listenInfo.announcedAddress.c_str() : listenInfo.announcedIp.c_str(),
                                                                       listenInfo.port,
                                                                       listnInfoPortRange,
                                                                       listenInfoSocketFlags,
                                                                       listenInfo.sendBufferSize,
                                                                       listenInfo.recvBufferSize
                                                                       );
        
        auto rtcpListnInfoPortRange = FBS::Transport::CreatePortRange(builder, rtcpListenInfo.portRange.min, rtcpListenInfo.portRange.max);
        
        auto rtcpListenInfoSocketFlags = FBS::Transport::CreateSocketFlags(builder, rtcpListenInfo.flags.ipv6Only, rtcpListenInfo.flags.udpReusePort);
        
        auto rtcpListenInfoOffset = FBS::Transport::CreateListenInfoDirect(builder,
                                                                           rtcpListenInfo.protocol == "udp" ? FBS::Transport::Protocol::UDP : FBS::Transport::Protocol::TCP,
                                                                           rtcpListenInfo.ip.c_str(),
                                                                           !rtcpListenInfo.announcedAddress.empty() ? rtcpListenInfo.announcedAddress.c_str() : rtcpListenInfo.announcedIp.c_str(),
                                                                           rtcpListenInfo.port,
                                                                           rtcpListnInfoPortRange,
                                                                           rtcpListenInfoSocketFlags,
                                                                           rtcpListenInfo.sendBufferSize,
                                                                           rtcpListenInfo.recvBufferSize
                                                                           );
        
        auto plainTransportOptionsOffset = FBS::PlainTransport::CreatePlainTransportOptions(builder,
                                                                                            baseTransportOptionsOffset,
                                                                                            listenInfoOffset,
                                                                                            rtcpListenInfoOffset,
                                                                                            rtcpMux,
                                                                                            comedia,
                                                                                            enableSrtp,
                                                                                            cryptoSuiteToFbs(srtpCryptoSuite)
                                                                                            );

        auto reqOffset = FBS::Router::CreateCreatePlainTransportRequestDirect(builder, internal.transportId.c_str(), plainTransportOptionsOffset);
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.routerId,
                                                     FBS::Request::Method::ROUTER_CREATE_PLAINTRANSPORT,
                                                     FBS::Request::Body::Router_CreatePlainTransportRequest,
                                                     reqOffset);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto dumpResponse = response->body_as_PlainTransport_DumpResponse();
        
        auto dump = parsePlainTransportDumpResponse(dumpResponse);
        
        auto plainTransportData = std::make_shared<PlainTransportData>();
        plainTransportData->rtcpMux = dump->rtcpMux;
        plainTransportData->comedia = dump->comedia;
        plainTransportData->tuple = dump->tuple;
        plainTransportData->rtcpTuple = dump->rtcpTuple;
        plainTransportData->sctpParameters = dump->sctpParameters;
        plainTransportData->sctpState = dump->sctpState;
        plainTransportData->srtpParameters = dump->srtpParameters;
        
        auto ptcOptions = std::make_shared<PlainTransportConstructorOptions>();
        ptcOptions->internal = internal;
        ptcOptions->data = plainTransportData;
        ptcOptions->channel = _channel.lock();
        ptcOptions->appData = appData;
        ptcOptions->getRouterRtpCapabilities = _getRouterRtpCapabilities;
        ptcOptions->getProducerController = _getProducerController;
        ptcOptions->getDataProducerController = _getDataProducerController;
        
            transportController = std::make_shared<PlainTransportController>(ptcOptions);
            transportController->init();
            _transportControllers.emplace(std::make_pair(internal.transportId, transportController));
        
        connectSignals(transportController);
        
        this->newTransportSignal(transportController);

        return transportController;
    }

    std::shared_ptr<ITransportController> RouterController::createDirectTransportController(const std::shared_ptr<DirectTransportOptions>& options)
    {
        SRV_LOGD("createDirectTransportController()");
        
        std::shared_ptr<DirectTransportController> transportController;
        
        if (!options) {
            SRV_LOGE("options must be a valid pointer");
            return transportController;
        }
        
        auto channel = _channel.lock();
        if (!channel) {
            SRV_LOGE("channel must be a valid pointer");
            return transportController;
        }
        
        const int32_t maxMessageSize = options->maxMessageSize;
        const nlohmann::json& appData = options->appData;

        if (maxMessageSize < 0) {
            SRV_LOGE("if given, maxMessageSize must be a positive number");
            return transportController;
        }
        
        TransportInternal internal;
        internal.routerId = _internal.routerId;
        internal.transportId = uuid::uuidv4();
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto baseTransportOptionsOffset = FBS::Transport::CreateOptions(builder, true, maxMessageSize);
        
        auto directTransportOptionsOffset = FBS::DirectTransport::CreateDirectTransportOptions(builder, baseTransportOptionsOffset);
        
        auto reqOffset = FBS::Router::CreateCreateDirectTransportRequestDirect(builder, internal.transportId.c_str(), directTransportOptionsOffset);
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.routerId,
                                                     FBS::Request::Method::ROUTER_CREATE_DIRECTTRANSPORT,
                                                     FBS::Request::Body::Router_CreateDirectTransportRequest,
                                                     reqOffset);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto dumpResponse = response->body_as_DirectTransport_DumpResponse();
        
        auto dump = parseDirectTransportDumpResponse(dumpResponse);
            
        auto directTransportData = std::make_shared<DirectTransportData>();
        directTransportData->sctpParameters = dump->sctpParameters;
        
        auto dtcOptions = std::make_shared<DirectTransportConstructorOptions>();
        dtcOptions->internal = internal;
        dtcOptions->data = directTransportData;
        dtcOptions->channel = _channel.lock();
        dtcOptions->appData = appData;
        dtcOptions->getRouterRtpCapabilities = _getRouterRtpCapabilities;
        dtcOptions->getProducerController = _getProducerController;
        dtcOptions->getDataProducerController = _getDataProducerController;
        
        transportController = std::make_shared<DirectTransportController>(dtcOptions);
        transportController->init();
        _transportControllers.emplace(std::make_pair(internal.transportId, transportController));
        
        connectSignals(transportController);
        
        this->newTransportSignal(transportController);

        return transportController;
    }

    std::shared_ptr<ITransportController> RouterController::createPipeTransportController(const std::shared_ptr<PipeTransportOptions>& options)
    {
        SRV_LOGD("createPipeTransportController()");
        
        std::shared_ptr<PipeTransportController> transportController;
        
        if (!options) {
            SRV_LOGE("options must be a valid pointer");
            return transportController;
        }
        
        auto channel = _channel.lock();
        if (!channel) {
            SRV_LOGE("channel must be a valid pointer");
            return transportController;
        }
        
        auto listenInfo = options->listenInfo;
        //const auto& listenIp = options->listenIp;
        //const uint16_t port = options->port;
        const bool enableSctp = options->enableSctp;
        const NumSctpStreams& numSctpStreams = options->numSctpStreams;
        const int32_t maxSctpMessageSize = options->maxSctpMessageSize;
        const int32_t sctpSendBufferSize = options->sctpSendBufferSize;
        const bool enableRtx = options->enableRtx;
        const bool enableSrtp = options->enableSrtp;
        const nlohmann::json& appData = options->appData;
        
        //if (listenInfo.ip.empty() && listenIp.ip.empty()) {
        if (listenInfo.ip.empty()) {
            SRV_LOGE("missing listenInfo and listenIp (one of them is mandatory)");
            return nullptr;
        }
        //else if (!listenInfo.ip.empty() && !listenIp.ip.empty()) {
        //    SRV_LOGE("only one of listenInfo and listenIp must be given");
        //    return nullptr;
        //}
        
        //// Convert deprecated TransportListenIps to TransportListenInfos.
        //if (!listenIp.ip.empty()) {
        //    // Normalize IP string to TransportListenInfo object.
        //    listenInfo.protocol = "udp";
        //    listenInfo.ip = listenIp.ip;
        //    listenInfo.announcedAddress = listenIp.announcedIp;
        //    listenInfo.port = port;
        //}
        
        TransportInternal internal;
        internal.routerId = _internal.routerId;
        internal.transportId = uuid::uuidv4();
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto numSctpStreamsOffset = FBS::SctpParameters::CreateNumSctpStreams(builder, numSctpStreams.OS, numSctpStreams.MIS);
        bool isDataChannel = false;
        auto baseTransportOptionsOffset = FBS::Transport::CreateOptions(builder,
                                                                        false,
                                                                        flatbuffers::nullopt,
                                                                        flatbuffers::nullopt,
                                                                        enableSctp,
                                                                        numSctpStreamsOffset,
                                                                        maxSctpMessageSize,
                                                                        sctpSendBufferSize,
                                                                        isDataChannel
                                                                        );
        
        auto portRange = FBS::Transport::CreatePortRange(builder, listenInfo.portRange.min, listenInfo.portRange.max);
        
        auto socketFlags = FBS::Transport::CreateSocketFlags(builder, listenInfo.flags.ipv6Only, listenInfo.flags.udpReusePort);
        
        auto listenInfoOffset = FBS::Transport::CreateListenInfoDirect(builder,
                                                                       listenInfo.protocol == "udp" ? FBS::Transport::Protocol::UDP : FBS::Transport::Protocol::TCP,
                                                                       listenInfo.ip.c_str(),
                                                                       !listenInfo.announcedAddress.empty() ? listenInfo.announcedAddress.c_str() : listenInfo.announcedIp.c_str(),
                                                                       listenInfo.port,
                                                                       portRange,
                                                                       socketFlags,
                                                                       listenInfo.sendBufferSize,
                                                                       listenInfo.recvBufferSize
                                                                       );
        
        auto pipeTransportOptionsOffset = FBS::PipeTransport::CreatePipeTransportOptions(builder,
                                                                                         baseTransportOptionsOffset,
                                                                                         listenInfoOffset,
                                                                                         enableRtx,
                                                                                         enableSrtp
                                                                                         );
        
        auto reqOffset = FBS::Router::CreateCreatePipeTransportRequestDirect(builder, internal.transportId.c_str(), pipeTransportOptionsOffset);
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.routerId,
                                                     FBS::Request::Method::ROUTER_CREATE_PIPETRANSPORT,
                                                     FBS::Request::Body::Router_CreatePipeTransportRequest,
                                                     reqOffset);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto dumpResponse = response->body_as_PipeTransport_DumpResponse();
        
        auto dump = parsePipeTransportDumpResponse(dumpResponse);
        
        auto pipeTransportData = std::make_shared<PipeTransportData>();
        pipeTransportData->tuple = dump->tuple;
        pipeTransportData->sctpParameters = dump->sctpParameters;
        pipeTransportData->sctpState = dump->sctpState;
        pipeTransportData->srtpParameters = dump->srtpParameters;
        pipeTransportData->rtx = dump->rtx;
        
        
        auto ptcOptions = std::make_shared<PipeTransportConstructorOptions>();
        ptcOptions->internal = internal;
        ptcOptions->data = pipeTransportData;
        ptcOptions->channel = _channel.lock();
        ptcOptions->appData = appData;
        ptcOptions->getRouterRtpCapabilities = _getRouterRtpCapabilities;
        ptcOptions->getProducerController = _getProducerController;
        ptcOptions->getDataProducerController = _getDataProducerController;
        
        transportController = std::make_shared<PipeTransportController>(ptcOptions);
        transportController->init();
        _transportControllers.emplace(std::make_pair(internal.transportId, transportController));
        
        connectSignals(transportController);
        
        this->newTransportSignal(transportController);
        
        return transportController;
    }

    std::shared_ptr<IRtpObserverController> RouterController::createActiveSpeakerObserverController(const std::shared_ptr<ActiveSpeakerObserverOptions>& options)
    {
        SRV_LOGD("createActiveSpeakerObserverController()");
        
        std::shared_ptr<ActiveSpeakerObserverController> rtpObserverController;
        
        if (!options) {
            SRV_LOGE("options must be a valid pointer");
            return rtpObserverController;
        }
        
        auto channel = _channel.lock();
        if (!channel) {
            SRV_LOGE("channel must be a valid pointer");
            return rtpObserverController;
        }
        
        const int interval = options->interval;
        const nlohmann::json& appData = options->appData;
        
        RtpObserverObserverInternal internal;
        internal.routerId = _internal.routerId;
        internal.rtpObserverId = uuid::uuidv4();
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto activeRtpObserverOptionsOffset = FBS::ActiveSpeakerObserver::CreateActiveSpeakerObserverOptions(builder, interval);
        
        auto reqOffset = FBS::Router::CreateCreateActiveSpeakerObserverRequestDirect(builder, internal.rtpObserverId.c_str(), activeRtpObserverOptionsOffset);
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.routerId,
                                                     FBS::Request::Method::ROUTER_CREATE_ACTIVESPEAKEROBSERVER,
                                                     FBS::Request::Body::Router_CreateActiveSpeakerObserverRequest,
                                                     reqOffset);
                    
        channel->request(reqId, reqData);
        
        auto roocOptions = std::make_shared<RtpObserverObserverConstructorOptions>();
        roocOptions->internal = internal;
        roocOptions->channel = _channel.lock();
        roocOptions->appData = appData;
        roocOptions->getProducerController = _getProducerController;
        
        rtpObserverController = std::make_shared<ActiveSpeakerObserverController>(roocOptions);
        rtpObserverController->init();
        
        _rtpObserverControllers.emplace(std::make_pair(internal.rtpObserverId, rtpObserverController));
        
        rtpObserverController->closeSignal.connect([id = rtpObserverController->id(), wself = std::weak_ptr<RouterController>(shared_from_this())]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (self->_rtpObserverControllers.contains(id)) {
                self->_rtpObserverControllers.erase(id);
            }
        });
        
        this->newRtpObserverSignal(rtpObserverController);
        
        return rtpObserverController;
    }

    std::shared_ptr<IRtpObserverController> RouterController::createAudioLevelObserverController(const std::shared_ptr<AudioLevelObserverOptions>& options)
    {
        SRV_LOGD("createAudioLevelObserverController()");
        
        std::shared_ptr<AudioLevelObserverController> rtpObserverController;
        
        if (!options) {
            SRV_LOGE("options must be a valid pointer");
            return rtpObserverController;
        }
        
        auto channel = _channel.lock();
        if (!channel) {
            SRV_LOGE("channel must be a valid pointer");
            return rtpObserverController;
        }
        
        const int maxEntries = options->maxEntries;
        const int threshold = options->threshold;
        const int interval = options->interval;
        const nlohmann::json& appData = options->appData;
        
        if (maxEntries <= 0) {
            SRV_LOGE("if given, maxEntries must be a positive number");
            return rtpObserverController;
        }
        
        if (threshold < -127 || threshold > 0) {
            SRV_LOGE("if given, threshole must be a negative number greater than -127");
            return rtpObserverController;
        }
        
        RtpObserverObserverInternal internal;
        internal.routerId = _internal.routerId;
        internal.rtpObserverId = uuid::uuidv4();

        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto audioLevelObserverOptionsOffset = FBS::AudioLevelObserver::CreateAudioLevelObserverOptions(builder, maxEntries, threshold, interval);
        
        auto reqOffset = FBS::Router::CreateCreateAudioLevelObserverRequestDirect(builder, internal.rtpObserverId.c_str(), audioLevelObserverOptionsOffset);
    
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.routerId,
                                                     FBS::Request::Method::ROUTER_CREATE_AUDIOLEVELOBSERVER,
                                                     FBS::Request::Body::Router_CreateAudioLevelObserverRequest,
                                                     reqOffset);
        
        channel->request(reqId, reqData);
        
        auto alocOptions = std::make_shared<AudioLevelObserverConstructorOptions>();
        alocOptions->internal = internal;
        alocOptions->channel = _channel.lock();
        alocOptions->appData = appData;
        alocOptions->getProducerController = _getProducerController;
        
        rtpObserverController = std::make_shared<AudioLevelObserverController>(alocOptions);
        rtpObserverController->init();
        _rtpObserverControllers.emplace(std::make_pair(internal.rtpObserverId, rtpObserverController));
        
        rtpObserverController->closeSignal.connect([id = rtpObserverController->id(), wself = std::weak_ptr<RouterController>(shared_from_this())]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (self->_rtpObserverControllers.contains(id)) {
                self->_rtpObserverControllers.erase(id);
            }
        });
        
        this->newRtpObserverSignal(rtpObserverController);
        
        return rtpObserverController;
    }

    bool RouterController::canConsume(const std::string& producerId, const RtpCapabilities& rtpCapabilities)
    {
        SRV_LOGD("canConsume()");
        
        if (!_producerControllers.contains(producerId)) {
            return false;
        }
        
        auto producerController = _producerControllers[producerId];
        
        if (!producerController) {
            SRV_LOGE("canConsume() | Producer with id '%s' not found", producerId.c_str());
            return false;
        }

        try {
            return ortc::canConsume(producerController->consumableRtpParameters(), rtpCapabilities);
        }
        catch (...) {
            SRV_LOGE("canConsume() | unexpected error");
            return false;
        }
    }

    void RouterController::connectSignals(const std::shared_ptr<ITransportController>& transportController)
    {
        transportController->closeSignal.connect([wself = std::weak_ptr<RouterController>(shared_from_this())](const std::string& transportId) {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            
            if (self->_transportControllers.contains(transportId)) {
                self->_transportControllers.erase(transportId);
            }
        });
        
        transportController->webRtcServerCloseSignal.connect([id = transportController->id(), wself = std::weak_ptr<RouterController>(shared_from_this())]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            
            if (self->_transportControllers.contains(id)) {
                self->_transportControllers.erase(id);
            }
        });
        
        transportController->newProducerSignal.connect([wself = std::weak_ptr<RouterController>(shared_from_this())](std::shared_ptr<IProducerController> producerController) {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            
            if (!producerController) {
                return;
            }
            
            if (!self->_producerControllers.contains(producerController->id())) {
                self->_producerControllers.emplace(std::make_pair(producerController->id(), producerController));
            }
        });
        
        transportController->producerCloseSignal.connect([wself = std::weak_ptr<RouterController>(shared_from_this())](std::shared_ptr<IProducerController> producerController) {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (!producerController) {
                return;
            }
            if (self->_producerControllers.contains(producerController->id())) {
                self->_producerControllers.erase(producerController->id());
            }
        });
        
        transportController->newDataProducerSignal.connect([wself = std::weak_ptr<RouterController>(shared_from_this())](std::shared_ptr<IDataProducerController> dataProducerController) {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (!dataProducerController) {
                return;
            }
            if (!self->_dataProducerControllers.contains(dataProducerController->id())) {
                self->_dataProducerControllers.emplace(std::make_pair(dataProducerController->id(), dataProducerController));
            }
        });
        
        transportController->dataProducerCloseSignal.connect([wself = std::weak_ptr<RouterController>(shared_from_this())](std::shared_ptr<IDataProducerController> dataProducerController) {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (!dataProducerController) {
                return;
            }
            if (self->_dataProducerControllers.contains(dataProducerController->id())) {
                self->_dataProducerControllers.erase(dataProducerController->id());;
            }
        });
    }

    std::shared_ptr<PipeToRouterResult> RouterController::pipeToRouter(const std::shared_ptr<PipeToRouterOptions>& options)
    {
        SRV_LOGD("pipeToRouter()");
        
        std::shared_ptr<PipeToRouterResult> result;
        
        if (!options) {
            SRV_LOGE("options must be a valid pointer");
            return result;
        }
        
        auto listenInfo = options->listenInfo;
        //const auto& listenIp = options->listenIp;
        const auto port = options->port;
        const auto& producerId = options->producerId;
        const auto& dataProducerId = options->dataProducerId;
        std::shared_ptr<IRouterController> routerController = options->routerController;
        bool enableSctp = options->enableSctp;
        const auto& numSctpStreams = options->numSctpStreams;
        bool enableRtx = options->enableRtx;
        bool enableSrtp = options->enableSctp;
        
        //if (listenInfo.ip.empty() && listenIp.ip.empty()) {
        if (listenInfo.ip.empty()) {
            listenInfo.protocol = "udp";
            listenInfo.ip = "127.0.0.1";
        }

        //if (!listenInfo.ip.empty() && !listenIp.ip.empty()) {
        if (listenInfo.ip.empty()) {
            SRV_LOGE("only one of listenInfo and listenIp must be given");
            return result;
        }
        
        if (producerId.empty() && dataProducerId.empty()) {
            SRV_LOGE("missing producerId or dataProducerId");
            return result;
        }
        else if (!producerId.empty() && !dataProducerId.empty()) {
            SRV_LOGE("just producerId or dataProducerId can be given");
            return result;
        }
        else if (!routerController) {
            SRV_LOGE("Router not found");
            return result;
        }
        else if (routerController.get() == this) {
            SRV_LOGE("cannot use this Router as destination");
            return result;
        }
        
        //// Convert deprecated TransportListenIps to TransportListenInfos.
        //if (!listenIp.ip.empty()) {
        //    // Normalize IP string to TransportListenIp object.
        //    listenInfo.protocol = "udp";
        //    listenInfo.ip = listenIp.ip;
        //    listenInfo.announcedAddress = listenIp.announcedIp;
        //}
        
        std::shared_ptr<IProducerController> producerController;
        std::shared_ptr<IDataProducerController> dataProducerController;

        if (!producerId.empty()) {
            if (!this->_producerControllers.contains(producerId)) {
                SRV_LOGE("Producer not found");
                return result;
            }
            producerController = _producerControllers[producerId];
        }
        else if (!dataProducerId.empty()) {
            if (!this->_dataProducerControllers.contains(dataProducerId)) {
                SRV_LOGE("Data producer not found");
                return result;
            }
            dataProducerController = _dataProducerControllers[dataProducerId];
        }

        auto pipeTransportPairKey = routerController->id();
        std::shared_ptr<PipeTransportController> localPipeTransportController;
        std::shared_ptr<PipeTransportController> remotePipeTransportController;
        PipeTransportControllerPair pipeTransportControllerPair;
        
        if (!_routerPipeTransportPairMap.contains(pipeTransportPairKey)) {
            SRV_LOGE("given key already exists in this Router");
            return result;
        }
        else {
            auto ptOptions = std::make_shared<PipeTransportOptions>();
            ptOptions->listenInfo = listenInfo;
            //ptOptions->listenIp = listenIp;
            ptOptions->port = port;
            ptOptions->enableSctp = enableSctp;
            ptOptions->numSctpStreams = numSctpStreams;
            ptOptions->enableRtx = enableRtx;
            ptOptions->enableSrtp = enableSrtp;
            
            localPipeTransportController = std::dynamic_pointer_cast<PipeTransportController>(this->createPipeTransportController(ptOptions));
            pipeTransportControllerPair[this->id()] = localPipeTransportController;
            
            remotePipeTransportController = std::dynamic_pointer_cast<PipeTransportController>(routerController->createPipeTransportController(ptOptions));
            pipeTransportControllerPair[routerController->id()] = remotePipeTransportController;
            
            localPipeTransportController->closeSignal.connect([wself = std::weak_ptr<RouterController>(shared_from_this()), pipeTransportPairKey, weakRemote = std::weak_ptr<PipeTransportController>(remotePipeTransportController)](const std::string& routerId){
                auto self = wself.lock();
                if (!self) {
                    return;
                }
                if (auto remote = weakRemote.lock()) {
                    remote->close();
                }
                if (self->_routerPipeTransportPairMap.contains(pipeTransportPairKey)) {
                    self->_routerPipeTransportPairMap.erase(pipeTransportPairKey);
                }
            });
            
            localPipeTransportController->closeSignal.connect([wself = std::weak_ptr<RouterController>(shared_from_this()), pipeTransportPairKey, weakLocal = std::weak_ptr<PipeTransportController>(remotePipeTransportController)](const std::string& routerId){
                auto self = wself.lock();
                if (!self) {
                    return;
                }
                if (auto local = weakLocal.lock()) {
                    local->close();
                }
                if (self->_routerPipeTransportPairMap.contains(pipeTransportPairKey)) {
                    self->_routerPipeTransportPairMap.erase(pipeTransportPairKey);
                }
            });
            
            auto rData = std::make_shared<ConnectParams>();
            rData->ip = remotePipeTransportController->tuple().localAddress;
            rData->port = remotePipeTransportController->tuple().localPort;
            rData->srtpParameters = remotePipeTransportController->srtpParameters();
            localPipeTransportController->connect(rData);
            
            auto lData = std::make_shared<ConnectParams>();
            lData->ip = localPipeTransportController->tuple().localAddress;
            lData->port = localPipeTransportController->tuple().localPort;
            lData->srtpParameters = localPipeTransportController->srtpParameters();
            remotePipeTransportController->connect(lData);
            
            this->_routerPipeTransportPairMap.emplace(std::make_pair(pipeTransportPairKey, pipeTransportControllerPair));
            
            routerController->addPipeTransportPair(this->id(), pipeTransportControllerPair);
        }
        
        if (producerController) {
            std::shared_ptr<IConsumerController> pipeConsumerController;
            std::shared_ptr<IProducerController> pipeProducerController;
            
            try {
                auto cOptions = std::make_shared<ConsumerOptions>();
                cOptions->producerId = producerId;
                pipeConsumerController = localPipeTransportController->consume(cOptions);
                
                auto pOptions = std::make_shared<ProducerOptions>();
                pOptions->id = producerController->id();
                pOptions->kind = pipeConsumerController->kind();
                pOptions->rtpParameters = pipeConsumerController->rtpParameters();
                pOptions->paused = pipeConsumerController->producerPaused();
                pOptions->appData = producerController->appData();
                pipeProducerController = remotePipeTransportController->produce(pOptions);
                
                if (producerController->closed()) {
                    SRV_LOGE("original Producer closed");
                    return result;
                }
                
                // Ensure that producer.paused has not changed in the meanwhile and, if so, sync the pipeProducer.
                if (pipeProducerController->paused() != producerController->paused()) {
                    if (producerController->paused()) {
                        pipeProducerController->pause();
                    }
                    else {
                        pipeProducerController->resume();
                    }
                }
                
                // Pipe events from the pipe Consumer to the pipe Producer.
                pipeConsumerController->closeSignal.connect([weakPipeProducerController = std::weak_ptr<IProducerController>(pipeProducerController)](){
                    if (auto pipeProducerController = weakPipeProducerController.lock()) {
                        pipeProducerController->close();
                    }
                });
                
                pipeConsumerController->pauseSignal.connect([weakPipeProducerController = std::weak_ptr<IProducerController>(pipeProducerController)](){
                    if (auto pipeProducerController = weakPipeProducerController.lock()) {
                        pipeProducerController->pause();
                    }
                });
                
                pipeConsumerController->resumeSignal.connect([weakPipeProducerController = std::weak_ptr<IProducerController>(pipeProducerController)](){
                    if (auto pipeProducerController = weakPipeProducerController.lock()) {
                        pipeProducerController->resume();
                    }
                });
                
                // Pipe events from the pipe Producer to the pipe Consumer.
                pipeProducerController->closeSignal.connect([weakPipeConsumerController = std::weak_ptr<IConsumerController>(pipeConsumerController)](){
                    if (auto pipeConsumerController = weakPipeConsumerController.lock()) {
                        pipeConsumerController->close();
                    }
                });
                
                result = std::make_shared<PipeToRouterResult>();
                result->pipeConsumerController = pipeConsumerController;
                result->pipeProducerController = pipeProducerController;
                
                return result;
            }
            catch (const char* what) {
                SRV_LOGE("pipeToRouter() | error creating pipe Consumer/Producer pair:%s", what);
                if (pipeConsumerController) {
                    pipeConsumerController->close();
                }

                if (pipeProducerController) {
                    pipeProducerController->close();
                }
            }
        }
        else if (dataProducerController) {
            std::shared_ptr<IDataConsumerController> pipeDataConsumerController;
            std::shared_ptr<IDataProducerController> pipeDataProducerController;
            try {
                auto cOptions = std::make_shared<DataConsumerOptions>();
                cOptions->dataProducerId = dataProducerId;
                pipeDataConsumerController = localPipeTransportController->consumeData(cOptions);
                
                auto pOptions = std::make_shared<DataProducerOptions>();
                pOptions->id = dataProducerController->id();
                pOptions->sctpStreamParameters = pipeDataConsumerController->sctpStreamParameters();
                pOptions->label = pipeDataConsumerController->label();
                pOptions->protocol = pipeDataConsumerController->protocol();
                pOptions->appData = dataProducerController->appData();
                
                if (dataProducerController->closed()) {
                    SRV_LOGE("original data producer closed");
                    return result;
                }
                
                // Pipe events from the pipe DataConsumer to the pipe DataProducer.
                pipeDataConsumerController->closeSignal.connect([weakPipeDataProducerController = std::weak_ptr<IDataProducerController>(pipeDataProducerController)](){
                    if (auto pipeDataProducerController = weakPipeDataProducerController.lock()) {
                        pipeDataProducerController->close();
                    }
                });
                
                // Pipe events from the pipe DataProducer to the pipe DataConsumer.
                pipeDataProducerController->closeSignal.connect([weakPipeDataConsumerController = std::weak_ptr<IDataConsumerController>(pipeDataConsumerController)](){
                    if (auto pipeDataConsumerController = weakPipeDataConsumerController.lock()) {
                        pipeDataConsumerController->close();
                    }
                });
                
                result = std::make_shared<PipeToRouterResult>();
                result->pipeDataConsumerController = pipeDataConsumerController;
                result->pipeDataProducerController = pipeDataProducerController;
                
                return result;
            }
            catch (const char* what) {
                SRV_LOGE("pipeToRouter() | error creating pipe DataConsumer/DataProducer pair:%s", what);
                if (pipeDataConsumerController) {
                    pipeDataConsumerController->close();
                }

                if (pipeDataProducerController) {
                    pipeDataProducerController->close();
                }
            }
        }
        
        return result;
    }

    // key: router.id
    void RouterController::addPipeTransportPair(const std::string& key, PipeTransportControllerPair& pair)
    {
        if (_routerPipeTransportPairMap.contains(key)) {
            SRV_LOGE("given key already exists in this Router");
            return;
        }
        
        _routerPipeTransportPairMap.emplace(std::make_pair(key, pair));
        
        auto localPipeTransportController = pair[_internal.routerId];
        
        localPipeTransportController->closeSignal.connect([wself = std::weak_ptr<RouterController>(shared_from_this()), key](const std::string& routerId){
            auto self = wself.lock();
            if (!self) {
                return;
            }
            self->_routerPipeTransportPairMap.erase(key);
        });
    }

}

namespace srv
{
    std::shared_ptr<RouterDump> parseRouterDumpResponse(const FBS::Router::DumpResponse* binary)
    {
        auto dump = std::make_shared<RouterDump>();

        dump->id = binary->id()->str();
        
        for (const auto& item : *binary->transportIds()) {
            dump->transportIds.emplace_back(item->str());
        }
        
        for (const auto& item : *binary->rtpObserverIds()) {
            dump->rtpObserverIds.emplace_back(item->str());
        }
        
        for (const auto& item : *binary->mapProducerIdConsumerIds()) {
            std::vector<std::string> consumerIds;
            for (const auto& id : *item->values()) {
                consumerIds.emplace_back(id->str());
            }
            dump->mapProducerIdConsumerIds.emplace_back(std::pair<std::string, std::vector<std::string>>(item->key()->str(), consumerIds));
        }
        
        for (const auto& item : *binary->mapConsumerIdProducerId()) {
            dump->mapConsumerIdProducerId.emplace_back(std::pair<std::string, std::string>(item->key()->str(), item->value()->str()));
        }
        
        for (const auto& item : *binary->mapProducerIdObserverIds()) {
            std::vector<std::string> observerIds;
            for (const auto& id : *item->values()) {
                observerIds.emplace_back(id->str());
            }
            dump->mapProducerIdObserverIds.emplace_back(std::pair<std::string, std::vector<std::string>>(item->key()->str(), observerIds));
        }
        
        for (const auto& item : *binary->mapDataProducerIdDataConsumerIds()) {
            std::vector<std::string> dataConsumerIds;
            for (const auto& id : *item->values()) {
                dataConsumerIds.emplace_back(id->str());
            }
            dump->mapDataProducerIdDataConsumerIds.emplace_back(std::pair<std::string, std::vector<std::string>>(item->key()->str(), dataConsumerIds));
        }
        
        for (const auto& item : *binary->mapDataConsumerIdDataProducerId()) {
            dump->mapDataConsumerIdDataProducerId.emplace_back(std::pair<std::string, std::string>(item->key()->str(), item->value()->str()));
        }
        
        return dump;
    }

    void to_json(nlohmann::json& j, const RouterOptions& st)
    {
        j["mediaCodecs"] = st.mediaCodecs;
        j["appData"] = st.appData;
    }

    void from_json(const nlohmann::json& j, RouterOptions& st)
    {
        if (j.contains("mediaCodecs")) {
            j.at("mediaCodecs").get_to(st.mediaCodecs);
        }
        if (j.contains("appData")) {
            j.at("appData").get_to(st.appData);
        }
    }
}
