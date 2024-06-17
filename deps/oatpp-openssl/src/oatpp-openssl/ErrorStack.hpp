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

namespace oatpp { namespace openssl {

/**
 * Log all errors from the OpenSSL Error stack. <br>
 * A single OpenSSL method call can produce multiple error messages; show them all and leave empty stack for the next call.
 */
class ErrorStack {
private:
  static int logOneError(const char *str, size_t len, void *u);
public:

  /**
   * Log all OpenSSL errors into the oat++ log.
   */
  static void logErrors(const char *tag);
};

}}
