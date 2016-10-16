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

#if !defined(cool_f36aacb0_dda1_4ce1_b25a_943f5951523a)
#define cool_f36aacb0_dda1_4ce1_b25a_943f5951523a

#include <string>
#include <functional>
#include <iostream>

#include "cool2/impl/platform.h"
#include "cool2/impl/traits.h"
#include "cool/exception.h"
#include "impl/task.h"

namespace cool { namespace async {

namespace entrails { void kick(const impl::context_ptr&); }

using impl::runner_not_available;

class taskop;

/**
 * A class template representing the basic objects that can be scheduled for execution
 * by one of the @ref runner "runners".
 *
 * A task can be one of the following:
 *  - a <em>simple task</em>, which contains a @em Callable and is associated with a
 *    specific @ref runner. Invoking the run method on a simple task will schedule
 *    the task for execution with the associated @ref runner by insertion its @em Callable
 *    into the runner's task queue. Simple task objects are created via
 *    taskop::create method teplate.
 *  - a <em>compound task</em> which contains other tasks as its subtasks. Subtasks can be
 *    either compound or simple tasks. When a compound task gets activated (via
 *    a call to run(), for instance) it fetches one or more of its simple subtasks 
 *    and schedules them for execution. The compound task remains active until the 
 *    execution of all of its subtasks is complete. Since the compound tasks do not
 *    contain executable code they are not associated with any @ref runner. Componud
 *    tasks are created by any of the following @ref taskop methods templates: @ref taskop::parallel
 *    "parallel()", @ref taskop::sequential "sequential()", @ref taskop::intercept
 *    "intercept()", or @ref taskop::intercept_all "intercept_all()", or via the corresponding
 *    task class template member methods templates.
 *  - an <em>exception handling task</em> which intercepts an exception of a particular
 *    type, or any exception, that may be thrown by the preceding task. An exception
 *    handling task is any compound or simple tasks that adheres to certain limitations
 *    about its parameter or return value type. One or more exception handling tasks
 *    can be attached to any task using  @ref taskop method templates @ref taskop::intercept
 *    "intercept()" or @ref taskop::intercept_all() "intercept_all()", or by using
 *    the corresponding method templates of the task class template. Note that the
 *    exception handling tasks are normally not sheduled for execution; they get
 *    scheduled only if an eception was thrown in one of the preceding tasks
 *    and is caught by the exception handling task.
 *
 * The compound tasks are internally organized in one of two possible ways:
 *  - @em parallel organization; when such compund task is activated it will
 *    immediatelly schedule all of its subtasks for execution with their respective
 *    @ref runner "runners". Note that should two or more subtasks be associated
 *    with the the same @ref runner, they will still get scheduled immediatelly but
 *    the scheduling order in which they are passed to the @ref runner is not
 *    defined. The parallel task remains active until the
 *    last of its subtasks completes the execution. Then it collects the results
 *    of all subtasks, if any, and returns the collection as its result.
 *    Parallel compound tasks are created by using @ref taskop
 *    method template @ref taskop::parallel "parallel()" or the corresponding method
 *    template of the task class template.
 *
 *  - @em sequential organization; when such compound tasks is activated, only
 *    the first task in the sequence is scheduled for execution; when this task
 *    task is completed, its result, if any, is passed as an input parameter to
 *    the next task in the sequence, which is then scheduled for execution with
 *    its associated runner, and so on. The sequential compound tasks remains
 *    activa until the last task in the sequence completes its execution, when
 *    it collects the result of the last task, if any, and returns it as its
 *    own result.  Sequential compound tasks are created via a call to @ref taskop
 *    method template @ref taskop::sequential "sequential()" or the corresponding method
 *    template of the task class template.
 * 
 * Every task either no input parameters or exactly one input parameter and a 
 * result type, which can be @c void if the task produces no results. For simple
 * tasks the presence or absence of the input parameter and its type, as well as
 * the task's result type (or @c void if none) are deduced from its @em Callable 
 * passed to the @ref taskop::create method template.
 *
 * The presence of the <em>input parameter</em> and its type of the compound task are
 * deduced from the first task in the parameter list passed to either
 * taskop:: sequential or taskop::parallel method. Thus by definition the
 * the input paramter presence and its type of the sequential compound task
 * matches the input parameter of the first task in the sequence, and when the
 * sequential compound task is activated, it will pass its input parameter, if
 * any, to the first task in the sequence. If the compound task is a parallel task,
 * all of its other subtasks must match the input parameter of the first task
 * passed to the taskop::parallel method template, and when parallel compound task is
 * activated it will pass its input parameter to all of its subtasks.
 *
 * The <em>return value</em> type of the sequential compound task is deduced from the
 * last subtask in the sequence (last task parameter passed to taskop::sequential).
 * The return value type of the parallel compound task is defined to be
 * <tt>std::tuple<t1, t2, t3 ...></tt> where @c t1, @c t2, @c t3 ... are the
 * result types of its subtasks in the exact order as passed to taskop::parallel.
 * If a subtask of the parallel compound task does not return result
 * (result type @c void), a type <tt>void*</tt> is used in its place in the
 * <tt>std::tuple</tt> template parameter list. This element of the tuple is
 * never set, it is just used as a placeholder to enable the instatination of the
 * result tuple.
 *
 * The <em>exception handling tasks</em> that are to intercept and handle exceptions
 * of a particular type must accept the parameter of this type (or better, a
 * const reference to avoid object slicing). The exception handling tasks that
 * are to intercept and handle any exception must accept <tt>std::exception_ptr</tt> as their
 * parameter. The result type of the exception handling task must match the
 * the result type of the task to which they are attached (preceding task).
 *
 * <b>Handling of Exceptions</b>
 *
 * The way the task compositions (compount tasks) handle the exceptions has been
 * modeled to follow the C++ @c catch paradigm as close as possible, having in
 * mind that the simple tasks exectute their @em Callables asynchronously. This
 * means that an exception thrown in one @em Callable cannot be simply caught
 * in another @em Callable since this another  @em Callable may execute some time
 * in the future or even has already completed its execution. This is where the
 * <em>exception handling</em> taks enter the picture. When a currently executing 
 * @em Callable throws a C++ exception, which remains unhandled when the execution
 * of the throwing callable completes, the task enclosing the @em Callable that
 * threw will intercept the exception and pass it on to the next task in the
 * sequence. If task is not designated to handle this exception type it will
 * pass it on to the next task, and so on, until the exception reaches the
 * eception handling task that will accept this, or any, exception type. If
 * no matching exception handling task is found the exception will be silently
 * and irrevocably discarded. The following is the full set of rules that govern
 * the handling of exceptions:
 *   - an exception thrown in a task which is not part of the @em sequential 
 *     compound task, and is not either caught or intercepted by this task, is 
 *     irrevocably lost
 *   - an exception thrown inside a subtask of the @em sequential task, and not
 *     intercepted within this subtask, will travel along the sequence until it
 *     reaches an appropriate <em>exception handling</em> subtask. If no appropriate
 *     exeption handling subtask is reached before the end of the sequence the
 *     exception will exit the compound task and will be passed on to the next
 *     task in the sequence, if any. <em>None of the tasks in sequence between 
 *     the task that threw the exception and the task that intercepted it, or
 *     the end of the sequence, will be scheduled for execution</em>.
 *   - an exception thrown inside a subtask of the @em parallel task will
 *     affect this subtask only. The remaining subtasks of the parallel compound
 *     task will run to their completion. Then the compound task will pass the
 *     exception on to the next task in the sequence, if any.
 *
 */
template <typename TagT, typename ResultT, typename ParamT>
class task
{
 public:
  using this_t       = task;
  using result_t     = ResultT;
  using parameter_t  = ParamT;
  using tag          = TagT;

