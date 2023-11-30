/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <string>
#include <vector>
#include <map>
#include <absl/container/flat_hash_map.h>
#include <absl/types/optional.h>
#include "nlohmann/json.hpp"
#include "flatbuffers/flexbuffers.h"
#include "parameters.h"
#include "FBS/rtpParameters.h"
#include "FBS/rtpPacket.h"

namespace srv
{
    /**
     * Provides information on RTCP feedback messages for a specific codec. Those
     * messages can be transport layer feedback messages or codec-specific feedback
     * messages. The list of RTCP feedbacks supported by mediasoup is defined in the
     * supportedRtpCapabilities.ts file.
     */
    struct RtcpFeedback
    {
        /**
         * RTCP feedback type.
         */
        std::string type;

        /**
         * RTCP feedback parameter.
         */
        std::string parameter;
        
        flatbuffers::Offset<FBS::RtpParameters::RtcpFeedback> serialize(flatbuffers::FlatBufferBuilder& builder) const;
    };

    void to_json(nlohmann::json& j, const RtcpFeedback& st);
    void from_json(const nlohmann::json& j, RtcpFeedback& st);

    /**
     * Provides information on the capabilities of a codec within the RTP
     * capabilities. The list of media codecs supported by mediasoup and their
     * settings is defined in the supportedRtpCapabilities.ts file.
     *
     * Exactly one RtpCodecCapability will be present for each supported combination
     * of parameters that requires a distinct value of preferredPayloadType. For
     * example:
     *
     * - Multiple H264 codecs, each with their own distinct 'packetization-mode' and
     *   'profile-level-id' values.
     * - Multiple VP9 codecs, each with their own distinct 'profile-id' value.
     *
     * RtpCodecCapability entries in the mediaCodecs array of RouterOptions do not
     * require preferredPayloadType field (if unset, mediasoup will choose a random
     * one). If given, make sure it's in the 96-127 range.
     */
    struct RtpCodecCapability
    {
        /**
         * Media kind ('audio' or 'video').
         */
        std::string kind;

        /**
         * The codec MIME media type/subtype (e.g. 'audio/opus', 'video/VP8').
         */
        std::string mimeType;

        /**
         * The preferred RTP payload type.
         */
        int preferredPayloadType = 0;

        /**
         * Codec clock rate expressed in Hertz.
         */
        int clockRate = 0;

        /**
         * The number of channels supported (e.g. two for stereo). Just for audio.
         * Default 1.
         */
        int channels;

        /**
         * Codec specific parameters. Some parameters (such as 'packetization-mode'
         * and 'profile-level-id' in H264 or 'profile-id' in VP9) are critical for
         * codec matching.
         */
        //Parameters parametersFbs;
        
        std::map<std::string, nlohmann::json> parameters;

        /**
         * Transport layer and codec-specific feedback messages for this codec.
         */
        std::vector<RtcpFeedback> rtcpFeedback;
    };

    void to_json(nlohmann::json& j, const RtpCodecCapability& st);
    void from_json(const nlohmann::json& j, RtpCodecCapability& st);

    /**
     * Provides information relating to supported header extensions. The list of
     * RTP header extensions supported by mediasoup is defined in the
     * supportedRtpCapabilities.ts file.
     *
     * mediasoup does not currently support encrypted RTP header extensions. The
     * direction field is just present in mediasoup RTP capabilities (retrieved via
     * router.rtpCapabilities or mediasoup.getSupportedRtpCapabilities()). It's
     * ignored if present in endpoints' RTP capabilities.
     */
    struct RtpHeaderExtension
    {
        /**
         * Media kind.
         */
        std::string kind;

        /*
         * The URI of the RTP header extension, as defined in RFC 5285.
         */
        std::string uri;

        /**
         * The preferred numeric identifier that goes in the RTP packet. Must be
         * unique.
         */
        int preferredId = 0;

        /**
         * If true, it is preferred that the value in the header be encrypted as per
         * RFC 6904. Default false.
         */
        bool preferredEncrypt = false;

