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

#include <chrono>
#include <thread>

#include <gtest/gtest.h>
#include "cool/gcd_task.h"

using namespace cool::basis;

#define NOT_FIXED_RVAL
// compilation tests to test template parameters to run method
// ultimate goal is to have no if 0'ed code

int f() { return 42; }

// 1. no parameters
cool::basis::aim<int> r00001()
{
  return cool::gcd::task::runner::sys_default().run([] () { return 42; } );
}
/*
                               by-value   by-const-lvalue-ref  by-lvalue ref   by-rvalue-ref

   literal                      Yes        Yes                  No              Yes

   func-cal                     Yes        Yes                  No              Yes

   static-const                 Yes        Yes                  No              No

   const                        Yes        Yes                  No              No

   lvalue-ref                   Yes        Yes                  Yes             No

   const-lvalue-ref             Yes        Yes                  No              No

   local variable               Yes        Yes                  Yes             Yes
*/

#if !defined(INCORRECT_VARIADIC)
// -------- use rvalue (literal)
#if !defined(NOT_FIXED_RVAL)
cool::basis::aim<int> r00002()
{
  return cool::gcd::task::runner::sys_default().run([] (int n) -> int { return n; } , 42 );
}
cool::basis::aim<int> r00003()
{
  return cool::gcd::task::runner::sys_default().run([] (const int& n) -> int { return n; } , 42 );
}
cool::basis::aim<int> r00004()
{
  return cool::gcd::task::runner::sys_default().run([] (int&& n) -> int { return n; } , 42 );
}
#endif
#if !defined(NOT_FIXED_RVAL)
// -------- use rvalue (function call)
cool::basis::aim<int> r00005()
{
  return cool::gcd::task::runner::sys_default().run([] (int n) -> int { return n; } , f() );
}
cool::basis::aim<int> r00006()
{
  return cool::gcd::task::runner::sys_default().run([] (const int& n) -> int { return n; } , f() );
}
cool::basis::aim<int> r00007()
{
  return cool::gcd::task::runner::sys_default().run([] (int&& n) -> int { return n; } , f() );
}
#endif
// --------- use lvalue (static const)
cool::basis::aim<int> r00008()
{
  static const int c = 42;
  return cool::gcd::task::runner::sys_default().run([] (int n) -> int { return n; } , c );
}
cool::basis::aim<int> r00009()
{
  static const int c = 42;
  return cool::gcd::task::runner::sys_default().run([] (const int& n) -> int { return n; } , c );
}

// --------- use lvalue (const)
cool::basis::aim<int> r00010()
{
  const int c = 42;
  return cool::gcd::task::runner::sys_default().run([] (int n) -> int { return n; } , c );
}
cool::basis::aim<int> r00011()
{
  const int c = 42;
  return cool::gcd::task::runner::sys_default().run([] (const int& n) -> int { return n; } , c );
}

// --------- use const ref
cool::basis::aim<int> r00012()
{
  int n = 42; const int& c = n;
  return cool::gcd::task::runner::sys_default().run([] (int n) -> int { return n; } , c );
}
cool::basis::aim<int> r00013()
{
  int n = 42; const int c = n;
  return cool::gcd::task::runner::sys_default().run([] (const int& n) -> int { return n; } , c );
}

// --------- use  ref
cool::basis::aim<int> r00014()
{
  int n = 42; int& c = n;
  return cool::gcd::task::runner::sys_default().run([] (int n) -> int { return n; } , c );
}
cool::basis::aim<int> r00015()
{
  int n = 42; int c = n;
  return cool::gcd::task::runner::sys_default().run([] (const int& n) -> int { return n; } , c );
}
cool::basis::aim<int> r00016()
{
  int n = 42; int& c = n;
  return cool::gcd::task::runner::sys_default().run([] (int& n) -> int { return n; } , c );
}

