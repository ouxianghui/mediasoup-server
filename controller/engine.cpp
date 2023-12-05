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
    }

    void Engine::run()
    {
         createWorkerController();
    }

    void Engine::destroy()
    {
        _workerSettings = nullptr;
        
        _webRtcServerOptions = nullptr;
        
        {
            std::lock_guard<std::mutex> lock(_workerControllersMutex);
            _workerControllers.clear();
        }
        
        MSConfig->destroy();
    }

    std::shared_ptr<IWorkerController> Engine::getWorkerController()
    {
        std::lock_guard<std::mutex> lock(_workerControllersMutex);
        std::shared_ptr<IWorkerController> worker = _workerControllers[_nextWorkerIdx];

        if (++_nextWorkerIdx == _workerControllers.size()) {
            _nextWorkerIdx = 0;
        }

        return worker;
    }

    std::shared_ptr<IWorkerController> Engine::createWorkerController()
    {
        SRV_LOGD("createWorker()");
        
        std::shared_ptr<IWorkerController> workerController;
        
        if (!_workerSettings) {
            return workerController;
        }

        {
            std::lock_guard<std::mutex> lock(_workerControllersMutex);
            workerController = std::make_shared<WorkerController>(_workerSettings);
            workerController->init();
            _workerControllers.emplace_back(workerController);
        }
        
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
        
        return workerController;
    }
}
