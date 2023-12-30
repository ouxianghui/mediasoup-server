/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "message_builder.h"

namespace srv {

    std::atomic_bool MessageBuilder::_hasSizePrefix = true;

    std::vector<uint8_t> MessageBuilder::createNotification(flatbuffers::FlatBufferBuilder& builder, const std::string& handlerId, FBS::Notification::Event event)
    {
        flatbuffers::Offset<void> bodyOffset;
        return createNotification(builder, handlerId, event, FBS::Notification::Body::NONE, bodyOffset);
    }

    std::vector<uint8_t> MessageBuilder::createRequest(flatbuffers::FlatBufferBuilder& builder, uint32_t requestId, const std::string& handlerId, FBS::Request::Method method)
    {
        flatbuffers::Offset<void> bodyOffset;
        return createRequest(builder, requestId, handlerId, method, FBS::Request::Body::NONE, bodyOffset);
    }

}
