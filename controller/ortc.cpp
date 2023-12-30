/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "ortc.h"
#include <string>
#include <regex>
#include <algorithm>
#include "srv_logger.h"
#include "types.h"
#include "h264_profile_level_id.h"
#include "utils.h"

#define SRV_CLASS "ortc"

namespace Utils
{
    class Json
    {
    public:
        static bool IsPositiveInteger(const nlohmann::json& value)
        {
            if (value.is_number_unsigned()) {
                return true;
            }
            else if (value.is_number_integer()) {
                return value.get<int64_t>() >= 0;
            }
            else {
                return false;
            }
        }
    };
}

namespace srv {
    
    std::vector<int> DynamicPayloadTypes {
        100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
        111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
        122, 123, 124, 125, 126, 127, 96, 97, 98, 99
    };

    void ortc::validateRtpCapabilities(nlohmann::json& caps)
    {
        if (!caps.is_object()) {
            SRV_THROW_TYPE_ERROR("caps is not an object");
        }
        
        auto codecsIt = caps.find("codecs");
        auto headerExtensionsIt = caps.find("headerExtensions");

        // codecs is optional. If unset, fill with an empty array.
        if (codecsIt != caps.end() && !codecsIt->is_array()) {
            SRV_THROW_TYPE_ERROR("caps.codecs is not an object");
        }
        else if (codecsIt == caps.end()) {
            caps["codecs"] = nlohmann::json::array();
            codecsIt = caps.find("codecs");
        }

        for (auto& codec : *codecsIt) {
            validateRtpCodecCapability(codec);
        }

        // headerExtensions is optional. If unset, fill with an empty array.
        if (headerExtensionsIt != caps.end() && !headerExtensionsIt->is_array()) {
            SRV_LOGD("caps.headerExtensions is not an array");
        }
        else if (headerExtensionsIt == caps.end()) {
            caps["headerExtensions"] = nlohmann::json::array();
            headerExtensionsIt = caps.find("headerExtensions");
        }

        for (auto& ext : *headerExtensionsIt)
        {
            validateRtpHeaderExtension(ext);
        }
    }

    void ortc::validateRtpCodecCapability(nlohmann::json& codec)
    {
        static const std::regex MimeTypeRegex("^(audio|video)/(.+)", std::regex_constants::ECMAScript | std::regex_constants::icase);

        if (!codec.is_object()) {
            SRV_THROW_TYPE_ERROR("codec is not an object");
        }
        auto mimeTypeIt = codec.find("mimeType");
        auto preferredPayloadTypeIt = codec.find("preferredPayloadType");
        auto clockRateIt = codec.find("clockRate");
        auto channelsIt = codec.find("channels");
        auto parametersIt = codec.find("parameters");
        auto rtcpFeedbackIt = codec.find("rtcpFeedback");

        // mimeType is mandatory.
        if (mimeTypeIt == codec.end() || !mimeTypeIt->is_string()) {
            SRV_THROW_TYPE_ERROR("missing codec.mimeType");
        }

        std::smatch mimeTypeMatch;
        std::string regexTarget = mimeTypeIt->get<std::string>();
        std::regex_match(regexTarget, mimeTypeMatch, MimeTypeRegex);

        if (mimeTypeMatch.empty()) {
            SRV_THROW_TYPE_ERROR("invalid codec.mimeType");
        }
        // Just override kind with media component : mimeType.
        codec["kind"] = mimeTypeMatch[1].str();

        // preferredPayloadType is optional.
        if (preferredPayloadTypeIt != codec.end() && !preferredPayloadTypeIt->is_number_integer()) {
            SRV_THROW_TYPE_ERROR("invalid codec.preferredPayloadType");
        }

        // clockRate is mandatory.
        if (clockRateIt == codec.end() || !clockRateIt->is_number_integer()) {
            SRV_THROW_TYPE_ERROR("missing codec.clockRate");
        }

        // channels is optional. If unset, set it to 1 (just if audio).
        if (codec["kind"] == "audio") {
            if (channelsIt == codec.end() || !channelsIt->is_number_integer()) {
                codec["channels"] = 1;
            }
        }
        else {
            if (channelsIt != codec.end()) {
                codec.erase("channels");
            }
        }

        // parameters is optional. If unset, set it to an empty object.
        if (parametersIt == codec.end() || !parametersIt->is_object()) {
            codec["parameters"] = nlohmann::json::object();
            parametersIt = codec.find("parameters");
        }

        for (auto it = parametersIt->begin(); it != parametersIt->end(); ++it) {
            const auto& key = it.key();
            auto& value = it.value();

            if (!value.is_string() && !value.is_number() && value != nullptr) {
                SRV_THROW_TYPE_ERROR("invalid codec parameter");
            }

            // Specific parameters validation.
            if (key == "apt") {
                if (!value.is_number_integer()) {
                    SRV_THROW_TYPE_ERROR("invalid codec apt parameter");
                }
            }
        }

        // rtcpFeedback is optional. If unset, set it to an empty array.
        if (rtcpFeedbackIt == codec.end() || !rtcpFeedbackIt->is_array())
        {
            codec["rtcpFeedback"] = nlohmann::json::array();
            rtcpFeedbackIt = codec.find("rtcpFeedback");
        }

        for (auto& fb : *rtcpFeedbackIt)
        {
            validateRtcpFeedback(fb);
        }
    }

    void ortc::validateRtcpFeedback(nlohmann::json& fb)
    {
        if (!fb.is_object()) {
            SRV_THROW_TYPE_ERROR("fb is not an object");
        }

        auto typeIt = fb.find("type");
        auto parameterIt = fb.find("parameter");

        // type is mandatory.
        if (typeIt == fb.end() || !typeIt->is_string()) {
            SRV_THROW_TYPE_ERROR("missing fb.type");
        }

        // parameter is optional. If unset set it to an empty string.
        if (parameterIt == fb.end() || !parameterIt->is_string()) {
            fb["parameter"] = "";
        }
    }

    void ortc::validateRtpHeaderExtension(nlohmann::json& ext)
    {
        if (!ext.is_object()) {
            SRV_THROW_TYPE_ERROR("ext is not an object");
        }

        auto kindIt = ext.find("kind");
        auto uriIt = ext.find("uri");
        auto preferredIdIt = ext.find("preferredId");
        auto preferredEncryptIt = ext.find("preferredEncrypt");
        auto directionIt = ext.find("direction");

        // kind is optional. If unset set it to an empty string.
        if (kindIt == ext.end() || !kindIt->is_string()) {
            ext["kind"] = "";
        }

        kindIt = ext.find("kind");
        std::string kind = kindIt->get<std::string>();

        if (!kind.empty() && kind != "audio" && kind != "video") {
            SRV_THROW_TYPE_ERROR("invalid ext.kind");
        }

        // uri is mandatory.
        if (uriIt == ext.end() || !uriIt->is_string() || uriIt->get<std::string>().empty()) {
            SRV_THROW_TYPE_ERROR("missing ext.uri");
        }

        // preferredId is mandatory.
        if (preferredIdIt == ext.end() || !preferredIdIt->is_number_integer()) {
            SRV_THROW_TYPE_ERROR("missing ext.preferredId");
        }

        // preferredEncrypt is optional. If unset set it to false.
        if (preferredEncryptIt != ext.end() && !preferredEncryptIt->is_boolean()) {
            SRV_THROW_TYPE_ERROR("invalid ext.preferredEncrypt");
        }
        else if (preferredEncryptIt == ext.end()) {
            ext["preferredEncrypt"] = false;
        }
        // direction is optional. If unset set it to sendrecv.
        if (directionIt != ext.end() && !directionIt->is_string()) {
            SRV_THROW_TYPE_ERROR("invalid ext.direction");
        }
        else if (directionIt == ext.end()) {
            ext["direction"] = "sendrecv";
        }
    }

