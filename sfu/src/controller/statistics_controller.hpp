/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: RTC-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#pragma once

#include "dto/config.hpp"
#include "utils/statistics.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/network/ConnectionHandler.hpp"
#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"


#include OATPP_CODEGEN_BEGIN(ApiController) /// <-- Begin Code-Gen

class StatisticsController : public oatpp::web::server::api::ApiController
{
public:
    StatisticsController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
    : oatpp::web::server::api::ApiController(objectMapper) {}
    
private:
    typedef StatisticsController __ControllerType;
    
private:
    OATPP_COMPONENT(oatpp::Object<ConfigDto>, _appConfig);
    OATPP_COMPONENT(std::shared_ptr<Statistics>, _statistics);
    
public:
    ENDPOINT_ASYNC("GET", _appConfig->statisticsUrl, Stats)
    {
        ENDPOINT_ASYNC_INIT(Stats)
        Action act() override
        {
            auto json = controller->_statistics->getJsonData();
            auto response = controller->createResponse(Status::CODE_200, json);
            response->putHeader(Header::CONTENT_TYPE, "application/json");
            return _return(response);
        }
    };
};

#include OATPP_CODEGEN_END(ApiController) /// <-- End Code-Gen

