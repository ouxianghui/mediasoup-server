/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: RTC-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#pragma once

#include "dto/dtos.hpp"
#include "oatpp/json/ObjectMapper.hpp"
#include "oatpp/Types.hpp"

#include <chrono>

class Statistics
{
public:

    std::atomic<v_uint64> EVENT_FRONT_PAGE_LOADED   {0};          // On Frontpage Loaded

    std::atomic<v_uint64> EVENT_PEER_CONNECTED      {0};          // On Connected event counter
    std::atomic<v_uint64> EVENT_PEER_DISCONNECTED   {0};          // On Disconnected event counter
    std::atomic<v_uint64> EVENT_PEER_ZOMBIE_DROPPED {0};          // On Disconnected due to failed ping counter
    std::atomic<v_uint64> EVENT_PEER_SEND_MESSAGE   {0};          // Sent messages counter

    std::atomic<v_uint64> EVENT_ROOM_CREATED        {0};          // On room created
    std::atomic<v_uint64> EVENT_ROOM_DELETED        {0};          // On room deleted

public:

    Statistics(const std::chrono::duration<v_int64, std::micro>& maxPeriod = std::chrono::hours(7 * 24),
               const std::chrono::duration<v_int64, std::micro>& pushInterval = std::chrono::hours(1),
               const std::chrono::duration<v_int64, std::micro>& updateInterval = std::chrono::seconds(1))
    : _maxPeriod(maxPeriod)
    , _pushInterval(pushInterval)
    , _updateInterval(updateInterval) {}

    void takeSample();
    
    oatpp::String getJsonData();

    void runStatLoop();

private:
    oatpp::json::ObjectMapper _objectMapper;
    
private:
    oatpp::List<oatpp::Object<StatPointDto>> _dataPoints = oatpp::List<oatpp::Object<StatPointDto>>::createShared();
    std::mutex _dataLock;
    
private:
    std::chrono::duration<v_int64, std::micro> _maxPeriod;
    std::chrono::duration<v_int64, std::micro> _pushInterval;
    std::chrono::duration<v_int64, std::micro> _updateInterval;
};
