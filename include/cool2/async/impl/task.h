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
enum class context_type { not_set, simple, parallel, serial, intercept };
namespace tag
{
struct simple    { static constexpr context_type value = context_type::simple;    };
struct serial    { static constexpr context_type value = context_type::serial;    };
struct parallel  { static constexpr context_type value = context_type::parallel;  };
struct intercept { static constexpr context_type value = context_type::intercept; };
} // namespace

struct context;
using context_ptr = std::shared_ptr<context>;
using bound_entry_point   = std::function<void(const runner::ptr&, const context_ptr&)>;

template <typename ResultT, typename ParamT> class types
{
 public:
  using user_callable       = std::function<ResultT(const runner::ptr&, const ParamT&)>;
  using bound_user_callable = std::function<ResultT(const std::shared_ptr<runner>&)>;
  using entry_point         = std::function<void(const runner::ptr&, const context_ptr&, const ParamT&)>;

  using binder_type         = std::function<bound_entry_point(const context_ptr&, const ParamT&)>;
  using binder_type_ptr     = binder_type*;
  using binder_result       = bound_entry_point;
};

template<typename ResultT> class types<ResultT, void>
{
 public:
  using user_callable       = std::function<ResultT(const runner::ptr&)>;
  using bound_user_callable = std::function<ResultT(const std::shared_ptr<runner>&)>;
  using entry_point         = std::function<void(const runner::ptr&, const context_ptr&)>;

  using binder_type         = std::function<bound_entry_point(const context_ptr&)>;
  using binder_type_ptr     = binder_type*;
  using binder_result       = bound_entry_point;
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
// Task context structures for different task types
//
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

namespace simple    { class context; }
namespace serial    { class context; }
namespace parallel  { class context; }
namespace intercept { class context; }


namespace simple
{

class context
{
  using cleaner_t = void(*)(void*);

 public:
  context() : m_unbound(nullptr)
  { /* noop */ }
  ~context()
  {
    if (m_unbound != nullptr)
      m_cleaner(m_unbound);
  }
  void unbound(void* ubnd_, const cleaner_t& clr_)
  {
    m_unbound = ubnd_;
    m_cleaner = clr_;
  };
  void* unbound()
  {
    return m_unbound;
  }
  void entry_point(const bound_entry_point& arg_)
  {
    if (m_unbound)
      m_cleaner(m_unbound);
    m_unbound = nullptr;
    m_ep = arg_;
  }
  bound_entry_point& entry_point()
  {
    return m_ep;
  }
  bool has_exception() const
  {
    return !!m_exception;
  }
  std::exception_ptr exception() const
  {
    return m_exception;
  }
  void exception(const std::exception_ptr& arg_)
  {
    m_exception = arg_;
  }
 private:
  bound_entry_point  m_ep;        // user callable with bound input parameter
  void*              m_unbound;   // binder for user callable to pass parameter to ep
  cleaner_t          m_cleaner;   // delete function to delete binder instance
  std::exception_ptr m_exception; // exception if thrown
};

} // namespace

namespace serial
{

class context
{
  using sequence_type = std::vector<context_ptr>;

 public:
  context(std::size_t set_size_) : m_sequence(set_size_) { /* noop */ }
  sequence_type& sequence()             { return m_sequence; }
  const sequence_type& sequence() const { return m_sequence; }

 private:
  sequence_type m_sequence;
};

}

class u_one_of
{
 public:
  u_one_of();
  ~u_one_of();

  void set(const std::shared_ptr<simple::context>& arg_);
  void set(const std::shared_ptr<serial::context>& arg_);
  void set(const std::shared_ptr<parallel::context>& arg_);
  void set(const std::shared_ptr<intercept::context>& arg_);

  // getters will throw cool::exception::bad_conversion if wrong content type
  std::shared_ptr<simple::context> simple();
  std::shared_ptr<serial::context> serial();
  std::shared_ptr<parallel::context> parallel();
  std::shared_ptr<intercept::context> intercept();

 private:
  void release();

 private:
  context_type m_type;
  union u_ {
    u_() : m_simple() { /* noop */ }
    ~u_()             { /* noop */ }
    std::shared_ptr<simple::context>    m_simple;
    std::shared_ptr<serial::context>    m_serial;
    std::shared_ptr<parallel::context>  m_parallel;
    std::shared_ptr<intercept::context> m_intercept;
  } m_u;
};

struct context
{
  template <typename ContextT>
  context(const runner::weak_ptr& r_, const std::shared_ptr<ContextT>& c_) : m_runner(r_)
  {
    m_ctx.set(c_);
  }
  template <typename ContextT>
  context(const std::shared_ptr<ContextT>& c_)
  {
    m_ctx.set(c_);
  }
  runner::weak_ptr m_runner;
  u_one_of         m_ctx;
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

 public:
  static void ep(const runner::ptr& r_, const context_ptr& ctx_, const bound_user_callable& task_)
  {
    try {
       ResultT res = task_(r_);  // this is a call into user callable
       // TODO: do something with result
    }
    catch (...) {
      ctx_->m_ctx.simple()->exception(std::current_exception());
    }
  }
};
template <>
class task_wrapper<void, tag::simple>
{
 public:
  using bound_user_callable = types<void, void>::bound_user_callable;

 public:
  static void ep(const runner::ptr& r_, const context_ptr& ctx_, const bound_user_callable& task_)
  {
    try {
       task_(r_);  // this is a call into user callable
    }
    catch (...) {
      ctx_->m_ctx.simple()->exception(std::current_exception());
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
  static bound_entry_point rebind(const context_ptr& ctx_, user_callable& func_, const ParamT& par_)
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
  static bound_entry_point rebind(const context_ptr& ctx_, const user_callable& func_)
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
  static context_ptr create(const runner::weak_ptr& r_, const user_callable& f_)
  {
    using binder_factory = binder_factory<ResultT, ParamT, tag::simple>;

    auto ctx = std::make_shared<context>(r_, std::make_shared<simple::context>());
    ctx->m_ctx.simple()->unbound(
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

  static context_ptr create(const runner::weak_ptr& r_, const user_callable& f_)
  {
    using binder_factory = binder_factory<ResultT, void, tag::simple>;

    auto ctx = std::make_shared<context>(r_, std::make_shared<simple::context>());
    ctx->m_ctx.simple()->unbound(
        new binder_type(
            std::bind(
                binder_factory::rebind
              , std::placeholders::_1
              , f_
            )
        )
      , binder_factory::deleter
    );
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
  static context_ptr create(TaskT&&...tasks)
  {
    auto ctx = std::make_shared<context>(std::make_shared<serial::context>(sizeof...(TaskT)));
    return ctx;

  }
};

} } } // namespace

#endif