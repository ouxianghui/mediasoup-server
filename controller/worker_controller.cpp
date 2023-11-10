/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "worker_controller.h"
#include <random>
#include <thread>
#include <chrono>
#include <iostream>
#include "srv_logger.h"
#include "lib.hpp"
#include "channel.h"
#include "payload_channel.h"
#include "router_controller.h"
#include "webrtc_server_controller.h"
#include "uuid.h"
#include "ortc.h"

using namespace std::chrono_literals;

namespace srv {

    static const std::string WORKER_VERSION("3.12.13");
    
    WorkerController::WorkerController(const std::shared_ptr<WorkerSettings>& settings)
    : _settings(settings)
    {
        _channel = std::make_shared<Channel>();
        
        _payloadChannel = std::make_shared<PayloadChannel>();
    }

    WorkerController::~WorkerController()
    {
        SRV_LOGD("~WorkerController()")
    }

    void WorkerController::init()
    {
        SRV_LOGD("init()");
        handleWorkerNotifications();
    }

    void WorkerController::destroy()
    {
        SRV_LOGD("destroy()");
    }

    std::vector<std::string> WorkerController::getArgs(const std::shared_ptr<WorkerSettings>& settings)
    {
        assert(settings);

        std::vector<std::string> args;
        		
        if (!settings->logLevel.empty()) {
            std::string level("--logLevel=");
            level += settings->logLevel;
			args.emplace_back(level);
		}

		for (const auto& tag : settings->logTags) {
			if (!tag.empty()) {
                std::string _tag("--logTag=");
                _tag += tag;
				args.emplace_back(_tag);
			}
		}

		if (settings->rtcMinPort > 0) {
            std::string port("--rtcMinPort=");
            port += std::to_string(settings->rtcMinPort);
			args.emplace_back(port);
		}

		if (settings->rtcMaxPort > 0) {            
            std::string port("--rtcMaxPort=");
            port += std::to_string(settings->rtcMaxPort);
			args.emplace_back(port);
		}

		if (!settings->dtlsCertificateFile.empty()) {
            std::string file("--dtlsCertificateFile=");
            file += settings->dtlsCertificateFile;
			args.emplace_back(file);
		}

        if (!settings->dtlsPrivateKeyFile.empty()) {
            std::string file("--dtlsPrivateKeyFile=");
            file += settings->dtlsPrivateKeyFile;
			args.emplace_back(file);
		}

        if (!settings->libwebrtcFieldTrials.empty()) {
            std::string fieldTrials("--libwebrtcFieldTrials=");
            fieldTrials += settings->libwebrtcFieldTrials;
			args.emplace_back(fieldTrials);
		}

        return args;
    }

    void WorkerController::runWorker()
    {
        auto args = getArgs(_settings);
        
        int argc = static_cast<int>(args.size());
        std::vector<char*> argv(argc);
        argv[0] = const_cast<char*>(args[0].c_str());
        for (size_t i = 0; i < args.size(); ++i) {
            argv[i] = const_cast<char*>(args[i].c_str());
        }
        
        mediasoup_worker_run(argc,
                             &argv[0],
                             WORKER_VERSION.c_str(),
                             0, 0, 0, 0,
                             &Channel::channelRead,
                             (void*)_channel.get(),
                             &Channel::channelWrite,
                             (void*)_channel.get(),
                             &PayloadChannel::payloadChannelRead,
                             (void*)_payloadChannel.get(),
                             &PayloadChannel::payloadChannelWrite,
                             (void*)_payloadChannel.get()
                             );
        
        close();
    }

    bool WorkerController::closed()
    {
        return _closed;
    }

    void WorkerController::close()
    {
        if (_closed) {
            return;
        }
        
        SRV_LOGD("close()");
        
        _closed = true;
        
        _channel->close();
        
        _payloadChannel->close();
        
        {
            std::lock_guard<std::mutex> lock(_webRtcServersMutex);
            for (const auto& item: _webRtcServerControllers) {
                item->onWorkerClosed();
            }
            _webRtcServerControllers.clear();
        }
        
        {
            std::lock_guard<std::mutex> lock(_routersMutex);
            for (const auto& item: _routerControllers) {
                item->onWorkerClosed();
            }
            _routerControllers.clear();
        }
        
        this->closeSignal();
    }

    nlohmann::json WorkerController::dump()
    {
        SRV_LOGD("dump()");
        return _channel->request("worker.dump", "", "{}");
    }

    std::shared_ptr<WorkerResourceUsage> WorkerController::getResourceUsage()
    {
        SRV_LOGD("getResourceUsage()");
        
        auto jsUsage = _channel->request("worker.getResourceUsage", "", "{}");
   
        auto usage = std::make_shared<WorkerResourceUsage>();
        if (jsUsage.is_object()) {
            *usage = jsUsage;
        }
        
        return usage;
    }

