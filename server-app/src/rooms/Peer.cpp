/***************************************************************************
 *
 * Project:   ______                ______ _
 *           / _____)              / _____) |          _
 *          | /      ____ ____ ___| /     | | _   ____| |_
 *          | |     / _  |  _ (___) |     | || \ / _  |  _)
 *          | \____( ( | | | | |  | \_____| | | ( ( | | |__
 *           \______)_||_|_| |_|   \______)_| |_|\_||_|\___)
 *
 *
 * Copyright 2020-present, Leonid Stryzhevskyi <lganzzzo@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************************/

#include "Peer.hpp"
#include <fcntl.h>

#if defined(WIN32) || defined(_WIN32)
#include <io.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "Room.hpp"
#include "oatpp/network/tcp/Connection.hpp"
#include "oatpp/encoding/Base64.hpp"
#include "oatpp/network/tcp/Connection.hpp"
#include "oatpp/core/utils/ConversionUtils.hpp"
#include "utils/Message.hpp"
#include "srv_logger.h"
#include "consumer_controller.h"
#include "producer_controller.h"
#include "data_consumer_controller.h"
#include "data_producer_controller.h"

namespace
{
    class SendMessageCoroutine : public oatpp::async::Coroutine<SendMessageCoroutine>
    {
    public:
        SendMessageCoroutine(oatpp::async::Lock* lock,
                             const std::shared_ptr<oatpp::websocket::AsyncWebSocket>& websocket,
                             const oatpp::String& message)
        : _lock(lock)
        , _websocket(websocket)
        , _message(message)
        {}

        Action act() override {
            return oatpp::async::synchronize(_lock, _websocket->sendOneFrameTextAsync(_message)).next(finish());
        }
        
    private:
        oatpp::async::Lock* _lock;
        std::shared_ptr<oatpp::websocket::AsyncWebSocket> _websocket;
        oatpp::String _message;
    };

    class SendPingCoroutine : public oatpp::async::Coroutine<SendPingCoroutine> {
    public:

        SendPingCoroutine(oatpp::async::Lock* lock, const std::shared_ptr<oatpp::websocket::AsyncWebSocket>& websocket)
        : _lock(lock)
        , _websocket(websocket) {}

        Action act() override {
            return oatpp::async::synchronize(_lock, _websocket->sendPingAsync(nullptr)).next(finish());
        }
        
    private:
        oatpp::async::Lock* _lock;
        std::shared_ptr<oatpp::websocket::AsyncWebSocket> _websocket;
    };

    class SendErrorCoroutine : public oatpp::async::Coroutine<SendErrorCoroutine>
    {
    public:
        SendErrorCoroutine(oatpp::async::Lock* lock,
                           const std::shared_ptr<oatpp::websocket::AsyncWebSocket>& websocket,
                           const oatpp::String& message)
        : _lock(lock)
        , _websocket(websocket)
        , _message(message) {}

        Action act() override
        {
            /* synchronized async pipeline */
            return oatpp::async::synchronize(
                       /* Async write-lock to prevent concurrent writes to socket */
                       _lock,
                       /* send error message, then close-frame */
                       std::move(_websocket->sendOneFrameTextAsync(_message).next(_websocket->sendCloseAsync()))
                       ).next(
                    /* async error after error message and close-frame are sent */
                    new oatpp::async::Error("API Error")
                    );
        }
        
    private:
        oatpp::async::Lock* _lock;
        std::shared_ptr<oatpp::websocket::AsyncWebSocket> _websocket;
        oatpp::String _message;
    };

    class RequestCoroutine : public oatpp::async::Coroutine<RequestCoroutine>
    {
    public:
        RequestCoroutine(const std::shared_ptr<Peer>& peer,
                         oatpp::async::Lock* lock,
                         const std::shared_ptr<oatpp::websocket::AsyncWebSocket>& websocket,
                         const nlohmann::json& message)
        : _peer(peer)
        , _lock(lock)
        , _websocket(websocket)
        , _message(message) {}
        
        Action act() override {
            auto sentMsg = oatpp::String(_message.dump().c_str());
            _peer->_sents[_message["id"].get<int>()] = _message;
            return oatpp::async::synchronize(_lock, _websocket->sendOneFrameTextAsync(sentMsg)).next(yieldTo(&RequestCoroutine::waitResponse));
        }
        
        Action waitResponse() {
            auto messageId = _message["id"].get<int>();
            auto method = _message["method"].get<std::string>();
            if (method == "newConsumer") {
                auto data = _message["data"];
                auto consumerId = data["id"].get<std::string>();
                return _peer->checkResponseAsync(messageId, method, consumerId, finish());
            }
            else {
                return _peer->checkResponseAsync(messageId, "", "", finish());
            }
        }
        
