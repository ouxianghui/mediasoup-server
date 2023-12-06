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
            // NOTE: Because the Worker used builder.FinishSizePrefixed(), we need to add sizeof(uint32_t) to skip the prefix
            std::vector<uint8_t> msg(message + sizeof(uint32_t), message + messageLen);
            channel->onMessage(msg);
        }
    }

    Channel::Channel()
    {
        SRV_LOGD("Channel()");
    }

    Channel::Channel(int consumerFd, int producerFd)
    : _channelSocket(std::make_shared<ChannelSocket>(consumerFd, producerFd))
    {
        _channelSocket->SetListener(this);
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

    void Channel::notifyRead()
    {
        if (_handle) {
            uv_async_send(_handle);
        }
    }

    void Channel::onMessage(const std::vector<uint8_t>& message)
    {
        asio::post(_threadPool.get_executor(), [wself = std::weak_ptr<Channel>(shared_from_this()), message]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            self->processMessage(message);
        });
    }

    void Channel::processMessage(const std::vector<uint8_t>& msg)
    {
        try {
            const auto* message = FBS::Message::GetMessage(&msg[0]);
            
            // We can receive JSON messages (Channel messages) or log strings.
            switch (message->data_type()) {
                case FBS::Message::Body::Response: {
                    auto response = message->data_as<FBS::Response::Response>();
                    processResponse(response, msg);
                    break;
                }
                case FBS::Message::Body::Notification: {
                    auto notification = message->data_as<FBS::Notification::Notification>();
                    processNotification(notification, msg);
                    break;
                }
                case FBS::Message::Body::Log: {
                    auto log = message->data_as<FBS::Log::Log>();
                    processLog(0, log);
                    break;
                }
                default:
                    break;
            }
        }
        catch (std::exception& ex) {
            SRV_LOGE("received invalid message from the worker process: %s", ex.what());
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

    void Channel::notify(FBS::Notification::Event event, const std::string& handlerId)
    {
        flatbuffers::Offset<void> bodyOffset;
        this->notify(event, FBS::Notification::Body::NONE, bodyOffset, handlerId);
    }

    std::vector<uint8_t> Channel::request(FBS::Request::Method method, const std::string& handlerId)
    {
        flatbuffers::Offset<void> bodyOffset;
        return this->request(method, FBS::Request::Body::NONE, bodyOffset, handlerId);
    }

    void Channel::processResponse(const FBS::Response::Response* response, const std::vector<uint8_t>& data)
    {
        assert(response);
        
        std::shared_ptr<Callback> callback;
        {
            std::lock_guard<std::mutex> lock(_callbackMutex);
            auto it = _callbackMap.find(response->id());
            if (it == _callbackMap.end()) {
                SRV_LOGE("received response does not match any sent request [id:%u]", response->id());
                return;
            }
            callback = it->second;
        }
        
        if (response->accepted()) {
            SRV_LOGD("request succeeded [method:%u, id:%u]", (uint8_t)callback->method(), callback->id());
            callback->resolve(data);
        }
        else if (response->error()) {
            SRV_LOGW("request failed [method:%u, id:%u]: %s", (uint8_t)callback->method(), callback->id(), response->reason()->c_str());

            if (response->error()->str() == "TypeError") {
                callback->reject(Error("TypeError", response->reason()->str()));
            }
            else {
                callback->reject(Error("Error", response->reason()->str()));
            }
        }
        else {
            SRV_LOGE("received response is not accepted nor rejected [method:%u, id:%u]", (uint8_t)callback->method(), callback->id());
        }
    }

    void Channel::processNotification(const FBS::Notification::Notification* notification, const std::vector<uint8_t>& data)
    {
        // Due to how Promises work, it may happen that we receive a response
        // from the worker followed by a notification from the worker. If we
        // emit the notification immediately it may reach its target **before**
        // the response, destroying the ordered delivery. So we must wait a bit
        // here.
        // See https://github.com/versatica/mediasoup/issues/510
        
        this->notificationSignal(notification->handlerId()->str(), notification->event(), data);
    }

    void Channel::processLog(int32_t pid, const FBS::Log::Log* log)
    {
        auto logData = log->data();
        
        switch(logData->data()[0]) {
                // 68 = 'D' (a debug log).
            case 68:
                SRV_LOGD("worker:%ull %s", pid, logData->c_str());
                break;
                
                // 87 = 'W' (a warn log).
            case 87:
                SRV_LOGW("worker:%ull %s", pid, logData->c_str());
                break;
                
                // 69 = 'E' (an error log).
            case 69:
                SRV_LOGE("worker:%ull %s", pid, logData->c_str());
                break;
                
                // 88 = 'X' (a dump log).
            case 88:
                // eslint-disable-next-line no-console
                SRV_LOGD("worker:%ull %s", pid, logData->c_str());
                break;
                
            default:
                // eslint-disable-next-line no-console
                SRV_LOGW("worker:%ull unexpected data:%s", pid, logData->c_str());
        }
    }

    void Channel::OnChannelMessage(char* msg, size_t msgLen)
    {
        std::vector<uint8_t> message(msg, msg + msgLen);
        asio::post(_threadPool.get_executor(), [wself = std::weak_ptr<Channel>(shared_from_this()), message]() {
            auto self = wself.lock();
            if (!self) {
                return;
            }
            self->processMessage(message);
        });
    }

    void Channel::OnChannelClosed(ChannelSocket* channel)
    {
        SRV_LOGD("OnChannelClosed()");
        
        close();
    }

}
