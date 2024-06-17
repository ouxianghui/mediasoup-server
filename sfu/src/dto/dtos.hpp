/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: RTC-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#pragma once

#include "oatpp/Types.hpp"
#include "oatpp/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

ENUM(MessageCodes, v_int32,
     VALUE(CODE_INFO, 0),

     VALUE(CODE_PEER_JOINED, 1),
     VALUE(CODE_PEER_LEFT, 2),
     VALUE(CODE_PEER_MESSAGE, 3),

     VALUE(CODE_API_ERROR, 9)
);

class PeerDto : public oatpp::DTO
{
public:
    DTO_INIT(PeerDto, DTO)
    DTO_FIELD(Int64, peerId);
    DTO_FIELD(String, peerName);
};

class MessageDto : public oatpp::DTO
{
public:
    DTO_INIT(MessageDto, DTO)
    
    DTO_FIELD(Int64, peerId);
    DTO_FIELD(String, peerName);
    DTO_FIELD(Enum<MessageCodes>::AsNumber::NotNull, code);
    DTO_FIELD(String, message);
    DTO_FIELD(Int64, timestamp);

    DTO_FIELD(List<Object<PeerDto>>, peers);
};

class StatPointDto : public oatpp::DTO
{
    DTO_INIT(StatPointDto, DTO);

    DTO_FIELD(Int64, timestamp);

    DTO_FIELD(UInt64, evFrontpageLoaded, "ev_front_page_loaded");

    DTO_FIELD(UInt64, evPeerConnected, "ev_peer_connected");
    DTO_FIELD(UInt64, evPeerDisconnected, "ev_peer_disconnected");
    DTO_FIELD(UInt64, evPeerZombieDropped, "ev_peer_zombie_dropped");
    DTO_FIELD(UInt64, evPeerSendMessage, "ev_peer_send_message");

    DTO_FIELD(UInt64, evRoomCreated, "ev_room_created");
    DTO_FIELD(UInt64, evRoomDeleted, "ev_room_deleted");
};

#include OATPP_CODEGEN_END(DTO)

