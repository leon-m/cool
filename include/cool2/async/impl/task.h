/* Copyright (c) 2016 Digiverse d.o.o.
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

#if !defined(cool_ec4dbec3_f2af_4c94_8dc2_8ecde18e1bb4)
#define cool_ec4dbec3_f2af_4c94_8dc2_8ecde18e1bb4

#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace cool { namespace async {

namespace impl {

class runner_not_available : public cool::exception::runtime_exception
{
 public:
  runner_not_available()
    : runtime_exception("the destination runner not available")
  { /* noop */ }
};
  

// ----- common types
enum class task_type { simple, parallel, serial, intercept };

namespace tag
{
struct simple    { static const constexpr task_type value = task_type::simple;    };
struct serial    { static const constexpr task_type value = task_type::serial;    };
struct parallel  { static const constexpr task_type value = task_type::parallel;  };
struct intercept { static const constexpr task_type value = task_type::intercept; };
} // namespace

struct taskinfo;      // static task data
using taskinfo_ptr = std::shared_ptr<taskinfo>;
struct context;       // task execution context
using context_ptr = context*;

using bound_entry_point   = std::function<void(const  runner::ptr&, context_ptr)>;
using exception_reporter  = std::function<void(const std::exception_ptr&)>;
using deleter_type        = void(*)(void*);

template <typename ResultT, typename ParamT> class types
{
 public:
  using user_callable       = std::function<ResultT(const runner::ptr&, const ParamT&)>;
  using bound_user_callable = std::function<ResultT(const runner::ptr&)>;
  using entry_point         = std::function<void(context_ptr, const ParamT&)>;

  using binder_type         = std::function<bound_entry_point(const taskinfo_ptr&, const ParamT&)>;
  using binder_type_ptr     = binder_type*;
  using binder_result       = bound_entry_point;
  using result_reporter     = std::function<void(const ResultT&)>;
};

template <typename ResultT> class types<ResultT, void>
{
 public:
  using user_callable       = std::function<ResultT(const runner::ptr&)>;
  using bound_user_callable = std::function<ResultT(const runner::ptr&)>;
  using entry_point         = std::function<void(context_ptr)>;

  using binder_type         = std::function<bound_entry_point(const taskinfo_ptr&)>;
  using binder_type_ptr     = binder_type*;
  using binder_result       = bound_entry_point;
  using result_reporter     = std::function<void(const ResultT&)>;
};

template <typename ParamT> class types<void, ParamT>
{
 public:
  using user_callable       = std::function<void(const runner::ptr&, const ParamT&)>;
  using bound_user_callable = std::function<void(const runner::ptr&)>;
  using entry_point         = std::function<void(context_ptr, const ParamT&)>;

  using binder_type         = std::function<bound_entry_point(const taskinfo_ptr&, const ParamT&)>;
  using binder_type_ptr     = binder_type*;
  using binder_result       = bound_entry_point;
};

template<> class types<void, void>
{
public:
  using user_callable       = std::function<void(const runner::ptr&)>;
  using bound_user_callable = std::function<void(const runner::ptr&)>;
  using entry_point         = std::function<void(context_ptr)>;

  using binder_type         = std::function<bound_entry_point(const taskinfo_ptr&)>;
  using binder_type_ptr     = binder_type*;
  using binder_result       = bound_entry_point;
};
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
// Task taskinfo structures for different task types
//
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

namespace simple    { class taskinfo; class context; }
namespace serial    { class taskinfo; class context; }
namespace parallel  { class taskinfo; class context; }
namespace intercept { class taskinfo; class context; }


namespace simple
{

class taskinfo
{
 public:
  taskinfo() : m_unbound(nullptr)
  { /* noop */ }
  ~taskinfo()
  {
    if (m_unbound != nullptr)
      m_cleaner(m_unbound);
  }
  void unbound(void* ubnd_, const deleter_type& clr_)
  {
    m_unbound = ubnd_;
    m_cleaner = clr_;
  };
  void* unbound()
  {
    return m_unbound;
  }
 private:
  void*              m_unbound;   // binder for user callable to pass parameter to ep
  deleter_type       m_cleaner;   // delete function to delete binder instance
};

class context
{
 public:
  context() : m_result(nullptr)                   { /* noop */ }
  ~context()
  {
    if (m_result != nullptr)
      (*m_deleter)(m_result);
  }

  void entry_point(const bound_entry_point& arg_) { m_ep = arg_; }
  bound_entry_point& entry_point()                { return m_ep; }
  void exception(const exception_reporter& arg_)  { m_exception = arg_; }
  const exception_reporter& exception() const     { return m_exception; }
  void result(void* arg_, const deleter_type& d_) { m_result = arg_; m_deleter = d_; }
  void* result() const                            { return m_result; }

 private:
  bound_entry_point  m_ep;        // user callable with bound input parameter
  exception_reporter m_exception; // function to use to report exception
  void*              m_result;    // pointer to function to use to report result
  deleter_type       m_deleter;
};


} // namespace

namespace serial
{

class taskinfo
{
  using sequence_type = std::vector<taskinfo_ptr>;

