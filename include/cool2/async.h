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

#if !defined(COOL_ASYNC_H_HEADER_GUARD)
#define COOL_ASYNC_H_HEADER_GUARD

#include <memory>
#include <string>
#include <functional>
#include <iostream>
#include "impl/platform.h"
#include "impl/traits.h"

namespace cool { namespace async {

namespace impl {
class runner;
}

/**
 * Runner types.
 *
 * Conceptually, a runner can either be sequential or cuncurrent. The sequential
 * runner executets the @ref task "tasks" from its task queue one after another,
 * only starting the execution of the next task after the exectuion of the
 * previous task completed. The concurrent runner start the execution of the next
 * task as soon as an idle thread is available in the thread pool, regardless
 * of whether the execution of the previous task has completed or not.
 *
 * The runner type is only indicative. In particular there  is no guarantee that 
 * the platform supports the cuncurrent runners an even if it does, there is no
 * guarantee that there will be an idle thread available in the worker thread
 * pool. The only guarantee offered by the RunnerType is that the sequential
 * runner <em>will not</em> execute the tasks from its task queue concurrently.
 */
enum class RunnerType { SEQUENTIAL, CONCURRENT };

/**
 * A representation of the queue of asynchronously executing tasks.
 *
 * This class is an abstraction representing a queue of asynchronously executing
 * tasks. Althought the actual implementation of the task queue is platform 
 * dependent with the implementation details not exposed through API, all 
 * platforms share the following common capabilities:
 *   - all runner implementations are capable of executing @ref task "tasks"
 *     sequentially, and some implementations can execute them concurrently
 *   - all runner implementations are thread-safe in a sense that all operations
 *     of this class are thread safe and that @ref task::run "run()" methods
 *     on tasks using the same runner may be called concurrently from several
 *     threads
 *   - all static methods returning runner will return a pointer to valid
 *     runner regardless of the implementation. Some implementations may
 *     return the same runner regardless of the method while other will return
 *     different runners.
 *
 * Reguraly created runner objects represent idependent task queues. However, 
 * runner objects created via copy construction are considered to be clones of
 * the original object and refer to the same task queue as the original object.
 * The same is true for runner objects that get a different task queue assigned
 * via copy assignment.
 *
 * The runner provides no public facilities for task scheduling and execution.
 * Tasks are submitted into the runner's task queue via @ref task::run() "run"
 * method of the @ref task.
 */
class runner
{
 public:
  using ptr_t = std::shared_ptr<runner>;
  using weak_ptr_t = std::weak_ptr<runner>;

 public:
  /**
   * Construct a new runner object.
   *
   * Constructs a new runner object, optionally with the desired priority and
   * task execution order.
   *
   * @param type_ optional parameter, set to RunnerType::SEQUENTIAL by default.
   *
   * @exception cool::exception::create_failure thrown if a new instance cannot
   *   be created.
   *
   * @note The runner object is created in started state and is immediately
   *   capable of executing tasks.
   */
  dlldecl runner(RunnerType type_ = RunnerType::SEQUENTIAL);

  /**
   * Copy constructor.
   *
   * Constructs a copy of a runner objects. Note that a newly construted runner
   * is considered to be a @em clone of the original object. Both runner objects
   * share the same internal task queue.
   */
  dlldecl runner(const runner&) = default;
  /**
   * Copy assignment operator.
   *
   * The left-hand side runer object will drop the reference to the current
   * internal task queue and receive the reference to the internal task queue
   * of the right-hand side runner object. The previous task queue of the
   * assignee may get destryoed, depending on whether there is another runner object
   * keeping a reference to it and on the platforms task queue destruction
   * strategy.
   */
  dlldecl runner& operator=(const runner&) = default;
  runner(runner&&) = delete;
  runner& operator=(runner&&) = delete;
  /**
   * Destroys the runner object.
   *
   * Destroying the runner object means dropping a reference to its internal
   * task queue. If this was the only reference to the task queue, the task
   * queue will begin its destruction cycle, but, depending on the platform,
   * may not be destroyed immediatelly. Some platforms will destroy the task
   * queue only after the last task from the queue was run.
   */
  dlldecl ~runner();
  /**
   * Return the name of this runner.
   *
   * Every runner object has a process level unique name.
   */
  dlldecl const std::string& name() const;
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
   * Returns system-wide runner object with the high priority.
   *
   * @note This runner may execute tasks concurrently.
   */
  dlldecl static ptr_t sys_high();
  /**
   * Returns system-wide runner object with the default priority.
   *
   * @note This runner may execute tasks concurrently.
   */
  dlldecl static ptr_t sys_default();
  /**
   * Returns system-wide runner object with the low priority.
   *
   * @note This runner may execute tasks concurrently.
   */
  dlldecl static ptr_t sys_low();
  /**
   * Returns system-wide runner object with the background (lowest) priority.
   *
   * @note This runner may execute tasks concurrently.
   */
  dlldecl static ptr_t sys_background();
  /**
   * Returns library default runner.
   *
   * @note This runner executes tasks sequentially.
   */
  dlldecl static ptr_t cool_default();
  