    void ortc::validateRtpParameters(nlohmann::json& params)
    {
        if (!params.is_object()) {
            SRV_THROW_TYPE_ERROR("params is not an object");
        }

        auto midIt = params.find("mid");
        auto codecsIt = params.find("codecs");
        auto headerExtensionsIt = params.find("headerExtensions");
        auto encodingsIt = params.find("encodings");
        auto rtcpIt = params.find("rtcp");

        // mid is optional.
        if (midIt != params.end() && (!midIt->is_string() || midIt->get<std::string>().empty())) {
            SRV_THROW_TYPE_ERROR("params.mid is not a string");
        }

        // codecs is mandatory.
        if (codecsIt == params.end() || !codecsIt->is_array()) {
            SRV_THROW_TYPE_ERROR("missing params.codecs");
        }

        for (auto& codec : *codecsIt) {
            validateRtpCodecParameters(codec);
        }

        // headerExtensions is optional. If unset, fill with an empty array.
        if (headerExtensionsIt != params.end() && !headerExtensionsIt->is_array()) {
            SRV_THROW_TYPE_ERROR("params.headerExtensions is not an array");
        }
        else if (headerExtensionsIt == params.end()) {
            params["headerExtensions"] = nlohmann::json::array();
            headerExtensionsIt = params.find("headerExtensions");
        }

        for (auto& ext : *headerExtensionsIt) {
            validateRtpHeaderExtensionParameters(ext);
        }

        // encodings is optional. If unset, fill with an empty array.
        if (encodingsIt != params.end() && !encodingsIt->is_array()) {
            SRV_THROW_TYPE_ERROR("params.encodings is not an array");
        }
        else if (encodingsIt == params.end()) {
            params["encodings"] = nlohmann::json::array();
            encodingsIt = params.find("encodings");
        }

        for (auto& encoding : *encodingsIt)  {
            validateRtpEncodingParameters(encoding);
        }

        // rtcp is optional. If unset, fill with an empty object.
        if (rtcpIt != params.end() && !rtcpIt->is_object()) {
            SRV_THROW_TYPE_ERROR("params.rtcp is not an object");
        }
        else if (rtcpIt == params.end()) {
            params["rtcp"] = nlohmann::json::object();
            rtcpIt = params.find("rtcp");
        }

        validateRtcpParameters(*rtcpIt);
    }
    
    void ortc::validateRtpCodecParameters(nlohmann::json& codec)
    {
        static const std::regex MimeTypeRegex("^(audio|video)/(.+)", std::regex_constants::ECMAScript | std::regex_constants::icase);

        if (!codec.is_object()) {
            SRV_THROW_TYPE_ERROR("codec is not an object");
        }

        auto mimeTypeIt = codec.find("mimeType");
        auto payloadTypeIt = codec.find("payloadType");
        auto clockRateIt = codec.find("clockRate");
        auto channelsIt = codec.find("channels");
        auto parametersIt = codec.find("parameters");
        auto rtcpFeedbackIt = codec.find("rtcpFeedback");

        // mimeType is mandatory.
        if (mimeTypeIt == codec.end() || !mimeTypeIt->is_string()) {
            SRV_THROW_TYPE_ERROR("missing codec.mimeType");
        }

        std::smatch mimeTypeMatch;
        std::string regexTarget = mimeTypeIt->get<std::string>();
        std::regex_match(regexTarget, mimeTypeMatch, MimeTypeRegex);

        if (mimeTypeMatch.empty()) {
            SRV_THROW_TYPE_ERROR("invalid codec.mimeType");
        }

        // payloadType is mandatory.
        if (payloadTypeIt == codec.end() || !payloadTypeIt->is_number_integer()) {
            SRV_THROW_TYPE_ERROR("missing codec.payloadType");
        }

        // clockRate is mandatory.
        if (clockRateIt == codec.end() || !clockRateIt->is_number_integer()) {
            SRV_THROW_TYPE_ERROR("missing codec.clockRate");
        }

        // Retrieve media kind from mimeType.
        auto kind = mimeTypeMatch[1].str();

        // channels is optional. If unset, set it to 1 (just for audio).
        if (kind == "audio") {
            if (channelsIt == codec.end() || !channelsIt->is_number_integer()) {
                codec["channels"] = 1;
            }
        }
        else {
            if (channelsIt != codec.end()) {
                codec.erase("channels");
            }
        }

        // parameters is optional. If unset, set it to an empty object.
        if (parametersIt == codec.end() || !parametersIt->is_object()) {
            codec["parameters"] = nlohmann::json::object();
            parametersIt = codec.find("parameters");
        }

        for (auto it = parametersIt->begin(); it != parametersIt->end(); ++it) {
            const auto& key = it.key();
            auto& value = it.value();

            if (!value.is_string() && !value.is_number() && value != nullptr) {
                SRV_THROW_TYPE_ERROR("invalid codec parameter");
            }

            // Specific parameters validation.
            if (key == "apt") {
                if (!value.is_number_integer()) {
                    SRV_THROW_TYPE_ERROR("invalid codec apt parameter");
                }
            }
        }

        // rtcpFeedback is optional. If unset, set it to an empty array.
        if (rtcpFeedbackIt == codec.end() || !rtcpFeedbackIt->is_array()) {
            codec["rtcpFeedback"] = nlohmann::json::array();
            rtcpFeedbackIt = codec.find("rtcpFeedback");
        }

        for (auto& fb : *rtcpFeedbackIt) {
            validateRtcpFeedback(fb);
        }
    }

    void ortc::validateRtpHeaderExtensionParameters(nlohmann::json& ext)
    {
        if (!ext.is_object()) {
            SRV_THROW_TYPE_ERROR("ext is not an object");
        }
        
        auto uriIt = ext.find("uri");
        auto idIt = ext.find("id");
        auto encryptIt = ext.find("encrypt");
        auto parametersIt = ext.find("parameters");

        // uri is mandatory.
        if (uriIt == ext.end() || !uriIt->is_string() || uriIt->get<std::string>().empty()) {
            SRV_THROW_TYPE_ERROR("missing ext.uri");
        }

        // id is mandatory.
        if (idIt == ext.end() || !idIt->is_number_integer()) {
            SRV_THROW_TYPE_ERROR("missing ext.id");
        }

        // encrypt is optional. If unset set it to false.
        if (encryptIt != ext.end() && !encryptIt->is_boolean()) {
            SRV_THROW_TYPE_ERROR("invalid ext.encrypt");
        }
        else if (encryptIt == ext.end()) {
            ext["encrypt"] = false;
        }

        // parameters is optional. If unset, set it to an empty object.
        if (parametersIt == ext.end() || !parametersIt->is_object()) {
            ext["parameters"] = nlohmann::json::object();
            parametersIt = ext.find("parameters");
        }

        for (auto it = parametersIt->begin(); it != parametersIt->end(); ++it) {
            auto& value = it.value();

            if (!value.is_string() && !value.is_number()) {
                SRV_THROW_TYPE_ERROR("invalid header extension parameter");
            }
        }
    }

