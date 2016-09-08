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

#include <gtest/gtest.h>
#include "cool/cool.h"

using namespace cool::gcd;
using namespace cool::net;

const short int use_port = 12345;
const std::size_t block_size = 10240;
const std::size_t num_blocks =   500;
const std::size_t total_size = block_size * num_blocks;

class server : public task::runner
{
 public:
  server();
  ~server();

  std::size_t received_size() const { return m_rx_size; }

 private:
  void accept(int fd, std::size_t size);
  void receive(int fd, std::size_t size);

 private:
  int m_listen_socket;
  int m_receive_socket;
  std::unique_ptr<async::reader> m_listener;
  std::unique_ptr<async::reader> m_receiver;
  uint8_t m_buffer[block_size];
  std::size_t m_rx_size;
  std::size_t m_rx_count;
};

class client : public task::runner
{
 public:
  client();
  ~client();
  std::size_t transmit_count() const { return m_tx_count; }
  void start() { m_writer->write(m_buffer, block_size); }

 private:
  void transmit(const void* data, std::size_t size);
  void on_error(int err);

 private:
  int m_socket;
  std::unique_ptr<async::writer> m_writer;
  std::size_t m_tx_size;
  std::size_t m_tx_count;
  uint8_t m_buffer[block_size];
};

class rw_client : public task::runner
{
 public:
  rw_client();
  ~rw_client();
  std::size_t transmit_count() const { return m_tx_count; }
  void start() { m_writer->write(m_buffer, block_size); }

 private:
  void transmit(const void* data, std::size_t size);
  void on_error(int err);
  void on_receive(int fd, std::size_t size);

 private:
  int m_socket;
  std::unique_ptr<async::reader_writer> m_writer;
  std::size_t m_tx_size;
  std::size_t m_tx_count;
  uint8_t m_buffer[block_size];
};

server::~server()
{
  if (m_listener) m_listener->stop();
  if (m_receiver) m_receiver->stop();
  ::close(m_listen_socket);
  ::close(m_receive_socket);
}

client::~client()
{
  ::close(m_socket);
}

rw_client::~rw_client()
{
  if (m_writer)
    m_writer->stop();
  ::close(m_socket);
}

server::server() : m_rx_size(0), m_rx_count(0)
{
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(use_port);
  addr.sin_addr = static_cast<struct in_addr>(ipv4::any);

  m_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  EXPECT_NE(-1, m_listen_socket);
  EXPECT_NE(-1, bind(m_listen_socket, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)));
  EXPECT_NE(-1, listen(m_listen_socket, 10));
  m_listener.reset(new async::reader(m_listen_socket, *this, std::bind(&server::accept, this, std::placeholders::_1, std::placeholders::_2), false));
  EXPECT_TRUE(!!m_listener);
  m_listener->start();
}

client::client() : m_tx_size(0), m_tx_count(1)
{
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(use_port);
  addr.sin_addr = static_cast<struct in_addr>(ipv4::loopback);

  m_socket = socket(AF_INET, SOCK_STREAM, 0);
  EXPECT_NE(-1, m_socket);
  EXPECT_NE(-1, ::connect(m_socket, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)));
  m_writer.reset(new async::writer(m_socket, *this, std::bind(&client::transmit, this, std::placeholders::_1, std::placeholders::_2), std::bind(&client::on_error, this, std::placeholders::_1), false));
}

rw_client::rw_client() : m_tx_size(0), m_tx_count(1)
{
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(use_port);
  addr.sin_addr = static_cast<struct in_addr>(ipv4::loopback);

  m_socket = socket(AF_INET, SOCK_STREAM, 0);
  EXPECT_NE(-1, m_socket);
  EXPECT_NE(-1, ::connect(m_socket, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)));
  m_writer.reset(new async::reader_writer(m_socket, *this, std::bind(&rw_client::on_receive, this, std::placeholders::_1, std::placeholders::_2), std::bind(&rw_client::transmit, this, std::placeholders::_1, std::placeholders::_2), std::bind(&rw_client::on_error, this, std::placeholders::_1), false));
  m_writer->start();
}

void server::accept(int fd, std::size_t size)
{
  m_receive_socket = ::accept(m_listen_socket, nullptr, nullptr);
  ASSERT_NE(-1, m_receive_socket);
  m_receiver.reset(new async::reader(m_receive_socket, *this, std::bind(&server::receive, this, std::placeholders::_1, std::placeholders::_2), false));
  ASSERT_TRUE(!!m_receiver);
  m_receiver->start();

}

void server::receive(int fd, std::size_t size)
{
  auto sz = ::read(m_receive_socket, m_buffer, sizeof(m_buffer));
  ASSERT_NE(-1, sz);
  ++m_rx_count;
  m_rx_size += sz;
}

void client::transmit(const void *data, std::size_t size)
{
  if (m_tx_count == num_blocks)
    return;

  ++m_tx_count;
  m_tx_size += block_size;
  m_writer->write(m_buffer, block_size);
}

void rw_client::transmit(const void *data, std::size_t size)
{
  if (m_tx_count == num_blocks)
    return;

  ++m_tx_count;
  m_tx_size += block_size;
  m_writer->write(m_buffer, block_size);
}

void client::on_error(int err)
{
  EXPECT_FALSE(true);
}

void rw_client::on_error(int err)
{
  EXPECT_FALSE(true);
}

void rw_client::on_receive(int fd, std::size_t size)
{
  EXPECT_FALSE(true);
}


TEST(reader_writer, with_writer_on_send)
{
  server srv;
  client clt;

  clt.start();

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  EXPECT_EQ(total_size, srv.received_size());
  EXPECT_EQ(num_blocks, clt.transmit_count());
}

TEST(reader_writer, with_reader_writer_on_send)
{
  server srv;
  rw_client clt;

  clt.start();

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  EXPECT_EQ(total_size, srv.received_size());
  EXPECT_EQ(num_blocks, clt.transmit_count());
}



