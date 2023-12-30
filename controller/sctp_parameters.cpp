/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "sctp_parameters.h"

namespace srv {
    
    flatbuffers::Offset<FBS::SctpParameters::SctpStreamParameters> SctpStreamParameters::serialize(flatbuffers::FlatBufferBuilder& builder) const
    {
        return FBS::SctpParameters::CreateSctpStreamParameters(builder,
                                                               this->streamId,
                                                               this->ordered,
                                                               this->maxPacketLifeTime ? flatbuffers::Optional<uint16_t>(this->maxPacketLifeTime) : flatbuffers::nullopt,
                                                               this->maxRetransmits ? flatbuffers::Optional<uint16_t>(this->maxRetransmits) : flatbuffers::nullopt);
    }

    std::shared_ptr<SctpParametersDump> parseSctpParametersDump(const FBS::SctpParameters::SctpParameters* binary)
    {
        auto dump = std::make_shared<SctpParametersDump>();
        
        dump->port = binary->port();
        dump->OS = binary->os();
        dump->MIS = binary->mis();
        dump->maxMessageSize = binary->maxMessageSize();
        dump->sendBufferSize = binary->sendBufferSize();
        dump->sctpBufferedAmount = binary->sctpBufferedAmount();
        dump->isDataChannel = binary->isDataChannel();
        
        return dump;
    }

    std::shared_ptr<SctpStreamParameters> parseSctpStreamParameters(const FBS::SctpParameters::SctpStreamParameters* data)
    {
        auto parameters = std::make_shared<SctpStreamParameters>();
        
        parameters->streamId = data->streamId();
        parameters->ordered = data->ordered().value_or(false);
        parameters->maxPacketLifeTime = data->maxPacketLifeTime().value_or(0);
        parameters->maxRetransmits = data->maxRetransmits().value_or(0);
        
        return parameters;
    }
    
}

namespace srv {
    
    void to_json(nlohmann::json& j, const SctpStreamParameters& st)
    {
        j["streamId"] = st.streamId;
        j["ordered"] = st.ordered;
        j["maxPacketLifeTime"] = st.maxPacketLifeTime;
        j["maxRetransmits"] = st.maxRetransmits;
    }
    
    void from_json(const nlohmann::json& j, SctpStreamParameters& st)
    {
        if (j.contains("streamId")) {
            j.at("streamId").get_to(st.streamId);
        }
        if (j.contains("ordered")) {
            j.at("ordered").get_to(st.ordered);
        }
        if (j.contains("maxPacketLifeTime")) {
            j.at("maxPacketLifeTime").get_to(st.maxPacketLifeTime);
        }
        if (j.contains("maxRetransmits")) {
            j.at("maxRetransmits").get_to(st.maxRetransmits);
        }
    }
    
    void to_json(nlohmann::json& j, const NumSctpStreams& st)
    {
        j["OS"] = st.OS;
        j["MIS"] = st.MIS;
    }
    
    void from_json(const nlohmann::json& j, NumSctpStreams& st)
    {
        if (j.contains("OS")) {
            j.at("OS").get_to(st.OS);
        }
        if (j.contains("MIS")) {
            j.at("MIS").get_to(st.MIS);
        }
    }
    
    void to_json(nlohmann::json& j, const SctpParameters& st)
    {
        j["port"] = st.port;
        j["OS"] = st.OS;
        j["MIS"] = st.MIS;
        j["maxMessageSize"] = st.maxMessageSize;
    }
    
    void from_json(const nlohmann::json& j, SctpParameters& st)
    {
        if (j.contains("port")) {
            j.at("port").get_to(st.port);
        }
        if (j.contains("OS")) {
            j.at("OS").get_to(st.OS);
        }
        if (j.contains("MIS")) {
            j.at("MIS").get_to(st.MIS);
        }
        if (j.contains("maxMessageSize")) {
            j.at("maxMessageSize").get_to(st.maxMessageSize);
        }
    }
    
    void to_json(nlohmann::json& j, const SctpCapabilities& st)
    {
        j["numStreams"] = st.numStreams;
    }
    
    void from_json(const nlohmann::json& j, SctpCapabilities& st)
    {
        if (j.contains("numStreams")) {
            j.at("numStreams").get_to(st.numStreams);
        }
    }
    
}
