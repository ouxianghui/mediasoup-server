/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: RTC-SFU
* @Version: 1.0.0
* @Author: Felix Zheng
* @CreateTime: 2023-11-17
*************************************************************************/

#pragma once
#include <mutex>
#include "threadsafe_unordered_map.hpp"

class VideoProducerQualityController {
private:
    struct ConsumedStatusInfo {
        bool m_paused = false;
        int m_layer = -1;
    };
    
public:
    void addOrUpdateConsumer(const std::string& consumerId, bool paused, int layer);
    void removeConsumer(const std::string& consumerId);
    bool isAllConsumerPaused();
    int getMaxDesiredQ();

private:
    std::threadsafe_unordered_map<std::string, ConsumedStatusInfo> m_mapConsumedStatusInfo;
};
