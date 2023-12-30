/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <string>
#include <vector>
#include "flatbuffers/flexbuffers.h"
#include "FBS/srtpParameters.h"

namespace srv
{
    /**
    * SRTP parameters.
    */
    struct SrtpParameters
    {
        /**
        * Encryption and authentication transforms to be used.
          | 'AEAD_AES_256_GCM'
          | 'AEAD_AES_128_GCM'
          | 'AES_CM_128_HMAC_SHA1_80'
          | 'AES_CM_128_HMAC_SHA1_32'
        */
        std::string cryptoSuite;

        /**
        * SRTP keying material (master key and salt) in Base64.
        */
        std::string keyBase64;
        
        flatbuffers::Offset<FBS::SrtpParameters::SrtpParameters> serialize(flatbuffers::FlatBufferBuilder& builder) const;
    };

    std::string cryptoSuiteFromFbs(const FBS::SrtpParameters::SrtpCryptoSuite cryptoSuite);

    FBS::SrtpParameters::SrtpCryptoSuite cryptoSuiteToFbs(const std::string& cryptoSuite);

    std::shared_ptr<SrtpParameters> parseSrtpParameters(const FBS::SrtpParameters::SrtpParameters* binary);
}
