/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "data_consumer_controller.h"
#include "srv_logger.h"
#include "channel.h"
#include "FBS/transport.h"
#include "message_builder.h"

namespace srv {

    DataConsumerController::DataConsumerController(const DataConsumerInternal& internal,
                                                   const DataConsumerData& data,
                                                   const std::shared_ptr<Channel>& channel,
                                                   bool paused,
                                                   bool dataProducerPaused,
                                                   const std::vector<uint16_t>& subchannels,
                                                   const nlohmann::json& appData)
    : _internal(internal)
    , _data(data)
    , _channel(channel)
    , _paused(paused)
    , _dataProducerPaused(dataProducerPaused)
    , _subchannels(subchannels)
    , _appData(appData)
    {
        SRV_LOGD("DataConsumerController()");
    }

    DataConsumerController::~DataConsumerController()
    {
        SRV_LOGD("~DataConsumerController()");
    }

    void DataConsumerController::init()
    {
        SRV_LOGD("init()");
        handleWorkerNotifications();
    }

    void DataConsumerController::destroy()
    {
        SRV_LOGD("destroy()");
    }

    void DataConsumerController::close()
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
        
        auto reqOffset = FBS::Transport::CreateCloseDataConsumerRequestDirect(builder, _internal.dataConsumerId.c_str());
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.transportId,
                                                     FBS::Request::Method::TRANSPORT_CLOSE_DATACONSUMER,
                                                     FBS::Request::Body::Transport_CloseDataConsumerRequest,
                                                     reqOffset);
        
        channel->request(reqId, reqData);
        
        this->closeSignal();
    }

    void DataConsumerController::onTransportClosed()
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

    std::shared_ptr<DataConsumerDump> DataConsumerController::dump()
    {
        SRV_LOGD("dump()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nullptr;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.dataConsumerId, FBS::Request::Method::DATACONSUMER_DUMP);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto dumpFbs = response->body_as_DataConsumer_DumpResponse();
        
        return parseDataConsumerDumpResponse(dumpFbs);
        
    }

    std::vector<std::shared_ptr<DataConsumerStat>> DataConsumerController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return {};
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.dataConsumerId, FBS::Request::Method::DATACONSUMER_GET_STATS);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto statsFbs = response->body_as_DataConsumer_GetStatsResponse();

        return std::vector<std::shared_ptr<DataConsumerStat>> { parseDataConsumerStats(statsFbs) };
    }

    void DataConsumerController::addSubchannel(uint16_t subchannel)
    {
        SRV_LOGD("addSubchannel()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.dataConsumerId, FBS::Request::Method::DATACONSUMER_ADD_SUBCHANNEL);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto response_ = response->body_as_DataConsumer_AddSubchannelResponse();
        
        std::vector<uint16_t> subchannels_;
        
        for (const auto& channel : *response_->subchannels()) {
            subchannels_.emplace_back(channel);
        }

        this->_subchannels = subchannels_;
    }

    void DataConsumerController::removeSubchannel(uint16_t subchannel)
    {
        SRV_LOGD("removeSubchannel()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.dataConsumerId, FBS::Request::Method::DATACONSUMER_REMOVE_SUBCHANNEL);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto response_ = response->body_as_DataConsumer_RemoveSubchannelResponse();

        std::vector<uint16_t> subchannels_;
        
        for (const auto& channel : *response_->subchannels()) {
            subchannels_.emplace_back(channel);
        }

        this->_subchannels = subchannels_;
    }

    void DataConsumerController::pause()
    {
        SRV_LOGD("pause()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.dataConsumerId, FBS::Request::Method::DATACONSUMER_PAUSE);
        
        channel->request(reqId, reqData);
        
        bool wasPaused = _paused;

        _paused = true;

        // Emit observer event.
        if (!wasPaused && !_dataProducerPaused) {
            this->pauseSignal();
        }
    }

    void DataConsumerController::resume()
    {
        SRV_LOGD("resume()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.dataConsumerId, FBS::Request::Method::DATACONSUMER_RESUME);
        
        channel->request(reqId, reqData);
        
        bool wasPaused = _paused;

        _paused = false;

        // Emit observer event.
        if (wasPaused && !_dataProducerPaused) {
            this->resumeSignal();
        }
    }

    void DataConsumerController::setBufferedAmountLowThreshold(uint32_t threshold)
    {
        SRV_LOGD("setBufferedAmountLowThreshold() [threshold:%d]", threshold);

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqOffset = FBS::DataConsumer::CreateSetBufferedAmountLowThresholdRequest(builder, threshold);
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.dataConsumerId,
                                                     FBS::Request::Method::DATACONSUMER_SET_BUFFERED_AMOUNT_LOW_THRESHOLD,
                                                     FBS::Request::Body::DataConsumer_SetBufferedAmountLowThresholdRequest,
                                                     reqOffset);
        
        channel->request(reqId, reqData);
    }

    void DataConsumerController::setSubchannels(const std::vector<uint16_t>& subchannels)
    {
        SRV_LOGD("setSubchannels");

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqOffset = FBS::DataConsumer::CreateSetSubchannelsRequestDirect(builder, &subchannels);
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.dataConsumerId,
                                                     FBS::Request::Method::DATACONSUMER_SET_SUBCHANNELS,
                                                     FBS::Request::Body::DataConsumer_SetSubchannelsRequest,
                                                     reqOffset);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto response_ = response->body_as_DataConsumer_SetSubchannelsResponse();
        
        _subchannels.clear();
        
        for (const auto& channel : *response_->subchannels()) {
            _subchannels.emplace_back(channel);
        }
    }

    void DataConsumerController::send(const std::vector<uint8_t>& data, bool isBinary)
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
        
        auto reqOffset = FBS::DataConsumer::CreateSendRequestDirect(builder, ppid, &data);

        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder,
                                                     reqId,
                                                     _internal.dataConsumerId,
                                                     FBS::Request::Method::DATACONSUMER_SEND,
                                                     FBS::Request::Body::DataConsumer_SendRequest,
                                                     reqOffset);
        
        channel->request(reqId, reqData);
    }

    uint32_t DataConsumerController::getBufferedAmount()
    {
        SRV_LOGD("getBufferedAmount()");

        auto channel = _channel.lock();
        if (!channel) {
            return 0;
        }
           
        flatbuffers::FlatBufferBuilder builder;
        
        auto reqId = channel->genRequestId();
        
        auto reqData = MessageBuilder::createRequest(builder, reqId, _internal.dataConsumerId, FBS::Request::Method::DATACONSUMER_GET_BUFFERED_AMOUNT);
        
        auto respData = channel->request(reqId, reqData);
        
        auto message = FBS::Message::GetMessage(respData.data());
        
        auto response = message->data_as_Response();
        
        auto bufferedAmountResponse = response->body_as_DataConsumer_GetBufferedAmountResponse();

        return bufferedAmountResponse->bufferedAmount();
    }

    void DataConsumerController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        channel->notificationSignal.connect(&DataConsumerController::onChannel, shared_from_this());
    }

    void DataConsumerController::onChannel(const std::string& targetId, FBS::Notification::Event event, const std::vector<uint8_t>& data)
    {
        if (targetId != _internal.dataConsumerId) {
            return;
        }
        
        if (event == FBS::Notification::Event::DATACONSUMER_DATAPRODUCER_CLOSE) {
            if (_closed) {
                return;
            }
            _closed = true;
            
            auto channel = _channel.lock();
            if (!channel) {
                return;
            }
            
            channel->notificationSignal.disconnect(shared_from_this());
            
            this->dataProducerCloseSignal();
            
            this->closeSignal();
        }
        else if (event == FBS::Notification::Event::DATACONSUMER_SCTP_SENDBUFFER_FULL) {
            sctpSendBufferFullSignal();
        }
        else if (event == FBS::Notification::Event::DATACONSUMER_DATAPRODUCER_PAUSE) {
            if (_dataProducerPaused) {
                return;
            }
            
            _dataProducerPaused = true;
            
            this->dataProducerPauseSignal();
            
            if (!_paused) {
                this->pauseSignal();
            }
        }
        else if (event == FBS::Notification::Event::DATACONSUMER_DATAPRODUCER_RESUME) {
            if (!_dataProducerPaused) {
                return;
            }
            
            _dataProducerPaused = false;
            
            this->dataProducerResumeSignal();
            
            if (!_paused) {
                this->resumeSignal();
            }
        }
        else if (event == FBS::Notification::Event::DATACONSUMER_BUFFERED_AMOUNT_LOW) {
            auto message = FBS::Message::GetMessage(data.data());
            auto nf = message->data_as_Notification();
            if (auto bufferedAmountLow = nf->body_as_DataConsumer_BufferedAmountLowNotification()) {
                auto bufferedAmount = bufferedAmountLow->bufferedAmount();
                this->bufferedAmountLowSignal(bufferedAmount);
            }
        }
        else if (event == FBS::Notification::Event::DATACONSUMER_MESSAGE) {
            if (_closed) {
                return;
            }
            auto message = FBS::Message::GetMessage(data.data());
            auto notification = message->data_as_Notification();
            if (auto nf = notification->body_as_DataConsumer_MessageNotification()) {
                std::vector<uint8_t> data(nf->data()->begin(), nf->data()->end());
                this->messageSignal(data, nf->ppid());
            }
        }
        else {
            SRV_LOGD("ignoring unknown event %u", (uint8_t)event);
        }        
    }
}

