/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "payload_channel.h"
#include <future>
#include <chrono>
#include "srv_logger.h"

namespace srv {
    
    using namespace std::chrono_literals;

    static const int32_t MESSAGE_MAX_LEN = 4194308;
    static const int32_t PAYLOAD_MAX_LEN = 4194304; // 4MB

    void PayloadChannel::payloadChannelReadFree(uint8_t* message, uint32_t messageLen, size_t messageCtx)
    {
        if (message) {
            delete[] message;
            message = nullptr;
        }
    }

    PayloadChannelReadFreeFn PayloadChannel::payloadChannelRead(uint8_t** message, uint32_t* messageLen, size_t* messageCtx,
                                                                uint8_t** payload, uint32_t* payloadLen, size_t* payloadCapacity,
                                                                const void* handle, PayloadChannelReadCtx ctx)
    {
        PayloadChannelReadFreeFn callback = nullptr;
        
        if (auto channel = static_cast<PayloadChannel*>(ctx)) {
            std::shared_ptr<Message> msg;
            if (channel->_requestQueue.try_dequeue(msg)) {
                *message = msg->message;
                *messageLen = msg->messageLen;
                *messageCtx = msg->messageCtx;
                
                *payload = msg->payload;
                *payloadLen = msg->payloadLen;
                *payloadCapacity = msg->payloadCapacity;
                
                callback = payloadChannelReadFree;
            }
            channel->setHandle((uv_async_t*)handle);
        }
        
        return callback;
    }

    void PayloadChannel::payloadChannelWrite(const uint8_t* message, uint32_t messageLen, const uint8_t* payload, uint32_t payloadLen, ChannelWriteCtx ctx)
    {
        auto channel = static_cast<PayloadChannel*>(ctx);
        if (channel && message && messageLen > 0) {
            std::string msg((char*)message, messageLen);
            channel->onMessage(msg, payload, payloadLen);
        }
    }

    PayloadChannel::PayloadChannel()
    {
        
    }

    PayloadChannel::~PayloadChannel()
    {
        clean();
    }

    void PayloadChannel::close()
    {
        SRV_LOGD("close()");
        
        _closed = true;
    }

    void PayloadChannel::notify(const std::string& event, const std::string& handlerId, const std::string& data, const uint8_t* payload, size_t payloadLen)
    {
        SRV_LOGD("notify() [event:%s]", event.c_str());
        
        if (_closed) {
            SRV_LOGD("PayloadChannel closed");
            return;
        }
        
        // `n:${event}:${handlerId}:${data}`;
        std::string notification;
        notification += "n:";
        notification += event;
        notification += ":";
        notification += handlerId;
        notification += ":";
        notification += data;

        if (notification.size() > MESSAGE_MAX_LEN) {
            SRV_LOGD("PayloadChannel request too big");
            return;
        } else if (payloadLen > PAYLOAD_MAX_LEN) {
            SRV_LOGD("PayloadChannel payload too big");
            return;
        }
        
        auto msg = std::make_shared<Message>();
        msg->messageLen = (uint32_t)notification.size();
        msg->message = new uint8_t[msg->messageLen  + 1];
        std::copy(notification.begin(), notification.end(), msg->message);
        msg->message[msg->messageLen] = '\0';
        
        auto _payload = new uint8_t[payloadLen];
        std::memcpy(_payload, payload, payloadLen);
        msg->payload = (uint8_t*)_payload;
        msg->payloadLen = (uint32_t)payloadLen;
        msg->payloadCapacity = 0;
        
        if (_requestQueue.try_enqueue(msg)) {
            notifyRead();
        } else {
            SRV_LOGD("Channel request enqueue failed");
        }
    }

