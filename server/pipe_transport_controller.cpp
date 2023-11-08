/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "pipe_transport_controller.h"
#include <regex>
#include <algorithm>
#include "srv_logger.h"
#include "types.h"
#include "uuid.h"
#include "ortc.h"
#include "channel.h"
#include "payload_channel.h"
#include "utils.h"

namespace
{
    using namespace srv;

    bool isRtxCodec(const nlohmann::json& codec)
    {
        static const std::regex RtxMimeTypeRegex("^(audio|video)/rtx$", std::regex_constants::ECMAScript | std::regex_constants::icase);

        std::smatch match;
        std::string mimeType = codec["mimeType"];

        return std::regex_match(mimeType, match, RtxMimeTypeRegex);
    }

    RtpParameters getPipeConsumerRtpParameters(const RtpParameters& consumableRtpParameters, bool enableRtx)
    {
        RtpParameters consumerParams;
        consumerParams.rtcp = consumableRtpParameters.rtcp;
        
        std::vector<RtpCodecParameters> consumableCodecs;
        consumableCodecs.insert(consumableCodecs.end(), consumableRtpParameters.codecs.begin(), consumableRtpParameters.codecs.end());
        for (auto &codec : consumableCodecs) {
            if (!enableRtx && isRtxCodec(codec)) {
                continue;
            }
            
            std::vector<RtcpFeedback> rtcpFeedback;
            for (auto &fb : codec.rtcpFeedback) {
                if ((fb.type == "nack" && fb.type == "pli") || (fb.type == "ccm" && fb.type == "fir") || (enableRtx && fb.type == "nack" && !fb.parameter.empty())) {
                    rtcpFeedback.push_back(fb);
                }
            }
            codec.rtcpFeedback.clear();
            codec.rtcpFeedback.insert(codec.rtcpFeedback.end(), rtcpFeedback.begin(), rtcpFeedback.end());
            
            consumerParams.codecs.push_back(codec);
        }
        
        // Reduce RTP extensions by disabling transport MID and BWE related ones.
        std::vector<RtpHeaderExtensionParameters> headerExtensions;;
        for (auto &ext : consumableRtpParameters.headerExtensions) {
            if (ext.uri != "urn:ietf:params:rtp-hdrext:sdes:mid" &&
                ext.uri != "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time" &&
                ext.uri != "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01") {
                headerExtensions.push_back(ext);
            }
        }
        consumerParams.headerExtensions.clear();
        consumerParams.headerExtensions.insert(consumerParams.headerExtensions.end(), headerExtensions.begin(), headerExtensions.end());
        
        std::vector<RtpEncodingParameters> consumableEncodings;
        consumableEncodings.insert(consumableEncodings.end(), consumableRtpParameters.encodings.begin(), consumableRtpParameters.encodings.end());
        
        auto baseSsrc = srv::getRandomInteger((uint32_t)100000000, (uint32_t)999999999);
        auto baseRtxSsrc = srv::getRandomInteger((uint32_t)100000000, (uint32_t)999999999);
        
        for (auto i = 0; i < (int32_t)consumableEncodings.size(); ++i) {
            auto encoding = consumableEncodings[i];
            
            encoding.ssrc = baseSsrc + i;
            
            if (enableRtx) {
                encoding.rtx.ssrc = (baseRtxSsrc + i );
            }
            else {
                //delete encoding.rtx;
                encoding.rtx.ssrc = 0;
            }
            
            consumerParams.encodings.push_back(encoding);
        }
        
        return consumerParams;
    }
}

namespace srv {

    PipeTransportController::PipeTransportController(const std::shared_ptr<PipeTransportConstructorOptions>& options)
    : TransportController(options)
    {
        SRV_LOGD("PipeTransportController()");
        
        const auto& data = options->data;
        
        _data["tuple"] = data["tuple"];
        _data["sctpParameters"] = data["sctpParameters"];
        _data["sctpState"] = data["sctpState"];
        _data["rtx"] = data["rtx"];
        _data["srtpParameters"] = data["srtpParameters"];
    }

    PipeTransportController::~PipeTransportController()
    {
        SRV_LOGD("~PipeTransportController()");
    }

    void PipeTransportController::init()
    {
        SRV_LOGD("init()");
        handleWorkerNotifications();
    }

    void PipeTransportController::destroy()
    {
        SRV_LOGD("destroy()");
    }

    void PipeTransportController::close()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("close()");
        
        this->_data["sctpState"] = "closed";

