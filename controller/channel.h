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
#include "threadsafe_unordered_map.hpp"
#include <atomic>
#include "sigslot/signal.hpp"
#include "types.h"
#include "moodycamel/concurrentqueue.h"
#include "channel_socket.h"
#include "nlohmann/json.hpp"
#include "common.hpp"
#include "uv.h"
#include "FBS/notification.h"
#include "FBS/log.h"

namespace srv {

    class Channel : public ChannelSocket::Listener, public std::enable_shared_from_this<Channel>
    {
    public:
        struct Message {
            uint8_t* message;
            uint32_t messageLen;
            size_t messageCtx;
        };
        
    public:
        // direct callback
        Channel();
        
        // pipe
        Channel(int consumerFd, int producerFd);
        
        ~Channel();
        
        uint32_t genRequestId();
        
        void notify(const std::vector<uint8_t>& data);
        
        std::vector<uint8_t> request(uint32_t requestId, const std::vector<uint8_t>& data);
        
        void close();
        
    public:
        static void channelReadFree(uint8_t* message, uint32_t messageLen, size_t messageCtx);

        static ChannelReadFreeFn channelRead(uint8_t** message, uint32_t* messageLen, size_t* messageCtx, const void* handle, ChannelReadCtx ctx);

        static void channelWrite(const uint8_t* message, uint32_t messageLen, ChannelWriteCtx ctx);
        
    private:
        // ChannelSocket::Listener
        void OnChannelMessage(char* msg, size_t msgLen) override;
        
        void OnChannelClosed(ChannelSocket* channel) override;
        
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
        asio::static_thread_pool _threadPool { 1 };
        
        asio::static_thread_pool _timerThread { 1 };
        
        std::threadsafe_unordered_map<uint64_t, std::shared_ptr<Callback>> _callbackMap;
        
        std::atomic<uint32_t> _nextId { 0 };
        
        std::atomic_bool _closed { false };
        
        moodycamel::ConcurrentQueue<std::shared_ptr<Message>> _requestQueue;
        
        uv_async_t* _handle = nullptr;
        
        std::shared_ptr<ChannelSocket> _channelSocket;
    };
}
