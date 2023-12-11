/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "rtp_parameters.h"
#include "srv_logger.h"

namespace srv {

    const Parameters& RtpCodecParameters::_getParameters() const
    {
        return _parameters;
    }

    void RtpCodecParameters::_setParameters(const Parameters& parameters)
    {
        _parameters = parameters;
        nlohmann::json jsonParameters;
        _parameters.serialize(jsonParameters);
        jsonParameters.get_to(this->parameters);
    }

    const std::map<std::string, nlohmann::json>& RtpCodecParameters::getParameters() const
    {
        return parameters;
    }

    void RtpCodecParameters::setParameters(const std::map<std::string, nlohmann::json>& parameters)
    {
        this->parameters = parameters;
        nlohmann::json jsonParameters = parameters;
        this->_parameters.Set(jsonParameters);
    }

    const Parameters& RtpHeaderExtensionParameters::_getParameters() const
    {
        return _parameters;
    }

    void RtpHeaderExtensionParameters::_setParameters(const Parameters& parameters)
    {
        _parameters = parameters;
    }

    const std::map<std::string, nlohmann::json>& RtpHeaderExtensionParameters::getParameters() const
    {
        return parameters;
    }

    void RtpHeaderExtensionParameters::setParameters(const std::map<std::string, nlohmann::json>& parameters)
    {
        this->parameters = parameters;
    }

    flatbuffers::Offset<FBS::RtpParameters::RtcpFeedback> RtcpFeedback::serialize(flatbuffers::FlatBufferBuilder& builder) const
    {
        return FBS::RtpParameters::CreateRtcpFeedbackDirect(builder, this->type.c_str(), this->parameter.c_str());
    }

    flatbuffers::Offset<FBS::RtpParameters::RtpCodecParameters> RtpCodecParameters::serialize(flatbuffers::FlatBufferBuilder& builder) const
    {
        auto parameters = this->_parameters.serialize(builder);

        std::vector<flatbuffers::Offset<FBS::RtpParameters::RtcpFeedback>> rtcpFeedback;
        rtcpFeedback.reserve(this->rtcpFeedback.size());

        for (const auto& fb : this->rtcpFeedback) {
            rtcpFeedback.emplace_back(fb.serialize(builder));
        }

        return FBS::RtpParameters::CreateRtpCodecParametersDirect(builder,
                                                                  this->mimeType.c_str(),
                                                                  this->payloadType,
                                                                  this->clockRate,
                                                                  this->channels > 1 ? flatbuffers::Optional<uint8_t>(this->channels) : flatbuffers::nullopt,
                                                                  &parameters,
                                                                  &rtcpFeedback);
    }
    
    flatbuffers::Offset<FBS::RtpParameters::RtpHeaderExtensionParameters> RtpHeaderExtensionParameters::serialize(flatbuffers::FlatBufferBuilder& builder) const
    {
        auto parameters = this->_parameters.serialize(builder);

        return FBS::RtpParameters::CreateRtpHeaderExtensionParametersDirect(builder, rtpHeaderExtensionUriToFbs(this->uri), this->id, this->encrypt, &parameters);
    }

    flatbuffers::Offset<FBS::RtpParameters::Rtx> RtpRtxParameters::serialize(flatbuffers::FlatBufferBuilder& builder) const
    {
        return FBS::RtpParameters::CreateRtx(builder, this->ssrc);
    }

    flatbuffers::Offset<FBS::RtpParameters::RtpEncodingParameters> RtpEncodingParameters::serialize(flatbuffers::FlatBufferBuilder& builder) const
    {
        return FBS::RtpParameters::CreateRtpEncodingParametersDirect(builder,
                                                                     this->ssrc != 0u ? flatbuffers::Optional<uint32_t>(this->ssrc) : flatbuffers::nullopt,
                                                                     this->rid.size() > 0 ? this->rid.c_str() : nullptr,
                                                                     this->hasCodecPayloadType ? flatbuffers::Optional<uint8_t>(this->codecPayloadType) : flatbuffers::nullopt,
                                                                     this->hasRtx ? this->rtx.serialize(builder) : 0u,
                                                                     this->dtx,
                                                                     this->scalabilityMode.c_str());
    }

    flatbuffers::Offset<FBS::RtpParameters::RtcpParameters> RtcpParameters::serialize(flatbuffers::FlatBufferBuilder& builder) const
    {
        return FBS::RtpParameters::CreateRtcpParametersDirect(builder, this->cname.c_str(), this->reducedSize);
    }
    
