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
#include <string>
#include "nlohmann/json.hpp"
#include "worker_controller.h"
#include "router_controller.h"
#include "webrtc_transport_controller.h"
#include "plain_transport_controller.h"
#include "webrtc_server_controller.h"

namespace srv {

namespace cfg {
        struct Tls {
            std::string cert;
            std::string key;
        };

        void to_json(nlohmann::json& j, const Tls& st);
        void from_json(const nlohmann::json& j, Tls& st);

        struct Https {
            std::string listenIp;
            int32_t listenPort;
            Tls tls;
        };

        void to_json(nlohmann::json& j, const Https& st);
        void from_json(const nlohmann::json& j, Https& st);

        struct Mediasoup {
            int32_t numWorkers;
            bool useWebRtcServer = true;
            bool multiprocess = false;
            std::string workerPath;
            WorkerSettings workerSettings;
            RouterOptions routerOptions;
            WebRtcServerOptions webRtcServerOptions;
            WebRtcTransportOptions webRtcTransportOptions;
            PlainTransportOptions plainTransportOptions;
        };

        void to_json(nlohmann::json& j, const Mediasoup& st);
        void from_json(const nlohmann::json& j, Mediasoup& st);

        struct Params {
            std::string domain;
            Https https;
            Mediasoup mediasoup;
        };

        void to_json(nlohmann::json& j, const Params& st);
        void from_json(const nlohmann::json& j, Params& st);
    }

    class Config : public std::enable_shared_from_this<Config>
    {
    public:
        ~Config();
        
        static std::shared_ptr<Config>& sharedInstance();

        void init(const std::string& configFileName);
        
        void destroy();
        
        const std::shared_ptr<cfg::Params>& params();

    private:
        Config();

        Config(const Config&) = delete;

        Config& operator=(const Config&) = delete;

        Config(Config&&) = delete;

        Config& operator=(Config&&) = delete;

    private:
        std::string _configFileName;
        
        std::shared_ptr<cfg::Params> _params;
    };
}

#ifndef MSConfig
#define MSConfig srv::Config::sharedInstance()
#endif
