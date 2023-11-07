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
#include "common.hpp"
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "types.h"
#include "worker_controller.h"

namespace srv {
    
    class Engine : public std::enable_shared_from_this<Engine>
    {
    public:
        ~Engine();
        
        static std::shared_ptr<Engine>& sharedInstance();

        void init(const std::string& configFileName);
        
        void run();
        
        void destroy();
        
        std::shared_ptr<WorkerController> getWorkerController();
        
    public:
        sigslot::signal<std::shared_ptr<WorkerController>> _newWorkerSignal;
        
    private:
        Engine();

        Engine(const Engine&) = delete;

        Engine& operator=(const Engine&) = delete;

        Engine(Engine&&) = delete;

        Engine& operator=(Engine&&) = delete;
        
    private:
        std::shared_ptr<WorkerController> createWorkerController();

    private:
        asio::static_thread_pool _threadPool {1};
        
        std::string _configFileName;
        
        std::shared_ptr<WorkerSettings> _workerSettings;
        
        std::shared_ptr<WebRtcServerOptions> _webRtcServerOptions;
        
        std::mutex _workerControllersMutex;
        
        std::size_t _nextWorkerIdx = 0;
        
        std::vector<std::shared_ptr<WorkerController>> _workerControllers;
    };

}

#ifndef MSEngine
#define MSEngine srv::Engine::sharedInstance()
#endif
