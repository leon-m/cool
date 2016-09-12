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

#include <chrono>
#include <thread>

#include <gtest/gtest.h>
#include "cool/gcd_task.h"

using ms = std::chrono::milliseconds;

using namespace cool::basis;

// Test for backward compatibility. Tasks with error handlers break execution chain
TEST(on_exception, error_handling_tasks)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();

  std::mutex mutexWait;
  std::condition_variable cvWait;
  bool ok = false;

  auto&& task = cool::gcd::task::factory::create(r,
      [](){ }).
  then(
      r,
      [](const std::exception_ptr& ex){
        FAIL() << "Should not get here. No error";
      },
      [](){
        throw std::range_error("");
      }).
  then(
      [&mutexWait, &cvWait, &ok](const std::exception_ptr& ex){
        std::unique_lock<std::mutex> l(mutexWait);
        ok = true;
        cvWait.notify_one();
      },
      [](){
        FAIL() << "Should not get here. Error handler should be called.";
      }).
  then(
      [](const std::exception_ptr& ex){
        FAIL() << "Should not get here. Error handler above breaks the chain";
      },
      [](){
        FAIL() << "Should not get here. Error handler above breaks the chain";
      });

  std::unique_lock<std::mutex> l(mutexWait);

  task.run();

  EXPECT_TRUE(cvWait.wait_for(l, ms(100), [&ok] { return ok; }));
  EXPECT_TRUE(ok);
}

TEST(on_exception, on_any_exception_void)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();

  std::mutex mutexWait;
  std::condition_variable cvWait;
  bool ok = false;

  auto&& task = cool::gcd::task::factory::create(r,
      [](){
        throw std::range_error("TestError");
      }).
  then_do(
      [](){
        FAIL() << "You should not see this. This task is skipped.";
      }).
  on_any_exception(r,
      [](const std::exception_ptr& ex_ptr){
        try {
          std::rethrow_exception(ex_ptr);
        }
        catch (const std::range_error& ex) {
        }
      }).
  then(
      [](const std::exception_ptr& ex){
        FAIL() << "Should not get here";
      },
      [&mutexWait, &cvWait, &ok](){
        std::unique_lock<std::mutex> l(mutexWait);
        ok = true;
        cvWait.notify_one();
      });

  std::unique_lock<std::mutex> l(mutexWait);
  task.run();

  EXPECT_TRUE(cvWait.wait_for(l, ms(100), [&ok] { return ok; }));
  EXPECT_TRUE(ok);
}

TEST(on_exception, on_any_exception_typed)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();

  std::mutex mutexWait;
  std::condition_variable cvWait;
  std::string result("");

  auto&& task = cool::gcd::task::factory::create(r,
      []()->std::string{
        throw std::range_error("TestError");
      }).
  then_do(
      [](const std::string&)->std::string{
        ADD_FAILURE(); return""; //This task should be skipped
      }).
  on_any_exception(r,
      [](const std::exception_ptr& ex_ptr)->std::string{
        try {
          std::rethrow_exception(ex_ptr);
        }
        catch (const std::range_error& ex) {
          return "Correct";
        }
      }).
  then(
      [](const std::exception_ptr& ex){
        FAIL() << "Should not get here";
      },
      [&mutexWait, &cvWait, &result](const std::string& r){
        std::unique_lock<std::mutex> l(mutexWait);
        result = r;
        cvWait.notify_one();
      });

  std::unique_lock<std::mutex> l(mutexWait);
  task.run();

  EXPECT_TRUE(cvWait.wait_for(l, ms(100), [&result] { return result != ""; } ));
  EXPECT_EQ("Correct", result);
}

// Test if various variants compile and are executed
TEST(on_exception, on_any_exception_variants)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();

  std::mutex mutexWait;
  std::condition_variable cvWait;
  int n = 0;

  // no error handler handles the exception so all are executed
  auto&& task = cool::gcd::task::factory::create(r,
      [](){ throw std::range_error("TestError"); })
  .on_any_exception(r,
      [&n](const std::exception_ptr& ex_ptr){ ++n; std::rethrow_exception(ex_ptr); })
  .on_any_exception( /* no runner */
      [&n](const std::exception_ptr& ex_ptr){ ++n; std::rethrow_exception(ex_ptr); })
  .then(
      [&mutexWait, &cvWait, &n](const std::exception_ptr&){
        std::unique_lock<std::mutex> l(mutexWait);
        ++n;
        cvWait.notify_one();
      },
      [](){
        FAIL() << "Should not get here";
      });

  std::unique_lock<std::mutex> l(mutexWait);
  task.run();

  EXPECT_TRUE(cvWait.wait_for(l, ms(100), [&n] { return n!= 0; } ));
  EXPECT_EQ(3, n);
}