    flatbuffers::Offset<FBS::RtpParameters::RtpParameters> RtpParameters::serialize(flatbuffers::FlatBufferBuilder& builder) const
    {
        // Add codecs.
        std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpCodecParameters>> codecs;
        codecs.reserve(this->codecs.size());

        for (const auto& codec : this->codecs) {
            codecs.emplace_back(codec.serialize(builder));
        }

        // Add encodings.
        std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpEncodingParameters>> encodings;
        encodings.reserve(this->encodings.size());

        for (const auto& encoding : this->encodings) {
            encodings.emplace_back(encoding.serialize(builder));
        }

        // Add headerExtensions.
        std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpHeaderExtensionParameters>> headerExtensions;
        headerExtensions.reserve(this->headerExtensions.size());

        for (const auto& headerExtension : this->headerExtensions) {
            headerExtensions.emplace_back(headerExtension.serialize(builder));
        }

        // Add rtcp.
        flatbuffers::Offset<FBS::RtpParameters::RtcpParameters> rtcp;

        rtcp = this->rtcp.serialize(builder);

        return FBS::RtpParameters::CreateRtpParametersDirect(builder, mid.c_str(), &codecs, &headerExtensions, &encodings, rtcp);
    }

    std::string rtpHeaderExtensionUriFromFbs(FBS::RtpParameters::RtpHeaderExtensionUri uri)
    {
        switch (uri)
        {
            case FBS::RtpParameters::RtpHeaderExtensionUri::Mid:
                return "urn:ietf:params:rtp-hdrext:sdes:mid";
            case FBS::RtpParameters::RtpHeaderExtensionUri::RtpStreamId:
                return "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id";
            case FBS::RtpParameters::RtpHeaderExtensionUri::RepairRtpStreamId:
                return "urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id";
            case FBS::RtpParameters::RtpHeaderExtensionUri::FrameMarkingDraft07:
                return "http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07";
            case FBS::RtpParameters::RtpHeaderExtensionUri::FrameMarking:
                return "urn:ietf:params:rtp-hdrext:framemarking";
            case FBS::RtpParameters::RtpHeaderExtensionUri::AudioLevel:
                return "urn:ietf:params:rtp-hdrext:ssrc-audio-level";
            case FBS::RtpParameters::RtpHeaderExtensionUri::VideoOrientation:
                return "urn:3gpp:video-orientation";
            case FBS::RtpParameters::RtpHeaderExtensionUri::TimeOffset:
                return "urn:ietf:params:rtp-hdrext:toffset";
            case FBS::RtpParameters::RtpHeaderExtensionUri::TransportWideCcDraft01:
                return "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01";
            case FBS::RtpParameters::RtpHeaderExtensionUri::AbsSendTime:
                return "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time";
            case FBS::RtpParameters::RtpHeaderExtensionUri::AbsCaptureTime:
                return "http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time";
            default:
                return "";
        }
    }

    FBS::RtpParameters::RtpHeaderExtensionUri rtpHeaderExtensionUriToFbs(const std::string& uri)
    {
        if (uri == "urn:ietf:params:rtp-hdrext:sdes:mid") {
            return FBS::RtpParameters::RtpHeaderExtensionUri::Mid;
        }
        else if (uri == "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id") {
            return FBS::RtpParameters::RtpHeaderExtensionUri::RtpStreamId;
        }
        else if (uri == "urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id") {
            return FBS::RtpParameters::RtpHeaderExtensionUri::RepairRtpStreamId;
        }
        else if (uri == "http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07") {
            return FBS::RtpParameters::RtpHeaderExtensionUri::FrameMarkingDraft07;
        }
        else if (uri == "urn:ietf:params:rtp-hdrext:framemarking") {
            return FBS::RtpParameters::RtpHeaderExtensionUri::FrameMarking;
        }
        else if (uri == "urn:ietf:params:rtp-hdrext:ssrc-audio-level") {
            return FBS::RtpParameters::RtpHeaderExtensionUri::AudioLevel;
        }
        else if (uri == "urn:3gpp:video-orientation") {
            return FBS::RtpParameters::RtpHeaderExtensionUri::VideoOrientation;
        }
        else if (uri == "urn:ietf:params:rtp-hdrext:toffset") {
            return FBS::RtpParameters::RtpHeaderExtensionUri::TimeOffset;
        }
        else if (uri == "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01") {
            return FBS::RtpParameters::RtpHeaderExtensionUri::TransportWideCcDraft01;
        }
        else if (uri == "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time") {
            return FBS::RtpParameters::RtpHeaderExtensionUri::AbsSendTime;
        }
        else if (uri == "http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time") {
            return FBS::RtpParameters::RtpHeaderExtensionUri::AbsCaptureTime;
        }

        SRV_LOGE("invalid RtpHeaderExtensionUri: %s", uri.c_str());
        assert(0);
        return FBS::RtpParameters::RtpHeaderExtensionUri::MAX;
    }

