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

#if !defined(ENTRAILS_RUNNER_H_HEADER_GUARD)
#define ENTRAILS_RUNNER_H_HEADER_GUARD

#include <functional>
#include <exception>
#include <cstdint>
#include <memory>

#include <dispatch/dispatch.h>

namespace cool { namespace gcd { namespace task {

#if defined(LINUX_TARGET)
#define DISPATCH_QUEUE_PRIORITY_HIGH        2
#define DISPATCH_QUEUE_PRIORITY_DEFAULT     0
#define DISPATCH_QUEUE_PRIORITY_LOW         (-2)
#define DISPATCH_QUEUE_PRIORITY_BACKGROUND  INT16_MIN

typedef long dispatch_queue_priority_t;
#endif

#if defined(WIN32_TARGET)
typedef long dispatch_queue_priority_t;
#endif

class runner;

namespace entrails {

using basis::vow;
using basis::aim;
using basis::named;

// ====== infrastructure for task model

using task_t = std::function<void(void)>;
template <typename T> using subtask_t = std::function<void(const T&)>;
template <typename T> using binder_t = std::function<task_t*(const T&)>;
using void_binder_t = std::function<task_t*()>;

using error_handler_t = std::function<void(const std::exception_ptr&)>;

struct taskinfo
{
  taskinfo(const std::weak_ptr<runner>& r)
    : m_runner(r)
    , m_next(nullptr)
    , m_prev(nullptr)
  {
    m_u.task = nullptr;
  }
  taskinfo(entrails::task_t* t, const std::weak_ptr<runner>& r)
      : m_runner(r)
      , m_next(nullptr)
      , m_prev(nullptr)
  {
    m_u.task = t;
  }
  dlldecl ~taskinfo();

