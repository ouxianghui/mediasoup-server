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
#include "FBS/webRtcTransport.h"

namespace srv {

    struct WebRtcTransportListenIndividualListenInfo
    {
        /**
         * Listening info.
         */
        std::vector<TransportListenInfo> listenInfos;
    };

    struct WebRtcTransportListenIndividualListenIp
    {
        /**
         * Listening IP address or addresses in order of preference (first one is the
         * preferred one).
         */
        std::vector<std::string> listenIps;

        /**
         * Fixed port to listen on instead of selecting automatically from Worker's port
         * range.
         */
        uint16_t port;
    };

    class WebRtcServerController;

    struct WebRtcTransportListenServer
    {
        /**
         * Instance of WebRtcServer.
         */
        std::shared_ptr<WebRtcServerController> webRtcServer;
    };

    struct WebRtcTransportListen
    {
        std::shared_ptr<WebRtcTransportListenIndividualListenInfo> webRtcTransportListenIndividualListenInfo;
        std::shared_ptr<WebRtcTransportListenIndividualListenIp> webRtcTransportListenIndividualListenIp;
        std::shared_ptr<WebRtcTransportListenServer> webRtcTransportListenServer;
    };

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
         * Listen in TCP. Default true if webrtcServer is given, false otherwise.
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

    struct IceParameters
    {
        std::string usernameFragment;
        std::string password;
        bool iceLite;
    };

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

    struct WebRtcTransportStat : BaseTransportStats
    {
        std::string type;
        
        std::string iceRole;
        
        // Options: 'new' | 'connected' | 'completed' | 'disconnected' | 'closed'
        std::string iceState;
        
        TransportTuple iceSelectedTuple;
        
        // Options: 'new' | 'connecting' | 'connected' | 'failed' | 'closed'
        std::string dtlsState;
    };

    struct WebRtcTransportDump : BaseTransportDump
    {
        std::string iceRole = "controlled";
        IceParameters iceParameters;
        std::vector<IceCandidate> iceCandidates;
        std::string iceState;
        TransportTuple iceSelectedTuple;
        DtlsParameters dtlsParameters;
        std::string dtlsState;
        std::string dtlsRemoteCert;
    };

    struct WebRtcTransportData : TransportData
    {
        std::string iceRole = "controlled";
        IceParameters iceParameters;
        std::vector<IceCandidate> iceCandidates;
        std::string iceState;
        TransportTuple iceSelectedTuple;
        DtlsParameters dtlsParameters;
        std::string dtlsState;
        std::string dtlsRemoteCert;
        
        //SctpParameters sctpParameters;
        std::string sctpState;
    };

    struct WebRtcTransportConstructorOptions : TransportConstructorOptions {};

    class WebRtcTransportController : public TransportController
    {
    public:
        WebRtcTransportController(const std::shared_ptr<WebRtcTransportConstructorOptions>& options);
        
        ~WebRtcTransportController();
 
        void init();
        
        void destroy();
        
        std::string iceRole() { return transportData()->iceRole; }

        IceParameters iceParameters() { return transportData()->iceParameters; }

        std::vector<IceCandidate> iceCandidates() { return transportData()->iceCandidates; }

        std::string iceState() { return transportData()->iceState; }

        TransportTuple iceSelectedTuple() { return transportData()->iceSelectedTuple; }

        DtlsParameters dtlsParameters() { return transportData()->dtlsParameters; }

        std::string dtlsState() { return transportData()->dtlsState; }

        std::string dtlsRemoteCert() { return transportData()->dtlsRemoteCert; }

        SctpParameters sctpParameters() { return transportData()->sctpParameters; }

        std::string sctpState() { return transportData()->sctpState; }

        void close();
        
        void onListenServerClosed();
        
        void onRouterClosed();
        
        std::shared_ptr<BaseTransportDump> dump() override;
        
        std::shared_ptr<BaseTransportStats> getStats() override;
        
        void connect(const std::shared_ptr<ConnectData>& data) override;
        
        std::shared_ptr<IceParameters> restartIce();

    public:
        // transport controller id
        sigslot::signal<const std::string&> iceStateChangeSignal;
        
        sigslot::signal<const TransportTuple&> iceSelectedTupleChangeSignal;
        
        sigslot::signal<const std::string&> dtlsStateChangeSignal;
        
        sigslot::signal<const std::string&> sctpStateChangeSignal;
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data);
        
        void cleanData();
        
        std::shared_ptr<WebRtcTransportData> transportData() { return std::dynamic_pointer_cast<WebRtcTransportData>(this->_data); }
    };

    std::string iceStateFromFbs(FBS::WebRtcTransport::IceState iceState);

    std::string iceRoleFromFbs(FBS::WebRtcTransport::IceRole role);

    std::string iceCandidateTypeFromFbs(FBS::WebRtcTransport::IceCandidateType type);

    std::string iceCandidateTcpTypeFromFbs(FBS::WebRtcTransport::IceCandidateTcpType type);

    std::string dtlsStateFromFbs(FBS::WebRtcTransport::DtlsState fbsDtlsState);

    std::string dtlsRoleFromFbs(FBS::WebRtcTransport::DtlsRole role);

    std::string fingerprintAlgorithmsFromFbs(FBS::WebRtcTransport::FingerprintAlgorithm algorithm);

    FBS::WebRtcTransport::FingerprintAlgorithm fingerprintAlgorithmToFbs(const std::string& algorithm);

    FBS::WebRtcTransport::DtlsRole dtlsRoleToFbs(const std::string& role);

    std::shared_ptr<WebRtcTransportDump> parseWebRtcTransportDumpResponse(const FBS::WebRtcTransport::DumpResponse* binary);

    flatbuffers::Offset<FBS::WebRtcTransport::ConnectRequest> createConnectRequest(flatbuffers::FlatBufferBuilder& builder, const DtlsParameters& dtlsParameters);

    std::shared_ptr<WebRtcTransportStat> parseGetStatsResponse(const FBS::WebRtcTransport::GetStatsResponse* binary);

    std::shared_ptr<IceCandidate> parseIceCandidate(const FBS::WebRtcTransport::IceCandidate* binary);

    std::shared_ptr<IceParameters> parseIceParameters(const FBS::WebRtcTransport::IceParameters* binary);

    std::shared_ptr<DtlsParameters> parseDtlsParameters(const FBS::WebRtcTransport::DtlsParameters* binary);

    flatbuffers::Offset<FBS::WebRtcTransport::DtlsParameters> serializeDtlsParameters(flatbuffers::FlatBufferBuilder& builder, const DtlsParameters& dtlsParameters);

}
