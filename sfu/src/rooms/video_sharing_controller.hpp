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
#include <atomic>
#include <unordered_map>
#include "nlohmann/json.hpp"

namespace srv {
    class IProducerController;
}

class Peer;

class VideoSharingController : public std::enable_shared_from_this<VideoSharingController> {
public:
    VideoSharingController();
    
    ~VideoSharingController();
    
    void init();
    
    void destroy();
    
    std::string id();
    
    void attach(const std::shared_ptr<Peer>& peer, const std::shared_ptr<srv::IProducerController>& producerController);
    
    void detach();
    
    bool attached();
    
    void pause();
    
    void resume();
    
    bool paused();
    
    void close();

    bool closed();
    
    const std::shared_ptr<Peer>& peer();
    
    const std::shared_ptr<srv::IProducerController>& producerController();
    
private:
    std::shared_ptr<Peer> _peer;
    
    std::shared_ptr<srv::IProducerController> _producerController;
};
