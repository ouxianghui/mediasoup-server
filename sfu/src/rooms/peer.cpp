/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: RTC-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#include "peer.hpp"
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

#include "room.hpp"
#include "oatpp/network/tcp/Connection.hpp"
#include "oatpp/encoding/Base64.hpp"
#include "oatpp/network/tcp/Connection.hpp"
#include "oatpp/utils/Conversion.hpp"
#include "utils/message.hpp"
#include "srv_logger.h"
#include "interface/i_consumer_controller.h"
#include "interface/i_data_consumer_controller.h"
#include "interface/i_data_producer_controller.h"
#include "interface/i_producer_controller.h"
#include "interface/i_transport_controller.h"
#include "interface/i_router_controller.h"
#include "interface/i_rtp_observer_controller.h"
#include "interface/i_webrtc_server_controller.h"
#include "interface/i_worker_controller.h"
#include "video_producer_quality_controller.hpp"

namespace
{
    class SendMessageCoroutine : public oatpp::async::Coroutine<SendMessageCoroutine>
    {
    public:
        SendMessageCoroutine(oatpp::async::Lock* lock,
                             const std::shared_ptr<oatpp::websocket::AsyncWebSocket>& websocket,
                             const nlohmann::json& message,
                             Peer::MessageType type,
                             std::function<void()> complete)
        : _lock(lock)
        , _websocket(websocket)
        , _message(message)
        , _type(type)
        , _complete(complete) {}

        Action act() override {
            auto message = oatpp::String(_message.dump().c_str());
            
            if (_type == Peer::MessageType::RESPONSE || _type == Peer::MessageType::NOTIFICATION || _type == Peer::MessageType::REQUEST) {
                return oatpp::async::synchronize(_lock, _websocket->sendOneFrameTextAsync(message)).next(yieldTo(&SendMessageCoroutine::waitFinish));
            }
            
            return finish();
        }
        
        Action waitFinish() {
            if (_complete) {
                _complete();
            }
            return finish();
        }
        
    private:
        oatpp::async::Lock* _lock;
        std::shared_ptr<oatpp::websocket::AsyncWebSocket> _websocket;
        std::weak_ptr<Peer> _peer;
        nlohmann::json _message;
        Peer::MessageType _type;
        std::function<void()> _complete;
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
    SRV_LOGD("init()");
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
    SRV_LOGD("destroy()");
}

void Peer::sendMessage(const nlohmann::json& message, MessageType type)
{
    if (_socket) {
        _executing = true;
        _asyncExecutor->execute<SendMessageCoroutine>(&_writeLock, _socket, message, type, [wself = std::weak_ptr<Peer>(shared_from_this())](){
            auto self = wself.lock();
            if (!self) {
                return;
            }
            self->_executing = false;
            self->runOne();
        });
    }
}

void Peer::runOne()
{
    if (_executing) {
        return;
    }
    std::shared_ptr<Message> msg;
    if (_messageQueue.try_dequeue(msg)) {
        sendMessage(msg->data(), msg->type());
    }
}

bool Peer::sendPing()
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

void Peer::request(const std::string& method, const nlohmann::json& message)
{
    if (_socket) {
        auto request = ::Message::createRequest(method, message);
        SRV_LOGD("[Room] [Peer] request: %s", request.dump(4).c_str());
        auto id = request["id"].get<int64_t>();
        auto msg = std::make_shared<Message>(id, MessageType::REQUEST, request);
        _messageQueue.enqueue(msg);
        _requestMap[id] = msg;
        if (!_executing) {
            runOne();
        }
    }
}

void Peer::notify(const std::string& method, const nlohmann::json& message)
{
    if (_socket) {
        auto notification = ::Message::createNotification(method, message);
        SRV_LOGD("[Room] [Peer] notification: %s", notification.dump(4).c_str());
        
        auto msg = std::make_shared<Message>(0,  MessageType::NOTIFICATION, notification);
        _messageQueue.enqueue(msg);
        
        if (!_executing) {
            runOne();
        }
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
        auto c = std::static_pointer_cast<oatpp::network::tcp::Connection>(connection.object);
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
    
    //this->closeSignal(_id);
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

        nlohmann::json msg = ::Message::parse(wholeMessage->c_str());
        return handleMessage(msg);

    } else if (size > 0) { // message frame received
        _messageBuffer.writeSimple(data, size);
    }

    return nullptr; // do nothing
}

void Peer::handleRequest(const nlohmann::json& request)
{
    this->requestSignal(shared_from_this(), request, _accept, _reject);
}

void Peer::handleResponse(const nlohmann::json& response)
{
    auto item = this->_requestMap.find(response["id"].get<int64_t>());
    if (item == _requestMap.end()) {
        SRV_LOGE("[Peer] response id not found in map!");
        return;
    }
    
    auto msg = item->second;
    auto data = msg->data();
    SRV_LOGE("data: %s", data.dump().c_str());
    if (data.contains("method") && data["method"].get<std::string>() == "newConsumer" && data.contains("data")) {
        auto consumerId = data["data"]["id"].get<std::string>();
        if (!consumerId.empty() && this->_data->consumerControllers.contains(consumerId)) {
            if (auto& controller = this->_data->consumerControllers[consumerId]) {
                controller->resume();
                if (controller->kind() == "video") {
                    this->newConsumerResumedSignal(controller);
                }
            }
        }
    }

    this->_requestMap.erase(item);
}

void Peer::handleNotification(const nlohmann::json& notification)
{
    this->notificationSignal(notification);
}

void Peer::accept(const nlohmann::json& request, const nlohmann::json& data)
{
    auto response = ::Message::createSuccessResponse(request, data);
    SRV_LOGD("[Room] [Peer] handleRequest with accept response: %s", response.dump(4).c_str());
    
    int64_t id = response["id"].get<int64_t>();
    auto msg = std::make_shared<Message>(id, MessageType::RESPONSE, response);
    _messageQueue.enqueue(msg);
    
    if (!_executing) {
        runOne();
    }
}

void Peer::reject(const nlohmann::json& request, int errorCode, const std::string& errorReason)
{
    auto response = ::Message::createErrorResponse(request, errorCode, errorReason);
    SRV_LOGD("[Room] [Peer] handleRequest with reject response: %s", response.dump(4).c_str());
    
    auto msg = std::make_shared<Message>(response["id"].get<int64_t>(), MessageType::RESPONSE, response);
    _messageQueue.enqueue(msg);
    
    if (!_executing) {
        runOne();
    }
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
        auto c = std::static_pointer_cast<oatpp::network::tcp::Connection>(connection.object);
        c->close();
    }

    _requestMap.clear();

    // Emit 'close' event.
    this->closeSignal(_id);
}
