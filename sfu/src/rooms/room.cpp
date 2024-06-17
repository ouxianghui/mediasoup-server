/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: RTC-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#include "room.hpp"
#include "config.h"
#include "engine.h"
#include "srv_logger.h"
#include "webrtc_transport_controller.h"
#include "plain_transport_controller.h"
#include "direct_transport_controller.h"
#include "pipe_transport_controller.h"
#include "audio_level_observer_controller.h"
#include "active_speaker_observer_controller.h"
#include "interface/i_transport_controller.h"
#include "interface/i_consumer_controller.h"
#include "interface/i_producer_controller.h"
#include "interface/i_data_consumer_controller.h"
#include "interface/i_data_producer_controller.h"
#include "interface/i_rtp_observer_controller.h"
#include "video_sharing_controller.hpp"
#include "video_producer_quality_controller.hpp"

std::shared_ptr<Room> Room::create(const std::string& roomId, int32_t consumerReplicas)
{
    std::shared_ptr<Room> room;
    
    auto params = MSConfig->params();
    if (!params) {
        SRV_LOGE("Config must not be empty");
        return room;
    }
    
    auto mediaCodecs = params->mediasoup.routerOptions.mediaCodecs;
    
    auto workerController = MSEngine->getWorkerController();
    if (!workerController) {
        SRV_LOGE("Worker controller must not be null");
        return room;
    }
    
    auto webRtcServerController = workerController->webRtcServerController();
    if (!webRtcServerController) {
        SRV_LOGE("WebRtc Server controller must not be null");
        return room;
    }
    
    nlohmann::json appData;
    auto routerController = workerController->createRouterController(mediaCodecs, appData);
    if (!routerController) {
        SRV_LOGE("Router controller must not be null");
        return room;
    }
    
    auto aloOptions = std::make_shared<srv::AudioLevelObserverOptions>();
    aloOptions->maxEntries = 1;
    aloOptions->threshold = -80;
    aloOptions->interval = 800;
    auto audioLevelObserverController = routerController->createAudioLevelObserverController(aloOptions);

    auto asoOptions = std::make_shared<srv::ActiveSpeakerObserverOptions>();
    asoOptions->interval = 300;
    auto activeSpeakerObserverController = routerController->createActiveSpeakerObserverController(asoOptions);
    
    room = std::make_shared<Room>(roomId,
                                 webRtcServerController,
                                 routerController,
                                 audioLevelObserverController,
                                 activeSpeakerObserverController,
                                 consumerReplicas);
    
    return room;
}

Room::Room(const std::string& roomId,
           const std::shared_ptr<srv::IWebRtcServerController>& webRtcServerController,
           const std::shared_ptr<srv::IRouterController>& routerController,
           const std::shared_ptr<srv::IRtpObserverController>& audioLevelObserverController,
           const std::shared_ptr<srv::IRtpObserverController>& activeSpeakerObserverController,
           int32_t consumerReplicas)
: _id(roomId)
, _webRtcServerController(webRtcServerController)
, _routerController(routerController)
, _audioLevelObserverController(audioLevelObserverController)
, _activeSpeakerObserverController(activeSpeakerObserverController)
, _videoSharingController(std::make_shared<VideoSharingController>())
{
    SRV_LOGD("Room()");
    
    ++_statistics->EVENT_ROOM_CREATED;
}

Room::~Room()
{
    SRV_LOGD("~Room()");
    
    ++_statistics->EVENT_ROOM_DELETED;
}

void Room::init()
{
    SRV_LOGD("init()");
    
    handleAudioLevelObserver();
    handleActiveSpeakerObserver();
}

void Room::destroy()
{
    SRV_LOGD("destroy()");
}

void Room::createPeer(const std::shared_ptr<oatpp::websocket::AsyncWebSocket>& socket, const std::string& roomId, const std::string& peerId)
{
    SRV_LOGD("createPeer()");
    
    if (_peerMap.contains(peerId)) {
        SRV_LOGE("there is already a Peer with same peerId [peerId:'%s']", peerId.c_str());
        return;
    }

    auto peer = std::make_shared<Peer>(socket, roomId, peerId);
    peer->init();

    socket->setListener(peer);

    peer->requestSignal.connect(&Room::onHandleRequest, shared_from_this());
    peer->notificationSignal.connect(&Room::onHandleNotification, shared_from_this());
    peer->newConsumerResumedSignal.connect(&Room::onNewConsumerResumed, shared_from_this());
    peer->closeSignal.connect(&Room::onPeerClose, shared_from_this());
    
    _peerMap.emplace(std::make_pair(peer->id(), peer));
    
    nlohmann::json msg;
    msg["mediasoup-version"] = "3.14.7";
    peer->notify("mediasoup-version", msg);
}

std::shared_ptr<Peer> Room::getPeer(const std::string& peerId)
{
    SRV_LOGD("getPeer()");

    if (_peerMap.contains(peerId)) {
        return _peerMap[peerId];
    }
    return nullptr;
}

void Room::removePeer(const std::string& peerId)
{
    SRV_LOGD("removePeer()");

    if (_peerMap.contains(peerId)) {
        auto peer = _peerMap[peerId];
        peer->requestSignal.disconnect_all();
        peer->notificationSignal.disconnect_all();
        peer->closeSignal.disconnect_all();
        _peerMap.erase(peerId);
    }
    
    if (_peerMap.empty()) {
        SRV_LOGI("last Peer in the room left, closing the room [roomId: %s]", _id.c_str());
        close();
    }
}

void Room::pingAllPeers()
{
    _peerMap.for_each([this](auto& pair) {
        auto& peer = pair.second;
        if (!peer->sendPing()) {
            peer->invalidateSocket();
            ++this->_statistics->EVENT_PEER_ZOMBIE_DROPPED;
        }
    });
}

bool Room::isEmpty()
{
    return _peerMap.size() == 0;
}

void Room::close()
{
    SRV_LOGD("close()");

    _closed = true;

    auto audioLevelObserverController = std::dynamic_pointer_cast<srv::AudioLevelObserverController>(_audioLevelObserverController);
    audioLevelObserverController->volumesSignal.disconnect_all();
    audioLevelObserverController->silenceSignal.disconnect_all();
    
    auto activeSpeakerObserverController = std::dynamic_pointer_cast<srv::ActiveSpeakerObserverController>(_activeSpeakerObserverController);
    activeSpeakerObserverController->dominantSpeakerSignal.disconnect_all();
    
    // Close the mediasoup Router.
    _routerController->close();
    
    this->closeSignal(_id);
}

