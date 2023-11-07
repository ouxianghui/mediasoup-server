/************************************************************************
 * @Copyright: 2023-2024
 * @FileName:
 * @Description: Open source mediasoup C++ controller library
 * @Version: 1.0.0
 * @Author: Jackie Ou
 * @CreateTime: 2023-10-30
 *************************************************************************/

#include "Message.hpp"
#include <random>

namespace {
    using std::default_random_engine;
    using std::uniform_real_distribution;
    int generateRandomNumber()
    {
        default_random_engine e;
        uniform_real_distribution<double> u(0, 1);
        
        return int(u(e) * 10000000);
    }
}

nlohmann::json Message::parse(const std::string& raw)
{
    //get json object from raw string
    auto object = nlohmann::json::parse(raw);
    auto message = nlohmann::json({});
    if (object.contains("request")) {
        if (object["request"].is_boolean()) {
            message["request"] = true;
            message["id"] = object["id"].get<int>();
            message["method"] = object["method"].get<std::string>();
            message["data"] = object["data"];
        }
    }
    else if (object.contains("response")) {
        if (object["response"].is_boolean()) {
            message["response"] = true;
            message["id"] = object["id"].get<int>();
            if (object["ok"].is_boolean()) {
                if (object["ok"]) {
                    message["ok"] = true;
                    message["data"] = object["data"];
                }
                else {
                    message["ok"] = false;
                    message["errorCode"] = object["errorCode"].get<int>();
                    message["errorReason"] = object["errorReason"].get<std::string>();
                }
            }
        }
    }
    else if (object.contains("notification")) {
        if (object["notification"].is_boolean()) {
            message["notification"] = true;
            message["id"] = object["id"];
            message["method"] = object["method"];
            message["data"] = object["data"];
            
        }
    }
    
    return message;
}

nlohmann::json Message::createRequest(const std::string& method, const nlohmann::json& data)
{
    nlohmann::json request;
    request["request"] = true;
    request["id"] = generateRandomNumber();
    request["method"] = method;
    request["data"] = data;
    
    return request;
}

nlohmann::json Message::createSuccessResponse(const nlohmann::json& request, const nlohmann::json& data)
{
    nlohmann::json response;
    response["response"] = true;
    response["id"] = request["id"].get<int>();
    response["ok"] = true;
    response["data"] = data;
    
    return response;
}

nlohmann::json Message::createErrorResponse(const nlohmann::json& request, int errorCode, const std::string& errorReason)
{
    nlohmann::json response;
    response["response"] = true;
    response["id"] = request["id"].get<int>();
    response["ok"] = false;
    response["errorCode"] = errorCode;
    response["errorReason"] = errorReason;
    
    return response;
}

nlohmann::json Message::createNotification(const std::string& method, const nlohmann::json& data)
{
    nlohmann::json notification;
    notification["notification"] = true;
    notification["method"] = method;
    notification["data"] = data;
    
    return notification;
}
