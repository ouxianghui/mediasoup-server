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
#include "threadsafe_unordered_map.hpp"
#include "interface/i_transport_controller.h"
#include "interface/i_webrtc_server_controller.h"
#include "FBS/webRtcServer.h"

namespace srv {

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

    class WebRtcServerController : public IWebRtcServerController, public std::enable_shared_from_this<WebRtcServerController> {
    public:
        WebRtcServerController(const WebRtcServerInternal& internal, std::weak_ptr<Channel> channel, const nlohmann::json& appData);
        
        ~WebRtcServerController();
        
        void init() override;
        
        void destroy() override;
        
        const std::string& id() override { return _id; }
        
        void setAppData(const nlohmann::json& data) override { _appData = data; }
        
        const nlohmann::json& appData() override { return _appData; }
    
        void close() override;

        bool closed() override { return _closed; }

        void handleWebRtcTransport(const std::shared_ptr<WebRtcTransportController>& controller) override;
        
        std::shared_ptr<WebRtcServerDump> dump() override;
        
        void onWorkerClosed() override;

    private:
        void onWebRtcTransportClose(const std::string& id_);
        
    private:
        std::string _id;
        
        // Channel instance.
        std::weak_ptr<Channel> _channel;

        // Closed flag.
        std::atomic_bool _closed { false };

        // Custom app data.
        nlohmann::json _appData;
        
        std::threadsafe_unordered_map<std::string, std::shared_ptr<WebRtcTransportController>> _webRtcTransportMap;
    };

    std::shared_ptr<IpPort> parseIpPort(const FBS::WebRtcServer::IpPort* binary);

    std::shared_ptr<IceUserNameFragment> parseIceUserNameFragment(const FBS::WebRtcServer::IceUserNameFragment* binary);

    std::shared_ptr<TupleHash> parseTupleHash(const FBS::WebRtcServer::TupleHash* binary);

    std::shared_ptr<WebRtcServerDump> parseWebRtcServerDump(const FBS::WebRtcServer::DumpResponse* data);

}