    void ortc::validateRtpEncodingParameters(nlohmann::json& encoding)
    {
        if (!encoding.is_object()) {
            SRV_THROW_TYPE_ERROR("encoding is not an object");
        }
        
        SRV_LOGD("[worker] validateRtpEncodingParameters encoding = %s", encoding.dump(4).c_str());
        
        auto ssrcIt = encoding.find("ssrc");
        auto ridIt = encoding.find("rid");
        auto rtxIt = encoding.find("rtx");
        auto dtxIt = encoding.find("dtx");
        auto scalabilityModeIt = encoding.find("scalabilityMode");

        // ssrc is optional.
        if (ssrcIt != encoding.end() && !ssrcIt->is_number_integer()) {
            SRV_THROW_TYPE_ERROR("invalid encoding.ssrc");
        }

        // rid is optional.
        if (ridIt != encoding.end() && (!ridIt->is_string())) {
            SRV_THROW_TYPE_ERROR("invalid encoding.rid");
        }

        // rtx is optional.
        if (rtxIt != encoding.end() && !rtxIt->is_object()) {
            SRV_THROW_TYPE_ERROR("invalid encoding.rtx");
        }
        else if (rtxIt != encoding.end()) {
            auto rtxSsrcIt = rtxIt->find("ssrc");

            // RTX ssrc is mandatory if rtx is present.
            if (rtxSsrcIt == rtxIt->end() || !rtxSsrcIt->is_number_integer()) {
                SRV_THROW_TYPE_ERROR("missing encoding.rtx.ssrc");
            }
        }

        // dtx is optional. If unset set it to false.
        if (dtxIt == encoding.end() || !dtxIt->is_boolean()) {
            encoding["dtx"] = false;
        }

        // scalabilityMode is optional.
        if (scalabilityModeIt != encoding.end() && (!scalabilityModeIt->is_string())) {
            SRV_THROW_TYPE_ERROR("invalid encoding.scalabilityMode");
        }
    }


    void ortc::validateRtcpParameters(nlohmann::json& rtcp)
    {
        if (!rtcp.is_object()) {
            SRV_THROW_TYPE_ERROR("rtcp is not an object");
        }
        
        auto cnameIt = rtcp.find("cname");
        auto reducedSizeIt = rtcp.find("reducedSize");

        // cname is optional.
        if (cnameIt != rtcp.end() && !cnameIt->is_string()) {
            SRV_THROW_TYPE_ERROR("invalid rtcp.cname");
        }

        // reducedSize is optional. If unset set it to true.
        if (reducedSizeIt == rtcp.end() || !reducedSizeIt->is_boolean()) {
            rtcp["reducedSize"] = true;
        }
    }

    void ortc::validateSctpCapabilities(nlohmann::json& caps)
    {
        if (!caps.is_object()) {
            SRV_THROW_TYPE_ERROR("caps is not an object");
        }
        
        auto numStreamsIt = caps.find("numStreams");

        // numStreams is mandatory.
        if (numStreamsIt == caps.end() || !numStreamsIt->is_object()) {
            SRV_THROW_TYPE_ERROR("missing caps.numStreams");
        }

        validateNumSctpStreams(*numStreamsIt);
    }

    void ortc::validateNumSctpStreams(nlohmann::json& numStreams)
    {
        if (!numStreams.is_object()) {
            SRV_THROW_TYPE_ERROR("numStreams is not an object");
        }
        
        auto osIt = numStreams.find("OS");
        auto misIt = numStreams.find("MIS");

        // OS is mandatory.
        if (osIt == numStreams.end() || !osIt->is_number_integer()) {
            SRV_THROW_TYPE_ERROR("missing numStreams.OS");
        }

        // MIS is mandatory.
        if (misIt == numStreams.end() || !misIt->is_number_integer()) {
            SRV_THROW_TYPE_ERROR("missing numStreams.MIS");
        }
    }

    void ortc::validateSctpParameters(nlohmann::json& params)
    {
        if (!params.is_object()) {
            SRV_THROW_TYPE_ERROR("params is not an object");
        }

        auto portIt = params.find("port");
        auto osIt = params.find("OS");
        auto misIt = params.find("MIS");
        auto maxMessageSizeIt = params.find("maxMessageSize");

        // port is mandatory.
        if (portIt == params.end() || !portIt->is_number_integer()) {
            SRV_THROW_TYPE_ERROR("missing params.port");
        }

        // OS is mandatory.
        if (osIt == params.end() || !osIt->is_number_integer()) {
            SRV_THROW_TYPE_ERROR("missing params.OS");
        }

        // MIS is mandatory.
        if (misIt == params.end() || !misIt->is_number_integer()) {
            SRV_THROW_TYPE_ERROR("missing params.MIS");
        }

        // maxMessageSize is mandatory.
        if (maxMessageSizeIt == params.end() || !maxMessageSizeIt->is_number_integer()) {
            SRV_THROW_TYPE_ERROR("missing params.maxMessageSize");
        }
    }

    void ortc::validateSctpStreamParameters(nlohmann::json& params)
    {
        if (!params.is_object()) {
            SRV_THROW_TYPE_ERROR("params is not an object");
        }

        auto streamIdIt = params.find("streamId");
        auto orderedIt = params.find("ordered");
        auto maxPacketLifeTimeIt = params.find("maxPacketLifeTime");
        auto maxRetransmitsIt = params.find("maxRetransmits");
        //auto labelIt = params.find("label");
        //auto protocolIt = params.find("protocol");

        // streamId is mandatory.
        if (streamIdIt == params.end() || !streamIdIt->is_number_integer()) {
            SRV_THROW_TYPE_ERROR("missing params.streamId");
        }

        // ordered is optional.
        bool orderedGiven = false;

        if (orderedIt != params.end() && orderedIt->is_boolean()) {
            orderedGiven = true;
        }
        else {
            params["ordered"] = true;
        }
        
        // maxPacketLifeTime is optional. If unset set it to 0.
        if (maxPacketLifeTimeIt == params.end() || !maxPacketLifeTimeIt->is_number_integer()) {
            params["maxPacketLifeTime"] = 0u;
        }

        // maxRetransmits is optional. If unset set it to 0.
        if (maxRetransmitsIt == params.end() || !maxRetransmitsIt->is_number_integer()) {
            params["maxRetransmits"] = 0u;
        }

        if (maxPacketLifeTimeIt != params.end() && maxRetransmitsIt != params.end()) {
            SRV_THROW_TYPE_ERROR("cannot provide both maxPacketLifeTime and maxRetransmits");
        }

        if (orderedGiven &&
            params["ordered"] == true &&
            (maxPacketLifeTimeIt != params.end() || maxRetransmitsIt != params.end())) {
            SRV_THROW_TYPE_ERROR("cannot be ordered with maxPacketLifeTime or maxRetransmits");
        }
        else if (!orderedGiven &&
            (maxPacketLifeTimeIt != params.end() || maxRetransmitsIt != params.end()) )
        {
            params["ordered"] = false;
        }
        
        // // label is optional. If unset set it to empty string.
        // if (labelIt == params.end() || !labelIt->is_string()) {
        //     params["label"] = "";
        // }
 
        // // protocol is optional. If unset set it to empty string.
        // if (protocolIt == params.end() || !protocolIt->is_string()) {
        //     params["protocol"] = "";
        // }
    }