 public:

  /**
   * Create a compound task which will concurrently execute its subtasks.
   *
   * Using this method in the following code fragment:
   * @code
   *   auto new_task = task.parallel(task1, task2, task3);
   * @endcode
   * is exactly the same as using:
   * @code
   *   auto new_task = taskop::parallel(task, task1, task2, task3);
   * @endcode
   * This method is provided for convenience to allow chaining the
   * task operations such as:
   * @code
   *   task.parallel(task1, task2).intercept(handler).run();
   * @endcode
   * @warning This method invalidates @c this task object.
   * @see taskop::parallel
   * @see @ref sequential
   * @see taskop::sequential
   */
  template <typename... TaskT>
  task<impl::tag::parallel
     , typename impl::traits::parallel_result<this_t, TaskT...>::type
     , parameter_t>
  parallel(TaskT&&... tasks);

  /**
   * Create a compound task which will cexecute its subtasks one after another.
   *
   * Using this method in the following code fragment:
   * @code
   *   auto new_task = task.sequential(task1, task2, task3);
   * @endcode
   * is exactly the same as using:
   * @code
   *   auto new_task = taskop::sequential(task, task1, task2, task3);
   * @endcode
   * This method is provided for convenience to allow chaining the
   * task operations such as:
   * @code
   *   task.sequential(task1, task2).intercept(handler).run();
   * @endcode
   * @warning This method invalidates @c this task object.
   * @see taskop::parallel
   * @see @ref parallel
   * @see taskop::sequential
   */
  template <typename... TaskT>
  task<impl::tag::serial
     , typename impl::traits::sequence_result<this_t, TaskT...>::type
     , parameter_t>
  sequential(TaskT&&... tasks);

