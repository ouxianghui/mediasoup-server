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

namespace srv {

    struct AudioLevelObserverOptions
    {
        /**
         * Maximum number of entries in the 'volumes”' event. Default 1.
         */
        int32_t maxEntries = 1;

        /**
         * Minimum average volume (in dBvo from -127 to 0) for entries in the
         * 'volumes' event.    Default -80.
         */
        int32_t threshold = -80;

        /**
         * Interval in ms for checking audio volumes. Default 1000.
         */
        int32_t interval = 1000;

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    struct AudioLevelObserverVolume
    {
        /**
         * The audio Producer instance.
         */
        std::shared_ptr<ProducerController> producerController;

        /**
         * The average volume (in dBvo from -127 to 0) of the audio Producer in the
         * last interval.
         */
        int32_t volume;
    };

    struct AudioLevelObserverConstructorOptions : RtpObserverConstructorOptions {};

    class AudioLevelObserverController : public RtpObserverController
    {
    public:
        AudioLevelObserverController(const std::shared_ptr<AudioLevelObserverConstructorOptions>& options);
        
        ~AudioLevelObserverController();
        
        void init();
        
        void destroy();
        
    public:
        sigslot::signal<const std::vector<AudioLevelObserverVolume>&> _volumesSignal;
        sigslot::signal<> _silenceSignal;
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, const std::string& event, const std::string& data);
    };

}