 private:
  std::shared_ptr<impl::runner> m_impl;
};

template <typename ResultT, typename ParamT>
class task
{
 public:
  using result_t     = ResultT;
  using parameter_t  = ParamT;

 public:

  template <typename FunctionT>
  task(const runner::weak_ptr_t& r_, FunctionT&& f_)
    : m_runner(r_)
  {

  }
  task() { }
 private:
  runner::weak_ptr_t m_runner;
};


class taskop
{
 private:

 public:
  template <typename CallableT>
  static task<typename impl::traits::function_traits<CallableT>::result_type
            , typename impl::traits::arg_type<1, CallableT>::type>
  create(const runner::weak_ptr_t& r_, CallableT&& f_)
  {
    // check number of parameters and the type of the first parameter
    static_assert(
        2 >= impl::traits::function_traits<CallableT>::arity::value && 0 < impl::traits::function_traits<CallableT>::arity::value
      , "Callable with signature RetT Callable([const] runner::weak_ptr_t[&] [, argument]) is required to construct a task");
    static_assert(
        std::is_same<
            runner::weak_ptr_t
          , typename std::decay<typename impl::traits::function_traits<CallableT>::template arg<0>::type>::type>
        ::value
      , "Callable with signature RetT Callable([const] runner::weak_ptr_t[&] [, argument]) is required to construct a task");

    return task<
          typename impl::traits::function_traits<CallableT>::result_type
        , typename impl::traits::arg_type<1, CallableT>::type
      >(runner::weak_ptr_t(r_), std::forward<CallableT>(f_));
  }

  /**
   * Create a compound task which will concurrently execute its subtasks.
   *
   * Creates a compound task which will, when run, schedule all its subtasks
   * for execution with their respective @ref runner "runner's" concurrently
   * and will collect the results of their executions. Whether the subtrasks will 
   * actually run concurrently depends on the runners they'll use and on the
   * availability of idle threads in the worker pool. The parallel compound task
   * will defer the completion of its execution until all subtasks are
   * finished and will return the
   */
  template <typename... TaskT>
  static task<typename impl::traits::parallel_result<TaskT...>::type
            , typename impl::traits::first_task<TaskT...>::type::parameter_t>
  parallel(TaskT&&... t)
  {
    static_assert(
        impl::traits::all_same<typename std::decay<typename std::decay<TaskT>::type::parameter_t>::type...>::value
      , "All parallel tasks must have the same parameter type or no parameter");

    return task<typename impl::traits::parallel_result<TaskT...>::type
              , typename impl::traits::first_task<TaskT...>::type::parameter_t
          >();
  }

  template <typename... TaskT>
  static task<typename impl::traits::sequence_result<TaskT...>::type
            , typename impl::traits::first_task<TaskT...>::type::parameter_t>
  sequential(TaskT&&... t)
  {
    static_assert(
        impl::traits::are_chained<typename std::decay<TaskT>::type...>::value
      , "The type of the parameter of each task in the sequence must match the return type of the preceding task.");
    return task<typename impl::traits::sequence_result<TaskT...>::type
              , typename impl::traits::first_task<TaskT...>::type::parameter_t
          >();
  }
};


#if 0
template <typename ResultT, typename... ResultY>
class sequential
{
};

template <typename ResultT, typename... ResultY>
class sequential<task<ResultT>, ResultY...>
{

};

template <typename ResultT, typename ResultY>
class sequential<task<ResultT>, task<ResultY>>
{
 public:
  static task<ResultY> combine(const task<ResultT>&lhs, const task<ResultY>& rhs)
  {
    return task<ResultY>();
  }
};
#endif
} } // namespace
#endif