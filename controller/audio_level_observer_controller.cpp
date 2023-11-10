/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "audio_level_observer_controller.h"
#include "srv_logger.h"
#include "channel.h"

namespace srv {

    AudioLevelObserverController::AudioLevelObserverController(const std::shared_ptr<AudioLevelObserverConstructorOptions>& options)
    : RtpObserverController(options)
    {
        SRV_LOGD("AudioLevelObserverController()");
    }

    AudioLevelObserverController::~AudioLevelObserverController()
    {
        SRV_LOGD("~AudioLevelObserverController()");
    }

    void AudioLevelObserverController::init()
    {
        SRV_LOGD("init()");
        handleWorkerNotifications();
    }

    void AudioLevelObserverController::destroy()
    {
        SRV_LOGD("destroy()");
    }

    void AudioLevelObserverController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto self = std::dynamic_pointer_cast<AudioLevelObserverController>(RtpObserverController::shared_from_this());
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        channel->notificationSignal.connect(&AudioLevelObserverController::onChannel, self);
    }

    void AudioLevelObserverController::onChannel(const std::string& targetId, const std::string& event, const std::string& data)
    {
        SRV_LOGD("onChannel()");
     
        if (targetId != _internal.rtpObserverId) {
            return;
        }
        
        if (event == "volumes") {
            auto js = nlohmann::json::parse(data);
            if (js.is_array()) {
                std::vector<AudioLevelObserverVolume> volumes;
                for (const auto& item : js) {
                    AudioLevelObserverVolume vol;
                    std::string producerId = item["producerId"];
                    vol.producerController = _getProducerController(producerId);
                    vol.volume = item["volume"];
                    volumes.push_back(vol);
                }
                if (volumes.size() > 0) {
                    this->volumesSignal(volumes);
                }
            }
        }
        else if (event == "silence") {
            this->silenceSignal();
        }
        else {
            SRV_LOGD("ignoring unknown event %s", event.c_str());
        }
    }
}