void Room::onPeerClose(const std::string& peerId)
{
    SRV_LOGD("onPeerClose()");
    
    if (_closed) {
        return;
    }

    SRV_LOGD("protoo Peer 'close' event [peerId: %s]", peerId.c_str());

    {
        nlohmann::json msg;
        msg["peerId"] = peerId;
        
        auto otherPeers = getJoinedPeers(peerId);
        
        std::threadsafe_unordered_map<std::string, std::shared_ptr<srv::ITransportController>> transportControllers;

        if (_peerMap.contains(peerId)) {
            auto peer = _peerMap[peerId];
            auto peerData = peer->data();
            if (peerData->joined) {
                peerData->consumerControllers.for_each([this](const auto& item) {
                   this->removeVideoProducerQuality(item.second);
                });
                
                for (const auto& otherPeer : otherPeers) {
                    otherPeer.second->notify("peerClosed", msg);
                }
            }
            transportControllers = peer->data()->transportControllers.value();
        }
            
        // Iterate and close all mediasoup Transport associated to this Peer, so all
        // its Producers and Consumers will also be closed.
        transportControllers.for_each([](const auto& item) {
            item.second->close();
        });
    }
}

void Room::handleAudioLevelObserver()
{
    auto audioLevelObserverController = std::dynamic_pointer_cast<srv::AudioLevelObserverController>(_audioLevelObserverController);
    audioLevelObserverController->volumesSignal.connect(&Room::onAudioVolumes, shared_from_this());
    audioLevelObserverController->silenceSignal.connect(&Room::onAudioSilence, shared_from_this());
}

void Room::handleActiveSpeakerObserver()
{
    auto activeSpeakerObserverController = std::dynamic_pointer_cast<srv::ActiveSpeakerObserverController>(_activeSpeakerObserverController);
    activeSpeakerObserverController->dominantSpeakerSignal.connect(&Room::onDominantSpeaker, shared_from_this());
}

std::unordered_map<std::string, std::shared_ptr<Peer>> Room::getJoinedPeers(const std::string& excludePeerId)
{
    std::unordered_map<std::string, std::shared_ptr<Peer>> peers;
    _peerMap.for_each([&peers, excludePeerId](const auto& item) {
        if (item.second->data()->joined && item.first != excludePeerId) {
            peers.emplace(item);
        }
    });
    return peers;
}

void Room::createConsumer(const std::shared_ptr<Peer>& consumerPeer, const std::shared_ptr<Peer>& producerPeer, const std::shared_ptr<srv::IProducerController>& producerController)
{
    // Optimization:
    // - Create the server-side Consumer in paused mode.
    // - Tell its Peer about it and wait for its response.
    // - Upon receipt of the response, resume the server-side Consumer.
    // - If video, this will mean a single key frame requested by the
    //   server-side Consumer (when resuming it).
    // - If audio (or video), it will avoid that RTP packets are received by the
    //   remote endpoint *before* the Consumer is locally created in the endpoint
    //   (and before the local SDP O/A procedure ends). If that happens (RTP
    //   packets are received before the SDP O/A is done) the PeerConnection may
    //   fail to associate the RTP stream.

    // NOTE: Don't create the Consumer if the remote Peer cannot consume it.
    srv::RtpCapabilities rtpCapabilities = consumerPeer->data()->rtpCapabilities;
    if (rtpCapabilities.codecs.empty() || !_routerController->canConsume(producerController->id(), rtpCapabilities)) {
        return;
    }

    // Must take the Transport the remote Peer is using for consuming.
    std::shared_ptr<srv::ITransportController> transportController;
    consumerPeer->data()->transportControllers.for_each([&transportController](auto &kv) {
        auto &t = kv.second;
        if(t->appData()["consuming"].template get<bool>() == true) {
            transportController = t;
        }
    });
    // This should not happen.
    if (!transportController) {
        SRV_LOGE("createConsumer() | Transport for consuming not found");
        return;
    }
    
    const int32_t consumerCount = 1 + _consumerReplicas;

    for (int32_t i = 0; i < consumerCount; ++i) {
        std::shared_ptr<srv::IConsumerController> consumerController;
        try {
            auto options = std::make_shared<srv::ConsumerOptions>();
            options->producerId = producerController->id();
            options->rtpCapabilities = consumerPeer->data()->rtpCapabilities;
            options->enableRtx = true;
            options->paused = true;
            consumerController = transportController->consume(options);
            consumerController->setAppData(producerController->appData());
            
        }
        catch (const char *error) {
            SRV_LOGE("createConsumer() | transport->consume(): %s", error);
            return;
        }
        
        consumerPeer->data()->consumerControllers.emplace(std::make_pair(consumerController->id(), consumerController));
        
        std::weak_ptr<srv::IConsumerController> wcc = consumerController;
        std::weak_ptr<Room> wself = shared_from_this();
        
        consumerController->transportCloseSignal.connect([id = consumerController->id(), wcp = std::weak_ptr<Peer>(consumerPeer)](){
            auto cp = wcp.lock();
            if (!cp) {
                return;
            }
            cp->data()->consumerControllers.erase(id);
        });
        
        consumerController->producerCloseSignal.connect([id = consumerController->id(), wcp = std::weak_ptr<Peer>(consumerPeer), wcc, wself](){
            auto cp = wcp.lock();
            if (!cp) {
                return;
            }
            
            nlohmann::json msg;
            msg["consumerId"] = id;
            
            if (cp->data()->consumerControllers.contains(id)) {
                auto consumerController = cp->data()->consumerControllers[id];
                msg["appData"] = consumerController->appData();
            }
            
            cp->data()->consumerControllers.erase(id);
            cp->notify("consumerClosed", msg);
            
            if (auto self = wself.lock()) {
                if(auto consumerController = wcc.lock()) {
                    self->removeVideoProducerQuality(consumerController);
                }
            }
        });
        
        consumerController->producerPauseSignal.connect([id = consumerController->id(), wcp = std::weak_ptr<Peer>(consumerPeer)](){
            auto cp = wcp.lock();
            if (!cp) {
                return;
            }
            nlohmann::json msg;
            msg["consumerId"] = id;
            
            cp->notify("consumerPaused", msg);
        });
        
        consumerController->producerResumeSignal.connect([id = consumerController->id(), wcp = std::weak_ptr<Peer>(consumerPeer)](){
            auto cp = wcp.lock();
            if (!cp) {
                return;
            }
            nlohmann::json msg;
            msg["consumerId"] = id;
            
            cp->notify("consumerResumed", msg);
        });
        
        consumerController->scoreSignal.connect([id = consumerController->id(), wcp = std::weak_ptr<Peer>(consumerPeer)](const srv::ConsumerScore& score){
            auto cp = wcp.lock();
            if (!cp) {
                return;
            }
            nlohmann::json msg;
            msg["consumerId"] = id;
            msg["score"] = score;
            
            cp->notify("consumerScore", msg);
        });
        
        consumerController->layersChangeSignal.connect([id = consumerController->id(), wcp = std::weak_ptr<Peer>(consumerPeer)](const srv::ConsumerLayers& layers){
            auto cp = wcp.lock();
            if (!cp) {
                return;
            }
            nlohmann::json msg;
            msg["consumerId"] = id;
            msg["spatialLayer"] = layers.spatialLayer;
            msg["temporalLayer"] = layers.temporalLayer;
            
            cp->notify("consumerLayersChanged", msg);
        });
        
        consumerController->traceSignal.connect([id = consumerController->id()](const srv::ConsumerTraceEventData& trace){
            nlohmann::json data = trace;
            SRV_LOGD("consumer 'trace' event [producerId: %s, trace.type: %s, trace: %s]", id.c_str(), trace.type.c_str(), data.dump().c_str());
        });
        
        try {
            nlohmann::json msg;
            msg["peerId"] = producerPeer->id();
            msg["producerId"] = producerController->id();
            msg["id"] = consumerController->id();
            msg["kind"] = consumerController->kind();
            msg["rtpParameters"] = consumerController->rtpParameters();
            msg["type"] = consumerController->type();
            msg["appData"] = producerController->appData();
            msg["producerPaused"] = consumerController->producerPaused();
            
            consumerPeer->request("newConsumer", msg);

            // Now that we got the positive response from the remote endpoint, resume
            // the Consumer so the remote endpoint will receive the a first RTP packet
            // of this new stream once its PeerConnection is already ready to process
            // and associate it.
            
            //consumerController->resume(); // TODO: should resume ?

            nlohmann::json scoreMsg;
            scoreMsg["consumerId"] = consumerController->id();
            scoreMsg["score"] = consumerController->score();
            
            consumerPeer->notify("consumerScore", scoreMsg);
        }
        catch (const char* error) {
            SRV_LOGE("createConsumer() | failed: %s", error);
        }
    }
}