    std::shared_ptr<RtpEncodingParameters> parseRtpEncodingParameters(const FBS::RtpParameters::RtpEncodingParameters* data)
    {
        auto parameters = std::make_shared<RtpEncodingParameters>();
    
        parameters->ssrc = data->ssrc().value_or(0);
        parameters->rid = data->rid()->str();
        parameters->codecPayloadType = data->codecPayloadType().value_or(0);
        parameters->rtx.ssrc = data->rtx()->ssrc();
        parameters->dtx = data->dtx();
        parameters->scalabilityMode = data->scalabilityMode()->str();
        parameters->maxBitrate = data->maxBitrate().value_or(0);
        
        return parameters;
    }

    std::shared_ptr<RtpParameters> parseRtpParameters(const FBS::RtpParameters::RtpParameters* data)
    {
        auto rtpParameters = std::make_shared<RtpParameters>();
        
        rtpParameters->mid = data->mid()->str();

        const auto* codecsFbs = data->codecs();
        for (const auto& codec : *codecsFbs) {
            RtpCodecParameters parameters;
            parameters.mimeType = codec->mimeType()->str();
            parameters.payloadType = codec->payloadType();
            parameters.clockRate = codec->clockRate();
            parameters.channels = codec->channels().value_or(0);
            Parameters params;
            params.Set(codec->parameters());
            parameters._setParameters(params);
            for (const auto& feedback : *codec->rtcpFeedback()) {
                RtcpFeedback rtcpFeedback;
                rtcpFeedback.type = feedback->type()->str();
                rtcpFeedback.parameter = feedback->parameter()->str();
                parameters.rtcpFeedback.emplace_back(rtcpFeedback);
            }
            
            rtpParameters->codecs.emplace_back(parameters);
        }

        for (const auto& headExtension : *data->headerExtensions()) {
            RtpHeaderExtensionParameters parameters;
            parameters.uri = rtpHeaderExtensionUriFromFbs(headExtension->uri());
            parameters.id = headExtension->id();
            parameters.encrypt = headExtension->encrypt();
            Parameters params;
            params.Set(headExtension->parameters());
            parameters._setParameters(params);
            rtpParameters->headerExtensions.emplace_back(parameters);
        }
        
        for (const auto& encoding : *data->encodings()) {
            RtpEncodingParameters parameters;
            parameters.ssrc = encoding->ssrc().value_or(0);
            parameters.rid = encoding->rid()->str();
            parameters.codecPayloadType = encoding->codecPayloadType().value_or(0);
            parameters.rtx.ssrc = encoding->rtx()->ssrc();
            parameters.dtx = encoding->dtx();
            parameters.scalabilityMode = encoding->scalabilityMode()->str();
            parameters.maxBitrate = encoding->maxBitrate().value_or(0);
            rtpParameters->encodings.emplace_back(parameters);
        }

        if (auto rtcp = data->rtcp()) {
            rtpParameters->rtcp.cname = rtcp->cname()->str();
            rtpParameters->rtcp.reducedSize = rtcp->reducedSize();
        }
        
        return rtpParameters;
    }

}

namespace srv {

    void to_json(nlohmann::json& j, const RtcpFeedback& st)
    {
        j["type"] = st.type;
        j["parameter"] = st.parameter;
    }

    void from_json(const nlohmann::json& j, RtcpFeedback& st)
    {
        if (j.contains("type")) {
            j.at("type").get_to(st.type);
        }
        if (j.contains("parameter")) {
            j.at("parameter").get_to(st.parameter);
        }
    }

    void to_json(nlohmann::json& j, const RtpRtxParameters& st)
    {
        j["ssrc"] = st.ssrc;
    }

    void from_json(const nlohmann::json& j, RtpRtxParameters& st)
    {
        if (j.contains("ssrc")) {
            j.at("ssrc").get_to(st.ssrc);
        }
    }

