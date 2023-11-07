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

#ifndef oatpp_openssl_Connection_hpp
#define oatpp_openssl_Connection_hpp

#include "oatpp/core/data/stream/Stream.hpp"
#include "oatpp/core/data/buffer/FIFOBuffer.hpp"

#include <openssl/ssl.h>

namespace oatpp { namespace openssl {

/**
 * TLS Connection implementation. Extends &id:oatpp::base::Countable; and &id:oatpp::data::stream::IOStream;.
 */
class Connection : public oatpp::base::Countable, public oatpp::data::stream::IOStream {
private:

  static int createBio(BIO*);
  static int destroyBio(BIO*);
  static BIO_METHOD* createBioMethod();
  static BIO_METHOD* getBioMethod();
  static int BioWrite(BIO*, const char *, int);
  static int BioRead(BIO*, char *, int);
  static long BioCtrl(BIO*, int, long, void *);

private:

  class ConnectionContext : public oatpp::data::stream::Context {
  private:
    Connection* m_connection;
    data::stream::StreamType m_streamType;
  public:

    ConnectionContext(Connection* connection, data::stream::StreamType streamType, Properties&& properties);

    void init() override;

    async::CoroutineStarter initAsync() override;

    bool isInitialized() const override;

    data::stream::StreamType getStreamType() const override;

  };

private:
  SSL* m_ssl;
  BIO* m_rbio;
  BIO* m_wbio;
private:
  std::shared_ptr<oatpp::data::stream::IOStream> m_stream;
private:
  std::atomic<bool> m_initialized;
private:
  async::Action m_readAction;
  async::Action m_writeAction;
private:
  ConnectionContext* m_inContext;
  ConnectionContext* m_outContext;
public:

  /**
   * Constructor.
   * @param ssl - pointer to OpenSSL ssl object;.
   * @param stream - underlying transport stream. &id:oatpp::data::stream::IOStream;.
   */
  Connection(SSL* ssl, const std::shared_ptr<oatpp::data::stream::IOStream>& stream);

  /**
   * Virtual destructor.
   */
  ~Connection();

  /**
   * Write operation callback.
   * @param data - pointer to data.
   * @param count - size of the data in bytes.
   * @param action - async specific action. If action is NOT &id:oatpp::async::Action::TYPE_NONE;, then
   * caller MUST return this action on coroutine iteration.
   * @return - actual number of bytes written. 0 - to indicate end-of-file.
   */
  v_io_size write(const void *data, v_buff_size count, async::Action& action) override;

  /**
   * Read operation callback.
   * @param buffer - pointer to buffer.
   * @param count - size of the buffer in bytes.
   * @param action - async specific action. If action is NOT &id:oatpp::async::Action::TYPE_NONE;, then
   * caller MUST return this action on coroutine iteration.
   * @return - actual number of bytes written to buffer. 0 - to indicate end-of-file.
   */
  oatpp::v_io_size read(void *buff, v_buff_size count, async::Action& action) override;

  /**
   * Set OutputStream I/O mode.
   * @param ioMode
   */
  void setOutputStreamIOMode(oatpp::data::stream::IOMode ioMode) override;

  /**
   * Set OutputStream I/O mode.
   * @return
   */
  oatpp::data::stream::IOMode getOutputStreamIOMode() override;

  /**
   * Get output stream context.
   * @return - &id:oatpp::data::stream::Context;.
   */
  oatpp::data::stream::Context& getOutputStreamContext() override;

  /**
   * Set InputStream I/O mode.
   * @param ioMode
   */
  void setInputStreamIOMode(oatpp::data::stream::IOMode ioMode) override;

  /**
   * Get InputStream I/O mode.
   * @return
   */
  oatpp::data::stream::IOMode getInputStreamIOMode() override;

  /**
   * Get input stream context. <br>
   * @return - &id:oatpp::data::stream::Context;.
   */
  oatpp::data::stream::Context& getInputStreamContext() override;

  /**
   * Get the underlying transport stream.
   * @return
   */
  std::shared_ptr<data::stream::IOStream> getTransportStream();
  
};
  
}}

#endif /* oatpp_openssl_Connection_hpp */