  union {
    task_t* task;
    void*   subtask;
  }                         m_u;
  entrails::error_handler_t m_eh;
  std::function<void()>     m_deleter;   // deleter for subtask
  std::weak_ptr<runner>     m_runner;
  taskinfo*                 m_next;
  taskinfo*                 m_prev;
};

// --- cleanup stuff
dlldecl void cleanup(taskinfo* info_);
dlldecl void cleanup_reverse(taskinfo* info_);

// deleter for subtasks
template<typename T> void subtask_deleter(void* address)
{
  delete static_cast<T*>(address);
}

// ------------- Task execution ------------------------------------------

// ====== Executors for task class ======

dlldecl void kickstart(taskinfo* info_);
dlldecl void kickstart(taskinfo* info_, const std::exception_ptr& e_);


// Called from libdispatch - unwraps the bound object and calls one of below
// executor functions
dlldecl void task_executor(void* ctx);

// entry point entered from the tast_executor - receives parameters and,
// through template specialization, retains enough type information
// to interpret task call result, rebinds the next task from the sequence and
// schedules it for execution in selected runner
template <typename ResultT> class task_entry
{
 public:
  static void entry_point(taskinfo* info_, const std::function<ResultT()>& task_)
  {
    auto next = info_->m_next;
    try
    {
      ResultT result = task_();               // execute the current task
      if (next != nullptr && next->m_u.subtask != nullptr)
      {
        // bind result to partially bound subtask
        binder_t<ResultT>* subtask = static_cast<binder_t<ResultT>*>(next->m_u.subtask);
        next->m_u.task = (*subtask)(result);  // rebind
        delete subtask;
        next->m_deleter = task_t();           // disarm subtask deleter
        kickstart(next);                      // schedule next task
      }
    }
    catch (...)
    {
      if (next != nullptr && next->m_eh)
        kickstart(next, std::current_exception());
      cleanup(next);
    }

    delete info_;
  }
};

// specialization for void type
template<> class task_entry<void>
{
 public:
  static void entry_point(taskinfo* info_, const std::function<void()>& task_)
  {
    auto next = info_->m_next;
    try
    {
      task_();  // execute the current task
      if (next != nullptr && next->m_u.subtask != nullptr)
      {
        // no rebind necessary, just reassign and disarm deleter
        void_binder_t* subtask = static_cast<void_binder_t*>(next->m_u.subtask);
        next->m_u.task = (*subtask)();        // rebind
        delete subtask;
        next->m_deleter = task_t();           // disarm subtask deleter
        kickstart(next);                      // schedule next task
      }
    }
    catch (...)
    {
      if (next != nullptr && next->m_eh)
        kickstart(next, std::current_exception());
      cleanup(next);
    }

    delete info_;
  }
};

// ====== Executors for runner::run method ======

// Called from libdispatch - unwraps the bound object and calls one of below
// executor functions
dlldecl void executor(void* ctx);

// Executor for tasks returning value
#if defined(INCORRECT_VARIADIC)
template <typename Ret>
void execute_ret(vow<Ret>& v, const std::function<Ret()>& task)
{
  try {
    v.set(task());
  }
  catch (...)
  {
    v.set(std::current_exception());
  }
}

// Executor for tasks not returnig value
inline void execute_void(vow<void>& v, const std::function<void()>& task)
{
  try
  {
    task();
    v.set();
  }
  catch (...)
  {
    v.set(std::current_exception());
  }
}

#else

template <typename Ret, typename... Args>
void execute_ret(vow<Ret>& v, const std::function<Ret(const Args&...)>& task, Args&&... args)
{
  try
  {
    v.set(task(std::forward<Args>(args)...));
  }
  catch (...)
  {
    v.set(std::current_exception());
  }
}
// Executor for tasks not returnig value
template <typename... Args>
void execute_void(vow<void>& v, const std::function<void(const Args&...)>& task, Args&&... args)
{
  try {
    task(std::forward<Args>(args)...);
    v.set();
  }
  catch (...)
  {
    v.set(std::current_exception());
  }
}
#endif


// ---------------- Task preparation --------------------------------------

// ====== Rebinders for task::then methods

// The subtask is a partially bound call to subtask_binder::rebind,
// with the user Callable, user supplied parameters and infrastructural parameters
// already bound, but with a  placeholder left for the result of previous task.
// This result known only after the previous task executes. The call to rebind
// from task_entry::entry_point with the missing ressult will thus form a
// complete task_t compatible function which can be submitted as a next task.
template <typename TaskResultT, typename SubtaskResultT, typename Function
#if !defined(INCORRECT_VARIADC)
, typename... Args
#endif
> class subtask_binder
{
 public:
  static task_t* rebind(taskinfo* info_, const TaskResultT& res_, const Function& func_
#if !defined(INCORRECT_VARIADC)
      , Args&&... args_
#endif
  )
  {
    return new task_t(std::bind(
        task_entry<SubtaskResultT>::entry_point
      , info_
      , static_cast<std::function<SubtaskResultT()>>(
          std::bind(func_, res_
#if !defined(INCORRECT_VARIADC)
            , std::forward<Args>(args_)...
#endif
    ))));
  }
};

// specialization used from task<void>::then doesn't leave a placeholder since
// there will be no result available.
template <typename SubtaskResultT, typename Function
#if !defined(INCORRECT_VARIADC)
  , typename... Args
#endif
> class subtask_binder<void, SubtaskResultT, Function
#if !defined(INCORRECT_VARIADC)
  , Args...
#endif
>
{
 public:
  static task_t* rebind(taskinfo* info_, const Function& func_
#if !defined(INCORRECT_VARIADC)
      , Args&&... args_
#endif
  )
  {
    return new task_t(std::bind(
        task_entry<SubtaskResultT>::entry_point
      , info_
      , static_cast<std::function<SubtaskResultT()>>(
          std::bind(func_
#if !defined(INCORRECT_VARIADC)
            , std::forward<Args>(args_)...
#endif
    ))));
  }
};

// ====== Binders for runner::run() method
// Binds the correct executor_* with the task to execute and its parameters. Since
// template functions cannot be partially specialized, this has to be static method
// wrapped into template class. Two specializations are necessary, one for
// tasks returning value and the other for void tasks.


#if defined(INCORRECT_VARIADIC)
template<bool is_void, typename Function> class binder { };

template<typename Function> class binder<false, Function>
{
 public:
  static task_t* bind(vow<typename std::result_of<Function()>::type>&& v, Function&& task)
  {
    return new task_t(
      std::bind(
          entrails::execute_ret<typename std::result_of<Function()>::type>
        , std::forward<vow<typename std::result_of<Function()>::type>>(v)
        , std::forward<Function>(task)
      )
    );
  }
};

template<typename Function> class binder<true, Function>
{
 public:
  static task_t* bind(vow<void>&& v, Function&& task)
  {
    return new task_t(
      std::bind(
          entrails::execute_void
        , std::forward<vow<void>>(v)
        , std::forward<std::function<void()>>(task)
      )
    );
  }
};
#else
template<bool is_void, typename Function, typename... Args> class binder { };

template<typename Function, typename... Args> class binder<false, Function, Args...>
{
 public:
  static task_t* bind(vow<typename std::result_of<Function(Args...)>::type>&& v, Function&& task, Args&&... args)
  {
    return new task_t(
      std::bind(
          entrails::execute_ret<typename std::result_of<Function(Args...)>::type, Args...>
        , std::forward<vow<typename std::result_of<Function(Args...)>::type>>(v)
        , std::forward<Function>(task)
        , std::forward<Args>(args)...
      )
    );
  }
};


template<typename Function, typename... Args> class binder<true, Function, Args...>
{
public:
  static task_t* bind(vow<void>&& v, Function&& task, Args&&... args)
  {
    return new task_t(
      std::bind(
        entrails::execute_void<Args...>
        , std::forward<vow<void>>(v)
        , std::forward<std::function<void(Args&...)>>(task)
        , std::forward<Args>(args)...
      )
    );
  }
};
#endif

struct queue
{
  queue(dispatch_queue_t q, bool is_sys = false)
    : m_queue(q)
    , m_is_system(is_sys)
  { m_suspended = false;/* noop */ }

  ~queue()
  {
    if (!m_is_system)
      ::dispatch_release(m_queue);
  }

  dispatch_queue_t m_queue;
  std::atomic_bool m_suspended;
  const bool m_is_system;

};

} // namespace

} } } //namespace

#endif