    void WorkerController::updateSettings(const std::string& logLevel, const std::vector<std::string>& logTags)
    {
        SRV_LOGD("updateSettings()");
    
        nlohmann::json tags = nlohmann::json::array();
        for (const auto& tag : logTags) {
            tags.emplace_back(tag);
        }
        nlohmann::json reqData;
        reqData["logLevel"] = logLevel;
        reqData["logTags"] = tags;
        
        _channel->request("worker.updateSettings", "", reqData.dump());
    }

    std::shared_ptr<WebRtcServerController> WorkerController::createWebRtcServerController(const std::shared_ptr<WebRtcServerOptions>& options, const nlohmann::json& appData)
    {
        SRV_LOGD("createWebRtcServer()");

        std::shared_ptr<WebRtcServerController> webRtcServerController;
        
        if (!options) {
            SRV_LOGE("webrtc server options must not be null");
            return webRtcServerController;
        }
        
        auto listenInfos = options->listenInfos;
        
        if (listenInfos.empty()) {
            SRV_LOGE("webrtc server listen infos must not be empty");
            return webRtcServerController;
        }
        
        std::string webRtcServerId = uuid::uuidv4();
        
        nlohmann::json reqData;
        reqData["webRtcServerId"] = webRtcServerId;
        reqData["listenInfos"] = listenInfos;
        
        _channel->request("worker.createWebRtcServer", "", reqData.dump());

        std::lock_guard<std::mutex> lock(_webRtcServersMutex);
        WebRtcServerInternal internal { webRtcServerId };
        
        webRtcServerController = std::make_shared<WebRtcServerController>(internal, _channel, appData);
        webRtcServerController->init();
        
        _webRtcServerControllers.emplace(webRtcServerController);
        
        webRtcServerController->closeSignal.connect(&WorkerController::onWebRtcServerClose, shared_from_this());
        
        // Emit observer event.
        this->newWebRtcServerSignal(webRtcServerController);

        return webRtcServerController;
    }

    std::shared_ptr<RouterController> WorkerController::createRouterController(const std::vector<RtpCodecCapability>& mediaCodecs, const nlohmann::json& appData)
    {
        SRV_LOGD("createRouter()");
        
        std::shared_ptr<RouterController> routerController;
        
        // This may throw.
        auto rtpCapabilities = ortc::generateRouterRtpCapabilities(mediaCodecs);

        RouterInternal internal;
        internal.routerId = uuid::uuidv4();
        
        nlohmann::json reqData;
        reqData["routerId"] = internal.routerId;

        _channel->request("worker.createRouter", "", reqData.dump());

        RouterData data;
        data.rtpCapabilities = rtpCapabilities;
        
        {
            std::lock_guard<std::mutex> lock(_routersMutex);
            routerController = std::make_shared<RouterController>(internal,
                                                                  data,
                                                                  _channel,
                                                                  _payloadChannel,
                                                                  appData);
            routerController->init();
            
            _routerControllers.emplace(routerController);
        }
        
        routerController->closeSignal.connect(&WorkerController::onRouterClose, shared_from_this());
        
        this->newRouterSignal(routerController);
        
        return routerController;
    }

    void WorkerController::onWebRtcServerClose(std::shared_ptr<WebRtcServerController> controller)
    {
        std::lock_guard<std::mutex> lock(_webRtcServersMutex);
        _webRtcServerControllers.erase(controller);
    }

    void WorkerController::onRouterClose(std::shared_ptr<RouterController> controller)
    {
        std::lock_guard<std::mutex> lock(_routersMutex);
        _routerControllers.erase(controller);
    }

    void WorkerController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto channel = _channel;
        if (!channel) {
            return;
        }
        channel->notificationSignal.connect(&WorkerController::onChannel, shared_from_this());
    }

    void WorkerController::onChannel(const std::string& targetId, const std::string& event, const std::string& data)
    {
        if (event == "running") {
            this->startSignal();
            this->startSignal.disconnect_all();
            //getDump();
        }
        else {
            SRV_LOGD("ignoring unknown event %s", event.c_str());
        }
    }

    void WorkerController::getDump()
    {
        auto val1 = dump();
        std::cout << "----> val1: " << val1.dump() << std::endl;
        
        auto usage = getResourceUsage();
        nlohmann::json val2 = *usage;
        std::cout << "----> val2: " << val2.dump() << std::endl;
    }
}

