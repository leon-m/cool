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

#include "cool2/async/impl/runner.h"

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
// --
// --
enum class task_type { simple, parallel, serial, intercept };

namespace simple    { class taskinfo; class context; }
namespace serial    { class taskinfo; class context; }
namespace parallel  { class taskinfo; class context; }
namespace intercept { class taskinfo; class context; }

namespace tag
{
struct simple {
//  static const constexpr task_type value = task_type::simple;
  using taskinfo = impl::simple::taskinfo;
  using context  = impl::simple::context;
};
struct serial {
//  static const constexpr task_type value = task_type::serial;
  using taskinfo = impl::serial::taskinfo;
  using context  = impl::serial::context;
};
struct parallel {
//  static const constexpr task_type value = task_type::parallel;
  using taskinfo = impl::parallel::taskinfo;
  using context  = impl::parallel::context;
};
struct intercept {
//  static const constexpr task_type value = task_type::intercept;
  using taskinfo = impl::intercept::taskinfo;
  using context  = impl::intercept::context;
};
} // namespace

class taskinfo;

using taskinfo_ptr        = std::shared_ptr<taskinfo>;
using context_ptr         = execution_context*;
using bound_entry_point   = execution_context::entry_point;
using exception_reporter  = execution_context::exception_reporter;
using deleter_type        = execution_context::generic_deleter;
using reporter_creator    = void* (*)(context_ptr);

// -- types that depend on task template parameters
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
  using result_reporter     = std::function<void()>;
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
  using result_reporter     = std::function<void()>;
};

// ----- taskinfo and context structures for different task types
// --
// --

class taskinfo
{
 public:
  taskinfo(const runner::weak_ptr& r_) : m_runner(r_) { /* noop */ }
  virtual ~taskinfo() { /* noop */ }

  const runner::weak_ptr& get_runner() const { return m_runner; }

 private:
  runner::weak_ptr m_runner;
};

// ----- taskinfo and context structures for simple tasks
namespace simple
{

class taskinfo : public impl::taskinfo
{
 public:
  using ptr = std::shared_ptr<taskinfo>;

 public:
  taskinfo(const runner::weak_ptr& r_) : impl::taskinfo(r_), m_unbound(nullptr)
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
  void* unbound() { return m_unbound; }

 private:
  void*        m_unbound;   // binder for user callable to pass parameter to ep
  deleter_type m_cleaner;   // delete function to delete binder instance
};

class context : public execution_context
{
 public:
  context(const taskinfo::ptr& t_, const bound_entry_point& ep_) : execution_context(ep_), m_info(t_)
  { /* noop */ }

  const taskinfo::ptr& info() const                   { return m_info; }
  const runner::weak_ptr& get_runner() const override { return m_info->get_runner(); }

 private:
  taskinfo::ptr m_info;    // pointer to static task data
};

} // namespace

// ----- taskinfo and context structures for serial tasks
namespace serial
{

class taskinfo : public impl::taskinfo
{

 public:
  using ptr = std::shared_ptr<taskinfo>;

 public:
  taskinfo (const runner::weak_ptr& r_) : impl::taskinfo(r_) { /* noop */ }
  std::vector<taskinfo_ptr>& sequence()             { return m_sequence; }
  const std::vector<taskinfo_ptr>& sequence() const { return m_sequence; }
  std::vector<reporter_creator>& reporter_creators()             { return m_reporter_creators; }
  const std::vector<reporter_creator>& reporter_creators() const { return m_reporter_creators; }
  std::vector<deleter_type>& reporter_deleters()             { return m_reporter_deleters; }
  const std::vector<deleter_type>& reporter_deleters() const { return m_reporter_deleters; }

 private:
  std::vector<taskinfo_ptr>     m_sequence;
  std::vector<reporter_creator> m_reporter_creators;
  std::vector<deleter_type>     m_reporter_deleters;
};

class context : public execution_context
{
 public:
  using ptr = context*;

