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

namespace srv {

    class Channel;
    class WebRtcTransportController;

    struct WebRtcServerListenInfo
    {
        /**
         * Network protocol.
         * options: tcp | udp
         */
        std::string protocol;

        /**
         * Listening IPv4 or IPv6.
         */
        std::string ip;

        /**
         * Announced IPv4 or IPv6 (useful when running mediasoup behind NAT with
         * private IP).
         */
        std::string announcedIp;

        /**
         * Listening port.
         */
        int32_t port = 0;
    };

    void to_json(nlohmann::json& j, const WebRtcServerListenInfo& st);
    void from_json(const nlohmann::json& j, WebRtcServerListenInfo& st);

    struct WebRtcServerOptions
    {
        /**
         * Listen infos.
         */
        std::vector<WebRtcServerListenInfo> listenInfos;

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    void to_json(nlohmann::json& j, const WebRtcServerOptions& st);
    void from_json(const nlohmann::json& j, WebRtcServerOptions& st);

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
        
        nlohmann::json dump();
        
        void onWorkerClosed();
        
    public:
        sigslot::signal<> _workerCloseSignal;
        
        sigslot::signal<std::shared_ptr<WebRtcServerController>> _closeSignal;
        
        sigslot::signal<std::shared_ptr<WebRtcTransportController>> _webrtcTransportHandledSignal;
        
        sigslot::signal<std::shared_ptr<WebRtcTransportController>> _webrtcTransportUnhandledSignal;
        
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

}