    RtpCapabilities ortc::generateRouterRtpCapabilities(const std::vector<RtpCodecCapability>& mediaCodecs)
    {
        // Normalize supported RTP capabilities.
        nlohmann::json cap = _supportedRtpCapabilities;
        validateRtpCapabilities(cap);

        auto clonedSupportedRtpCapabilities = _supportedRtpCapabilities;
        std::vector<int> dynamicPayloadTypes;
        dynamicPayloadTypes.insert(dynamicPayloadTypes.end(), DynamicPayloadTypes.begin(), DynamicPayloadTypes.end());
          
        RtpCapabilities caps;
        caps.headerExtensions = clonedSupportedRtpCapabilities.headerExtensions;

        for (auto &mediaCodec : mediaCodecs) {
          // This may throw.
          nlohmann::json jmediaCodec = mediaCodec;
          validateRtpCodecCapability(jmediaCodec);

          RtpCodecCapability matchedSupportedCodec;
          bool matched = false;
          for (auto &supportedCodec : clonedSupportedRtpCapabilities.codecs) {
              nlohmann::json jsupportedCodec = supportedCodec;
              matched = matchCodecs(jmediaCodec, jsupportedCodec,/*strict*/ false, /*modify*/ false);
              if (matched == true) {
                  matchedSupportedCodec = supportedCodec;
                  break;
              }
          }
            
          if (matched == false) {
              SRV_LOGD("media codec not supported [mimeType:%s]", mediaCodec.mimeType.c_str());
              continue;
          }

          // Clone the supported codec.
          auto codec = matchedSupportedCodec;

          // If the given media codec has preferredPayloadType, keep it.
          if (mediaCodec.preferredPayloadType != 0) {
            codec.preferredPayloadType = mediaCodec.preferredPayloadType;

            // Also remove the pt from the list : available dynamic values.
            auto idx = std::find(dynamicPayloadTypes.begin(), dynamicPayloadTypes.end(), codec.preferredPayloadType);
            if (idx != dynamicPayloadTypes.end()) {
                dynamicPayloadTypes.erase(idx);
            }
          }
          // Otherwise if the supported codec has preferredPayloadType, use it.
          else if (codec.preferredPayloadType != 0) {
            // No need to remove it from the list since it"s not a dynamic value.
          }
          // Otherwise choose a dynamic one.
          else {
              // Take the first available pt and remove it from the list.
              auto k = dynamicPayloadTypes.begin();
              auto pt = *k;
              dynamicPayloadTypes.erase(k);
              
              if (!pt) {
                  SRV_THROW_TYPE_ERROR("cannot allocate more dynamic codec payload types");
              }

              codec.preferredPayloadType = pt;
          }

          // Ensure there is not duplicated preferredPayloadType values.
          for (auto it = caps.codecs.begin(); it != caps.codecs.end();) {
              if ((*it).preferredPayloadType == codec.preferredPayloadType) {
                  SRV_THROW_TYPE_ERROR("duplicated codec.preferredPayloadType = %d", codec.preferredPayloadType);
              }
              else {
                  ++it;
              }
          }

          // Merge the media codec parameters.
          for (auto &kv : mediaCodec.parameters) {
              codec.parameters[kv.first] = kv.second;
          }

          // Append to the codec list.
          caps.codecs.push_back(codec);

          // Add a RTX video codec if video.
          if (codec.kind == "video") {
              // Take the first available pt and remove it from the list.
              auto k = dynamicPayloadTypes.begin();
              auto pt = *k;
              dynamicPayloadTypes.erase(k);
              if (!pt) {
                  SRV_THROW_TYPE_ERROR("cannot allocate more dynamic codec payload types");
              }
              std::map<std::string, nlohmann::json> parameters;
              
              parameters[std::string("apt")] = codec.preferredPayloadType;
              
              RtpCodecCapability rtxCodec;
              rtxCodec.kind = codec.kind;
              rtxCodec.mimeType = codec.kind+"/rtx";
              rtxCodec.preferredPayloadType = pt;
              rtxCodec.clockRate = codec.clockRate;
              rtxCodec.parameters = parameters;

              // Append to the codec list.
              caps.codecs.push_back(rtxCodec);
          }
        }

        return caps;
    }

    int32_t ortc::getMultiOpusNumStreams(const nlohmann::json& codec)
    {
        auto& parameters = codec["parameters"];
        auto numStreamsIt = parameters.find("num_streams");

        if (numStreamsIt == parameters.end() || !numStreamsIt->is_number_integer() ) {
            return 0;
        }

        return numStreamsIt->get<int32_t>();
    }

    int32_t ortc::getMultiOpusCoupledStreams(const nlohmann::json& codec)
    {
        auto& parameters = codec["parameters"];
        auto coupledStreamsIt = parameters.find("coupled_streams");

        if (coupledStreamsIt == parameters.end() || !coupledStreamsIt->is_number_integer() ) {
            return 0;
        }

        return coupledStreamsIt->get<int32_t>();
    }

    uint8_t ortc::getH264PacketizationMode(const nlohmann::json& codec)
    {
        auto& parameters = codec["parameters"];
        auto packetizationModeIt = parameters.find("packetization-mode");

        // clang-format off
        if (packetizationModeIt == parameters.end() || !packetizationModeIt->is_number_integer()) {
            return 0;
        }

        return packetizationModeIt->get<uint8_t>();
    }

    uint8_t ortc::getH264LevelAssimetryAllowed(const nlohmann::json& codec)
    {
        const auto& parameters = codec["parameters"];
        auto levelAssimetryAllowedIt = parameters.find("level-asymmetry-allowed");

        if (levelAssimetryAllowedIt == parameters.end() || !levelAssimetryAllowedIt->is_number_integer()) {
            return 0;
        }

        return levelAssimetryAllowedIt->get<uint8_t>();
    }

    std::string ortc::getH264ProfileLevelId(const nlohmann::json& codec)
    {
        const auto& parameters = codec["parameters"];
        auto profileLevelIdIt = parameters.find("profile-level-id");

        if (profileLevelIdIt == parameters.end()) {
            return "";
        }
        else if (profileLevelIdIt->is_number()) {
            return std::to_string(profileLevelIdIt->get<int32_t>());
        }
        else {
            return profileLevelIdIt->get<std::string>();
        }
    }

    std::string ortc::getVP9ProfileId(const nlohmann::json& codec)
    {
        const auto& parameters = codec["parameters"];
        auto profileIdIt = parameters.find("profile-id");

        if (profileIdIt == parameters.end()) {
            return "0";
        }
        
        if (profileIdIt->is_number()) {
            return std::to_string(profileIdIt->get<int32_t>());
        }
        else {
            return profileIdIt->get<std::string>();
        }
    }