  /**
   * Add an exception handler(s) for specific exception(s) to the task.
   *
   * Using this method in the following code fragment:
   * @code
   *   auto new _task = task.intercept(handler1, handler2);
   * @endcode
   * is exactly the same as using:
   * @code
   *   auto new _task = taskop::intercept(task, handler1, handler2);
   * @endcode
   * This method is provided for convenience to allow chaining the
   * task operations, such as:
   * @code
   *   task.intercept(handler).parallel(task2).run();
   * @endcode
   * @warning This method invalidates @c this task object.
   * @see taskop::intercept
   * @see @ref intercept_all
   * @see taskop::intercept_all
   */
  template <typename... HandlerTaskT>
  task <impl::tag::serial, ResultT, ParamT>
  intercept(HandlerTaskT&&... handlers);

  /**
   * Add an exception handler for any exception to the task.
   *
   * Using this method in the following code fragment:
   * @code
   *   auto new _task = task.intercept_all(handler);
   * @endcode
   * is exactly the same as using:
   * @code
   *   auto new _task = taskop::intercept_all(task, handler);
   * @endcode
   * This method is provided for convenience to allow chaining the
   * task operations, such as:
   * @code
   *   task.intercept_all(handler).parallel(task2).run();
   * @endcode
   * @warning This method invalidates @c this task object.
   * @see taskop::intercept
   * @see @ref intercept
   * @see taskop::intercept_all
   */
  template <typename HandlerTaskT>
  task <impl::tag::serial, ResultT, ParamT>
  intercept_all(HandlerTaskT&& handler);

  /**
   * Schedule a task for execution by its runner.
   *
   * If the @ref task is a simple task, this method will sumbit its
   * @em Callable to its associated @ref runner for execution. If the task
   * is a compound task, it will activate it. The activation means that the
   * compound task will start submitting its subtasks to their respective
   * @ref runner "runners" for execution.
   *
   * @param param_ the parameter forwarded to the task.
   */
  void run(parameter_t&& param_)
  {
    if (m_impl == nullptr)
      throw exception::illegal_state("this task object is not valid");

    entrails::kick(impl::task_factory<ResultT, ParamT, TagT>::make_context(m_impl, param_));
  }

 private:
  friend class taskop;
  task(const impl::taskinfo_ptr& arg_) : m_impl(arg_)
  { /* noop */ }

 private:
  impl::taskinfo_ptr m_impl;
};

// ------ task specialization for tasks with no input parameter
//
template <typename TagT, typename ResultT>
class task<TagT, ResultT, void>
{
 public:
  using this_t       = task;
  using result_t     = ResultT;
  using parameter_t  = void;

 public:

  template <typename... TaskT>
  task<impl::tag::parallel
      , typename impl::traits::parallel_result<this_t, TaskT...>::type
      , parameter_t>
  parallel(TaskT&&... tasks);

  template <typename... TaskT>
  task<impl::tag::serial
     , typename impl::traits::sequence_result<this_t, TaskT...>::type
     , parameter_t>
  sequential(TaskT&&... tasks);

  template <typename... HandlerTaskT>
  task<impl::tag::serial, ResultT, void>
  intercept(HandlerTaskT&&... handlers);