        /**
         * If 'sendrecv', mediasoup supports sending and receiving this RTP extension.
         * 'sendonly' means that mediasoup can send (but not receive) it. 'recvonly'
         * means that mediasoup can receive (but not send) it.
         * sendrecv' | 'sendonly' | 'recvonly' | 'inactive'
         */
        std::string direction;
    };

    void to_json(nlohmann::json& j, const RtpHeaderExtension& st);
    void from_json(const nlohmann::json& j, RtpHeaderExtension& st);

    /**
     * The RTP capabilities define what mediasoup or an endpoint can receive at
     * media level.
     */
    struct RtpCapabilities
    {
        /**
         * Supported media and RTX codecs.
         */
        std::vector<RtpCodecCapability> codecs;

        /**
         * Supported RTP header extensions.
         */
        std::vector<RtpHeaderExtension> headerExtensions;
    };

    void to_json(nlohmann::json& j, const RtpCapabilities& st);
    void from_json(const nlohmann::json& j, RtpCapabilities& st);

    /**
     * Provides information on codec settings within the RTP parameters. The list
     * of media codecs supported by mediasoup and their settings is defined in the
     * supportedRtpCapabilities.ts file.
     */
    struct RtpCodecParameters
    {
        /**
         * The codec MIME media type/subtype (e.g. 'audio/opus', 'video/VP8').
         */
        std::string mimeType;

        /**
         * The value that goes in the RTP Payload Type Field. Must be unique.
         */
        uint8_t payloadType = 0;

        /**
         * Codec clock rate expressed in Hertz.
         */
        uint32_t clockRate = 0;

        /**
         * The number of channels supported (e.g. two for stereo). Just for audio.
         * Default 1.
         */
        uint8_t channels = 0;

        /**
         * Transport layer and codec-specific feedback messages for this codec.
         */
        std::vector<RtcpFeedback> rtcpFeedback;
        
        flatbuffers::Offset<FBS::RtpParameters::RtpCodecParameters> serialize(flatbuffers::FlatBufferBuilder& builder) const;
        
        const Parameters& _getParameters() const;
        
        void _setParameters(const Parameters& parameters);
        
        const std::map<std::string, nlohmann::json>& getParameters() const;
        
        void setParameters(const std::map<std::string, nlohmann::json>& parameters);
        
    private:
        /**
         * Codec-specific parameters available for signaling. Some parameters (such
         * as 'packetization-mode' and 'profile-level-id' in H264 or 'profile-id' in
         * VP9) are critical for codec matching.
         */
        Parameters _parameters;
        
        std::map<std::string, nlohmann::json> parameters;
    };

    void to_json(nlohmann::json& j, const RtpCodecParameters& st);
    void from_json(const nlohmann::json& j, RtpCodecParameters& st);

    /**
     * Defines a RTP header extension within the RTP parameters. The list of RTP
     * header extensions supported by mediasoup is defined in the
     * supportedRtpCapabilities.ts file.
     *
     * mediasoup does not currently support encrypted RTP header extensions and no
     * parameters are currently considered.
     */
    struct RtpHeaderExtensionParameters
    {
        /**
         * The URI of the RTP header extension, as defined in RFC 5285.
         */
        std::string uri;

        /**
         * The numeric identifier that goes in the RTP packet. Must be unique.
         */
        int id = 0;

        /**
         * If true, the value in the header is encrypted as per RFC 6904. Default false.
         */
        bool encrypt = false;
        
        flatbuffers::Offset<FBS::RtpParameters::RtpHeaderExtensionParameters> serialize(flatbuffers::FlatBufferBuilder& builder) const;
        
        const Parameters& _getParameters() const;
        
        void _setParameters(const Parameters& parameters);
        
        const std::map<std::string, nlohmann::json>& getParameters() const;
        
        void setParameters(const std::map<std::string, nlohmann::json>& parameters);
        
    private:
        /**
         * Configuration parameters for the header extension.
         */
        Parameters _parameters;
        
        std::map<std::string, nlohmann::json> parameters;
    };