 public:
  context(const taskinfo::ptr& t_, const bound_entry_point& ep_) : execution_context(ep_), m_info(t_)
  { /* noop */ }
  template <typename T>
  void report_result(const T& res)
  {
    // TODO:
  }
  void report_void()
  {
    // TODO:
  }

private:
  taskinfo::ptr m_info;    // pointer to static task data
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

} // namespace

// ----- Task wrappers
// --
// -- Task wrappers provide an entry point called from the runner when the
// -- task begins the execution. The wrappers invoke the user Callable, fetch
// -- the result (if any) and report it to the enclosing context, if any.
// -- They also intercept and report any exception thrown by the user code, so
// -- no exception can escape into the runner's executor
// --

template <typename ResultT, typename TagT> class task_wrapper { };

// ----- Task wrappers for simple tasks with and without input parameter
// --
template <typename ResultT>
class task_wrapper<ResultT, tag::simple> : public types<ResultT, void>
{
 public:
  using typename types<ResultT, void>::bound_user_callable;
  using typename types<ResultT, void>::result_reporter;

 public:
  static void ep(const runner::ptr& r_, context_ptr ctx_, const bound_user_callable& task_)
  {
    auto ctx = static_cast<simple::context*>(ctx_);

    try {
      ResultT res = task_(r_);  // this is a call into the user callable
      if (ctx->get_result_reporter() != nullptr)
        (*static_cast<result_reporter*>(ctx->get_result_reporter()))(res);
    }
    catch (...) {
      if (ctx->get_exception_reporter())
        ctx->get_exception_reporter()(std::current_exception());
    }
    delete ctx_;
  }
};
// --
template <>
class task_wrapper<void, tag::simple> : public types<void, void>
{
 public:
  using types<void, void>::bound_user_callable;

 public:
  static void ep(const runner::ptr& r_, context_ptr ctx_, const bound_user_callable& task_)
  {
    auto ctx = static_cast<simple::context*>(ctx_);
    try {
      task_(r_);  // this is a call into the user callable
      if (ctx->get_result_reporter() != nullptr)
        (*static_cast<result_reporter*>(ctx->get_result_reporter()))();
    }
    catch (...) {
      if (ctx->get_exception_reporter())
        ctx->get_exception_reporter()(std::current_exception());
    }
    delete ctx;
  }
};

// ----- Task wrappers for serial tasks with and without input parameter
// --
template <typename ResultT>
class task_wrapper<ResultT, tag::serial> : public types<ResultT, void>
{
public:
  using typename types<ResultT, void>::bound_user_callable;
  using typename types<ResultT, void>::result_reporter;

public:
  static void ep(const runner::ptr& r_, context_ptr ctx_)
  {
    auto ctx = static_cast<serial::context*>(ctx_);

    try {
    }
    catch (...) {
      if (ctx->get_exception_reporter())
        ctx->get_exception_reporter()(std::current_exception());
    }
    delete ctx_;
  }
};
// --
template <>
class task_wrapper<void, tag::serial> : public types<void, void>
{
public:
  using types<void, void>::bound_user_callable;

public:
  static void ep(const runner::ptr& r_, context_ptr ctx_, const bound_user_callable& task_)
  {
    auto ctx = static_cast<serial::context*>(ctx_);
    try {
      task_(r_);  // this is a call into the user callable
      if (ctx->get_result_reporter() != nullptr)
        (*static_cast<result_reporter*>(ctx->get_result_reporter()))();
    }
    catch (...) {
      if (ctx->get_exception_reporter())
        ctx->get_exception_reporter()(std::current_exception());
    }
    delete ctx;
  }
};
  

// ----- Task wrapper binder class templates
// --
// -- NOTE: class templates are used instead of function templates as the
// -- latter do not supporta partial specialization.
// --
// -- Task wrapper binders bind the user Callable with their (optional) input
// -- parameter. Then they bind the resulting function with the entry point
// -- by the task wrapper. The resulting function is the bound entry point
// -- called from the runner's task execution, which will also provide the
// -- two missing parameters, the shared pointer to the runner and the
// -- pointer to the task's execution context.
// --
// -- Task wrapper's binders are called immediatelly before the task is sent
// -- to the runner. This can either be by the task's run() method or by
// -- the entry point of the compound task when it's scheduling the next task
// -- for the execution.
// --
template <typename ResultT, typename ParamT, typename TagT> class binder_factory { };

// ----- Task wrapper binder class templates for simple tasks
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

// ----- Result reporter factory class template
template <typename ResultT, typename TagT>
class reporter_factory : public types<ResultT, void>
{
  using typename types<ResultT, void>::result_reporter;

