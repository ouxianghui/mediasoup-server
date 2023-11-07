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

#include "VectorTest.hpp"

#include "oatpp/core/Types.hpp"

namespace oatpp { namespace test { namespace core { namespace data { namespace mapping { namespace  type {

void VectorTest::onRun() {

  {
    OATPP_LOGI(TAG, "test default constructor...");
    oatpp::Vector<oatpp::String> vector;

    OATPP_ASSERT(!vector);
    OATPP_ASSERT(vector == nullptr);

    OATPP_ASSERT(vector.get() == nullptr);
    OATPP_ASSERT(vector.valueType->classId.id == oatpp::data::mapping::type::__class::AbstractVector::CLASS_ID.id);
    OATPP_ASSERT(vector.valueType->params.size() == 1);
    OATPP_ASSERT(vector.valueType->params.front() == oatpp::String::Class::getType());
    OATPP_LOGI(TAG, "OK");
  }

  {
    OATPP_LOGI(TAG, "test empty ilist constructor...");
    oatpp::Vector<oatpp::String> vector({});

    OATPP_ASSERT(vector);
    OATPP_ASSERT(vector != nullptr);
    OATPP_ASSERT(vector->size() == 0);

    OATPP_ASSERT(vector.get() != nullptr);
    OATPP_ASSERT(vector.valueType->classId.id == oatpp::data::mapping::type::__class::AbstractVector::CLASS_ID.id);
    OATPP_ASSERT(vector.valueType->params.size() == 1);
    OATPP_ASSERT(vector.valueType->params.front() == oatpp::String::Class::getType());
    OATPP_LOGI(TAG, "OK");
  }

  {
    OATPP_LOGI(TAG, "test createShared()...");
    oatpp::Vector<oatpp::String> vector = oatpp::Vector<oatpp::String>::createShared();

    OATPP_ASSERT(vector);
    OATPP_ASSERT(vector != nullptr);
    OATPP_ASSERT(vector->size() == 0);

    OATPP_ASSERT(vector.get() != nullptr);
    OATPP_ASSERT(vector.valueType->classId.id == oatpp::data::mapping::type::__class::AbstractVector::CLASS_ID.id);
    OATPP_LOGI(TAG, "OK");
  }

  {
    OATPP_LOGI(TAG, "test copy-assignment operator...");
    oatpp::Vector<oatpp::String> vector1({});
    oatpp::Vector<oatpp::String> vector2;

    vector2 = vector1;

    OATPP_ASSERT(vector1);
    OATPP_ASSERT(vector2);

    OATPP_ASSERT(vector1->size() == 0);
    OATPP_ASSERT(vector2->size() == 0);

    OATPP_ASSERT(vector1.get() == vector2.get());

    vector2->push_back("a");

    OATPP_ASSERT(vector1->size() == 1);
    OATPP_ASSERT(vector2->size() == 1);

    vector2 = {"b", "c"};

    OATPP_ASSERT(vector1->size() == 1);
    OATPP_ASSERT(vector2->size() == 2);

    OATPP_ASSERT(vector2[0] == "b");
    OATPP_ASSERT(vector2[1] == "c");
    OATPP_LOGI(TAG, "OK");
  }

  {
    OATPP_LOGI(TAG, "test move-assignment operator...");
    oatpp::Vector<oatpp::String> vector1({});
    oatpp::Vector<oatpp::String> vector2;

    vector2 = std::move(vector1);

    OATPP_ASSERT(!vector1);
    OATPP_ASSERT(vector2);
    OATPP_LOGI(TAG, "OK");
  }

  {
    OATPP_LOGI(TAG, "test get element by index...");
    oatpp::Vector<oatpp::String> vector = {"a", "b", "c"};

    OATPP_ASSERT(vector);
    OATPP_ASSERT(vector != nullptr);
    OATPP_ASSERT(vector->size() == 3);

    OATPP_ASSERT(vector[0] == "a");
    OATPP_ASSERT(vector[1] == "b");
    OATPP_ASSERT(vector[2] == "c");

    vector[1] = "Hello!";

    OATPP_ASSERT(vector->size() == 3);

    OATPP_ASSERT(vector[0] == "a");
    OATPP_ASSERT(vector[1] == "Hello!");
    OATPP_ASSERT(vector[2] == "c");
    OATPP_LOGI(TAG, "OK");
  }

  {
    OATPP_LOGI(TAG, "test polymorphicDispatcher...");
    oatpp::Vector<oatpp::String> vector = {"a", "b", "c"};

    auto polymorphicDispatcher = static_cast<const typename oatpp::Vector<oatpp::String>::Class::PolymorphicDispatcher*>(
      vector.valueType->polymorphicDispatcher
    );

    polymorphicDispatcher->addPolymorphicItem(vector, oatpp::String("d"));

    OATPP_ASSERT(vector->size() == 4);

    OATPP_ASSERT(vector[0] == "a");
    OATPP_ASSERT(vector[1] == "b");
    OATPP_ASSERT(vector[2] == "c");
    OATPP_ASSERT(vector[3] == "d");
    OATPP_LOGI(TAG, "OK");
  }

}

}}}}}}
