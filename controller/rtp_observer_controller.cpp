/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "rtp_observer_controller.h"
#include "srv_logger.h"
#include "types.h"
#include "uuid.h"
#include "channel.h"
#include "payload_channel.h"


namespace srv {

    RtpObserverController::RtpObserverController(const std::shared_ptr<RtpObserverConstructorOptions>& options)
    : _options(options)
    {
        SRV_LOGD("RtpObserverController()");
    }

    RtpObserverController::~RtpObserverController()
    {
        SRV_LOGD("~RtpObserverController()");
    }

    void RtpObserverController::close()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");

        _closed = true;

        // Remove notification subscriptions.
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        channel->notificationSignal.disconnect(shared_from_this());
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->notificationSignal.disconnect(shared_from_this());
        
        nlohmann::json reqData;
        reqData["rtpObserverId"] = _internal.rtpObserverId;
        
        channel->request("router.closeRtpObserver", _internal.routerId, reqData.dump());
        
        this->closeSignal();
    }

    void RtpObserverController::onRouterClosed()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("onRouterClosed()");

        _closed = true;

        // Remove notification subscriptions.
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        channel->notificationSignal.disconnect(shared_from_this());
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->notificationSignal.disconnect(shared_from_this());
        
        this->routerCloseSignal();
        
        this->closeSignal();
    }

    void RtpObserverController::pause()
    {
        SRV_LOGD("pause()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        bool wasPaused = _paused;

        channel->request("rtpObserver.pause", _internal.rtpObserverId, "{}");

        _paused = true;

        // Emit observer event.
        if (!wasPaused) {
            this->pauseSignal();
        }
    }

    void RtpObserverController::resume()
    {
        SRV_LOGD("resume()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        bool wasPaused = _paused;

        channel->request("rtpObserver.resume", _internal.rtpObserverId, "{}");

        _paused = false;

        // Emit observer event.
        if (wasPaused) {
            this->resumeSignal();
        }
    }

    void RtpObserverController::addProducer(const std::string& producerId)
    {
        SRV_LOGD("addProducer()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        if (!_getProducerController) {
            return;
        }
        
        if (producerId.empty()) {
            return;
        }
        
        auto producer = _getProducerController(producerId);

        if (!producer)
        {
            SRV_LOGE("Producer with id '%s' not found", producerId.c_str());
            return;
        }

        nlohmann::json reqData;
        reqData["producerId"] = producerId;

        channel->request("rtpObserver.addProducer", _internal.rtpObserverId, reqData.dump());

        this->addProducerSignal(producer);
    }

    void RtpObserverController::removeProducer(const std::string& producerId)
    {
        SRV_LOGD("removeProducer()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        if (!_getProducerController) {
            return;
        }
        
        if (producerId.empty()) {
            return;
        }
        
        auto producer = _getProducerController(producerId);

        if (!producer)
        {
            SRV_LOGE("Producer with id '%s' not found", producerId.c_str());
            return;
        }

        nlohmann::json reqData;
        reqData["producerId"] = producerId;

        channel->request("rtpObserver.removeProducer", _internal.rtpObserverId, reqData.dump());

        this->removeProducerSignal(producer);
    }

}
