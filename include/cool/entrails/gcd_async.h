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

#if !defined(ENTRAILS_GCD_ASYNC_H_HEADER_GUARD)
#define ENTRAILS_GCD_ASYNC_H_HEADER_GUARD

#include <iostream>
#include <functional>
#include <atomic>
#include <memory>
#include <dispatch/dispatch.h>

namespace cool { namespace gcd { namespace async {

namespace entrails {

// -- context base class - every event source has this elements
template <typename Handler> class context_base
{
 public:
  context_base(const dispatch_source_t& s_, const Handler& h_)
    : m_source(s_)
#if !defined(WIN32_TARGET)
    , m_suspended(true)
#endif
    , m_handler(h_)
  {
#if defined(WIN32_TARGET)
    m_suspended = true;
#endif
  }
  ~context_base()
  {
  }

  void resume()
  {
    bool expected = true;
    if (m_suspended.compare_exchange_strong(expected, false))
      ::dispatch_resume(m_source);
  }

  void suspend()
  {
    bool expected = false;
    if (m_suspended.compare_exchange_strong(expected, true))
      ::dispatch_suspend(m_source);
  }

  const Handler& handler() const  { return m_handler; }
  const dispatch_source_t& event_source() const { return m_source; }
  dispatch_source_t& event_source() { return m_source; }

 private:
  dispatch_source_t m_source;
  std::atomic_bool  m_suspended;
  Handler           m_handler;
};

// -- Conditionally owned file descriptor
struct conditionally_owned
{
  conditionally_owned()
  { /* noop */ }
  conditionally_owned(int fd_, bool owned_)
      : fd(fd_)
      , is_owner(owned_)
  { /* noop */ }
  int  fd;
  bool is_owner;
};
// -- The dispatch source context
template <typename Handler>
class source_data_base
{
  typedef source_data_base<Handler> this_t;

 protected:
  source_data_base(const source_data_base&)            = delete;
  source_data_base& operator=(const source_data_base&) = delete;
  source_data_base(source_data_base&&)                 = delete;
  source_data_base& operator=(source_data_base&&)      = delete;
  source_data_base()                              = delete;
  source_data_base(dispatch_source_t ds, const Handler& cb)
      : m_source(ds)
#if defined(APPLE_TARGET) || defined(LINUX_TARGET)
      , m_suspended(true)
#endif
      , m_cb(cb)
  {
#if defined(WIN32_TARGET)
    m_suspended = true;
#else
    /* noop */
#endif
  }

 public:

  static Handler& handler(void* ctx)
  {
    return static_cast<this_t*>(ctx)->m_cb;
  }

  static dispatch_source_t& source(void* ctx)
  {
    return static_cast<this_t*>(ctx)->m_source;
  }

  dispatch_source_t& source()                 { return m_source; }
  const dispatch_source_t& source() const     { return m_source; }
  std::atomic_bool& suspended()               { return m_suspended; }
  const std::atomic_bool& suspended() const   { return m_suspended; }

 private:
  dispatch_source_t m_source;
  std::atomic_bool  m_suspended;
  Handler           m_cb;
};

template <typename Handler, typename Data>
struct source_data : public source_data_base<Handler>
{
  typedef source_data<Handler, Data> this_t;
  typedef source_data_base<Handler> base_t;

 public:
  source_data(dispatch_source_t ds, const Handler& cb)
      : base_t(ds, cb)
  { /* noop */ }

  static Data& data(void* ctx)
  {
    return static_cast<this_t*>(ctx)->m_data;
  }

  Data& data()             { return m_data; }
  const Data& data() const { return m_data; }
  void data(const Data& d) { m_data = d; }

 private:
  Data m_data;
};

/* specialization for void has no data member */
template <typename Handler>
struct source_data<Handler, void> : public source_data_base<Handler>
{
  typedef source_data_base<Handler> base_t;

 public:
  source_data(dispatch_source_t ds, const Handler& cb)
      : base_t(ds, cb)
  { /* noop */ }
};

// --- Reference class storing pointer to dispatch context data
template <typename Handler, typename Data>
class async_source_ref_base
{
 protected:
  typedef struct source_data<Handler, Data> source_data_t;

  async_source_ref_base(const async_source_ref_base&)            = delete;
  async_source_ref_base& operator=(const async_source_ref_base&) = delete;
  async_source_ref_base(async_source_ref_base&&)                 = delete;
  async_source_ref_base& operator=(async_source_ref_base&&)      = delete;
  async_source_ref_base()                                        = delete;

  async_source_ref_base(const dispatch_source_t& src, const Handler& cb)
  {
    if (src == NULL)
      throw exception::create_failure("Failed to create asynchronous event source");

    m_source = new source_data_t(src, cb);
    ::dispatch_set_context(src, m_source);
    ::dispatch_source_set_cancel_handler_f(src, cancel_cb);
  }
  ~async_source_ref_base()
  {
    if (m_source->suspended())
      ::dispatch_resume(m_source->source());

    ::dispatch_source_cancel(m_source->source());
  }

