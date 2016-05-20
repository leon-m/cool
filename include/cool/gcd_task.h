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

#if !defined(RUNNER_H_HEADER_GUARD)
#define RUNNER_H_HEADER_GUARD

#include <functional>
#include <exception>
#include <atomic>
#include <memory>
#include <dispatch/dispatch.h>

#include "entrails/platform.h"
#include "vow.h"
#include "miscellaneous.h"
#include "entrails/gcd_task.h"

namespace cool { namespace gcd {

namespace async {
  class timer;
  class data_observer;
#if !defined(WIN32_TARGET)
  class signal;
  class reader;
  class writer;
  class fs_observer;
#if !defined(LINUX_TARGET)
  class proc_observer;
#endif
#endif
}

/**
 * Namespace containing GCD's task queue abstractions.
 *
 */
namespace task {

class group;


using basis::vow;
using basis::aim;
using basis::named;

class runner;

/**
 * Exception type thrown by the task library if the @ref cool::gcd::task::runner "runner"
 * that was supposed to run the task is no longer available. This exception is
 * passed to the error handler, if one was specified, and the error handler
 * is scheduled to run if possible (if its runner is not the one that is no
 * longer available).
 */
class runner_not_available : public cool::exception::runtime_exception
{
 public:
  runner_not_available()
      : runtime_exception("the destination runner not available")
  { /* noop */ }
};

/**
 * A class representing the queue of asynchronously executing tasks.
 *
 * This class is a higher level abstraction build on the top of the task queue
 * of the Grand Central Dispatch library (libdispatch). It represents a
 * single task queue the tasks from which are executed by the pool of threads
 * managed by the system.
 *
 * Depending on the construction parameter, the runner object may execute the
 * tasks sequentially in the FIFO order, or can attempt to execute the tasks
 * as concurrently as possible. In the latter case the sequence of task
 * completions is not defined as tasks may or may not be run concurrently by
 * several threads.
 *
 * The runner is capable of accepting any C++ Callable object as a task, and
 * will create a cool::basis::aim object as a mechanism for the task submitter
 * to either be notified about, or to collect the result of each task.
 *
 * @note The runner objects normally represent independent task queues. However,
 *   the runner objects created using copy construction or copy assignment
 *   operator are considered clones and represent the same task queue.
 *
 * <b>Thread Safety</b><br>
 * Although the main use model is not multi-thread scenario, the runner objects
 * are thread safe.
 *
 * <b>Platform Availability</b><br>
 * This class is available on Mac OS/X, Linux and Microsoft Windows, with the
 * following exceptions:
 *  - queue priority <tt>DISPATCH_QUEUE_PRIORITY_BACKGROUND</tt> is not available
 *    on Microsoft Windows.
 */
class runner : public named
{
 public:
  /**
   * Construct a new runner object.
   *
   * Constructs a new runner object with the desired task execution order. The
   * <i>run_order</i> parameter can have the following values:
   *   - <i>DISPATCH_QUEUE_SERIAL</i>; the tasks will be executed sequentially
   *     in FIFO order
   *   - <i>DISPATCH_QUEUE_CONCURRENT</i>; the tasks will start executing in
   *     FIFO order, but the order of their completion is not defined. The tasks
   *     may or may not be executed by multiple threads.
   *
   * @param run_order DISPATCH_QUEUE_SERIAL for sequential task execution or
   *    DISPATCH_QUEUE_CONCURRENT for concurrent execution. The default value
   *    is DISPATCH_QUEUE_SERIAL
   *
   * @exception cool::exception::create_failure thrown if a new instance cannot
   *   be created.
   *
   * @note The runner object is created in started state and is immediately
   *   capable of executing tasks.
   *
   * <b>Portability</b><br>
   * This constructor is only available on OS/X operating system. Only the
   * default constructor is available on other platforms and the only supported
   * execution order for user runners is sequential.
   */
#if defined(APPLE_TARGET)
  runner(dispatch_queue_attr_t run_order = DISPATCH_QUEUE_SERIAL);
#else
  dlldecl runner();
#endif
  dlldecl runner(const runner&) = default;
  dlldecl runner& operator=(const runner&) = default;
  runner(runner&&) = delete;
  runner& operator=(runner&&) = delete;
  /**
   * Destroys the runner object.
   *
   * @note Note that before being destroyed the stopped runner is restarted. The
   *   execution of all tasks waiting in the runner's queue will continue even
   *   after the runner object destruction and will cease only after the completion
   *   of the last task.
   */
  dlldecl ~runner();
  /**
   * Stop executing the tasks from this runner's queue.
   *
   * Currently executing tasks from this runner queue are executed to their
   * completion but new tasks are no longer scheduled for execution.
   * Note that suspending the execution will affect all clones that share the
   * same task queue.
   */
  dlldecl void stop();
  /**
   * Resume execution of tasks from this runner's queue.
   *
   * Note that resuming the execution will affect all clones that share the
   * same task queue.
   */
  dlldecl void start();
  /**
   * Stop accepting tasks.
   *
   * After this call the runner will stop accepting new tasks, and the run()
   * method will throw cool::exception::illegal_state.
   *
   * @note
   * Unlike start/stop, close() only affects this runner instance and does not
   * affect clones. Also note that stop() is irreversible and the runner, once
   * closed, cannot be reopened.
   */
  void close() { m_active = false; }
  /**
   * Accept the task for asynchronous execution.
   *
   * This function template accepts arbitrary Callable object and its parameters
   * and submits it to the task queue for asynchronous execution. The provided
   * parameters are passed to the Callable object when it starts executing.
   *
   * @param task Callable object to be executed
   * @param args A list of zero or more parameters to be passed to the Callable
   *             object when it starts executing.
   * @return cool::basis::aim object of the type instantiated with the type of
   *     the return value of the Callable object, or void if there is no
   *     return value.
   * @exception cool::exception::illegal_state thrown if this runner was closed.
   *
   * @warning
   *   The cool::basis::aim object returned by this method is meant for thread
   *   synchronization in multi-threading programming model. As such it refers
   *   to a shared state which is shared with the associated cool::basis::vow.
   *   The shared state is guarded by internal @c std::mutex. The thread
   *   safety may incur an unnecessary overhead in pure asynchronous programming
   *   model. When multi-thread synchronization is not an issue, use
   *   @ref cool::gcd::task::task "task" approach instead.
   *
   * @note
   *   Due to incorrect handling of empty parameter packs for variadic templates,
   *   the Callable object may not accept parameters when used in Microsoft
   *   Visual Studio 2013.
   */
#if defined(INCORRECT_VARIADIC)
  template <typename Function>
  aim<typename std::result_of<Function()>::type> run(Function&& task) const
  {
    if (!m_active)
      throw cool::exception::illegal_state("this runner is closed");

    vow<typename std::result_of<Function()>::type> v;
    auto a = v.get_aim();

    void *ctx = static_cast<void*>(
      entrails::binder<
        std::is_same<typename std::result_of<Function()>::type, void>::value
        , Function
      >::bind(
        std::move(v)
        , std::forward<Function>(task)
      )
    );

    ::dispatch_async_f(m_data->m_queue, ctx, entrails::executor);

    return a;
  }
#else
  template <typename Function, typename... Args>
  aim<typename std::result_of<Function(Args...)>::type> run(Function&& task, Args&&... args) const
  {
    if (!m_active)
      throw cool::exception::illegal_state("this runner is closed");

    vow<typename std::result_of<Function(Args...)>::type> v;
    auto a = v.get_aim();

    void *ctx = static_cast<void*>(
            entrails::binder<
                std::is_same<typename std::result_of<Function(Args...)>::type, void>::value
              , Function
              , Args...
            >::bind(
                std::move(v)
              , std::forward<Function>(task)
              , std::forward<Args>(args)...)
    );

    ::dispatch_async_f(m_data->m_queue, ctx, entrails::executor);

    return a;
  }
#endif
  /**
   * Returns system-wide runner object with the high priority.
   *
   * @note This runner may execute tasks concurrently.
   */
  dlldecl static const runner& sys_high();
  /**
   * Returns system-wide runner object with the high priority.
   *
   * @note This runner may execute tasks concurrently.
   */
  dlldecl static std::shared_ptr<runner> sys_high_ptr();
  /**
   * Returns system-wide runner object with the default priority.
   *
   * @note This runner may execute tasks concurrently.
   */
  dlldecl static const runner& sys_default();
  /**
   * Returns system-wide runner object with the default priority.
   *
   * @note This runner may execute tasks concurrently.
   */
  dlldecl static std::shared_ptr<runner> sys_default_ptr();
  /**
   * Returns system-wide runner object with the low priority.
   *
   * @note This runner may execute tasks concurrently.
   */
  dlldecl static const runner& sys_low();
  /**
   * Returns system-wide runner object with the low priority.
   *
   * @note This runner may execute tasks concurrently.
   */
  dlldecl static std::shared_ptr<runner> sys_low_ptr();
  /**
   * Returns system-wide runner object with the background (lowest) priority.
   *
   * @note This runner may execute tasks concurrently.
   *
   * <b>Platform Availability</b><br>
   * This method is available on Mac OS/X and Linux.
   */
#if !defined(WIN32_TARGET)
  dlldecl static const runner& sys_background();
#endif
  /**
   * Returns system-wide runner object with the background (lowest) priority.
   *
   * @note This runner may execute tasks concurrently.
   *
   * <b>Platform Availability</b><br>
   * This method is available on Mac OS/X and Linux.
   */
#if !defined(WIN32_TARGET)
  dlldecl static std::shared_ptr<runner> sys_background_ptr();
#endif
  /**
   * Returns library default runner.
   *
   * @note This runner executes tasks sequentially.
   */
  dlldecl static const runner& cool_default();
  /**
   * Returns library default runner.
   *
   * @note This runner executes tasks sequentially.
   */
  dlldecl static std::shared_ptr<runner> cool_default_ptr();