// --------- use  local variable
cool::basis::aim<int> r00017()
{
  int c = 42;
  return cool::gcd::task::runner::sys_default().run([] (int n) -> int { return n; } , c );
}
cool::basis::aim<int> r00018()
{
  int c = 42;
  return cool::gcd::task::runner::sys_default().run([] (const int& n) -> int { return n; } , c );
}
cool::basis::aim<int> r00019()
{
  int c = 42;
  return cool::gcd::task::runner::sys_default().run([] (int& n) -> int { return n; } , c );
}
#if !defined(NOT_FIXED_RVAL)
cool::basis::aim<int> r00020()
{
  int c = 42;
  return cool::gcd::task::runner::sys_default().run([] (int&& n) -> int { return n; } , std::move(c) );
}
#endif

#endif

TEST(runner, run)
{
  EXPECT_EQ(r00001().get(), 42);
#if !defined(INCORRECT_VARIADIC)
#if !defined(NOT_FIXED_RVAL)
  EXPECT_EQ(r00002().get(), 42);
  EXPECT_EQ(r00003().get(), 42);
  EXPECT_EQ(r00004().get(), 42);
  EXPECT_EQ(r00005().get(), 42);
  EXPECT_EQ(r00006().get(), 42);
  EXPECT_EQ(r00007().get(), 42);
#endif
  EXPECT_EQ(r00008().get(), 42);
  EXPECT_EQ(r00009().get(), 42);
  EXPECT_EQ(r00010().get(), 42);
  EXPECT_EQ(r00012().get(), 42);
  EXPECT_EQ(r00012().get(), 42);
  EXPECT_EQ(r00013().get(), 42);
  EXPECT_EQ(r00014().get(), 42);
  EXPECT_EQ(r00015().get(), 42);
  EXPECT_EQ(r00016().get(), 42);
  EXPECT_EQ(r00017().get(), 42);
  EXPECT_EQ(r00018().get(), 42);
  EXPECT_EQ(r00019().get(), 42);
#if !defined(NOT_FIXED_RVAL)
  EXPECT_EQ(r00020().get(), 42);
#endif
#endif
}

std::shared_ptr<cool::gcd::task::runner> test_runner(new cool::gcd::task::runner);


void t00001()
{
  int n = 42;
  cool::gcd::task::factory::create(
      test_runner
#if defined(INCORRECT_VARIADIC)
    , [n] () -> int { std::cout << "00001 - received: " << n << "\n"; return n; }
#else
    , [] (int i) -> int { std::cout << "00001 - received: " << i << "\n"; return i; }
    , n
#endif
  ).run();
}

void t00001a()
{
  cool::gcd::task::factory::create(
      test_runner
    , [] () -> int { std::cout << "00001a - received: " << 42 << "a\n"; return 42;}
  ).run();
}

void t00002()
{
  int n = 42;
  cool::gcd::task::factory::create(
        test_runner
#if defined(INCORRECT_VARIADIC)
      , [n] () { std::cout << "00002 - received: " << n << "\n"; }
#else
      , [] (int i) { std::cout << "00002 - received: " << i << "\n"; }
      , n
#endif
  ).run();
}

void t00002a()
{
  cool::gcd::task::factory::create(
        test_runner
      , [] () { std::cout << "00002a - received: " << 42 << "a\n"; }
    ).run();
}

void t00002b()
{
  cool::gcd::task::factory::create(
        test_runner
      , []() { std::cout << "00002b - first lambda received: " << 42 << "a\n"; return 42; }
    ).then(
        [](const std::exception_ptr& e) {}
      , [](int n) { std::cout << "00002b - second lambda, got " << n << "\n"; }
    ).then(
        [](const std::exception_ptr& e) {}
      , []() { std::cout << "00002b - third lambda\n"; throw 42; }
    ).finally(
        [](const std::exception_ptr& e) { std::cout << "0002b - exception\n"; }
  ).run();
}


// non-void sub-tasks on a non-void task
void t00002c()
{
  cool::gcd::task::factory::create(
        test_runner
      , []() -> double { std::cout << "00002c - first lambda received: " << 42 << "a\n"; return 42;}
    ).then(
        [](const std::exception_ptr& e) {}
      , [](double n) -> int { std::cout << "00002c - second lambda, got " << 0 << "\n"; return 42;}
    ).then(
        [](const std::exception_ptr& e) {}
      , [](int n) -> int { std::cout << "00002c - third lambda, got " << n << "\n"; throw 42; }
    ).finally(
        [](const std::exception_ptr& e) { std::cout << "0002c - exception\n"; }
  ).run();
}

