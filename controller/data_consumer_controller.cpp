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
#include "payload_channel.h"

namespace srv {

    DataConsumerController::DataConsumerController(const DataConsumerInternal& internal,
                                                   const DataConsumerData& data,
                                                   const std::shared_ptr<Channel>& channel,
                                                   std::shared_ptr<PayloadChannel> payloadChannel,
                                                   const nlohmann::json& appData)
    : _internal(internal)
    , _data(data)
    , _channel(channel)
    , _payloadChannel(payloadChannel)
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
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->notificationSignal.disconnect(shared_from_this());
        
        nlohmann::json reqData;
        reqData["dataConsumerId"] = _internal.dataConsumerId;
        
        channel->request("transport.closeDataConsumer", _internal.transportId, reqData.dump());
        
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
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->notificationSignal.disconnect(shared_from_this());
        
        this->transportCloseSignal();
        
        this->closeSignal();
    }

    nlohmann::json DataConsumerController::dump()
    {
        SRV_LOGD("dump()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nlohmann::json();
        }
        
        return channel->request("dataConsumer.dump", _internal.dataConsumerId, "{}");
    }

    nlohmann::json DataConsumerController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return std::vector<DataConsumerStat>();
        }
        
        return channel->request("dataConsumer.getStats", _internal.dataConsumerId, "{}");
    }

    void DataConsumerController::setBufferedAmountLowThreshold(int32_t threshold)
    {
        SRV_LOGD("setBufferedAmountLowThreshold() [threshold:%d]", threshold);

        nlohmann::json reqData;
        reqData["threshold"] = threshold;
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        
        channel->request("dataConsumer.setBufferedAmountLowThreshold", _internal.dataConsumerId, reqData.dump());
    }

    void DataConsumerController::send(const uint8_t* payload, size_t payloadLen, bool isBinary)
    {
        if (payloadLen <= 0)
        {
            SRV_LOGD("message must be a string or a Buffer");
            return;
        }

        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
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

        uint32_t ppid = !isBinary ? (payloadLen > 0 ? 51 : 56) : (payloadLen > 0 ? 53 : 57);

        std::string requestData = std::to_string(ppid);

        payloadChannel->request("dataConsumer.send", _internal.dataConsumerId, requestData, payload, payloadLen);
    }

    int32_t DataConsumerController::getBufferedAmount()
    {
        SRV_LOGD("getBufferedAmount()");

        auto channel = _channel.lock();
        if (!channel) {
            return 0;
        }
        
        auto jsBufferedAmount = channel->request("dataConsumer.getBufferedAmount", _internal.dataConsumerId, "{}");

        int32_t bufferedAmount = 0;
        if (jsBufferedAmount.is_object() && jsBufferedAmount.find("bufferedAmount") != jsBufferedAmount.end()) {
            bufferedAmount = jsBufferedAmount["bufferedAmount"];
        }
        return bufferedAmount;
    }

    void DataConsumerController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return;
        }
        channel->notificationSignal.connect(&DataConsumerController::onChannel, shared_from_this());
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->notificationSignal.connect(&DataConsumerController::onPayloadChannel, shared_from_this());
    }

    void DataConsumerController::onChannel(const std::string& targetId, const std::string& event, const std::string& data)
    {
        if (targetId != _internal.dataConsumerId) {
            return;
        }
        
        if (event == "dataproducerclose") {
            if (_closed) {
                return;
            }
            _closed = true;
            
            auto channel = _channel.lock();
            if (!channel) {
                return;
            }
            channel->notificationSignal.disconnect(shared_from_this());
            
            auto payloadChannel = _payloadChannel.lock();
            if (!payloadChannel) {
                return;
            }
            payloadChannel->notificationSignal.disconnect(shared_from_this());
            
            this->dataProducerCloseSignal();
            
            this->closeSignal();
        }
        else if (event == "sctpsendbufferfull") {
            sctpSendBufferFullSignal();
        }
        else if (event == "bufferedamountlow") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object() && js.find("score") != js.end()) {
                int32_t bufferedAmount = js["bufferedAmount"];
                this->bufferedAmountLowSignal(bufferedAmount);
            }
        }
        else {
            SRV_LOGD("ignoring unknown event %s", event.c_str());
        }        
    }

    void DataConsumerController::onPayloadChannel(const std::string& targetId, const std::string& event, const std::string& data, const uint8_t* payload, size_t payloadLen)
    {
        if (_closed) {
            return;
        }
        
        if (targetId != _internal.dataConsumerId) {
            return;
        }
        
        if (event == "message") {
            auto js = nlohmann::json::parse(data);
            if (js.is_object() && js.find("ppid") != js.end()) {
                int32_t ppid = js["ppid"];
                this->messageSignal(payload, payloadLen, ppid);
            }
        }
        else {
            SRV_LOGD("ignoring unknown event %s", event.c_str());
        }
    }
}

namespace srv
{
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
