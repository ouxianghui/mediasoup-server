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
        
        auto self = dynamic_pointer_cast<ActiveSpeakerObserverController>(RtpObserverController::shared_from_this());
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        channel->_notificationSignal.connect(&ActiveSpeakerObserverController::onChannel, self);
    }

    void ActiveSpeakerObserverController::onChannel(const std::string& targetId, const std::string& event, const std::string& data)
    {
        SRV_LOGD("onChannel()");
     
        if (targetId != _internal.rtpObserverId) {
            return;
        }
        
        if (event == "dominantspeaker") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                auto producerId = js["producerId"];
                assert(_getProducerController);
                auto producer = _getProducerController(producerId);
                ActiveSpeakerObserverDominantSpeaker speaker { producer };
                _dominantSpeakerSignal(speaker);
            }
        }
        else {
            SRV_LOGD("ignoring unknown event %s", event.c_str());
        }
    }

}
