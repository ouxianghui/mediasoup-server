/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "data_producer_controller.h"
#include "srv_logger.h"
#include "channel.h"
#include "message_builder.h"

namespace srv {

    DataProducerController::DataProducerController(const DataProducerInternal& internal,
                                                   const DataProducerData& data,
                                                   const std::shared_ptr<Channel>& channel,
                                                   bool paused,
                                                   const nlohmann::json& appData)
    : _internal(internal)
    , _data(data)
    , _channel(channel)
    , _paused(paused)
    , _appData(appData)
    {
        SRV_LOGD("DataProducerController()");
    }

    DataProducerController::~DataProducerController()
    {
        SRV_LOGD("~DataProducerController()");
    }

    void DataProducerController::init()
    {
        SRV_LOGD("init()");
        handleWorkerNotifications();
    }

    void DataProducerController::destroy()
    {
        SRV_LOGD("destroy()");
    }

    void DataProducerController::pause()
    {
        SRV_LOGD("pause()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.dataProducerId, FBS::Request::Method::DATAPRODUCER_PAUSE);
        
        channel->request(reqId, reqData);
        
        bool wasPaused = _paused;

        _paused = true;

        // Emit observer event.
        if (!wasPaused) {
            this->pauseSignal();
        }
    }

    void DataProducerController::resume()
    {
        SRV_LOGD("resume()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.dataProducerId, FBS::Request::Method::DATAPRODUCER_RESUME);
        
        channel->request(reqId, reqData);
        
        bool wasPaused = _paused;

        _paused = false;

        // Emit observer event.
        if (wasPaused) {
            this->resumeSignal();
        }
    }

    void DataProducerController::close()
    {
        if (_closed) {
            return;
        }
        
        SRV_LOGD("close()");

        _closed = true;

        // Remove notification subscriptions.
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        channel->notificationSignal.disconnect(shared_from_this());
     
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqOffset = FBS::Transport::CreateCloseDataProducerRequestDirect(builder, _internal.dataProducerId.c_str());
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::TRANSPORT_CLOSE_DATAPRODUCER,
                                                     FBS::Request::Body::Transport_CloseDataProducerRequest,
                                                     reqOffset);
        
        channel->request(reqId, reqData);
        
        this->closeSignal();
    }

    void DataProducerController::onTransportClosed()
    {
        if (_closed) {
            return;
        }
        
        SRV_LOGD("onTransportClosed()");
        
        _closed = true;

        // Remove notification subscriptions.
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        channel->notificationSignal.disconnect(shared_from_this());
        
        this->transportCloseSignal();
        
        this->closeSignal();
    }

    std::shared_ptr<DataProducerDump> DataProducerController::dump()
    {
        SRV_LOGD("dump()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nullptr;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.dataProducerId, FBS::Request::Method::DATAPRODUCER_DUMP);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        return parseDataProducerDumpResponse(response->body_as_DataProducer_DumpResponse());
    }

    std::vector<std::shared_ptr<DataProducerStat>> DataProducerController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return {};
        }
        
        //return channel->request("dataProducer.getStats", _internal.dataProducerId, "{}");
        
        std::vector<std::shared_ptr<DataProducerStat>> result;
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.dataProducerId, FBS::Request::Method::DATAPRODUCER_GET_STATS);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        return std::vector<std::shared_ptr<DataProducerStat>> { parseDataProducerStats(response->body_as_DataProducer_GetStatsResponse()) };
    }

    void DataProducerController::send(const std::vector<uint8_t>& data, const std::vector<uint16_t>& subchannels, uint16_t requiredSubchannel, bool isBinary)
    {
        if (data.size() <= 0) {
            SRV_LOGD("message must be a string or a Buffer");
            return;
        }

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        /*
         * +-------------------------------+----------+
         * | Value                         | SCTP     |
         * |                               | PPID     |
         * +-------------------------------+----------+
         * | WebRTC String                 | 51       |
         * | WebRTC Binary Partial         | 52       |
         * | (Deprecated)                  |          |
         * | WebRTC Binary                 | 53       |
         * | WebRTC String Partial         | 54       |
         * | (Deprecated)                  |          |
         * | WebRTC String Empty           | 56       |
         * | WebRTC Binary Empty           | 57       |
         * +-------------------------------+----------+
         */

        uint32_t ppid = !isBinary ? (data.size() > 0 ? 51 : 56) : (data.size() > 0 ? 53 : 57);

        flatbuffers::FlatBufferBuilder builder;
        
        auto nfOffset = FBS::DataProducer::CreateSendNotificationDirect(builder, ppid, &data, &subchannels, requiredSubchannel);

        auto nfData = MessageBuilder::createNotification(builder,
                                                       _internal.dataProducerId,
                                                       FBS::Notification::Event::DATAPRODUCER_SEND,
                                                       FBS::Notification::Body::DataProducer_SendNotification,
                                                       nfOffset);
        
        channel->notify(nfData);
    }

    void DataProducerController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
    }
}

