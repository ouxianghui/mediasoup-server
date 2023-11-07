/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "srtp_parameters.h"

namespace srv {
    void to_json(nlohmann::json& j, const SrtpParameters& st)
    {
        j["cryptoSuite"] = st.cryptoSuite;
        j["keyBase64"] = st.keyBase64;
    }
    
    void from_json(const nlohmann::json& j, SrtpParameters& st) 
    {
        if (j.contains("cryptoSuite")) {
            j.at("cryptoSuite").get_to(st.cryptoSuite);
        }
        if (j.contains("keyBase64")) {
            j.at("keyBase64").get_to(st.keyBase64);
        }
    }
}
