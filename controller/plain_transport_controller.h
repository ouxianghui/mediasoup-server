/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "types.h"
#include "transport_controller.h"
#include "sctp_parameters.h"
#include "srtp_parameters.h"
#include "rtp_parameters.h"

namespace srv {

    struct PlainTransportOptions
    {
        /**
         * Listening IP address.
         */
        nlohmann::json listenIps;

        /**
         * Fixed port to listen on instead of selecting automatically from Worker's port
         * range.
         */
        int32_t port = 0;

        /**
         * Use RTCP-mux (RTP and RTCP in the same port). Default true.
         */
        bool rtcpMux = true;

        /**
         * Whether remote IP:port should be auto-detected based on first RTP/RTCP
         * packet received. If enabled, connect() method must not be called unless
         * SRTP is enabled. If so, it must be called with just remote SRTP parameters.
         * Default false.
         */
        bool comedia = false;

        /**
         * Create a SCTP association. Default false.
         */
        bool enableSctp = false;

        /**
         * SCTP streams number.
         */
        NumSctpStreams numSctpStreams;

        /**
         * Maximum allowed size for SCTP messages sent by DataProducers.
         * Default 262144.
         */
        int32_t maxSctpMessageSize = 262144;

        /**
         * Maximum SCTP send buffer used by DataConsumers.
         * Default 262144.
         */
        int32_t sctpSendBufferSize = 262144;

        /**
         * Enable SRTP. For this to work, connect() must be called
         * with remote SRTP parameters. Default false.
         */
        bool enableSrtp = false;

        /**
         * The SRTP crypto suite to be used if enableSrtp is set. Default
         * 'AES_CM_128_HMAC_SHA1_80'.
         * Options:  | 'AEAD_AES_256_GCM'
         *        | 'AEAD_AES_128_GCM'
         *        | 'AES_CM_128_HMAC_SHA1_80'
         *        | 'AES_CM_128_HMAC_SHA1_32';
         */
        std::string srtpCryptoSuite = "AES_CM_128_HMAC_SHA1_80";

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    void to_json(nlohmann::json& j, const PlainTransportOptions& st);
    void from_json(const nlohmann::json& j, PlainTransportOptions& st);

    struct PlainTransportStat
    {
        // Common to all Transports.
        std::string type;
        std::string transportId;
        uint64_t timestamp;
        
        // Options: 'new' | 'connecting' | 'connected' | 'failed' | 'closed';
        std::string sctpState;
        
        int32_t bytesReceived;
        int32_t recvBitrate;
        int32_t bytesSent;
        int32_t sendBitrate;
        int32_t rtpBytesReceived;
        int32_t rtpRecvBitrate;
        int32_t rtpBytesSent;
        int32_t rtpSendBitrate;
        int32_t rtxBytesReceived;
        int32_t rtxRecvBitrate;
        int32_t rtxBytesSent;
        int32_t rtxSendBitrate;
        int32_t probationBytesSent;
        int32_t probationSendBitrate;
        int32_t availableOutgoingBitrate;
        int32_t availableIncomingBitrate;
        int32_t maxIncomingBitrate;
        
        // PlainTransport specific.
        bool rtcpMux;
        bool comedia;
        TransportTuple tuple;
        TransportTuple rtcpTuple;
    };

    void to_json(nlohmann::json& j, const PlainTransportStat& st);
    void from_json(const nlohmann::json& j, PlainTransportStat& st);

    struct PlainTransportConstructorOptions : TransportConstructorOptions {};

    class PlainTransportController : public TransportController
    {
    public:
        PlainTransportController(const std::shared_ptr<PlainTransportConstructorOptions>& options);
        
        ~PlainTransportController();
        
        void init();
        
        void destroy();
        
        TransportTuple tuple() { return _data["tuple"]; }

        TransportTuple rtcpTuple() { return _data["rtcpTuple"]; }

        SctpParameters sctpParameters() { return _data["sctpParameters"]; }

        std::string sctpState() { return _data["sctpState"]; }

        SrtpParameters srtpParameters() { return _data["srtpParameters"]; }

        void close();
        
        void onRouterClosed();
        
        nlohmann::json getStats() override;
        
        void connect(const nlohmann::json& data) override;
        
    public:
        sigslot::signal<const TransportTuple&> tupleSignal;
        
        sigslot::signal<const TransportTuple&> rtcpTupleSignal;
        
        sigslot::signal<const std::string&> sctpStateChangeSignal;
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, const std::string& event, const std::string& data);
    };

}
