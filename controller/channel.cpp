/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "channel.h"
#include <future>
#include <chrono>
#include "srv_logger.h"

namespace srv {
    
    using namespace std::chrono_literals;

    static const int32_t MESSAGE_MAX_LEN = 4194308;
    static const int32_t PAYLOAD_MAX_LEN = 4194304; // 4MB

    void Channel::channelReadFree(uint8_t* message, uint32_t messageLen, size_t messageCtx)
    {
        if (message) {
            delete[] message;
            message = nullptr;
        }
    }

    ChannelReadFreeFn Channel::channelRead(uint8_t** message, uint32_t* messageLen, size_t* messageCtx, const void* handle, ChannelReadCtx ctx)
    {
        ChannelReadFreeFn callback = nullptr;
        
        if (auto channel = static_cast<Channel*>(ctx)) {
            std::shared_ptr<Message> msg;
            if (channel->_requestQueue.try_dequeue(msg)) {
                *message = msg->message;
                *messageLen = msg->messageLen;
                *messageCtx = msg->messageCtx;
                callback = channelReadFree;
            }
            channel->setHandle((uv_async_t*)handle);
        }
        
        return callback;
    }

    void Channel::channelWrite(const uint8_t* message, uint32_t messageLen, ChannelWriteCtx ctx)
    {
        auto channel = static_cast<Channel*>(ctx);
        if (channel && message && messageLen > 0) {
            std::string msg((char*)message, messageLen);
            channel->onMessage(msg);
        }
    }

    Channel::Channel()
    {
        SRV_LOGD("Channel()");
    }

    Channel::~Channel()
    {
        SRV_LOGD("~Channel()");
        clean();
    }

    void Channel::close()
    {
        SRV_LOGD("close()");
        
        _closed = true;
    }

