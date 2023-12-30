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
#include "interface/i_rtp_observer_controller.h"

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

    class Channel;
    class IProducerController;

    struct RtpObserverConstructorOptions
    {
        RtpObserverObserverInternal internal;
        std::shared_ptr<Channel> channel;
        nlohmann::json appData;
        std::function<std::shared_ptr<IProducerController>(const std::string&)> getProducerController;
    };
    
    class RtpObserverController : public IRtpObserverController, public std::enable_shared_from_this<RtpObserverController>
    {
    public:
        RtpObserverController(const std::shared_ptr<RtpObserverConstructorOptions>& options);
        
        virtual ~RtpObserverController();
        
        const std::string& id() override { return _internal.rtpObserverId; }

        bool paused() override { return _paused; }

        bool closed() override { return _closed; }
        
        void setAppData(const nlohmann::json& data) override { _appData = data; }
        
        const nlohmann::json& appData() override { return _appData; }
        
        void close() override;
        
        void onRouterClosed() override;
        
        void pause() override;
        
        void resume() override;
        
        void addProducer(const std::string& producerId) override;

        void removeProducer(const std::string& producerId) override;
        
    protected:
        std::shared_ptr<RtpObserverConstructorOptions> _options;
        
        // Internal data.
        RtpObserverObserverInternal _internal;

        std::weak_ptr<Channel> _channel;

        // Closed flag.
        std::atomic_bool _closed { false };
        
        // Paused flag.
        std::atomic_bool _paused { false };

        // Custom app data.
        nlohmann::json _appData;

        std::function<std::shared_ptr<IProducerController>(const std::string&)> _getProducerController;
    };

}