 public:
  sequence_type& sequence()             { return m_sequence; }
  const sequence_type& sequence() const { return m_sequence; }

 private:
  sequence_type m_sequence;
};

class context
{
  
};
} // namespace

namespace parallel
{
class taskinfo
{

};

class context
{

};

} // namespace

namespace intercept
{

class taskinfo
{

};

class context
{

};

}

template <typename SimpleT, typename SerialT, typename ParallelT, typename InterceptT>
class u_one_of
{
 public:
  u_one_of(task_type t_) : m_type(t_)
  {
    m_u.m_simple.~SimpleT();  // union is initialized, destroy current object

    switch (t_)
    {
      case task_type::simple:    new (&m_u.m_simple) SimpleT(); break;
      case task_type::serial:    new (&m_u.m_serial) SerialT(); break;
      case task_type::parallel:  new (&m_u.m_parallel) ParallelT; break;
      case task_type::intercept: new (&m_u.m_intercept) InterceptT; break;
    }
  }
  ~u_one_of()
  {
    switch (m_type)
    {
      case task_type::simple:    m_u.m_simple.~SimpleT(); break;
      case task_type::serial:    m_u.m_serial.~SerialT(); break;
      case task_type::parallel:  m_u.m_parallel.~ParallelT(); break;
      case task_type::intercept: m_u.m_intercept.~InterceptT(); break;
    }
  }

  // getters will throw cool::exception::bad_conversion if wrong content type
  SimpleT& simple()
  {
    if (m_type != task_type::simple)
      throw cool::exception::bad_conversion("wrong content type");
      return m_u.m_simple;
  }
  const SimpleT& simple() const
  {
    if (m_type != task_type::simple)
      throw cool::exception::bad_conversion("wrong content type");
    return m_u.m_simple;
  }
  SerialT& serial()
  {
    if (m_type != task_type::serial)
      throw cool::exception::bad_conversion("wrong content type");
    return m_u.m_serial;
  }
  const SerialT& serial() const
  {
    if (m_type != task_type::serial)
      throw cool::exception::bad_conversion("wrong content type");
    return m_u.m_serial;
  }
  ParallelT& parallel()
  {
    if (m_type != task_type::parallel)
      throw cool::exception::bad_conversion("wrong content type");
    return m_u.m_parallel;
  }
  const ParallelT& parallel() const
  {
    if (m_type != task_type::parallel)
      throw cool::exception::bad_conversion("wrong content type");
    return m_u.m_parallel;
  }
  InterceptT& intercept()
  {
    if (m_type != task_type::intercept)
      throw cool::exception::bad_conversion("wrong content type");
    return m_u.m_intercept;
  }
  const InterceptT& intercept() const
  {
    if (m_type != task_type::intercept)
      throw cool::exception::bad_conversion("wrong content type");
    return m_u.m_intercept;
  }

 private:
  const task_type m_type;
  union u_ {
    u_() : m_simple() { /* noop */ }
    ~u_()             { /* noop */ }
    SimpleT    m_simple;
    SerialT    m_serial;
    ParallelT  m_parallel;
    InterceptT m_intercept;
  } m_u;
};

struct taskinfo
{
  using one_of = u_one_of<simple::taskinfo, serial::taskinfo, parallel::taskinfo, intercept::taskinfo>;

  taskinfo(const runner::weak_ptr& r_, task_type t_) : m_runner(r_), m_info(t_)
  { /* noop */  }
  taskinfo(task_type t_) : m_info(t_)
  { /* noop */  }

  runner::weak_ptr m_runner;
  one_of           m_info;
};

struct context
{
  using one_of = u_one_of<simple::context, serial::context, parallel::context, intercept::context>;

  context(task_type t_) : m_ctx(t_)
  { /* noop */ }

  one_of       m_ctx;
  taskinfo_ptr m_info;
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
// Task wrappers
//
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

template <typename ResultT, typename TagT> class task_wrapper { };

// ----------------------------------------------------------------------------
//
// Task wrappers for simple tasks
//
template <typename ResultT>
class task_wrapper<ResultT, tag::simple> : public types<ResultT, void>
{
 public:
  using typename types<ResultT, void>::bound_user_callable;
  using typename types<ResultT, void>::result_reporter;

 public:
  static void ep(const runner::ptr& r_, context_ptr ctx_, const bound_user_callable& task_)
  {
    try {
      ResultT res = task_(r_);  // this is a call into user callable
      if (ctx_->m_ctx.simple().result() != nullptr)
        (*static_cast<result_reporter*>(ctx_->m_ctx.simple().result()))(res);
    }
    catch (...) {
      if (ctx_->m_ctx.simple().exception())
        ctx_->m_ctx.simple().exception()(std::current_exception());
    }
  }
};
template <>
class task_wrapper<void, tag::simple> : public types<void, void>
{
 public:
  using types<void, void>::bound_user_callable;

