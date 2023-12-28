/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "active_speaker_observer_controller.h"
#include "srv_logger.h"
#include "channel.h"
#include "FBS/message.h"

namespace srv {

    ActiveSpeakerObserverController::ActiveSpeakerObserverController(const std::shared_ptr<RtpObserverObserverConstructorOptions>& options)
    : RtpObserverController(options)
    {
        SRV_LOGD("ActiveSpeakerObserverController()");
    }

    ActiveSpeakerObserverController::~ActiveSpeakerObserverController()
    {
        SRV_LOGD("~ActiveSpeakerObserverController()");
    }

    void ActiveSpeakerObserverController::init()
    {
        SRV_LOGD("init()");
        handleWorkerNotifications();
    }

    void ActiveSpeakerObserverController::destroy()
    {
        SRV_LOGD("destroy()");
    }

    void ActiveSpeakerObserverController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto self = std::dynamic_pointer_cast<ActiveSpeakerObserverController>(RtpObserverController::shared_from_this());
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        channel->notificationSignal.connect(&ActiveSpeakerObserverController::onChannel, self);
    }

    void ActiveSpeakerObserverController::onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data)
    {
        //SRV_LOGD("onChannel()");
     
        if (targetId != _internal.rtpObserverId) {
            return;
        }
        
        if (event == FBS::Notification::Event::ACTIVESPEAKEROBSERVER_DOMINANT_SPEAKER) {
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_ActiveSpeakerObserver_DominantSpeakerNotification()) {
                auto producerId = nf->producerId()->str();
                auto producer = _getProducerController(producerId);
                ActiveSpeakerObserverDominantSpeaker speaker { producer };
                this->dominantSpeakerSignal(speaker);
            }
        }
        else {
            SRV_LOGD("ignoring unknown event %u", (uint8_t)event);
        }
    }

}