 private:
  friend class cool::gcd::async::timer;
  friend class cool::gcd::async::data_observer;
#if !defined(WIN32_TARGET)
  friend class cool::gcd::async::signal;
  friend class cool::gcd::async::reader;
  friend class cool::gcd::async::writer;
  friend class cool::gcd::async::fs_observer;
#if !defined(LINUX_TARGET)
  friend class cool::gcd::async::proc_observer;
#endif
#endif
  friend class group;
  operator const dispatch_queue_t() const { return m_data->m_queue; }
  operator dispatch_queue_t () { return m_data->m_queue; }

  friend void entrails::kickstart(entrails::taskinfo*);
  void task_run(entrails::taskinfo* info_);
  friend void entrails::kickstart(entrails::taskinfo* info_, const std::exception_ptr& e_);
  void task_run(entrails::task_t* task_);

 protected:
  dlldecl runner(const std::string& name, dispatch_queue_priority_t priority);

 private:
  bool                             m_active;
  std::shared_ptr<entrails::queue> m_data;
};

/**
 * A class representing the task, or a sequence of tasks, to be synchronously
 * executed in one or more task queues.
 *
 * The task interface represents an alternative to the runner::run() method which
 * is more suitable for the asynchronous processing model. While both models
 * provide the means for sequential execution of tasks and transferring the
 * results (return values) of preceding task to the next task, the task
 * interface provides the following advantages over the runner::run() method:
 *
 * - the runner::run() produced a pair of @ref cool::basis::aim "aim"/@ref cool::basis::vow "vow"
 *   objects for each tun task. In addition, sequencing the tasks via one of
 *   @ref cool::basis::aim::then() "aim::then()" methods produced another pair for each task
 *   added to the sequence. And since these objects were primarily designed to
 *   support multi-threading model, each such pair shares a shared state guarded
 *   by std::mutex, which unnecessarily slows down the asynchronous processing.
 * - the @ref cool::basis::aim "aim" offered no guaranties about which
 *   @ref cool::gcd::task::runner "runner" will execute the tasks supplied via
 *   @ref cool::basis::aim::then() "aim::then()" method. Depending on
 *   the availability of the results it could have been the runner executing the
 *   first task, or the runner calling runner::run method. The task interface
 *   not only guarantees which runner will execute the next task in the
 *   sequence but also allows the user code to select a different runner than
 *   the default.
 *
 * The task objects are not copyable but are movable. They cannot be constructed
 * directly but are created either by @ref cool::gcd::task::factory "task factory"
 * or by one of the task::then() method templates.
 *
 * @exception cool::gcd::task::runner_not_available thrown when the tasks are
 *   to be submitted for execution to their respective
 *   @ref cool::gcd::task::runner "runner" task queues, but the destination runner
 *   no longer exists. In this case, the task library will throw this exception,
 *   which will be passed to the error handler provided through one of the
 *   @ref task::then() "then()" methods. Note however, that if the error handler
 *   was to be run on the same runner that is no longer available, the error
 *   handler will not be scheduled to run and this exception will disappear
 *   unnoticed. This exception is thrown during the task sequence execution and is
 *   asynchronous with regard to the code that constructed and manipulated the
 *   task object(s).
 *
 * @exception cool::exception::illegal_state thrown immediately by task methods
 *   if the task object on which the method was tried is no longer valid. This
 *   exception is synchronous with regard to the caller of the method.
 *
 *
 * <b>Portability and Limitations</b><br>
 * The task interface is available on Max OS/X using Xcode 7 or later,
 * Linux using gcc 5.0 or later, and Microsoft Windows using
 * Visual Studio 2013 or later.
 *
 * The following are limitations applicable to Microsoft Windows:
 *
 * 1. when using Visual Studio 2013:
 *   - user Callable objects may not accept parameters, except for error
 *     handlers which must be of @ref error_handler_t compatible type,
 *     and the mandatory first parameter if preceding task returns value.
 *
 * 2. when using Visual Studio 2015:
 *   - user Callable objects may not accept parameters, except for error
 *     handlers which must be of @ref error_handler_t compatible type,
 *     and the mandatory first parameter if preceding task returns value.
 */
template <typename Result> class task
{
 public:
 /**
  * User provided error handler type. This type must be convertible to
  * @c std::function<void(const std::exception_ptr&)> function type.
  */
  using error_handler_t  = entrails::error_handler_t;
 /**
  * Return value of this task
  */
  using result_type = Result;

 public:
  task()                       = delete;
  task(const task&)            = delete;
  void operator =(const task&) = delete;
  task(task&& other)
  {
    m_info = other.m_info;
    other.m_info = nullptr;
  }
  task& operator =(task&& other)
  {
    m_info = other.m_info;
    other.m_info = nullptr;
    return *this;
  }
  ~task()
  {
    if (m_info != nullptr)
      entrails::cleanup_reverse(m_info);
  }

  /**
   * Adds a new task to the sequence and returns it.
   *
   * This method template adds a new task to be scheduled for execution upon
   * the completion of this task. The new task is passed the return value
   * of this task as its first parameter. The template parameters,
   * auto-deducted by compiler's template parameter deduction rules that apply
   * to function templates, are the following:
   * @tparam Function the function type of the user supplied task (Callable object).
   *    If the return value type of this task is non-void, the Callable's first
   *    argument must be @c const @c ResultT&, where ResultT is the type of the
   *    return value of this task's Callable.
   * @tparam Args... the template parameter pack of additional arguments passed to
   *    the user supplied Callable, after the optional first argument.
   *
   * The method must be provided with the following parameters:
   * @param err_ the error handler to be called if this task throws an exception
   *    during its execution.
   * @param func_ the user supplied Callable to be scheduled for execution upon
   *    the successful completion of this task.
   * @param args_ additional arguments to be passed to the user provided
   *    Callable when it begins the execution. Note that the additional arguments
   *    are passed after the first argument, if the current task's return value
   *    is non-void, or as the first, second, etc. argument if it is void.
   *
   * @return a new task object, which is to be used from this point on instead
   *   of the current task object.
   *
   * If the current task throws an exception during its execution, the task
   * library will schedule an error handler (@c err_) for the execution and will not
   * schedule the user provided Callable @c func_ . If the current task does not
   * throw an exception, the task library will schedule @c func_ for execution,
   * optionally passing it a return value of the current task (if non-void). Either
   * @c err_ or @c func_ will be scheduled to run on the same @ref cool::gcd::task::runner "runner"
   * that ran the current task.
   *
   * @note This method invalidates the current (@c this) object, and returns
   *   a new task object. All further operations must be performed on a new
   *   object.
   *
   * @note Note that since all @ref then() methods invalidate the current and
   *   return a new task object, the used runner will the last runner explicitly
   *   passed to any preceding task through @ref then(), or the runner specified
   *   at @ref factory::create() if none.
   *
   * @warning The error handler @c err is scheduled for the execution
   *   if the preceding task threw an exception. Scheduling the error handler
   *   breaks the task sequence and none of the tasks following the task that
   *   threw an exception will be scheduled to run.
   */
#if defined(INCORRECT_VARIADIC)
  template <typename Function>
  task<typename std::result_of<Function(const Result&)>::type>
  then(const error_handler_t& err_, Function&& func_)
  {
    if (m_info == nullptr)
      throw cool::exception::illegal_state("this task object is in undefined state");
    return then(m_info->m_runner, err_, std::forward<Function>(func_));
  }
#else
  template <typename Function, typename... Args >
  task<typename std::result_of<Function(const Result&, Args...)>::type>
  then(const error_handler_t& err_, Function&& func_, Args&&... args_)
  {
    if (m_info == nullptr)
      throw cool::exception::illegal_state("this task object is in undefined state");
    return then(m_info->m_runner, err_, std::forward<Function>(func_), std::forward<Args>(args_)...);
  }
#endif
  /**
   * Adds a new task to the sequence and returns it.
   *
   * This method template adds a new task to be scheduled for execution upon
   * the completion of this task. The new task is passed the return value
   * of this task as its first parameter. The template parameters,
   * auto-deducted by compiler's template parameter deduction rules that apply
   * to function templates, are the following:
   * @tparam Function the function type of the user supplied task (Callable object).
   *    If the return value type of this task is non-void, the Callable's first
   *    argument must be @c const @c ResultT&, where ResultT is the type of the
   *    return value of this task's Callable.
   * @tparam Args... the template parameter pack of additional arguments passed to
   *    the user supplied Callable, after the optional first argument.
   *
   * The method must be provided with the following parameters:
   * @param runner_ the @ref cool::gcd::task::runner "runner" to run either
   *    the error handler (@c err_) or @c func_ Callable.
   * @param err_ the error handler to be called if this task throws an exception
   *    during its execution.
   * @param func_ the user supplied Callable to be scheduled for execution upon
   *    the successful completion of this task.
   * @param args_ additional arguments to be passed to the user provided
   *    Callable when it begins the execution. Note that the additional arguments
   *    are passed after the first argument, if the current task's return value
   *    is non-void, or as the first, second, etc. argument if it is void.
   *
   * @return a new task object, which is to be used from this point on instead
   *   of the current task object.
   *
   * If the current task throws an exception during its execution, the task
   * library will schedule an error handler (@c err_) for the execution and will not
   * schedule the user provided Callable @c func_ . If the current task does not
   * throw an exception, the task library will schedule @c func_ for execution,
   * optionally passing it a return value of the current task (if non-void). Either
   * @c err_ or @c func_ will be scheduled to run on the @ref cool::gcd::task::runner "runner"
   * specified by parameter @c runner_ .
   *
   * @note This method invalidates the current (@c this) object, and returns
   *   a new task object. All further operations must be performed on a new
   *   object.
   *
   * @warning The error handler @c err is scheduled for the execution
   *   if the preceding task threw an exception. Scheduling the error handler
   *   breaks the task sequence and none of the tasks following the task that
   *   threw an exception will be scheduled to run.
   */
#if defined(INCORRECT_VARIADIC)
  template <typename Function>
  task<typename std::result_of<Function(const Result&)>::type>
  then(const std::weak_ptr<runner>& runner_, const error_handler_t &err_, Function &&func_)
#else
  template <typename Function, typename... Args>
  task<typename std::result_of<Function(const Result&, Args...)>::type>
  then(const std::weak_ptr<runner>& runner_, const error_handler_t &err_, Function &&func_, Args&&... args_)
#endif
  {
#if defined(INCORRECT_VARIADIC)
    using subtask_result_t = typename std::result_of<Function(const Result&)>::type;
#else
    using subtask_result_t = typename std::result_of<Function(const Result&, Args...)>::type;
#endif
    using subtask_t = std::function<entrails::task_t*(const Result&)>;

    if (m_info == nullptr)
      throw cool::exception::illegal_state("this task object is in undefined state");

    entrails::taskinfo* aux = new entrails::taskinfo(runner_);
    aux->m_eh = err_;
    m_info->m_next = aux;  // double link
    aux->m_prev = m_info;

    aux->m_u.subtask = new subtask_t(std::bind(
            entrails::subtask_binder<Result, subtask_result_t, Function
#if !defined(INCORRECT_VARIADIC)
          , Args...
#endif
            >::rebind
          , aux
          , std::placeholders::_1
          , std::forward<Function>(func_)
#if !defined(INCORRECT_VARIADIC)
          , std::forward<Args>(args_)...
#endif
    ));

    aux->m_deleter = std::bind(entrails::subtask_deleter<subtask_t>, aux->m_u.subtask);
    m_info = nullptr;     // invalidate state of current task

    return task<subtask_result_t>(aux);
  }

  /**
   * Specifies the error handling task for the current task.
   *
   * The error handler for the current task is a new task that is run only
   * if the current task throws an exception during its execution. The error
   * handling task is run by the same runner which executed the current
   * task.
   *
   * @note This method invalidates the current (@c this) object, and returns
   *   a new task object. All further operations must be performed on a new
   *   object.
   * @note Note that since all @ref then() methods invalidate the current and
   *   return a new task object, the used runner will the last runner explicitly
   *   passed to any preceding task through @ref then(), or the runner specified
   *   at @ref factory::create() if none.
   *
   * @note This method, if used, finalizes the task sequence. Although it is
   *   technically possible to use @ref then() on the task returned by this
   *   method, this and any subsequent tasks would never get run.
   */
  task finally(const error_handler_t& err_)
  {
    if (m_info == nullptr)
      throw cool::exception::illegal_state("this task object is in undefined state");
    return finally(m_info->m_runner, err_);
  }

  /**
   * Specifies the error handling task for the current task.
   *
   * The error handler for the current task is a new task that is run only
   * if the current task throws an exception during its execution. The error
   * handling task is run by the runner specified by @c runner_ parameter.
   *
   * @note This method invalidates the current (@c this) object, and returns
   *   a new task object. All further operations must be performed on a new
   *   object.
   *
   * @note This method, if used, finalizes the task sequence. Although it is
   *   technically possible to use @ref then() on the task returned by this
   *   method, this and any subsequent tasks would never get run.
   */
  task finally(const std::weak_ptr<runner>& runner_, const error_handler_t& err_)
  {
    if (m_info == nullptr)
      throw cool::exception::illegal_state("this task object is in undefined state");

    entrails::taskinfo* aux = new entrails::taskinfo(runner_);
    m_info->m_next = aux;

    aux->m_eh = err_;
    aux->m_prev = m_info;
    m_info = aux;

    return task(std::move(*this));
  }

  /**
   * Submits a task, or a sequence of tasks into the @ref cool::gcd::task::runner "runner"'s
   * task queue(s) for execution.
   *
   * This method schedules the execution of the task, or the sequence of tasks
   * into one of more task queues for execution. The tasks are submitted in
   * the sequence from the first to the last, the next task submitted only
   * upon the completion of the preceding task. Note that this is an
   * asynchronous operation; the run() method returns immediately, most
   * likely before the first task commences its execution.
   *
   * @warning This method invalidates the task object. No operations on the task
   *   object, except its destruction, are possible after this method returns.
   */
  void run()
  {
    if (m_info == nullptr)
      throw cool::exception::illegal_state("this task object is in undefined state");

    auto ptr = m_info;
    m_info = nullptr;  // prevent double delete

    for ( ; ptr->m_prev != nullptr; ptr = ptr->m_prev)
    ;
    entrails::kickstart(ptr);
  }

 private:
  friend class factory;

  template <typename Function
#if !defined(INCORRECT_VARIADIC)
  , typename... Args
#endif
  > task(const std::weak_ptr<runner>& runner_, Function&& func_
#if !defined(INCORRECT_VARIADIC)
      , Args&&... args_
#endif
  )
  {
    m_info = new entrails::taskinfo(runner_);
    m_info->m_u.task = new entrails::task_t(std::bind(
        entrails::task_entry<Result>::entry_point
      , m_info
      , static_cast<std::function<Result()>>(
          std::bind(
              std::forward<Function>(func_)
#if !defined(INCORRECT_VARIADIC)
            , std::forward<Args>(args_)...
#endif
          )
        )
    ));
  }

  template <typename T> friend class task;
  task(entrails::taskinfo* info) : m_info(info)
  { /* noop */ }

 private:
  entrails::taskinfo* m_info;
};

/**
 * Specialization of task class template for @c void Callable objects.
 *
 * This specialization is used for user Callable objects which do not return
 * value. Its methods are the same as those of @ref cool::gcd::task::task "task"
 * class template.
 *
 * @see @ref cool::gcd::task::task "task" class template.
 */
template <> class task<void>
{
 public:
  using error_handler_t  = entrails::error_handler_t;
  using result_type      = void;

 public:
  task()                       = delete;
  task(const task&)            = delete;
  void operator =(const task&) = delete;
  task(task&& other)
  {
    m_info = other.m_info;
    other.m_info = nullptr;
  }
  task& operator =(task&& other)
  {
    m_info = other.m_info;
    other.m_info = nullptr;
    return *this;
  }
  ~task()
  {
    if (m_info != nullptr)
      entrails::cleanup_reverse(m_info);
  }

#if defined(INCORRECT_VARIADIC)
  template <typename Function>
  task<typename std::result_of<Function()>::type>
  then(const error_handler_t& err_, Function&& func_)
  {
    if (m_info == nullptr)
      throw cool::exception::illegal_state("this task object is in undefined state");
    return then(m_info->m_runner, err_, std::forward<Function>(func_));
  }
#else
  template <typename Function, typename... Args >
  task<typename std::result_of<Function(Args...)>::type>
  then(const error_handler_t& err_, Function&& func_, Args&&... args_)
  {
    if (m_info == nullptr)
      throw cool::exception::illegal_state("this task object is in undefined state");
    return then(m_info->m_runner, err_, std::forward<Function>(func_), std::forward<Args>(args_)...);
  }
#endif


#if defined(INCORRECT_VARIADIC)
  template <typename Function>
  task<typename std::result_of<Function()>::type>
  then(const std::weak_ptr<runner>& runner_, const error_handler_t& err_, Function&& func_)
#else
  template <typename Function, typename... Args >
  task<typename std::result_of<Function(Args...)>::type>
  then(const std::weak_ptr<runner>& runner_, const error_handler_t& err_, Function&& func_, Args&&... args_)
#endif
  {
#if defined(INCORRECT_VARIADIC)
    using subtask_result_t = typename std::result_of<Function()>::type;
#else
    using subtask_result_t = typename std::result_of<Function(Args...)>::type;
#endif
    using subtask_t = std::function<entrails::task_t*()>;

    if (m_info == nullptr)
      throw cool::exception::illegal_state("this task object is in undefined state");

    entrails::taskinfo* aux = new entrails::taskinfo(runner_);
    aux->m_eh = err_;
    m_info->m_next = aux;  // double link
    aux->m_prev = m_info;

    aux->m_u.subtask = new subtask_t(std::bind(
            entrails::subtask_binder<void, subtask_result_t, Function
#if !defined(INCORRECT_VARIADIC)
          , Args...
#endif
            >::rebind
          , aux
          , std::forward<Function>(func_)
#if !defined(INCORRECT_VARIADIC)
          , std::forward<Args>(args_)...
#endif
    ));

    aux->m_deleter = std::bind(entrails::subtask_deleter<subtask_t>, aux->m_u.subtask);
    m_info = nullptr;     // invalidate state of current task

    return task<subtask_result_t>(aux);

  }


  task finally(const error_handler_t& err_)
  {
    if (m_info == nullptr)
      throw cool::exception::illegal_state("this task object is in undefined state");
    return finally(m_info->m_runner, err_);
  }

  task finally(const std::weak_ptr<runner>& runner_, const error_handler_t& err_)
  {
    if (m_info == nullptr)
      throw cool::exception::illegal_state("this task object is in undefined state");

    entrails::taskinfo* aux = new entrails::taskinfo(runner_);
    m_info->m_next = aux;

    aux->m_eh = err_;
    aux->m_prev = m_info;

    return task(std::move(*this));
  }

  void run()
  {
    if (m_info == nullptr)
      throw cool::exception::illegal_state("this task object is in undefined state");

    auto ptr = m_info;
    m_info = nullptr;  // prevent double delete

    for ( ; ptr->m_prev != nullptr; ptr = ptr->m_prev)
    ;
    entrails::kickstart(ptr);
  }

 private:
  friend class factory;
#if defined(INCORRECT_VARIADIC)
  template <typename Function>
  task(const std::weak_ptr<runner>& runner_, Function&& func_)
#else
  template <typename Function, typename... Args>
  task(const std::weak_ptr<runner>& runner_, Function&& func_, Args&&... args_)
#endif
  {
    m_info = new entrails::taskinfo(runner_);
    m_info->m_u.task = new entrails::task_t(std::bind(
        entrails::task_entry<void>::entry_point
      , m_info
      , static_cast<std::function<void()>>(
          std::bind(
              std::forward<Function>(func_)
#if !defined(INCORRECT_VARIADIC)
            , std::forward<Args>(args_)...
#endif
          )
        )
    ));
  }

  template <typename T> friend class task;
  task(entrails::taskinfo* info) : m_info(info)
  { /* noop */ }

 private:
  entrails::taskinfo* m_info;
};


/**
 * @ref cool::gcd::task::task "Task" factory class.
 */
class factory
{
 public:
  /**
   * Creates and returns a @ref cool::gcd::task::task "task".
   *
   * This method creates and returns the initial task of the sequence. A
   * sequence of tasks will consist of this task and zero or more tasks
   * created by @ref task::then() or @ref task::finally() methods.
   *
   * @param runner_ the @ref cool::gcd::task::runner "runner" to run the
   *    @c func_ Callable.
   * @param func_ the user supplied Callable to be scheduled for execution
   * @param args_ additional arguments to be passed to the user provided
   *    Callable when it begins the execution.
   *
   * @return a new task object
   */
#if defined(INCORRECT_VARIADIC)
  template <typename Function>
  static task<typename std::result_of<Function()>::type>
  create(const std::weak_ptr<runner>& runner_, Function&& func_)
  {
    return task<typename std::result_of<Function()>::type>(
          runner_
        , std::forward<Function>(func_));
  }
#else
  template <typename Function, typename... Args>
  static task<typename std::result_of<Function(Args...)>::type>
  create(const std::weak_ptr<runner>& runner_, Function&& func_, Args&&... args_)
  {
    return task<typename std::result_of<Function(Args...)>::type>(
          runner_
        , std::forward<Function>(func_)
        , std::forward<Args>(args_)...);
  }
#endif
};

/**
 * A class representing a group of asynchronous tasks.
 *
 * Grouping asynchronous tasks allows for aggregate synchronization. The
 * application code can submit multiple tasks and track when they all complete,
 * even though they might use different @ref cool::gcd::task::runner "runners".
 * Such synchronization may be helpful when the application cannot progress
 * until all of the asynchronous tasks are complete.
 *
 * @note The group objects created via copy constructor or copy assignment are
 * considered clones and they represent the same group of tasks.
 *
 * <b>Thread Safety</b><br>
 * The group objects are mostly, but not entirely thread safe. In particular,
 * it may happen that wait() returns prematurely if called from one thread
 * while another thread is adding tasks to the group. This limitation extends
 * to the clones of the group object.
 */
class group
{
  group(group&& other) = delete;
  group& operator=(group&& other) = delete;

