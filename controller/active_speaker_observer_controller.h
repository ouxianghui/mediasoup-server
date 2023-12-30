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
#include "rtp_observer_controller.h"
#include "FBS/notification.h"
#include "FBS/activeSpeakerObserver.h"

namespace srv {

    struct ActiveSpeakerObserverOptions
    {
        int32_t interval;

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    class IProducerController;

    struct ActiveSpeakerObserverDominantSpeaker
    {
        /**
         * The audio Producer instance.
         */
        std::shared_ptr<IProducerController> producerController;
    };

    struct RtpObserverObserverConstructorOptions : RtpObserverConstructorOptions {};


    class ActiveSpeakerObserverController : public RtpObserverController
    {
    public:
        ActiveSpeakerObserverController(const std::shared_ptr<RtpObserverObserverConstructorOptions>& options);
        
        ~ActiveSpeakerObserverController();
        
        void init();
        
        void destroy();
        
    public:
        sigslot::signal<const ActiveSpeakerObserverDominantSpeaker&> dominantSpeakerSignal;
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data);
    };

}