    void to_json(nlohmann::json& j, const RtpEncodingParameters& st)
    {
        j["ssrc"] = st.ssrc;
        j["rid"] = st.rid;
        j["codecPayloadType"] = st.codecPayloadType;
        j["rtx"] = st.rtx;
        j["dtx"] = st.dtx;
        j["scalabilityMode"] = st.scalabilityMode;
        j["scaleResolutionDownBy"] = st.scaleResolutionDownBy;
        j["maxBitrate"] = st.maxBitrate;

        if (st.ssrc == 0) {
            j.erase("ssrc");
        }
        if (st.rid.empty()) {
            j.erase("rid");
        }
        if (st.codecPayloadType == 0) {
            j.erase("codecPayloadType");
        }
        if (st.rtx.ssrc == 0) {
            j.erase("rtx");
        }
        if (st.scalabilityMode.empty()) {
            j.erase("scalabilityMode");
        }
        if (st.scaleResolutionDownBy == 0) {
            j.erase("scaleResolutionDownBy");
        }
        if (st.maxBitrate == 0 )  {
            j.erase("maxBitrate");
        }
    }

    void from_json(const nlohmann::json& j, RtpEncodingParameters& st)
    {
        if (j.contains("ssrc")) {
            j.at("ssrc").get_to(st.ssrc);
        }
        if (j.contains("rid")) {
            j.at("rid").get_to(st.rid);
        }
        if (j.contains("codecPayloadType")) {
            j.at("codecPayloadType").get_to(st.codecPayloadType);
        }
        if (j.contains("rtx")) {
            j.at("rtx").get_to(st.rtx);
        }
        if (j.contains("dtx")) {
            j.at("dtx").get_to(st.dtx);
        }
        if (j.contains("scalabilityMode")) {
            j.at("scalabilityMode").get_to(st.scalabilityMode);
        }
        if (j.contains("scaleResolutionDownBy")) {
            j.at("scaleResolutionDownBy").get_to(st.scaleResolutionDownBy);
        }
        if (j.contains("maxBitrate")) {
            j.at("maxBitrate").get_to(st.maxBitrate);
        }
    }

    void to_json(nlohmann::json& j, const RtpHeaderExtensionParameters& st)
    {
        j["uri"] = st.uri;
        j["id"] = st.id;
        j["encrypt"] = st.encrypt;
        j["parameters"] = st.getParameters();
    }

    void from_json(const nlohmann::json& j, RtpHeaderExtensionParameters& st)
    {
        if (j.contains("uri")) {
            j.at("uri").get_to(st.uri);
        }
        if (j.contains("id")) {
            j.at("id").get_to(st.id);
        }
        if (j.contains("encrypt")) {
            j.at("encrypt").get_to(st.encrypt);
        }
        if (j.contains("parameters")) {
            std::map<std::string, nlohmann::json> parameters;
            j.at("parameters").get_to(parameters);
            st.setParameters(parameters);
        }
    }

    void to_json(nlohmann::json& j, const RtpCodecParameters& st)
    {
        j["mimeType"] = st.mimeType;
        j["payloadType"] = st.payloadType;
        j["clockRate"] = st.clockRate;
        j["channels"] = st.channels;
        j["parameters"] = st.getParameters();
        j["rtcpFeedback"] = st.rtcpFeedback;

        if (st.mimeType.empty()) {
            j.erase("mimeType");
        }
        if (st.payloadType == 0) {
            j.erase("payloadType");
        }
        if (st.clockRate == 0) {
            j.erase("clockRate");
        }
        if (st.channels == 0) {
            j.erase("channels");
        }
        if (st.rtcpFeedback.size() == 0) {
            j.erase("rtcpFeedback");
        }
        if (st.getParameters().size() == 0) {
            j.erase("parameters");
        }
    }

    void from_json(const nlohmann::json& j, RtpCodecParameters& st)
    {
        if (j.contains("mimeType")) {
            j.at("mimeType").get_to(st.mimeType);
        }
        if (j.contains("payloadType")) {
            j.at("payloadType").get_to(st.payloadType);
        }
        if (j.contains("clockRate")) {
            j.at("clockRate").get_to(st.clockRate);
        }
        if (j.contains("channels")) {
            j.at("channels").get_to(st.channels);
        }
        if (j.contains("parameters")) {
            std::map<std::string, nlohmann::json> parameters;
            j.at("parameters").get_to(parameters);
            st.setParameters(parameters);
        }
        if (j.contains("rtcpFeedback")) {
            j.at("rtcpFeedback").get_to(st.rtcpFeedback);
        }
    }

    void to_json(nlohmann::json& j, const RtpHeaderExtension& st)
    {
        j["kind"] = st.kind;
        j["uri"] = st.uri;
        j["preferredId"] = st.preferredId;
        j["preferredEncrypt"] = st.preferredEncrypt;
        j["direction"] = st.direction;
    }