 public:
 /**
  * Application handler type for completion callback.
  *
  * The handler called when all asynchronous tasks in the group complete
  * execution must be a Callable that can be assigned to
  * <tt>std::function<void(void)></tt> function type.
  */
  typedef entrails::task_t handler_t;

 public:
 /**
  * Construct a new group object.
  */
  dlldecl group();
 /**
  * Create a clone of the original group object.
  *
  * A new group object is the clone of the original and both represent the
  * same group of tasks and share its state. Thus wait() on two clones from
  * two different threads would return as soon as a set of tasks, added through
  * either (or both) clones is completed.
  */
  dlldecl group(const group& original);
 /**
  * Create and return a clone of the original group object.
  *
  * A new group object is the clone of the original and both represent the
  * same group of tasks and share its state. Thus wait() on two clones from
  * two different threads would return as soon as a set of tasks, added through
  * either (or both) clones is completed.
  */
  dlldecl group& operator =(const group& original);
 /**
  * Destructs the group object.
  *
  * @note The asynchronous tasks submitted to the group object but not yet
  *   finished, or even not yet started, will run to their completion. The
  *   specified completion callback, if specified, will be called when the
  *   last task completes.
  */
  dlldecl ~group();

  /**
   * Accept the task for asynchronous execution as a part of the group.
   *
   * Accepts the task as a part of the group and schedules it for asynchronous
   * execution using the specified runner.
   *
   * @param runner The runner object to use for task execution
   * @param task   The Callable object to be executed
   * @param args   A list of zero or more parameters to pass to the task when
   *               called.
   * @note
   *   Due to incorrect handling of empty parameter packs for variadic templates,
   *   the Callable object may not accept parameters when used in Microsoft
   *   Visual Studio 2013.
   */
#if defined(INCORRECT_VARIADIC)
  template <typename Function>
  void run(const runner& runner, Function&& task)
  {
    void* ctx = new entrails::task_t(task);

    ::dispatch_group_async_f(m_group, runner, ctx, entrails::executor);
  }
#else
  template <typename Function, typename... Args>
  void run(const runner& runner, Function&& task, Args&&... args)
  {
    std::function<typename std::result_of<Function(Args...)>::type(Args...)> aux = task;
    void* ctx = new entrails::task_t(
          bind(aux, std::forward<Args>(args)...));

    ::dispatch_group_async_f(m_group, runner, ctx, entrails::executor);
  }
#endif
  /**
   * Set application handler for task completion.
   *
   * @param handler Handler to be called when task execution completes.
   *
   * Sets the application defined handler to be called when all asynchronous
   * tasks currently in the group complete the execution. If no tasks are
   * running or are scheduled to run the handler is called immediately.
   *
   * @note The handler is called in the context of @ref cool::gcd::task::runner::cool_default()
   *   "global library runner".
   * @note The handler is called only once. If new tasks are added after the
   *   handler was called the application must set a new handler. Setting
   *   multiple handlers before all tasks in the group complete will result
   *   in calling all handlers after the last task completes.
   */
  dlldecl void then(const handler_t& handler);

