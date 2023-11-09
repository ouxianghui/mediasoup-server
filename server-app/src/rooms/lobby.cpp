/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Leia-SFU
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
    std::lock_guard<std::mutex> lock(_roomMapMutex);
    std::shared_ptr<Room>& room = _roomMap[roomId];
    if (!room) {
        room = Room::create(roomId, 0);
        room->init();
        room->_closeSignal.connect(&Lobby::onRoomClose, shared_from_this());
    }
    return room;
}

std::shared_ptr<Room> Lobby::getRoom(const std::string& roomId)
{
    std::lock_guard<std::mutex> lock(_roomMapMutex);
    auto it = _roomMap.find(roomId);
    if (it != _roomMap.end()) {
        return it->second;
    }
    return nullptr;
}

void Lobby::deleteRoom(const std::string& roomId)
{
    _roomMap.erase(roomId);
}

void Lobby::runPingLoop(const std::chrono::duration<v_int64, std::micro>& interval)
{
    while(true) {
        std::chrono::duration<v_int64, std::micro> elapsed = std::chrono::microseconds(0);
        auto startTime = std::chrono::system_clock::now();

        do {
            std::this_thread::sleep_for(interval - elapsed);
            elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - startTime);
        } while (elapsed < interval);

        std::lock_guard<std::mutex> lock(_roomMapMutex);
        for (const auto &room : _roomMap) {
            room.second->pingAllPeers();
        }
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

    {
        std::lock_guard<std::mutex> lock(_roomMapMutex);
        if (auto room = _roomMap[roomId.c_str()]) {
            room->removePeer(peer->id());
            peer->invalidateSocket();
            peer->close();
            if (room->isEmpty()) {
                deleteRoom(roomId.c_str());
            }
        }
    }
}

void Lobby::onRoomClose(const std::string& roomId)
{
    deleteRoom(roomId.c_str());
}