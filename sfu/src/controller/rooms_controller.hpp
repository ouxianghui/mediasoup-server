/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: RTC-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#pragma once

#include "oatpp-websocket/Handshaker.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/network/ConnectionHandler.hpp"
#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"
#include "srv_logger.h"

#include OATPP_CODEGEN_BEGIN(ApiController) /// <-- Begin Code-Gen

#define WS_SUBPROTOCOL "protoo"

class RoomsController : public oatpp::web::server::api::ApiController
{
public:
    RoomsController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
    : oatpp::web::server::api::ApiController(objectMapper) {}
    
private:
    typedef RoomsController __ControllerType;
    
private:
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, websocketConnectionHandler, "websocket");
    
public:
    ENDPOINT_ASYNC("GET", "/*", WS)
    {
        ENDPOINT_ASYNC_INIT(WS)
        
        Action act() override
        {
            oatpp::String subprotocol = request->getHeader("Sec-WebSocket-Protocol");
            
            OATPP_ASSERT_HTTP(subprotocol == WS_SUBPROTOCOL, Status::CODE_400, "unknown subprotocol")
            
            auto roomId = request->getQueryParameter("roomId", "139415115");
            auto peerId = request->getQueryParameter("peerId", "1394");
            auto forceH264 = request->getQueryParameter("forceH264", "false"); //type is oatpp::string should convert to bool
            auto forceVP9 = request->getQueryParameter("forceVP9", "false");
            
            SRV_LOGD("[Room] new connection roomId: %s peerId: %s", roomId->c_str(), peerId->c_str());
            
            /* Websocket handshake */
            auto response = oatpp::websocket::Handshaker::serversideHandshake(request->getHeaders(), controller->websocketConnectionHandler);

            auto parameters = std::make_shared<oatpp::network::ConnectionHandler::ParameterMap>();

            (*parameters)["roomId"] = roomId;
            (*parameters)["peerId"] = peerId;
            (*parameters)["forceH264"] = forceH264;
            (*parameters)["forceVP9"] = forceVP9;

            /* Set connection upgrade params */
            response->setConnectionUpgradeParameters(parameters);
            
            response->putHeader("Sec-WebSocket-Protocol", WS_SUBPROTOCOL);

            return _return(response);
        }
    };
};

#include OATPP_CODEGEN_END(ApiController) /// <-- End Code-Gen

