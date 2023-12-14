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
    
    struct DataConsumerOptions
    {
        /**
         * The id of the DataProducer to consume.
         */
        std::string dataProducerId;

        /**
         * Just if consuming over SCTP.
         * Whether data messages must be received in order. If true the messages will
         * be sent reliably. Defaults to the value in the DataProducer if it has type
         * 'sctp' or to true if it has type 'direct'.
         */
        bool ordered;

        /**
         * Just if consuming over SCTP.
         * When ordered is false indicates the time (in milliseconds) after which a
         * SCTP packet will stop being retransmitted. Defaults to the value in the
         * DataProducer if it has type 'sctp' or unset if it has type 'direct'.
         */
        int64_t maxPacketLifeTime;

        /**
         * Just if consuming over SCTP.
         * When ordered is false indicates the maximum number of times a packet will
         * be retransmitted. Defaults to the value in the DataProducer if it has type
         * 'sctp' or unset if it has type 'direct'.
         */
        int32_t maxRetransmits;
        
        /**
         * Whether the data consumer must start in paused mode. Default false.
         */
        bool paused = false;

        /**
         * Subchannels this data consumer initially subscribes to.
         * Only used in case this data consumer receives messages from a local data
         * producer that specifies subchannel(s) when calling send().
         */
        std::vector<uint16_t> subchannels;

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    struct DataConsumerStat
    {
        std::string type;
        uint64_t timestamp;
        std::string label;
        std::string protocol;
        int64_t messagesSent;
        int64_t bytesSent;
        int32_t bufferedAmount;
    };

    void to_json(nlohmann::json& j, const DataConsumerStat& st);
    void from_json(const nlohmann::json& j, DataConsumerStat& st);

    struct DataConsumerInternal
    {
        std::string transportId;
        std::string dataConsumerId;
    };

    struct DataConsumerData
    {
        std::string dataProducerId;
        
        // Options: 'sctp' | 'direct'
        std::string type;
        
        SctpStreamParameters sctpStreamParameters;
        
        std::string label;
        
        std::string protocol;
        
        uint32_t bufferedAmountLowThreshold;
    };

    struct DataConsumerDump : DataConsumerData
    {
        std::string id;
        bool paused;
        bool dataProducerPaused;
        std::vector<uint16_t> subchannels;
    };

    class IDataConsumerController
    {
    public:

        virtual ~IDataConsumerController() = default;
        
        virtual void init() = 0;
        
        virtual void destroy() = 0;
        
        virtual const std::string& id() = 0;

        virtual const std::string& dataProducerId() = 0;

        virtual const std::string& type() = 0;

        virtual const SctpStreamParameters& sctpStreamParameters() = 0;
        
        virtual const std::string& label() = 0;

        virtual const std::string& protocol() = 0 ;
        
        virtual const std::vector<uint16_t>& subchannels() = 0;

        virtual void setAppData(const nlohmann::json& data) = 0;
        
        virtual const nlohmann::json& appData() = 0 ;
        
        virtual void close() = 0;

        virtual bool closed() = 0;
        
        virtual void onTransportClosed() = 0;
        
        virtual std::shared_ptr<DataConsumerDump> dump() = 0;
        
        virtual std::vector<std::shared_ptr<DataConsumerStat>> getStats() = 0;
        
        virtual void addSubchannel(uint16_t subchannel) = 0;
        
        virtual void removeSubchannel(uint16_t subchannel) = 0;
        
        virtual void pause() = 0;
        
        virtual void resume() = 0;

        virtual bool paused() = 0;
        
        virtual bool dataProducerPaused() = 0;
        
        virtual void setBufferedAmountLowThreshold(uint32_t threshold) = 0;
        
        virtual void setSubchannels(const std::vector<uint16_t>& subchannels) = 0;

        virtual void send(const std::vector<uint8_t>& data, bool isBinary = false) = 0;
        
        virtual uint32_t getBufferedAmount() = 0;
        
    public:
        sigslot::signal<> transportCloseSignal;
        
        sigslot::signal<> dataProducerCloseSignal;
        
        sigslot::signal<> dataProducerPauseSignal;
        
        sigslot::signal<> dataProducerResumeSignal;
        
        // data, ppid
        sigslot::signal<const std::vector<uint8_t>&, int32_t> messageSignal;
        
        sigslot::signal<> sctpSendBufferFullSignal;
        
        sigslot::signal<int32_t> bufferedAmountLowSignal;
        
        sigslot::signal<> closeSignal;
        
        sigslot::signal<> pauseSignal;
        
        sigslot::signal<> resumeSignal;
    };
}
