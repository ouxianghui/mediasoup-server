/***************************************************************************
 *
 * Project         _____    __   ____   _      _
 *                (  _  )  /__\ (_  _)_| |_  _| |_
 *                 )(_)(  /(__)\  )( (_   _)(_   _)
 *                (_____)(__)(__)(__)  |_|    |_|
 *
 *
 * Copyright 2018-present, Leonid Stryzhevskyi <lganzzzo@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************************/

#include "Base64Test.hpp"

#include "oatpp/encoding/Base64.hpp"

namespace oatpp { namespace test { namespace encoding {
  
void Base64Test::onRun() {

  oatpp::String message = "oat++ web framework";
  oatpp::String messageEncoded = "b2F0Kysgd2ViIGZyYW1ld29yaw==";
  
  {
    oatpp::String encoded = oatpp::encoding::Base64::encode(message);
    OATPP_LOGV(TAG, "encoded='%s'", encoded->c_str());
    OATPP_ASSERT(encoded->equals(messageEncoded.get()));
    oatpp::String decoded = oatpp::encoding::Base64::decode(encoded);
    OATPP_ASSERT(message->equals(decoded.get()));
  }
  
  {
    oatpp::String encoded = oatpp::encoding::Base64::encode(message, oatpp::encoding::Base64::ALPHABET_BASE64_URL_SAFE);
    OATPP_LOGV(TAG, "encoded='%s'", encoded->c_str());
    oatpp::String decoded = oatpp::encoding::Base64::decode(encoded, oatpp::encoding::Base64::ALPHABET_BASE64_URL_SAFE_AUXILIARY_CHARS);
    OATPP_ASSERT(message->equals(decoded.get()));
  }

}
  
}}}
