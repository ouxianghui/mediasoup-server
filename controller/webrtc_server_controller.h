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
#include <vector>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "types.h"
#include "transport_controller.h"
#include "FBS/webRtcServer.h"

namespace srv {

    class Channel;
    class WebRtcTransportController;

    struct WebRtcServerOptions
    {
        /**
         * Listen infos.
         */
        std::vector<TransportListenInfo> listenInfos;

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    void to_json(nlohmann::json& j, const WebRtcServerOptions& st);
    void from_json(const nlohmann::json& j, WebRtcServerOptions& st);

    struct WebRtcServerListenInfo : TransportListenInfo {};

    struct IpPort
    {
        std::string ip;
        uint16_t port;
    };

    struct IceUserNameFragment
    {
        std::string localIceUsernameFragment;
        std::string webRtcTransportId;
    };

    struct TupleHash
    {
        uint64_t tupleHash;
        std::string webRtcTransportId;
    };

    struct WebRtcServerDump
    {
        std::string id;
        std::vector<IpPort> udpSockets;
        std::vector<IpPort> tcpServers;
        std::vector<std::string> webRtcTransportIds;
        std::vector<IceUserNameFragment> localIceUsernameFragments;
        std::vector<TupleHash> tupleHashes;
    };

    struct WebRtcServerInternal
    {
        std::string webRtcServerId;
    };

    class WebRtcServerController : public std::enable_shared_from_this<WebRtcServerController> {
    public:
        WebRtcServerController(const WebRtcServerInternal& internal, std::weak_ptr<Channel> channel, const nlohmann::json& appData);
        
        ~WebRtcServerController();
        
        void init();
        
        void destroy();
        
        const std::string& id() { return _id; }
        
        bool closed() { return _closed; }
        
        void setAppData(const nlohmann::json& data) { _appData = data; }
        
        const nlohmann::json& appData() { return _appData; }
        
        void handleWebRtcTransport(const std::shared_ptr<WebRtcTransportController>& controller);
        
        std::shared_ptr<WebRtcServerDump> dump();
        
        void onWorkerClosed();
        
    public:
        sigslot::signal<> workerCloseSignal;
        
        sigslot::signal<std::shared_ptr<WebRtcServerController>> closeSignal;
        
        sigslot::signal<std::shared_ptr<WebRtcTransportController>> webrtcTransportHandledSignal;
        
        sigslot::signal<std::shared_ptr<WebRtcTransportController>> webrtcTransportUnhandledSignal;
        
    private:
        void close();
        
        void onWebRtcTransportClose(const std::string& id_);
        
    private:
        std::string _id;
        
        // Channel instance.
        std::weak_ptr<Channel> _channel;

        // Closed flag.
        std::atomic_bool _closed { false };

        // Custom app data.
        nlohmann::json _appData;
        
        std::mutex _webRtcTransportsMutex;
        std::unordered_map<std::string, std::shared_ptr<WebRtcTransportController>> _webRtcTransportMap;
    };

    std::shared_ptr<IpPort> parseIpPort(const FBS::WebRtcServer::IpPort* binary);

    std::shared_ptr<IceUserNameFragment> parseIceUserNameFragment(const FBS::WebRtcServer::IceUserNameFragment* binary);

    std::shared_ptr<TupleHash> parseTupleHash(const FBS::WebRtcServer::TupleHash* binary);

    std::shared_ptr<WebRtcServerDump> parseWebRtcServerDump(const FBS::WebRtcServer::DumpResponse* data);

}
