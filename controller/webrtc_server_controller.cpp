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
#include "message_builder.h"

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

    std::shared_ptr<WebRtcServerDump> WebRtcServerController::dump()
    {
        SRV_LOGD("dump()");

        auto channel = _channel.lock();
        if (!channel) {
            return nullptr;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _id,
                                                     FBS::Request::Method::WEBRTCSERVER_DUMP);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto dumpResponse = response->body_as_WebRtcServer_DumpResponse();
        
        return parseWebRtcServerDump(dumpResponse);
    }

    void WebRtcServerController::onWorkerClosed()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("workerClosed()");

        _closed = true;
        
        // NOTE: No need to close WebRtcTransports since they are closed by their
        // respective Router parents.
        _webRtcTransportMap.clear();

        this->workerCloseSignal();

        // Emit observer event.
        this->closeSignal(shared_from_this());
    }

    void WebRtcServerController::close()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");

        _closed = true;

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
     
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqOffset = FBS::Worker::CreateCloseWebRtcServerRequestDirect(builder, _id.c_str());
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _id,
                                                     FBS::Request::Method::WORKER_WEBRTCSERVER_CLOSE,
                                                     FBS::Request::Body::Worker_CloseWebRtcServerRequest,
                                                     reqOffset);

        channel->request(reqId, reqData);
        
        std::threadsafe_unordered_map<std::string, std::shared_ptr<WebRtcTransportController>> webRtcTransportMap = _webRtcTransportMap.value();
        
        // Close every WebRtcTransport.
        webRtcTransportMap.for_each ([this](const auto& webRtcTransport) {
            auto transport = webRtcTransport.second;
            transport->onWebRtcServerClosed();
            this->webrtcTransportUnhandledSignal(transport);
        });
        
        _webRtcTransportMap.clear();

        this->closeSignal(shared_from_this());
    }

    void WebRtcServerController::handleWebRtcTransport(const std::shared_ptr<WebRtcTransportController>& transportController)
    {
        _webRtcTransportMap.emplace(std::make_pair(transportController->id(), transportController));

        // Emit observer event.
        this->webrtcTransportHandledSignal(transportController);
        
        transportController->closeSignal.connect(&WebRtcServerController::onWebRtcTransportClose, shared_from_this());
    }

    void WebRtcServerController::onWebRtcTransportClose(const std::string& id_)
    {
        if (_webRtcTransportMap.contains(id_)) {
            auto controller = _webRtcTransportMap[id_];
            this->webrtcTransportUnhandledSignal(controller);
            _webRtcTransportMap.erase(id_);
        }
    }
}

namespace srv
{
    std::shared_ptr<IpPort> parseIpPort(const FBS::WebRtcServer::IpPort* binary)
    {
        auto ipPort = std::make_shared<IpPort>();
        
        ipPort->ip = binary->ip()->str();
        ipPort->port = binary->port();
        
        return ipPort;
    }

    std::shared_ptr<IceUserNameFragment> parseIceUserNameFragment(const FBS::WebRtcServer::IceUserNameFragment* binary)
    {
        auto fragment = std::make_shared<IceUserNameFragment>();
        
        fragment->localIceUsernameFragment = binary->localIceUsernameFragment()->str();
        fragment->webRtcTransportId = binary->webRtcTransportId()->str();
        
        return fragment;
    }

    std::shared_ptr<TupleHash> parseTupleHash(const FBS::WebRtcServer::TupleHash* binary)
    {
        auto tupleHash = std::make_shared<TupleHash>();
        
        tupleHash->tupleHash = binary->tupleHash();
        tupleHash->webRtcTransportId = binary->webRtcTransportId()->str();
        
        return tupleHash;
    }

    std::shared_ptr<WebRtcServerDump> parseWebRtcServerDump(const FBS::WebRtcServer::DumpResponse* data)
    {
        auto dump = std::make_shared<WebRtcServerDump>();
        
        dump->id = data->id()->str();
        
        for (const auto& item : *data->udpSockets()) {
            IpPort ip = *parseIpPort(item);
            dump->udpSockets.emplace_back(ip);
        }
        
        for (const auto& item : *data->tcpServers()) {
            IpPort ip = *parseIpPort(item);
            dump->tcpServers.emplace_back(ip);
        }
        
        for (const auto& item : *data->webRtcTransportIds()) {
            dump->webRtcTransportIds.emplace_back(item->str());
        }
        
        for (const auto& item : *data->localIceUsernameFragments()) {
            IceUserNameFragment fragment = *parseIceUserNameFragment(item);
            dump->localIceUsernameFragments.emplace_back(fragment);
        }
        
        for (const auto& item : *data->tupleHashes()) {
            TupleHash hash = *parseTupleHash(item);
            dump->tupleHashes.emplace_back(hash);
        }
        
        return dump;
    }

    void to_json(nlohmann::json& js, const WebRtcServerListenInfo& st)
    {
        js["protocol"] = st.protocol;
        js["ip"] = st.ip;
        js["announcedIp"] = st.announcedIp;
        js["announcedAddress"] = st.announcedAddress;
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
        if (j.contains("announcedAddress")) {
            j.at("announcedAddress").get_to(st.announcedAddress);
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