  template <typename HandlerTaskT>
  task<impl::tag::serial, ResultT, void>
  intercept_all(HandlerTaskT&& handler);

  void run()
  {
    entrails::kick(impl::task_factory<ResultT, void, TagT>::make_context(m_impl));
  }

 private:
  friend class taskop;
  task(const impl::taskinfo_ptr& arg_) : m_impl(arg_)
  { /* noop */ }

 private:
  impl::taskinfo_ptr m_impl;
};

/**
 * Class implementing the operations on tasks.
 */
class taskop
{
 public:
  /**
   * Create a simple task.
   *
   * Creates and returns a simple task associated with the @ref runner. When
   * activated via @ref task::run() method, the task will submit its @em Callable for
   * execution with the associated runner. The @em Callable must satisfy the
   * following requirements:
   *
   *   - it can be a function pointer, lambda closure, @c std::function, or
   *     a functor object as long as it provides a single overload of the
   *     function call operator <tt>()</tt>.
   *   - it must accept <tt>const runner::weak_ptr>&</tt> as its first
   *     parameter
   *   - it may accept one additional input parameter which is then the input
   *     parameter of the task
   *   - it may, but does not need to, return a value of any type.
   *
   * The @em Callable implementation should not contain any blocking code. The
   * blocking code will block the worker thread executing the callable. Since
   * the number of working threads in the pool may be limited (optimal number
   * should be close to the number of precessor cores to avoid taskinfo switches)
   * the pool may run out of the active, non-blocked threads, will will block
   * the execution of the entire program. The @em Callable implementation should
   * use I/O event sources instead of blocking read/write calls, and should
   * organize the data around runners instead of using the thread synchronization
   * primitives.
   *
   * The @em Callable may return result. The result value will be passed to the
   * next task in the sequence as an input parameter. If the next task in the
   * sequence us the parallel compound task. the result will be passed to every
   * subtask of this compund task.
   *
   * The @em Callable may throw a C++ exception. The unhandled exception will
   * travel along the sequence of tasks until it reaches the exception handling
   * task that can interept it. None of the tasks between the throwing task and
   * the interception task will be scheduled for execution.
   *
   * @param r_ the @ref runner which will be used to run the @em Callable
   * @param f_ @em Callable to execute
   *
   * @note Unfortunately the objects returned by the @c std::bind template
   *   provide multiple function call operator overloads and cannot be passed
   *   to the simple task directly, without transforming them into @c std::function
   *   object first.
   *
   * @warning The @em Callable should not contain blocking code.
   */
  template <typename CallableT>
  static task<impl::tag::simple
            , typename impl::traits::function_traits<CallableT>::result_type
            , typename impl::traits::arg_type<1, CallableT>::type>
  create(const runner::weak_ptr& r_, CallableT&& f_)
  {
    // check number of parameters and the type of the first parameter
    static_assert(
        2 >= impl::traits::function_traits<CallableT>::arity::value && 0 < impl::traits::function_traits<CallableT>::arity::value
      , "Callable with signature RetT (const runner::ptr& [, argument]) is required to construct a task");
    static_assert(
        std::is_same<
            runner::ptr
          , typename std::decay<typename impl::traits::function_traits<CallableT>::template arg<0>::type>::type>
        ::value
      , "Callable with signature RetT (const runner::ptr& [, argument]) is required to construct a task");

    using result_t = typename impl::traits::function_traits<CallableT>::result_type;
    using param_t  = typename impl::traits::arg_type<1, CallableT>::type;

    return task<impl::tag::simple, result_t, param_t>(
        impl::task_factory<result_t, param_t, impl::tag::simple>::create(r_, f_));
  }