    nlohmann::json Channel::request(const std::string& method, const std::string& handlerId, const std::string& data)
    {
        if (_closed) {
            SRV_LOGD("Channel closed");
            return nlohmann::json();
        }
        
        std::promise<nlohmann::json> promise;
        auto result = promise.get_future();
        
        {
            std::lock_guard<std::mutex> lock(_idMutex);
            _nextId < std::numeric_limits<uint32_t>::max() ? ++_nextId : (_nextId = 1);
        }
                  
        const uint32_t id = _nextId;
        
        auto callback = std::make_shared<Callback>(id, method,
        [wself = std::weak_ptr<Channel>(shared_from_this()), id, &promise](const nlohmann::json& data) {
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
        
        SRV_LOGD("request() [method:%s, id:%u]", method.c_str(), id);
        
        // "${id}:${method}:${handlerId}:${JSON.stringify(data)}"
        std::string req;
        req += std::to_string(_nextId);
        req += ":";
        req += method;
        req += ":";
        req += handlerId;
        req += ":";
        req += data;

        if (req.size() > MESSAGE_MAX_LEN) {
            SRV_LOGD("Channel request too big");
            return nlohmann::json();
        }
        
        auto msg = std::make_shared<Message>();
        msg->messageLen = (uint32_t)req.size();
        msg->message = new uint8_t[msg->messageLen  + 1];
        std::copy(req.begin(), req.end(), msg->message);
        msg->message[msg->messageLen] = '\0';
        
        if (_requestQueue.try_enqueue(msg)) {
            notifyRead();
        } else {
            SRV_LOGD("Channel request enqueue failed");
        }
        
        return result.get();
    }

    void Channel::notifyRead()
    {
        if (_handle) {
            uv_async_send(_handle);
        }
    }

    void Channel::onMessage(const std::string& msg)
    {
        asio::post(_threadPool.get_executor(), [wself = std::weak_ptr<Channel>(shared_from_this()), msg]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            self->onMessageImpl(msg);
        });
    }

    void Channel::onMessageImpl(const std::string& msg)
    {
        try {
            // We can receive JSON messages (Channel messages) or log strings.
            switch (msg[0]) {
                // 123 = '{' (a Channel JSON message).
                case 123: {
                    nlohmann::json jsonMessage = nlohmann::json::parse(msg);
                    processMessage(jsonMessage);
                    break;
                }
                // 68 = 'D' (a debug log).
                case 68:
                    SRV_LOGD("worker:%ull %s", 0, msg.c_str());
                    break;

                // 87 = 'W' (a warn log).
                case 87:
                    SRV_LOGW("worker:%ull %s", 0, msg.c_str());
                    break;

                // 69 = 'E' (an error log).
                case 69:
                    SRV_LOGE("worker:%ull %s", 0, msg.c_str());
                    break;

                // 88 = 'X' (a dump log).
                case 88:
                    // eslint-disable-next-line no-console
                    SRV_LOGD("worker:%ull %s", 0, msg.c_str());
                    break;

                default:
                    // eslint-disable-next-line no-console
                    SRV_LOGW("worker:%ull unexpected data:%s", 0, msg.c_str());
            }
        }
        catch (std::exception& ex) {
            SRV_LOGE("received invalid message from the worker process: %s", ex.what());
        }
    }

    void Channel::processMessage(const nlohmann::json& msg)
    {
        if (msg.contains("id")) {
            std::shared_ptr<Callback> callback;
            {
                std::lock_guard<std::mutex> lock(_callbackMutex);
                auto it = _callbackMap.find(msg["id"]);
                if (it == _callbackMap.end()) {
                    SRV_LOGE("received response does not match any sent request [id:%s]", msg["id"].dump().c_str());
                    return;
                }
                callback = it->second;
            }

            if (msg.contains("accepted")) {
                SRV_LOGD("request succeeded [method:%s, id:%u]", callback->method().c_str(), callback->id());
                
                if (msg.contains("data")) {
                    callback->resolve(msg["data"]);
                }
                else {
                    callback->resolve(nlohmann::json::object());
                }
            }
            else if (msg.contains("error")) {
                SRV_LOGW("request failed [method:%s, id:%u]: %s", callback->method().c_str(), callback->id(), msg["reason"].get<std::string>().c_str());

                if (msg["error"] == "TypeError") {
                    callback->reject(Error("TypeError", msg["reason"]));
                }
                else {
                    callback->reject(Error("Error", msg["reason"]));
                }
            }
            else {
                SRV_LOGE("received response is not accepted nor rejected [method:%s, id:%u]", callback->method().c_str(), callback->id());
            }
        }
        else if (msg.contains("targetId") && msg.contains("event")) {
            if (msg.contains("data")) {
                if (msg["targetId"].is_string()) {
                    this->notificationSignal(msg["targetId"], msg["event"], msg["data"].dump());
                }
                else if (msg["targetId"].is_number()) {
                    this->notificationSignal(std::to_string(msg["targetId"].get<uint64_t>()), msg["event"], msg["data"]);
                }
            }
            else{
                if (msg["targetId"].is_string()) {
                    this->notificationSignal(msg["targetId"], msg["event"], "");
                }
                else if (msg["targetId"].is_number()) {
                    this->notificationSignal(std::to_string(msg["targetId"].get<uint64_t>()), msg["event"], "");
                }
            }
        }
        else {
            SRV_LOGE("received message is not a response nor a notification");
        }
    }

    bool Channel::removeCallback(uint32_t id)
    {
        std::lock_guard<std::mutex> lock(_callbackMutex);
        if (_callbackMap.find(id) != _callbackMap.end()) {
            _callbackMap.erase(id);
            return true;
        }
        return false;
    }

    void Channel::clean()
    {
        {
            std::lock_guard<std::mutex> lock(_callbackMutex);
            for (const auto& item : _callbackMap) {
                item.second->close();
            }
        }
        
        std::shared_ptr<Message> msg;
        while (_requestQueue.try_dequeue(msg)) {
            delete[] msg->message;
            msg->message = nullptr;
        }
    }
}
