/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <string>
#include <atomic>
#include "srv_logger.h"
#include "flatbuffers/flexbuffers.h"
#include "FBS/request.h"
#include "FBS/response.h"
#include "FBS/message.h"
#include "FBS/notification.h"

namespace srv {

    static const int32_t MESSAGE_MAX_LEN = 4194308;
    static const int32_t PAYLOAD_MAX_LEN = 4194304; // 4MB

    class MessageBuilder
    {
    public:
        static std::vector<uint8_t> createNotification(flatbuffers::FlatBufferBuilder& builder, const std::string& handlerId, FBS::Notification::Event event);
        
        template<typename T>
        static std::vector<uint8_t> createNotification(flatbuffers::FlatBufferBuilder& builder,
                                                       const std::string& handlerId,
                                                       FBS::Notification::Event event,
                                                       FBS::Notification::Body bodyType,
                                                       flatbuffers::Offset<T>& bodyOffset);
        
        static std::vector<uint8_t> createRequest(flatbuffers::FlatBufferBuilder& builder, uint32_t requestId, const std::string& handlerId, FBS::Request::Method method);
        
        template<typename T>
        static std::vector<uint8_t> createRequest(flatbuffers::FlatBufferBuilder& builder,
                                                  uint32_t requestId,
                                                  const std::string& handlerId,
                                                  FBS::Request::Method method,
                                                  FBS::Request::Body bodyType,
                                                  flatbuffers::Offset<T>& bodyOffset);
        
        static void setSizePrefix(bool hasSizePrefix) { _hasSizePrefix = hasSizePrefix; }
        
    private:
        static std::atomic_bool _hasSizePrefix;
    };

    template<typename T>
    std::vector<uint8_t> MessageBuilder::createNotification(flatbuffers::FlatBufferBuilder& builder, 
                                                            const std::string& handlerId,
                                                            FBS::Notification::Event event,
                                                            FBS::Notification::Body bodyType,
                                                            flatbuffers::Offset<T>& bodyOffset)
    {
        SRV_LOGD("createNotification() [event:%u]", (uint8_t)event);
        
        auto nfOffset = FBS::Notification::CreateNotificationDirect(builder, handlerId.c_str(), event, bodyType, bodyOffset.Union());
        
        auto msgOffset = FBS::Message::CreateMessage(builder, FBS::Message::Body::Notification, nfOffset.Union());
        
        std::vector<uint8_t> buffer;
        
        if (_hasSizePrefix) {
            builder.FinishSizePrefixed(msgOffset);
        }
        else {
            builder.Finish(msgOffset);
        }
        
        if (builder.GetSize() > MESSAGE_MAX_LEN) {
            SRV_LOGE("Channel request too big");
            return {};
        }
        
        buffer = std::vector<uint8_t>(builder.GetBufferPointer(), builder.GetBufferPointer() + (uint32_t)builder.GetSize());
        
        builder.Clear();
        
        return buffer;
    }

    template<typename T>
    std::vector<uint8_t> MessageBuilder::createRequest(flatbuffers::FlatBufferBuilder& builder, 
                                                       uint32_t requestId,
                                                       const std::string& handlerId,
                                                       FBS::Request::Method method,
                                                       FBS::Request::Body bodyType,
                                                       flatbuffers::Offset<T>& bodyOffset)
    {
        SRV_LOGD("createRequest() [method:%d, id:%u]", (uint8_t)method, requestId);
            
        auto reqOffset = FBS::Request::CreateRequestDirect(builder, requestId, method, handlerId.c_str(), bodyType, bodyOffset.Union());
        
        auto msgOffset = FBS::Message::CreateMessage(builder, FBS::Message::Body::Request, reqOffset.Union());
        
        std::vector<uint8_t> buffer;
        
        if (_hasSizePrefix) {
            builder.FinishSizePrefixed(msgOffset);
        }
        else {
            builder.Finish(msgOffset);
        }
        
        if (builder.GetSize() > MESSAGE_MAX_LEN) {
            SRV_LOGE("Channel request too big");
            return {};
        }
        
        buffer = std::vector<uint8_t>(builder.GetBufferPointer(), builder.GetBufferPointer() + (uint32_t)builder.GetSize());
        
        builder.Clear();
        
        return buffer;
    }

}
