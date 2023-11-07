/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "supported_rtp_capabilities.h"
#include "rtp_parameters.h"


namespace srv
{
    //c++14
    const RtpCapabilities _supportedRtpCapabilities =
    {
        // codecs
        {
            {
                "audio",		// kind
                "audio/opus",	// mimeType
                0,				// preferredPayloadType
                48000,			// clockRate
                2,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "nack",  "" },
                    { "transport-cc", "" }
                }
            },
            {
                "audio",            // kind
                "audio/multiopus",	// mimeType
                0,                  // preferredPayloadType
                48000,              // clockRate
                4,                  // channels
                   // parameters
                {
                    { "channel_mapping", "0,1,2,3" },
                    { "num_streams", 2 },
                    { "coupled_streams", 2 }
                },
                   // rtcpFeedback
                {
                    { "nack",  "" },
                    { "transport-cc", "" }
                }
            },
            {
                "audio",            // kind
                "audio/multiopus",	// mimeType
                0,                  // preferredPayloadType
                48000,              // clockRate
                6,                  // channels
                   // parameters
                {
                    { "channel_mapping", "0,4,1,2,3,5" },
                    { "num_streams", 4 },
                    { "coupled_streams", 2 }
                },
                   // rtcpFeedback
                {
                    { "nack",  "" },
                    { "transport-cc", "" }
                }
            },
            {
                "audio",            // kind
                "audio/multiopus",	// mimeType
                0,                  // preferredPayloadType
                48000,              // clockRate
                8,                  // channels
                   // parameters
                {
                    { "channel_mapping", "0,6,1,2,3,4,5,7" },
                    { "num_streams", 5 },
                    { "coupled_streams", 3 }
                },
                   // rtcpFeedback
                {
                    { "nack",  "" },
                    { "transport-cc", "" }
                }
            },
            {
                "audio",		// kind
                "audio/PCMU",	// mimeType
                0,				// preferredPayloadType
                8000,			// clockRate
                1,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "transport-cc", "" }
                }
            },
            {
                "audio",		// kind
                "audio/PCMA",	// mimeType
                8,				// preferredPayloadType
                8000,			// clockRate
                1,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "transport-cc", "" }
                }
            },
            {
                "audio",		// kind
                "audio/ISAC",	// mimeType
                0,				// preferredPayloadType
                32000,			// clockRate
                1,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "transport-cc", "" }
                }
            },
            {
                "audio",		// kind
                "audio/ISAC",	// mimeType
                0,				// preferredPayloadType
                16000,			// clockRate
                1,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "transport-cc", "" }
                }
            },
            {
                "audio",		// kind
                "audio/G722",	// mimeType
                9,				// preferredPayloadType
                8000,			// clockRate
                1,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "transport-cc", "" }
                }
            },
            {
                "audio",		// kind
                "audio/iLBC",	// mimeType
                0,				// preferredPayloadType
                8000,			// clockRate
                1,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "transport-cc", "" }
                }
            },
            {
                "audio",		// kind
                "audio/SILK",	// mimeType
                0,				// preferredPayloadType
                24000,			// clockRate
                1,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "transport-cc", "" }
                }
            },
            {
                "audio",		// kind
                "audio/SILK",	// mimeType
                0,				// preferredPayloadType
                16000,			// clockRate
                1,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "transport-cc", "" }
                }
            },
            {
                "audio",		// kind
                "audio/SILK",	// mimeType
                0,				// preferredPayloadType
                12000,			// clockRate
                1,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "transport-cc", "" }
                }
            },
            {
                "audio",		// kind
                "audio/SILK",	// mimeType
                0,				// preferredPayloadType
                8000,			// clockRate
                1,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "transport-cc", "" }
                }
            },
            {
                "audio",		// kind
                "audio/CN",	    // mimeType
                13,				// preferredPayloadType
                32000,			// clockRate
                1,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "", "" }
                }
            },
            {
                "audio",		// kind
                "audio/CN",	    // mimeType
                13,				// preferredPayloadType
                16000,			// clockRate
                1,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "", "" }
                }
            },
            {
                "audio",		// kind
                "audio/CN",	    // mimeType
                13,				// preferredPayloadType
                8000,			// clockRate
                1,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "", "" }
                }
            },
            {
                "audio",                        // kind
                "audio/telephone-event",	    // mimeType
                0,                              // preferredPayloadType
                48000,                          // clockRate
                1,                              // channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "", "" }
                }
            },
            {
                "audio",                        // kind
                "audio/telephone-event",	    // mimeType
                0,                              // preferredPayloadType
                32000,                          // clockRate
                1,                              // channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "", "" }
                }
            },
            {
                "audio",                        // kind
                "audio/telephone-event",	    // mimeType
                0,                              // preferredPayloadType
                16000,                          // clockRate
                1,                              // channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "", "" }
                }
            },
            {
                "audio",                        // kind
                "audio/telephone-event",	    // mimeType
                0,                              // preferredPayloadType
                8000,                          // clockRate
                1,                              // channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "", "" }
                }
            },
            {
                "video",		// kind
                "video/VP8",	// mimeType
                0,				// preferredPayloadType
                90000,			// clockRate
                1,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "nack", "" },
                    { "nack", "pli"	},
                    { "ccm",  "fir" },
                    { "goog-remb", "" },
                    { "transport-cc", "" }
                }
            },
            {
                "video",		// kind
                "video/VP9",	// mimeType
                0,				// preferredPayloadType
                90000,			// clockRate
                1,				// channels
                   // parameters
                {},
                    // rtcpFeedback
                {
                    { "nack", "" },
                    { "nack", "pli"	},
                    { "ccm",  "fir" },
                    { "goog-remb", "" },
                    { "transport-cc", "" }
                }
            },
            {
                "video",		// kind
                "video/H264",	// mimeType
                0,				// preferredPayloadType
                90000,			// clockRate
                0,				// channels
                   // parameters
                {
                    {"level-asymmetry-allowed", 1}
                },
                   // rtcpFeedback
                {
                    { "nack", "" },
                    { "nack", "pli"	},
                    { "ccm",  "fir" },
                    { "goog-remb", "" },
                    { "transport-cc", "" }
                }
            },
            {
                "video",            // kind
                "video/H264-SVC",	// mimeType
                0,                  // preferredPayloadType
                90000,              // clockRate
                0,                  // channels
                   // parameters
                {
                    {"level-asymmetry-allowed", 1}
                },
                   // rtcpFeedback
                {
                    { "nack", "" },
                    { "nack", "pli"	},
                    { "ccm",  "fir" },
                    { "goog-remb", "" },
                    { "transport-cc", "" }
                }
            },
            {
                "video",		// kind
                "video/H265",	// mimeType
                0,				// preferredPayloadType
                90000,			// clockRate
                0,				// channels
                   // parameters
                {
                    {"level-asymmetry-allowed", 1}
                },
                   // rtcpFeedback
                {
                    { "nack", "" },
                    { "nack", "pli"	},
                    { "ccm",  "fir" },
                    { "goog-remb", "" },
                    { "transport-cc", "" }
                }
            }
        },
           //headerExtensions
        {
            {
                "audio",            // kind
                "urn:ietf:params:rtp-hdrext:sdes:mid", // uri
                1,                  // preferredId
                false,              // preferredEncrypt
                "sendrecv"          // direction
            },
            {
                "video",            // kind
                "urn:ietf:params:rtp-hdrext:sdes:mid", // uri
                1,                  // preferredId
                false,              // preferredEncrypt
                "sendrecv"          // direction
            },
            {
                "video",            // kind
                "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id", // uri
                2,                  // preferredId
                false,              // preferredEncrypt
                "recvonly"          // direction
            },
            {
                "video",            // kind
                "urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id", // uri
                3,                  // preferredId
                false,              // preferredEncrypt
                "recvonly"          // direction
            },
            {
                "audio",            // kind
                "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time", // uri
                4,                  // preferredId
                false,              // preferredEncrypt
                "sendrecv"          // direction
            },
            {
                "video",            // kind
                "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time", // uri
                4,                  // preferredId
                false,              // preferredEncrypt
                "sendrecv"          // direction
            },
               // NOTE: For audio we just enable transport-wide-cc-01 when receiving media.
            {
                "audio",            // kind
                "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01", // uri
                5,                  // preferredId
                false,              // preferredEncrypt
                "recvonly"          // direction
            },
            {
                "video",            // kind
                "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01", // uri
                5,                  // preferredId
                false,              // preferredEncrypt
                "sendrecv"          // direction
            },
               // NOTE: Remove this once framemarking draft becomes RFC.
            {
                "video",            // kind
                "http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07", // uri
                6,                  // preferredId
                false,              // preferredEncrypt
                "sendrecv"          // direction
            },
            {
                "video",            // kind
                "urn:ietf:params:rtp-hdrext:framemarking", // uri
                7,                  // preferredId
                false,              // preferredEncrypt
                "sendrecv"          // direction
            },
            {
                "audio",            // kind
                "urn:ietf:params:rtp-hdrext:ssrc-audio-level", // uri
                10,                  // preferredId
                false,              // preferredEncrypt
                "sendrecv"          // direction
            },
            {
                "video",            // kind
                "urn:3gpp:video-orientation", // uri
                11,                  // preferredId
                false,              // preferredEncrypt
                "sendrecv"          // direction
            },
            {
                "video",            // kind
                "urn:ietf:params:rtp-hdrext:toffset", // uri
                12,                  // preferredId
                false,              // preferredEncrypt
                "sendrecv"          // direction
            },
            {
                "audio",
                "http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time",
                13,
                false,
                "sendrecv"
            },
            {
                "video",
                "http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time",
                13,
                false,
                "sendrecv"
            }
        }
    };

    const RtpCapabilities& getSupportedRtpCapabilities() { return _supportedRtpCapabilities; }
}