    bool ortc::matchCodecs(nlohmann::json& aCodec, const nlohmann::json& bCodec, bool strict, bool modify)
    {
         auto aMimeTypeIt = aCodec.find("mimeType");
         auto bMimeTypeIt = bCodec.find("mimeType");
        
         auto aMimeType = aMimeTypeIt->get<std::string>();
         auto bMimeType = bMimeTypeIt->get<std::string>();

         std::transform(aMimeType.begin(), aMimeType.end(), aMimeType.begin(), ::tolower);
         std::transform(bMimeType.begin(), bMimeType.end(), bMimeType.begin(), ::tolower);

        if (aMimeType != bMimeType) {
            return false;
        }

        if (aCodec["clockRate"] != bCodec["clockRate"]) {
            return false;
        }

         if (aMimeType == "audio/opus") {
             if (aCodec.contains("channels") != bCodec.contains("channels")) {
                 return false;
             }
             if (aCodec.contains("channels") && aCodec["channels"] != bCodec["channels"]) {
                 return false;
             }
         }
        
        if (aMimeType == "audio/multiopus") {
            auto aNumStreams = getMultiOpusNumStreams(aCodec);
            auto bNumStreams = getMultiOpusNumStreams(bCodec);

            if (aNumStreams != bNumStreams) {
                return false;
            }

            auto aCoupledStreams = getMultiOpusCoupledStreams(aCodec);
            auto bCoupledStreams = getMultiOpusCoupledStreams(bCodec);

            if (aCoupledStreams != bCoupledStreams) {
                return false;
            }
        }
        else if (aMimeType == "video/h264" || aMimeType == "video/h264-svc") {
            // If strict matching check profile-level-id.
            if (strict) {
                auto aPacketizationMode = getH264PacketizationMode(aCodec);
                auto bPacketizationMode = getH264PacketizationMode(bCodec);

                if (aPacketizationMode != bPacketizationMode) {
                    return false;
                }
                
                webrtc::H264::CodecParameterMap aParameters;
                webrtc::H264::CodecParameterMap bParameters;

                aParameters["level-asymmetry-allowed"] = std::to_string(getH264LevelAssimetryAllowed(aCodec));
                aParameters["packetization-mode"] = std::to_string(aPacketizationMode);
                aParameters["profile-level-id"] = getH264ProfileLevelId(aCodec);
                bParameters["level-asymmetry-allowed"] = std::to_string(getH264LevelAssimetryAllowed(bCodec));
                bParameters["packetization-mode"] = std::to_string(bPacketizationMode);
                bParameters["profile-level-id"] = getH264ProfileLevelId(bCodec);

                if (!webrtc::H264::IsSameH264Profile(aParameters, bParameters)) {
                    return false;
                }

                webrtc::H264::CodecParameterMap newParameters;

                try {
                    webrtc::H264::GenerateProfileLevelIdForAnswer(aParameters, bParameters, &newParameters);
                }
                catch (std::runtime_error&) {
                    return false;
                }

                if (modify) {
                    auto profileLevelIdIt = newParameters.find("profile-level-id");

                    if (profileLevelIdIt != newParameters.end()) {
                        aCodec["parameters"]["profile-level-id"] = profileLevelIdIt->second;
                    }
                    else {
                        aCodec["parameters"].erase("profile-level-id");
                    }
                }
            }
        }
        // Match VP9 parameters.
        else if (aMimeType == "video/vp9") {
            // If strict matching check profile-id.
            if (strict) {
                auto aProfileId = getVP9ProfileId(aCodec);
                auto bProfileId = getVP9ProfileId(bCodec);

                if (aProfileId != bProfileId) {
                    return false;
                }
            }
        }

        return true;
    }

    bool ortc::isRtxCodec(const nlohmann::json& codec)
    {
        static const std::regex RtxMimeTypeRegex("^(audio|video)/rtx$", std::regex_constants::ECMAScript | std::regex_constants::icase);

        std::smatch match;
        std::string mimeType = codec["mimeType"];

        return std::regex_match(mimeType, match, RtxMimeTypeRegex);
    }

    nlohmann::json ortc::getProducerRtpParametersMapping(const RtpParameters& params, const RtpCapabilities& caps)
    {
        RtpMapping rtpMapping;
        
        // Match parameters media codecs to capabilities media codecs.
        std::unordered_map<int,std::pair<RtpCodecParameters,RtpCodecCapability>> codecToCapCodec;
        for (auto &codec : params.codecs) {
            if (isRtxCodec(codec)) {
                continue;
            }
            // Search for the same media codec in capabilities.
            RtpCodecCapability matchedCapCodec;
            bool matched = false;
            for (auto &capCodec : caps.codecs) {
                nlohmann::json jcapCodec = capCodec;
                nlohmann::json jcodec = codec;
                matched = matchCodecs(jcodec, jcapCodec, /*strict*/ true, /*modify*/ true);
                if (matched == true) {
                    matchedCapCodec = capCodec;
                    break;
                }
            }
            
            if (matched == false) {
                SRV_THROW_TYPE_ERROR("unsupported codec [mimeType:%s, payloadType: %d]", codec.mimeType.c_str(), codec.payloadType);
            }
            
            codecToCapCodec[codec.payloadType] = std::make_pair(codec,matchedCapCodec);
        }
        
        // Match parameters RTX codecs to capabilities RTX codecs.
        for (auto codec : params.codecs) {
            if (!isRtxCodec(codec)) {
                continue;
            }
            
            // Search for the associated media codec.
            RtpCodecParameters associatedMediaCodec;
            bool matched = false;
            int apt = 0;
            for (auto & pair : codec.getParameters()) {
                if (pair.first == "apt") {
                    apt = pair.second;
                }
            }
            for (auto &mediaCodec : params.codecs) {
                matched = (mediaCodec.payloadType == apt);
                if (matched == true) {
                    associatedMediaCodec = mediaCodec;
                    break;
                }
            }
            
            if (matched == false) {
                SRV_THROW_TYPE_ERROR("missing media codec found for RTX PT codec.payloadType = %d", codec.payloadType);
            }
            
            auto capMediaCodec = codecToCapCodec[associatedMediaCodec.payloadType].second;
            
            // Ensure that the capabilities media codec has a RTX codec.
            RtpCodecCapability associatedCapRtxCodec;
            bool associatedCapRtxCodecMatched = false;
            for (auto &capCodec : caps.codecs) {
                int capapt = 0;
                for (auto & pair : capCodec.parameters) {
                    if (pair.first == "apt") {
                        capapt = pair.second;
                    }
                }
                if (isRtxCodec(capCodec) && capapt == capMediaCodec.preferredPayloadType) {
                    associatedCapRtxCodec = capCodec;
                    associatedCapRtxCodecMatched = true;
                }
            }
            
            if (associatedCapRtxCodecMatched == false) {
                SRV_THROW_TYPE_ERROR("no RTX codec for capability codec PT ${capMediaCodec.preferredPayloadType}");
            }
            codecToCapCodec[codec.payloadType] = std::make_pair(codec,associatedCapRtxCodec);
        }
        
        // Generate codecs mapping.
        for (const auto& kp : codecToCapCodec)
        {
            //auto& codec1 = kp.first;
            const auto& kv = kp.second;
            
            auto& codec = kv.first;
            const auto& capCodec = kv.second;
            nlohmann::json _codec;
            _codec["payloadType"] = codec.payloadType;
            _codec["mappedPayloadType"] = capCodec.preferredPayloadType;
            rtpMapping.codecs.push_back(_codec);
        }
        
        // Generate encodings mapping.
        auto mappedSsrc = srv::getRandomInteger(100000000, 999999999);
        
        for (auto &encoding : params.encodings) {
            RtpEncodingMapping mappedEncoding;
            
            mappedEncoding.mappedSsrc = mappedSsrc++;
            
            if (!encoding.rid.empty()) {
                mappedEncoding.rid = encoding.rid;
            }
            if (encoding.ssrc != 0) {
                mappedEncoding.ssrc = encoding.ssrc;
            }
            if (!encoding.scalabilityMode.empty()) {
                mappedEncoding.scalabilityMode = encoding.scalabilityMode;
            }
            
            nlohmann::json jmappedEncoding;
            jmappedEncoding["ssrc"] = mappedEncoding.ssrc;
            jmappedEncoding["rid"] = mappedEncoding.rid;
            jmappedEncoding["scalabilityMode"] = mappedEncoding.scalabilityMode;
            jmappedEncoding["mappedSsrc"] = mappedEncoding.mappedSsrc;
            
            if (mappedEncoding.ssrc == 0) {
                jmappedEncoding.erase("ssrc");
            }
            if (mappedEncoding.rid.empty()) {
                jmappedEncoding.erase("rid");
            }
            if (mappedEncoding.scalabilityMode.empty()) {
                jmappedEncoding.erase("scalabilityMode");
            }
            if (mappedEncoding.mappedSsrc == 0) {
                jmappedEncoding.erase("mappedSsrc");
            }
            rtpMapping.encodings.push_back(jmappedEncoding);
        }
        
        nlohmann::json jRtpMapping;
        jRtpMapping["codecs"] =  rtpMapping.codecs;
        jRtpMapping["encodings"] = rtpMapping.encodings;
        
        return jRtpMapping;
    }