// Test for on_exception handling concrete type of exception.
TEST(on_exception, on_exception_void)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();

  std::mutex mutexWait;
  std::condition_variable cvWait;
  bool ok = false;

  auto&& task = cool::gcd::task::factory::create(r,
      [](){
        throw std::range_error("TestError");
      }).
  then_do(
      [](){
        ADD_FAILURE(); //This task should be skipped
      }).
  on_exception(r,
      [](const std::range_error& ex){
      }).
  then(
      [](const std::exception_ptr& ex){
        FAIL() << "Should not get here";
      },
      [&mutexWait, &cvWait, &ok](){
        std::unique_lock<std::mutex> l(mutexWait);
        ok = true;
        cvWait.notify_one();
      });

  std::unique_lock<std::mutex> l(mutexWait);
  task.run();

  EXPECT_TRUE(cvWait.wait_for(l, ms(100), [&ok] { return ok; }));
  EXPECT_TRUE(ok);
}

// Test for on_exception handling concrete type of exception.
TEST(on_exception, on_exception_typed)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();

  std::mutex mutexWait;
  std::condition_variable cvWait;
  std::string result("");

  auto&& task = cool::gcd::task::factory::create(r,
      []()->std::string{
        throw std::range_error("TestError");
      }).
  then_do(
      [](const std::string&)->std::string{
        ADD_FAILURE(); return ""; //This task should be skipped
      }).
  on_exception(r,
      [](const std::range_error& ex)->std::string{
        return "Correct";
      }).
  then(
      [](const std::exception_ptr& ex){
        FAIL() << "Should not get here";
      },
      [&mutexWait, &cvWait, &result](const std::string& r){
        std::unique_lock<std::mutex> l(mutexWait);
        result = r;
        cvWait.notify_one();
      });

  std::unique_lock<std::mutex> l(mutexWait);
  task.run();

  EXPECT_TRUE(cvWait.wait_for(l,  ms(100), [&result] { return result != ""; }));
  EXPECT_EQ("Correct", result);
}

// Test if various variants compile and are executed
TEST(on_exception, on_exception_variants)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();

  std::mutex mutexWait;
  std::condition_variable cvWait;
  int n = 0;

  // no error handler takes the right exception type so none are executed
  auto&& task = cool::gcd::task::factory::create(r,
      [](){
        throw std::overflow_error("TestError");
      })
  .on_exception(r, // int as exception is regular code
      [&n](int ex_ptr){ ++n; })
  .on_exception( // int as exception is regular code
      [&n](int ex_ptr){ ++n; })
  .on_exception(r,
      [&n](const std::range_error& ex){ ++n; })
  .on_exception( // no runner
      [&n](const std::range_error& ex){ ++n; })
  .then(
      [&mutexWait, &cvWait, &n](const std::exception_ptr&){
        std::unique_lock<std::mutex> l(mutexWait);
        ++n;
        cvWait.notify_one();
      },
      [](){
        FAIL() << "Should not get here";
      });

  std::unique_lock<std::mutex> l(mutexWait);
  task.run();

  EXPECT_TRUE(cvWait.wait_for(l, ms(100), [&n] { return n != 0; }));
  EXPECT_EQ(1, n);
}

// Test that irregular code doesn't compile
TEST(on_exception, on_exception_no_compile)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();

  auto&& void_task = cool::gcd::task::factory::create(r,
      [](){ })
#if defined(COMPILE_TESTS)
  .on_exception(r,
      // must have 1 parameter
      [](const std::exception_ptr& ex_ptr, const int& i){ })
  .on_exception(
      // must have 1 parameter
      [](const std::exception_ptr& ex_ptr, const int& i){ })
  .on_exception(r,
      // must have 1 parameter
      [](){ })
  .on_exception(
      // must have 1 parameter
      [](){ })
  .on_exception(r,
      // return type must match
      [](const std::exception_ptr& ex_ptr){ return 0; })
  .on_exception(
      // return type must match
      [](const std::exception_ptr& ex_ptr){ return 0; })
#endif
  ;

  auto&& typed_task = cool::gcd::task::factory::create(r,
      []()->int{ return 57; })
#if defined(COMPILE_TESTS)
  .on_exception(r,
      // must have 1 parameter
      [](const std::exception_ptr& ex_ptr, const int& i){ return 0; })
  .on_exception(
      // must have 1 parameter
      [](const std::exception_ptr& ex_ptr, const int& i){ return 0; })
  .on_exception(r,
      // must have 1 parameter
      [](){ return 0; })
  .on_exception(
      // must have 1 parameter
      [](){ return 0; })
  .on_exception(r,
      // return type must match
      [](const std::exception_ptr& ex_ptr){ return ""; })
  .on_exception(
      // return type must match
      [](const std::exception_ptr& ex_ptr){ return ""; })
