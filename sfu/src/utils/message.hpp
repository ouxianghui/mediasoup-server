/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: RTC-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#pragma once

#include <string>
#include "nlohmann/json.hpp"

class Message
{
public:
    static nlohmann::json parse(const std::string& raw);
    
    static nlohmann::json createRequest(const std::string& method, const nlohmann::json& data);
    
    static nlohmann::json createSuccessResponse(const nlohmann::json& request, const nlohmann::json& data);
    
    static nlohmann::json createErrorResponse(const nlohmann::json& request, int errorCode, const std::string& errorReason);
    
    static nlohmann::json createNotification(const std::string& method, const nlohmann::json& data);
};