    nlohmann::json ortc::getConsumableRtpParameters(const std::string& kind, const RtpParameters& params, const RtpCapabilities& caps, const nlohmann::json& rtpMapping)
    {
        RtpParameters consumableParams;
        
        for (auto codec : params.codecs) {
            if (isRtxCodec(codec)) {
                continue;
            }
            int consumableCodecPt = 0;
            for (auto &entry : rtpMapping["codecs"]) {
                if (entry["payloadType"] == codec.payloadType) {
                    consumableCodecPt = entry["mappedPayloadType"];
                    break;
                }
            }
            
            RtpCodecCapability matchedCapCodec;
            bool matched = false;
            for (auto &capCodec : caps.codecs) {
                matched = ( capCodec.preferredPayloadType == consumableCodecPt);
                if (matched == true) {
                    matchedCapCodec = capCodec;
                    break;
                }
            }
            
            RtpCodecParameters consumableCodec;
            consumableCodec.mimeType = matchedCapCodec.mimeType;
            consumableCodec.payloadType = matchedCapCodec.preferredPayloadType;
            consumableCodec.clockRate = matchedCapCodec.clockRate;
            consumableCodec.channels = matchedCapCodec.channels;
            consumableCodec.setParameters(codec.getParameters()); // Keep the Producer codec parameters.
            consumableCodec.rtcpFeedback = matchedCapCodec.rtcpFeedback;
            
            consumableParams.codecs.push_back(consumableCodec);
            
            RtpCodecCapability consumableCapRtxCodec;
            bool consumableCapRtxCodecMatched = false;
            for (auto &capRtxCodec : caps.codecs) {
                nlohmann::json jcapRtxCodec = capRtxCodec;
                int capapt = 0;
                for (auto & pair : capRtxCodec.parameters) {
                    if (pair.first == "apt") {
                        capapt = pair.second;
                    }
                }
                if (isRtxCodec(jcapRtxCodec) && capapt == consumableCodec.payloadType) {
                    consumableCapRtxCodecMatched = true;
                }
                
                if (consumableCapRtxCodecMatched == true) {
                    consumableCapRtxCodec = capRtxCodec;
                    break;
                }
            }
            
            if (consumableCapRtxCodecMatched) {
                RtpCodecParameters consumableRtxCodec;
                consumableRtxCodec.mimeType = consumableCapRtxCodec.mimeType;
                consumableRtxCodec.payloadType = consumableCapRtxCodec.preferredPayloadType;
                consumableRtxCodec.clockRate = consumableCapRtxCodec.clockRate;
                consumableRtxCodec.setParameters(consumableCapRtxCodec.parameters);
                consumableRtxCodec.rtcpFeedback = consumableCapRtxCodec.rtcpFeedback;
                consumableParams.codecs.push_back(consumableRtxCodec);
            }
        }
        
        for (auto capExt : caps.headerExtensions) {
            // Just take RTP header extension that can be used in Consumers.
            if (capExt.kind != kind || (capExt.direction != "sendrecv" && capExt.direction != "sendonly")) {
                continue;
            }
            
            RtpHeaderExtensionParameters consumableExt;
            consumableExt.uri = capExt.uri;
            consumableExt.id = capExt.preferredId;
            consumableExt.encrypt = capExt.preferredEncrypt;
            
            consumableParams.headerExtensions.push_back(consumableExt);
        }
        
        // Clone Producer encodings since we"ll mangle them.
        std::vector<RtpEncodingParameters> consumableEncodings;
        consumableEncodings.insert(consumableEncodings.end(), params.encodings.begin(), params.encodings.end());
        
        for (int i = 0; i < (int32_t)consumableEncodings.size(); ++i) {
            auto consumableEncoding = consumableEncodings[i];
            auto mappedSsrc = rtpMapping["encodings"][i]["mappedSsrc"];
            consumableEncoding.rid = "";
            consumableEncoding.rtx.ssrc = 0;
            consumableEncoding.codecPayloadType = 0;
            
            // Set the mapped ssrc.
            consumableEncoding.ssrc = mappedSsrc;
            
            consumableParams.encodings.push_back(consumableEncoding);
        }
        
        consumableParams.rtcp.cname = params.rtcp.cname;
        consumableParams.rtcp.reducedSize = true;
        consumableParams.rtcp.mux = true;
        nlohmann::json jconsumableParams = consumableParams;
        
        return jconsumableParams;
    }

    bool ortc::canConsume(const RtpParameters& consumableParams, const RtpCapabilities& caps)
    {
        // This may throw.
        nlohmann::json jcaps = caps;
        validateRtpCapabilities(jcaps);
        
        std::vector<RtpCodecParameters> matchingCodecs;
        for (auto codec : consumableParams.codecs) {
            RtpCodecCapability matchedCapCodec;
            bool matched = false;
            for (auto &capCodec : caps.codecs) {
                nlohmann::json jcapCodec = capCodec;
                nlohmann::json jcodec = codec;
                matched = matchCodecs(jcapCodec, jcodec, /*strict*/ true, /*modify*/ false);
                if (matched == true) {
                    matchedCapCodec = capCodec;
                    break;
                }
            }
            
            if (!matched) {
                continue;
            }
            
            matchingCodecs.push_back(codec);
        }
        
        // Ensure there is at least one media codec.
        if (matchingCodecs.size() == 0 || isRtxCodec(matchingCodecs[0])) {
            return false;
        }
        
        return true;
    }

