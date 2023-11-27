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

        const auto* codecsFBS = data->codecs();
        for (const auto& codec : *codecsFBS) {
            RtpCodecParameters parameters;
            parameters.mimeType = codec->mimeType()->str();
            parameters.payloadType = codec->payloadType();
            parameters.clockRate = codec->clockRate();
            parameters.channels = codec->channels().value_or(0);
            parameters.parameters.Set(codec->parameters());
            for (const auto& feedback : *codec->rtcpFeedback()) {
                RtcpFeedback rtcpFeedback;
                rtcpFeedback.type = feedback->type()->str();
                rtcpFeedback.parameter = feedback->parameter()->str();
                parameters.rtcpFeedback.emplace_back(feedback);
            }
            
            rtpParameters->codecs.emplace_back(parameters);
        }

        for (const auto& headExtension : *data->headerExtensions()) {
            RtpHeaderExtensionParameters parameters;
            parameters.uri = rtpHeaderExtensionUriFromFbs(headExtension->uri());
            parameters.id = headExtension->id();
            parameters.encrypt = headExtension->encrypt();
            parameters.parameters.Set(headExtension->parameters());
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

        rtpParameters->rtcp.cname = data->rtcp()->cname()->str();
        rtpParameters->rtcp.reducedSize = data->rtcp()->reducedSize();
        
        return rtpParameters;
    }

}
