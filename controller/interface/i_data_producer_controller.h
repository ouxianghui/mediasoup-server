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
#include "sctp_parameters.h"

namespace srv {

    struct DataProducerOptions
    {
        /**
        * DataProducer id (just for Router.pipeToRouter() method).
        */
        std::string id;

        /**
        * SCTP parameters defining how the endpoint is sending the data.
        * Just if messages are sent over SCTP.
        */
        SctpStreamParameters sctpStreamParameters;

        /**
        * A label which can be used to distinguish this DataChannel from others.
        */
        std::string label;

        /**
        * Name of the sub-protocol used by this DataChannel.
        */
        std::string protocol;

        /**
        * Whether the data producer must start in paused mode. Default false.
        */
        bool paused = false;

        /**
        * Custom application data.
        */
        nlohmann::json appData;
    };

    struct DataProducerStat
    {
        std::string type;
        uint64_t timestamp;
        std::string label;
        std::string protocol;
        uint64_t messagesReceived;
        uint64_t bytesReceived;
    };

    void to_json(nlohmann::json& j, const DataProducerStat& st);
    void from_json(const nlohmann::json& j, DataProducerStat& st);

    struct DataProducerInternal
    {
        std::string transportId;
        std::string dataProducerId;
    };

    struct DataProducerData
    {
        // Options: 'sctp' | 'direct'
        std::string type;
        SctpStreamParameters sctpStreamParameters;
        std::string label;
        std::string protocol;
    };

    struct DataProducerDump : DataProducerData
    {
        std::string id;
        bool paused;
    };

    class IDataProducerController
    {
    public:
        virtual ~IDataProducerController() = default;
        
        virtual void init() = 0;
        
        virtual void destroy() = 0;
        
        virtual const std::string& id() = 0;

        virtual const std::string& type() = 0;

        virtual const SctpStreamParameters& sctpStreamParameters() = 0;

        virtual const std::string& label() = 0;

        virtual const std::string& protocol() = 0;

        virtual void setAppData(const nlohmann::json& data) = 0;
        
        virtual const nlohmann::json& appData() = 0;
        
        virtual void pause() = 0;
        
        virtual void resume() = 0;
        
        virtual bool paused() = 0;

        virtual void close() = 0;

        virtual bool closed() = 0;
        
        virtual std::shared_ptr<DataProducerDump> dump() = 0;
        
        virtual std::vector<std::shared_ptr<DataProducerStat>> getStats() = 0;

        virtual void send(const std::vector<uint8_t>& data, const std::vector<uint16_t>& subchannels, uint16_t requiredSubchannel, bool isBinary = false) = 0;
        
        virtual void onTransportClosed() = 0;
          
    public:
        sigslot::signal<> transportCloseSignal;
        
        sigslot::signal<> closeSignal;
        
        sigslot::signal<> pauseSignal;
        
        sigslot::signal<> resumeSignal;
    };
}