    RtpParameters ortc::getConsumerRtpParameters(const RtpParameters& consumableRtpParameters, const RtpCapabilities& remoteRtpCapabilities, bool pipe, bool enableRtx)
    {
        RtpParameters consumerParams;
        consumerParams.rtcp = consumableRtpParameters.rtcp;
        
        for (const auto& capCodec : remoteRtpCapabilities.codecs) {
            nlohmann::json jcapCodec = capCodec;
            validateRtpCodecCapability(jcapCodec);
        }

        std::vector<RtpCodecParameters> consumableCodecs;
        consumableCodecs.insert(consumableCodecs.end(), consumableRtpParameters.codecs.begin(), consumableRtpParameters.codecs.end());
        auto rtxSupported = false;
        
        for (auto& codec : consumableCodecs) {
            if (!enableRtx && isRtxCodec(codec)) {
                continue;
            }
            
            RtpCodecCapability matchedCapCodec;
            bool matched = false;
            for (const auto& capCodec : remoteRtpCapabilities.codecs) {
                nlohmann::json jcapCodec = capCodec;
                nlohmann::json jcodec = codec;
                matched = matchCodecs(jcapCodec, jcodec,/*strict*/ true, /*modify*/ false);
                if (matched == true) {
                    matchedCapCodec = capCodec;
                    break;
                }
            }
            
            if (!matched) {
                continue;
            }
            
            std::vector<RtcpFeedback> filtedRtcpFeedback;
            for (const auto& fb : matchedCapCodec.rtcpFeedback) {
                if (enableRtx || fb.type != "nack" || !fb.parameter.empty()) {
                    filtedRtcpFeedback.emplace_back(fb);
                }
            }
            codec.rtcpFeedback = filtedRtcpFeedback;
            
            consumerParams.codecs.push_back(codec);
        }
        
        // Must sanitize the list : matched codecs by removing useless RTX codecs.
        for (int idx = ((int32_t)consumerParams.codecs.size() - 1); idx >= 0; --idx) {
            auto codec = consumerParams.codecs[idx];
            
            if (isRtxCodec(codec)) {
                // Search for the associated media codec.
                RtpCodecParameters associatedMediaCodec;
                bool associatedMediaCodecMatched = false;
                for (auto& mediaCodec : consumerParams.codecs) {
                    nlohmann::json jmediaCodec = mediaCodec;
                    int capapt = 0;
                    for (auto & pair : codec.getParameters()) {
                        if (pair.first == "apt") {
                            capapt = pair.second;
                        }
                    }
                    associatedMediaCodecMatched = ( mediaCodec.payloadType == capapt);
                    if (associatedMediaCodecMatched == true) {
                        associatedMediaCodec = mediaCodec;
                        break;
                    }
                }
                
                if (associatedMediaCodecMatched) {
                    rtxSupported = true;
                }
                else {
                    int index = 0;
                    for (auto it = consumerParams.codecs.begin(); it != consumerParams.codecs.end();) {
                        if (index == idx) {
                            consumerParams.codecs.erase(it);
                        }
                        else {
                            ++it;
                        }
                        index++;
                    }
                }
            }
        }
        
        // Ensure there is at least one media codec.
        if (consumerParams.codecs.size() == 0 || isRtxCodec(consumerParams.codecs[0])) {
            SRV_THROW_TYPE_ERROR("no compatible media codecs");
        }
        
        std::vector<RtpHeaderExtensionParameters> headerExtensions;
        bool matched = false;
        bool have01 = false;
        bool have02 = false;
        for (auto& ext : consumableRtpParameters.headerExtensions) {
            for (auto& capExt : remoteRtpCapabilities.headerExtensions) {
                matched = ( (capExt.preferredId == ext.id) && (capExt.uri == ext.uri) );
                if (matched == true) {
                    headerExtensions.push_back(ext);
                }
            }
        }
        consumerParams.headerExtensions.clear();
        consumerParams.headerExtensions.insert(consumerParams.headerExtensions.end(), headerExtensions.begin(), headerExtensions.end());
        
        for (auto &ext : consumableRtpParameters.headerExtensions) {
            if (ext.uri == "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01") {
                have01 = true;
            }
            else if (ext.uri == "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time") {
                have02 = true;
            }
        }
        
        // Reduce codecs" RTCP feedback. Use Transport-CC if available, REMB otherwise.
        if (have01) {
            for (auto &codec : consumerParams.codecs) {
                std::vector<RtcpFeedback> rtcpFeedback;
                for (auto &fb : codec.rtcpFeedback) {
                    if (fb.type != "goog-remb") {
                        rtcpFeedback.push_back(fb);
                    }
                }
                codec.rtcpFeedback.clear();
                codec.rtcpFeedback.insert(codec.rtcpFeedback.end(), rtcpFeedback.begin(), rtcpFeedback.end());
            }
        }
        else if (have02) {
            for (auto &codec : consumerParams.codecs) {
                std::vector<RtcpFeedback> rtcpFeedback;
                for (auto &fb : codec.rtcpFeedback) {
                    if (fb.type != "transport-cc") {
                        rtcpFeedback.push_back(fb);
                    }
                }
                codec.rtcpFeedback.clear();
                codec.rtcpFeedback.insert(codec.rtcpFeedback.end(), rtcpFeedback.begin(), rtcpFeedback.end());
            }
        }
        else {
            for (auto &codec : consumerParams.codecs) {
                std::vector<RtcpFeedback> rtcpFeedback;
                for (auto &fb : codec.rtcpFeedback) {
                    if (fb.type != "transport-cc" && fb.type != "goog-remb") {
                        rtcpFeedback.push_back(fb);
                    }
                }
                codec.rtcpFeedback.clear();
                codec.rtcpFeedback.insert(codec.rtcpFeedback.end(), rtcpFeedback.begin(), rtcpFeedback.end());
            }
        }
        
        if (!pipe) {
            RtpEncodingParameters consumerEncoding;
            consumerEncoding.ssrc = srv::getRandomInteger((uint32_t)100000000, (uint32_t)999999999);
            
            if (rtxSupported) {
                consumerEncoding.rtx.ssrc = (consumerEncoding.ssrc + 1 );
            }
            
            // If any : the consumableRtpParameters.encodings has scalabilityMode, process it (assume all encodings have the same value).
            RtpEncodingParameters encodingWithScalabilityMode;
            for (auto &encoding : consumableRtpParameters.encodings) {
                if (!encoding.scalabilityMode.empty()) {
                    encodingWithScalabilityMode = encoding;
                    break;
                }
            }
            
            auto scalabilityMode = encodingWithScalabilityMode.scalabilityMode;
            
            // If there is simulast, mangle spatial layers in scalabilityMode.
            if (consumableRtpParameters.encodings.size() > 1) {
                auto jScalabilityMode = parseScalabilityMode(scalabilityMode);
                scalabilityMode = "L" + std::to_string(consumableRtpParameters.encodings.size()) + "T" + std::to_string(jScalabilityMode["temporalLayers"].get<int32_t>());
            }
            
            if (!scalabilityMode.empty()) {
                consumerEncoding.scalabilityMode = scalabilityMode;
            }
            
            int maxEncodingMaxBitrate = 0;
            int maxBitrate = 0;
            for (auto &encoding : consumableRtpParameters.encodings) {
                maxBitrate = encoding.maxBitrate > maxBitrate ? encoding.maxBitrate : maxBitrate;
            }
            
            if (maxEncodingMaxBitrate != 0) {
                consumerEncoding.maxBitrate = maxEncodingMaxBitrate;
            }
            
            // Set a single encoding for the Consumer.
            consumerParams.encodings.push_back(consumerEncoding);
        }
        else {
            std::vector<RtpEncodingParameters> consumableEncodings;
            consumableEncodings.insert(consumableEncodings.end(), consumableRtpParameters.encodings.begin(), consumableRtpParameters.encodings.end());
            
            auto baseSsrc = srv::getRandomInteger((uint32_t)100000000, (uint32_t)999999999);
            auto baseRtxSsrc = srv::getRandomInteger((uint32_t)100000000, (uint32_t)999999999);
            
            for (auto i = 0; i < (int32_t)consumableEncodings.size(); ++i) {
                auto encoding = consumableEncodings[i];
                
                encoding.ssrc = baseSsrc + i;
                
                if (rtxSupported) {
                    encoding.rtx.ssrc = baseRtxSsrc + i;
                }
                else {
                    encoding.rtx.ssrc = 0;
                }
                
                consumerParams.encodings.push_back(encoding);
            }
        }
        
        return consumerParams;
    }