 public:
  static void* result_reporter_creator(context_ptr ctx_)
  {
    return new result_reporter(
      std::bind(&serial::context::report_result<ResultT>, static_cast<typename TagT::context*>(ctx_), std::placeholders::_1)
    );
  }

  static void reporter_deleter(void* ptr_)
  {
    delete static_cast<result_reporter*>(ptr_);
  }
};

template <typename TagT>
class reporter_factory<void, TagT> : public types<void, void>
{
  using typename types<void, void>::result_reporter;

public:
  static void* result_reporter_creator(context_ptr ctx_)
  {
    return new result_reporter(std::bind(&serial::context::report_void, static_cast<typename TagT::context*>(ctx_)));
  }

  static void reporter_deleter(void* ptr_)
  {
    delete static_cast<result_reporter*>(ptr_);
  }
};

// ----- Task factory class template
// --
// -- The class factory class teplate provides two methods:
// --
// -- create method creates the taskinfo structure which contains the task's
// -- static data. For simple tasks, this includes a binder function, whcih is
// -- used just prior scheduling the task for execution to bind together the
// -- user callable and its input parameter. The taskinfo structure is resuable
// -- an permits the same task to be executed several times.
// --
// -- create_context method which creates the task's execution context. The
// -- execution context is created just before the task is scheduled for
// -- execution and is a disposable object, thrown away once the execution of
// -- the task completes.
// --
template <typename ResultT, typename ParamT, typename TagT> class task_factory { };

// ----- Task factories for simple tasks
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

    auto info = std::make_shared<simple::taskinfo>(r_);
    info->unbound(
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

  static context_ptr create_context(const taskinfo_ptr& t_, const ParamT& p_)
  {
    auto info = std::dynamic_pointer_cast<simple::taskinfo>(t_);
    auto aux = static_cast<typename impl::types<ResultT, ParamT>::binder_type_ptr>(info->unbound());
    return new simple::context(info, (*aux)(t_, p_));
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

    auto info = std::make_shared<simple::taskinfo>(r_);
    info->unbound(
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

  static context_ptr create_context(const taskinfo_ptr& t_)
  {
    auto info = std::dynamic_pointer_cast<simple::taskinfo>(t_);
    auto aux = static_cast<typename impl::types<ResultT, void>::binder_type_ptr>(info->unbound());
    return new simple::context(info, (*aux)(t_));
  }
};

// ----- Task factories for sequential tasks

template <typename ResultT, typename ParamT>
class task_factory<ResultT, ParamT, tag::serial>  : public types<ResultT, ParamT>
{
 public:
  using typename types<ResultT, ParamT>::entry_point;
  using typename types<ResultT, ParamT>::binder_type;
  using typename types<ResultT, ParamT>::result_reporter;

 public:
  template <typename ...TaskT>
  static taskinfo_ptr create(const runner::weak_ptr& r_, TaskT&&...tasks)
  {
    auto info = std::make_shared<serial::taskinfo>(r_);
    info->sequence() = { tasks.m_impl... };
    info->reporter_creators() = {
      &reporter_factory<typename std::decay<TaskT>::type::result_t, tag::serial>::result_reporter_creator...
    };
    info->reporter_deleters() = {
      &reporter_factory<typename std::decay<TaskT>::type::result_t, tag::serial>::reporter_deleter...
    };
    return info;
  }
#if 0
  static context_ptr create_context(const taskinfo_ptr& t_)
  {
    auto info = std::dynamic_pointer_cast<serial::taskinfo>(t_);
    return new serial::context(
  }
#endif
};


} } } // namespace

#endif