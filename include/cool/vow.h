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

#if !defined(VOW_H_HEADER_GUARD)
#define VOW_H_HEADER_GUARD

#include <functional>
#include <atomic>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <memory>

#include "entrails/platform.h"
#include "exception.h"
#include "entrails/vow.h"

namespace cool { namespace basis {

template <typename T> class vow;
template <typename T> class vow_base;

// ------ aim
/**
 * Base class of the aim<T> class template.
 *
 * @tparam T Type of the result.
 *
 * @see @ref cool::basis::aim "aim<T>"
 */
template <typename T> class aim_base
{
 protected:
  typedef entrails::state<T> state_t;

 public:
  typedef vow<T>             vow_t;

 private:
  typedef aim_base<T>        this_t;

 protected:
  aim_base(const aim_base&)            = default;
  aim_base& operator= (aim_base&)      = default;
  aim_base(aim_base&& rhs)             = delete;
  aim_base& operator =(aim_base&& rhs) = delete;
  aim_base()                           = delete;

 public:
  /**
   * Block until the result becomes available and return result.
   *
   * @return Result as reported via one of set() overloads of cool::basis::vow.
   *
   * @exception any Exception as reported by the asynchronous operation
   *   via set(const std::exception_ptr&) overload of cool::basis::vow.
   * @exception cool::exception::illegal_state if the result stored in
   *   the shared stateexceptionwas already consumed.
   * @exception cool::exception::broken_vow if the last cool::basis::vow dropped
   *   the reference to the shared state without making it ready.
   * @note When two or more threads are calling get() on the aim<> clones
   *   only one thread will receive the result and the other threads will get
   *   an cool::exception::illegal_state exception. It cannot be predicted
   *   which thread will receive results.
   */
  T get();
  /**
   * Block until the result becomes available or at most <i>interval</i> time units.
   *
   * @return Result as reported via one of set() overloads of cool::basis::vow.
   *
   * @exception any Exception as reported by the asynchronous operation
   *   via set(const std::exception_ptr&) overload of cool::basis::vow.
   * @exception cool::exception::illegal_state if the result stored in
   *   the shared state was already consumed.
   * @exception cool::exception::broken_vow if the last cool::basis::vow dropped
   *   the reference to the shared state without making it ready.
   * @exception cool::exception::timeout if the period as stated by
   *   <i>interval</i> elapsed before the result became available.
   * @note When two or more threads are calling get() on the aim<> clones
   *   only one thread will receive the result and the other threads will get
   *   an cool::exception::illegal_state exception. It cannot be predicted
   *   which thread will receive results.
   */
  template <typename Rep, typename Period>
  T get(const std::chrono::duration<Rep, Period>& interval);
  /**
   * Block until the result becomes available or until <i>timepoint</i> time is reached.
   *
   * @return Result as reported via one of set() overloads of cool::basis::vow.
   *
   * @exception any Exception as reported by the asynchronous operation
   *   via set(const std::exception_ptr&) overload of cool::basis::vow.
   * @exception cool::exception::illegal_state if the result stored in
   *   the shared state was already consumed.
   * @exception cool::exception::broken_vow if the last cool::basis::vow dropped
   *   the reference to the shared state without making it ready.
   * @exception cool::exception::timeout if the time as stated by
   *   <i>timepoint</i> was reached before the result became available.
   * @note When two or more threads are calling get() on the aim<> clones
   *   only one thread will receive the result and the other threads will get
   *   an cool::exception::illegal_state exception. It cannot be predicted
   *   which thread will receive results.
   */
  template <typename Clock, typename Duration>
  T get(const std::chrono::time_point<Clock, Duration>& timepoint);

 protected:
  aim_base(const typename state_t::ptr_t& state);
  bool then_base(bool& fail,
                 const typename state_t::success_cb_t& scb,
                 const typename state_t::failure_cb_t& fcb);

 protected:
  typename state_t::ptr_t m_state;
};

/**
 * Waits for a result of asynchronous execution.
 *
 * @tparam T Type of the result.
 *
 * The class template cool::basis::aim provides a mechanism to access the result
 * of the asynchronous operations, or to specify a callback to be called when
 * the result of the asynchronous operation becomes available.
 *
 * The aim objects cannot be created directly but can only be obtained
 * through the cool::basis::vow::get_aim() method, which creates and returns
 * the aim object associated with the same shared state. Once the results are
 * obtained either through the callback specified through one of the then()
 * overloads or throught one of the get() overloads the shared state is
 * marked <i>consumed</i> and the subsequent attempt to obtain the result
 * will throw the exception of type cool::basis::illegal_state_exception.
 *
 * the cool::basis::aim object is the consumer side of the vow-aim communication
 * channel. The asynchonous operation uses cool::basis::vow object to set
 * the result and the creator of the asynchronous operation can use associated
 * aim object to access the result, once it becomes avaialable, as in the
 * following example code fragment:
 * @code
 *     vow<int> v;
 *     auto a = v.get_aim();
 *     std::thread thr([=] { v.set(37 + 5); });
 *     std::cout << "Thread reported " << a.get() << std::endl;
 * @endcode
 * @note All cool::basis::aim instances created though successive calls to
 *   cool::basis::vow::get_aim() on the same aim object, or created through
 *   copy construction or copy assignment will refer to the same shared state.
 *   However only one such instance can fetch the result. Other instances
 *   trying to fetch the result will throw an exception of type
 *   cool::exception::illegal_state.
 *
 * @see @ref cool::basis::vow "vow<T>"
 */
template <typename T> class aim : public aim_base<T> {
 private:
  typedef aim<T>             this_t;

 public:
  /**
   * Type of the result accessible through aim.
   *
   * The result type must be copy-constructible.
   */
  typedef T                  result_t;
  /**
   * Type for the user code callback.
   *
   * This callback, if set, is called when the result of an asynchronous operation
   * becomes known. The user callback must be Callable object that can be assigned
   * to the function type @c std::function<void(const T&)>. When called, the callback
   * receives the value of result as its argument.
   */
  typedef typename aim_base<T>::state_t::success_cb_t success_cb_t;
  /**
   * Type for the user code callback.
   *
   * This callback, if set, is called when the result of an asynchronous operation
   * becomes known and is an exception. The user callback must be Callable object that can be assigned
   * to the function type @c std::function<void(const std::exception_ptr&)>. When called, the callback
   * receives the exception object as its argument.
   */
  typedef typename aim_base<T>::state_t::failure_cb_t failure_cb_t;
  /**
   * Type for the user code chained callbacks.
   *
   * This callback type is used for chaining the user callbacks. The user callback
   * code must be Callable object that can be assinged to the function type
   * @c std::function<void(const vow<T>&, const T&)> .
   */
  typedef typename aim_base<T>::state_t::chained_cb_t chained_cb_t;

 public:
  /**
   * Set the user callbacks.
   *
   * Sets the user callbacks to be called when the associated cool::basis:vow
   * makes the shared state ready.
   *
   * @param scb Callable to be called when the result is set
   * @param fcb Callable to be called when the exception object is set
   *
   * @note If the shared state was made ready before the call to this method,
   *   the appropriate function object will be called immediatelly from the
   *   context of the calling thread. Otherwise, the appropriate function object
   *   will be called when the shared state is made ready from the context of
   *   the thread that made the shared state ready.
   * @note <i>fcb</i> function object will be called if the cool::basis::vow
   *   <i>abandons</i> the shared state. The exception pointer will point to
   *   the exception object of type cool::exception::broken_vow.
   *
   * <b>Example</b><br>
   *
   * This simplified example shows the use of then(). In this case the
   * callbacks (lambda expressions) will run from the context of thread @c t,
   * since the shared state will be made ready after using then() to set the
   * callbacks.
   * @code
       cool::basis::vow<double> v;

       {
         auto a = v.get_aim();

         a.then(
           [] (const double& result)
           {
             std::cout << "++++++ OK got result " << result << std::endl;
           },
           [] (const std::exception_ptr& err)
           {
             std::cout << "****** ERROR got exception" << std::endl;
           }
         );
       }

       std::thread t([=] () { v.set(42.42); } );
       t.join();
   * @endcode
   */
  void then(const success_cb_t& scb,
            const failure_cb_t& fcb);

  /**
   * Set the chain of user callbacks.
   *
   * This function is a chaining variant of the simple then() method and
   * enables the user code to set a chain of callbacks to be called when
   * the shared state associated with the aim object is made ready. Each
   * but the last callback in the chain receives an internally generated
   * vow object which it should use to propagate the result of its execution
   * to the next callback in the chain. The execution of the callbacks
   * in the chain stops immediatelly when the user code throws an exception
   * and the execption object is propagated to the error callback, specified to
   * the last link in the chain.
   *
   * @param ccb Chained callback to be called when the shared state is made ready
   *   the result is set.
   * @return The aim object associated with the vow object passed to the user
   *   callback.
   *
   * <b>Example</b><br>
   * This simplified example illustrates the use of the chained then(). This
   * code:
   * @code
  auto err_cb = [] (const std::exception_ptr& err)
  {
    std::cout << "------ ERROR got exception" << std::endl;
  };
  auto cb_1st = [] (const vow<double>& v1, const double& result)
  {
    std::cout << "++++++ OK first internediate result " << result << std::endl;
    v1.set(result * 2);
  };
  auto cb_2nd = [] (const vow<double>& v2, const double& result)
  {
    std::cout << "++++++ OK second internediate result " << result << std::endl;
    v2.set(result * 2);
  };
  auto cb_3rd = [] (const double result)
  {
    std::cout << "++++++ OK final result " << result << std::endl;
  };

  cool::basis::vow<double> v;

  v.get_aim().then(cb_1st).then(cb_2nd).then(cb_3rd, err_cb);

  std::thread t([=] () { v.set(42.42); } );
  t.join();
   * @endcode
   * will produce the following output:
   * @code
   ++++++ OK first internediate result 42.42
   ++++++ OK second internediate result 84.84
   ++++++ OK final result 169.68
   * @endcode
   * @note All user callbacks in the chain expect the result of the same
   *   data type.
   * @note Chaining the user callbacks allow the better structure of the
   *   callback code since several smaller, specialized functions can be used
   *   in place of a single, larger and unstructured block of code.
   * @note The user callbacks specified in the chain after the user callback
   *   that threw an exception will not be called. Thus replacing variable
   *   @c cb_1st from the above example with the following lamda:
   * @code
  auto cb_1st = [] (const vow<double>& v1, const double& result)
  {
    std::cout << "++++++ OK internediate result " << result << std::endl;
    throw 42;
  };
   * @endcode
   * will skip the execution of the lambda @c cb_2nd and will immediatelly engage
   * the error handler @c err_cb, yielding the following output:
   * @code
   ++++++ OK first internediate result 42.42
   ------ ERROR got exception
   * @endcode
   * @note Instead of throwing an exception the @c cb_1st lambda could use
   *   the set() method on vow object @c v1 to set the exception pointer and
   *   to achieve the same effect,
   */
  this_t then(const chained_cb_t& ccb);

  /**
   * Set the chain of user callbacks passing different result types.
   *
   * This method template acts the same as the chained
   * @ref then(const chained_cb_t& ccb) "then()" but it allows results of different
   * to be passed between callbacks in the chain, as in the following code
   * fragment:
   * @code
  auto err_cb = [] (const std::exception_ptr& err)
  {
    std::cout << "****** ERROR got exception" << std::endl;
  };
  auto cb_1st = [] (const vow<double>& v1, const bool& result)
  {
    std::cout << "++++++ OK first internediate result " << result << std::endl;
    v1.set(42.42);
  };
  auto cb_2nd = [] (const double result)
  {
    std::cout << "++++++ OK final result " << result << std::endl;
  };

  cool::basis::vow<bool> v;

  v.get_aim().then<double>(cb_1st).then(cb_2nd, err_cb);

  std::thread t([=] () { v.set(true); } );
  t.join();
   * @endcode
   */
  template <typename Y>
  aim<Y> then(const std::function<void (const vow<Y>&, const result_t&)>& ccb);

 private:
  template <typename Y> friend class vow_base;
  aim(const typename aim_base<T>::state_t::ptr_t& state);
};

/**
 * Specialization of the cool::basis::aim class template for @c void data type.
 *
 * @see @ref cool::basis::aim "aim<T>"
 * @see @ref cool::basis::vow<void> "vow<void>"
 */
template <> class aim<void> : public aim_base<void>
{
 public:
  typedef void                  result_t;
  typedef aim<void>             this_t;
  /**
   * Type for the user code callback.
   *
   * This callback, if set, is called when the result of an asynchronous operation
   * becomes known. The user callback must be Callable object that can be assigned
   * to the function type @c std::function<void(void)>.
   */
  typedef aim_base<void>::state_t::success_cb_t success_cb_t;
  /**
   * Type for the user code callback.
   *
   * This callback, if set, is called when the result of an asynchronous operation
   * becomes known and is an exception. The user callback must be Callable object that can be assigned
   * to the function type @c std::function<void(const std::exception_ptr&)>. When called, the callback
   * receives the exception object as its argument.
   */
  typedef aim_base<void>::state_t::failure_cb_t failure_cb_t;
  /**
   * Type for the user code chained callbacks.
   *
   * This callback type is used for chaining the user callbacks. The user callback
   * code must be Callable object that can be assinged to the function type
   * @c std::function<void(const vow<void>&)> .
   */
  typedef aim_base<void>::state_t::chained_cb_t chained_cb_t;

 public:
  /**
   * Set the user callbacks.
   *
   * Sets the user callbacks to be called when the associated cool::basis:vow
   * makes the shared state ready.
   *
   * @param scb Callable to be called when the result is set
   * @param fcb Callable to be called when the exception object is set
   *
   * @note If the shared state was made ready before the call to this method,
   *   the appropriate function object will be called immediatelly from the
   *   context of the calling thread. Otherwise, the appropriate function object
   *   will be called when the shared state is made ready from the context of
   *   the thread that made the shared state ready.
   * @note <i>fcb</i> function object will be called if the cool::basis::vow
   *   <i>abandons</i> the shared state. The exception pointer will point to
   *   the exception object of type cool::exception::broken_vow.
   *
   * <b>Example</b><br>
   * This simplified example shows the use of then(). In this case the
   * callbacks (lambda expressions) will run from the context of thread @c t,
   * since the shared state will be made ready after using then() to set the
   * callbacks.
   * @code
  cool::basis::vow<void> v;

  {
    auto a = v.get_aim();

    a.then(
      [] ()
      {
        std::cout << "++++++ OK task finished" << std::endl;
      },
      [] (const std::exception_ptr& err)
      {
        std::cout << "****** ERROR got exception" << std::endl;
      }
    );
  }

  std::thread t([=] () { v.set(); } );
  t.join();
   * @endcode
   */
  dlldecl void then(const success_cb_t& scb,
            const failure_cb_t& fcb);

  /**
   * Set the chain of user callbacks.
   *
   * This function is a chaining variant of the simple then() method and
   * enables the user code to set a chain of callbacks to be called when
   * the shared state associated with the aim object is made ready. Each
   * but the last callback in the chain receives an internally generated
   * vow object which it should use to propagate the result of its execution
   * to the next callback in the chain. The execution of the callbacks
   * in the chain stops immediatelly when the user code throws an exception
   * and the execption object is propagated to the error callback, specified to
   * the last link in the chain.
   *
   * @param ccb Chained callback to be called when the shared state is made ready
   *   the result is set.
   * @return The aim object associated with the vow object passed to the user
   *   callback.
   *
   * <b>Example</b><br>
   * This simplified example illustrates the use of the chained then(). This
   * code:
   * @code
  auto err_cb = [] (const std::exception_ptr& err)
  {
    std::cout << "------ ERROR got exception" << std::endl;
  };
  auto cb_1st = [] (const vow<void>& v1)
  {
    std::cout << "++++++ OK first callback called." << std::endl;
    v1.set();
  };
  auto cb_2nd = [] (const vow<void>& v2)
  {
    std::cout << "++++++ OK second callback called." << std::endl;
    v2.set();
  };
  auto cb_3rd = [] ()
  {
    std::cout << "++++++ OK final callback called." << std::endl;
  };

  cool::basis::vow<void> v;

  v.get_aim().then(cb_1st).then(cb_2nd).then(cb_3rd, err_cb);

  std::thread t([=] () { v.set(); } );
  t.join();
   * @endcode
   * will produce the following output:
   * @code
   ++++++ OK first callback called.
   ++++++ OK second callback called.
   ++++++ OK final callback called.
   * @endcode
   * @note All user callbacks in the chain expect the result of the same
   *   data type.
   * @note Chaining the user callbacks allow the better structure of the
   *   callback code since several smaller, specialized functions can be used
   *   in place of a single, larger and unstructured block of code.
   * @note The user callbacks specified in the chain after the user callback
   *   that threw an exception will not be called. Thus replacing variable
   *   @c cb_1st from the above example with the following lamda:
   * @code
  auto cb_1st = [] (const vow<void>& v1)
  {
    std::cout << "++++++ OK first callback called." << std::endl;
    throw 42;
  };
   * @endcode
   * will skip the execution of the lambda @c cb_2nd and will immediatelly engage
   * the error handler @c err_cb, yielding the following output:
   * @code
   ++++++ OK first callback called.
   ------ ERROR got exception
   * @endcode
   * @note Instead of throwing an exception the @c cb_1st lambda could use
   *   the set() method on vow object @c v1 to set the exception pointer and
   *   to achieve the same effect,
   */
  dlldecl this_t then(const chained_cb_t& ccb);

  /**
   * Set the chain of user callbacks passing different result types.
   *
   * This method template acts the same as the chained
   * @ref then(const chained_cb_t& ccb) "then()" but it allows results of different
   * to be passed between callbacks in the chain, as in the following code
   * fragment:
   * @code
  auto err_cb = [] (const std::exception_ptr& err)
  {
    std::cout << "****** ERROR got exception" << std::endl;
  };
  auto cb_1st = [] (const vow<double>& v1)
  {
    std::cout << "++++++ OK first callback called" << std::endl;
    v1.set(42.42);
  };
  auto cb_2nd = [] (const double result)
  {
    std::cout << "++++++ OK final result " << result << std::endl;
  };

  cool::basis::vow<void> v;

  v.get_aim().then<double>(cb_1st).then(cb_2nd, err_cb);

  std::thread t([=] () { v.set(); } );
  t.join();
   * @endcode
   */
  template <typename Y>
  aim<Y> then(const std::function<void (const vow<Y>&)>& ccb);

 private:
  template <typename Y> friend class vow_base;
  aim(const state_t::ptr_t& state);
};

// ------ vow
/**
 * Base class of the vow<T> class template.
 *
 * @tparam  T Type of the result to be reported to the associated aim<T> object.
 *
 * @note This class template servers as the base class for the
 *   vow<T> class template and its specializations. It cannot be
 *   used on its own.
 *
 * @see @ref cool::basis::vow "vow<T>"
 */
template <typename T> class vow_base
{
 protected:
  typedef aim<T>                  aim_t;
  typedef entrails::state<T>      state_t;

  vow_base(const vow_base&);
  vow_base& operator= (const vow_base&);
  vow_base();
  ~vow_base();

  void set(const std::exception_ptr&) const;

 public:
  /**
   * Predicate to check whether the shared state was made ready.
   *
   * Checks whether the shared state associated with vow object was made
   * ready by setting the result or the exception.
   *
   * @return @c true if the shared state was made ready, @c false otherwise.
   */
  bool is_set() const;
  /**
   * Create and return the cool::basis::aim object associated with the
   * vow object.
   *
   * @return cool::basis::aim object associated with vow object.
   *
   * @note Multiple calls to get_aim() are permitted. All cool::basis::aim
   *   objects returned through multiple calls to get_aim() on vow object, or
   *   multiple vow objects associated with the same shared state, will
   *   be associated with the same shared state.
   */
  aim_t get_aim() const;

 protected:
  typename state_t::ptr_t m_state;
};

/**
 * Stores a result of an execution in a seperate thread (asynchronous task).
 *
 * @tparam T Type of the result to be reported to the vow.
 *
 * The cool::basis::vow class template provides a facility to store a result,
 * or an exception, of a function executing in a separate thread and which can
 * later be acquired through cool::basis::aim object created by the
 * cool::basis::vow object. Each cool::basis::vow object is associated with a
 * shared state, which
 * contain some state information and the <i>result</i>, which may not yet be
 * evaluated, evaluated to a value (possibly void) or evaluated to an
 * exception. A vow may do one the the following three things
 * with the shared state:
 *  - <i>make ready</i>; the vow stores the result or the exception in the
 *    shared state and marks it ready. The latter either unlocks the thread
 *    waiting on an cool::basis::aim, associated with the same shared state,
 *    or calls one of the callbacks specified through one of the then()
 *    overloads of associated cool::basis::aim object.
 *  - <i>release</i>; the vow gives up the reference to the shared state after
 *    the shared state was made ready. If this was last reference to the shared
 *    state from either vow or the cool::basis::aim, created by the vow object,
 *    the shared state will cease to exist
 *  - <i>abandon</i>; the vow gives up the reference to the shared state before
 *    the shared state was made ready. The vow stores the exception of type
 *    cool::exception::broken_vow and makes it ready before dropping the reference.
 *
 * The cool::basis::vow is the <i>push</i> end of the vow-aim communication
 * channel; the asynchronous task uses it to communicate the result of its
 * execution to the thread waiting for the result.
 *
 * @note All cool::basis::vow instances created through the copy construction or
 *   copy assignment will refer to the same shared state. Only one instance can
 *   make the shared state ready. The last vow instance referring to the same
 *   shared state will <i>abandon</i> it if it was not yet made ready by any
 *   other instance <i>releasing</i> it.
 *
 * <b>Thread Safety</b> <br>
 * cool::basis::vow objects are not thread safe.
 *
 * @see @ref cool::basis::aim "aim<T>"
 */
template <typename T> class vow : public vow_base<T>
{
 public:
  typedef T                       result_t;
  typedef vow<T>                  this_t;
  typedef std::shared_ptr<this_t> ptr_t;

 public:
  vow(const vow&)              = default;
  vow& operator= (const vow&)  = default;
  vow();

  /**
   * Store the exception into the shared state and make it ready.
   *
   * @param exc std::exception_ptr to the exception object to be stored
   *            into the shared state.
   *
   * @exception cool::exception::illegal thrown if the shared
   *   state associated with the vow object was already made ready.
   *
   * @note The call to set() will invoke the user callback if one was set
   *   through one of then() overloads of associated  cool::basis::aim object.
   */
  void set(const std::exception_ptr& exc) const;
  /**
   * Store the result into the shared state and make it ready.
   *
   * @param value result to be stored into the shared state.
   *
   * @exception cool::exception::illegal_state thrown if the shared
   *   state associated with the vow object was already made ready.
   *
   * @note The call to set() will invoke the user callback if one was set
   *   through one of then() overloads of associated  cool::basis::aim object.
   */
  void set(const T& value) const;
};

/**
 * Specialization of cool::basis::vow for @c void data type.
 *
 * @see @ref cool::basis::vow "vow<T>"
 * @see @ref cool::basis::aim<void> "aim<void>"
 */
template <> class vow<void> : public vow_base<void>
{
 public:
  typedef void                       result_t;
  typedef vow<void>                  this_t;
  typedef std::shared_ptr<this_t>    ptr_t;

 public:
  vow(const vow&)              = default;
  vow& operator= (const vow&)  = default;
  vow();

  /**
   * Store the exception into the shared state and make it ready.
   *
   * @param exc std::exception_ptr to the exception object to be stored
   *            into the shared state.
   *
   * @exception cool::exception::illegal_state thrown if the shared
   *   state associated with the vow object was already made ready.
   *
   * @note The call to set() will invoke the user callback if one was set
   *   through one of then() overloads of associated  cool::basis::aim object.
   */
  void set(const std::exception_ptr& exc) const;
  /**
   * Make the shared state ready.
   *
   * @exception cool::exception::illegal_state thrown if the shared
   *   state associated with the vow object was already made ready.
   *
   * @note The call to set() will invoke the user callback if one was set
   *   through one of then() overloads of associated  cool::basis::aim object.
   */
  dlldecl void set() const;
};

// --------------------------------------------------------------------------
//
// aim implementation
//
// --------------------------------------------------------------------------

template <typename T>
aim_base<T>::aim_base(const typename state_t::ptr_t& state) : m_state(state)
{ /* noop */ }

template <typename T>
T aim_base<T>::get()
{
  std::unique_lock<std::mutex> lock(m_state->mutex());

  m_state->cv().wait(lock, [this] () { return !m_state->is_empty(); });

  if (!m_state->is_set())   // if already wasted through callbacks
    throw exception::illegal_state("Results were already consumed");

  if (m_state->is_failure())
    std::rethrow_exception(m_state->failure());

  return m_state->get_result();
}

template <typename T> template <typename Rep, typename Period>
T aim_base<T>::get(const std::chrono::duration<Rep, Period>& interval)
{
  std::unique_lock<std::mutex> lock(m_state->mutex());

  if (!m_state->cv().wait_for(lock, interval, [this]() { return !m_state->is_empty(); }))
    throw exception::timeout("timeout expired");

  if (!m_state->is_set())   // if already wasted through callbacks
    throw exception::illegal_state("Results were already consumed");

  if (m_state->is_failure())
    std::rethrow_exception(m_state->failure());

  return m_state->get_result();
}

template <typename T> template <typename Clock, typename Duration>
T aim_base<T>::get(const std::chrono::time_point<Clock, Duration>& tp)
{
  std::unique_lock<std::mutex> lock(m_state->mutex());

  if (!m_state->cv().wait_until(lock, tp, [&]() { return !m_state->is_empty(); }))
    throw exception::timeout("timeout expired");

  if (!m_state->is_set())   // if already wasted through callbacks
    throw exception::illegal_state("Results were already consumed");

  if (m_state->is_failure())
    std::rethrow_exception(m_state->failure());

  return m_state->get_result();
}

template <typename T>
bool aim_base<T>::then_base(bool& fail
                        , const typename state_t::success_cb_t& scb
                        , const typename state_t::failure_cb_t& fcb)
{
  bool fire = false;

  std::unique_lock<std::mutex> l(m_state->mutex());

  fire = m_state->is_set();
  if (fire)
  {
    fail = m_state->is_failure();
    m_state->deplete();
  }
  else
  {
    m_state->on_success(scb);
    m_state->on_failure(fcb);
  }

  return fire;
}


// ------------------------------------
template <typename T>
aim<T>::aim(const typename aim_base<T>::state_t::ptr_t& state) : aim_base<T>(state)
{ /* noop */ }

template <typename T>
void aim<T>::then(const typename aim<T>::success_cb_t& scb,
                  const typename aim<T>::failure_cb_t& fcb)
{
  bool fail;
  // if the result is already available fire immediatelly
  if (aim_base<T>::then_base(fail, scb, fcb))
  {
    try
    {
      if (fail)
      {
        if (fcb)
          fcb(aim_base<T>::m_state->failure());
      }
      else
      {
        if (scb)
          scb(aim_base<T>::m_state->get_result());
      }
    }
    catch (...)
    { /* noop */ }
  }
}

template <typename T>
typename aim<T>::this_t aim<T>::then(const typename aim_base<T>::state_t::chained_cb_t& ccb)
{
  typename aim_base<T>::vow_t* p = new typename aim_base<T>::vow_t();
  this_t h = p->get_aim();

  then(
    [=] (const T& val)
    {
      try {
        ccb(*p, val);
      }
      catch (...) {
        p->set(std::current_exception());
      }
      delete p;
    },
    [=] (const std::exception_ptr& err)
    {
      p->set(err);
      delete p;
    }
  );

  return h;
}

template <typename T> template <typename Y>
aim<Y> aim<T>::then(const std::function<void (const vow<Y>&, const T&)>& ccb)
{
  vow<Y>* p = new vow<Y>();
  auto h = p->get_aim();

  then(
    [=] (const T& val)
    {
      try {
        ccb(*p, val);
      }
      catch (...) {
        p->set(std::current_exception());
      }
      delete p;
    },
    [=] (const std::exception_ptr& err)
    {
      p->set(err);
      delete p;
    }
  );

  return h;
}

// ------------------------------------
inline aim<void>::aim(const aim_base<void>::state_t::ptr_t& state) : aim_base<void>(state)
{ /* noop */ }

template <typename Y>
aim<Y> aim<void>::then(const std::function<void (const vow<Y>&)>& ccb)
{
  vow<Y>* p = new vow<Y>();
  auto h = p->get_aim();

  then(
    [=] ()
    {
      try {
        ccb(*p);
      }
      catch (...) {
        p->set(std::current_exception());
      }
      delete p;
    },
    [=] (const std::exception_ptr& err)
    {
       p->set(err);
       delete p;
     }
  );

  return h;
}


// --------------------------------------------------------------------------
//
// vow implementation
//
// --------------------------------------------------------------------------
template <typename T>
vow_base<T>::vow_base() : m_state(std::make_shared<state_t>())
{ /* noop */ }

template <typename T>
vow_base<T>::vow_base(const vow_base& other)
{
  m_state = other.m_state;
  m_state->inc_vow_ref_cnt();
}

template <typename T>
vow_base<T>& vow_base<T>::operator =(const vow_base& other)
{
  m_state = other.m_state;
  m_state->inc_vow_ref_cnt();
  return *this;
}

template <typename T>
vow_base<T>::~vow_base()
{
  if (m_state->dec_vow_ref_cnt() == 0)
  {
    if (m_state->is_empty())
      m_state->failure(std::make_exception_ptr(cool::exception::broken_vow("Result was not set.")));
  }
}

template <typename T>
void vow_base<T>::set(const std::exception_ptr& err) const
{
  typename state_t::failure_cb_t fcb;

  {
    std::unique_lock<std::mutex> l(m_state->mutex());

    if (!m_state->is_empty())
      throw exception::illegal_state("This vow was already set.");

    m_state->failure(err);

    fcb = std::move(m_state->on_failure());
    if (fcb)
      m_state->deplete();
    else
      m_state->cv().notify_all();
  }

  if (fcb)
  {
    fcb(m_state->failure());
  }
}

template <typename T>
bool vow_base<T>::is_set() const
{
  return m_state->is_set();
}

template <typename T>
typename vow_base<T>::aim_t vow_base<T>::get_aim() const
{
  return aim_t(m_state);
}

// -----------------------------
template <typename T>
vow<T>::vow() : vow_base<T>()
{ /* noop */ }

template <typename T>
void vow<T>::set(const std::exception_ptr& err) const
{
  vow_base<T>::set(err);
}

template <typename T>
void vow<T>::set(const T& value) const
{
  typename vow_base<T>::state_t::success_cb_t scb;

  {
    std::unique_lock<std::mutex> l (vow_base<T>::m_state->mutex());

    if (!vow_base<T>::m_state->is_empty())
      throw exception::illegal_state("This vow was already set.");

    vow_base<T>::m_state->set_result(value);
    scb = std::move(vow_base<T>::m_state->on_success());
    if (scb)
      vow_base<T>::m_state->deplete();
    else
      vow_base<T>::m_state->cv().notify_all();
  }

  if (scb)
  {
    scb(vow_base<T>::m_state->get_result());
  }
}

// -----------------------------
inline vow<void>::vow() : vow_base<void>()
{ /* noop */ }
inline void vow<void>::set(const std::exception_ptr& err) const
{
  vow_base<void>::set(err);
}

} } // namespace

#endif
