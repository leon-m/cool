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
        FAIL() << "Should not get here";
      }).
  then(
      [](const std::exception_ptr& ex){
        FAIL() << "Should not get here";
      },
      [](){
        FAIL() << "Should not get here";
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





