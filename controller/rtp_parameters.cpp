/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "rtp_parameters.h"

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

    void to_json(nlohmann::json& j, const _rtx& st)
    {
        j["ssrc"] = st.ssrc;
    }

    void from_json(const nlohmann::json& j, _rtx& st)
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
        j["parameters"] = st.parameters;
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
            j.at("parameters").get_to(st.parameters);
        }
    }

    void to_json(nlohmann::json& j, const RtpCodecParameters& st)
    {
        j["mimeType"] = st.mimeType;
        j["payloadType"] = st.payloadType;
        j["clockRate"] = st.clockRate;
        j["channels"] = st.channels;
        j["parameters"] = st.parameters;
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
        if (st.parameters.size() == 0) {
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
            j.at("parameters").get_to(st.parameters);
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
