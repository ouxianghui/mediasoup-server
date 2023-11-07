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

#include "BodyDecoder.hpp"

namespace oatpp { namespace web { namespace protocol { namespace http { namespace incoming {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BodyDecoder

oatpp::async::CoroutineStarterForResult<const oatpp::String&>
BodyDecoder::decodeToStringAsync(const Headers& headers, const std::shared_ptr<data::stream::InputStream>& bodyStream) const {

  class ToStringDecoder : public oatpp::async::CoroutineWithResult<ToStringDecoder, const oatpp::String&> {
  private:
    const BodyDecoder* m_decoder;
    Headers m_headers;
    std::shared_ptr<oatpp::data::stream::InputStream> m_bodyStream;
    std::shared_ptr<oatpp::data::stream::ChunkedBuffer> m_outputStream;
  public:

    ToStringDecoder(const BodyDecoder* decoder,
                    const Headers& headers,
                    const std::shared_ptr<data::stream::InputStream>& bodyStream)
      : m_decoder(decoder)
      , m_headers(headers)
      , m_bodyStream(bodyStream)
      , m_outputStream(std::make_shared<data::stream::ChunkedBuffer>())
    {}

    Action act() override {
      return m_decoder->decodeAsync(m_headers, m_bodyStream, m_outputStream).next(yieldTo(&ToStringDecoder::onDecoded));
    }

    Action onDecoded() {
      return _return(m_outputStream->toString());
    }

  };

  return ToStringDecoder::startForResult(this, headers, bodyStream);

}

}}}}}
