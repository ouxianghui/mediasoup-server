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
#include "webrtc_server_controller.h"
#include "sctp_parameters.h"
#include "rtp_parameters.h"
#include "srv_logger.h"

namespace srv {

    struct WebRtcTransportOptions
    {
        /**
         * Listening IP address or addresses in order of preference (first one is the
         * preferred one). Mandatory unless webRtcServer is given.
         *  TransportListenIp | string
         */
        nlohmann::json listenIps;

        /**
         * Fixed port to listen on instead of selecting automatically from Worker's port
         * range.
         */
        int32_t port = 0;
        
        std::shared_ptr<WebRtcServerController> webRtcServer;
        
        /**
         * Listen in UDP. Default true.
         */
        bool enableUdp = true;

        /**
         * Listen in TCP. Default false.
         */
        bool enableTcp = false;

        /**
         * Prefer UDP. Default false.
         */
        bool preferUdp = false;

        /**
         * Prefer TCP. Default false.
         */
        bool preferTcp = false;

        /**
         * Initial available outgoing bitrate (in bps). Default 600000.
         */
        int32_t initialAvailableOutgoingBitrate = 600000;

        int32_t minimumAvailableOutgoingBitrate = 600000;
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

        int32_t maxIncomingBitrate = 1500000;
        
        /**
         * Maximum SCTP send buffer used by DataConsumers.
         * Default 262144.
         */
        int32_t sctpSendBufferSize = 262144;

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    void to_json(nlohmann::json& j, const WebRtcTransportOptions& st);
    void from_json(const nlohmann::json& j, WebRtcTransportOptions& st);

    struct IceParameters
    {
        std::string usernameFragment;
        std::string password;
        bool iceLite;
    };

    void to_json(nlohmann::json& j, const IceParameters& st);
    void from_json(const nlohmann::json& j, IceParameters& st);

    struct IceCandidate
    {
        std::string foundation;
        int32_t priority;
        std::string ip;
        
        // Options: 'udp' | 'tcp'
        std::string protocol;
        int32_t port;
        std::string type = "host";
        std::string tcpType = "passive";
    };

    void to_json(nlohmann::json& j, const IceCandidate& st);
    void from_json(const nlohmann::json& j, IceCandidate& st);

    /**
     * The hash function algorithm (as defined in the "Hash function Textual Names"
     * registry initially specified in RFC 4572 Section 8) and its corresponding
     * certificate fingerprint value (in lowercase hex string as expressed utilizing
     * the syntax of "fingerprint" in RFC 4572 Section 5).
     */
    struct DtlsFingerprint
    {
        std::string algorithm;
        std::string value;
    };

    void to_json(nlohmann::json& j, const DtlsFingerprint& st);
    void from_json(const nlohmann::json& j, DtlsFingerprint& st);

    struct DtlsParameters
    {
        // DtlsRole, Options: 'auto' | 'client' | 'server'
        std::string role;
        
        std::vector<DtlsFingerprint> fingerprints;
    };

    void to_json(nlohmann::json& j, const DtlsParameters& st);
    void from_json(const nlohmann::json& j, DtlsParameters& st);

    struct WebRtcTransportStat
    {
        // Common to all Transports.
        std::string type;
        std::string transportId;
        int32_t timestamp;
        
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
        
        // WebRtcTransport specific.
        std::string iceRole;
        
        // Options: 'new' | 'connected' | 'completed' | 'disconnected' | 'closed'
        std::string iceState;
        
        TransportTuple iceSelectedTuple;
        
        // Options: 'new' | 'connecting' | 'connected' | 'failed' | 'closed'
        std::string dtlsState;
    };

    void to_json(nlohmann::json& j, const WebRtcTransportStat& st);
    void from_json(const nlohmann::json& j, WebRtcTransportStat& st);

    struct WebRtcTransportConstructorOptions : TransportConstructorOptions {};

    class WebRtcTransportController : public TransportController
    {
    public:
        WebRtcTransportController(const std::shared_ptr<WebRtcTransportConstructorOptions>& options);
        
        ~WebRtcTransportController();
 
        void init();
        
        void destroy();
        
        std::string iceRole() { return _data["iceRole"]; }

        IceParameters iceParameters() { return _data["iceParameters"]; }

        std::vector<IceCandidate> iceCandidates() { return _data["iceCandidates"]; }

        std::string iceState() { return _data["iceState"]; }

        TransportTuple iceSelectedTuple() { return _data["iceSelectedTuple"]; }

        DtlsParameters dtlsParameters() { return _data["dtlsParameters"]; }

        std::string dtlsState() { return _data["dtlsState"]; }

        std::string dtlsRemoteCert() { return _data["dtlsRemoteCert"]; }

        SctpParameters sctpParameters() { return _data["sctpParameters"]; }

        std::string sctpState() { return _data["sctpState"]; }

        void close();
        
        void onListenServerClosed();
        
        void onRouterClosed();
        
        nlohmann::json getStats() override;
        
        void connect(const nlohmann::json& data) override;
        
        IceParameters restartIce();

    public:
        // transport controller id
        sigslot::signal<const std::string&> _iceStateChangeSignal;
        
        sigslot::signal<const TransportTuple&> _iceSelectedTupleChangeSignal;
        
        sigslot::signal<const std::string&> _dtlsStateChangeSignal;
        
        sigslot::signal<const std::string&> _sctpStateChangeSignal;
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, const std::string& event, const std::string& data);
        
        void cleanData();
    };

}