    void to_json(nlohmann::json& j, const RtpHeaderExtensionParameters& st);
    void from_json(const nlohmann::json& j, RtpHeaderExtensionParameters& st);

    struct RtpRtxParameters
    {
        uint32_t ssrc = 0;
        
        flatbuffers::Offset<FBS::RtpParameters::Rtx> serialize(flatbuffers::FlatBufferBuilder& builder) const;
    };

    void to_json(nlohmann::json& j, const RtpRtxParameters& st);
    void from_json(const nlohmann::json& j, RtpRtxParameters& st);

    /**
     * Provides information relating to an encoding, which represents a media RTP
     * stream and its associated RTX stream (if any).
     */
    struct RtpEncodingParameters
    {
        /**
         * The media SSRC.
         */
        uint32_t ssrc = 0;

        /**
         * The RID RTP extension value. Must be unique.
         */
        std::string rid;

        /**
         * Codec payload type this encoding affects. If unset, first media codec is
         * chosen.
         */
        int codecPayloadType = 0;

        /**
         * RTX stream information. It must contain a numeric ssrc field indicating
         * the RTX SSRC.
         */
        RtpRtxParameters rtx;

        /**
         * It indicates whether discontinuous RTP transmission will be used. Useful
         * for audio (if the codec supports it) and for video screen sharing (when
         * static content is being transmitted, this option disables the RTP
         * inactivity checks in mediasoup). Default false.
         */
        bool dtx = false;

        /**
         * Number of spatial and temporal layers in the RTP stream (e.g. 'L1T3').
         * See webrtc-svc.
         */
        std::string scalabilityMode;

        /**
         * Others.
         */
        int scaleResolutionDownBy = 0;

        int maxBitrate = 0;
   
        bool hasCodecPayloadType { false };
        
        bool hasRtx { false };
        
        double maxFramerate { 0 };
     
        uint8_t spatialLayers { 1u };
        
        uint8_t temporalLayers { 1u };
        
        bool ksvc { false };
        
        flatbuffers::Offset<FBS::RtpParameters::RtpEncodingParameters> serialize(flatbuffers::FlatBufferBuilder& builder) const;
    };

    void to_json(nlohmann::json& j, const RtpEncodingParameters& st);
    void from_json(const nlohmann::json& j, RtpEncodingParameters& st);

    /**
     * Provides information on RTCP settings within the RTP parameters.
     *
     * If no cname is given in a producer's RTP parameters, the mediasoup transport
     * will choose a random one that will be used into RTCP SDES messages sent to
     * all its associated consumers.
     *
     * mediasoup assumes reducedSize to always be true.
     */
    struct RtcpParameters
    {
        /**
         * The Canonical Name (CNAME) used by RTCP (e.g. in SDES messages).
         */
        std::string cname;

        /**
         * Whether reduced size RTCP RFC 5506 is configured (if true) or compound RTCP
         * as specified in RFC 3550 (if false). Default true.
         */
        bool reducedSize = true;

        /**
         * Whether RTCP-mux is used. Default true.
         */
        bool mux = true;
        
        flatbuffers::Offset<FBS::RtpParameters::RtcpParameters> serialize(flatbuffers::FlatBufferBuilder& builder) const;
    };

    void to_json(nlohmann::json& j, const RtcpParameters& st);
    void from_json(const nlohmann::json& j, RtcpParameters& st);