namespace srv {

    FBS::DataProducer::Type dataProducerTypeToFbs(const std::string& type)
    {
        if ("sctp" == type) {
            return FBS::DataProducer::Type::SCTP;
        }
        else if ("direct" == type) {
            return FBS::DataProducer::Type::DIRECT;
        }

        SRV_LOGE("invalid DataConsumerType: %s", type.c_str());
        return  FBS::DataProducer::Type::MIN;
    }

    std::string dataProducerTypeFromFbs(FBS::DataProducer::Type type)
    {
        switch (type)
        {
            case FBS::DataProducer::Type::SCTP:
                return "sctp";
            case FBS::DataProducer::Type::DIRECT:
                return "direct";
            default:
                SRV_LOGE("invalid DataConsumerType: %u", (uint8_t)type);
                return "";
        }
    }

    std::shared_ptr<DataProducerDump> parseDataProducerDumpResponse(const FBS::DataProducer::DumpResponse* data)
    {
        auto dump = std::make_shared<DataProducerDump>();
        
        dump->type = dataProducerTypeFromFbs(data->type());
        if (auto params = data->sctpStreamParameters()) {
            dump->sctpStreamParameters = *parseSctpStreamParameters(params);
        }
        dump->label = data->label()->str();
        dump->protocol = data->protocol()->str();
        dump->id = data->id()->str();
        dump->paused = data->paused();
        
        return dump;
    }

    std::shared_ptr<DataProducerStat> parseDataProducerStats(const FBS::DataProducer::GetStatsResponse* binary)
    {
        auto stat = std::make_shared<DataProducerStat>();
        
        stat->type = "data-producer";
        stat->timestamp = binary->timestamp();
        stat->label = binary->label()->str();
        stat->protocol = binary->protocol()->str();
        stat->messagesReceived = binary->messagesReceived();
        stat->bytesReceived = binary->bytesReceived();
        
        return stat;
    }

    void to_json(nlohmann::json& j, const DataProducerStat& st)
     {
         j["type"] = st.type;
         j["timestamp"] = st.timestamp;
         j["label"] = st.label;
         j["protocol"] = st.protocol;
         j["messagesReceived"] = st.messagesReceived;
         j["bytesReceived"] = st.bytesReceived;
     }

     void from_json(const nlohmann::json& j, DataProducerStat& st)
     {
         if (j.contains("type")) {
             j.at("type").get_to(st.type);
         }
         if (j.contains("timestamp")) {
             j.at("timestamp").get_to(st.timestamp);
         }
         if (j.contains("label")) {
             j.at("label").get_to(st.label);
         }
         if (j.contains("protocol")) {
             j.at("protocol").get_to(st.protocol);
         }
         if (j.contains("messagesReceived")) {
             j.at("messagesReceived").get_to(st.messagesReceived);
         }
         if (j.contains("bytesReceived")) {
             j.at("bytesReceived").get_to(st.bytesReceived);
         }
     }

}