 private:
  static void cancel_cb(void* ctx)
  {
    delete static_cast<source_data_t*>(ctx);
  }

 public:
  const dispatch_source_t& source() const { return m_source->source(); }
  dispatch_source_t& source()             { return m_source->source(); }

  void resume()
  {
    bool expected = true;
    if (m_source->suspended().compare_exchange_strong(expected, false))
      ::dispatch_resume(m_source->source());
  }

  void suspend()
  {
    bool expected = false;
    if (m_source->suspended().compare_exchange_strong(expected, true))
      ::dispatch_suspend(m_source->source());
  }

 protected:
  source_data_t* m_source;
};

template <typename Handler, typename Data>
class async_source_ref : public async_source_ref_base<Handler, Data>
{
  typedef async_source_ref_base<Handler, Data> base_t;

 public:
  async_source_ref(const dispatch_source_t& src, const Handler& cb)
      : base_t(src, cb)
  { /* noop */ }

  Data& data()             { return base_t::m_source->data(); }
  const Data& data() const { return base_t::m_source->data(); }
  void data(const Data& d) { base_t::m_source->data(d); }
};

template <typename Handler>
class async_source_ref<Handler, void> : public async_source_ref_base<Handler, void>
{
  typedef async_source_ref_base<Handler, void> base_t;

 public:
  async_source_ref(const dispatch_source_t& src, const Handler& cb)
      : base_t(src, cb)
  { /* noop */ }
  /* empty */
};

template <typename Handler, typename Data>
class async_source
{
  typedef async_source_ref<Handler, Data> source_data_t;

 protected:
  async_source(const dispatch_source_t& source, const Handler& cb)
    : m_source(std::make_shared<async_source_ref<Handler, Data>>(source, cb))
  { /* noop */ }

  const dispatch_source_t& source() const { return m_source->source(); }
  dispatch_source_t& source()             { return m_source->source(); }

  void resume()
  {
    m_source->resume();
  }

  void suspend()
  {
    m_source->suspend();
  }

  source_data_t& context_data()             { return *m_source; }
  const source_data_t& context_data() const { return *m_source; }

 private:
  std::shared_ptr<source_data_t> m_source;
};

#if defined(APPLE_TARGET) || defined(LINUX_TARGET)

class fd_io : public async_source<std::function<void(int, std::size_t)>, conditionally_owned>
{
 protected:
  typedef std::function<void(int, std::size_t)> handler_t;

 protected:
  typedef entrails::source_data<handler_t, conditionally_owned> context_t;

 protected:
  fd_io(dispatch_source_type_t type,
        int fd,
        const handler_t& cb,
        const dispatch_queue_t& run,
        bool owner);
  ~fd_io();
  int fd() const { return context_data().data().fd; }

 private:
  static void cancel_cb(void* ctx);
  static void event_cb(void* ctx);
};

// -------------------------------------------------------------------------
// -----
// ----- Writer needs different implementation as it translates raw
// ----- write ready file descriptor events into write complete application
// ----- level events. The callbacks as stored by context_base template
// ----- would therefore not be made into user code but an interim object
// ----- which could go away too early - hence the context needs a weak
// ----- pointer to the writer object to check if it's still there.
// -----
// -------------------------------------------------------------------------

using write_complete_handler = std::function<void(const void *, std::size_t)>;
using error_handler = std::function<void(int)>;

// defined in implementation file. Among other things contains weak_ptr
// to below writer object
struct writer_context;

class writer
{
 public:
  using ptr      = std::shared_ptr<writer>;
  using weak_ptr = std::weak_ptr<writer>;

 public:
  writer(int fd_
       , const dispatch_queue_t& r_
       , const write_complete_handler& h_
       , const error_handler& eh_
       , bool owner_);
  ~writer();

  void self(const weak_ptr& self_);
  void write(const void *data, std::size_t size);
  bool busy() const { return m_busy; }

 private:
  void idle();
  void suspend();
  void resume();

  static void cancel_callback(void* ctx);
  static void event_callback(void* ctx);

  void write_ready_callback(std::size_t size_);

 private:
  // runtime context
  writer_context*        m_context;
  // suspend/resume traking flag
  std::atomic_bool       m_suspended;
  // user handlers
  write_complete_handler m_handler;
  error_handler          m_herror;
  // write operation info
  std::atomic_bool       m_busy;
  std::size_t            m_size;
  const void*            m_data;
  std::size_t            m_remain;
  const std::uint8_t*    m_position;
};



#endif


} // namespace

} } } // namespace
#endif
