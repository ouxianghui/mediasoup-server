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

#ifndef oatpp_openssl_Config_hpp
#define oatpp_openssl_Config_hpp

#include "oatpp-openssl/configurer/ContextConfigurer.hpp"

#include "oatpp/Types.hpp"

namespace oatpp { namespace openssl {

/**
 * Config.
 */
class Config {
private:
  std::list<std::shared_ptr<configurer::ContextConfigurer>> m_contextConfigs;
public:

  /**
   * Constructor.
   */
  Config();

  /**
   * Virtual destructor.
   */
  virtual ~Config();

  /**
   * Create shared `Config`.
   * @return
   */
  static std::shared_ptr<Config> createShared();

  /**
   * Create default shared `Config` for server.
   * @param certChainFile
   * @param privateKeyFile
   * @return
   */
  static std::shared_ptr<Config> createDefaultServerConfigShared(const oatpp::String& certChainFile,
                                                                 const oatpp::String& privateKeyFile);

  /**
   * Create default shared `Config` for client.
   * @return
   */
  static std::shared_ptr<Config> createDefaultClientConfigShared();

  /**
   * Clear context configurers.
   */
  void clearContextConfigurers();

  /**
   * Add context configurer.
   * @param contextConfigurer - &id:oatpp::openssl::configurer::ContextConfigurer;.
   */
  void addContextConfigurer(const std::shared_ptr<configurer::ContextConfigurer>& contextConfigurer);

  /**
   * Configure SSL context.
   * @param ctx
   */
  void configureContext(SSL_CTX* ctx) const;

};
  
}}

#endif /* oatpp_openssl_Config_hpp */
