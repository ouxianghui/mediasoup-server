/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: RTC-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#pragma once

#include <memory>
#include "threadsafe_unordered_map.hpp"
#include <mutex>
#include "room.hpp"
#include "utils/statistics.hpp"
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
    std::threadsafe_unordered_map<std::string, std::shared_ptr<Room>> _roomMap;
    
private:
    OATPP_COMPONENT(std::shared_ptr<Statistics>, _statistics);
};
