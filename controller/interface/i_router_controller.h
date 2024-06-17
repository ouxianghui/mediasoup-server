/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <vector>
#include <atomic>
#include <string>
#include <unordered_map>
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "types.h"
#include "rtp_parameters.h"
#include "i_transport_controller.h"
#include "sctp_parameters.h"

namespace srv {

    class IRouterController;

    struct PipeToRouterOptions
    {
        /**
         * Listening info.
         */
        TransportListenInfo listenInfo;
        
        ///**
        // * Listening IP address.
        // */
        //TransportListenIp listenIp;
        
        /**
         * Fixed port to listen on instead of selecting automatically from Worker's port
         * range.
         */
        uint16_t port;
        
        /**
         * The id of the Producer to consume.
         */
        std::string producerId;

        /**
         * The id of the DataProducer to consume.
         */
        std::string dataProducerId;

        /**
         * Target Router instance.
         */
        std::shared_ptr<IRouterController> routerController;

        /**
         * Create a SCTP association. Default true.
         */
        bool enableSctp;

        /**
         * SCTP streams number.
         */
        NumSctpStreams numSctpStreams;

        /**
         * Enable RTX and NACK for RTP retransmission.
         */
        bool enableRtx;

        /**
         * Enable SRTP.
         */
        bool enableSrtp;
    };

    struct PipeToRouterResult
    {
        /**
         * The Consumer created in the current Router.
         */
        std::shared_ptr<IConsumerController> pipeConsumerController;

        /**
         * The Producer created in the target Router.
         */
        std::shared_ptr<IProducerController> pipeProducerController;

        /**
         * The DataConsumer created in the current Router.
         */
        std::shared_ptr<IDataConsumerController> pipeDataConsumerController;

        /**
         * The DataProducer created in the target Router.
         */
        std::shared_ptr<IDataProducerController> pipeDataProducerController;
    };

    struct RouterDump
    {
        /**
         * The Router id.
         */
        std::string id;
        /**
         * Id of Transports.
         */
        std::vector<std::string> transportIds;
        /**
         * Id of RtpObservers.
         */
        std::vector<std::string> rtpObserverIds;
        /**
         * Array of Producer id and its respective Consumer ids.
         */
        std::vector<std::pair<std::string, std::vector<std::string>>> mapProducerIdConsumerIds;
        /**
         * Array of Consumer id and its Producer id.
         */
        std::vector<std::pair<std::string, std::string>> mapConsumerIdProducerId;
        /**
         * Array of Producer id and its respective Observer ids.
         */
        std::vector<std::pair<std::string, std::vector<std::string>>> mapProducerIdObserverIds;
        /**
         * Array of Producer id and its respective DataConsumer ids.
         */
        std::vector<std::pair<std::string, std::vector<std::string>>> mapDataProducerIdDataConsumerIds;
        /**
         * Array of DataConsumer id and its DataProducer id.
         */
        std::vector<std::pair<std::string, std::string>> mapDataConsumerIdDataProducerId;
    };

    struct WebRtcTransportOptions;
    struct PlainTransportOptions;
    struct DirectTransportOptions;
    struct PipeTransportOptions;
    struct ActiveSpeakerObserverOptions;
    struct AudioLevelObserverOptions;

    class ITransportController;
    class IRtpObserverController;

    using PipeTransportControllerPair = std::unordered_map<std::string, std::shared_ptr<ITransportController>>;

    class IRouterController {
    public:
        virtual ~IRouterController() = default;
        
        virtual void init() = 0;
        
        virtual void destroy() = 0;
        
        virtual const std::string& id() = 0;
        
        virtual const RtpCapabilities& rtpCapabilities() = 0;
        
        virtual void setAppData(const nlohmann::json& data) = 0;
        
        virtual const nlohmann::json& appData() = 0;
        
        virtual std::shared_ptr<RouterDump> dump() = 0;
        
        virtual void close() = 0;
        
        virtual bool closed() = 0;

        virtual bool canConsume(const std::string& producerId, const RtpCapabilities& rtpCapabilities) = 0;
            
        virtual std::shared_ptr<ITransportController> createWebRtcTransportController(const std::shared_ptr<WebRtcTransportOptions>& options) = 0;
        
        virtual std::shared_ptr<ITransportController> createPlainTransportController(const std::shared_ptr<PlainTransportOptions>& options) = 0;
        
        virtual std::shared_ptr<ITransportController> createDirectTransportController(const std::shared_ptr<DirectTransportOptions>& options) = 0;
        
        virtual std::shared_ptr<ITransportController> createPipeTransportController(const std::shared_ptr<PipeTransportOptions>& options) = 0;
        
        virtual std::shared_ptr<IRtpObserverController> createActiveSpeakerObserverController(const std::shared_ptr<ActiveSpeakerObserverOptions>& options) = 0;
        
        virtual std::shared_ptr<IRtpObserverController> createAudioLevelObserverController(const std::shared_ptr<AudioLevelObserverOptions>& options) = 0;
        
        virtual std::shared_ptr<PipeToRouterResult> pipeToRouter(const std::shared_ptr<PipeToRouterOptions>& options) = 0;
        
        virtual void addPipeTransportPair(const std::string& key, PipeTransportControllerPair& pair) = 0;
        
        virtual void onWorkerClosed() = 0;
        
    public:
        // signals
        sigslot::signal<std::shared_ptr<IRouterController>> closeSignal;
        
        sigslot::signal<> workerCloseSignal;
        
        sigslot::signal<std::shared_ptr<ITransportController>> newTransportSignal;
        
        sigslot::signal<std::shared_ptr<IRtpObserverController>> newRtpObserverSignal;
    };
}
