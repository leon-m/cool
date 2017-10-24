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
  ::dispatch_release(context_t::source(ctx));
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

// ---------------------------------------------------------------------------
// -----
// ----- Writer object implementation. Rather than passing the file descriptor
// ----- read ready events into the user code it accepts a buffer to write
// ----- and triggers an event when entire buffer was written
// -----
// ---------------------------------------------------------------------------

writer::writer(int fd
             , const task::runner& run
             , const handler_t& cb
             , const err_handler_t& ecb
             , bool owner)
{
  // need temp variable as runner -> dispatch_queue_t conversion is
  // private and std::make_shared is not a friend of the runner
  ::dispatch_queue_t q = run;
  m_impl = std::make_shared<entrails::writer>(fd, q, cb, ecb, owner);
  m_impl->self(m_impl);
}

void writer::write(const void *data, std::size_t size)
{
  m_impl->write(data, size);
}

bool writer::is_busy() const
{
  return m_impl->busy();
}

namespace entrails {

struct writer_context
{
  writer::weak_ptr    m_writer;
  conditionally_owned m_fd;
  dispatch_source_t   m_source;
};

writer::writer(int fd_
             , const dispatch_queue_t& r_
             , const write_complete_handler& h_
             , const error_handler& eh_
             , bool owner_)
   : m_suspended(true)
   , m_handler(h_)
   , m_herror(eh_)
   , m_busy(false)
   , m_data(nullptr)
{
  // note: this context will be deleted in cancel callback
  m_context = new writer_context;
  m_context->m_fd = conditionally_owned(fd_, owner_);
  m_context->m_source = ::dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE, fd_, 0, r_);

  ::dispatch_source_set_cancel_handler_f(m_context->m_source, cancel_callback);
  ::dispatch_source_set_event_handler_f(m_context->m_source, event_callback);
  ::dispatch_set_context(m_context->m_source, m_context);
}

writer::~writer()
{
  resume();
  ::dispatch_source_cancel(m_context->m_source);
}

void writer::self(const weak_ptr& self_)
{
  m_context->m_writer = self_;
}

void writer::resume()
{
  bool expected = true;
  if (m_suspended.compare_exchange_strong(expected, false))
    ::dispatch_resume(m_context->m_source);
}

void writer::suspend()
{
  bool expected = false;
  if (m_suspended.compare_exchange_strong(expected, true))
    ::dispatch_suspend(m_context->m_source);
}

void writer::write(const void *data, std::size_t size)
{
  bool expect = false;
  if (!m_busy.compare_exchange_strong(expect, true))
    throw exception::illegal_state("writer is busy");

  m_data = data;
  m_size = size;
  m_remain = size;
  m_position = static_cast<const std::uint8_t*>(data);
  resume();
}

void writer::idle()
{
  suspend();
  m_data = nullptr;
  m_busy = false;
}

void writer::cancel_callback(void *ctx)
{
  auto context = static_cast<writer_context*>(ctx);
  ::dispatch_release(context->m_source);
  if (context->m_fd.is_owner)
    ::close(context->m_fd.fd);

  delete context;
}

void writer::event_callback(void *ctx)
{
  auto self = static_cast<writer_context*>(ctx)->m_writer.lock();
  if (!self)
    return;
  std::size_t size = ::dispatch_source_get_data(self->m_context->m_source);
  self->write_ready_callback(size);
}

void writer::write_ready_callback(std::size_t size_)
{
  if (m_remain == 0)
    return;

  auto res = ::write(m_context->m_fd.fd, m_position, m_remain);

  // upon error cancel current write operation and invoke error callback
  if (res < 0)
  {
    auto err = errno;
    idle();
    if (m_herror)
      m_herror(err);

    return;
  }

  m_remain -= res;
  m_position += res;

  if (m_remain == 0)
  {
    idle();
    if (m_handler)
      m_handler(m_data, m_size);
  }
}

} // namespace


// NOTE: below dup is necessary as ubuntu 16.04 does not run read and
// write event sources on the same file descriptor
reader_writer::reader_writer(int fd,
                             const task::runner& run,
                             const reader::handler_t& rd_cb,
                             const writer::handler_t& wr_cb,
                             const writer::err_handler_t& err_cb,
                             bool owner)
    : m_rd(fd, run, rd_cb, owner)
    , m_wr(::dup(fd), run, wr_cb, err_cb, true)
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
  ::dispatch_source_set_cancel_handler_f(source(), &timer::cancel_handler);
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

void timer::cancel_handler(void *ctx)
{
  ::dispatch_release(context_t::source(ctx));
  delete static_cast<context_t*>(ctx);
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
  ::dispatch_release(context_t::source(ctx));
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
            strategy == Add ? DISPATCH_SOURCE_TYPE_DATA_ADD : DISPATCH_SOURCE_TYPE_DATA_OR, 0, mask, runner)
          , handler)
{
  ::dispatch_source_set_cancel_handler_f(source(), &data_observer::cancel_handler);
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

void data_observer::cancel_handler(void *ctx)
{
  ::dispatch_release(context_t::source(ctx));
  delete static_cast<context_t*>(ctx);
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
  ::dispatch_source_set_cancel_handler_f(source(), &proc_observer::cancel_handler);
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

void proc_observer::cancel_handler(void *ctx)
{
  ::dispatch_release(context_t::source(ctx));
  delete static_cast<context_t*>(ctx);
}

#endif

} } } // namespace