void Room::createDataConsumer(const std::shared_ptr<Peer>& dataConsumerPeer, const std::shared_ptr<Peer>& dataProducerPeer, const std::shared_ptr<srv::IDataProducerController>& dataProducerController)
{
    
    if (dataConsumerPeer->data()->sctpCapabilities.dump().empty()) {
        SRV_LOGE("sctpCapabilities must not be empty");
        return;
    }

    // Must take the Transport the remote Peer is using for consuming.
    std::shared_ptr<srv::ITransportController> transportController;
    dataConsumerPeer->data()->transportControllers.for_each2([&transportController](auto &kv){
        auto t = kv.second;
        if(t->appData()["consuming"].template get<bool>() == true) {
            transportController = t;
            return true;
        }
        return false;
    });
    // This should not happen.
    if (!transportController) {
        SRV_LOGW("createDataConsumer() | Transport for consuming not found");
        return;
    }

    // Create the Dataconsumer
    std::shared_ptr<srv::IDataConsumerController> dataConsumerController;

    try {
        auto options = std::make_shared<srv::DataConsumerOptions>();
        options->dataProducerId = dataProducerController->id();
        dataConsumerController = transportController->consumeData(options);
    }
    catch (const char* error) {
        SRV_LOGE("createDataConsumer() | transport->consumeData(): %s", error);
        return;
    }
    
    dataConsumerPeer->data()->dataConsumerControllers.emplace(std::make_pair(dataConsumerController->id(), dataConsumerController));

    dataConsumerController->transportCloseSignal.connect([id = dataConsumerController->id(), wdcp = std::weak_ptr<Peer>(dataConsumerPeer)](){
        auto dcp = wdcp.lock();
        if (!dcp) {
            return;
        }
        dcp->data()->dataConsumerControllers.erase(id);
    });
    
    dataConsumerController->dataProducerCloseSignal.connect([id = dataConsumerController->id(), wdcp = std::weak_ptr<Peer>(dataConsumerPeer)](){
        auto dcp = wdcp.lock();
        if (!dcp) {
            return;
        }
        dcp->data()->dataConsumerControllers.erase(id);
        
        nlohmann::json msg;
        msg["dataConsumerId"] = id;
    
        dcp->notify("dataConsumerClosed", msg);
    });
    
    // Send a protoo request to the remote Peer with Consumer parameters.
    try {
        nlohmann::json msg;
        // This is null for bot dataProducer
        msg["peerId"] = dataProducerPeer->id();
        msg["dataProducerId"] = dataProducerController->id();
        msg["id"] = dataConsumerController->id();
        msg["sctpStreamParameters"] = dataConsumerController->sctpStreamParameters();
        msg["label"] = dataConsumerController->label();
        msg["protocol"] = dataConsumerController->protocol();
        msg["appData"] = dataProducerController->appData();
        
        dataConsumerPeer->request("newDataConsumer", msg);
    }
    catch (...) {
        SRV_LOGE("createDataConsumer() | failed");
    }
}

void Room::onAudioVolumes(const std::vector<srv::AudioLevelObserverVolume>& volumes)
{
    if (volumes.empty()) {
        return;
    }
    
    auto volume = volumes[0];
    
    auto producerController = volume.producerController;
    
    SRV_LOGD("audioLevelObserver 'volumes' event [producerId: %s, volume: %d]", producerController->id().c_str(), volume.volume);
    
    if (producerController->appData().contains("peerId")) {
        auto peers = getJoinedPeers();
        nlohmann::json msg;
        msg["peerId"] = producerController->appData()["peerId"];
        msg["volume"] = volume.volume;
        
        for (const auto& peer : peers) {
            peer.second->notify("activeSpeaker", msg);
        }
    }
}

void Room::onAudioSilence()
{
    SRV_LOGD("audioLevelObserver 'silence' event");
    
    auto peers = getJoinedPeers();
    nlohmann::json msg;
    msg["peerId"] = "";
    
    for (const auto& peer : peers) {
        peer.second->notify("activeSpeaker", msg);
    }
}

