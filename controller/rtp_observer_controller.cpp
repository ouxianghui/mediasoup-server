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
#include "FBS/rtpObserver.h"
#include "message_builder.h"

namespace srv {

    RtpObserverController::RtpObserverController(const std::shared_ptr<RtpObserverConstructorOptions>& options)
    : _options(options)
    {
        SRV_LOGD("RtpObserverController()");
        _internal = options->internal;
        _channel = options->channel;
        _getProducerController = options->getProducerController;
        _appData = options->appData;
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
    
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqOffset = FBS::Router::CreateCloseRtpObserverRequestDirect(builder, _internal.rtpObserverId.c_str());
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.routerId,
                                                     FBS::Request::Method::ROUTER_CLOSE_RTPOBSERVER,
                                                     FBS::Request::Body::Router_CloseRtpObserverRequest,
                                                     reqOffset);
        
        channel->request(reqId, reqData);
        
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

        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.rtpObserverId,
                                                     FBS::Request::Method::RTPOBSERVER_PAUSE);

        channel->request(reqId, reqData);
        
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

        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.rtpObserverId,
                                                     FBS::Request::Method::RTPOBSERVER_RESUME);

        channel->request(reqId, reqData);
        
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

        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqOffset = FBS::RtpObserver::CreateAddProducerRequestDirect(builder, producerId.c_str());
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.rtpObserverId,
                                                     FBS::Request::Method::RTPOBSERVER_ADD_PRODUCER,
                                                     FBS::Request::Body::RtpObserver_AddProducerRequest,
                                                     reqOffset);
        
        channel->request(reqId, reqData);
        
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

        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqOffset = FBS::RtpObserver::CreateRemoveProducerRequestDirect(builder, producerId.c_str());
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.rtpObserverId,
                                                     FBS::Request::Method::RTPOBSERVER_REMOVE_PRODUCER,
                                                     FBS::Request::Body::RtpObserver_RemoveProducerRequest,
                                                     reqOffset);
        
        channel->request(reqId, reqData);
        
        this->removeProducerSignal(producer);
    }

}
