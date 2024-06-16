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
#include "abstract_transport_controller.h"
#include "sctp_parameters.h"
#include "srtp_parameters.h"
#include "rtp_parameters.h"
#include "FBS/notification.h"

namespace srv {

    struct PlainTransportOptions
    {
        /**
         * Listening info.
         */
        TransportListenInfo listenInfo;

        /**
         * Optional listening info for RTCP.
         */
        TransportListenInfo rtcpListenInfo;
        
        /**
         * Fixed port to listen on instead of selecting automatically from Worker's port
         * range.
         */
        uint16_t port;

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

    struct PlainTransportDump : BaseTransportDump
    {
        bool rtcpMux;
        bool comedia;
        TransportTuple tuple;
        TransportTuple rtcpTuple;
        SrtpParameters srtpParameters;
    };

    struct PlainTransportStat : BaseTransportStats
    {
        std::string type;
        bool rtcpMux;
        bool comedia;
        TransportTuple tuple;
        TransportTuple rtcpTuple;
    };

    struct PlainTransportData : public TransportData
    {
        bool rtcpMux;
        bool comedia;
        TransportTuple tuple;
        TransportTuple rtcpTuple;
        
        //SctpParameters sctpParameters;
        std::string sctpState;
        SrtpParameters srtpParameters;
    };

    struct PlainTransportConstructorOptions : TransportConstructorOptions {};

    class PlainTransportController : public AbstractTransportController
    {
    public:
        PlainTransportController(const std::shared_ptr<PlainTransportConstructorOptions>& options);
        
        ~PlainTransportController();
        
        void init() override;
        
        void destroy() override;
        
        TransportTuple tuple() { return transportData()->tuple; }

        TransportTuple rtcpTuple() { return transportData()->rtcpTuple; }

        SctpParameters sctpParameters() { return transportData()->sctpParameters; }

        std::string sctpState() { return transportData()->sctpState; }

        SrtpParameters srtpParameters() { return transportData()->srtpParameters; }

        void close() override;
        
        void onRouterClosed() override;
        
        std::shared_ptr<BaseTransportDump> dump() override;
        
        std::shared_ptr<BaseTransportStats> getStats() override;
        
        void connect(const std::shared_ptr<ConnectParams>& params) override;
        
    public:
        sigslot::signal<const TransportTuple&> tupleSignal;
        
        sigslot::signal<const TransportTuple&> rtcpTupleSignal;
        
        sigslot::signal<const std::string&> sctpStateChangeSignal;
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data);
        
        std::shared_ptr<PlainTransportData> transportData() { return std::dynamic_pointer_cast<PlainTransportData>(this->_data); }
    };

    std::shared_ptr<PlainTransportDump> parsePlainTransportDumpResponse(const FBS::PlainTransport::DumpResponse* binary);

    std::shared_ptr<PlainTransportStat> parseGetStatsResponse(const FBS::PlainTransport::GetStatsResponse* binary);

    flatbuffers::Offset<FBS::PlainTransport::ConnectRequest> createConnectRequest(flatbuffers::FlatBufferBuilder& builder,
                                                                                  const std::string& ip,
                                                                                  uint16_t port,
                                                                                  uint16_t rtcpPort,
                                                                                  const SrtpParameters& srtpParameters);

}