        TransportController::close();
    }

    void PipeTransportController::onRouterClosed()
    {
        if (_closed) {
            return;
        }

        SRV_LOGD("onRouterClosed()");
        
        this->_data["sctpState"] = "closed";

        TransportController::onRouterClosed();
    }

    nlohmann::json PipeTransportController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nlohmann::json();
        }
        
        return channel->request("transport.getStats", _internal.transportId, "{}");
    }

    void PipeTransportController::connect(const nlohmann::json& reqData)
    {
        SRV_LOGD("connect()");
        
       // const reqData = { ip, port, srtpParameters };

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        auto data = channel->request("transport.connect", _internal.transportId, reqData.dump());

        // Update data.
        this->_data["tuple"] = data["tuple"];
    }

    std::shared_ptr<ConsumerController> PipeTransportController::consume(const std::shared_ptr<ConsumerOptions>& options)
    {
        SRV_LOGD("consume()");
        
        std::shared_ptr<ConsumerController> consumerController;
        
        auto channel = _channel.lock();
        if (!channel) {
            return consumerController;
        }
        
        const std::string& producerId = options->producerId;
        const bool pipe = true;
        const nlohmann::json& appData = options->appData;
        
        if (producerId.empty()) {
            SRV_LOGE("missing producerId");
            return consumerController;
        }

        auto producerController = _getProducerController(producerId);
        if (!producerController) {
            SRV_LOGE("Producer with id '%s' not found", producerId.c_str());
            return consumerController;
        }
        
        auto consumableRtpParameters = producerController->consumableRtpParameters();
        bool enableRtx = _data["rtx"];
        
        // This may throw.
        auto rtpParameters = ortc::getPipeConsumerRtpParameters(consumableRtpParameters, enableRtx);
        
        auto consumerId = uuid::uuidv4();
        
        nlohmann::json reqData;
        reqData["consumerId"] = consumerId;
        reqData["producerId"] = producerId;
        reqData["kind"] = producerController->kind();
        reqData["rtpParameters"] = rtpParameters;
        reqData["type"] = pipe ? "pipe" : producerController->type();
        reqData["consumableRtpEncodings"] = producerController->consumableRtpParameters().encodings;

        nlohmann::json status = channel->request("transport.consume", _internal.transportId, reqData.dump());
        
        bool paused_ = status["paused"];
        bool producerPaused_ = status["producerPaused"];
        
        ConsumerInternal internal;
        internal.transportId = _internal.transportId;
        internal.consumerId = consumerId;
        
        ConsumerData data;
        data.producerId = producerId;
        data.kind = producerController->kind();
        data.rtpParameters = rtpParameters;
        data.type = pipe ? "pipe" : producerController->type();
        
        {
            std::lock_guard<std::mutex> lock(_consumersMutex);
            consumerController = std::make_shared<ConsumerController>(internal,
                                                                      data,
                                                                      _channel.lock(),
                                                                      _payloadChannel.lock(),
                                                                      appData,
                                                                      paused_,
                                                                      producerPaused_,
                                                                      ConsumerScore(),
                                                                      ConsumerLayers());
            consumerController->init();
            
            _consumerControllers[consumerController->id()] = consumerController;
            
            auto wself = std::weak_ptr<TransportController>(TransportController::shared_from_this());
            auto removeLambda = [id = consumerController->id(), wself]() {
                auto self = wself.lock();
                if (!self) {
                    return;
                }
                if (auto ptc = std::dynamic_pointer_cast<PipeTransportController>(self)) {
                    ptc->removeConsumerController(id);
                }
                else {
                    assert(0);
                }
            };
            
            consumerController->_closeSignal.connect(removeLambda);
            consumerController->_producerCloseSignal.connect(removeLambda);
            
            _newConsumerSignal(consumerController);
        }

        return consumerController;
    }

    void PipeTransportController::removeConsumerController(const std::string& id)
    {
        std::lock_guard<std::mutex> lock(_consumersMutex);
        if (_consumerControllers.find(id) != _consumerControllers.end()) {
            _consumerControllers.erase(id);
        }
    }

    void PipeTransportController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        auto self = std::dynamic_pointer_cast<PipeTransportController>(TransportController::shared_from_this());
        channel->_notificationSignal.connect(&PipeTransportController::onChannel, self);
    }

    void PipeTransportController::onChannel(const std::string& targetId, const std::string& event, const std::string& data)
    {
        SRV_LOGD("onChannel()");
        
        if (targetId != _internal.transportId) {
            return;
        }
        
        if (event == "sctpstatechange") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                auto sctpState = js["sctpState"];
                this->_data["sctpState"] = sctpState;
                _sctpStateChangeSignal(sctpState);
            }
        }
        else if (event == "trace") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object()) {
                TransportTraceEventData eventData = js;
                _traceSignal(eventData);
            }
        }
        else {
            SRV_LOGD("ignoring unknown event %s", event.c_str());
        }
    }
}