    RtpParameters ortc::getPipeConsumerRtpParameters(const RtpParameters& consumableRtpParameters, bool enableRtx)
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

    void from_json(const nlohmann::json& j, RtpCodecMapping& st) {
        if (j.contains("payloadType")) {
            j.at("payloadType").get_to(st.payloadType);
        }
        if (j.contains("mappedPayloadType")) {
            j.at("mappedPayloadType").get_to(st.mappedPayloadType);
        }
    }

    void to_json(nlohmann::json& j, RtpCodecMapping& st) {
        j["payloadType"] = st.payloadType;
        j["mappedPayloadType"] = st.mappedPayloadType;
    }

    void from_json(const nlohmann::json& j, RtpEncodingMapping& st) {
        if (j.contains("ssrc")) {
            j.at("ssrc").get_to(st.ssrc);
        }
        if (j.contains("rid")) {
            j.at("rid").get_to(st.rid);
        }
        if (j.contains("scalabilityMode")) {
            j.at("scalabilityMode").get_to(st.scalabilityMode);
        }
        if (j.contains("mappedSsrc")) {
            j.at("mappedSsrc").get_to(st.mappedSsrc);
        }
    }

    void to_json(nlohmann::json& j, RtpEncodingMapping& st) {
        j["ssrc"] = st.ssrc;
        j["rid"] = st.rid;
        j["scalabilityMode"] = st.scalabilityMode;
        j["mappedSsrc"] = st.mappedSsrc;
    }

    void from_json(const nlohmann::json& j, RtpMapping& st) {
        if (j.contains("codecs")) {
            j.at("codecs").get_to(st.codecs);
        }
        if (j.contains("encodings")) {
            j.at("encodings").get_to(st.encodings);
        }
    }

    void to_json(nlohmann::json& j, RtpMapping& st) {
        j["codecs"] = st.codecs;
        j["encodings"] = st.encodings;
    }

    flatbuffers::Offset<FBS::RtpParameters::RtpMapping> RtpMappingFbs::serialize(flatbuffers::FlatBufferBuilder& builder) const
    {
        // Add rtpMapping.codecs.
        std::vector<flatbuffers::Offset<FBS::RtpParameters::CodecMapping>> codecs;

        for (const auto& kv : this->codecs) {
            codecs.emplace_back(FBS::RtpParameters::CreateCodecMapping(builder, kv.first, kv.second));
        }

        // Add rtpMapping.encodings.
        std::vector<flatbuffers::Offset<FBS::RtpParameters::EncodingMapping>> encodings;
        encodings.reserve(this->encodings.size());

        for (const auto& encodingMapping : this->encodings) {
            encodings.emplace_back(FBS::RtpParameters::CreateEncodingMappingDirect(
              builder,
              encodingMapping.rid.c_str(),
              encodingMapping.ssrc != 0u ? flatbuffers::Optional<uint32_t>(encodingMapping.ssrc)
                                         : flatbuffers::nullopt,
              nullptr, /* capability mode. NOTE: Present in NODE*/
              encodingMapping.mappedSsrc));
        }

        // Build rtpMapping.
        auto rtpMapping = FBS::RtpParameters::CreateRtpMappingDirect(builder, &codecs, &encodings);
        
        return rtpMapping;
    }

    void convert(const nlohmann::json& data, RtpMappingFbs& rtpMapping)
    {
        auto jsonRtpMappingIt = data.find("rtpMapping");

        if (jsonRtpMappingIt == data.end() || !jsonRtpMappingIt->is_object()) {
            SRV_THROW_TYPE_ERROR("missing rtpMapping");
        }

        auto jsonCodecsIt = jsonRtpMappingIt->find("codecs");

        if (jsonCodecsIt == jsonRtpMappingIt->end() || !jsonCodecsIt->is_array()) {
            SRV_THROW_TYPE_ERROR("missing rtpMapping.codecs");
        }

        for (auto& codec : *jsonCodecsIt) {
            if (!codec.is_object())
            {
                SRV_THROW_TYPE_ERROR("wrong entry in rtpMapping.codecs (not an object)");
            }

            auto jsonPayloadTypeIt = codec.find("payloadType");

            if (jsonPayloadTypeIt == codec.end() || !Utils::Json::IsPositiveInteger(*jsonPayloadTypeIt)) {
                SRV_THROW_TYPE_ERROR("wrong entry in rtpMapping.codecs (missing payloadType)");
            }

            auto jsonMappedPayloadTypeIt = codec.find("mappedPayloadType");

            if (jsonMappedPayloadTypeIt == codec.end() || !Utils::Json::IsPositiveInteger(*jsonMappedPayloadTypeIt)) {
                SRV_THROW_TYPE_ERROR("wrong entry in rtpMapping.codecs (missing mappedPayloadType)");
            }

            rtpMapping.codecs[jsonPayloadTypeIt->get<uint8_t>()] = jsonMappedPayloadTypeIt->get<uint8_t>();
        }

        auto jsonEncodingsIt = jsonRtpMappingIt->find("encodings");

        if (jsonEncodingsIt == jsonRtpMappingIt->end() || !jsonEncodingsIt->is_array()) {
            SRV_THROW_TYPE_ERROR("missing rtpMapping.encodings");
        }

        rtpMapping.encodings.reserve(jsonEncodingsIt->size());

        for (auto& encoding : *jsonEncodingsIt) {
            if (!encoding.is_object()) {
                SRV_THROW_TYPE_ERROR("wrong entry in rtpMapping.encodings");
            }

            rtpMapping.encodings.emplace_back();

            auto& encodingMapping = rtpMapping.encodings.back();

            // ssrc is optional.
            auto jsonSsrcIt = encoding.find("ssrc");

            if (jsonSsrcIt != encoding.end() && Utils::Json::IsPositiveInteger(*jsonSsrcIt)) {
                encodingMapping.ssrc = jsonSsrcIt->get<uint32_t>();
            }

            // rid is optional.
            auto jsonRidIt = encoding.find("rid");

            if (jsonRidIt != encoding.end() && jsonRidIt->is_string()) {
                encodingMapping.rid = jsonRidIt->get<std::string>();
            }

            // However ssrc or rid must be present (if more than 1 encoding).
            if (jsonEncodingsIt->size() > 1 && jsonSsrcIt == encoding.end() && jsonRidIt == encoding.end()) {
                SRV_THROW_TYPE_ERROR("wrong entry in rtpMapping.encodings (missing ssrc or rid)");
            }

            // // If there is no mid and a single encoding, ssrc or rid must be present.
            // if (rtpParameters.mid.empty() &&
            //     jsonEncodingsIt->size() == 1 &&
            //     jsonSsrcIt == encoding.end() &&
            //     jsonRidIt == encoding.end()) {
            //     SRV_THROW_TYPE_ERROR("wrong entry in rtpMapping.encodings (missing ssrc or rid, or rtpParameters.mid)");
            // }

            // mappedSsrc is mandatory.
            auto jsonMappedSsrcIt = encoding.find("mappedSsrc");

            if (jsonMappedSsrcIt == encoding.end() || !Utils::Json::IsPositiveInteger(*jsonMappedSsrcIt)) {
                SRV_THROW_TYPE_ERROR("wrong entry in rtpMapping.encodings (missing mappedSsrc)");
            }

            encodingMapping.mappedSsrc = jsonMappedSsrcIt->get<uint32_t>();
        }
    }
}
