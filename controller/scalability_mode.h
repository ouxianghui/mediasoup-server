/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include "nlohmann/json.hpp"

namespace srv
{
    nlohmann::json parseScalabilityMode(const std::string& scalabilityMode);
}
