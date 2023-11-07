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

#ifndef oatpp_test_web_app_Controller_hpp
#define oatpp_test_web_app_Controller_hpp

#include "./DTOs.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "oatpp/core/utils/ConversionUtils.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"

#include <sstream>

namespace oatpp { namespace test { namespace openssl { namespace app {

class Controller : public oatpp::web::server::api::ApiController {
private:
  static constexpr const char* TAG = "test::web::app::Controller";
public:
  Controller(const std::shared_ptr<ObjectMapper>& objectMapper)
    : oatpp::web::server::api::ApiController(objectMapper)
  {}
public:
  
  static std::shared_ptr<Controller> createShared(const std::shared_ptr<ObjectMapper>& objectMapper = OATPP_GET_COMPONENT(std::shared_ptr<ObjectMapper>)){
    return std::make_shared<Controller>(objectMapper);
  }
  
#include OATPP_CODEGEN_BEGIN(ApiController)
  
  ENDPOINT("GET", "/", root) {
    //OATPP_LOGD(TAG, "GET '/'");
    return createResponse(Status::CODE_200, "Hello World!!!");
  }
  
  ENDPOINT("GET", "params/{param}", getWithParams,
           PATH(String, param))
  {
    //OATPP_LOGD(TAG, "GET params/%s", param->c_str());
    auto dto = TestDto::createShared();
    dto->testValue = param;
    return createDtoResponse(Status::CODE_200, dto);
  }
  
  ENDPOINT("GET", "queries", getWithQueries,
           QUERY(String, name), QUERY(Int32, age))
  {
    auto dto = TestDto::createShared();
    dto->testValue = "name=" + name + "&age=" + oatpp::utils::conversion::int32ToStr(age);
    return createDtoResponse(Status::CODE_200, dto);
  }

  ENDPOINT("GET", "queries/map", getWithQueriesMap,
           QUERIES(QueryParams, queries))
  {
    auto dto = TestDto::createShared();
    dto->testMap = Fields<String>({});
    for(auto& it : queries.getAll_Unsafe()) {
      dto->testMap->push_back({it.first.toString(), it.second.toString()});
    }
    return createDtoResponse(Status::CODE_200, dto);
  }
  
  ENDPOINT("GET", "headers", getWithHeaders,
           HEADER(String, param, "X-TEST-HEADER"))
  {
    //OATPP_LOGD(TAG, "GET headers {X-TEST-HEADER: %s}", param->c_str());
    auto dto = TestDto::createShared();
    dto->testValue = param;
    return createDtoResponse(Status::CODE_200, dto);
  }
  
  ENDPOINT("POST", "body", postBody,
           BODY_STRING(String, body))
  {
    //OATPP_LOGD(TAG, "POST body %s", body->c_str());
    auto dto = TestDto::createShared();
    dto->testValue = body;
    return createDtoResponse(Status::CODE_200, dto);
  }

  ENDPOINT("POST", "echo", echo,
           BODY_STRING(String, body))
  {
    //OATPP_LOGD(TAG, "POST body(echo) size=%d", body->getSize());
    return createResponse(Status::CODE_200, body);
  }

  ENDPOINT("GET", "header-value-set", headerValueSet,
           HEADER(String, valueSet, "X-VALUE-SET"))
  {
    oatpp::web::protocol::http::HeaderValueData values;
    oatpp::web::protocol::http::Parser::parseHeaderValueData(values, valueSet, ',');
    OATPP_ASSERT_HTTP(values.tokens.find("VALUE_1") != values.tokens.end(), Status::CODE_400, "No header 'VALUE_1' in value set");
    OATPP_ASSERT_HTTP(values.tokens.find("VALUE_2") != values.tokens.end(), Status::CODE_400, "No header 'VALUE_2' in value set");
    OATPP_ASSERT_HTTP(values.tokens.find("VALUE_3") != values.tokens.end(), Status::CODE_400, "No header 'VALUE_3' in value set");
    return createResponse(Status::CODE_200, "");
  }

#include OATPP_CODEGEN_END(ApiController)
  
};

}}}}

#endif /* oatpp_test_web_app_Controller_hpp */
