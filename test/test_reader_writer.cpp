/* Copyright (c) 2015 Digiverse d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License. The
 * license should be included in the source distribution of the Software;
 * if not, you may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * The above copyright notice and licensing terms shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <iostream>
#include <chrono>
#include <thread>
#include <condition_variable>

#include <gtest/gtest.h>
#include "cool/cool.h"

using namespace cool::gcd;
using namespace cool::net;

const short int use_port = 12345;
const std::size_t block_size = 10240;
const std::size_t num_blocks =   500;
const std::size_t total_size = block_size * num_blocks;

class waiter
{
 public:
  std::mutex& mutex()           { return m_mutex; }
  std::condition_variable& cv() { return m_cv; }

 private:
  std::mutex              m_mutex;
  std::condition_variable m_cv;
};

class server : public task::runner
             , public waiter
{
 public:
  enum class mode { reader, reader_writer, periodic_writer };

 public:
  server(mode = mode::reader);
  ~server();

  std::size_t rx_size() const  { return m_rx_size; }
  std::size_t tx_size() const  { return m_tx_size; }
  bool read_complete() const   { return m_r_complete; }
  bool write_complete() const  { return m_w_complete; }
  bool got_close() const       { return m_got_close; }
  void shutdown();

 private:
  void accept(int fd, std::size_t size);
  void receive(int fd, std::size_t size);
  void transmit(const void* data, std::size_t size);
  void error(int fd);

 private:
  mode m_mode;
  bool m_r_complete;
  bool m_w_complete;
  bool m_got_close;
  int m_listen_socket;
  int m_work_socket;
  std::unique_ptr<async::reader> m_listener;
  std::unique_ptr<async::reader_writer> m_receiver;
  std::unique_ptr<async::writer> m_writer;
  uint8_t m_buffer[block_size];
  std::size_t m_rx_size;
  std::size_t m_tx_size;
};

class client : public task::runner
             , public waiter
{
 public:
  client();
  ~client();
  std::size_t transmit_count() const { return m_tx_count; }
  std::size_t transmit_size() const { return m_tx_size; }

  bool complete() const { return m_complete; }

  void start() { m_writer->write(m_buffer, block_size); }
  void shutdown();

 private:
  void transmit(const void* data, std::size_t size);
  void on_error(int err);

 private:
  bool m_complete;
  int m_socket;
  std::unique_ptr<async::writer> m_writer;
  std::size_t m_tx_size;
  std::size_t m_tx_count;
  uint8_t m_buffer[block_size];
};

class rw_client : public task::runner
                , public waiter
{
 public:
  rw_client();
  ~rw_client();
  void start()                { m_writer->write(m_buffer, block_size); }
  void shudown();
  std::size_t rx_size() const { return m_rx_size; }
  std::size_t tx_size() const { return m_tx_size; }
  bool read_complete() const  { return m_r_complete; }
  bool write_complete() const { return m_w_complete; }
  bool got_close() const      { return m_got_close; }

 private:
  void transmit(const void* data, std::size_t size);
  void on_error(int err);
  void on_receive(int fd, std::size_t size);

 private:
  int m_socket;
  bool m_r_complete;
  bool m_w_complete;
  bool m_got_close;

  std::unique_ptr<async::reader_writer> m_writer;
  std::size_t m_tx_size;
  std::size_t m_rx_size;
  uint8_t m_buffer[block_size];
};

server::server(mode m)
  : m_mode(m), m_r_complete(false), m_w_complete(false)
  , m_got_close(false), m_rx_size(0), m_tx_size(0)
{
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(use_port);
  addr.sin_addr = static_cast<struct in_addr>(ipv4::any);

  m_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  EXPECT_NE(-1, m_listen_socket);
  {
    int enable = 1;
    EXPECT_NE(-1, setsockopt(m_listen_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)));
  }
  EXPECT_NE(-1, bind(m_listen_socket, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)));
  EXPECT_NE(-1, listen(m_listen_socket, 10));
  m_listener.reset(new async::reader(
      m_listen_socket
    , *this
    , std::bind(&server::accept, this, std::placeholders::_1, std::placeholders::_2)
    , false));

  EXPECT_TRUE(!!m_listener);
  m_listener->start();
}

server::~server()
{
  if (m_listener) m_listener->stop();
  if (m_receiver) m_receiver->stop();
  m_listener.reset();
  m_receiver.reset();
  if (m_listen_socket != -1)
    ::close(m_listen_socket);
  if (m_work_socket != -1)
    ::close(m_work_socket);
}

void server::shutdown()
{
  if (m_listener) m_listener->stop();
  if (m_receiver) m_receiver->stop();
  m_listener.reset();
  m_receiver.reset();
  ::close(m_listen_socket);
  ::close(m_work_socket);
  m_listen_socket = -1;
  m_work_socket = -1;
}


void server::accept(int fd, std::size_t size)
{
  m_work_socket = ::accept(m_listen_socket, nullptr, nullptr);
  ASSERT_NE(-1, m_work_socket);
  switch (m_mode)
  {
    case mode::reader:
    case mode::reader_writer:
      m_receiver.reset(new async::reader_writer(
          m_work_socket
        , *this
        , std::bind(&server::receive, this, std::placeholders::_1, std::placeholders::_2)
        , std::bind(&server::transmit, this, std::placeholders::_1, std::placeholders::_2)
        , std::bind(&server::error, this, std::placeholders::_1)
        , true));
      ASSERT_TRUE(!!m_receiver);
      m_receiver->start();
      break;

    case mode::periodic_writer:

      break;

    default:
      ASSERT_TRUE(false);
      break;
  }
}

void server::receive(int fd, std::size_t size)
{
  if (size == 0)
  {
    std::unique_lock<std::mutex> l(mutex());
    m_got_close = true;
    cv().notify_one();
    return;
  }

  auto sz = ::read(m_work_socket, m_buffer, sizeof(m_buffer));
  ASSERT_NE(-1, sz);
  m_rx_size += sz;
  if (m_rx_size == total_size)
  {
    std::unique_lock<std::mutex> l(mutex());
    m_r_complete = true;
    if (m_mode == mode::reader_writer)
      m_receiver->write(m_buffer, block_size);
    cv().notify_one();
  }
}

void server::transmit(const void *data, std::size_t size)
{
  m_tx_size += size;
  if (m_tx_size == total_size)
  {
    std::unique_lock<std::mutex> l(mutex());
    m_w_complete = true;
    cv().notify_one();
    return;
  }

  m_receiver->write(m_buffer, block_size);
}

void server::error(int fd)
{
  EXPECT_FALSE(true);
}

// ----- client
client::client() : m_complete(false), m_tx_size(0), m_tx_count(0)
{
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(use_port);
  addr.sin_addr = static_cast<struct in_addr>(ipv4::loopback);

  m_socket = socket(AF_INET, SOCK_STREAM, 0);
  EXPECT_NE(-1, m_socket);
  EXPECT_NE(-1, ::connect(m_socket, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)));
  m_writer.reset(new async::writer(
      m_socket
    , *this
    , std::bind(&client::transmit, this, std::placeholders::_1, std::placeholders::_2), std::bind(&client::on_error, this, std::placeholders::_1)
    , false)
  );
}

client::~client()
{
  if (m_socket >= 0)
    ::close(m_socket);
}

void client::shutdown()
{
  m_writer.reset();
  EXPECT_NE(-1, ::close(m_socket));
  m_socket = -1;
}

void client::transmit(const void *data, std::size_t size)
{
  ++m_tx_count;
  m_tx_size += size;

  if (m_tx_size == total_size)
  {
    std::unique_lock<std::mutex> l(mutex());
    m_complete = true;
    cv().notify_one();
    return;
  }

  m_writer->write(m_buffer, block_size);
}

void client::on_error(int err)
{
  EXPECT_FALSE(true);
}

// ----- rw_client
rw_client::rw_client() : m_socket(-1), m_r_complete(false), m_w_complete(false), m_got_close(false), m_tx_size(0), m_rx_size(0)
{
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(use_port);
  addr.sin_addr = static_cast<struct in_addr>(ipv4::loopback);

  m_socket = socket(AF_INET, SOCK_STREAM, 0);
  EXPECT_NE(-1, m_socket);
  EXPECT_NE(-1, ::connect(m_socket, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)));
  m_writer.reset(new async::reader_writer(
      m_socket
    , *this
    , std::bind(&rw_client::on_receive, this, std::placeholders::_1, std::placeholders::_2)
    , std::bind(&rw_client::transmit, this, std::placeholders::_1, std::placeholders::_2)
    , std::bind(&rw_client::on_error, this, std::placeholders::_1)
    , false));
  m_writer->start();
}

rw_client::~rw_client()
{
  if (m_writer)
    m_writer->stop();
  if (m_socket != -1)
    ::close(m_socket);
}

void rw_client::shudown()
{
  m_writer->stop();
  m_writer.reset();
  EXPECT_NE(-1, ::close(m_socket));
  m_socket = -1;
}

void rw_client::transmit(const void *data, std::size_t size)
{
  m_tx_size += size;
  if (m_tx_size == total_size)
  {
    std::unique_lock<std::mutex> l(mutex());
    m_w_complete = true;
    cv().notify_one();
    return;
  }

  m_writer->write(m_buffer, block_size);
}

void rw_client::on_error(int err)
{
  EXPECT_FALSE(true);
}

void rw_client::on_receive(int fd, std::size_t size)
{
  uint8_t aux[block_size];
  auto res = ::read(fd, aux, block_size);
  EXPECT_NE(-1, res);

  if (res == 0)
  {
    std::unique_lock<std::mutex> l(mutex());
    m_got_close = true;
    cv().notify_one();
    return;
  }

  m_rx_size += res;
  if (m_rx_size == total_size)
  {
    std::unique_lock<std::mutex> l(mutex());
    m_r_complete = true;
    cv().notify_one();
  }
}

TEST(reader_writer, with_writer_on_send)
{
  server srv;
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  client clt;

  clt.start();

  {
    std::unique_lock<std::mutex> l(srv.mutex());
    EXPECT_NO_THROW(srv.cv().wait_for(l, std::chrono::milliseconds(1000), [&srv] () { return srv.read_complete(); }));

    clt.shutdown();
    EXPECT_NO_THROW(srv.cv().wait_for(l, std::chrono::milliseconds(1000), [&srv] () { return srv.got_close(); }));
  }

  EXPECT_EQ(total_size, srv.rx_size());
  EXPECT_EQ(0, srv.tx_size());
  EXPECT_EQ(total_size, clt.transmit_size());
  EXPECT_TRUE(clt.complete());
  EXPECT_TRUE(srv.read_complete());
  EXPECT_TRUE(srv.got_close());
}

TEST(reader_writer, bidirectional_server_close)
{
  server srv(server::mode::reader_writer);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  rw_client clt;

  clt.start();   // client will start transmitting, wait for server to receive it all
  {
    std::unique_lock<std::mutex> l(srv.mutex());
    EXPECT_NO_THROW(srv.cv().wait_for(l, std::chrono::milliseconds(1000), [&srv] () { return srv.read_complete(); }));
  }

  // server should start transmitting automatically, wait for client to receive it all
  {
    std::unique_lock<std::mutex> l(clt.mutex());
    EXPECT_NO_THROW(clt.cv().wait_for(l, std::chrono::milliseconds(1000), [&clt] () { return clt.read_complete(); }));

    srv.shutdown();
    EXPECT_NO_THROW(clt.cv().wait_for(l, std::chrono::milliseconds(1000), [&clt] () { return clt.got_close(); }));
  }

  EXPECT_EQ(total_size, srv.tx_size());
  EXPECT_EQ(total_size, srv.rx_size());
  EXPECT_EQ(total_size, clt.tx_size());
  EXPECT_EQ(total_size, clt.rx_size());
  EXPECT_TRUE(srv.read_complete());
  EXPECT_TRUE(srv.write_complete());
  EXPECT_TRUE(clt.read_complete());
  EXPECT_TRUE(clt.write_complete());
  EXPECT_TRUE(clt.got_close());

}