    private:
        std::shared_ptr<Peer> _peer;
        oatpp::async::Lock* _lock;
        std::shared_ptr<oatpp::websocket::AsyncWebSocket> _websocket;
        nlohmann::json _message;
    };

    class NotifyCoroutine : public oatpp::async::Coroutine<NotifyCoroutine>
    {
    public:
        NotifyCoroutine(oatpp::async::Lock* lock,
                        const std::shared_ptr<oatpp::websocket::AsyncWebSocket>& websocket,
                        const oatpp::String& message)
        : _lock(lock)
        , _websocket(websocket)
        , _message(message) {}
        
        Action act() override {
            return oatpp::async::synchronize(_lock, _websocket->sendOneFrameTextAsync(_message)).next(finish());
        }
        
    private:
        oatpp::async::Lock* _lock;
        std::shared_ptr<oatpp::websocket::AsyncWebSocket> _websocket;
        oatpp::String _message;
    };
}

Peer::Peer(const std::shared_ptr<AsyncWebSocket>& socket, const std::string& roomId, const std::string& peerId)
: _data(std::make_shared<PeerData>())
, _socket(socket)
, _roomId(roomId)
, _id(peerId)
, _pingPoingCounter(0)
{
    SRV_LOGD("Peer()");
}

Peer::~Peer()
{
    SRV_LOGD("~Peer(), id: %s", _id.c_str());
}

void Peer::init()
{
    _accept = [wself = std::weak_ptr<Peer>(shared_from_this())](const nlohmann::json& request, const nlohmann::json& data){
        auto self = wself.lock();
        if (!self) {
            return;
        }
        self->accept(request, data);
    };
    
    _reject = [wself = std::weak_ptr<Peer>(shared_from_this())](const nlohmann::json& request, int errorCode, const std::string& errorReason){
        auto self = wself.lock();
        if (!self) {
            return;
        }
        self->reject(request, errorCode, errorReason);
    };
}

void Peer::destroy()
{
    
}

void Peer::sendMessageAsync(const nlohmann::json& message)
{
    if (_socket) {
        _asyncExecutor->execute<SendMessageCoroutine>(&_writeLock, _socket, oatpp::String(message.dump().c_str()));
    }
}

bool Peer::sendPingAsync()
{
    /******************************************************
     *
     * Ping counter is increased on sending ping
     * and decreased on receiving pong from the client.
     *
     * If the server didn't receive pong from client
     * before the next ping,- then the client is
     * considered to be disconnected.
     *
     ******************************************************/

    ++_pingPoingCounter;

    if (_socket && _pingPoingCounter == 1) {
        _asyncExecutor->execute<SendPingCoroutine>(&_writeLock, _socket);
        return true;
    }

    return false;
}

oatpp::async::CoroutineStarter Peer::onApiError(const oatpp::String& errorMessage)
{
    auto message = MessageDto::createShared();
    message->code = MessageCodes::CODE_API_ERROR;
    message->message = errorMessage;

    return SendErrorCoroutine::start(&_writeLock, _socket, _objectMapper->writeToString(message));
}

oatpp::async::Action Peer::checkResponseAsync(int messageId, const std::string& method, const std::string& param, oatpp::async::Action&& nextAction)
{
    auto item = this->_sents.find(messageId);
    if (item != _sents.end()) {
        return oatpp::async::Action::createActionByType(oatpp::async::Action::TYPE_REPEAT);
    }
    else {
        if (method == "newConsumer" && param != "") {
            auto& controller = this->_data->consumerControllers[param];
            controller->resume();
        }
        return std::forward<oatpp::async::Action>(nextAction);
    }
}

void Peer::requestAsync(const std::string& method, const nlohmann::json& message)
{
    if (_socket) {
        auto request = Message::createRequest(method, message);
        _asyncExecutor->execute<RequestCoroutine>(shared_from_this(), &_writeLock, _socket, request);
    }
}

void Peer::notifyAsync(const std::string& method, const nlohmann::json& message)
{
    if (_socket) {
        auto notify = Message::createNotification(method, message);
        SRV_LOGD("[Room] [Peer] notifyAsync notify: %s", notify.dump(4).c_str());
        _asyncExecutor->execute<NotifyCoroutine>(&_writeLock, _socket, oatpp::String(notify.dump().c_str()));
    }
}

