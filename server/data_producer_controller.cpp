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
#include "payload_channel.h"

namespace srv {

    DataProducerController::DataProducerController(const DataProducerInternal& internal,
                                                   const DataProducerData& data,
                                                   const std::shared_ptr<Channel>& channel,
                                                   std::shared_ptr<PayloadChannel> payloadChannel,
                                                   const nlohmann::json& appData)
    : _internal(internal)
    , _data(data)
    , _channel(channel)
    , _payloadChannel(payloadChannel)
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
        channel->_notificationSignal.disconnect(shared_from_this());
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->_notificationSignal.disconnect(shared_from_this());
        
        nlohmann::json reqData;
        reqData["dataProducerId"] = _internal.dataProducerId;
        
        channel->request("transport.closeDataProducer", _internal.transportId, reqData.dump());
        
        _closeSignal();
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
        channel->_notificationSignal.disconnect(shared_from_this());
        
        auto payloadChannel = _payloadChannel.lock();
        if (!payloadChannel) {
            return;
        }
        payloadChannel->_notificationSignal.disconnect(shared_from_this());
        
        _transportCloseSignal();
        
        _closeSignal();
    }

    nlohmann::json DataProducerController::dump()
    {
        SRV_LOGD("dump()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return nlohmann::json();
        }
        
        return channel->request("dataProducer.dump", _internal.dataProducerId, "{}");
    }

nlohmann::json DataProducerController::getStats()
    {
        SRV_LOGD("getStats()");
        
        auto channel = _channel.lock();
        if (!channel) {
            return std::vector<DataProducerStat>();
        }
        
        return channel->request("dataProducer.getStats", _internal.dataProducerId, "{}");
    }

    void DataProducerController::send(const uint8_t* payload, size_t payloadLen, bool isBinary)
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

        std::string nfData = std::to_string(ppid);

        payloadChannel->notify("dataProducer.send", _internal.dataProducerId, nfData, payload, payloadLen);
    }

    void DataProducerController::handleWorkerNotifications()
    {
        SRV_LOGD("handleWorkerNotifications()");
        
        // auto channel = _channel.lock();
        // if (!channel) {
        //     return;
        // }
        // channel->_notificationSignal.connect(&DataProducerController::onChannel, shared_from_this());
        //
        // auto payloadChannel = _payloadChannel.lock();
        // if (!payloadChannel) {
        //     return;
        // }
        // payloadChannel->_notificationSignal.connect(&DataProducerController::onPayloadChannel, shared_from_this());
    }

    void DataProducerController::onChannel(const std::string& targetId, const std::string& event, const std::string& data)
    {
 
    }

    void DataProducerController::onPayloadChannel(const std::string& targetId, const std::string& event, const std::string& data, const uint8_t* payload, size_t payloadLen)
    {

    }

}


namespace srv
{
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
