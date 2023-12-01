/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <memory>
#include <functional>
#include <unordered_map>
#include <atomic>
#include "sigslot/signal.hpp"
#include "types.h"
#include "moodycamel/concurrentqueue.h"
#include "nlohmann/json.hpp"
#include "common.hpp"
#include "uv.h"
#include "flatbuffers/flexbuffers.h"
#include "FBS/request.h"
#include "FBS/response.h"
#include "FBS/message.h"
#include "FBS/notification.h"


namespace srv {
    
    static const int32_t MESSAGE_MAX_LEN = 4194308;
    static const int32_t PAYLOAD_MAX_LEN = 4194304; // 4MB

    class Channel : public std::enable_shared_from_this<Channel>
    {
    public:
        struct Message {
            uint8_t* message;
            uint32_t messageLen;
            size_t messageCtx;
        };
        
    public:
        Channel();
        
        ~Channel();

        void notify(FBS::Notification::Event event, const std::string& handlerId);
        
        template<typename T>
        void notify(FBS::Notification::Event event,
                    FBS::Notification::Body bodyType,
                    flatbuffers::Offset<T>& bodyOffset,
                    const std::string& handlerId);
        
        std::vector<uint8_t> request(FBS::Request::Method method, const std::string& handlerId);
        
        template<typename T>
        std::vector<uint8_t> request(FBS::Request::Method method,
                                     FBS::Request::Body bodyType,
                                     flatbuffers::Offset<T>& bodyOffset,
                                     const std::string& handlerId);
        
        flatbuffers::FlatBufferBuilder& builder() { return _builder; }
        
        void close();
        
    public:
        static void channelReadFree(uint8_t* message, uint32_t messageLen, size_t messageCtx);

        static ChannelReadFreeFn channelRead(uint8_t** message, uint32_t* messageLen, size_t* messageCtx, const void* handle, ChannelReadCtx ctx);

        static void channelWrite(const uint8_t* message, uint32_t messageLen, ChannelWriteCtx ctx);
        
    private:
        void setHandle(uv_async_t* handle) { _handle = handle; }
        
        uv_async_t* handle() { return _handle; }
        
        void notifyRead();
        
        void onMessage(const std::vector<uint8_t>& msesage);
        
        void processMessage(const std::vector<uint8_t>& msesage);
        
        bool removeCallback(uint32_t id);
        
        void clean();
        
        void processResponse(const FBS::Response::Response* response, const std::vector<uint8_t>& data);

        void processNotification(const FBS::Notification::Notification* notification, const std::vector<uint8_t>& data);

        void processLog(int32_t pid, const FBS::Log::Log* log);

    public:
        // target id, event, notification)
        sigslot::signal<const std::string&, FBS::Notification::Event, const std::vector<uint8_t>&> notificationSignal;
        
    private:
        asio::static_thread_pool _threadPool {1};
        
        std::mutex _callbackMutex;
        std::unordered_map<uint64_t, std::shared_ptr<Callback>> _callbackMap;
        
        std::mutex _idMutex;
        uint32_t _nextId { 0 };
        
        std::atomic_bool _closed { false };
        
        moodycamel::ConcurrentQueue<std::shared_ptr<Message>> _requestQueue;
        
        uv_async_t* _handle = nullptr;
        
        std::mutex _builderMutex;
        flatbuffers::FlatBufferBuilder _builder {8192};
    };