    /**
     * The RTP send parameters describe a media stream received by mediasoup from
     * an endpoint through its corresponding mediasoup Producer. These parameters
     * may include a mid value that the mediasoup transport will use to match
     * received RTP packets based on their MID RTP extension value.
     *
     * mediasoup allows RTP send parameters with a single encoding and with multiple
     * encodings (simulcast). In the latter case, each entry in the encodings array
     * must include a ssrc field or a rid field (the RID RTP extension value). Check
     * the Simulcast and SVC sections for more information.
     *
     * The RTP receive parameters describe a media stream as sent by mediasoup to
     * an endpoint through its corresponding mediasoup Consumer. The mid value is
     * unset (mediasoup does not include the MID RTP extension into RTP packets
     * being sent to endpoints).
     *
     * There is a single entry in the encodings array (even if the corresponding
     * producer uses simulcast). The consumer sends a single and continuous RTP
     * stream to the endpoint and spatial/temporal layer selection is possible via
     * consumer.setPreferredLayers().
     *
     * As an exception, previous bullet is not true when consuming a stream over a
     * PipeTransport, in which all RTP streams from the associated producer are
     * forwarded verbatim through the consumer.
     *
     * The RTP receive parameters will always have their ssrc values randomly
     * generated for all of its  encodings (and optional rtx: { ssrc: XXXX } if the
     * endpoint supports RTX), regardless of the original RTP send parameters in
     * the associated producer. This applies even if the producer's encodings have
     * rid set.
     */
    struct RtpParameters
    {
        /**
         * The MID RTP extension value as defined in the BUNDLE specification.
         */
        std::string mid;

        /**
         * Media and RTX codecs in use.
         */
        std::vector<RtpCodecParameters> codecs;

        /**
         * RTP header extensions in use.
         */
        std::vector<RtpHeaderExtensionParameters> headerExtensions;

        /**
         * Transmitted RTP streams and their settings.
         */
        std::vector<RtpEncodingParameters> encodings;

        /**
         * Parameters used for RTCP.
         */
        RtcpParameters rtcp;
        
        flatbuffers::Offset<FBS::RtpParameters::RtpParameters> serialize(flatbuffers::FlatBufferBuilder& builder) const;
    };

    void to_json(nlohmann::json& j, const RtpParameters& st);
    void from_json(const nlohmann::json& j, RtpParameters& st) ;

    std::string rtpHeaderExtensionUriFromFbs(FBS::RtpParameters::RtpHeaderExtensionUri uri);

    FBS::RtpParameters::RtpHeaderExtensionUri rtpHeaderExtensionUriToFbs(const std::string& uri);

    struct RtpPacketDump {
        uint8_t payloadType;
        uint16_t sequenceNumber;
        uint64_t timestamp;
        bool marker;
        uint32_t ssrc;
        bool isKeyFrame;
        uint64_t size;
        uint64_t payloadSize;
        uint8_t spatialLayer;
        uint8_t temporalLayer;
        std::string mid;
        std::string rid;
        std::string rrid;
        absl::optional<uint16_t> wideSequenceNumber;
        
        srv::RtpPacketDump& operator=(const FBS::RtpPacket::Dump* dump) {
            this->payloadType = dump->payloadType();
            this->sequenceNumber = dump->sequenceNumber();
            this->timestamp = dump->timestamp();
            this->marker = dump->marker();
            this->ssrc = dump->ssrc();
            this->isKeyFrame = dump->isKeyFrame();
            this->size = dump->size();
            this->payloadSize = dump->payloadSize();
            this->spatialLayer = dump->spatialLayer();
            this->temporalLayer = dump->temporalLayer();
            this->mid = dump->mid()->str();
            this->rid = dump->rid()->str();
            this->rrid = dump->rrid()->str();
            if (dump->wideSequenceNumber()) {
                this->wideSequenceNumber = dump->wideSequenceNumber().value();
            }
            return *this;
        }
    };

    struct TraceInfo {};

    struct KeyFrameTraceInfo : TraceInfo {
        RtpPacketDump rtpPacket;
        bool isRtx;
    };

    struct FirTraceInfo : TraceInfo {
        uint32_t ssrc;
    };

    struct PliTraceInfo : TraceInfo {
        uint32_t ssrc;
    };

    struct RtpTraceInfo : TraceInfo {
        RtpPacketDump rtpPacket;
        bool isRtx;
    };

    std::shared_ptr<RtpEncodingParameters> parseRtpEncodingParameters(const FBS::RtpParameters::RtpEncodingParameters* data);

    std::shared_ptr<RtpParameters> parseRtpParameters(const FBS::RtpParameters::RtpParameters* data);
}
