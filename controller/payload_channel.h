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

namespace srv {
    
    class PayloadChannel : public std::enable_shared_from_this<PayloadChannel>
    {
    public:
        struct Message {
            uint8_t* message;
            uint32_t messageLen;
            size_t messageCtx;
            uint8_t* payload;
            uint32_t payloadLen;
            size_t payloadCapacity;
        };
        
    public:
        PayloadChannel();

        ~PayloadChannel();

        void notify(const std::string& event, const std::string& handlerId, const std::string& data, const uint8_t* payload, size_t payloadLen);
        
        nlohmann::json request(const std::string& method, const std::string& handlerId, const std::string& data, const uint8_t* payload, size_t payloadLen);
        
        void close();

    public:
        static void payloadChannelReadFree(uint8_t* message, uint32_t messageLen, size_t messageCtx);

        static PayloadChannelReadFreeFn payloadChannelRead(uint8_t** message, uint32_t* messageLen, size_t* messageCtx,
                                                           uint8_t** payload, uint32_t* payloadLen, size_t* payloadCapacity,
                                                           const void* handle, PayloadChannelReadCtx ctx);

        static void payloadChannelWrite(const uint8_t* message, uint32_t messageLen, const uint8_t* payload,
                                        uint32_t payloadLen, ChannelWriteCtx ctx);
        
    private:
        void setHandle(uv_async_t* handle) { _handle = handle; }
        
        uv_async_t* handle() { return _handle; }
        
        void notifyRead();
        
        void onMessage(const std::string& msg, const uint8_t* payload, uint32_t payloadLen);
        
        void onMessageImpl(const std::string& msg, const uint8_t* payload, uint32_t payloadLen);
        
        void processMessage(const nlohmann::json& msg, const uint8_t* payload, uint32_t payloadLen);
        
        bool removeCallback(uint32_t id);
        
        void clean();
        
    public:
        // target id, event, data(json | string), payload, payloadlen
        sigslot::signal<const std::string&, const std::string&, const std::string&, const uint8_t*, size_t> notificationSignal;
        
    public:
        asio::static_thread_pool _threadPool {1};
        
        std::mutex _callbackMutex;
        std::unordered_map<uint64_t, std::shared_ptr<Callback>> _callbackMap;
        
        std::mutex _idMutex;
        uint32_t _nextId { 0 };
        
        std::atomic_bool _closed { false };
        
        moodycamel::ConcurrentQueue<std::shared_ptr<Message>> _requestQueue;
        
        uv_async_t* _handle = nullptr;
    };
    
}