// void subtask (last then) on a non-void task
void t00002d()
{
  cool::gcd::task::factory::create(
        test_runner
      , []() -> double { std::cout << "00002d - first lambda received: " << 42 << "a\n"; return 42;}
    ).then(
        [](const std::exception_ptr& e) {}
      , [](double n) -> int { std::cout << "00002d - second lambda, got " << 0 << "\n"; return 42;}
    ).then(
        [](const std::exception_ptr& e) {}
      , [](int n) -> void { std::cout << "00002d - third lambda, got " << n << "\n"; throw 42; }
    ).finally(
        [](const std::exception_ptr& e) { std::cout << "0002d - exception\n"; }
  ).run();
}

// non-void subtask on a void base task
void t00002e()
{
  cool::gcd::task::factory::create(
        test_runner
      , []() -> void { std::cout << "00002e - first lambda received: " << 42 << "a\n"; }
    ).then(
        [](const std::exception_ptr& e) {}
      , []() -> int { std::cout << "00002e - second lambda, got " << 0 << "\n"; return 42;}
    ).then(
        [](const std::exception_ptr& e) {}
      , [](int n) -> void { std::cout << "00002e - third lambda, got " << n << "\n"; throw 42; }
    ).finally(
        [](const std::exception_ptr& e) { std::cout << "0002e - exception\n"; }
  ).run();
}

// void subtask on a void base task
void t00002f()
{
  cool::gcd::task::factory::create(
        test_runner
      , []() -> void { std::cout << "00002f - first lambda received: " << 42 << "a\n"; }
    ).then(
        [](const std::exception_ptr& e) {}
      , []() -> void { std::cout << "00002f - second lambda, got " << 0 << "\n"; }
    ).then(
        [](const std::exception_ptr& e) {}
      , []() -> void { std::cout << "00002f - third lambda, got " << 0 << "\n"; throw 42; }
    ).finally(
        [](const std::exception_ptr& e) { std::cout << "0002f - exception\n"; }
  ).run();
}

#if !defined(INCORRECT_VARIADIC)
void t00003()
{
  int n = 21;
  double a = 3.14;

  cool::gcd::task::factory::create(
        test_runner
      , [] (int n, double pi) { std::cout << "00003 - first task: " << n << "\n"; return n; }
      , n, a
    ).then(
        test_runner,
        [] (const std::exception_ptr& e) { }
      , [] (int res, int n, double d) { std::cout << "00003 - second task: " << (res+n) << "\n"; return n; }
      , n, a
    ).run();
}

void t00004()
{
  int n = 21;
  cool::gcd::task::factory::create(
        test_runner
      , [] () { std::cout << "00004 - first task: " << 21 << "\n"; return 21; }
    ).then(
        [] (const std::exception_ptr& e) { }
      , [] (int res, int n) { std::cout << "00004 - second task: " << (res+n) << "\n"; }
      , n
    ).run();
}

void t00005()
{
  int n = 42;
  cool::gcd::task::factory::create(
        test_runner
      , [] () { std::cout << "00005 - first task: " << 21 << "\n"; })
    .then(
        [] (const std::exception_ptr& e) { }
      , [] (int n) { std::cout << "00005 - second task: " << n << "\n"; return n; }
      , n
    ).run();
}

void t00006()
{
  int n = 42;
  cool::gcd::task::factory::create(
        test_runner
      , [] () { std::cout << "00006 - first task: " << 21 << "\n"; throw cool::exception::operation_failed("hello world"); })
    .then(
        [] (const std::exception_ptr& e)
        {
          try { std::rethrow_exception(e); }
          catch (const std::exception& ee) { std::cout << "00006 - caught exception: " << ee.what() << "\n"; }
        }
      , [n] () -> int { std::cout << "00006 - second task: " << n << "\n"; return n; }
    ).run();
}