void Room::onDominantSpeaker(const srv::ActiveSpeakerObserverDominantSpeaker& speaker)
{
    SRV_LOGD("activeSpeakerObserver 'dominantspeaker' event [producerId: %s]", speaker.producerController->id().c_str());
}

void Room::onHandleRequest(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    if (!peer) {
        return;
    }
    
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    if (method == "getRouterRtpCapabilities") {
        nlohmann::json rtpCapabilities = _routerController->rtpCapabilities();
        accept(request, rtpCapabilities);
    }
    else if (method == "join") {
        onHandleJoin(peer, request, accept, reject);
    }
    else if (method == "createWebRtcTransport") {
        onHandleCreateWebRtcTransport(peer, request, accept, reject);
    }
    else if (method == "connectWebRtcTransport") {
        onHandleConnectWebRtcTransport(peer, request, accept, reject);
    }
    else if (method == "restartIce") {
        onHandleRestartIce(peer, request, accept, reject);
    }
    else if (method == "produce") {
        onHandleProduce(peer, request, accept, reject);
    }
    else if (method == "closeProducer") {
        onHandleCloseProducer(peer, request, accept, reject);
    }
    else if (method == "pauseProducer") {
        onHandlePauseProducer(peer, request, accept, reject);
    }
    else if (method == "resumeProducer") {
        onHandleResumeProducer(peer, request, accept, reject);
    }
    else if (method == "pauseConsumer") {
        onHandlePauseConsumer(peer, request, accept, reject);
    }
    else if (method == "resumeConsumer") {
        onHandleResumeConsumer(peer, request, accept, reject);
    }
    else if (method == "setConsumerPreferredLayers") {
        onHandleSetConsumerPreferredLayers(peer, request, accept, reject);
    }
    else if (method == "setConsumerPriority") {
        onHandleSetConsumerPriority(peer, request, accept, reject);
    }
    else if (method == "requestConsumerKeyFrame") {
        onHandleRequestConsumerKeyFrame(peer, request, accept, reject);
    }
    else if (method == "produceData") {
        onHandleProduceData(peer, request, accept, reject);
    }
    else if (method == "changeDisplayName") {
        onHandleChangeDisplayName(peer, request, accept, reject);
    }
    else if (method == "getTransportStats") {
        onHandleGetTransportStats(peer, request, accept, reject);
    }
    else if (method == "getProducerStats") {
        onHandleGetProducerStats(peer, request, accept, reject);
    }
    else if (method == "getConsumerStats") {
        onHandleGetConsumerStats(peer, request, accept, reject);
    }
    else if (method == "getDataProducerStats") {
        onHandleGetDataProducerStats(peer, request, accept, reject);
    }
    else if (method == "getDataConsumerStats") {
        onHandleGetDataConsumerStats(peer, request, accept, reject);
    }
    else if (method == "applyNetworkThrottle") {
        onHandleApplyNetworkThrottle(peer, request, accept, reject);
    }
    else if (method == "resetNetworkThrottle") {
        onHandleResetNetworkThrottle(peer, request, accept, reject);
    }
    else {
        SRV_LOGE("unknown request.method %s", method.c_str());
        reject(request, 500, "unknown request.method request.method");
    }
}

void Room::onHandleJoin(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "join");
 
    if (peer->data()->joined) {
        SRV_LOGE("[Room] Peer already joined");
        accept(request, {});
        return;
    }
    
    const auto& data = request["data"];
    const auto& displayName = data["displayName"];
    const auto& device = data["device"];
    const auto& rtpCapabilities = data["rtpCapabilities"];
    const auto& sctpCapabilities = data["sctpCapabilities"];
    
    peer->setNickname(displayName);
    peer->data()->joined = true;
    peer->data()->displayName = displayName;
    peer->data()->device = device;
    peer->data()->rtpCapabilities = rtpCapabilities;
    peer->data()->sctpCapabilities = sctpCapabilities;
    
    auto otherPeers = getJoinedPeers(peer->id());
    
    nlohmann::json peerInfos = nlohmann::json::array();
    for( const auto& kv : otherPeers) {
        const auto& peerId = kv.first;
        const auto& otherPeer = kv.second;
        nlohmann::json info;
        info["id"] = otherPeer->id();
        info["displayName"] = otherPeer->data()->displayName;
        info["device"] = otherPeer->data()->device;
        peerInfos.push_back(info);
    }
    
    accept(request, {{ "peers", peerInfos }});
    
    peer->data()->joined = true;

    for (const auto& kv : otherPeers) {
        const auto& joinedPeer = kv.second;
        // Create Consumers for existing Producers.
        joinedPeer->data()->producerControllers.for_each([this, peer, joinedPeer](const auto& ikv) {
            const auto& producerController = ikv.second;
            this->createConsumer(peer, joinedPeer, producerController);
        });

        // Create DataConsumers for existing DataProducers.
        joinedPeer->data()->dataProducerControllers.for_each([this, peer, joinedPeer](const auto& ikv) {
            const auto& dataProducerController = ikv.second;
            if (dataProducerController->label() != "bot") {
                this->createDataConsumer(peer, joinedPeer, dataProducerController);
            }
        });
    }
    
    // Create Consumer for sharing Producer.
    {
        std::lock_guard<std::mutex> lock(_sharingMutex);
        if (_videoSharingController->attached() && !_videoSharingController->closed()) {
            const auto& producerController = _videoSharingController->producerController();
            createConsumer(peer, _videoSharingController->peer(), producerController);
        }
    }
    
    // TODO: Create DataConsumers for bot DataProducer.
    // createDataConsumer(peer, nullptr, _bot.dataProducer);

    nlohmann::json msg;
    msg["id"] = peer->id();
    msg["displayName"] = peer->data()->displayName;
    msg["device"] = peer->data()->device;
    
    for (const auto& otherPeer : otherPeers) {
        otherPeer.second->notify("newPeer", msg);
    }
}

