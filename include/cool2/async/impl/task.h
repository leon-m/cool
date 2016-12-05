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
#include <tuple>
#include <stack>

#include "cool2/async/impl/runner.h"

//#define REP(a) std::cerr << "[" << __LINE__ << "] " << a << "\n"
#define REP(a) do { } while(false)

#define PUSH(a) push(a)
namespace cool { namespace async {

// forward declarations magic
namespace impl {
class context;

// ---- execution context stack
// ---- each run() call creates a new execution context stack which is then
// ---- (re)submitted to task queues as long as there are unfinished contexts
class context_stack
{
 public:
  virtual ~context_stack() { /* noop */ }
  virtual void push(context*) = 0;
  virtual context* top() = 0;
  virtual void pop() = 0;
  virtual bool empty() const = 0;
};

}

namespace entrails { void kick(impl::context_stack* ctx_); }

namespace impl {

class internal_exception : public cool::exception::runtime_exception
{
public:
  internal_exception(const std::string& msg_) : runtime_exception(msg_)
  { /* noop */ }
};

class bad_runner_cast : public internal_exception
{
public:
  bad_runner_cast()
    : internal_exception("dynamic_pointer_cast to RunnerT type unexpectedly failed")
  { /* noop */ }
};

class runner_not_available : public internal_exception
{
 public:
  runner_not_available()
    : internal_exception("the destination runner not available")
  { /* noop */ }
};

using default_runner_type = runner;

namespace tag
{
struct simple      { };
struct serial      { }; // compound task with sequential execution
struct parallel    { }; // compound task with concurrent execution
struct conditional { }; // compount task with conditional execution
struct oneof       { }; // oneof compound task
struct loop        { }; // compound task that iterates the subtask
struct repeat      { }; // compound task that repeats the subtask n times
struct intercept { };
} // namespace

// ---- context interface needed for task entry point
class context
{
 public:
  using exception_reporter = std::function<void(const std::exception_ptr&)>;

 public:
  virtual ~context() { /* noop */ }

  virtual std::weak_ptr<async::runner> get_runner() const = 0;
  virtual void entry_point(const std::shared_ptr<async::runner>&, context*) = 0;
  virtual const char* name() const = 0;
};


namespace exec
{
// -- implementation of context stack
class context_stack : public impl::context_stack
{
 public:
  void push(impl::context* arg_) override  { REP("<<<<<< push: " << arg_->name()); m_stack.push(arg_); }
  void pop() override                      { REP(">>>>>> pop: " << top()->name()); delete top(); m_stack.pop(); }
  impl::context* top() override            { return m_stack.top(); }
  bool empty() const override              { return m_stack.empty(); }

 private:
  std::stack<impl::context*> m_stack;
};


// Generic context template that is unsuable - will later be specialized
// for different task context types but we need to declare full template here
template <typename TagT, typename RunnerT, typename InputT, typename ResultT, typename... TaskT>
class context { };


} // namespace

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// -----
// -----
// ----- Task implementations for various task types
// -----
// -----
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <typename TagT, typename RunnerT, typename InputT, typename ResultT, typename... TaskT>
class task { };

// ---------------------------------------------------------------------------
// ------
// ------
// ------ tag::simple task implementation
// ------
// ------
// ---------------------------------------------------------------------------
template <typename RunnerT, typename InputT, typename ResultT>
class task<tag::simple, RunnerT, InputT, ResultT>
{
 public:
  using this_type        = task;
  using result_type      = ResultT;
  using input_type       = InputT;
  using runner_type      = RunnerT;
  using tag_type         = tag::simple;
  using unbound_type     = typename traits::unbound_type<RunnerT, InputT, ResultT>::type;

 public:
  inline explicit task(const std::weak_ptr<runner_type>& r_, const unbound_type& f_)
    : m_runner(r_), m_unbound(f_)
  { /* noop */ }

