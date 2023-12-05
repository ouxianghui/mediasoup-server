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
#include "router_controller.h"
#include "webrtc_server_controller.h"
#include "uuid.h"
#include "ortc.h"

using namespace std::chrono_literals;

namespace srv {

    static int _consumerChannelFd[2] = {3, 4};
    static int _producerChannelFd[2] = {5, 6};

    static const std::string WORKER_VERSION("3.13.6");
    
    WorkerController::WorkerController(const std::shared_ptr<WorkerSettings>& settings)
    : _settings(settings)
    {
        //_channel = std::make_shared<Channel>();
        _loop = new uv_loop_t;
        assert(_loop);
        
        uv_loop_init(_loop);
        
        _channel = std::make_shared<Channel>(_loop, _producerChannelFd[0], _consumerChannelFd[1]);
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
        
        if (_loop) {
            uv_loop_close(_loop);
            delete _loop;
            _loop = nullptr;
        }
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
        
        auto channelThread = std::thread([this](){
            uv_run(this->_loop, uv_run_mode::UV_RUN_DEFAULT);
        });
        
        mediasoup_worker_run(argc,
                             &argv[0],
                             WORKER_VERSION.c_str(),
                             _consumerChannelFd[0],
                             _producerChannelFd[1],
                             0,
                             0,
                             &Channel::channelRead,
                             (void*)_channel.get(),
                             &Channel::channelWrite,
                             (void*)_channel.get()
                             );
        
        close();
        channelThread.join();
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
        
        if (_loop) {
            uv_loop_close(_loop);
            delete _loop;
            _loop = nullptr;
        }
        
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

    std::shared_ptr<WorkerDump> WorkerController::dump()
    {
        SRV_LOGD("dump()");
        
        auto data = _channel->request(FBS::Request::Method::WORKER_DUMP, "");
        
        auto message = FBS::Message::GetMessage(data.data());
        
        auto response = message->data_as_Response();
        
        auto workerDumpResponse = response->body_as_Worker_DumpResponse();

        return parseWorkerDumpResponse(workerDumpResponse);
    }

    std::shared_ptr<WorkerResourceUsage> WorkerController::getResourceUsage()
    {
        SRV_LOGD("getResourceUsage()");
        
        auto data = _channel->request(FBS::Request::Method::WORKER_GET_RESOURCE_USAGE, "");
   
        auto usage = std::make_shared<WorkerResourceUsage>();
        
        auto message = FBS::Message::GetMessage(data.data());
        
        auto response = message->data_as_Response();
        
        auto resourceUsage = response->body_as_Worker_ResourceUsageResponse();
        
        usage->ru_utime = resourceUsage->ruUtime();
        usage->ru_stime = resourceUsage->ruStime();
        usage->ru_maxrss = resourceUsage->ruMaxrss();
        usage->ru_ixrss = resourceUsage->ruIxrss();
        usage->ru_idrss = resourceUsage->ruIdrss();
        usage->ru_isrss = resourceUsage->ruIsrss();
        usage->ru_minflt = resourceUsage->ruMinflt();
        usage->ru_majflt = resourceUsage->ruMajflt();
        usage->ru_nswap = resourceUsage->ruNswap();
        usage->ru_inblock = resourceUsage->ruInblock();
        usage->ru_oublock = resourceUsage->ruOublock();
        usage->ru_msgsnd = resourceUsage->ruMsgsnd();
        usage->ru_msgrcv = resourceUsage->ruMsgrcv();
        usage->ru_nsignals = resourceUsage->ruNsignals();
        usage->ru_nvcsw = resourceUsage->ruNvcsw();
        usage->ru_nivcsw = resourceUsage->ruNivcsw();
        
        return usage;
    }

    void WorkerController::updateSettings(const std::string& logLevel, const std::vector<std::string>& logTags)
    {
        SRV_LOGD("updateSettings()");
        
        std::vector<::flatbuffers::Offset<::flatbuffers::String>> logTags_;
        for (const auto& item : logTags) {
            logTags_.emplace_back(_channel->builder().CreateString(item));
        }
        
        auto reqOffset = FBS::Worker::CreateUpdateSettingsRequestDirect(_channel->builder(), logLevel.c_str(), &logTags_);
        
        _channel->request(FBS::Request::Method::WORKER_UPDATE_SETTINGS,
                          FBS::Request::Body::Worker_UpdateSettingsRequest,
                          reqOffset,
                          "");
    }

    std::shared_ptr<IWebRtcServerController> WorkerController::createWebRtcServerController(const std::shared_ptr<WebRtcServerOptions>& options, const nlohmann::json& appData)
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
        
        std::vector<flatbuffers::Offset<FBS::Transport::ListenInfo>> infos;
        
        for (auto& info : listenInfos) {
            auto info_ = FBS::Transport::CreateListenInfoDirect(_channel->builder(),
                                                                info.protocol == "udp" ? FBS::Transport::Protocol::UDP : FBS::Transport::Protocol::TCP,
                                                                info.ip.c_str(),
                                                                info.announcedIp.c_str(),
                                                                info.port,
                                                                info.sendBufferSize,
                                                                info.recvBufferSize
                                                                );
            infos.emplace_back(info_);
        }
        
        auto reqOffset = FBS::Worker::CreateCreateWebRtcServerRequestDirect(_channel->builder(), webRtcServerId.c_str(), &infos);
        
        _channel->request(FBS::Request::Method::WORKER_CREATE_WEBRTCSERVER,
                          FBS::Request::Body::Worker_CreateWebRtcServerRequest,
                          reqOffset,
                          "");

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

    std::shared_ptr<IRouterController> WorkerController::createRouterController(const std::vector<RtpCodecCapability>& mediaCodecs, const nlohmann::json& appData)
    {
        SRV_LOGD("createRouter()");
        
        std::shared_ptr<RouterController> routerController;
        
        // This may throw.
        auto rtpCapabilities = ortc::generateRouterRtpCapabilities(mediaCodecs);

        RouterInternal internal;
        internal.routerId = uuid::uuidv4();
        
        auto reqOffset = FBS::Worker::CreateCreateRouterRequestDirect(_channel->builder(), internal.routerId.c_str());

        _channel->request(FBS::Request::Method::WORKER_CREATE_ROUTER, FBS::Request::Body::Worker_CreateRouterRequest, reqOffset, "");

        RouterData data;
        data.rtpCapabilities = rtpCapabilities;
        
        {
            std::lock_guard<std::mutex> lock(_routersMutex);
            routerController = std::make_shared<RouterController>(internal,
                                                                  data,
                                                                  _channel,
                                                                  appData);
            routerController->init();
            
            _routerControllers.emplace(routerController);
        }
        
        routerController->closeSignal.connect(&WorkerController::onRouterClose, shared_from_this());
        
        this->newRouterSignal(routerController);
        
        return routerController;
    }

    void WorkerController::onWebRtcServerClose(std::shared_ptr<IWebRtcServerController> controller)
    {
        std::lock_guard<std::mutex> lock(_webRtcServersMutex);
        _webRtcServerControllers.erase(controller);
    }

    void WorkerController::onRouterClose(std::shared_ptr<IRouterController> controller)
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

    void WorkerController::onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data)
    {
        if (event == FBS::Notification::Event::WORKER_RUNNING) {
            this->startSignal();
            this->startSignal.disconnect_all();
        }
        else {
            SRV_LOGD("ignoring unknown event %u", (uint8_t)event);
        }
    }
}

namespace srv
{

    std::shared_ptr<WorkerDump> parseWorkerDumpResponse(const FBS::Worker::DumpResponse* response)
    {
        auto workerDump = std::make_shared<WorkerDump>();
        
        auto serverIds = response->webRtcServerIds();
        
        for (const auto& item : *serverIds) {
            workerDump->webRtcServerIds.emplace_back(item->str());
        }
        
        auto routerIds = response->routerIds();
        
        for (const auto& item : *routerIds) {
            workerDump->routerIds.emplace_back(item->str());
        }
        
        auto messageHandlers = response->channelMessageHandlers();
        
        auto requestHandlers = messageHandlers->channelRequestHandlers();
        
        for (const auto& item : *requestHandlers) {
            workerDump->channelMessageHandlers.channelRequestHandlers.emplace_back(item->str());
        }
        
        auto notificationHandlers = messageHandlers->channelNotificationHandlers();
        
        for (const auto& item : *notificationHandlers) {
            workerDump->channelMessageHandlers.channelNotificationHandlers.emplace_back(item->str());
        }
        
        return workerDump;
    }

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
}