void Room::onHandleCreateWebRtcTransport(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "createWebRtcTransport");
    
    const auto& data = request["data"];
    bool forceTcp = data["forceTcp"];
    bool producing = data["producing"];
    bool consuming = data["consuming"];
    nlohmann::json sctpCapabilities = data["sctpCapabilities"];
    
    SRV_LOGD("createWebRtcTransport request.data: %s",data.dump().c_str());

    nlohmann::json jWebRtcTransportOptions = MSConfig->params()->mediasoup.webRtcTransportOptions;
    jWebRtcTransportOptions["appData"] = { { "producing", producing }, { "consuming", consuming } };
    
    if (jWebRtcTransportOptions.is_object() && !sctpCapabilities["numStreams"].is_null()) {
        jWebRtcTransportOptions["enableSctp"] = true;
        jWebRtcTransportOptions["numSctpStreams"] = sctpCapabilities["numStreams"];
    }
    else {
        jWebRtcTransportOptions["enableSctp"] = false;
        jWebRtcTransportOptions["numSctpStreams"] = nlohmann::json::object();
    }
    
    auto webRtcTransportOptions = std::make_shared<srv::WebRtcTransportOptions>(jWebRtcTransportOptions);
    webRtcTransportOptions->iceConsentTimeout = 20;
    if (forceTcp) {
       webRtcTransportOptions->enableUdp = false;
       webRtcTransportOptions->enableTcp = true;
        for (const auto& listenInfo: webRtcTransportOptions->listenInfos) {
            if (listenInfo.protocol == "tcp") {
                webRtcTransportOptions->listenInfos.clear();
                webRtcTransportOptions->listenInfos.emplace_back(listenInfo);
                break;
            }
        }
    }
    
    // single port multiplexing
    if (MSConfig->params()->mediasoup.useWebRtcServer) {
        webRtcTransportOptions->webRtcServer = _webRtcServerController;
    }
    
    auto transportController = std::dynamic_pointer_cast<srv::WebRtcTransportController>(_routerController->createWebRtcTransportController(webRtcTransportOptions));
    
    transportController->iceStateChangeSignal.connect([](const std::string& iceState) {
        if (iceState == "disconnected" || iceState == "closed") {
            SRV_LOGD("WebRtcTransport 'icestatechange' event [iceState: %s]", iceState.c_str());
            // TODO: close peer
            //peer->close();
        }
    });
    
    transportController->sctpStateChangeSignal.connect([](const std::string& sctpState){
        SRV_LOGD("WebRtcTransport 'sctpstatechange' event [sctpState: %s]", sctpState.c_str());
    });
    
    transportController->dtlsStateChangeSignal.connect([](const std::string& dtlsState){
        if (dtlsState == "failed" || dtlsState == "closed") {
            SRV_LOGW("WebRtcTransport 'dtlsstatechange' event [sctpState: %s]", dtlsState.c_str());
            // TODO: close peer
            //peer->close();
        }
    });
    
    // NOTE: For testing.
    // transport->enableTraceEvent([ "probation", "bwe" ]);
    std::vector<std::string> types;
    types.push_back("probation");
    types.push_back("bwe");
    
    transportController->enableTraceEvent(types);
    
    transportController->traceSignal.connect([transportId = transportController->id(), wpeer = std::weak_ptr<Peer>(peer)](const srv::TransportTraceEventData& data){
        nlohmann::json trace = data;
        SRV_LOGD("transport 'trace' event [transportId: %s, trace.type: %s, trace: %s]", transportId.c_str(), data.type.c_str(), trace.dump().c_str());
        
        if (auto peer = wpeer.lock()) {
            if (data.type == "bwe" && data.direction == "out") {
                nlohmann::json msg;
                if (auto info = std::dynamic_pointer_cast<srv::BweTraceInfo>(data.info)) {
                    msg["desiredBitrate"] = info->desiredBitrate;
                    msg["effectiveDesiredBitrate"] = info->effectiveDesiredBitrate;
                    msg["availableBitrate"] = info->availableBitrate;
                }
                peer->notify("downlinkBwe", msg);
            }
        }
    });
    
    peer->data()->transportControllers.emplace(std::make_pair(transportController->id(), transportController));
    
    nlohmann::json jiceCandidates = transportController->iceCandidates();
    SRV_LOGD("iceCandidates: %s", jiceCandidates.dump(4).c_str());
    
    nlohmann::json jdtlsParameters = transportController->dtlsParameters();
    SRV_LOGD("dtlsParameters: %s", jdtlsParameters.dump(4).c_str());
    
    nlohmann::json msg;
    msg["id"] = transportController->id();
    msg["iceParameters"] = transportController->iceParameters();
    msg["iceCandidates"] = transportController->iceCandidates();
    msg["dtlsParameters"] = transportController->dtlsParameters();
                                                    
    if (transportController->sctpParameters().port != 0) {
        msg["sctpParameters"] = transportController->sctpParameters();
    }

    SRV_LOGD("msg: %s", msg.dump(4).c_str());
    accept(request, msg);
    
    auto maxIncomingBitrate = MSConfig->params()->mediasoup.webRtcTransportOptions.maxIncomingBitrate;
    // If set, apply max incoming bitrate limit.
    if (maxIncomingBitrate != 0) {
        try {
            transportController->setMaxIncomingBitrate(maxIncomingBitrate);
        }
        catch (...) {}
    }
}

void Room::onHandleConnectWebRtcTransport(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "connectWebRtcTransport");
    
    const auto& data = request["data"];
    const auto& transportId = data["transportId"];
    const auto& dtlsParameters = data["dtlsParameters"];
    
    if (!peer->data()->transportControllers.contains(transportId)) {
        SRV_LOGE("transport with id transportId: %s not found", transportId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& transportController = peer->data()->transportControllers[transportId];
    
    nlohmann::json params;
    params["dtlsParameters"] = dtlsParameters;
    
    SRV_LOGD("connectWebRtcTransport dtlsParameters: %s", params.dump(4).c_str());
             
    auto connectParams = std::make_shared<srv::ConnectParams>();
    connectParams->dtlsParameters = dtlsParameters;
    
    transportController->connect(connectParams);
    
    accept(request, {});
}

void Room::onHandleRestartIce(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "restartIce");
    
    const auto& data = request["data"];
    const auto& transportId = data["transportId"];
    
    if (!peer->data()->transportControllers.contains(transportId)) {
        SRV_LOGE("transport with id transportId: %s not found", transportId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& transportController = peer->data()->transportControllers[transportId];
    
    if (auto wtc = std::dynamic_pointer_cast<srv::WebRtcTransportController>(transportController)) {
        auto iceParameters = *wtc->restartIce();
        accept(request, iceParameters);
    }
}

void Room::onHandleProduce(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "produce");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }
    
    const auto& data = request["data"];
    const auto& kind = data["kind"];
    if (kind == "video" && data.contains("appData") && data["appData"].contains("sharing")) {
        onHandleSharingProduce(peer, request, accept, reject);
    }
    else {
        onHandleDefaultProduce(peer, request, accept, reject);
    }
}