  // NOTE:
  // Run methods for simple tasks cheat a little - since a simple task by definition
  // has no subtasks they will allocate an execution context which is also a dummy
  // context stack. As its empty() method always return true it will get deleted
  // by task executor immediatelly after the task is done. This optimisation will
  // boost the run() speed for simple tasks to almost double the speed we can get
  // if stack and context were allocated separatelly.
  template <typename T = InputT>
  inline void run(
        const std::shared_ptr<this_type>& self_
      , const typename std::enable_if<!std::is_same<T, void>::value, T>::type& i_)
  {
    ::cool::async::entrails::kick(create_context(nullptr, self_, i_));
  }
  inline void run(const std::shared_ptr<this_type>& self_)
  {
    ::cool::async::entrails::kick(create_context(nullptr, self_));
  }
  template <typename T = InputT>
  exec::context<tag_type, RunnerT, InputT, ResultT>*
  create_context(
        context_stack* stack_
      , const std::shared_ptr<this_type>& self_
      , const typename std::enable_if<!std::is_same<T, void>::value, T>::type& i_);
  exec::context<tag_type, RunnerT, InputT, ResultT>*
  create_context(context_stack* stack_, const std::shared_ptr<this_type>& self_);

  inline std::weak_ptr<runner_type> get_runner() const { return m_runner; }
  inline unbound_type& user_callable()                 { return m_unbound; }

 private:
  std::weak_ptr<runner_type> m_runner;
  unbound_type               m_unbound;    // user Callable
};

template <typename RunnerT, typename InputT, typename ResultT, typename... TaskT>
class task<tag::serial, RunnerT, InputT, ResultT, TaskT...>
{
 public:
  using this_type        = task;
  using result_type      = ResultT;
  using input_type       = InputT;
  using runner_type      = RunnerT;
  using tag_type         = tag::serial;

  template <typename T = InputT>
  void run(const std::shared_ptr<this_type>& self_, const typename std::enable_if<!std::is_same<T, void>::value, T>::type& i_);
  void run(const std::shared_ptr<this_type>& self_);

  explicit inline task(const std::shared_ptr<TaskT>&... tp_)
  {
    m_tasks = std::make_tuple(tp_...);
  }

 private:
  std::weak_ptr<runner_type> m_runner;
  std::tuple<std::shared_ptr<TaskT>...> m_tasks;

};

// ---------------------------------------------------------------------------
// ------
// ------
// ------ tag::conditional task implementation
// ------
// ------
// ---------------------------------------------------------------------------
template <typename InputT, typename ResultT, typename TaskT, typename TaskY>
class task<tag::conditional, default_runner_type, InputT, ResultT, TaskT, TaskY>
{
 public:
  using this_type      = task;
  using result_type    = ResultT;
  using input_type     = InputT;
  using runner_type    = default_runner_type;
  using tag_type       = tag::conditional;
  using predicate_type = std::function<bool(const InputT&)>;

 public:
  explicit inline task(const predicate_type& p_
                     , const std::shared_ptr<TaskT>& t_
                     , const std::shared_ptr<TaskY>& y_)
  : m_predicate(p_), m_if_task(t_), m_else_task(y_)
  { /* noop */ }

  inline void run(const std::shared_ptr<this_type>& self_, const InputT& i_)
  {
    auto stack = new exec::context_stack;
    create_context(stack, self_, i_);
    ::cool::async::entrails::kick(stack);
  }
  exec::context<tag_type, runner_type, InputT, ResultT, TaskT, TaskY>*
  create_context(context_stack* st_, const std::shared_ptr<this_type>& self_, const InputT& i_);

  inline std::weak_ptr<runner_type> get_runner() const    { return m_if_task->get_runner(); }
  inline const predicate_type& predicate() const          { return m_predicate; }
  inline const std::shared_ptr<TaskT>& task_if() const    { return m_if_task; }
  inline const std::shared_ptr<TaskY>& task_else() const  { return m_else_task; }