oatpp::async::CoroutineStarter Peer::handleMessage(const nlohmann::json& message)
{
    SRV_LOGD("[Peer] handleMessage message: %s", message.dump(4).c_str());
    
    if (message.contains("request")) {
        if (message["request"].is_boolean()) {
            this->handleRequest(message);
        }
    }
    else if (message.contains("response")){
        if (message["response"].is_boolean()) {
            this->handleResponse(message);
        }
    }
    else if (message.contains("notification")) {
        if (message["notification"].is_boolean()) {
            this->handleNotification(message);
        }
    }
    else {
        return onApiError("Invalid client message");
    }
    
    return nullptr;
}

void Peer::invalidateSocket()
{
    if (_socket) {
        auto connection = _socket->getConnection();
        auto c = std::static_pointer_cast<oatpp::network::tcp::Connection>(connection);
        oatpp::v_io_handle handle = c->getHandle();
#if defined(WIN32) || defined(_WIN32)
        shutdown(handle, SD_BOTH);
#else
        shutdown(handle, SHUT_RDWR);
#endif
    }
    _socket.reset();
}

oatpp::async::CoroutineStarter Peer::onPing(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message)
{
    return oatpp::async::synchronize(&_writeLock, socket->sendPongAsync(message));
}

oatpp::async::CoroutineStarter Peer::onPong(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message)
{
    --_pingPoingCounter;
    return nullptr; // do nothing
}

oatpp::async::CoroutineStarter Peer::onClose(const std::shared_ptr<AsyncWebSocket>& socket, v_uint16 code, const oatpp::String& message)
{
    SRV_LOGD("onClose()");
    
    //_closeSignal(_id);
    return nullptr; // do nothing
}

oatpp::async::CoroutineStarter Peer::readMessage(const std::shared_ptr<AsyncWebSocket>& socket, v_uint8 opcode, p_char8 data, oatpp::v_io_size size)
{
    if (_messageBuffer.getCurrentPosition() + size >  _appConfig->maxMessageSizeBytes) {
        return onApiError("Message size exceeds max allowed size.");
    }

    if (size == 0) { // message transfer finished
        auto wholeMessage = _messageBuffer.toString();
        _messageBuffer.setCurrentPosition(0);

        //oatpp::Object<MessageDto> message;
        //try {
        //    message = _objectMapper->readFromString<oatpp::Object<MessageDto>>(wholeMessage);
        //} catch (const std::runtime_error& e) {
        //    return onApiError("Can't parse message");
        //}
        //message->peerName = _nickname;
        //message->peerId = _id;
        //message->timestamp = oatpp::base::Environment::getMicroTickCount();

        nlohmann::json msg = Message::parse(wholeMessage->std_str());
        return handleMessage(msg);

    } else if (size > 0) { // message frame received
        _messageBuffer.writeSimple(data, size);
    }

    return nullptr; // do nothing
}

void Peer::handleRequest(const nlohmann::json& request)
{
    _requestSignal(shared_from_this(), request, _accept, _reject);
}

void Peer::handleResponse(const nlohmann::json& response)
{
    auto item = this->_sents.find(response["id"].get<int>());
    if (item == _sents.end()) {
        SRV_LOGE("[Peer] response id not found in map!");
        return;
    }
    
    if (response.contains("ok")) {
        if (response["ok"].get<bool>()) {
            this->_sents.erase(item);
        }
    }
    else {
        SRV_LOGW("error response!");
        this->_sents.erase(item);
    }
}

void Peer::handleNotification(const nlohmann::json& notification)
{
    _notificationSignal(notification);
}

void Peer::accept(const nlohmann::json& request, const nlohmann::json& data)
{
    auto response = Message::createSuccessResponse(request, data);
    SRV_LOGD("[Room] [Peer] handleRequest with accept response: %s", response.dump(4).c_str());
    sendMessageAsync(response);
}

void Peer::reject(const nlohmann::json& request, int errorCode, const std::string& errorReason)
{
    auto response = Message::createErrorResponse(request, errorCode, errorReason);
    SRV_LOGD("[Room] [Peer] handleRequest with reject response: %s", response.dump(4).c_str());
    sendMessageAsync(response);
}

void Peer::close()
{
    if (_closed) {
        return;
    }

    SRV_LOGD("close()");

    _closed = true;

    // Close Transport.
    // TODO:
    if (_socket) {
        auto connection = _socket->getConnection();
        auto c = std::static_pointer_cast<oatpp::network::tcp::Connection>(connection);
        c->close();
    }

    _sents.clear();

    // Emit 'close' event.
    _closeSignal(_id);
}