    nlohmann::json PayloadChannel::request(const std::string& method, const std::string& handlerId, const std::string& data, const uint8_t* payload, size_t payloadLen)
    {
        if (_closed) {
            SRV_LOGD("PayloadChannel closed");
            return nlohmann::json();
        }
        
        uint8_t* _payload = nullptr;
        if (payloadLen > 0) {
            _payload = new uint8_t[payloadLen];
            std::memcpy(_payload, payload, payloadLen);
        }
        
        
        std::promise<nlohmann::json> promise;
        auto result = promise.get_future();
        
        {
            std::lock_guard<std::mutex> lock(_idMutex);
            _nextId < std::numeric_limits<uint32_t>::max() ? ++_nextId : (_nextId = 1);
        }
                  
        const uint32_t id = _nextId;
    
        auto callback = std::make_shared<Callback>(id, method,
        [wself = std::weak_ptr<PayloadChannel>(shared_from_this()), id, &promise](const nlohmann::json& data) {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (self->removeCallback(id)) {
                promise.set_value(data);
            }
        },
        [wself = std::weak_ptr<PayloadChannel>(shared_from_this()), id, &promise](const IError& error) {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (self->removeCallback(id)) {
                promise.set_exception(std::make_exception_ptr(ChannelError(error.message().c_str())));
            }
        },
        [wself = std::weak_ptr<PayloadChannel>(shared_from_this()), id, &promise]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            if (self->removeCallback(id)) {
                promise.set_exception(std::make_exception_ptr(ChannelError("callback was closed")));
            }
        },
        [wself = std::weak_ptr<PayloadChannel>(shared_from_this()), id, &promise]() {
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
        
        // "r:${id}:${method}:${handlerId}:${JSON.stringify(data)}"
        std::string req;
        req += "r:";
        req += std::to_string(_nextId);
        req += ":";
        req += method;
        req += ":";
        req += handlerId;
        req += ":";
        req += data;

        if (req.size() > MESSAGE_MAX_LEN) {
            SRV_LOGD("PayloadChannel request too big");
            return nlohmann::json();
        } else if (payloadLen > PAYLOAD_MAX_LEN) {
            SRV_LOGD("PayloadChannel payload too big");
            return nlohmann::json();
        }
        
        auto msg = std::make_shared<Message>();
        msg->messageLen = (uint32_t)req.size();
        msg->message = new uint8_t[msg->messageLen  + 1];
        std::copy(req.begin(), req.end(), msg->message);
        msg->message[msg->messageLen] = '\0';
        
        msg->payload = (uint8_t*)payload;
        msg->payloadLen = (uint32_t)payloadLen;
        msg->payloadCapacity = 0;
        
        if (_requestQueue.try_enqueue(msg)) {
            notifyRead();
        } else {
            SRV_LOGD("Channel request enqueue failed");
        }
        
        return result.get();
    }

    void PayloadChannel::notifyRead()
    {
        if (_handle) {
            uv_async_send(_handle);
        }
    }

    void PayloadChannel::onMessage(const std::string& msg, const uint8_t* payload, uint32_t payloadLen)
    {
        uint8_t* _payload = nullptr;
        if (payloadLen > 0) {
            _payload = new uint8_t[payloadLen];
            std::memcpy(_payload, payload, payloadLen);
        }
        asio::post(_threadPool.get_executor(), [wself = std::weak_ptr<PayloadChannel>(shared_from_this()), msg, _payload, payloadLen]() {
            auto self = wself.lock();
            if (!self) {
                delete[] _payload;
                return;
            }
            self->onMessageImpl(msg, _payload, payloadLen);
        });
    }

    void PayloadChannel::onMessageImpl(const std::string& msg, const uint8_t* payload, uint32_t payloadLen)
    {
        try {
            nlohmann::json jsonMessage = nlohmann::json::parse(msg);
            processMessage(jsonMessage, payload, payloadLen);
        }
        catch (...) {
            SRV_LOGE("received invalid message from the worker process: %s", msg.c_str());
        }
    }

    void PayloadChannel::processMessage(const nlohmann::json& msg, const uint8_t* payload, uint32_t payloadLen)
    {
        if (msg.contains("id")) {
            auto it = _callbackMap.find(msg["id"]);
            if (it == _callbackMap.end()) {
                SRV_LOGE("received response does not match any sent request [id:%s]", msg["id"].dump().c_str());
                return;
            }
            auto callback = it->second;

            if (msg.contains("accepted")) {
                SRV_LOGD("request succeeded [method:%s, id:%u]", callback->method().c_str(), callback->id());
                
                if (msg.contains("data")) {
                    callback->resolve(msg["data"]);
                }
                else {
                    nlohmann::json data = nlohmann::json::object();
                    callback->resolve(data);
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
                _notificationSignal(msg["targetId"], msg["event"], msg["data"].dump(), payload, payloadLen);
            }
            else{
                _notificationSignal(msg["targetId"], msg["event"], "", payload, payloadLen);
            }
        }
        else {
            SRV_LOGE("received message is not a response nor a notification");
        }
    }

    bool PayloadChannel::removeCallback(uint32_t id)
    {
        std::lock_guard<std::mutex> lock(_callbackMutex);
        if (_callbackMap.find(id) != _callbackMap.end()) {
            _callbackMap.erase(id);
            return true;
        }
        return false;
    }

    void PayloadChannel::clean()
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
