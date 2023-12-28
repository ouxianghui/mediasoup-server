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
#include "FBS/message.h"

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

    void AudioLevelObserverController::onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data)
    {
        //SRV_LOGD("onChannel()");
     
        if (targetId != _internal.rtpObserverId) {
            return;
        }
        
        if (event == FBS::Notification::Event::AUDIOLEVELOBSERVER_VOLUMES) {
            std::vector<AudioLevelObserverVolume> aloVolumes;
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_AudioLevelObserver_VolumesNotification()) {
                for (const auto& item : *nf->volumes()) {
                    auto volume = parseVolume(item);
                    AudioLevelObserverVolume vol;
                    vol.producerController = _getProducerController(volume->producerId);
                    vol.volume = volume->volume;
                    aloVolumes.emplace_back(vol);
                }
                if (aloVolumes.size() > 0) {
                    this->volumesSignal(aloVolumes);
                }
            }
        }
        else if (event == FBS::Notification::Event::AUDIOLEVELOBSERVER_SILENCE) {
            this->silenceSignal();
        }
        else {
            SRV_LOGD("ignoring unknown event %u", (uint8_t)event);
        }
    }

    std::shared_ptr<Volume> parseVolume(const FBS::AudioLevelObserver::Volume* binary)
    {
        auto volume = std::make_shared<Volume>();
        
        volume->producerId = binary->producerId()->c_str();
        volume->volume = binary->volume();
        
        return volume;
    }
}
