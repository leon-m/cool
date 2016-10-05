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

namespace cool { namespace async {

class runner;

namespace impl {

// ----- the actual callback function that is called from the task being executed
// ----- will receive no parameters and returns no value. Return value is
// ----- intercepted by the entry point and bound into new call
using task_callback_t = std::function<void()>;

struct u_callable
{
  using unbound_t = void*;
  using bound_t   = void(*)();
  using deletor_t = void(*)(unbound_t);

  enum value_t { Empty, Unbound, Bound };

  u_callable() : m_value_type(Empty) { /* noop */ }

  void unbound(void* unb_, const deletor_t& del_)
  {
    m_value_type = Unbound;
    u.m_unbound = unb_;
    m_deletor = del_;
  }

  value_t               m_value_type;
  union {
    void  (*m_ep)();
    void* m_unbound;
  } u;
  deletor_t             m_deletor;
  std::weak_ptr<runner> m_runner;
};

struct info
{
  info() : m_next(nullptr), m_prev(nullptr) { /* noop */ }

  u_callable m_callable;
  std::unique_ptr<info> m_next;
  info* m_prev;
};

// ---------------------------------------------------------------------
template <typename ResultT> class entry_point
{
 public:
  using callable_t = std::function<ResultT(const runner::ptr_t&)>;

 public:
  static void call(info* info_, const callable_t& task_)
  {
  }
};

// --- binder_factory ---------------------------------------------------------
//
// Binder binds together an entry point with its parameters, including
// task's Callable, leaving placeholder for the parameter value
template <typename ResultT, typename ParamT>
class binder_factory
{
  using callable_t    = std::function<ResultT(const runner::ptr_t&, const ParamT&)>;
  using entry_point_t = entry_point<ResultT>;

 public:
  using type = std::function<task_callback_t*(const ParamT&)>;

 public:
  static task_callback_t* rebind(info* info_, const callable_t& func_, const ParamT& res_)
  {
    return new task_callback_t(std::bind(
        entry_point<ResultT>::call
      , info_
      , static_cast<typename entry_point_t::callable_t>(std::bind(func_, std::placeholders::_1, res_)
    )));
  }

  static void delete_unbound(void *arg_)
  {
    delete static_cast<type*>(arg_);
  }
};

// --- partial specialization for task Callables with no parameters
template <typename ResultT>
class binder_factory<ResultT, void>
{
  using callable_t    = std::function<ResultT(const runner::ptr_t&)>;
  using entry_point_t = entry_point<ResultT>;

public:
  using type  = std::function<task_callback_t*()>;

public:
  static task_callback_t* rebind(info* info_, const callable_t& func_)
  {
    return new task_callback_t(std::bind(
        entry_point<ResultT>::call
      , info_
      , static_cast<typename entry_point_t::callable_t>(std::bind(func_, std::placeholders::_1)
    )));
  }
  
  static void delete_unbound(void *arg_)
  {
    delete static_cast<type*>(arg_);
  }

};

// --- task_factory ----------------------------------------------------------
// The task_factory template can be partially specialized for void parameter
// type (no parameter) so that taskop::create does not need special cases
// for void parameter type.
template <typename ResultT, typename ParamT>
class task_factory
{
 public:
  using task_callable_t = std::function<ResultT(const runner::ptr_t&, const ParamT&)>;

  static info* create(const runner::weak_ptr_t& r_, const task_callable_t& f_)
  {
    using binder_factory = typename impl::binder_factory<ResultT, ParamT>;

    auto inf = new impl::info();
    inf->m_callable.unbound(
        new typename binder_factory::type(
            std::bind(
                binder_factory::rebind
              , inf
              , f_
              , std::placeholders::_1
            )
        )
      , binder_factory::delete_unbound
    );
    return inf;
  }
};

// --- partial specialization for task Callables with no parameters
template <typename ResultT>
class task_factory<ResultT, void>
{
 public:
  using task_callable_t = std::function<ResultT(const runner::ptr_t&)>;

  static info* create(const runner::weak_ptr_t& r_, const task_callable_t& f_)
  {
    using binder_factory = typename impl::binder_factory<ResultT, void>;

    auto inf = new impl::info();
    inf->m_callable.unbound(
        new typename binder_factory::type(
            std::bind(
                binder_factory::rebind
              , inf
              , f_
            )
        )
      , binder_factory::delete_unbound
    );
    return inf;
  }

};

} } } // namespace

#endif