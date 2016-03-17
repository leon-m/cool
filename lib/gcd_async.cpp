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
#include <sstream>
#include <cerrno>
#include <cassert>
#include <cstdint>
#if defined(APPLE_TARGET) || defined(LINUX_TARGET)
#include <unistd.h>
#endif

#if defined(WIN32_TARGET)
#define WIN32_COOL_BUILD
#endif

#include "cool/gcd_async.h"

namespace cool { namespace gcd { namespace async {
  
namespace entrails {

#if defined(APPLE_TARGET) || defined(LINUX_TARGET)

fd_io::fd_io(dispatch_source_type_t type,
             int fd,
             const handler_t& cb,
             const dispatch_queue_t& runner,
             bool owner)
    : async_source(::dispatch_source_create(type, fd, 0, runner), cb)
{
  ::dispatch_source_set_cancel_handler_f(source(), cancel_cb);
  ::dispatch_source_set_event_handler_f(source(), event_cb);
  context_data().data(conditionally_owned(fd, owner));
}

fd_io::~fd_io()
{
}

void fd_io::cancel_cb(void *ctx)
{
  auto aux = context_t::data(ctx);
  if (aux.is_owner)
    ::close(aux.fd);
  delete static_cast<context_t*>(ctx);
}

void fd_io::event_cb(void *ctx)
{
  try
  {
    std::size_t size = ::dispatch_source_get_data(context_t::source(ctx));
    context_t::handler(ctx)(context_t::data(ctx).fd,size);
  }
  catch (...)
  {
    assert(0 && "User callback has thrown an exception");
  }
}
#endif
} // namespace
  
#if defined(APPLE_TARGET) || defined(LINUX_TARGET)

writer::writer(int fd, const task::runner& run, const handler_t& cb, const err_handler_t& ecb, bool owner)
  : m_writer(fd, std::bind(&writer::write_cb, this, std::placeholders::_1, std::placeholders::_2), run, owner)
  , m_data(nullptr)
  , m_cb(cb)
  , m_err_cb(ecb)
  , m_busy(false)
{ /* noop */ }

void writer::write(const void *data, std::size_t size)
{
  bool expect = false;
  if (!m_busy.compare_exchange_strong(expect, true))
    throw exception::illegal_state("writer is busy");
  
  m_data = data;
  m_size = size;
  m_remain = size;
  m_position = static_cast<const std::uint8_t*>(data);
  m_writer.start();
}

void writer::set_idle()
{
  m_writer.stop();
  m_data = nullptr;
  m_busy = false;
}

void writer::write_cb(int fd, std::size_t n)
{
  if (m_remain == 0)
    return;
  
  auto res = ::write(fd, m_position, m_remain);
  
  // upon error disable current write operation and invoke error callback
  if (res < 0)
  {
    auto err = errno;
    set_idle();
    if (m_err_cb)
      m_err_cb(err);
    
    m_busy = false;
    return;
  }
  
  m_remain -= res;
  m_position += res;
  
  if (m_remain == 0)
  {
    set_idle();
    if (m_cb)
      m_cb(m_data, m_size);
  }
}

reader_writer::reader_writer(int fd,
                             const task::runner& run,
                             const reader::handler_t& rd_cb,
                             const writer::handler_t& wr_cb,
                             const writer::err_handler_t& err_cb,
                             bool owner)
    : m_rd(fd, run, rd_cb, owner)
    , m_wr(fd, run, wr_cb, err_cb, false)
{ /* noop */ }
#endif
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// ----
// ---- Signals
// ----
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
#if defined(APPLE_TARGET) || defined(LINUX_TARGET)

signal::signal(int signo, const handler_t& handler)
    : signal(signo, handler, cool::gcd::task::runner::cool_default())
{ /* noop */ }
  
signal::signal(int signo, const handler_t& handler, const task::runner& runner)
    : async_source(::dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, signo, 0, runner), handler)
{
  switch (signo)
  {
    case SIGKILL:
    case SIGSTOP:
      throw exception::illegal_argument("SIGKILL and SIGSTOP cannot be intercepted.");
      
    default:
      if (::signal(signo, SIG_IGN) == SIG_ERR)
      {
        std::stringstream ss;
        ss << "Invalid signal number " << signo;
        throw exception::illegal_argument(ss.str());
      }
      ::dispatch_source_set_event_handler_f(source(), &signal::signal_handler);
      context_data().data(signo);
      start();
  }
}

void signal::signal_handler(void *ctx)
{
  try
  {
    unsigned long count = ::dispatch_source_get_data(context_t::source(ctx));
    context_t::handler(ctx)(context_t::data(ctx), static_cast<int>(count));
  }
  catch(...)
  {
    assert(0 && "User callback has thrown an exception");
  }
}
#endif
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// ----
// ---- Timers
// ----
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
timer::timer(const std::string& prefix, const handler_t& handler, const task::runner& runner)
    : named(prefix)
    , async_source(::dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, runner), handler)
{
  ::dispatch_source_set_event_handler_f(source(), &timer::timer_handler);
}

void timer::_set_period(uint64_t period, uint64_t leeway)
{
  if (period == 0)
    throw exception::illegal_argument("Timer interval must be greater than 0");
  
  leeway = leeway == 0 ? period / 100 : leeway;
  
  if (leeway < 1)
    leeway = 1;
  
  m_period = period;
  m_leeway = leeway;
}


void timer::timer_handler(void *ctx)
{
  try
  {
    context_t::handler(ctx)(::dispatch_source_get_data(context_t::source(ctx)));
  }
  catch(...)
  {
    assert(0 && "User callback has thrown an exception");
  }
}

void timer::start()
{
  if (m_period == 0)
    throw exception::illegal_state("The timer period was not set.");
  
  ::dispatch_source_set_timer(source(), ::dispatch_time(DISPATCH_TIME_NOW, m_period), m_period, m_leeway);
  async_source::resume();
}

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// ----
// ---- File System Observer
// ----
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
#if defined(APPLE_TARGET) || defined(LINUX_TARGET)

fs_observer::fs_observer(const handler_t& handler,
                         int fd,
                         unsigned long events,
                         const task::runner& runner)
    : async_source(::dispatch_source_create(DISPATCH_SOURCE_TYPE_VNODE, fd, events, runner), handler)
{
  ::dispatch_source_set_cancel_handler_f(source(), cancel_handler);
  ::dispatch_source_set_event_handler_f(source(), &fs_observer::handler);
  context_data().data(fd);
}

void fs_observer::handler(void* ctx)
{
  try
  {
    context_t::handler(ctx)(::dispatch_source_get_data(context_t::source(ctx)));
  }
  catch (...)
  {
    assert(0 && "User callback has thrown an exception");
  }
}

void fs_observer::cancel_handler(void *ctx)
{
  ::close(context_t::data(ctx));
  delete static_cast<context_t*>(ctx);
}
#endif
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// ----
// ---- Data Observer
// ----
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
data_observer::data_observer(const handler_t& handler,
                             const task::runner& runner,
                             CoalesceStrategy strategy,
                             unsigned long mask)
    : async_source(::dispatch_source_create(
            strategy == Add ? DISPATCH_SOURCE_TYPE_DATA_ADD : DISPATCH_SOURCE_TYPE_DATA_ADD, 0, mask, runner)
          , handler)
{
  ::dispatch_source_set_event_handler_f(source(), &data_observer::handler);
}

void data_observer::send(unsigned long value)
{
  ::dispatch_source_merge_data(source(), value);
}

void data_observer::handler(void* ctx)
{
  try
  {
    context_t::handler(ctx)(::dispatch_source_get_data(context_t::source(ctx)));
  }
  catch (...)
  {
    assert(0 && "User callback has thrown an exception");
  }
}

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// ----
// ---- Process Observer
// ----
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
#if defined(APPLE_TARGET)
proc_observer::proc_observer(const handler_t& handler,
                             pid_t pid,
                             const task::runner& runner,
                             unsigned long mask)
    : async_source(::dispatch_source_create(
            DISPATCH_SOURCE_TYPE_PROC, pid, mask, runner)
          , handler)
{
  ::dispatch_source_set_event_handler_f(source(), &proc_observer::handler);
}

void proc_observer::handler(void* ctx)
{
  try
  {
    context_t::handler(ctx)(::dispatch_source_get_data(context_t::source(ctx)));
  }
  catch (...)
  {
    assert(0 && "User callback has thrown an exception");
  }
}
#endif

} } } // namespace
