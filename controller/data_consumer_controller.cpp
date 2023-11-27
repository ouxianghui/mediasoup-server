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

        auto requestOffset = FBS::Transport::CreateCloseDataConsumerRequestDirect(channel->builder(), _internal.dataConsumerId.c_str());
        
        channel->request(FBS::Request::Method::TRANSPORT_CLOSE_DATACONSUMER, FBS::Request::Body::Transport_CloseDataConsumerRequest, requestOffset, _internal.transportId);
        
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
        
        auto data = channel->request(FBS::Request::Method::DATACONSUMER_DUMP, _internal.dataConsumerId);
        
        auto message = FBS::Message::GetMessage(data.data());
        
        auto response = message->data_as_Response();
        
        auto dumpFBS = response->body_as_DataConsumer_DumpResponse();
        
        return parseDataConsumerDumpResponse(dumpFBS);
        
    }

    std::vector<std::shared_ptr<DataConsumerStat>> DataConsumerController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return {};
        }
        
        auto data = channel->request(FBS::Request::Method::DATACONSUMER_GET_STATS, _internal.dataConsumerId);
        
        auto message = FBS::Message::GetMessage(data.data());
        
        auto response = message->data_as_Response();
        
        auto statsFBS = response->body_as_DataConsumer_GetStatsResponse();

        return std::vector<std::shared_ptr<DataConsumerStat>> { parseDataConsumerStats(statsFBS) };
    }

    void DataConsumerController::pause()
    {
        SRV_LOGD("pause()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        channel->request(FBS::Request::Method::DATACONSUMER_PAUSE, _internal.dataConsumerId);
        
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
        
        channel->request(FBS::Request::Method::DATACONSUMER_RESUME, _internal.dataConsumerId);
        
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
        
        auto requestOffset = FBS::DataConsumer::CreateSetBufferedAmountLowThresholdRequest(channel->builder(), threshold);
        
        channel->request(FBS::Request::Method::DATACONSUMER_SET_BUFFERED_AMOUNT_LOW_THRESHOLD, FBS::Request::Body::DataConsumer_SetBufferedAmountLowThresholdRequest, requestOffset, _internal.dataConsumerId);
    }

    void DataConsumerController::setSubchannels(const std::vector<uint16_t>& subchannels)
    {
        SRV_LOGD("setSubchannels");

        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        auto requestOffset = FBS::DataConsumer::CreateSetSubchannelsRequestDirect(channel->builder(), &subchannels);
        
        auto data = channel->request(FBS::Request::Method::DATACONSUMER_SET_SUBCHANNELS, FBS::Request::Body::DataConsumer_SetSubchannelsRequest, requestOffset, _internal.dataConsumerId);
        
        auto message = FBS::Message::GetMessage(data.data());
        
        auto response = message->data_as_Response();
        
        auto subchannelsResponse = response->body_as_DataConsumer_SetSubchannelsResponse();
        
        _subchannels.clear();
        
        for (const auto& channel : *subchannelsResponse->subchannels()) {
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
        
        auto requestOffset = FBS::DataConsumer::CreateSendRequestDirect(channel->builder(), ppid, &data);

        channel->request(FBS::Request::Method::DATACONSUMER_SEND, FBS::Request::Body::DataConsumer_SendRequest, requestOffset, _internal.dataConsumerId);
    }

    uint32_t DataConsumerController::getBufferedAmount()
    {
        SRV_LOGD("getBufferedAmount()");

        auto channel = _channel.lock();
        if (!channel) {
            return 0;
        }
           
        auto data = channel->request(FBS::Request::Method::DATACONSUMER_GET_BUFFERED_AMOUNT, _internal.dataConsumerId);
        
        auto message = FBS::Message::GetMessage(data.data());
        
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
            auto notification = message->data_as_Notification();
            if (auto bufferedAmountLow = notification->body_as_DataConsumer_BufferedAmountLowNotification()) {
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

    FBS::DataProducer::Type DataConsumerController::dataConsumerTypeToFbs(const std::string& type)
    {
        if ("sctp") {
            return FBS::DataProducer::Type::SCTP;
        }
        else if ("direct") {
            return FBS::DataProducer::Type::DIRECT;
        }
        
        SRV_LOGE("invalid DataConsumerType: %s", type.c_str());
        
        return FBS::DataProducer::Type::MIN;
    }

    std::string DataConsumerController::dataConsumerTypeFromFbs(FBS::DataProducer::Type type)
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

    std::shared_ptr<DataConsumerDump> DataConsumerController::parseDataConsumerDumpResponse(const FBS::DataConsumer::DumpResponse* data)
    {
        auto dump = std::make_shared<DataConsumerDump>();
        
        dump->dataProducerId = data->dataProducerId()->str();
        dump->type = dataConsumerTypeFromFbs(data->type());
        dump->sctpStreamParameters = *parseSctpStreamParameters(data->sctpStreamParameters());
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

    std::shared_ptr<DataConsumerStat> DataConsumerController::parseDataConsumerStats(const FBS::DataConsumer::GetStatsResponse* binary)
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

}