#endif
  ;
}

// Testing variants of tasks in one chain
TEST(on_exception, task_chain)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();

  std::mutex mutexWait;
  std::condition_variable cvWait;
  int result = 0;

  std::unique_lock<std::mutex> l(mutexWait);

  cool::gcd::task::factory::create(r,
      []()->int{
        throw std::range_error("Test error");
      }).
  then_do(
      [](int)->int{
        ADD_FAILURE(); return 34; //This task should be skipped
      }).
  on_exception(r,
      [](const std::underflow_error& ex) {
        ADD_FAILURE(); return -__LINE__;
      }).
  on_exception(r,
      [](const std::range_error& ex) {
        return 57;
      }).
  on_exception(r,
      [](const std::runtime_error& ex) {
        ADD_FAILURE(); return -__LINE__;
      }).
  on_any_exception(r,
      [](const std::exception_ptr& ex_ptr) {
        ADD_FAILURE(); return -__LINE__;
      }).
  then_do(
      [](const int&i)->int{
        return i;
      }).
  then(
      [](const std::exception_ptr& ex){
        FAIL() << "Should not get here";
      },
      [&mutexWait, &cvWait, &result](const int& r){
        std::unique_lock<std::mutex> l(mutexWait);
        result = r;
        cvWait.notify_one();
      }).
  finally(
      [](const std::exception_ptr&){
        FAIL() << "Should not get here";
      }).
  run();

  EXPECT_TRUE(cvWait.wait_for(l, ms(100), [&result] { return result != 0; }));
  EXPECT_EQ(57, result);
}

// Test variants of then tasks returning void
TEST(then, then_void_variants)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();

  std::mutex mutexWait;
  std::condition_variable cvWait;
  int n = 0;

  int i = -1;
  auto&& task = cool::gcd::task::factory::create(r,
      [](){})

  // tasks under test
#if !defined(INCORRECT_VARIADIC)
  .then_do(r,                                           [&n](int){ ++n; }, i)
  .then_do(                                             [&n](int){ ++n; }, i)
  .then   (r, [](const std::exception_ptr&){ FAIL(); }, [&n](int){ ++n; }, i)
  .then   (   [](const std::exception_ptr&){ FAIL(); }, [&n](int){ ++n; }, i)
#endif
  .then_do(r,                                           [&n](){ ++n; })
  .then_do(                                             [&n](){ ++n; })
  .then(   r, [](const std::exception_ptr&){ FAIL();},  [&n](){ ++n; })
  .then(      [](const std::exception_ptr&){ FAIL();},  [&n](){ ++n; })
  // end of tested tasks

  .then(
      [](const std::exception_ptr&){ FAIL();},
      [&mutexWait, &cvWait](){
        std::unique_lock<std::mutex> l(mutexWait);
        cvWait.notify_one();
      })
  ;

  std::unique_lock<std::mutex> l(mutexWait);
  task.run();

  EXPECT_TRUE(cvWait.wait_for(l, ms(100), [&n] { return n != 0; }));

#if !defined(INCORRECT_VARIADIC)
  EXPECT_EQ(8, n);
#else
  EXPECT_EQ(4, n);
#endif
}


// Test variants of then tasks returning result
TEST(then, then_typed_variants)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();

  std::mutex mutexWait;
  std::condition_variable cvWait;

  int result = 0;
  int i = 1;
  auto&& task = cool::gcd::task::factory::create(r,
      [](){ return 0; })

  // tasks under test
#if !defined(INCORRECT_VARIADIC)
  .then_do(r,                                           [](int r, int p){ return r+p; }, i)
  .then_do(                                             [](int r, int p){ return r+p; }, i)
  .then   (r, [](const std::exception_ptr&){ FAIL(); }, [](int r, int p){ return r+p; }, i)
  .then   (   [](const std::exception_ptr&){ FAIL(); }, [](int r, int p){ return r+p; }, i)
#endif
  .then_do(r,                                           [](int r){ return ++r; })
  .then_do(                                             [](int r){ return ++r; })
  .then(   r, [](const std::exception_ptr&){ FAIL();},  [](int r){ return ++r; })
  .then(      [](const std::exception_ptr&){ FAIL();},  [](int r){ return ++r; })
  // end of tested tasks

  .then(
      [](const std::exception_ptr&){ FAIL();},
      [&result, &mutexWait, &cvWait](int r){
        result = r;
        std::unique_lock<std::mutex> l(mutexWait);
        cvWait.notify_one();
      })
  ;

  std::unique_lock<std::mutex> l(mutexWait);
  task.run();

  EXPECT_TRUE(cvWait.wait_for(l, ms(100), [&result] { return result != 0; }));

