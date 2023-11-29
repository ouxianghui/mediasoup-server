/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "srtp_parameters.h"
#include "srv_logger.h"

namespace srv {

    flatbuffers::Offset<FBS::SrtpParameters::SrtpParameters> SrtpParameters::serialize(flatbuffers::FlatBufferBuilder& builder) const
    {
        return FBS::SrtpParameters::CreateSrtpParametersDirect(builder, cryptoSuiteToFbs(this->cryptoSuite), this->keyBase64.c_str());
    }

    std::string cryptoSuiteFromFbs(const FBS::SrtpParameters::SrtpCryptoSuite cryptoSuite)
    {
        switch (cryptoSuite)
        {
            case FBS::SrtpParameters::SrtpCryptoSuite::AEAD_AES_256_GCM:
                return "AEAD_AES_256_GCM";
            case FBS::SrtpParameters::SrtpCryptoSuite::AEAD_AES_128_GCM:
                return "AEAD_AES_128_GCM";
            case FBS::SrtpParameters::SrtpCryptoSuite::AES_CM_128_HMAC_SHA1_80:
                return "AES_CM_128_HMAC_SHA1_80";
            case FBS::SrtpParameters::SrtpCryptoSuite::AES_CM_128_HMAC_SHA1_32:
                return "AES_CM_128_HMAC_SHA1_32";
            default:
                return "";
        }
    }

    FBS::SrtpParameters::SrtpCryptoSuite cryptoSuiteToFbs(const std::string& cryptoSuite)
    {
        if (cryptoSuite == "AEAD_AES_256_GCM") {
            return FBS::SrtpParameters::SrtpCryptoSuite::AEAD_AES_256_GCM;
        }
        else if (cryptoSuite == "AEAD_AES_128_GCM") {
            return FBS::SrtpParameters::SrtpCryptoSuite::AEAD_AES_128_GCM;
        }
        else if (cryptoSuite == "AES_CM_128_HMAC_SHA1_80") {
            return FBS::SrtpParameters::SrtpCryptoSuite::AES_CM_128_HMAC_SHA1_80;
        }
        else if (cryptoSuite == "AES_CM_128_HMAC_SHA1_32") {
            return FBS::SrtpParameters::SrtpCryptoSuite::AES_CM_128_HMAC_SHA1_32;
        }
        else {
            SRV_LOGE("invalid SrtpCryptoSuite: %s", cryptoSuite.c_str());
            return FBS::SrtpParameters::SrtpCryptoSuite::MIN;
        }
    }

    std::shared_ptr<SrtpParameters> parseSrtpParameters(const FBS::SrtpParameters::SrtpParameters* binary)
    {
        auto parameters = std::make_shared<SrtpParameters>();
        
        parameters->cryptoSuite = cryptoSuiteFromFbs(binary->cryptoSuite());
        parameters->keyBase64 = binary->keyBase64()->str();
        
        return parameters;
    }

}