namespace srv
{
    void to_json(nlohmann::json& j, const PipeTransportStat& st)
    {
        j["type"] = st.type;
        j["transportId"] = st.transportId;
        j["timestamp"] = st.timestamp;
        j["sctpState"] = st.sctpState;

        j["bytesReceived"] = st.bytesReceived;
        j["recvBitrate"] = st.recvBitrate;
        j["bytesSent"] = st.bytesSent;
        j["sendBitrate"] = st.sendBitrate;
        j["rtpBytesReceived"] = st.rtpBytesReceived;
        j["rtpRecvBitrate"] = st.rtpRecvBitrate;

        j["rtpBytesSent"] = st.rtpBytesSent;
        j["rtpSendBitrate"] = st.rtpSendBitrate;
        j["rtxBytesReceived"] = st.rtxBytesReceived;
        j["rtxRecvBitrate"] = st.rtxRecvBitrate;
        j["rtxBytesSent"] = st.rtxBytesSent;

        j["rtxSendBitrate"] = st.rtxSendBitrate;
        j["probationBytesSent"] = st.probationBytesSent;
        j["probationSendBitrate"] = st.probationSendBitrate;
        j["availableOutgoingBitrate"] = st.availableOutgoingBitrate;
        j["availableIncomingBitrate"] = st.availableIncomingBitrate;
        j["maxIncomingBitrate"] = st.maxIncomingBitrate;

        j["tuple"] = st.tuple;
    }

    void from_json(const nlohmann::json& j, PipeTransportStat& st)
    {
        if (j.contains("type")) {
            j.at("type").get_to(st.type);
        }
        if (j.contains("transportId")) {
            j.at("transportId").get_to(st.transportId);
        }
        if (j.contains("timestamp")) {
            j.at("timestamp").get_to(st.timestamp);
        }
        if (j.contains("sctpState")) {
            j.at("sctpState").get_to(st.sctpState);
        }
        if (j.contains("bytesReceived")) {
            j.at("bytesReceived").get_to(st.bytesReceived);
        }
        if (j.contains("recvBitrate")) {
            j.at("recvBitrate").get_to(st.recvBitrate);
        }
        if (j.contains("bytesSent")) {
            j.at("bytesSent").get_to(st.bytesSent);
        }
        if (j.contains("sendBitrate")) {
            j.at("sendBitrate").get_to(st.sendBitrate);
        }
        if (j.contains("rtpBytesReceived")) {
            j.at("rtpBytesReceived").get_to(st.rtpBytesReceived);
        }
        if (j.contains("rtpRecvBitrate")) {
            j.at("rtpRecvBitrate").get_to(st.rtpRecvBitrate);
        }
        if (j.contains("rtpBytesSent")) {
            j.at("rtpBytesSent").get_to(st.rtpBytesSent);
        }
        if (j.contains("rtpSendBitrate")) {
            j.at("rtpSendBitrate").get_to(st.rtpSendBitrate);
        }
        if (j.contains("rtxBytesReceived")) {
            j.at("rtxBytesReceived").get_to(st.rtxBytesReceived);
        }
        if (j.contains("rtxRecvBitrate")) {
            j.at("rtxRecvBitrate").get_to(st.rtxRecvBitrate);
        }
        if (j.contains("rtxBytesSent")) {
            j.at("rtxBytesSent").get_to(st.rtxBytesSent);
        }
        if (j.contains("rtxSendBitrate")) {
            j.at("rtxSendBitrate").get_to(st.rtxSendBitrate);
        }
        if (j.contains("probationBytesSent")) {
            j.at("probationBytesSent").get_to(st.probationBytesSent);
        }
        if (j.contains("probationSendBitrate")) {
            j.at("probationSendBitrate").get_to(st.probationSendBitrate);
        }
        if (j.contains("availableOutgoingBitrate")) {
            j.at("availableOutgoingBitrate").get_to(st.availableOutgoingBitrate);
        }
        if (j.contains("availableIncomingBitrate")) {
            j.at("availableIncomingBitrate").get_to(st.availableIncomingBitrate);
        }
        if (j.contains("maxIncomingBitrate")) {
            j.at("maxIncomingBitrate").get_to(st.maxIncomingBitrate);
        }
        
        if (j.contains("tuple")) {
            j.at("tuple").get_to(st.tuple);
        }
    }
}