/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "video_sharing_controller.hpp"
#include "interface/i_producer_controller.h"
#include "peer.hpp"

VideoSharingController::VideoSharingController()
{
    
}

VideoSharingController::~VideoSharingController()
{
    
}

void VideoSharingController::init()
{
    
}

void VideoSharingController::destroy()
{
    
}

std::string VideoSharingController::id()
{
    return _producerController ? _producerController->id() : "";
}

void VideoSharingController::attach(const std::shared_ptr<Peer>& peer, const std::shared_ptr<srv::IProducerController>& producerController)
{
    _peer = peer;
    _producerController = producerController;
}

void VideoSharingController::detach()
{
    _peer = nullptr;
    _producerController = nullptr;
}

bool VideoSharingController::attached()
{
    return _peer && _producerController;
}

void VideoSharingController::pause()
{
    if (_producerController && !_producerController->paused()) {
        _producerController->pause();
    }
}

void VideoSharingController::resume()
{
    if (_producerController && _producerController->paused()) {
        _producerController->resume();
    }
}

bool VideoSharingController::paused()
{
    return _producerController ? _producerController->paused() : true;
}

void VideoSharingController::close()
{
    if (_producerController && !_producerController->closed()) {
        _producerController->close();
    }
}

bool VideoSharingController::closed()
{
    return _producerController ? _producerController->closed() : true;
}

const std::shared_ptr<Peer>& VideoSharingController::peer()
{
    return _peer;
}

const std::shared_ptr<srv::IProducerController>& VideoSharingController::producerController()
{
    return _producerController;
}
