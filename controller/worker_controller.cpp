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
#include <unistd.h>
#include "srv_logger.h"
#include "lib.hpp"
#include "channel.h"
#include "router_controller.h"
#include "webrtc_server_controller.h"
#include "uuid.h"
#include "ortc.h"
#include "config.h"
#include "message_builder.h"

using namespace std::chrono_literals;

namespace
{
    static int createPipe(int* fds)
    {
        if (pipe(fds) == -1) {
            SRV_LOGE("pipe failed");
            return -1;
        }

        if (fcntl(fds[0], F_SETFD, FD_CLOEXEC) == -1 || fcntl(fds[1], F_SETFD, FD_CLOEXEC) == -1) {
            close(fds[0]);
            close(fds[1]);
            SRV_LOGE("fcntl failed");
            return -1;
        }

        return 0;
    }
}

namespace srv {

    static int _consumerChannelFd[2] = {3, 5};
    static int _producerChannelFd[2] = {6, 4};

    static const std::string MEDIASOUP_VERSION("3.14.7");
    
    WorkerController::WorkerController(const std::shared_ptr<WorkerSettings>& settings)
    : _settings(settings)
    {
        if (MSConfig->params()->mediasoup.multiprocess) {
            createPipe(_consumerChannelFd);
            createPipe(_producerChannelFd);
            _channel = std::make_shared<Channel>(_producerChannelFd[0], _consumerChannelFd[1]);
        }
        else {
            _channel = std::make_shared<Channel>();
        }
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
        		
        if (MSConfig->params()->mediasoup.multiprocess) {
            std::string bin("mediasoup-worker");
            args.emplace_back(bin);
        }
        
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
        
        if (MSConfig->params()->mediasoup.multiprocess) {
            // ediasoup_worker_run(argc,
            //                     &argv[0],
            //                     MEDIASOUP_VERSION.c_str(),
            //                     _consumerChannelFd[0],
            //                     _producerChannelFd[1],
            //                     0,
            //                     0,
            //                     0,
            //                     0,
            //                     0,
            //                     0
            //                     );
            // close();
            
            int argc = static_cast<int>(args.size() + 1);
            std::vector<char*> argv(argc);
            
            for (size_t i = 0; i < args.size(); ++i) {
                argv[i] = const_cast<char*>(args[i].c_str());
            }
            
            argv[args.size()] = nullptr;
            
            uv_stdio_container_t childStdio[5];
            
            // fd 0 (stdin)   : Just ignore it.
            // fd 1 (stdout)  : Pipe it for 3rd libraries that log their own stuff.
            // fd 2 (stderr)  : Same as stdout.
            // fd 3 (channel) : Producer Channel fd.
            // fd 4 (channel) : Consumer Channel fd.
            
            std::string version("MEDIASOUP_VERSION");
            version += "=";
            version += MEDIASOUP_VERSION;
            
            char* env[2];
            env[0] = (char*)version.c_str();
            env[1] = nullptr;
            
            childStdio[0].flags = UV_IGNORE;
            
            childStdio[1].flags =  uv_stdio_flags(UV_INHERIT_FD | UV_WRITABLE_PIPE);;
            childStdio[1].data.fd = 1;
            
            childStdio[2].flags =  uv_stdio_flags(UV_INHERIT_FD | UV_WRITABLE_PIPE);;
            childStdio[2].data.fd = 2;
            
            childStdio[3].flags = uv_stdio_flags(UV_INHERIT_FD | UV_READABLE_PIPE);
            childStdio[3].data.fd = _consumerChannelFd[0];
            
            childStdio[4].flags = uv_stdio_flags(UV_INHERIT_FD | UV_WRITABLE_PIPE);
            childStdio[4].data.fd = _producerChannelFd[1];
            
            uv_process_options_t options;
            memset(&options, 0, sizeof(uv_process_options_t));
            
            options.env = env;
            options.stdio_count = 5;
            options.stdio = childStdio;
            //options.flags = UV_PROCESS_DETACHED;
            
            auto callback = [](uv_process_t* process, int64_t exit_status, int term_signal) {
                assert(process);
                
                if (exit_status == 42) {
                    SRV_LOGE("worker process failed due to wrong settings [pid:%d]", process->pid);
                }
                else {
                    SRV_LOGE("worker process failed unexpectedly [pid:%d, code:%lld, signal:%d]", process->pid, exit_status, term_signal);
                }
                
                if (auto workerController = static_cast<WorkerController*>(process->data)) {
                    workerController->close();
                }
            };
            
            assert(!MSConfig->params()->mediasoup.workerPath.empty());
            
            options.exit_cb = (uv_exit_cb)callback;
            options.file = MSConfig->params()->mediasoup.workerPath.c_str();
            options.args = &argv[0];
            
            auto ret = uv_spawn(_loop.get(), &_process, &options);
            if (ret != 0) {
                SRV_LOGE("%s", uv_strerror(ret));
                return;
            }
            
            _process.data = static_cast<void*>(this);
            
            SRV_LOGD("launched mediasoup worker with PID %d\n", _process.pid);
            
            //uv_unref((uv_handle_t*)&_process);
            
            _loop.asyncRun();
            
        }
        else {
            int argc = static_cast<int>(args.size());
            std::vector<char*> argv(argc);
            
            for (size_t i = 0; i < args.size(); ++i) {
                argv[i] = const_cast<char*>(args[i].c_str());
            }
            
            mediasoup_worker_run(argc,
                                 &argv[0],
                                 MEDIASOUP_VERSION.c_str(),
                                 0,//int consumerChannelFd,
                                 0,//int producerChannelFd,
                                 &Channel::channelRead,
                                 (void*)_channel.get(),
                                 &Channel::channelWrite,
                                 (void*)_channel.get()
                                 );
            close();
        }
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
        
        _channel->notificationSignal.disconnect_all();
        
        _channel->close();
        
        _webRtcServerControllers.for_each([](const auto& item){
            item->onWorkerClosed();
            item->closeSignal.disconnect_all();
        });
        
        _webRtcServerControllers.clear();
        
        _routerControllers.for_each([](const auto& item) {
            item->onWorkerClosed();
            item->closeSignal.disconnect_all();
        });
        
        _routerControllers.clear();
        
        this->closeSignal();
    }