void Room::onHandleDefaultProduce(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "produce");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }
    
    const auto& data = request["data"];
    const auto& transportId = data["transportId"];
    const auto& kind = data["kind"];
    
    const auto& rtpParameters = data["rtpParameters"];
    SRV_LOGD("produce rtpParameters: %s", rtpParameters.dump(4).c_str());
    
    if (!peer->data()->transportControllers.contains(transportId)) {
        SRV_LOGE("transport with id transportId: %s not found", transportId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& transportController = peer->data()->transportControllers[transportId];
    
    nlohmann::json appData;
    if (data.contains("appData")) {
        appData = data["appData"];
    }
    //const auto& paused = data["paused"];
    
    nlohmann::json producerAppData;
    producerAppData["peerId"] = peer->id();
    appData.merge_patch(producerAppData);
    
    auto options = std::make_shared<srv::ProducerOptions>();
    options->kind = kind;
    options->rtpParameters = rtpParameters;
    options->appData = appData;
    options->keyFrameRequestDelay = 5000;

    nlohmann::json jrtpParameters = options->rtpParameters;
    
    SRV_LOGD("produce jrtpParameters: %s", jrtpParameters.dump(4).c_str());
    
    auto producerController = transportController->produce(options);
    
    peer->data()->producerControllers.emplace(std::make_pair(producerController->id(), producerController));
    
    producerController->scoreSignal.connect([wpeer = std::weak_ptr<Peer>(peer), id = producerController->id()](const std::vector<srv::ProducerScore>& scores){
        auto peer = wpeer.lock();
        if (!peer) {
            return;
        }
        nlohmann::json msg;
        msg["producerId"] = id;
        msg["scores"] = scores;
        peer->notify("producerScore", msg);
    });

    producerController->videoOrientationChangeSignal.connect([id = producerController->id()](const srv::ProducerVideoOrientation& videoOrientation){
        nlohmann::json jVideoOrientation = videoOrientation;
        SRV_LOGD("producer 'videoorientationchange' event [producerId: %s, videoOrientation: %s]", id.c_str(), jVideoOrientation.dump().c_str());
    });
    
    producerController->traceSignal.connect([id = producerController->id()](const srv::ProducerTraceEventData& data){
        nlohmann::json trace = data;
        SRV_LOGD("producer 'videoorientationchange' event [producerId: %s, videoOrientation: %s]", id.c_str(), trace.dump().c_str());
    });
    
    nlohmann::json msg;
    msg["id"] = producerController->id();
    
    accept(request, msg);
    
    auto peers = getJoinedPeers(peer->id());
    
    for( const auto& kv : peers) {
        createConsumer(kv.second, peer, producerController);
    }
    
    if (producerController->kind() == "audio") {
        _audioLevelObserverController->addProducer(producerController->id());
        _activeSpeakerObserverController->addProducer(producerController->id());
    }
}

void Room::onHandleSharingProduce(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "produce");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }
    
    const auto& data = request["data"];
    const auto& transportId = data["transportId"];
    const auto& kind = data["kind"];
    
    const auto& rtpParameters = data["rtpParameters"];
    SRV_LOGD("produce rtpParameters: %s", rtpParameters.dump(4).c_str());
    
    if (!peer->data()->transportControllers.contains(transportId)) {
        SRV_LOGE("transport with id transportId: %s not found", transportId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& transportController = peer->data()->transportControllers[transportId];

    nlohmann::json appData;
    if (data.contains("appData")) {
        appData = data["appData"];
    }
    
    nlohmann::json producerAppData;
    producerAppData["peerId"] = peer->id();
    appData.merge_patch(producerAppData);
    
    auto options = std::make_shared<srv::ProducerOptions>();
    options->kind = kind;
    options->rtpParameters = rtpParameters;
    options->appData = appData;
    options->keyFrameRequestDelay = 5000;

    nlohmann::json jrtpParameters = options->rtpParameters;
    
    SRV_LOGD("produce jrtpParameters: %s", jrtpParameters.dump(4).c_str());
    
    auto producerController = transportController->produce(options);
    producerController->setAppData(appData);
    
    {
        std::lock_guard<std::mutex> lock(_sharingMutex);
        if (_videoSharingController->attached() && !_videoSharingController->closed()) {
            _videoSharingController->close();
            _videoSharingController->detach();
        }
        _videoSharingController->attach(peer, producerController);
    }
    
    producerController->scoreSignal.connect([wpeer = std::weak_ptr<Peer>(peer), id = producerController->id()](const std::vector<srv::ProducerScore>& scores){
        auto peer = wpeer.lock();
        if (!peer) {
            return;
        }
        nlohmann::json msg;
        msg["producerId"] = id;
        msg["scores"] = scores;
        peer->notify("producerScore", msg);
    });

    producerController->videoOrientationChangeSignal.connect([id = producerController->id()](const srv::ProducerVideoOrientation& videoOrientation){
        nlohmann::json jVideoOrientation = videoOrientation;
        SRV_LOGD("producer 'videoorientationchange' event [producerId: %s, videoOrientation: %s]", id.c_str(), jVideoOrientation.dump().c_str());
    });
    
    producerController->traceSignal.connect([id = producerController->id()](const srv::ProducerTraceEventData& data){
        nlohmann::json trace = data;
        SRV_LOGD("producer 'videoorientationchange' event [producerId: %s, videoOrientation: %s]", id.c_str(), trace.dump().c_str());
    });
    
    nlohmann::json msg;
    msg["id"] = producerController->id();
    
    accept(request, msg);
    
    auto peers = getJoinedPeers(peer->id());
    
    for( const auto& kv : peers) {
        createConsumer(kv.second, peer, producerController);
    }
}

void Room::onHandleCloseProducer(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "closeProducer");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }

    const auto& data = request["data"];
    const auto& producerId = data["producerId"];
    
    std::shared_ptr<srv::IProducerController> producerConstroller;
    if (peer->data()->producerControllers.contains(producerId)) {
        producerConstroller = peer->data()->producerControllers[producerId];
        producerConstroller->close();
        peer->data()->producerControllers.erase(producerConstroller->id());
        accept(request, {});
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(_sharingMutex);
        if (_videoSharingController->producerController()->id() == producerId) {
            producerConstroller = _videoSharingController->producerController();
            producerConstroller->close();
            _videoSharingController->detach();
            accept(request, {});
            return;
        }
    }

    SRV_LOGD("producer with id producerId: %s not found", producerId.dump().c_str());
    accept(request, {});
}