void t00007()
{
  cool::gcd::task::factory::create(
        test_runner
      , [] () { std::cout << "00007 - first task: " << 21 << "\n"; throw cool::exception::operation_failed("hello world"); })
    .finally(
        [] (const std::exception_ptr& e)
        {
          try { std::rethrow_exception(e); }
          catch (const std::exception& ee) { std::cout << "00007 - caught exception: " << ee.what() << "\n"; }
        })
    .run();
}

void t00007a()
{
  cool::gcd::task::factory::create(
        test_runner
      , [] () { std::cout << "00007a - first task: " << 21 << "\n"; })
    .finally(
        [] (const std::exception_ptr& e)
        {
          try { std::rethrow_exception(e); }
          catch (const std::exception& ee) { std::cout << "00007a - caught exception: " << ee.what() << "\n"; }
        })
    .run();
}
void t00008()
{
  int n = 42;
  cool::gcd::task::factory::create(
        test_runner
      , []() { std::cout << "00008 - first task: " << 21 << "\n"; return 42; }
    ).then(
        [] (const std::exception_ptr& e)
        {
          try { std::rethrow_exception(e); }
          catch (const std::exception& ee) { std::cout << "00008 - 2 caught exception: " << ee.what() << "\n"; }
        }
      , [n] (int res) { std::cout << "00008 - second task: " << n << "\n"; return n; }
    ).then(
        [] (const std::exception_ptr& e)
        {
          try { std::rethrow_exception(e); }
          catch (const std::exception& ee) { std::cout << "00008 - 3 caught exception: " << ee.what() << "\n"; }
        }
      , [n] (int a) { std::cout << "00008 - third task: " << n << "\n";}
    ).then(
        [] (const std::exception_ptr& e)
        {
          try { std::rethrow_exception(e); }
          catch (const std::exception& ee) { std::cout << "00008 - 4 caught exception: " << ee.what() << "\n"; }
        }
      , [] () { std::cout << "00008 - fourth task: " << 42 << "\n"; return 42; }
    ).then(
        [] (const std::exception_ptr& e)
        {
          try { std::rethrow_exception(e); }
          catch (const std::exception& ee) { std::cout << "00008 - 5 caught exception: " << ee.what() << "\n"; }
        }
      , [n] (int a) -> int { std::cout << "00008 - fifth task: " << n << "\n"; throw cool::exception::operation_failed("hello world"); return n; }
    ).then(
        [] (const std::exception_ptr& e)
        {
          try { std::rethrow_exception(e); }
          catch (const std::exception& ee) { std::cout << "00008 - 6 caught exception: " << ee.what() << "\n"; }
        }
      , [n] (int res) { std::cout << "00008 - sixth task: " << n << "\n"; return n; }
    ).then(
        [] (const std::exception_ptr& e)
        {
          try { std::rethrow_exception(e); }
          catch (const std::exception& ee) { std::cout << "00008 - 7 caught exception: " << ee.what() << "\n"; }
        }
      , [n] (int a) { std::cout << "00008 - seventh task: " << n << "\n";}
    ).then(
        [] (const std::exception_ptr& e)
        {
          try { std::rethrow_exception(e); }
          catch (const std::exception& ee) { std::cout << "00008 - 8 caught exception: " << ee.what() << "\n"; }
        }
      , [] () { std::cout << "00008 - eighth task: " << 42 << "\n"; return 42; }
    ).then(
        [] (const std::exception_ptr& e)
        {
          try { std::rethrow_exception(e); }
          catch (const std::exception& ee) { std::cout << "00008 - 9 caught exception: " << ee.what() << "\n"; }
        }
      , [n] (int a) -> int { std::cout << "00008 - nineth task: " << n << "\n"; return n; }
    ).run();
}
#endif


TEST(runner, task)
{
  t00001();
  t00001a();
  t00002();
  t00002a();
  t00002b();
  t00002c();
  t00002d();
  t00002e();
  t00002f();
#if !defined(INCORRECT_VARIADIC)
  t00003();
  t00004();
  t00005();
  t00006();
  t00007();
  t00007a();
  t00008();
#endif

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
}





