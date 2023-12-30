/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <vector>
#include <atomic>
#include <string>
#include <functional>
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "types.h"

namespace srv {
    
    class IProducerController;

    class IRtpObserverController
    {
    public:
        virtual ~IRtpObserverController() = default;
        
        virtual const std::string& id() = 0;
        
        virtual bool paused() = 0;
        
        virtual bool closed() = 0;
        
        virtual void setAppData(const nlohmann::json& data) = 0;
        
        virtual const nlohmann::json& appData() = 0;
        
        virtual void close() = 0;
        
        virtual void pause() = 0;
        
        virtual void resume() = 0;
        
        virtual void addProducer(const std::string& producerId) = 0;
        
        virtual void removeProducer(const std::string& producerId) = 0;
        
        virtual void onRouterClosed() = 0;
        
    public:
        sigslot::signal<> routerCloseSignal;
        
        sigslot::signal<> closeSignal;
        
        sigslot::signal<> pauseSignal;
        
        sigslot::signal<> resumeSignal;
        
        sigslot::signal<std::shared_ptr<IProducerController>> addProducerSignal;
        
        sigslot::signal<std::shared_ptr<IProducerController>> removeProducerSignal;
    };

}
