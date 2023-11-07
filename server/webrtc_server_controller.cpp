/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "webrtc_server_controller.h"
#include "srv_logger.h"
#include "channel.h"
#include "webrtc_transport_controller.h"

namespace srv {

    WebRtcServerController::WebRtcServerController(const WebRtcServerInternal& internal, std::weak_ptr<Channel> channel, const nlohmann::json& appData)
    : _id(internal.webRtcServerId)
    , _channel(channel)
    , _appData(appData)
    {
        SRV_LOGD("WebRtcServerController()");
    }
        
    WebRtcServerController::~WebRtcServerController()
    {
        SRV_LOGD("~WebRtcServerController()");
    }

    void WebRtcServerController::init()
    {
        SRV_LOGD("init()");
    }

    void WebRtcServerController::destroy()
    {
        SRV_LOGD("destroy()");
    }

    nlohmann::json WebRtcServerController::dump()
    {
        SRV_LOGD("dump()");

        auto channel = _channel.lock();
        if (!channel) {
            return nlohmann::json();
        }
        
        return channel->request("webRtcServer.dump", _id, "{}");
    }

    void WebRtcServerController::onWorkerClosed()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("workerClosed()");

        _closed = true;

        {
            std::lock_guard<std::mutex> lock(_webRtcTransportsMutex);
            // NOTE: No need to close WebRtcTransports since they are closed by their
            // respective Router parents.
            _webRtcTransportMap.clear();
        }

        _workerCloseSignal();

        // Emit observer event.
        _closeSignal(shared_from_this());
    }

    void WebRtcServerController::close()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");

        _closed = true;

        nlohmann::json reqData;
        reqData["webRtcServerId"] = _id;

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        channel->request("worker.closeWebRtcServer", "", reqData.dump());

        {
            std::lock_guard<std::mutex> lock(_webRtcTransportsMutex);
            
            // Close every WebRtcTransport.
            for (const auto& webRtcTransport : _webRtcTransportMap) {
                auto transport = webRtcTransport.second;
                
                transport->onListenServerClosed();
                
                _webrtcTransportUnhandledSignal(transport);
            }
            _webRtcTransportMap.clear();
        }

        _closeSignal(shared_from_this());
    }

    void WebRtcServerController::handleWebRtcTransport(const std::shared_ptr<WebRtcTransportController>& transportController)
    {
        {
            std::lock_guard<std::mutex> lock(_webRtcTransportsMutex);
            _webRtcTransportMap[transportController->id()] = transportController;
        }

        // Emit observer event.
        _webrtcTransportHandledSignal(transportController);
        
        transportController->_closeSignal.connect(&WebRtcServerController::onWebRtcTransportClose, shared_from_this());
    }

    void WebRtcServerController::onWebRtcTransportClose(const std::string& id_)
    {
        std::lock_guard<std::mutex> lock(_webRtcTransportsMutex);
        if (_webRtcTransportMap.find(id_) != _webRtcTransportMap.end()) {
            auto controller = _webRtcTransportMap[id_];
            _webrtcTransportUnhandledSignal(controller);
            _webRtcTransportMap.erase(id_);
        }
    }
}

namespace srv
{
    void to_json(nlohmann::json& js, const WebRtcServerListenInfo& st)
    {
        js["protocol"] = st.protocol;
        js["ip"] = st.ip;
        js["announcedIp"] = st.announcedIp;
        js["port"] = st.port;
    }

    void from_json(const nlohmann::json& j, WebRtcServerListenInfo& st)
    {
        if (j.contains("protocol")) {
            j.at("protocol").get_to(st.protocol);
        }
        if (j.contains("ip")) {
            j.at("ip").get_to(st.ip);
        }
        if (j.contains("announcedIp")) {
            j.at("announcedIp").get_to(st.announcedIp);
        }
        if (j.contains("port")) {
            j.at("port").get_to(st.port);
        }
    }

    void to_json(nlohmann::json& js, const WebRtcServerOptions& st)
    {
        js["listenInfos"] = st.listenInfos;
        js["appData"] = st.appData;
    }

    void from_json(const nlohmann::json& j, WebRtcServerOptions& st)
    {
        if (j.contains("listenInfos")) {
            j.at("listenInfos").get_to(st.listenInfos);
        }
        if (j.contains("appData")) {
            j.at("appData").get_to(st.appData);
        }
    }

}
