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

#include "Statistics.hpp"
#include <thread>

void Statistics::takeSample()
{
    auto maxPeriodMicro = _maxPeriod.count();
    auto pushIntervalMicro = _pushInterval.count();

    std::lock_guard<std::mutex> guard(_dataLock);

    auto nowMicro = oatpp::base::Environment::getMicroTickCount();

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