namespace srv
{
    void to_json(nlohmann::json& j, const WorkerSettings& st)
    {
        j["logLevel"] = st.logLevel;
        j["logTags"] = st.logTags;
        j["rtcMinPort"] = st.rtcMinPort;
        j["rtcMaxPort"] = st.rtcMaxPort;
        j["dtlsCertificateFile"] = st.dtlsCertificateFile;
        j["dtlsPrivateKeyFile"] = st.dtlsPrivateKeyFile;
        j["libwebrtcFieldTrials"] = st.libwebrtcFieldTrials;
        j["appData"] = st.appData;
    }

    void from_json(const nlohmann::json& j, WorkerSettings& st)
    {
        if (j.contains("logLevel")) {
            j.at("logLevel").get_to(st.logLevel);
        }
        if (j.contains("logTags")) {
            j.at("logTags").get_to(st.logTags);
        }
        if (j.contains("rtcMinPort")) {
            j.at("rtcMinPort").get_to(st.rtcMinPort);
        }
        if (j.contains("rtcMaxPort")) {
            j.at("rtcMaxPort").get_to(st.rtcMaxPort);
        }
        if (j.contains("dtlsCertificateFile")) {
            j.at("dtlsCertificateFile").get_to(st.dtlsCertificateFile);
        }
        if (j.contains("dtlsPrivateKeyFile")) {
            j.at("dtlsPrivateKeyFile").get_to(st.dtlsPrivateKeyFile);
        }
        if (j.contains("libwebrtcFieldTrials")) {
            j.at("libwebrtcFieldTrials").get_to(st.libwebrtcFieldTrials);
        }
        if (j.contains("appData")) {
            j.at("appData").get_to(st.appData);
        }
    }

    void to_json(nlohmann::json& j, const WorkerResourceUsage& st)
    {
        j["ru_utime"] = st.ru_utime;
        j["ru_stime"] = st.ru_stime;
        j["ru_maxrss"] = st.ru_maxrss;
        j["ru_ixrss"] = st.ru_ixrss;
        j["ru_idrss"] = st.ru_idrss;
        j["ru_isrss"] = st.ru_isrss;
        j["ru_minflt"] = st.ru_minflt;
        j["ru_majflt"] = st.ru_majflt;
        j["ru_nswap"] = st.ru_nswap;
        j["ru_inblock"] = st.ru_inblock;
        j["ru_oublock"] = st.ru_oublock;
        j["ru_msgsnd"] = st.ru_msgsnd;
        j["ru_msgrcv"] = st.ru_msgrcv;
        j["ru_nsignals"] = st.ru_nsignals;
        j["ru_nvcsw"] = st.ru_nvcsw;
        j["ru_nivcsw"] = st.ru_nivcsw;
    }

    void from_json(const nlohmann::json& j, WorkerResourceUsage& st)
    {
        if (j.contains("ru_utime")) {
            j.at("ru_utime").get_to(st.ru_utime);
        }
        if (j.contains("ru_stime")) {
            j.at("ru_stime").get_to(st.ru_stime);
        }
        if (j.contains("ru_maxrss")) {
            j.at("ru_maxrss").get_to(st.ru_maxrss);
        }
        if (j.contains("ru_ixrss")) {
            j.at("ru_ixrss").get_to(st.ru_ixrss);
        }
        if (j.contains("ru_idrss")) {
            j.at("ru_idrss").get_to(st.ru_idrss);
        }
        if (j.contains("ru_isrss")) {
            j.at("ru_isrss").get_to(st.ru_isrss);
        }
        if (j.contains("ru_minflt")) {
            j.at("ru_minflt").get_to(st.ru_minflt);
        }
        if (j.contains("ru_majflt")) {
            j.at("ru_majflt").get_to(st.ru_majflt);
        }
        if (j.contains("ru_nswap")) {
            j.at("ru_nswap").get_to(st.ru_nswap);
        }
        if (j.contains("ru_inblock")) {
            j.at("ru_inblock").get_to(st.ru_inblock);
        }
        if (j.contains("ru_oublock")) {
            j.at("ru_oublock").get_to(st.ru_oublock);
        }
        if (j.contains("ru_msgsnd")) {
            j.at("ru_msgsnd").get_to(st.ru_msgsnd);
        }
        if (j.contains("ru_msgrcv")) {
            j.at("ru_msgrcv").get_to(st.ru_msgrcv);
        }
        if (j.contains("ru_nsignals")) {
            j.at("ru_nsignals").get_to(st.ru_nsignals);
        }
        if (j.contains("ru_nvcsw")) {
            j.at("ru_nvcsw").get_to(st.ru_nvcsw);
        }
        if (j.contains("ru_nivcsw")) {
            j.at("ru_nivcsw").get_to(st.ru_nivcsw);
        }
    }
}