 private:
  predicate_type         m_predicate;
  std::shared_ptr<TaskT> m_if_task;
  std::shared_ptr<TaskY> m_else_task;
};

// ---------------------------------------------------------------------------
// ------
// ------
// ------ tag::loop task implementation
// ------
// ------
// ---------------------------------------------------------------------------
template <typename InputT, typename ResultT, typename TaskT>
class task<tag::loop, default_runner_type, InputT, ResultT, TaskT>
{
 public:
  using this_type   = task;
  using result_type = ResultT;
  using input_type  = InputT;
  using runner_type = default_runner_type;
  using tag_type    = tag::loop;
  using predicate_type = std::function<bool(const InputT&)>;

 public:
  explicit inline task(const predicate_type& p_, const std::shared_ptr<TaskT>& t_)
      : m_predicate(p_), m_task(t_)
  { /* noop */ }

  inline void run(const std::shared_ptr<this_type>& self_, const InputT& i_)
  {
    auto stack = new exec::context_stack;
    create_context(stack, self_, i_);
    REP("++++++ task::loop::run(" << i_ << ")" );
    ::cool::async::entrails::kick(stack);
  }
  exec::context<tag_type, runner_type, InputT, ResultT, TaskT>*
  create_context(context_stack* st_, const std::shared_ptr<this_type>& self_, const InputT& i_);

  inline std::weak_ptr<runner_type> get_runner() const { return m_task->get_runner(); }
  const std::shared_ptr<TaskT>& subtask() const        { return m_task; }
  const predicate_type& predicate() const              { return m_predicate; }

 private:
  predicate_type         m_predicate;
  std::shared_ptr<TaskT> m_task;
};

// ----------------------------------------------------------------------------
// ----- tag::repeat task
template <typename InputT, typename ResultT, typename TaskT>
class task<tag::repeat, default_runner_type, InputT, ResultT, TaskT>
{
 public:
  using this_type   = task;
  using result_type = ResultT;
  using input_type  = InputT;
  using runner_type = default_runner_type;
  using tag_type    = tag::repeat;

 public:
  explicit inline task(const std::shared_ptr<TaskT>& t_) : m_task(t_)
  { /* noop */ }

  inline void run(const std::shared_ptr<this_type>& self_, const InputT& i_)
  {
    auto stack = new exec::context_stack;
    create_context(stack, self_, i_);
    ::cool::async::entrails::kick(stack);
  }
  exec::context<tag_type, runner_type, InputT, ResultT, TaskT>*
  create_context(context_stack* stack_, const std::shared_ptr<this_type>& self_, const InputT& i_);

  inline std::weak_ptr<runner_type> get_runner() const { return m_task->get_runner(); }
  const std::shared_ptr<TaskT>& subtask() const        { return m_task; }

