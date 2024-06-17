/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: RTC-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#include "lobby.hpp"
#include "srv_logger.h"

Lobby::Lobby()
{
    SRV_LOGD("Lobby()");
}

Lobby::~Lobby()
{
    SRV_LOGD("~Lobby()");
}

std::shared_ptr<Room> Lobby::getOrCreateRoom(const std::string& roomId)
{
    if (_roomMap.contains(roomId)) {
        return _roomMap[roomId];
    }
    else {
        auto room = Room::create(roomId, 0);
        room->init();
        room->closeSignal.connect(&Lobby::onRoomClose, shared_from_this());
        _roomMap.emplace(std::make_pair(roomId, room));
        return room;
    }
}

std::shared_ptr<Room> Lobby::getRoom(const std::string& roomId)
{
    if (_roomMap.contains(roomId)) {
        return _roomMap[roomId];
    }
    return nullptr;
}

void Lobby::deleteRoom(const std::string& roomId)
{
    _roomMap.erase(roomId);
}

void Lobby::runPingLoop(const std::chrono::duration<v_int64, std::micro>& interval)
{
    while (true) {
        std::chrono::duration<v_int64, std::micro> elapsed = std::chrono::microseconds(0);
        auto startTime = std::chrono::system_clock::now();

        do {
            std::this_thread::sleep_for(interval - elapsed);
            elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - startTime);
        } while (elapsed < interval);

        _roomMap.for_each([](const auto& item){
            item.second->pingAllPeers();
        });
    }
}

void Lobby::onAfterCreate_NonBlocking(const std::shared_ptr<AsyncWebSocket>& socket, const std::shared_ptr<const ParameterMap>& params)
{
    ++_statistics->EVENT_PEER_CONNECTED;

    auto roomId = params->find("roomId")->second;
    auto peerId = params->find("peerId")->second;
    auto forceH264 = params->find("forceH264")->second;
    auto forceVP9 = params->find("forceVP9")->second;
    
    auto room = getOrCreateRoom(roomId->c_str());

    if (!room) {
        SRV_LOGD("get or create room failed");
        return;
    }
    
    room->createPeer(socket, roomId->c_str(), peerId->c_str());
}

void Lobby::onBeforeDestroy_NonBlocking(const std::shared_ptr<AsyncWebSocket>& socket)
{
    ++_statistics->EVENT_PEER_DISCONNECTED;

    auto peer = std::static_pointer_cast<Peer>(socket->getListener());
    auto roomId = peer->roomId();

    if (!_roomMap.contains(roomId)) {
        return;
    }
    
    auto room = _roomMap[roomId];
    peer->close();
    peer->invalidateSocket();
    room->removePeer(peer->id());
    if (room->isEmpty()) {
        deleteRoom(roomId);
    }
}

void Lobby::onRoomClose(const std::string& roomId)
{
    deleteRoom(roomId);
}
