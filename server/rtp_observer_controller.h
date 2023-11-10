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
#include <atomic>
#include <string>
#include <functional>
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "types.h"
#include "producer_controller.h"
#include "consumer_controller.h"

namespace srv {

    struct RtpObserverObserverInternal
    {
        std::string routerId;
        std::string rtpObserverId;
    };

    struct RtpObserverAddRemoveProducerOptions
    {
        /**
         * The id of the Producer to be added or removed.
         */
        std::string producerId;
    };

    struct RtpObserverConstructorOptions
    {
        RtpObserverObserverInternal internal;
        std::shared_ptr<Channel> channel;
        std::shared_ptr<PayloadChannel> payloadChannel;
        nlohmann::json appData;
        std::function<std::shared_ptr<ProducerController>(const std::string&)> getProducerController;
    };

    class Channel;
    class PayloadChannel;
    
    class RtpObserverController : public std::enable_shared_from_this<RtpObserverController>
    {
    public:
        RtpObserverController(const std::shared_ptr<RtpObserverConstructorOptions>& options);
        
        virtual ~RtpObserverController();
        
        const std::string& id() { return _internal.rtpObserverId; }

        bool paused() { return _paused; }

        bool closed() { return _closed; }
        
        void setAppData(const nlohmann::json& data) { _appData = data; }
        
        const nlohmann::json& appData() { return _appData; }
        
        void close();
        
        void onRouterClosed();
        
        void pause();
        
        void resume();
        
        void addProducer(const std::string& producerId);

        void removeProducer(const std::string& producerId);
        
    public:
        sigslot::signal<> routerCloseSignal;
        sigslot::signal<> closeSignal;
        sigslot::signal<> pauseSignal;
        sigslot::signal<> resumeSignal;
        sigslot::signal<std::shared_ptr<ProducerController>> addProducerSignal;
        sigslot::signal<std::shared_ptr<ProducerController>> removeProducerSignal;
        
    protected:
        std::shared_ptr<RtpObserverConstructorOptions> _options;
        
        // Internal data.
        RtpObserverObserverInternal _internal;

        std::weak_ptr<Channel> _channel;

        // PayloadChannel instance.
        std::weak_ptr<PayloadChannel> _payloadChannel;

        // Closed flag.
        std::atomic_bool _closed { false };
        
        // Paused flag.
        std::atomic_bool _paused { false };

        // Custom app data.
        nlohmann::json _appData;

        std::function<std::shared_ptr<ProducerController>(const std::string&)> _getProducerController;
    };

}
