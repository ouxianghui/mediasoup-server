/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Leia-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#pragma once

#include <memory>
#include <atomic>
#include <unordered_map>
#include <list>
#include "peer.hpp"
#include "dto/dtos.hpp"
#include "utils/statistics.hpp"
#include "oatpp/core/macro/component.hpp"
#include "nlohmann/json.hpp"
#include "oatpp-websocket/AsyncConnectionHandler.hpp"

namespace srv {
    class WebRtcServerController;
    class RouterController;
    class AudioLevelObserverController;
    class ActiveSpeakerObserverController;
    class AudioLevelObserverVolume;
    class ActiveSpeakerObserverDominantSpeaker;
}

class Room : public std::enable_shared_from_this<Room>
{
public:
    static std::shared_ptr<Room> create(const std::string& roomId, int32_t consumerReplicas = 0);
    
    Room(const std::string& roomId,
         const std::shared_ptr<srv::WebRtcServerController>& webRtcServerController,
         const std::shared_ptr<srv::RouterController>& routerController,
         const std::shared_ptr<srv::AudioLevelObserverController>& audioLevelObserverController,
         const std::shared_ptr<srv::ActiveSpeakerObserverController>& activeSpeakerObserverController,
         int32_t consumerReplicas);

    ~Room();

    void init();
    
    void destroy();
    
    void close();
    
    bool closed() { return _closed; }

    const std::string& id() { return _id; }

    void createPeer(const std::shared_ptr<oatpp::websocket::AsyncWebSocket>& socket, const std::string& roomId, const std::string& peerId);

    std::shared_ptr<Peer> getPeer(const std::string& peerId);

    void removePeer(const std::string& peerId);

    void pingAllPeers();

    bool isEmpty();
    
    void onPeerClose(const std::string& peerId);
    
    void onHandleRequest(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleNotification(const nlohmann::json& notification) {}
    
public:
    // roomId
    sigslot::signal<const std::string&> closeSignal;
    
private:
    void handleAudioLevelObserver();

    void handleActiveSpeakerObserver();
    
    std::unordered_map<std::string, std::shared_ptr<Peer>> getJoinedPeers(const std::string& excludePeerId = "");
    
    void createConsumer(const std::shared_ptr<Peer>& consumerPeer, const std::shared_ptr<Peer>& producerPeer, const std::shared_ptr<srv::ProducerController>& producerController);
    
    void createDataConsumer(const std::shared_ptr<Peer>& dataConsumerPeer, const std::shared_ptr<Peer>& dataProducerPeer, const std::shared_ptr<srv::DataProducerController>& dataProducerController);

    void onAudioVolumes(const std::vector<srv::AudioLevelObserverVolume>& volumes);
    
    void onAudioSilence();
    
    void onDominantSpeaker(const srv::ActiveSpeakerObserverDominantSpeaker& speaker);
    
private:
    void onHandleJoin(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleCreateWebRtcTransport(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleConnectWebRtcTransport(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleRestartIce(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleProduce(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleCloseProducer(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandlePauseProducer(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleResumeProducer(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandlePauseConsumer(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleResumeConsumer(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleSetConsumerPreferredLayers(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleSetConsumerPriority(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleRequestConsumerKeyFrame(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleProduceData(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleChangeDisplayName(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleGetTransportStats(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleGetProducerStats(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleGetConsumerStats(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleGetDataConsumerStats(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleGetDataProducerStats(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
    void onHandleResetNetworkThrottle(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
                                      
    void onHandleApplyNetworkThrottle(const std::shared_ptr<Peer>& peer, const nlohmann::json& request, AcceptFunc& accept, RejectFunc& reject);
    
private:
    std::string _id;
    
    std::mutex _peerMutex;
    
    std::unordered_map<std::string, std::shared_ptr<Peer>> _peerMap;
    
    std::shared_ptr<srv::WebRtcServerController> _webRtcServerController;
    
    std::shared_ptr<srv::RouterController> _routerController;
    
    std::shared_ptr<srv::AudioLevelObserverController> _audioLevelObserverController;
    
    std::shared_ptr<srv::ActiveSpeakerObserverController> _activeSpeakerObserverController;
    
    std::atomic_int32_t _consumerReplicas {0};
    
    std::atomic_bool _closed {false};
    
private:
    OATPP_COMPONENT(oatpp::Object<ConfigDto>, _appConfig);
    
    OATPP_COMPONENT(std::shared_ptr<Statistics>, _statistics);
};
