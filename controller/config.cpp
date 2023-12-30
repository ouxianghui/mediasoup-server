/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "config.h"
#include <fstream>
#include <sstream>
#include "srv_logger.h"
#include "nlohmann/json.hpp"

namespace srv {

    std::shared_ptr<Config>& Config::sharedInstance()
    {
        static std::shared_ptr<Config> _instance;
        static std::once_flag ocf;
        std::call_once(ocf, []() {
            _instance.reset(new Config());
        });
        return _instance;
    }

    Config::Config()
    {
        _params = std::make_shared<cfg::Params>();
    }

    Config::~Config()
    {
        
    }

    void Config::init(const std::string& configFileName)
    {
        std::ifstream fin;
        fin.open(configFileName, std::ios::in);
        if (!fin.is_open()) {
            SRV_LOGE("config.json not found");
            return;
        }
        
        char buff[1024] = { 0 };
        std::stringstream ss;
        
        while (fin.getline(buff, sizeof(buff))) {
            ss << buff;
        }
        
        fin.close();
        
        try {
            *_params = nlohmann::json::parse(ss.str());
        }
        catch(const char* error) {
            SRV_LOGE("parse config.json failed: %s", error);
        }
    }

    void Config::destroy()
    {
        
    }

    const std::shared_ptr<cfg::Params>& Config::params()
    {
        return _params;
    }
}

namespace srv {
    namespace cfg {
        void to_json(nlohmann::json& j, const Tls& st)
        {
            j["cert"] = st.cert ;
            j["key"] = st.key;
        }

        void from_json(const nlohmann::json& j, Tls& st)
        {
            if (j.contains("cert")) {
                j.at("cert").get_to(st.cert);
            }
            if (j.contains("key")) {
                j.at("key").get_to(st.key);
            }
        }

        void to_json(nlohmann::json& j, const Https& st)
        {
            j["listenIp"] = st.listenIp;
            j["listenPort"] = st.listenPort;
            j["tls"] = st.tls;
        }

        void from_json(const nlohmann::json& j, Https& st)
        {
            if (j.contains("listenIp")) {
                j.at("listenIp").get_to(st.listenIp);
            }
            if (j.contains("listenPort")) {
                j.at("listenPort").get_to(st.listenPort);
            }
            if (j.contains("tls")) {
                j.at("tls").get_to(st.tls);
            }
        }
    
        void to_json(nlohmann::json& j, const Mediasoup& st)
        {
            j["numWorkers"] = st.numWorkers;
            j["useWebRtcServer"] = st.useWebRtcServer;
            j["multiprocess"] = st.multiprocess;
            j["workerPath"] = st.workerPath;
            j["workerSettings"] = st.workerSettings;
            j["routerOptions"] = st.routerOptions;
            j["webRtcServerOptions"] = st.webRtcServerOptions;
            j["webRtcTransportOptions"] = st.webRtcTransportOptions;
            j["plainTransportOptions"] = st.plainTransportOptions;
        }

        void from_json(const nlohmann::json& j, Mediasoup& st)
        {
            if (j.contains("numWorkers")) {
                j.at("numWorkers").get_to(st.numWorkers);
            }
            if (j.contains("useWebRtcServer")) {
                j.at("useWebRtcServer").get_to(st.useWebRtcServer);
            }
            if (j.contains("multiprocess")) {
                j.at("multiprocess").get_to(st.multiprocess);
            }
            if (j.contains("workerPath")) {
                j.at("workerPath").get_to(st.workerPath);
            }
            if (j.contains("workerSettings")) {
                j.at("workerSettings").get_to(st.workerSettings);
            }
            if (j.contains("routerOptions")) {
                j.at("routerOptions").get_to(st.routerOptions);
            }
            if (j.contains("webRtcServerOptions")) {
                j.at("webRtcServerOptions").get_to(st.webRtcServerOptions);
            }
            if (j.contains("webRtcTransportOptions")) {
                j.at("webRtcTransportOptions").get_to(st.webRtcTransportOptions);
            }
            if (j.contains("plainTransportOptions")) {
                j.at("plainTransportOptions").get_to(st.plainTransportOptions);
            }
        }

        void to_json(nlohmann::json& j, const Params& st)
        {
            j["domain"] = st.domain;
            j["https"] = st.https;
            j["mediasoup"] = st.mediasoup;
        }

        void from_json(const nlohmann::json& j, Params& st)
        {
            if (j.contains("domain")) {
                j.at("domain").get_to(st.domain);
            }
            if (j.contains("https")) {
                j.at("https").get_to(st.https);
            }
            if (j.contains("mediasoup")) {
                j.at("mediasoup").get_to(st.mediasoup);
            }
        }
    }
}