 private:
  std::shared_ptr<TaskT> m_task;
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// -----
// -----
// ----- Execution contexts for various task types
// -----
// -----
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
namespace exec
{
// value container that can hold even "values" of void type
template <typename ResultT>
struct container
{
  inline container() { /* noop */ }
  inline container(const ResultT& r_) : value(r_) { /* noop */ }
  ResultT value;
};

template <>
struct container<void>
{
};
// a little helper to avoid specializing entire context for void ResultT
template <typename InputT, typename ResultT>
struct report
{
  template <typename RunnerT, typename TaskPtrT, typename ReporterT, typename T = InputT>
  inline static void run_report(
      const RunnerT& r_
    , const TaskPtrT& p_
    , const container<typename std::enable_if<!std::is_same<T, void>::value, T>::type>& i_
    , const ReporterT& rep_)
  {
    ResultT aux = p_->user_callable()(r_, i_.value);
    if (rep_)
      rep_(aux);
  }
  template <typename RunnerT, typename TaskPtrT, typename ReporterT, typename T = InputT>
  inline static void run_report(
      const RunnerT& r_
    , const TaskPtrT& p_
    , const container<typename std::enable_if<std::is_same<T, void>::value, void>::type>& i_
    , const ReporterT& rep_)
  {
    ResultT aux = p_->user_callable()(r_);
    if (rep_)
      rep_(aux);
  }
};

template <typename InputT>
struct report<InputT, void>
{
  template <typename RunnerT, typename TaskPtrT, typename ReporterT, typename T = InputT>
  inline static void run_report(
      const RunnerT& r_
    , const TaskPtrT& p_
    , const container<typename std::enable_if<!std::is_same<T, void>::value, T>::type>& i_
    , const ReporterT& rep_)
  {
    p_->user_callable()(r_, i_.value);
    if (rep_)
      rep_();
  }
  template <typename RunnerT, typename TaskPtrT, typename ReporterT, typename T = InputT>
  inline static void run_report(
      const RunnerT& r_
    , const TaskPtrT& p_
    , const container<typename std::enable_if<std::is_same<T, void>::value, void>::type>& i_
    , const ReporterT& rep_)
  {
    p_->user_callable()(r_);
    if (rep_)
      rep_();
  }
};

template <typename ValueT> class generic_reporter
{
 public:
  static void subtask_result(container<ValueT>* r_, const ValueT& v_)
  {
    r_->value = v_;
  }

  static inline std::function<void(const ValueT&)> get_subtask_reporter (container<ValueT>* r_)
  {
    return std::bind(&generic_reporter::subtask_result, r_, std::placeholders::_1);
  }

  static inline void report_result(std::function<void(const ValueT&)>& f_, container<ValueT>& r_)
  {
    f_(r_.value);
  }
};

template <> class generic_reporter<void>
{
 public:
  static void subtask_result(container<void>* r_)
  { /* noop */ }

  static inline std::function<void()> get_subtask_reporter (container<void>* r_)
  {
    return std::bind(&generic_reporter::subtask_result, r_);
  }

  static inline void report_result(std::function<void()>& f_, container<void>& r_)
  {
    f_();
  }
};

template <typename ResultT> class context_base
{
 public:
  using result_reporter = typename traits::result_reporter<ResultT>::type;
  using exception_reporter = std::function<void(const std::exception_ptr&)>;

 public:
  inline context_base(impl::context_stack* stack_, const result_reporter& r_rep_, const exception_reporter& e_rep_)
      : m_stack(stack_), m_res_reporter(r_rep_), m_exc_reporter(e_rep_)
  { /* noop */ }
  inline ~context_base()
  { /* noop */ }

  void set_res_reporter(const result_reporter& arg_)    { m_res_reporter = arg_; }
  void set_exc_reporter(const exception_reporter& arg_) { m_exc_reporter = arg_; }

 protected:
  impl::context_stack*  m_stack;
  result_reporter       m_res_reporter; // result reporter if set
  exception_reporter    m_exc_reporter; // exception reporter if set
};

// ---------------------------------------------------------------------------
// ------
// ------
// ------ tag::simple execution context
// ------
// ------ This context is a bit of cheat - since it is known that simple
// ------ tasks do not have subtasks, it can implement fake context_stack
// ------ interface so that is serves as its own context_stack. This way the
// ------ run() method for simple tasks can avoid double allocation (one for
// ------ stack and the other for context) thus almost doubling the execution
// ------ speeds for simple tasks.
// ---------------------------------------------------------------------------
template <typename RunnerT, typename InputT, typename ResultT>
class context<tag::simple, RunnerT, InputT, ResultT> : public impl::context
                                                     , public context_base<ResultT>
                                                     , public impl::context_stack
{
 public:
  using task_type  = impl::task<tag::simple, RunnerT, InputT, ResultT>;
  using base       = context_base<ResultT>;

  public:
   template<typename T = InputT>
   inline context(
           impl::context_stack* st_
         , const std::shared_ptr<task_type>& t_
         , const typename std::enable_if<!std::is_same<T, void>::value, T>::type& r_
         , const typename base::result_reporter& r_rep_ = typename base::result_reporter()
         , const typename base::exception_reporter& e_rep_ = typename base::exception_reporter())
     : base(st_, r_rep_, e_rep_), m_input(r_), m_task(t_)
  { /* noop */
    REP("++++++ context::simple::context");
    if (st_ != nullptr)
      st_->push(this);
  }
  inline context(
          impl::context_stack* st_
        , const std::shared_ptr<task_type>& t_
        , const typename base::result_reporter& r_rep_ = typename base::result_reporter()
        , const typename base::exception_reporter& e_rep_ = typename base::exception_reporter())
    : base(st_, r_rep_, e_rep_), m_task(t_)
  { /* noop */ }

  // impl::context interface
  void entry_point(const std::shared_ptr<async::runner>& r_, impl::context* ctx_) override;
  inline std::weak_ptr<async::runner> get_runner() const override { return m_task->get_runner(); }
  const char* name() const override { return "context::simple"; }
  // impl::context_stack interface
  void push(impl::context*) override { /* noop */ }
  impl::context* top() override      { return this; }
  void pop() override                { /* noop */ }
  bool empty() const override        { return true; }

 private:
  container<InputT>              m_input;  // Input to pass to user Callable
  std::shared_ptr<task_type>     m_task;   // Reference to static task data
  std::unique_ptr<impl::context> m_self;
};

// ---------------------------------------------------------------------------
// ------
// ------
// ------ tag::conditional execution context
// ------
// ------
// ---------------------------------------------------------------------------
template <typename InputT, typename ResultT, typename TaskT, typename TaskY>
class context<tag::conditional, default_runner_type, InputT, ResultT, TaskT, TaskY>
    : public impl::context
    , public context_base<ResultT>
{
 public:
  using task_type = impl::task<tag::conditional, default_runner_type, InputT, ResultT, TaskT, TaskY>;
  using base      = context_base<ResultT>;

 public:
  inline context(
          impl::context_stack* st_
        , const std::shared_ptr<task_type>& t_
        , const InputT& i_
        , const typename base::result_reporter& r_rep_ = typename base::result_reporter()
        , const typename base::exception_reporter& e_rep_ = typename base::exception_reporter());

  // impl::context interface
  void entry_point(const std::shared_ptr<async::runner>& r_, impl::context* ctx_) override;
  inline std::weak_ptr<async::runner> get_runner() const override
  {
    std::weak_ptr<async::runner> aux;
    if (m_pred_result)
      aux = m_task->task_if()->get_runner();
    else
      aux = m_task->task_else()->get_runner();
    return aux;
  }
  const char* name() const override { return "context::conditional"; }

  // exception and result reporting from subtasks
  inline void report_exception(const std::exception_ptr& exc) { m_last_exception = exc; }

 private:
  template <typename T>
  void prepare_context(const std::shared_ptr<T>& t_, const InputT& i_)
  {
    auto ctx = t_->create_context(base::m_stack, t_, i_);
    ctx->set_exc_reporter(m_this_exc_reporter);
    ctx->set_res_reporter(generic_reporter<ResultT>::get_subtask_reporter(&m_result));
  }

 private:
  bool                              m_pred_result;
  container<ResultT>                m_result;
  std::shared_ptr<task_type>        m_task;       // reference to static task data
  typename base::exception_reporter m_this_exc_reporter;
  std::exception_ptr                m_last_exception;
};

// ---------------------------------------------------------------------------
// ------
// ------
// ------ tag::loop execution context
// ------
// ------
// ---------------------------------------------------------------------------
template <typename InputT, typename ResultT, typename TaskT>
class context<tag::loop, default_runner_type, InputT, ResultT, TaskT>
    : public impl::context
    , public context_base<ResultT>
{
 public:
  using task_type = impl::task<tag::loop, default_runner_type, InputT, ResultT, TaskT>;
  using base      = context_base<ResultT>;

 public:
  inline context(
          impl::context_stack* st_
        , const std::shared_ptr<task_type>& t_
        , const InputT& i_
        , const typename base::result_reporter& r_rep_ = typename base::result_reporter()
        , const typename base::exception_reporter& e_rep_ = typename base::exception_reporter())
    : base(st_, r_rep_, e_rep_), m_value(i_), m_task(t_)
  {
    m_this_exc_reporter = std::bind(&context::report_exception, this, std::placeholders::_1);
    m_this_res_reporter = std::bind(&context::report_result, this, std::placeholders::_1);
    REP("++++++ context::loop::context(" << i_ << ")");
    if (st_ != nullptr)
      st_->push(this);
  }

  // impl::context interface
  void entry_point(const std::shared_ptr<async::runner>& r_, impl::context* ctx_) override;
  std::weak_ptr<async::runner> get_runner() const override { return m_task->get_runner(); }
  const char* name() const override { return "context::loop"; }

  // exception and result reporting from subtasks
  void report_exception(const std::exception_ptr& exc) { m_last_exception = exc; }
  void report_result(const InputT& i_) { m_value = i_; }

 private:
  InputT                            m_value;      // input to user lambda and predicate
  std::shared_ptr<task_type>        m_task;       // reference to static task data
  typename base::exception_reporter m_this_exc_reporter;
  typename base::result_reporter    m_this_res_reporter;
  std::exception_ptr                m_last_exception;
};

// ---------------------------------------------------------------------------
// ------
// ------
// ------ tag::repeat execution context
// ------
// ------
// ---------------------------------------------------------------------------
template <typename InputT, typename ResultT, typename TaskT>
class context<tag::repeat, default_runner_type, InputT, ResultT, TaskT>
    : public impl::context
    , public context_base<ResultT>
{
 public:
  using task_type = impl::task<tag::repeat, default_runner_type, InputT, ResultT, TaskT>;
  using base      = context_base<ResultT>;

 public:
  inline context(
          impl::context_stack* st_
        , const std::shared_ptr<task_type>& t_
        , const InputT& i_
        , const typename base::result_reporter& r_rep_ = typename base::result_reporter()
        , const typename base::exception_reporter& e_rep_ = typename base::exception_reporter())
    : base(st_, r_rep_, e_rep_), m_iterations(i_), m_current(0), m_task(t_)
  {
    m_this_exc_reporter = std::bind(&context::report_exception, this, std::placeholders::_1);
    if (st_ != nullptr)
      st_->push(this);
  }

  // impl::context interface
  void entry_point(const std::shared_ptr<async::runner>& r_, impl::context* ctx_) override;
  std::weak_ptr<async::runner> get_runner() const override { return m_task->get_runner(); }
  const char* name() const override { return "context::repeat"; }

  // exception reporting from subtasks
  void report_exception(const std::exception_ptr& exc) { m_last_exception = exc; }

 private:
  const InputT               m_iterations; // number of iterations
  InputT                     m_current;    // current iteration
  std::shared_ptr<task_type> m_task;       // reference to static task data
  typename base::exception_reporter m_this_exc_reporter;
  std::exception_ptr                m_last_exception;
};


} // namespace

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// -----
// -----
// ----- Task and context methods implementation
// -----
// ----- They need to be implemented later as they create and use execution
// ----- contexts and need to know details of them.
// -----
// -----
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// -----
// ----- tag::simple
// -----
// ---------------------------------------------------------------------------
// --- task::create_context
template <typename RunnerT, typename InputT, typename ResultT>
template <typename T>
inline exec::context<tag::simple, RunnerT, InputT, ResultT>*
task<tag::simple, RunnerT, InputT, ResultT>::create_context(
      impl::context_stack* stack_
    , const std::shared_ptr<this_type>& self_
    , const typename std::enable_if<!std::is_same<T, void>::value, T>::type& i_)
{
  return new exec::context<tag_type, RunnerT, InputT, ResultT>(stack_, self_, i_);
}

// --- task::create_context
template <typename RunnerT, typename InputT, typename ResultT>
inline exec::context<tag::simple, RunnerT, InputT, ResultT>*
task<tag::simple, RunnerT, InputT, ResultT>::create_context(
      impl::context_stack* stack_
    , const std::shared_ptr<this_type>& self_)
{
  return new exec::context<tag_type, RunnerT, InputT, ResultT>(stack_, self_);
}

// --- context::entry_point
template <typename RunnerT, typename InputT, typename ResultT>
void exec::context<tag::simple, RunnerT, InputT, ResultT>::entry_point(
      const std::shared_ptr<async::runner>& r_
    , impl::context* ctx_)
{
  REP("------ context::simple::entry_point");
  try
  {
    auto r = std::dynamic_pointer_cast<RunnerT>(r_);
    if (!r)
      throw bad_runner_cast();

    report<InputT, ResultT>::run_report(r, m_task, m_input, base::m_res_reporter);
  }
  catch (...)
  {
    if (base::m_exc_reporter)
      base::m_exc_reporter(std::current_exception());
  }

  if (base::m_stack != nullptr)
    base::m_stack->pop(); // task complete, delete the context
}
// ---------------------------------------------------------------------------
// -----
// ----- tag::conditional
// -----
// ---------------------------------------------------------------------------
// --- task::create_context
template <typename InputT, typename ResultT, typename TaskT, typename TaskY>
inline exec::context<tag::conditional, default_runner_type, InputT, ResultT, TaskT, TaskY>*
task<tag::conditional, default_runner_type, InputT, ResultT, TaskT, TaskY>::create_context(
      impl::context_stack* stack_
    , const std::shared_ptr<this_type>& self_
    , const InputT& i_)
{
  return new exec::context<tag_type, default_runner_type, InputT, ResultT, TaskT, TaskY>(stack_, self_, i_);
}

// --- context::context
template <typename InputT, typename ResultT, typename TaskT, typename TaskY>
exec::context<tag::conditional, default_runner_type, InputT, ResultT, TaskT, TaskY>::context(
    impl::context_stack* st_
  , const std::shared_ptr<impl::task<tag::conditional, default_runner_type, InputT, ResultT, TaskT, TaskY>>& t_
  , const InputT& i_
  , const typename base::result_reporter& r_rep_
  , const typename base::exception_reporter& e_rep_)
      : base(st_, r_rep_, e_rep_), m_task(t_)
{
  REP("++++++ context::conditional::context");
  // push immediatelly so that the context of "if" or "else" task comes on the top
  if (st_ != nullptr)
    st_->push(this);

  m_this_exc_reporter = std::bind(&context::report_exception, this, std::placeholders::_1);

  // evaluate predicate and create the appropriate task's context and push it on the top
  m_pred_result = m_task->predicate()(i_);
  if (m_pred_result)
    prepare_context(m_task->task_if(), i_);
  else
    prepare_context(m_task->task_else(), i_);
}

// context::entry_point
template <typename InputT, typename ResultT, typename TaskT, typename TaskY>
void exec::context<tag::conditional, default_runner_type, InputT, ResultT, TaskT, TaskY>::entry_point(
    const std::shared_ptr<async::runner> &r_
  , impl::context *ctx_)
{
  REP("------ context::conditional::entry_point");
  if (m_last_exception)
  {
    if (base::m_exc_reporter)
      base::m_exc_reporter(m_last_exception);
  }
  else
  {
    if (base::m_res_reporter)
      generic_reporter<ResultT>::report_result(base::m_res_reporter, m_result);
  }
  base::m_stack->pop();
}

// ---------------------------------------------------------------------------
// -----
// ----- tag::loop
// -----
// ---------------------------------------------------------------------------
// ----- task::create_context
template <typename InputT, typename ResultT, typename TaskT>
inline exec::context<tag::loop, default_runner_type, InputT, ResultT, TaskT>*
task<tag::loop, default_runner_type, InputT, ResultT, TaskT>::create_context(
      impl::context_stack* stack_
    , const std::shared_ptr<this_type> &self_
    , const InputT &i_)
{
  return new exec::context<tag_type, default_runner_type, InputT, ResultT, TaskT>(stack_, self_, i_);
}
// ----- context::entry_point
template <typename InputT, typename ResultT, typename TaskT>
void exec::context<tag::loop, default_runner_type, InputT, ResultT, TaskT>::entry_point(
      const std::shared_ptr<async::runner> &r_
    , impl::context *ctx_)
{
  REP("------ context::loop::entry_point");
  try
  {
    if (m_last_exception)  // did subtask report exception?
    {
      if (base::m_exc_reporter)
        base::m_exc_reporter(m_last_exception);
      base::m_stack->pop();  // task interrupted owing to exception, remove context
      return;
    }

    // first check the predicate to determine whether to terminate loop or not
    if (!m_task->predicate()(m_value))
    {
      if (base::m_res_reporter)  // tag::loop task has results
        base::m_res_reporter(m_value);
      base::m_stack->pop(); // task complete, delete the context
      return;
    }

    // create exec context for subtask - since new context will be at the
    // top of the stack it will get run next time this context stack is scheduled
    auto ctx = m_task->subtask()->create_context(base::m_stack, m_task->subtask(), m_value);
    ctx->set_exc_reporter(m_this_exc_reporter);
    ctx->set_res_reporter(m_this_res_reporter);
  }
  catch (...)
  {
    if (base::m_exc_reporter)
      base::m_exc_reporter(std::current_exception());
    base::m_stack->pop();  // task interrupted owing to predicate exception, remove context
  }
}

// ---------------------------------------------------------------------------
// -----
// ----- tag::repeat
// -----
// ---------------------------------------------------------------------------
// ----- task::create_context
template <typename InputT, typename ResultT, typename TaskT>
inline exec::context<tag::repeat, default_runner_type, InputT, ResultT, TaskT>*
task<tag::repeat, default_runner_type, InputT, ResultT, TaskT>::create_context(
      impl::context_stack* stack_
    , const std::shared_ptr<this_type> &self_
    , const InputT &i_)
{
  return new exec::context<tag_type, default_runner_type, InputT, ResultT, TaskT>(stack_, self_, i_);
}
// ----- context::entry_point
template <typename InputT, typename ResultT, typename TaskT>
void exec::context<tag::repeat, default_runner_type, InputT, ResultT, TaskT>::entry_point(
      const std::shared_ptr<async::runner> &r_
    , impl::context *ctx_)
{
  if (m_last_exception)  // did subtask report exception?
  {
    if (base::m_exc_reporter)
      base::m_exc_reporter(m_last_exception);
    base::m_stack->pop();  // task interrupted owing to exception, remove context
    return;
  }
  // first check if the iteration limit was reached
  if (++m_current > m_iterations)
  {
    base::m_stack->pop(); // task complete, delete the context
    return;
  }

  auto ctx = m_task->subtask()->create_context(base::m_stack, m_task->subtask(), m_current);
  ctx->set_exc_reporter(m_this_exc_reporter);
}




#if 0
// ----- common types
// --
// --
enum class task_type { simple, parallel, serial, intercept };

namespace simple    { class taskinfo; class context; }
namespace serial    { class taskinfo; class context; }
namespace parallel  { class taskinfo; class context; }
namespace intercept { class taskinfo; class context; }

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
#endif

} } } // namespace

#endif