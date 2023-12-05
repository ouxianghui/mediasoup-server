/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <vector>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "types.h"

namespace srv {
 
    struct WebRtcServerListenInfo : TransportListenInfo {};

    struct IpPort
    {
        std::string ip;
        uint16_t port;
    };

    struct IceUserNameFragment
    {
        std::string localIceUsernameFragment;
        std::string webRtcTransportId;
    };

    struct TupleHash
    {
        uint64_t tupleHash;
        std::string webRtcTransportId;
    };

    struct WebRtcServerDump
    {
        std::string id;
        std::vector<IpPort> udpSockets;
        std::vector<IpPort> tcpServers;
        std::vector<std::string> webRtcTransportIds;
        std::vector<IceUserNameFragment> localIceUsernameFragments;
        std::vector<TupleHash> tupleHashes;
    };

    struct WebRtcServerInternal
    {
        std::string webRtcServerId;
    };

    class WebRtcTransportController;

    class IWebRtcServerController
    {
    public:
        virtual ~IWebRtcServerController() = default;
        
        virtual void init() = 0;
        
        virtual void destroy() = 0;
        
        virtual const std::string& id() = 0;
        
        virtual void setAppData(const nlohmann::json& data) = 0;
        
        virtual const nlohmann::json& appData() = 0;
        
        virtual void close() = 0;
        
        virtual bool closed() = 0;

        virtual void handleWebRtcTransport(const std::shared_ptr<WebRtcTransportController>& controller) = 0;
        
        virtual std::shared_ptr<WebRtcServerDump> dump() = 0;
        
        virtual void onWorkerClosed() = 0;
        
    public:
        sigslot::signal<> workerCloseSignal;
        
        sigslot::signal<std::shared_ptr<IWebRtcServerController>> closeSignal;
        
        sigslot::signal<std::shared_ptr<WebRtcTransportController>> webrtcTransportHandledSignal;
        
        sigslot::signal<std::shared_ptr<WebRtcTransportController>> webrtcTransportUnhandledSignal;
    };

}
