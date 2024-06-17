/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: RTC-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include "threadsafe_unordered_map.hpp"
#include "dto/dtos.hpp"
#include "dto/config.hpp"
#include "utils/statistics.hpp"
#include "oatpp-websocket/AsyncWebSocket.hpp"
#include "oatpp/network/ConnectionProvider.hpp"
#include "oatpp/async/Lock.hpp"
#include "oatpp/async/Executor.hpp"
#include "oatpp/data/mapping/ObjectMapper.hpp"
#include "oatpp/macro/component.hpp"
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "moodycamel/concurrentqueue.h"
#include "types.h"


using AcceptFunc = std::function<void(const nlohmann::json& request, const nlohmann::json& data)>;
using RejectFunc = std::function<void(const nlohmann::json& request, int errorCode, const std::string& errorReason)>;

namespace srv {
    class ITransportController;
    class IProducerController;
    class IConsumerController;
    class IDataProducerController;
    class IDataConsumerController;
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

class VideoProducerQualityController;

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
    
    std::threadsafe_unordered_map<std::string, std::shared_ptr<srv::ITransportController>> transportControllers;
    std::threadsafe_unordered_map<std::string, std::shared_ptr<srv::IProducerController>> producerControllers;
    std::threadsafe_unordered_map<std::string, std::shared_ptr<srv::IConsumerController>> consumerControllers;
    std::threadsafe_unordered_map<std::string, std::shared_ptr<srv::IDataProducerController>> dataProducerControllers;
    std::threadsafe_unordered_map<std::string, std::shared_ptr<srv::IDataConsumerController>> dataConsumerControllers;
    std::threadsafe_unordered_map<std::string, std::shared_ptr<VideoProducerQualityController>> videoProducerQualityControllers;
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
    sigslot::signal<const std::string&> closeSignal;
    
    sigslot::signal<const std::shared_ptr<Peer>&, const nlohmann::json&, AcceptFunc&, RejectFunc&> requestSignal;
    
    sigslot::signal<const std::shared_ptr<srv::IConsumerController>&> newConsumerResumedSignal;
    
    sigslot::signal<const nlohmann::json&> notificationSignal;
    
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
