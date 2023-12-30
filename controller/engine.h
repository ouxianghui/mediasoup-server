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
#include "threadsafe_vector.hpp"
#include "common.hpp"
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "types.h"

namespace srv {
    
    struct WorkerSettings;
    struct WebRtcServerOptions;
    class IWorkerController;

    class Engine : public std::enable_shared_from_this<Engine>
    {
    public:
        ~Engine();
        
        static std::shared_ptr<Engine>& sharedInstance();

        void init(const std::string& configFileName);
        
        void run();
        
        void destroy();
        
        std::shared_ptr<IWorkerController> getWorkerController();
        
    public:
        sigslot::signal<std::shared_ptr<IWorkerController>> newWorkerSignal;
        
    private:
        Engine();

        Engine(const Engine&) = delete;

        Engine& operator=(const Engine&) = delete;

        Engine(Engine&&) = delete;

        Engine& operator=(Engine&&) = delete;
        
    private:
        void createWorkerControllers();

    private:
        asio::static_thread_pool _threadPool {1};
        
        std::string _configFileName;
        
        std::shared_ptr<WorkerSettings> _workerSettings;
        
        std::shared_ptr<WebRtcServerOptions> _webRtcServerOptions;
        
        std::size_t _nextWorkerIdx = 0;
        
        std::threadsafe_vector<std::shared_ptr<IWorkerController>> _workerControllers;
    };

}

#ifndef MSEngine
#define MSEngine srv::Engine::sharedInstance()
#endif
