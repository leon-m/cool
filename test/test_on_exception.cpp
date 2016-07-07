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

using namespace cool::basis;


// Test for backward compatibility. Tasks with error handlers break execution chain
TEST(on_exception, error_handling_tasks)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();

  std::mutex mutexWait;
  std::condition_variable cvWait;
  bool ok = false;

  auto&& task = cool::gcd::task::factory::create(r,
      [](){
        std::cout << "task 1 throwing" << std::endl;
        throw std::range_error("TestError");
      }).
  then(
      [&mutexWait, &cvWait, &ok](const std::exception_ptr& ex){
        std::cout << "expected" << std::endl;
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

  task.run();

  std::unique_lock<std::mutex> l(mutexWait);
  cvWait.wait(l);

  ASSERT_TRUE(ok);
}

TEST(on_exception, on_any_exception_void)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();

  std::mutex mutexWait;
  std::condition_variable cvWait;
  bool ok = false;

  auto&& task = cool::gcd::task::factory::create(r,
      [](){
        std::cout << "task 1 throwing" << std::endl;
        throw std::range_error("TestError");
      }).
//  then(
//      [](){
//        std::cout << "You should not see this. This task is skipped." << std::endl;
//      }).
  on_any_exception(r,
      [](const std::exception_ptr& ex_ptr){
        try {
          std::rethrow_exception(ex_ptr);
        }
        catch (const std::range_error& ex) {
          std::cout << __LINE__ << " handling: " << ex.what() << std::endl;
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

  task.run();

  std::unique_lock<std::mutex> l(mutexWait);
  cvWait.wait(l);

  ASSERT_TRUE(ok);
}

TEST(on_exception, on_any_exception_typed)
{
  auto&& r = std::make_shared<cool::gcd::task::runner>();

  std::mutex mutexWait;
  std::condition_variable cvWait;
  std::string result("");

  auto&& task = cool::gcd::task::factory::create(r,
      []()->std::string{
        std::cout << "task 1 throwing" << std::endl;
        throw std::range_error("TestError");
      }).
//  then(
//      [](){
//        std::cout << "You should not see this. This task is skipped." << std::endl;
//      }).
  on_any_exception(r,
      [](const std::exception_ptr& ex_ptr)->std::string{
        try {
          std::rethrow_exception(ex_ptr);
        }
        catch (const std::range_error& ex) {
          std::cout << __LINE__ << " handling: " << ex.what() << std::endl;
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

  task.run();

  std::unique_lock<std::mutex> l(mutexWait);
  cvWait.wait(l);

  ASSERT_EQ("Correct", result);
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

  task.run();

  std::unique_lock<std::mutex> l(mutexWait);
  cvWait.wait(l);

  ASSERT_EQ(3, n);
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
        std::cout << "task 1 throwing" << std::endl;
        throw std::range_error("TestError");
      }).
//  then(
//      [](){
//        std::cout << "You should not see this. This task is skipped." << std::endl;
//      }).
  on_exception(r,
      [](const std::range_error& ex){
        std::cout << __LINE__ << " handling: " << ex.what() << std::endl;
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

  task.run();

  std::unique_lock<std::mutex> l(mutexWait);
  cvWait.wait(l);

  ASSERT_TRUE(ok);
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
        std::cout << "task 1 throwing" << std::endl;
        throw std::range_error("TestError");
      }).
//  then(
//      [](){
//        std::cout << "You should not see this. This task is skipped." << std::endl;
//      }).
  on_exception(r,
      [](const std::range_error& ex)->std::string{
        std::cout << __LINE__ << " handling: " << ex.what() << std::endl;
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

  task.run();

  std::unique_lock<std::mutex> l(mutexWait);
  cvWait.wait(l);

  ASSERT_EQ("Correct", result);
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
        std::cout << "task 1 throwing" << std::endl;
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

  task.run();

  std::unique_lock<std::mutex> l(mutexWait);
  cvWait.wait(l);

  ASSERT_EQ(1, n);
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

  cool::gcd::task::factory::create(r,
      []()->int{
        //throw std::runtime_error("Test error");
        throw std::range_error("Can't do.");
      }).
//  then(
//      [](std::string result)->std::string{
//        std::cout << "You should not see this. This task is skipped." << std::endl;
//        return "Oops" + result;
//      }).
  on_exception(r,
      [](const std::underflow_error& ex) {
        std::cout << "Should not get here. Incorrect exception type";
        return -__LINE__;
      }).
  on_exception(r,
      [](const std::range_error& ex) {
        std::cout << __LINE__ << " handling: " << ex.what() << std::endl;
        return 57;
      }).
  on_exception(r,
      [](const std::runtime_error& ex) {
        std::cout << "Should not get here.";
        return -__LINE__;
      }).
  on_any_exception(r,
      [](const std::exception_ptr& ex_ptr) {
        std::cout << "Should not get here.";
        return -__LINE__;
      }).
//  then(
//      [](const std::string& result){
//        std::cout << "Result: " << result << std::endl;
//        return 0;
//      }).
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
        std::cerr << "Bad things happened" << std::endl;
      }).
  run();

  std::unique_lock<std::mutex> l(mutexWait);
  cvWait.wait(l);

  ASSERT_EQ(57, result);
}