void Room::onHandlePauseProducer(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "pauseProducer");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }

    const auto& data = request["data"];
    const auto& producerId = data["producerId"];
    
    if (!peer->data()->producerControllers.contains(producerId)) {
        SRV_LOGD("producer with id producerId: %s not found", producerId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& producerConstroller = peer->data()->producerControllers[producerId];
    
    producerConstroller->pause();
    
    accept(request, {});
}

void Room::onHandleResumeProducer(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "resumeProducer");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }

    const auto& data = request["data"];
    const auto& producerId = data["producerId"];
    
    if (!peer->data()->producerControllers.contains(producerId)) {
        SRV_LOGD("producer with id producerId: %s not found", producerId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& producerConstroller = peer->data()->producerControllers[producerId];
    
    producerConstroller->resume();
    
    accept(request, {});
}

void Room::onHandlePauseConsumer(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "pauseConsumer");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }

    const auto& data = request["data"];
    const auto& consumerId = data["consumerId"];
    
    if (!peer->data()->consumerControllers.contains(consumerId)) {
        SRV_LOGD("consumer with id consumerId: %s not found", consumerId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& consumerConstroller = peer->data()->consumerControllers[consumerId];

    consumerConstroller->pause();
    
    accept(request, {});
    
    if (consumerConstroller->kind() == "video") {
        updateVideoProducerQuality(consumerConstroller);
    }
}

void Room::onHandleResumeConsumer(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "resumeConsumer");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }

    const auto& data = request["data"];
    const auto& consumerId = data["consumerId"];
    
    if (!peer->data()->consumerControllers.contains(consumerId)) {
        SRV_LOGD("consumer with id consumerId: %s not found", consumerId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& consumerConstroller = peer->data()->consumerControllers[consumerId];
    
    consumerConstroller->resume();
    
    accept(request, {});
    
    if (consumerConstroller->kind() == "video") {
        updateVideoProducerQuality(consumerConstroller);
    }
}

void Room::onHandleSetConsumerPreferredLayers(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "setConsumerPreferredLayers");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }

    const auto& data = request["data"];
    const auto& consumerId = data["consumerId"];
    
    srv::ConsumerLayers consumerLayers;
    consumerLayers.spatialLayer = data["spatialLayer"];
    consumerLayers.temporalLayer = data["temporalLayer"];

    if (!peer->data()->consumerControllers.contains(consumerId)) {
        SRV_LOGD("consumer with id consumerId: %s not found", consumerId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& consumerConstroller = peer->data()->consumerControllers[consumerId];
    
    consumerConstroller->setPreferredLayers(consumerLayers);
    
    accept(request, {});
    
    if (consumerConstroller->kind() == "video") {
        updateVideoProducerQuality(consumerConstroller);
    }
}

void Room::onHandleSetConsumerPriority(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "setConsumerPriority");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }

    //SRV_LOGD("request.dump: %s", request.dump().c_str());
    const auto& data = request["data"];
    const auto& consumerId = data["consumerId"];
    const auto& priority = data["priority"];
    
    if (!peer->data()->consumerControllers.contains(consumerId)) {
        SRV_LOGD("consumer with id consumerId: %s not found", consumerId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& consumerConstroller = peer->data()->consumerControllers[consumerId];
    
    consumerConstroller->setPriority(priority);
    
    accept(request, {});
}

void Room::onHandleRequestConsumerKeyFrame(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "requestConsumerKeyFrame");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }

    const auto& data = request["data"];
    const auto& consumerId = data["consumerId"];
    
    if (!peer->data()->consumerControllers.contains(consumerId)) {
        SRV_LOGD("consumer with id consumerId: %s not found", consumerId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& consumerConstroller = peer->data()->consumerControllers[consumerId];
    
    consumerConstroller->requestKeyFrame();
    
    accept(request, {});
}

void Room::onHandleProduceData(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "produceData");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }

    const auto& data = request["data"];
    const auto& transportId = data["transportId"];
    const auto& sctpStreamParameters = data["sctpStreamParameters"];
    const auto& label = data["label"];
    const auto& protocol = data["protocol"];
    const auto& appData = data["appData"];
    
    if (!peer->data()->transportControllers.contains(transportId)) {
        SRV_LOGE("transport with id transportId: %s not found", transportId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& transportController = peer->data()->transportControllers[transportId];
    
    auto option = std::make_shared<srv::DataProducerOptions>();
    option->sctpStreamParameters = sctpStreamParameters,
    option->label = label;
    option->protocol = protocol;
    option->appData = appData;
    
    auto dataProducerController = transportController->produceData(option);
    
    peer->data()->dataProducerControllers.emplace(std::make_pair(dataProducerController->id(), dataProducerController));
    
    nlohmann::json msg;
    msg["id"] = dataProducerController->id();
    
    accept(request, msg);
    
    if (dataProducerController->label() == "chat") {
        auto peers = getJoinedPeers(peer->id());
        
        for( const auto& kv : peers) {
            createDataConsumer(kv.second, peer, dataProducerController);
        }
    }
}

void Room::onHandleChangeDisplayName(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "changeDisplayName");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }
    
    const auto& data = request["data"];
    const auto& displayName = data["displayName"];
    const auto oldDisplayName = peer->data()->displayName;
    
    // Store the display name into the custom data Object of the protoo
    // Peer.
    peer->data()->displayName = displayName;
    
    nlohmann::json msg;
    msg["peerId"] = peer->id();
    msg["displayName"] = displayName;
    msg["oldDisplayName"] = oldDisplayName;
    
    // Notify other joined Peers.
    auto peers = getJoinedPeers(peer->id());
    for( const auto& kv : peers) {
        const auto& otherPeer = kv.second;
        otherPeer->notify("peerDisplayNameChanged", msg);
    }
    
    accept(request, {});
}

