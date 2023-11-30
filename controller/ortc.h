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
#include "nlohmann/json.hpp"
#include "rtp_parameters.h"
#include "sctp_parameters.h"
#include "scalability_mode.h"
#include "supported_rtp_capabilities.h"

namespace srv {

    struct RtpCodecMapping
    {
        int32_t payloadType;
        int32_t mappedPayloadType;
    };

    void to_json(nlohmann::json& j, RtpCodecMapping& st);
    void from_json(const nlohmann::json& j, RtpCodecMapping& st);

    struct RtpEncodingMapping
    {
        uint32_t ssrc;
        std::string rid;
        std::string scalabilityMode;
        uint32_t mappedSsrc;
    };

    void to_json(nlohmann::json& j, RtpEncodingMapping& st);
    void from_json(const nlohmann::json& j, RtpEncodingMapping& st);

    struct RtpMapping
    {
        nlohmann::json codecs;
        nlohmann::json encodings;
    };

    void to_json(nlohmann::json& j, RtpMapping& st);
    void from_json(const nlohmann::json& j, RtpMapping& st);

    struct RtpMappingFbs
    {
        absl::flat_hash_map<uint8_t, uint8_t> codecs;
        
        std::vector<RtpEncodingMapping> encodings;
        
        flatbuffers::Offset<FBS::RtpParameters::RtpMapping> serialize(flatbuffers::FlatBufferBuilder& builder) const;
    };

    void convert(const nlohmann::json& data, RtpMappingFbs& rtpMapping);

    class ortc
    {
    public:
        /**
         * Validates RtpCapabilities. It may modify given data by adding missing
         * fields with default values.
         * It throws if invalid.
         */
        static void validateRtpCapabilities(nlohmann::json& caps);
        
        /**
         * Validates RtpCodecCapability. It may modify given data by adding missing
         * fields with default values.
         * It throws if invalid.
         */
        static void validateRtpCodecCapability(nlohmann::json& codec);
        
        /**
         * Validates RtcpFeedback. It may modify given data by adding missing
         * fields with default values.
         * It throws if invalid.
         */
        static void validateRtcpFeedback(nlohmann::json& fb);
        
        /**
         * Validates RtpHeaderExtension. It may modify given data by adding missing
         * fields with default values.
         * It throws if invalid.
         */
        static void validateRtpHeaderExtension(nlohmann::json& ext);
        
        /**
         * Validates RtpParameters. It may modify given data by adding missing
         * fields with default values.
         * It throws if invalid.
         */
        static void validateRtpParameters(nlohmann::json& params);
        
        /**
         * Validates RtpCodecParameters. It may modify given data by adding missing
         * fields with default values.
         * It throws if invalid.
         */
        static void validateRtpCodecParameters(nlohmann::json& codec);
        
        /**
         * Validates RtpHeaderExtensionParameteters. It may modify given data by adding
         * missing fields with default values. It throws if invalid.
         */
        static void validateRtpHeaderExtensionParameters(nlohmann::json& ext);
        
        /**
         * Validates RtpEncodingParameters. It may modify given data by adding missing
         * fields with default values.
         * It throws if invalid.
         */
        static void validateRtpEncodingParameters(nlohmann::json& encoding);
        
        /**
         * Validates RtcpParameters. It may modify given data by adding missing
         * fields with default values.
         * It throws if invalid.
         */
        static void validateRtcpParameters(nlohmann::json& rtcp);
        
        /**
         * Validates SctpCapabilities. It may modify given data by adding missing
         * fields with default values.
         * It throws if invalid.
         */
        static void validateSctpCapabilities(nlohmann::json& caps);
        
        /**
         * Validates NumSctpStreams. It may modify given data by adding missing
         * fields with default values.
         * It throws if invalid.
         */
        static void validateNumSctpStreams(nlohmann::json& numStreams);
        
        /**
         * Validates SctpParameters. It may modify given data by adding missing
         * fields with default values.
         * It throws if invalid.
         */
        static void validateSctpParameters(nlohmann::json& params);
        
        /**
         * Validates SctpStreamParameters. It may modify given data by adding missing
         * fields with default values.
         * It throws if invalid.
         */
        static void validateSctpStreamParameters(nlohmann::json& params);
        
        /**
         * Generate RTP capabilities for the Router based on the given media codecs and
         * mediasoup supported RTP capabilities.
         */
        static RtpCapabilities generateRouterRtpCapabilities(const std::vector<RtpCodecCapability>& mediaCodecs);
        
        /**
         * Get a mapping of codec payloads and encodings of the given Producer RTP
         * parameters as values expected by the Router.
         *
         * It may throw if invalid or non supported RTP parameters are given.
         */
        static nlohmann::json getProducerRtpParametersMapping(const RtpParameters& params, const RtpCapabilities& caps);
        
        /**
         * Generate RTP parameters to be internally used by Consumers given the RTP
         * parameters of a Producer and the RTP capabilities of the Router.
         */
        static nlohmann::json getConsumableRtpParameters(const std::string& kind, const RtpParameters& params, const RtpCapabilities& caps, const nlohmann::json& rtpMapping);
        
        /**
         * Check whether the given RTP capabilities can consume the given Producer.
         */
        static bool canConsume(const RtpParameters& consumableParams, const RtpCapabilities& caps);
        
        /**
         * Generate RTP parameters for a specific Consumer.
         *
         * It reduces encodings to just one and takes into account given RTP capabilities
         * to reduce codecs, codecs" RTCP feedback and header extensions, and also enables
         * or disabled RTX.
         */
        static RtpParameters getConsumerRtpParameters(const RtpParameters& consumableRtpParameters, const RtpCapabilities& remoteRtpCapabilities, bool pipe, bool enableRtx);
        
        /**
         * Generate RTP parameters for a pipe Consumer.
         *
         * It keeps all original consumable encodings and removes support for BWE. If
         * enableRtx is false, it also removes RTX and NACK support.
         */
        static RtpParameters getPipeConsumerRtpParameters(const RtpParameters& consumableRtpParameters, bool enableRtx);
        
    private:
        static int32_t getMultiOpusNumStreams(const nlohmann::json& codec);

        static int32_t getMultiOpusCoupledStreams(const nlohmann::json& codec);

        static uint8_t getH264PacketizationMode(const nlohmann::json& codec);
        
        static uint8_t getH264LevelAssimetryAllowed(const nlohmann::json& codec);

        static std::string getH264ProfileLevelId(const nlohmann::json& codec);

        static std::string getVP9ProfileId(const nlohmann::json& codec);
        
        static bool matchCodecs(nlohmann::json& aCodec, const nlohmann::json& bCodec, bool strict = false, bool modify = false);
        
        static bool isRtxCodec(const nlohmann::json& codec);
    };

}