  /**
   * Wait for the tasks to complete.
   *
   * Waits for the asynchronous tasks in this group to run and complete, or
   * at most the specified amount of time.
   *
   * @param interval Amount of time to wait.
   *
   * @exception cool::exception::timeout The time to wait expired before tasks
   *   completed.
   * @exception cool::exception::illegal_argument The interval is negative.
   * @note It is possible to both wait for the tasks completion and to
   *   specify the completion handler for a group of tasks.
   */
  template <typename Rep, typename Period>
  void wait(const std::chrono::duration<Rep, Period>& interval)
  {
    wait(std::chrono::duration_cast<std::chrono::nanoseconds>(interval).count());
  }

  /**
   * Wait for the tasks to complete.
   *
   * Waits for the asynchronous tasks in this group to run and complete, or
   * at most until the specified time.
   *
   * @param when Time to wait until.
   *
   * @exception cool::exception::timeout The time was reached before tasks
   *   completed, or if the specified time is earlier than the time this
   *   call was made.
   * @note It is possible to both wait for the tasks completion and to
   *   specify the completion handler for a group of tasks.
   */
  template <typename Clock, typename Duration>
  void wait(const std::chrono::time_point<Clock, Duration>& when)
  {
    int64_t w = std::chrono::duration_cast<std::chrono::nanoseconds>(when - Clock::now()).count();
    if (w < 0)
      throw cool::exception::timeout("Timeout while waiting for tasks to complete");

    wait(w);
  }

  /**
   * Wait for the tasks to complete.
   *
   * Waits for the asynchronous tasks in this group to run and complete, or
   * at most the specified amount of time, in nanoseconds.
   *
   * @param interval Amount of time to wait, in nanoseconds.
   *
   * @exception cool::exception::timeout The time to wait expired before tasks
   *   completed.
   * @exception cool::exception::illegal_argument The interval is negative.
   * @note It is possible to both wait for the tasks completion and to
   *   specify the completion handler for a group of tasks.
   */
  dlldecl void wait(int64_t interval);

  /**
   * Wait for the tasks to complete.
   *
   * Waits for the asynchronous tasks in this group to run and complete.
   *
   * @note It is possible to both wait for the tasks completion and to
   *   specify the completion handler for a group of tasks.
   */
  dlldecl void wait();

 private:
  static void executor(void* ctx);
  static void finalizer(void* ctx);

 private:
  dispatch_group_t m_group;
};

} } } // namespace

#endif
