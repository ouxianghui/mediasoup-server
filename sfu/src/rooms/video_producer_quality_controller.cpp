/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: RTC-SFU
* @Version: 1.0.0
* @Author: Felix Zheng
* @CreateTime: 2023-11-17
*************************************************************************/


#include "video_producer_quality_controller.hpp"

void VideoProducerQualityController::addOrUpdateConsumer(const std::string& consumerId, bool paused, int layer)
{
    if (!m_mapConsumedStatusInfo.contains(consumerId)) {
        ConsumedStatusInfo consumeStatus;
        consumeStatus.m_paused = paused;
        consumeStatus.m_layer = layer;
        m_mapConsumedStatusInfo.emplace(std::make_pair(consumerId, consumeStatus));
    }
    else {
        ConsumedStatusInfo consumeStatus;
        consumeStatus.m_paused = paused;
        consumeStatus.m_layer = layer;
        m_mapConsumedStatusInfo.set(consumerId, consumeStatus);
    }
}

void VideoProducerQualityController::removeConsumer(const std::string& consumerId) 
{
    if (m_mapConsumedStatusInfo.contains(consumerId)) {
        m_mapConsumedStatusInfo.erase(consumerId);
    }
}

bool VideoProducerQualityController::isAllConsumerPaused()
{
    bool ret = true;
    m_mapConsumedStatusInfo.for_each2([&ret](const auto& pair) {
        const auto& consumeStatus = pair.second;
        if (!consumeStatus.m_paused) {
            ret = false;
            return true;
        }
        return false;
    });
    
    return ret;
}

int VideoProducerQualityController::getMaxDesiredQ() 
{
    auto ret = -1;
    m_mapConsumedStatusInfo.for_each([&ret](const auto& pair) {
        const auto& consumeStatus = pair.second;
        if (!consumeStatus.m_paused) {
            if (consumeStatus.m_layer > ret) {
                ret = consumeStatus.m_layer;
            }
        }
    });
    
    return ret;
}