namespace srv {

    FBS::DataProducer::Type dataConsumerTypeToFbs(const std::string& type)
    {
        if ("sctp" == type) {
            return FBS::DataProducer::Type::SCTP;
        }
        else if ("direct" == type) {
            return FBS::DataProducer::Type::DIRECT;
        }
        
        SRV_LOGE("invalid DataConsumerType: %s", type.c_str());
        
        return FBS::DataProducer::Type::MIN;
    }

    std::string dataConsumerTypeFromFbs(FBS::DataProducer::Type type)
    {
        switch (type)
        {
            case FBS::DataProducer::Type::SCTP:
                return "sctp";
            case FBS::DataProducer::Type::DIRECT:
                return "direct";
            default:
                return "";
        }
    }

    std::shared_ptr<DataConsumerDump> parseDataConsumerDumpResponse(const FBS::DataConsumer::DumpResponse* data)
    {
        auto dump = std::make_shared<DataConsumerDump>();
        
        dump->dataProducerId = data->dataProducerId()->str();
        dump->type = dataConsumerTypeFromFbs(data->type());
        
        if (auto params = data->sctpStreamParameters()) {
            dump->sctpStreamParameters = *parseSctpStreamParameters(params);
        }
        
        dump->label = data->label()->str();
        dump->protocol = data->protocol()->str();
        dump->bufferedAmountLowThreshold = data->bufferedAmountLowThreshold();
        dump->id = data->id()->str();
        dump->paused = data->paused();
        dump->dataProducerPaused = data->dataProducerPaused();
        
        for (const auto& channel : *data->subchannels()) {
            dump->subchannels.emplace_back(channel);
        }
        
        return dump;
    }

