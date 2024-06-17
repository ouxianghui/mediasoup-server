/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: RTC-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#include "statistics.hpp"
#include <thread>
#include "oatpp/Environment.hpp"

void Statistics::takeSample()
{
    auto maxPeriodMicro = _maxPeriod.count();
    auto pushIntervalMicro = _pushInterval.count();

    std::lock_guard<std::mutex> guard(_dataLock);

    auto nowMicro = oatpp::Environment::getMicroTickCount();

    oatpp::Object<StatPointDto> point;

    if (_dataPoints->size() > 0) {
        const auto& p = _dataPoints->back();
        if (nowMicro - *p->timestamp < pushIntervalMicro) {
            point = p;
        }
    }

    if (!point) {
        point = StatPointDto::createShared();
        point->timestamp = nowMicro;

        _dataPoints->push_back(point);

        auto diffMicro = nowMicro - *_dataPoints->front()->timestamp;
        while(diffMicro > maxPeriodMicro) {
            _dataPoints->pop_front();
            diffMicro = nowMicro - *_dataPoints->front()->timestamp;
        }
    }

    point->evFrontpageLoaded = EVENT_FRONT_PAGE_LOADED.load();

    point->evPeerConnected = EVENT_PEER_CONNECTED.load();
    point->evPeerDisconnected = EVENT_PEER_DISCONNECTED.load();
    point->evPeerZombieDropped = EVENT_PEER_ZOMBIE_DROPPED.load();
    point->evPeerSendMessage = EVENT_PEER_SEND_MESSAGE.load();

    point->evRoomCreated = EVENT_ROOM_CREATED.load();
    point->evRoomDeleted = EVENT_ROOM_DELETED.load();
}

oatpp::String Statistics::getJsonData()
{
    std::lock_guard<std::mutex> guard(_dataLock);
    return _objectMapper.writeToString(_dataPoints);
}

void Statistics::runStatLoop()
{
    while(true) {
        std::chrono::duration<v_int64, std::micro> elapsed = std::chrono::microseconds(0);
        auto startTime = std::chrono::system_clock::now();

        do {
            std::this_thread::sleep_for(_updateInterval - elapsed);
            elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - startTime);
        } while (elapsed < _updateInterval);

        takeSample();
    }
}