    void from_json(const nlohmann::json& j, RtpHeaderExtension& st)
    {
        if (j.contains("kind")) {
            j.at("kind").get_to(st.kind);
        }
        if (j.contains("uri")) {
            j.at("uri").get_to(st.uri);
        }
        if (j.contains("preferredId")) {
            j.at("preferredId").get_to(st.preferredId);
        }
        if (j.contains("preferredEncrypt")) {
            j.at("preferredEncrypt").get_to(st.preferredEncrypt);
        }
        if (j.contains("direction")) {
            j.at("direction").get_to(st.direction);
        }
    }

    void to_json(nlohmann::json& j, const RtpCodecCapability& st)
    {
        j["kind"] = st.kind;
        j["mimeType"] = st.mimeType;
        j["preferredPayloadType"] = st.preferredPayloadType;
        j["clockRate"] = st.clockRate;
        j["channels"] = st.channels;
        j["parameters"] = st.parameters;
        j["rtcpFeedback"] = st.rtcpFeedback;

        if (st.kind.empty()) {
            j.erase("kind");
        }
        if (st.mimeType.empty()) {
            j.erase("mimeType");
        }
        if (st.preferredPayloadType == 0) {
            j.erase("preferredPayloadType");
        }
        if (st.clockRate == 0) {
            j.erase("clockRate");
        }
        if (st.channels == 0) {
            j.erase("channels");
        }
        if (st.rtcpFeedback.size() == 0) {
            j.erase("rtcpFeedback");
        }
        if (st.parameters.size() == 0) {
            j.erase("parameters");
        }
    }

    void from_json(const nlohmann::json& j, RtpCodecCapability& st)
    {
        if (j.contains("kind")) {
            j.at("kind").get_to(st.kind);
        }
        if (j.contains("mimeType")) {
            j.at("mimeType").get_to(st.mimeType);
        }
        if (j.contains("preferredPayloadType")) {
            j.at("preferredPayloadType").get_to(st.preferredPayloadType);
        }
        if (j.contains("clockRate")) {
            j.at("clockRate").get_to(st.clockRate);
        }
        if (j.contains("channels")) {
            j.at("channels").get_to(st.channels);
        }
        if (j.contains("parameters")) {
            j.at("parameters").get_to(st.parameters);
        }
        if (j.contains("rtcpFeedback")) {
            j.at("rtcpFeedback").get_to(st.rtcpFeedback);
        }
    }

    void to_json(nlohmann::json& j, const RtpCapabilities& st)
    {
        j["codecs"] = st.codecs;
        j["headerExtensions"] = st.headerExtensions;
    }
        
    void from_json(const nlohmann::json& j, RtpCapabilities& st)
    {
        if (j.contains("codecs")) {
            j.at("codecs").get_to(st.codecs);
        }
        if (j.contains("headerExtensions")) {
            j.at("headerExtensions").get_to(st.headerExtensions);
        }
    }

    void to_json(nlohmann::json& j, const RtcpParameters& st)
    {
        j["cname"] = st.cname;
        j["reducedSize"] = st.reducedSize;
        j["mux"] = st.mux;
    }

    void from_json(const nlohmann::json& j, RtcpParameters& st)
    {
        if (j.contains("cname")) {
            j.at("cname").get_to(st.cname);
        }
        if (j.contains("reducedSize")) {
            j.at("reducedSize").get_to(st.reducedSize);
        }
        if (j.contains("mux")) {
            j.at("mux").get_to(st.mux);
        }
    }

    void to_json(nlohmann::json& j, const RtpParameters& st)
    {
        j["mid"] = st.mid;
        j["codecs"] = st.codecs;
        j["headerExtensions"] = st.headerExtensions;
        j["encodings"] = st.encodings;
        j["rtcp"] = st.rtcp;
    }

    void from_json(const nlohmann::json& j, RtpParameters& st)
    {
        if (j.contains("mid")) {
            j.at("mid").get_to(st.mid);
        }
        if (j.contains("codecs")) {
            j.at("codecs").get_to(st.codecs);
        }
        if (j.contains("headerExtensions")) {
            j.at("headerExtensions").get_to(st.headerExtensions);
        }
        if (j.contains("encodings")) {
            j.at("encodings").get_to(st.encodings);
        }
        if (j.contains("rtcp")) {
            j.at("rtcp").get_to(st.rtcp);
        }
    }

}
