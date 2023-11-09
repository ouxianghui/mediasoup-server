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

#ifndef ASYNC_SERVER_ROOMS_PEER_HPP
#define ASYNC_SERVER_ROOMS_PEER_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include "dto/DTOs.hpp"
#include "dto/Config.hpp"
#include "utils/Statistics.hpp"
#include "oatpp-websocket/AsyncWebSocket.hpp"
#include "oatpp/network/ConnectionProvider.hpp"
#include "oatpp/core/async/Lock.hpp"
#include "oatpp/core/async/Executor.hpp"
#include "oatpp/core/data/mapping/ObjectMapper.hpp"
#include "oatpp/core/macro/component.hpp"
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "moodycamel/concurrentqueue.h"
#include "types.h"

using AcceptFunc = std::function<void(const nlohmann::json& request, const nlohmann::json& data)>;
using RejectFunc = std::function<void(const nlohmann::json& request, int errorCode, const std::string& errorReason)>;

namespace srv {
    class TransportController;
    class ProducerController;
    class ConsumerController;
    class DataProducerController;
    class DataConsumerController;
}

class Room;

struct _device
{
    std::string flag;       //"broadcaster",
    std::string name;       //device.name || "Unknown device",
    std::string version;    //device.version
};

struct PeerInfo
{
    std::string id;
    std::string displayName;
    _device device;
    nlohmann::json producerInfo;
    std::vector<nlohmann::json> producers;
};

class PeerData
{
public:
    std::string id;
    bool consume { true };
    bool joined { false };
    std::string displayName;
    nlohmann::json device;
    nlohmann::json rtpCapabilities;
    nlohmann::json sctpCapabilities;
    
    std::unordered_map<std::string, std::shared_ptr<srv::TransportController>> transportControllers;
    std::unordered_map<std::string, std::shared_ptr<srv::ProducerController>> producerControllers;
    std::unordered_map<std::string, std::shared_ptr<srv::ConsumerController>> consumerControllers;
    std::unordered_map<std::string, std::shared_ptr<srv::DataProducerController>> dataProducerControllers;
    std::unordered_map<std::string, std::shared_ptr<srv::DataConsumerController>> dataConsumerControllers;
};

class Peer : public oatpp::websocket::AsyncWebSocket::Listener, public std::enable_shared_from_this<Peer>
{
public:
    enum class MessageType : uint8_t {
        REQUEST = 0,
        RESPONSE = 1,
        NOTIFICATION = 2
    };

    class Message {
    public:
        Message(int64_t id, MessageType type, const nlohmann::json& data)
        : _id(id)
        , _type(type)
        , _data(data) {}
        
        int64_t id() { return _id; }
        
        MessageType type() { return _type; }
        
        const nlohmann::json& data() { return _data; }
        
    private:
        int64_t _id;
        MessageType _type;
        nlohmann::json _data;
    };
    
public:
    Peer(const std::shared_ptr<AsyncWebSocket>& socket, const std::string& roomId, const std::string& peerId);
    
    ~Peer();

    void init();
    
    void destroy();

    const std::string& nickname() { return _nickname; }
    
    void setNickname(const std::string& name) { _nickname = name.c_str(); }

    std::string id() { return _id; }

    void invalidateSocket();
    
    std::shared_ptr<PeerData>& data() { return _data; }
    
    const std::string& roomId() { return _roomId; }
    
    void close();
    
    bool closed() { return _closed; }
    
    bool sendPing();

    void request(const std::string& method, const nlohmann::json& message);

    void notify(const std::string& method, const nlohmann::json& message);

private:
    // WebSocket Listener methods
    CoroutineStarter onPing(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message) override;
    
    CoroutineStarter onPong(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message) override;
    
    CoroutineStarter onClose(const std::shared_ptr<AsyncWebSocket>& socket, v_uint16 code, const oatpp::String& message) override;
    
    CoroutineStarter readMessage(const std::shared_ptr<AsyncWebSocket>& socket, v_uint8 opcode, p_char8 data, oatpp::v_io_size size) override;
    
private:
    oatpp::async::CoroutineStarter onApiError(const oatpp::String& errorMessage);
    
    oatpp::async::CoroutineStarter handleMessage(const nlohmann::json& message);
    
    void sendMessage(const nlohmann::json& message, MessageType type);
    
    void handleRequest(const nlohmann::json& request);
    
    void handleResponse(const nlohmann::json& response);
    
    void handleNotification(const nlohmann::json& notification);

    void accept(const nlohmann::json& request, const nlohmann::json& data);
    
    void reject(const nlohmann::json& request, int errorCode, const std::string& errorReason);
    
    void runOne();
    
public:
    // signals
    sigslot::signal<const std::string&> _closeSignal;
    
    sigslot::signal<const std::shared_ptr<Peer>&, const nlohmann::json&, AcceptFunc&, RejectFunc&> _requestSignal;
    
    sigslot::signal<const nlohmann::json&> _notificationSignal;
    
private:
    std::shared_ptr<PeerData> _data;
    
    /**
     * Buffer for messages. Needed for multi-frame messages.
     */
    oatpp::data::stream::BufferOutputStream _messageBuffer;

    /**
     * Lock for synchronization of writes to the web socket.
     */
    oatpp::async::Lock _writeLock;

    std::shared_ptr<AsyncWebSocket> _socket;
    
    std::string _roomId;
    
    std::string _id;
    
    std::string _nickname;
    
    std::atomic<v_int32> _pingPoingCounter;
    
    AcceptFunc _accept;
    
    RejectFunc _reject;
    
    std::atomic_bool _closed {false};
    
public:
    moodycamel::ConcurrentQueue<std::shared_ptr<Message>> _messageQueue;
    
    std::unordered_map<int64_t, std::shared_ptr<Message>> _requestMap;
    
    std::atomic_bool _executing {false};
    
private:
    /* Inject application components */
    OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, _asyncExecutor);
    
    OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, _objectMapper);
    
    OATPP_COMPONENT(oatpp::Object<ConfigDto>, _appConfig);
    
    OATPP_COMPONENT(std::shared_ptr<Statistics>, _statistics);
};


#endif //ASYNC_SERVER_ROOMS_PEER_HPP
