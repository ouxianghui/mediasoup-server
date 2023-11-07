/***************************************************************************
 *
 * Project:   ______                ______ _
 *           / _____)              / _____) |          _
 *          | /      ____ ____ ___| /     | | _   ____| |_
 *          | |     / _  |  _ (___) |     | || \ / _  |  _)
 *          | \____( ( | | | | |  | \_____| | | ( ( | | |__
 *           \______)_||_|_| |_|   \______)_| |_|\_||_|\___)
 *
 *
 * Copyright 2020-present, Leonid Stryzhevskyi <lganzzzo@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************************/

#ifndef ASYNC_SERVER_ROOMS_LOBBY_HPP
#define ASYNC_SERVER_ROOMS_LOBBY_HPP

#include <memory>
#include <unordered_map>
#include <mutex>
#include "Room.hpp"
#include "utils/Statistics.hpp"
#include "oatpp-websocket/AsyncConnectionHandler.hpp"

class Lobby : public oatpp::websocket::AsyncConnectionHandler::SocketInstanceListener, public std::enable_shared_from_this<Lobby>
{
public:
    Lobby();
    
    ~Lobby();

    /**
     * Get room by name or create new one if not exists.
     * @param roomId
     * @return
     */
    std::shared_ptr<Room> getOrCreateRoom(const std::string& roomId);

    /**
     * Get room by name.
     * @param roomName
     * @return
     */
    std::shared_ptr<Room> getRoom(const std::string& roomId);

    /**
     * Delete room by name.
     * @param roomName
     */
    void deleteRoom(const std::string& roomId);

    /**
     * Websocket-Ping all peers in the loop. Each time `interval`.
     * @param interval
     */
    void runPingLoop(const std::chrono::duration<v_int64, std::micro>& interval = std::chrono::minutes(1));
    
    void onRoomClose(const std::string& roomId);

public:
    /**
     *  Called when socket is created
     */
    void onAfterCreate_NonBlocking(const std::shared_ptr<AsyncWebSocket>& socket, const std::shared_ptr<const ParameterMap>& params) override;

    /**
     *  Called before socket instance is destroyed.
     */
    void onBeforeDestroy_NonBlocking(const std::shared_ptr<AsyncWebSocket>& socket) override;
    
public:
    std::mutex _roomMapMutex;
    
    std::unordered_map<std::string, std::shared_ptr<Room>> _roomMap;
    
private:
    OATPP_COMPONENT(std::shared_ptr<Statistics>, _statistics);
};


#endif //ASYNC_SERVER_ROOMS_LOBBY_HPP