    std::shared_ptr<IWebRtcServerController> WorkerController::webRtcServerController()
    {
        std::shared_ptr<IWebRtcServerController> controller;
        _webRtcServerControllers.for_each([&controller](const auto& item){
            controller = item;
            return true;
        });
        
        return controller;
    }

    std::shared_ptr<WorkerDump> WorkerController::dump()
    {
        SRV_LOGD("dump()");
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = _channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     "",
                                                     FBS::Request::Method::WORKER_DUMP);
        
        auto respData = _channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto workerDumpResponse = response->body_as_Worker_DumpResponse();

        return parseWorkerDumpResponse(workerDumpResponse);
    }

    std::shared_ptr<WorkerResourceUsage> WorkerController::getResourceUsage()
    {
        SRV_LOGD("getResourceUsage()");
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = _channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     "",
                                                     FBS::Request::Method::WORKER_GET_RESOURCE_USAGE);
   
        auto respData = _channel->request(reqId, reqData);
        
        auto usage = std::make_shared<WorkerResourceUsage>();
        
        auto message = FBS::Message::GetMessage(respData.data());
        
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
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = _channel->genRequestId();
        
        std::vector<::flatbuffers::Offset<::flatbuffers::String>> logTags_;
        for (const auto& item : logTags) {
            logTags_.emplace_back(builder.CreateString(item));
        }
        
        auto reqOffset = FBS::Worker::CreateUpdateSettingsRequestDirect(builder, logLevel.c_str(), &logTags_);
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     "",
                                                     FBS::Request::Method::WORKER_UPDATE_SETTINGS,
                                                     FBS::Request::Body::Worker_UpdateSettingsRequest,
                                                     reqOffset);
        
        _channel->request(reqId, reqData);
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
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = _channel->genRequestId();
        
        std::string webRtcServerId = uuid::uuidv4();
        
        std::vector<flatbuffers::Offset<FBS::Transport::ListenInfo>> infos;
        
        for (auto& info : listenInfos) {
            auto portRange = FBS::Transport::CreatePortRange(builder, info.portRange.min, info.portRange.max);
            
            auto socketFlags = FBS::Transport::CreateSocketFlags(builder, info.flags.ipv6Only, info.flags.udpReusePort);
            
            auto info_ = FBS::Transport::CreateListenInfoDirect(builder,
                                                                info.protocol == "udp" ? FBS::Transport::Protocol::UDP : FBS::Transport::Protocol::TCP,
                                                                info.ip.c_str(),
                                                                info.announcedIp.c_str(),
                                                                info.port,
                                                                portRange,
                                                                socketFlags,
                                                                info.sendBufferSize,
                                                                info.recvBufferSize
                                                                );
            infos.emplace_back(info_);
        }
        
        auto reqOffset = FBS::Worker::CreateCreateWebRtcServerRequestDirect(builder, webRtcServerId.c_str(), &infos);
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     "",
                                                     FBS::Request::Method::WORKER_CREATE_WEBRTCSERVER,
                                                     FBS::Request::Body::Worker_CreateWebRtcServerRequest,
                                                     reqOffset);
        
        _channel->request(reqId, reqData);

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
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = _channel->genRequestId();
        
        auto reqOffset = FBS::Worker::CreateCreateRouterRequestDirect(builder, internal.routerId.c_str());

        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     "",
                                                     FBS::Request::Method::WORKER_CREATE_ROUTER,
                                                     FBS::Request::Body::Worker_CreateRouterRequest,
                                                     reqOffset);

        _channel->request(reqId, reqData);
        
        RouterData data;
        data.rtpCapabilities = rtpCapabilities;

        routerController = std::make_shared<RouterController>(internal,
                                                              data,
                                                              _channel,
                                                              appData);
        routerController->init();
        
        _routerControllers.emplace(routerController);
        
        routerController->closeSignal.connect(&WorkerController::onRouterClose, shared_from_this());
        
        this->newRouterSignal(routerController);
        
        return routerController;
    }

    void WorkerController::onWebRtcServerClose(std::shared_ptr<IWebRtcServerController> controller)
    {
        _webRtcServerControllers.erase(controller);
    }

    void WorkerController::onRouterClose(std::shared_ptr<IRouterController> controller)
    {
        _routerControllers.erase(controller);
    }

    void WorkerController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto channel = _channel;
        if (!channel) {
            return;
        }
        
        _channel->notificationSignal.connect(&WorkerController::onChannel, shared_from_this());
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
        
        if (auto liburing = response->liburing()) {
            workerDump->liburing = std::make_shared<WorkerDump::LibUring>();
            workerDump->liburing->sqeMissCount = liburing->sqeMissCount();
            workerDump->liburing->sqeProcessCount = liburing->sqeProcessCount();
            workerDump->liburing->userDataMissCount = liburing->userDataMissCount();
        }
        
        return workerDump;
    }

    void to_json(nlohmann::json& j, const WorkerSettings& st)
    {
        j["logLevel"] = st.logLevel;
        j["logTags"] = st.logTags;
        //j["rtcMinPort"] = st.rtcMinPort;
        //j["rtcMaxPort"] = st.rtcMaxPort;
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
        //if (j.contains("rtcMinPort")) {
        //    j.at("rtcMinPort").get_to(st.rtcMinPort);
        //}
        //if (j.contains("rtcMaxPort")) {
        //    j.at("rtcMaxPort").get_to(st.rtcMaxPort);
        //}
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