  /**
   * Create a compound task which will concurrently execute its subtasks.
   *
   * Creates a compound task which will, when activated, schedule all its subtasks
   * for execution with their respective @ref runner "runners" concurrently
   * and will collect the results of their executions. Whether the subtrasks will
   * actually run concurrently depends on the runners they'll use and on the
   * availability of idle threads in the worker pool. The parallel compound task
   * will defer the completion of its execution until all subtasks are
   * finished and will return the results of all subtasks as its result.
   *
   * All subtasks @c t_ must accept the input parameter of the same type, or no
   * input paramter. The input paramter of the parallel compound task is the
   * same as the input parameter of its subtasks. When activated, the parallel
   * compund task will pass its input parameter to all its subtasks.
   *
   * The result of the parallel compund task is an union of the results of its
   * subtasks, implemented using the @c std::tuple template. The position of the
   * subtasks result in the tuple is the same as the position of the subtask
   * in the parameter list. For subtasks that do not return the result the
   * tuple will contain element of the type <tt>void*</tt> which value is not
   * defined.
   *
   * @param t_ two or more subtasks to be scheduled to run concurrently.
   *
   * @note Subtasks of the parallel compound task are <em>placed into the
   *       task queues</em> of their runners concurrently. This does not
   *       guarantee concurrent execution of the subtasks.
   * @note The method consumes all tasks passed as parameters @c t_. After the
   *       completion of this call they are all invalidated.
   * @warning The parallel compound task may find one or more subtasks impossible to
   *       schedule as their runners may no longer exist. In such case the
   *       compound task will behave as if the task that could not have been
   *       run threw @ref runner_not_available exception.
   */
#if 0
  template <typename... TaskT>
  static task<impl::tag::parallel
            , typename impl::traits::parallel_result<TaskT...>::type
            , typename impl::traits::first_task<TaskT...>::type::parameter_t>
  parallel(TaskT&&... t_)
  {
    using result_t = typename impl::traits::parallel_result<TaskT...>::type;
    using param_t = typename impl::traits::first_task<TaskT...>::type::parameter_t;

    static_assert(
        impl::traits::all_same<typename std::decay<typename std::decay<TaskT>::type::parameter_t>::type...>::value
      , "All parallel tasks must have the same parameter type or no parameter");

    return task<impl::tag::parallel, result_t, param_t>(
        impl::task_factory<impl::tag::parallel, result_t, param_t>::create());
  }
#endif
  /**
   * Create a compound task which will execute its subtasks one after another.
   *
   * Creates a compound task which will, when executed, schedule its first subtask
   * for execution, wait for it completion, then schedule the next task
   * for execution, and so on, until the end of the sequence is reached. The
   * order of task execution is from the left to the right with respect to
   * the subtask positions in the parameter list.
   *
   * The inout parameter of the compound task is the same as the input parameter
   * of the first, the leftmost, task. When activated the compound task will
   * pass its parameter to the first subtask. The type of the input parameter of the
   * next subtask task must match the result type of the preeding subtask task.
   * Upon the preceding task completion its result will be passed to the next
   * task as the input parameter.
   *
   * The result of the sequential compund task is the result of its last,
   * the righmost, subtask.
   *
   * @param t_ two or more subtasks to be scheduled to run one after another.
   *
   * @note The method consumes all tasks passed as parameters @c t_. After the
   *       completion of this call they are all invalidated.
   * @warning The compound task may find the subtask to be run impossible
   *       to schedule as its runner may no longer exist. In such case the
   *       compound task will behave as if the task that could not have been
   *       run threw @ref runner_not_available exception.
   */
  template <typename... TaskT>
  static task<impl::tag::serial
            , typename impl::traits::sequence_result<TaskT...>::type
            , typename impl::traits::first_task<TaskT...>::type::parameter_t>
  sequential(TaskT&&... t_)
  {
    using result_t = typename impl::traits::sequence_result<TaskT...>::type;
    using param_t = typename impl::traits::first_task<TaskT...>::type::parameter_t;

    static_assert(
        impl::traits::all_chained<typename std::decay<TaskT>::type...>::result::value
      , "The type of the parameter of each task in the sequence must match the return type of the preceding task.");
    return task<impl::tag::serial, result_t, param_t>(
        impl::task_factory<result_t, param_t, impl::tag::serial>::create(t_.m_impl...));
  }
  /**
   * Add an exception handler(s) to the task.
   */
#if 0
  template <typename TaskT, typename... HandlerTaskT>
  static task<impl::tag::serial, typename TaskT::result_t, typename TaskT::param_t>
  intercept(TaskT&& t_, HandlerTaskT&&... handlers)
  {
    using result_t = typename TaskT::result_t;
    using param_t = typename TaskT::parameter_t;

    static_assert(
        impl::traits::all_same<typename TaskT::result_t, typename HandlerTaskT::result_t...>::value
      , "The task and its exception handlers must have the same result type");
    return task<impl::tag::serial, result_t, param_t>(
        impl::task_factory<impl::tag::serial, result_t, param_t>::create());
  }
#endif
  /**
   * Add an exception handler to the task.
   */
#if 0
  template <typename TaskT, typename HandlerTaskT>
  static task<impl::tag::serial, typename TaskT::result_t, typename TaskT::param_t>
  intercept_all(TaskT&& t_, HandlerTaskT&& handler)
  {
    using result_t = typename TaskT::result_t;
    using param_t = typename TaskT::parameter_t;

    static_assert(
        std::is_same<typename TaskT::result_t, typename HandlerTaskT::result_t>::value
      , "The task and its exception handlers must have the same result type");
    static_assert(
        std::is_same<std::exception_ptr, typename std::decay<typename HandlerTaskT::parameter_t>::type>::value
      , "The exception handler to catch all exceptions must accept std::exception_ptr as its parameter");
    return task<impl::tag::serial, result_t, param_t>(
        impl::task_factory<impl::tag::serial, result_t, param_t>::create());
  }
#endif
};

// ---------------------------------------------------------------------------
// implementation of task methods
#if 0
template <typename TagT, typename ResultT, typename ParameterT>
template <typename... HandlerTaskT>
inline task<impl::tag::serial, ResultT, ParameterT>
task<TagT, ResultT, ParameterT>::intercept(HandlerTaskT&&... handlers)
{
  return taskop::intercept(*this, std::forward<HandlerTaskT>(handlers)...);
}

template <typename TagT, typename ResultT, typename ParameterT>
template <typename HandlerTaskT>
inline task<impl::tag::serial, ResultT, ParameterT>
task<TagT, ResultT, ParameterT>::intercept_all(HandlerTaskT&& handler)
{
  return taskop::intercept_all(*this, std::forward<HandlerTaskT>(handler));
}

template <typename TagT, typename ResultT, typename ParameterT>
template <typename... TaskT>
inline task<impl::tag::parallel
          , typename impl::traits::parallel_result<task<TagT, ResultT, ParameterT>, TaskT...>::type
          , ParameterT>
task<TagT, ResultT, ParameterT>::parallel(TaskT&&... tasks)
{
  return taskop::parallel(*this, std::forward<TaskT>(tasks)...);
}
#endif
template <typename TagT, typename ResultT, typename ParameterT>
template <typename... TaskT>
inline task<impl::tag::serial
          , typename impl::traits::sequence_result<task<TagT, ResultT, ParameterT>, TaskT...>::type
          , ParameterT>
task<TagT, ResultT, ParameterT>::sequential(TaskT&&... tasks)
{
  return taskop::sequential(*this, std::forward<TaskT>(tasks)...);
}
#if 0
// ---- implementation of task methods for void partial specialization

template <typename TagT, typename ResultT>
template <typename... HandlerTaskT>
inline task<impl::tag::serial, ResultT, void>
task<TagT, ResultT, void>::intercept(HandlerTaskT&&... handlers)
{
  return taskop::intercept(*this, std::forward<HandlerTaskT>(handlers)...);
}

template <typename TagT, typename ResultT>
template <typename HandlerTaskT>
inline task<impl::tag::serial, ResultT, void>
task<TagT, ResultT, void>::intercept_all(HandlerTaskT&& handler)
{
  return taskop::intercept_all(*this, std::forward<HandlerTaskT>(handler));
}

template<typename TagT, typename ResultT>
template <typename... TaskT>
inline task<impl::tag::parallel
          , typename impl::traits::parallel_result<task<TagT, ResultT, void>, TaskT...>::type
          , void>
task<TagT, ResultT, void>::parallel(TaskT&&... tasks)
{
  return taskop::parallel(*this, std::forward<TaskT>(tasks)...);
}
#endif
template<typename TagT, typename ResultT>
template <typename... TaskT>
inline task<impl::tag::serial
          , typename impl::traits::sequence_result<task<TagT, ResultT, void>, TaskT...>::type
          , void>
task<TagT, ResultT, void>::sequential(TaskT&&... tasks)
{
  return taskop::sequential(*this, std::forward<TaskT>(tasks)...);
}
} } // namespace

#endif