 public:
  static void ep(const runner::ptr& r_, context_ptr ctx_, const bound_user_callable& task_)
  {
    try {
      task_(r_);  // this is a call into user callable
    }
    catch (...) {
      ctx_->m_ctx.simple().exception()(std::current_exception());
    }
  }
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
// Task wrapper binders
//
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
template <typename ResultT, typename ParamT, typename TagT> class binder_factory { };

// ----------------------------------------------------------------------------
//
// Task wrapper binders for simple tasks
//
template <typename ResultT, typename ParamT>
class binder_factory<ResultT, ParamT, tag::simple> : public types<ResultT, ParamT>
{
 public:
  using typename types<ResultT, ParamT>::user_callable;
  using typename types<ResultT, ParamT>::binder_type_ptr;
  using typename types<ResultT, ParamT>::bound_user_callable;

 public:
  static bound_entry_point rebind(const taskinfo_ptr& ctx_, user_callable& func_, const ParamT& par_)
  {
    return bound_entry_point(std::bind(
        task_wrapper<ResultT, tag::simple>::ep
      , std::placeholders::_1
      , std::placeholders::_2
      , bound_user_callable(std::bind(func_, std::placeholders::_1, par_))
    ));
  }

  static void deleter(void* arg_)
  {
    delete static_cast<binder_type_ptr>(arg_);
  }
};

template <typename ResultT>
class binder_factory<ResultT, void, tag::simple> : public types<ResultT, void>
{
 public:
  using typename types<ResultT, void>::user_callable;
  using typename types<ResultT, void>::binder_type_ptr;
  using typename types<ResultT, void>::bound_user_callable;

 public:
  static bound_entry_point rebind(const taskinfo_ptr& ctx_, const user_callable& func_)
  {
    return bound_entry_point(std::bind(
        task_wrapper<ResultT, tag::simple>::ep
      , std::placeholders::_1
      , std::placeholders::_2
      , bound_user_callable(std::bind(func_, std::placeholders::_1))
    ));
  }

  static void deleter(void* arg_)
  {
    delete static_cast<binder_type_ptr>(arg_);
  }
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
// Task factory
//
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
template <typename ResultT, typename ParamT, typename TagT> class task_factory { };

// ----------------------------------------------------------------------------
//
// Task factories for simple tasks
//
template <typename ResultT, typename ParamT>
class task_factory<ResultT, ParamT, tag::simple> : public types<ResultT, ParamT>
{
 public:
  using typename types<ResultT, ParamT>::user_callable;
  using typename types<ResultT, ParamT>::entry_point;
  using typename types<ResultT, ParamT>::binder_type;

 public:
  static taskinfo_ptr create(const runner::weak_ptr& r_, const user_callable& f_)
  {
    using binder_factory = binder_factory<ResultT, ParamT, tag::simple>;

    auto info = std::make_shared<taskinfo>(r_, tag::simple::value);
    info->m_info.simple().unbound(
        new binder_type(
              std::bind(
                  binder_factory::rebind
                , std::placeholders::_1
                , f_
                , std::placeholders::_2
            )
        )
      , binder_factory::deleter
    );
    return info;
  }

  static context_ptr make_context(const taskinfo_ptr& t_, const ParamT& p_)
  {
    auto aux = static_cast<typename impl::types<ResultT, ParamT>::binder_type_ptr>(t_->m_info.simple().unbound());
    auto ctx = new context(tag::simple::value);
    ctx->m_ctx.simple().entry_point((*aux)(t_, p_));
    ctx->m_info = t_;
    return ctx;
  }
};

template <typename ResultT>
class task_factory<ResultT, void, tag::simple> : public types<ResultT, void>
{
 public:
  using typename types<ResultT, void>::user_callable;
  using typename types<ResultT, void>::entry_point;
  using typename types<ResultT, void>::binder_type;

  static taskinfo_ptr create(const runner::weak_ptr& r_, const user_callable& f_)
  {
    using binder_factory = binder_factory<ResultT, void, tag::simple>;

    auto info = std::make_shared<taskinfo>(r_, tag::simple::value);
    info->m_info.simple().unbound(
        new binder_type(
            std::bind(
                binder_factory::rebind
              , std::placeholders::_1
              , f_
            )
        )
      , binder_factory::deleter
    );
    return info;
  }

  static context_ptr make_context(const taskinfo_ptr& t_)
  {
    auto aux = static_cast<typename impl::types<ResultT, void>::binder_type_ptr>(t_->m_info.simple().unbound());
    auto ctx = new context(tag::simple::value);
    ctx->m_ctx.simple().entry_point((*aux)(t_));
    ctx->m_info = t_;
    return ctx;
  }
};

// ----------------------------------------------------------------------------
//
// Task factories for sequential tasks
//
template <typename ResultT, typename ParamT>
class task_factory<ResultT, ParamT, tag::serial>  : public types<ResultT, ParamT>
{
 public:
  using typename types<ResultT, ParamT>::entry_point;
  using typename types<ResultT, ParamT>::binder_type;

 public:
  template <typename ...TaskT>
  static taskinfo_ptr create(TaskT&&...tasks)
  {
    auto info = std::make_shared<taskinfo>(tag::serial::value);
    return info;

  }
};

} } } // namespace

#endif