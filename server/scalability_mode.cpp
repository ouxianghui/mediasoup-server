/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "scalability_mode.h"
#include <regex>
#include "srv_logger.h"

static const std::regex ScalabilityModeRegex("^[LS]([1-9]\\d{0,1})T([1-9]\\d{0,1}).*", std::regex_constants::ECMAScript);

namespace srv
{
    nlohmann::json parseScalabilityMode(const std::string& scalabilityMode)
    {
        nlohmann::json jsonScalabilityMode;
        jsonScalabilityMode["spatialLayers"] =  1;
        jsonScalabilityMode["temporalLayers"] = 1;
        //jsonScalabilityMode["ksvc"] = 0;
        
        std::smatch match;
        
        std::regex_match(scalabilityMode, match, ScalabilityModeRegex);
        
        if (!match.empty()) {
            try {
                jsonScalabilityMode["spatialLayers"]  = std::stoul(match[1].str());
                jsonScalabilityMode["temporalLayers"] = std::stoul(match[2].str());
                //jsonScalabilityMode["ksvc"] = std::stoul(match[3].str());
            }
            catch (std::exception& e) {
                SRV_LOGW("invalid scalabilityMode: %s", e.what());
            }
        }
        else {
            SRV_LOGW("invalid scalabilityMode: %s", scalabilityMode.c_str());
        }
        
        return jsonScalabilityMode;
    }
}