#if !defined(INCORRECT_VARIADIC)
  EXPECT_EQ(8, result);
#else
  EXPECT_EQ(4, result);
#endif
}



TEST(then_with_task, no_finally)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();
  auto&& r2 = std::make_shared<cool::gcd::task::runner>();

  std::mutex mutexWait;
  std::condition_variable cvWait;

  int result = 0;

  {
    result = 0;
    std::unique_lock<std::mutex> l(mutexWait);

#if !defined(INCORRECT_VARIADIC)
    auto&& task_a = cool::gcd::task::factory::create(
         r
       , [ &result ] (int n)
         {
           result =  n+1;
         }
      , 1);
#else
    int n = 1;
    auto&& task_a = cool::gcd::task::factory::create(
        r
      , [ &result, n ] ()
      {
        result =  n+1;
      });
#endif
    auto&& task_b = cool::gcd::task::factory::create(
        r
      , [&result]
        {
          result = result + 1;
        })
    .then_do(
       r,
       [&mutexWait, &cvWait]( )
       {
         std::unique_lock<std::mutex> l(mutexWait);
         cvWait.notify_one();
       });

    EXPECT_NO_THROW(task_a.then_add(task_b).run());

    EXPECT_TRUE(cvWait.wait_for(l, ms(100), [&result] { return result != 0; } ));
    EXPECT_EQ(3, result);
  }
  {
    result = 0;
    std::unique_lock<std::mutex> l(mutexWait);

#if !defined(INCORRECT_VARIADIC)
    auto&& task_a = cool::gcd::task::factory::create(
        r, [ ] (int n) { return n+1; }, 1)
#else
    int n = 1;
    auto&& task_a = cool::gcd::task::factory::create(
        r, [n] () { return n+1; })
#endif
    .then_do([&result] (int n) { result = n + 1; });

    auto&& task_b = cool::gcd::task::factory::create(
        r2
      , [&result] () { return result + 1; })
    .then_do(r, [&result] (int n) { result = n + 1; })
    .then_do(
        r
      , [&mutexWait, &cvWait, &result]()
        {
          std::unique_lock<std::mutex> l(mutexWait);
          cvWait.notify_one();
        }
    );

    EXPECT_NO_THROW(task_a.then_add(task_b).run());

    EXPECT_TRUE(cvWait.wait_for(l, ms(100), [&result] { return result != 0; } ));
    EXPECT_EQ(5, result);
  }
}

TEST(then_with_task, failed_operation)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();

  std::mutex mutexWait;
  std::condition_variable cvWait;

  int result = 0;

  {
    result = 0;
    std::unique_lock<std::mutex> l(mutexWait);

    auto&& task_a = cool::gcd::task::factory::create(
        r
      , [&result]
        {
          result = 5;
        }
    );

    auto&& task_b = cool::gcd::task::factory::create(
        r
      , [&result, &mutexWait, &cvWait] ()
      {
        result = 10;
        std::unique_lock<std::mutex> l(mutexWait);
        cvWait.notify_one();
      }
    );

    task_b.run();
    EXPECT_TRUE(cvWait.wait_for(l, ms(100), [&result] { return result != 0; } ));
    EXPECT_EQ(10, result);
    EXPECT_THROW(task_a.then_add(task_b).run(), cool::exception::illegal_argument);
  }

  {
    result = 0;
    std::unique_lock<std::mutex> l(mutexWait);

    auto&& task_a = cool::gcd::task::factory::create(
        r
      , [&result, &mutexWait, &cvWait]
        {
          result = 5;
          std::unique_lock<std::mutex> l(mutexWait);
          cvWait.notify_one();
        }
    );

    auto&& task_b = cool::gcd::task::factory::create(
        r
      , [] () { return 5; }
    );

    task_a.run();
    EXPECT_TRUE(cvWait.wait_for(l, ms(100), [&result] { return result != 0; } ));
    EXPECT_EQ(5, result);
    EXPECT_THROW(task_a.then_add(task_b).run(), cool::exception::illegal_state);
  }

  {
    result = 0;
    std::unique_lock<std::mutex> l(mutexWait);

    auto&& task_a = cool::gcd::task::factory::create(
        r
      , [&result]
        {
          result = 5;
        })
    .finally([] (const std::exception_ptr& e) { });

    auto&& task_b = cool::gcd::task::factory::create(
        r
      , [] () { return 5; }
    );
//    task_a.show();
    EXPECT_THROW(task_a.then_add(task_b).run(), cool::exception::operation_failed);
  }
}