void Room::onHandleGetTransportStats(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "getTransportStats");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }
    
    const auto& data = request["data"];
    const auto& transportId = data["transportId"];
    
    if (!peer->data()->transportControllers.contains(transportId)) {
        SRV_LOGE("transport with id transportId: %s not found", transportId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& transportController = peer->data()->transportControllers[transportId];

    auto stats = transportController->getStats();
    nlohmann::json jsonStats = *stats;
    
    accept(request, jsonStats);
}

void Room::onHandleGetProducerStats(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "getProducerStats");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }

    const auto& data = request["data"];
    const auto& producerId = data["producerId"];
    
    if (!peer->data()->producerControllers.contains(producerId)) {
        SRV_LOGD("producer with id producerId: %s not found", producerId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& producerConstroller = peer->data()->producerControllers[producerId];

    auto stats = producerConstroller->getStats();
    
    std::vector<srv::ProducerStat> statsVec;
    for (const auto& item : stats) {
        statsVec.emplace_back(*item);
    }
    
    nlohmann::json jsonStats = statsVec;
    
    accept(request, jsonStats);
}

void Room::onHandleGetConsumerStats(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "getConsumerStats");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }

    const auto& data = request["data"];
    const auto& consumerId = data["consumerId"];
    
    if (!peer->data()->consumerControllers.contains(consumerId)) {
        SRV_LOGD("consumer with id consumerId: %s not found", consumerId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& consumerConstroller = peer->data()->consumerControllers[consumerId];

    auto stats = consumerConstroller->getStats();
    
    std::vector<srv::ConsumerStat> statsVec;
    for (const auto& item : stats) {
        statsVec.emplace_back(*item);
    }
    
    nlohmann::json jsonStats = statsVec;
    
    accept(request, jsonStats);
}

void Room::onHandleGetDataProducerStats(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "getDataProducerStats");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }

    const auto& data = request["data"];
    const auto& dataProducerId = data["dataProducerId"];
    
    if (!peer->data()->dataProducerControllers.contains(dataProducerId)) {
        SRV_LOGD("data producer with id producerId: %s not found", dataProducerId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& dataProducerConstroller = peer->data()->dataProducerControllers[dataProducerId];
    
    auto stats = dataProducerConstroller->getStats();
    
    std::vector<srv::DataProducerStat> statsVec;
    for (const auto& item : stats) {
        statsVec.emplace_back(*item);
    }
    
    nlohmann::json jsonStats = statsVec;
    
    accept(request, jsonStats);
}

void Room::onHandleGetDataConsumerStats(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "getDataConsumerStats");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }

    const auto& data = request["data"];
    const auto& dataConsumerId = data["dataConsumerId"];
    
    if (!peer->data()->dataConsumerControllers.contains(dataConsumerId)) {
        SRV_LOGD("data consumer with id consumerId: %s not found", dataConsumerId.dump().c_str());
        accept(request, {});
        return;
    }
    
    const auto& dataConsumerConstroller = peer->data()->dataConsumerControllers[dataConsumerId];
    
    auto stats = dataConsumerConstroller->getStats();
    
    std::vector<srv::DataConsumerStat> statsVec;
    for (const auto& item : stats) {
        statsVec.emplace_back(*item);
    }
    
    nlohmann::json jsonStats = statsVec;
    
    accept(request, jsonStats);
}
    
void Room::onHandleResetNetworkThrottle(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "applyNetworkThrottle");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }
    
    accept(request, {});
}

void Room::onHandleApplyNetworkThrottle(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject)
{
    std::string method;
    if (request.contains("method")) {
        method = request["method"];
    }
    
    assert(method == "resetNetworkThrottle");
    
    if (!peer->data()->joined) {
        SRV_LOGE("Peer not yet joined");
        accept(request, {});
        return;
    }
    
    accept(request, {});
}

void Room::updateVideoProducerQuality(const std::shared_ptr<srv::IConsumerController>& consumerController)
{
    if (consumerController->kind() != "video") {
        return;
    }
    
    auto producerId = consumerController->producerId();
    
    std::shared_ptr<Peer> producerPeer;
    auto peers = getJoinedPeers("");
    for (const auto& item : peers) {
        if (item.second->data()->producerControllers.contains(producerId)) {
            producerPeer = item.second;
            break;
        }
    }

    if (!producerPeer) {
        return;
    }
    
    SRV_LOGE("--> PeerId: %s", producerPeer->id().c_str());
    
    auto producerPeerData = producerPeer->data();
    
    std::shared_ptr<VideoProducerQualityController> qualityController;
    if (!producerPeerData->videoProducerQualityControllers.contains(producerId)) {
        qualityController = std::make_shared<VideoProducerQualityController>();
        producerPeerData->videoProducerQualityControllers.emplace(std::make_pair(producerId, qualityController));
    }
    else {
        qualityController = producerPeerData->videoProducerQualityControllers[producerId];
    }
    
    srv::ConsumerLayers layers = consumerController->preferredLayers();
    auto consumerPaused = consumerController->paused();
    qualityController->addOrUpdateConsumer(consumerController->id(), consumerPaused, layers.spatialLayer);

    auto maxQ = qualityController->getMaxDesiredQ();
    auto paused = qualityController->isAllConsumerPaused();
    
    nlohmann::json msg;
    msg["producerId"] = producerId;
    msg["paused"] = paused;
    msg["desiredQ"] = maxQ;
    producerPeer->request("videoProducerQualityChanged", msg);
}

void Room::removeVideoProducerQuality(const std::shared_ptr<srv::IConsumerController>& consumerController)
{
    auto producerId = consumerController->producerId();
    
    std::shared_ptr<Peer> producerPeer;
    auto peers = getJoinedPeers("");
    for (const auto& item : peers) {
        if (item.second->data()->producerControllers.contains(producerId)) {
            producerPeer = item.second;
            break;
        }
    }

    if (!producerPeer) {
        return;
    }
    
    auto producerPeerData = producerPeer->data();

    std::shared_ptr<VideoProducerQualityController> qualityController;
    
    if (!producerPeerData->videoProducerQualityControllers.contains(producerId)) {
        return;
    }
    
    qualityController = producerPeerData->videoProducerQualityControllers[producerId];
    
    qualityController->removeConsumer(consumerController->id());

    auto maxQ = qualityController->getMaxDesiredQ();
    auto paused = qualityController->isAllConsumerPaused();

    nlohmann::json msg;
    msg["producerId"] = producerId;
    msg["paused"] = paused;
    msg["desiredQ"] = maxQ;
    producerPeer->request("videoProducerQualityChanged", msg);
}

void Room::onNewConsumerResumed(const std::shared_ptr<srv::IConsumerController>& consumerController)
{
    assert(consumerController);
    
    updateVideoProducerQuality(consumerController);
}
