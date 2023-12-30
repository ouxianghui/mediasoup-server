/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "engine.h"
#include <mutex>
#include <future>
#include "DepLibUV.hpp"
#include "srv_logger.h"
#include "config.h"
#include "interface/i_worker_controller.h"
#include "worker_controller.h"
#include "message_builder.h"

namespace srv {
    std::shared_ptr<Engine>& Engine::sharedInstance()
    {
        static std::shared_ptr<Engine> _instance;
        static std::once_flag ocf;
        std::call_once(ocf, []() {
            _instance.reset(new Engine());
        });
        return _instance;
    }

    Engine::Engine()
    : _workerSettings(std::make_shared<WorkerSettings>())
    , _webRtcServerOptions(std::make_shared<WebRtcServerOptions>())
    {
        SRV_LOGD("Engine()");
    }

    Engine::~Engine()
    {
        SRV_LOGD("~Engine()");
    }

    void Engine::init(const std::string& configFileName)
    {
        _configFileName = configFileName;
        
        MSConfig->init(configFileName);
        
        auto params = MSConfig->params();
        
        *_workerSettings = params->mediasoup.workerSettings;
        
        *_webRtcServerOptions = params->mediasoup.webRtcServerOptions;
        
        MessageBuilder::setSizePrefix(params->mediasoup.multiprocess);
    }

    void Engine::run()
    {
        createWorkerControllers();
    }

    void Engine::destroy()
    {
        _workerSettings = nullptr;
        
        _webRtcServerOptions = nullptr;
        
        _workerControllers.clear();
        
        MSConfig->destroy();
    }

    std::shared_ptr<IWorkerController> Engine::getWorkerController()
    {
        std::shared_ptr<IWorkerController> worker = _workerControllers[_nextWorkerIdx];

        if (++_nextWorkerIdx == _workerControllers.size()) {
            _nextWorkerIdx = 0;
        }

        return worker;
    }

    void Engine::createWorkerControllers()
    {
        SRV_LOGD("createWorker()");
        
        if (!_workerSettings) {
            SRV_LOGE("_workerSettings must not be null");
            return;
        }

        if (MSConfig->params()->mediasoup.multiprocess) {
            auto workerNum = MSConfig->params()->mediasoup.numWorkers;
            for (int i = 0; i < workerNum; ++i) {
                std::shared_ptr<IWorkerController> workerController = std::make_shared<WorkerController>(_workerSettings);
                workerController->init();
                
                _workerControllers.push_back(workerController);
                
                workerController->startSignal.connect([wself = std::weak_ptr<Engine>(shared_from_this()),
                                                       wWorkerController = std::weak_ptr<IWorkerController>(workerController),
                                                       portIncrement = (_workerControllers.size() - 1)]() {
                    auto self = wself.lock();
                    if (!self) {
                        return;
                    }
                    
                    auto workerController = wWorkerController.lock();
                    if (!workerController) {
                        return;
                    }
                    
                    asio::post(self->_threadPool.get_executor(), [wself, wWorkerController, portIncrement]() {
                        auto self = wself.lock();
                        if (!self) {
                            return;
                        }
                        auto workerController = wWorkerController.lock();
                        if (!workerController) {
                            return;
                        }
                        auto options = std::make_shared<WebRtcServerOptions>();
                        *options = *self->_webRtcServerOptions;
                        for (auto& info : options->listenInfos) {
                            info.port += portIncrement;
                        }
                        workerController->createWebRtcServerController(options, nlohmann::json());
                    });
                    
                    self->newWorkerSignal(workerController);
                });
                
                workerController->runWorker();
            }
        }
        else {
            auto workerController = std::make_shared<WorkerController>(_workerSettings);
            workerController->init();
            _workerControllers.push_back(workerController);
            
            workerController->startSignal.connect([wself = std::weak_ptr<Engine>(shared_from_this()), wWorkerController = std::weak_ptr<IWorkerController>(workerController)]() {
                auto self = wself.lock();
                if (!self) {
                    return;
                }
                
                auto workerController = wWorkerController.lock();
                if (!workerController) {
                    return;
                }
                
                asio::post(self->_threadPool.get_executor(), [wself, wWorkerController]() {
                    auto self = wself.lock();
                    if (!self) {
                        return;
                    }
                    auto workerController = wWorkerController.lock();
                    if (!workerController) {
                        return;
                    }
                    workerController->createWebRtcServerController(self->_webRtcServerOptions, nlohmann::json());
                });
                
                self->newWorkerSignal(workerController);
            });
            
            workerController->runWorker();
        }
    }
}