    template<typename T>
    void Channel::notify(FBS::Notification::Event event,
                         FBS::Notification::Body bodyType,
                         flatbuffers::Offset<T>& bodyOffset,
                         const std::string& handlerId)
    {
        SRV_LOGD("notify() [event:%u]", (uint8_t)event);
        
        if (_closed) {
            SRV_LOGD("Channel closed");
            return;
        }
        
        {
            std::lock_guard<std::mutex> lock(_idMutex);
            
            flatbuffers::Offset<FBS::Notification::Notification> nfOffset;
            if (bodyType != FBS::Notification::Body::NONE) {
                nfOffset = FBS::Notification::CreateNotificationDirect(_builder, handlerId.c_str(), event, bodyType, bodyOffset.Union());
            }
            else {
                nfOffset = FBS::Notification::CreateNotificationDirect(_builder, handlerId.c_str(), event, FBS::Notification::Body::NONE, 0);
            }
            
            auto msgOffset = FBS::Message::CreateMessage(_builder, FBS::Message::Body::Notification, nfOffset.Union());
            
            _builder.Finish(msgOffset);
            
            if (_builder.GetSize() > MESSAGE_MAX_LEN) {
                SRV_LOGD("Channel request too big");
                return;
            }
            
            auto msg = std::make_shared<Message>();
            msg->messageLen = (uint32_t)_builder.GetSize();
            msg->message = new uint8_t[msg->messageLen];
            std::memcpy(msg->message, _builder.GetBufferPointer(), msg->messageLen);
            
            _builder.Clear();
            
            if (_requestQueue.try_enqueue(msg)) {
                notifyRead();
            }
            else {
                SRV_LOGD("Channel request enqueue failed");
            }
        }
    }

    template<typename T>
    std::vector<uint8_t> Channel::request(FBS::Request::Method method,
                                          FBS::Request::Body bodyType,
                                          flatbuffers::Offset<T>& bodyOffset,
                                          const std::string& handlerId)
    {
        if (_closed) {
            SRV_LOGD("Channel closed");
            return {};
        }
        
        std::promise<std::vector<uint8_t>> promise;
        auto result = promise.get_future();
        
        {
            std::lock_guard<std::mutex> lock(_idMutex);
            _nextId < std::numeric_limits<uint32_t>::max() ? ++_nextId : (_nextId = 1);
        }
                  
        const uint32_t id = _nextId;
        
        auto callback = std::make_shared<Callback>(id, method,
        [wself = std::weak_ptr<Channel>(shared_from_this()), id, &promise](const std::vector<uint8_t>& data) {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (self->removeCallback(id)) {
                promise.set_value(data);
            }
        },
        [wself = std::weak_ptr<Channel>(shared_from_this()), id, &promise](const IError& error) {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (self->removeCallback(id)) {
                promise.set_exception(std::make_exception_ptr(ChannelError(error.message().c_str())));
            }
        },
        [wself = std::weak_ptr<Channel>(shared_from_this()), id, &promise]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (self->removeCallback(id)) {
                promise.set_exception(std::make_exception_ptr(ChannelError("callback was closed")));
            }
        },
        [wself = std::weak_ptr<Channel>(shared_from_this()), id, &promise](){
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (self->removeCallback(id)) {
                promise.set_exception(std::make_exception_ptr(ChannelError("callback was timeout")));
            }
        });
        
        uint32_t duration = 1000 * (15 + (0.1 * _callbackMap.size()));
        callback->setTimeout(_threadPool, duration);
        
        {
            std::lock_guard<std::mutex> lock(_callbackMutex);
            _callbackMap[id] = callback;
        }
        
        SRV_LOGD("request() [method:%d, id:%u]", (uint8_t)method, id);
        
        {
            std::lock_guard<std::mutex> lock(_idMutex);
            
            flatbuffers::Offset<FBS::Request::Request> reqOffset;
            if (bodyType != FBS::Request::Body::NONE) {
                reqOffset = FBS::Request::CreateRequestDirect(_builder, _nextId, method, handlerId.c_str(), bodyType, bodyOffset.Union());
            }
            else {
                reqOffset = FBS::Request::CreateRequestDirect(_builder, _nextId, method, handlerId.c_str(), FBS::Request::Body::NONE, 0);
            }
            
            auto msgOffset = FBS::Message::CreateMessage(_builder, FBS::Message::Body::Request, reqOffset.Union());
            
            _builder.Finish(msgOffset);
            
            if (_builder.GetSize() > MESSAGE_MAX_LEN) {
                SRV_LOGD("Channel request too big");
                return {};
            }
            
            auto msg = std::make_shared<Message>();
            msg->messageLen = (uint32_t)_builder.GetSize();
            msg->message = new uint8_t[msg->messageLen];
            std::memcpy(msg->message, _builder.GetBufferPointer(), msg->messageLen);
            
            _builder.Clear();
            
            if (_requestQueue.try_enqueue(msg)) {
                notifyRead();
            }
            else {
                SRV_LOGD("Channel request enqueue failed");
            }
        }
        
        return result.get();
    }

}