    std::shared_ptr<DataConsumerStat> parseDataConsumerStats(const FBS::DataConsumer::GetStatsResponse* binary)
    {
        auto stat = std::make_shared<DataConsumerStat>();
        
        stat->type = "data-consumer";
        stat->timestamp = binary->timestamp();
        stat->label = binary->label()->str();
        stat->protocol = binary->protocol()->str();
        stat->messagesSent = binary->messagesSent();
        stat->bytesSent = binary->bytesSent();
        stat->bufferedAmount = binary->bufferedAmount();
        
        return stat;
    }

    void to_json(nlohmann::json& j, const DataConsumerStat& st)
    {
        j["type"] = st.type;
        j["timestamp"] = st.timestamp;
        j["label"] = st.label;
        j["protocol"] = st.protocol;
        j["messagesSent"] = st.messagesSent;
        j["bytesSent"] = st.bytesSent;
        j["bufferedAmount"] = st.bufferedAmount;
    }

    void from_json(const nlohmann::json& j, DataConsumerStat& st)
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
        if (j.contains("messagesSent")) {
            j.at("messagesSent").get_to(st.messagesSent);
        }
        if (j.contains("bytesSent")) {
            j.at("bytesSent").get_to(st.bytesSent);
        }
        if (j.contains("bufferedAmount")) {
            j.at("bufferedAmount").get_to(st.bufferedAmount);
        }
    }

}
