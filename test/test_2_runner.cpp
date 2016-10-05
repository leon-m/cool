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
#include <atomic>
#include <typeinfo>
#include <gtest/gtest.h>
#include <cool/vow.h>
#include "cool2/async.h"

using namespace cool::async;
using ms = std::chrono::milliseconds;

TEST(runner, basic)
{
  const std::string base = "si.digiverse.cool2.runner";
  runner r;
  EXPECT_EQ(base, r.name().substr(0,base.length()));

}

// ------------- compilation test ---------
//
// Create tasks witj all possible kinds of callables

double d_f_d(runner::weak_ptr_t r, double d) { return d; }
void v_f_d(runner::weak_ptr_t r, double d) {  }
double d_f(runner::weak_ptr_t r) { return 3; }
void v_f(runner::weak_ptr_t r) {  }
double d_f_d_i(runner::weak_ptr_t r, double d, int n) { return d; }
void v_f_d_i(runner::weak_ptr_t r, double d, int n) {  }
double d_f_i(runner::weak_ptr_t r, int n) { return 3; }
void v_f_i(runner::weak_ptr_t r, int n) {  }

struct c_int_int  { int  operator ()(const runner::weak_ptr_t& r, int n) { return 3; } };
struct c_void_int { void operator ()(const runner::weak_ptr_t& r, int n) { } };
struct c_int      { int  operator ()(const runner::weak_ptr_t& r) { return 3; } };
struct c_void     { void operator ()(const runner::weak_ptr_t& r) { } };

struct my_runner : public runner
{
};

#define IS_RET_TYPE(type_, task_) \
   EXPECT_EQ(std::string(typeid(type_).name()), \
             std::string(typeid(decltype(task_)::result_t).name()))
#define IS_PAR_TYPE(type_, task_) \
   EXPECT_EQ(std::string(typeid(type_).name()), \
             std::string(typeid(decltype(task_)::parameter_t).name()))
#if 0
// This tests si basically compilation test to see whether the stuff compiles
// with different Callable types
TEST(runner, basic_compile_task)
{
  auto r = std::make_shared<my_runner>();

#if 1
  // lambdas
  {
    auto t = taskop::create(r, [] (const runner::weak_ptr_t& r, int n) { return 3; });
    auto y = taskop::create(r, [] (const runner::weak_ptr_t& r, int n) {  });
    IS_RET_TYPE(int, t);
    IS_RET_TYPE(void, y);
    IS_PAR_TYPE(int, t);
    IS_PAR_TYPE(int, y);
  }
#endif
  // std::function
#if 1
  {
    std::function<double(const runner::weak_ptr_t&, double)> f1 = [] (const runner::weak_ptr_t& r, double n) { return 3; };
    std::function<void(const runner::weak_ptr_t&, double)> f2 = [] (const runner::weak_ptr_t& r, double n) { };

    auto t = taskop::create(r, f1);
    auto y = taskop::create(r, f2);
    IS_RET_TYPE(double, t);
    IS_RET_TYPE(void, y);
    IS_PAR_TYPE(double, t);
    IS_PAR_TYPE(double, y);
  }
#endif
#if 0
  // std::bind
  {
    auto t = taskop::create(r, std::bind(d_f_d_i, std::placeholders::_1, std::placeholders::_2, 3));
    auto y = taskop::create(r, std::bind(v_f_d_i, std::placeholders::_1, std::placeholders::_2, 3));
  }
#endif
#if 1
  // functor
  {
    auto t = taskop::create(r, c_int_int());
    auto y = taskop::create(r, c_void_int());
    IS_RET_TYPE(int, t);
    IS_RET_TYPE(void, y);
    IS_PAR_TYPE(int, t);
    IS_PAR_TYPE(int, y);
  }
#endif
#if 1
  // function pointers
  {
    auto t = taskop::create(r, &d_f_d);
    auto y = taskop::create(r, &v_f_d);
    IS_RET_TYPE(double, t);
    IS_RET_TYPE(void, y);
    IS_PAR_TYPE(double, t);
    IS_PAR_TYPE(double, y);
  }
#endif
  // ------------------- Tasks without input parameter
  // lambdas
#if 1
  {
    auto t = taskop::create(r, [] (const runner::weak_ptr_t& r) { return 3; });
    auto y = taskop::create(r, [] (const runner::weak_ptr_t& r) {  });
    IS_RET_TYPE(int, t);
    IS_RET_TYPE(void, y);
    IS_PAR_TYPE(void, t);
    IS_PAR_TYPE(void, y);
  }
#endif
  // std::function
#if 1
  {
    std::function<double(const runner::weak_ptr_t&)> f1 = [] (const runner::weak_ptr_t& r) { return 3; };
    std::function<void(const runner::weak_ptr_t&)> f2 = [] (const runner::weak_ptr_t& r) { };

    auto t = taskop::create(r, f1);
    auto y = taskop::create(r, f2);
    IS_RET_TYPE(double, t);
    IS_RET_TYPE(void, y);
    IS_PAR_TYPE(void, t);
    IS_PAR_TYPE(void, y);
  }
#endif
#if 0
  // std::bind
  {
    auto t = taskop::create(r, std::bind(d_f_i, std::placeholders::_1, 3));
    auto y = taskop::create(r, std::bind(v_f_i, std::placeholders::_1, 3));
  }
#endif
  // functor
#if 1
  {
    auto t = taskop::create(r, c_int());
    auto y = taskop::create(r, c_void());
    IS_RET_TYPE(int, t);
    IS_RET_TYPE(void, y);
    IS_PAR_TYPE(void, t);
    IS_PAR_TYPE(void, y);
  }
#endif
#if 1
  // function pointers
  {
    auto t = taskop::create(r, &d_f);
    auto y = taskop::create(r, &v_f);
    IS_RET_TYPE(double, t);
    IS_RET_TYPE(void, y);
    IS_PAR_TYPE(void, t);
    IS_PAR_TYPE(void, y);
  }
#endif
}

TEST(runner, basic_compile_parallel)
{
  auto r = std::make_shared<my_runner>();

  {
    auto t = taskop::create(r, &d_f);
    auto y = taskop::create(r, &d_f);

    auto c = taskop::parallel(t, y);

    IS_PAR_TYPE(void, c);
    EXPECT_EQ(typeid(std::tuple<double, double>), typeid(decltype(c)::result_t));
  }
  {
    auto t = taskop::create(r, &d_f);
    auto y = taskop::create(r, &d_f);

    auto c = t.parallel(y);

    IS_PAR_TYPE(void, c);
    EXPECT_EQ(typeid(std::tuple<double, double>), typeid(decltype(c)::result_t));
  }
  {
    auto t = taskop::create(r, &d_f_d);
    auto y = taskop::create(r, &v_f_d);

    auto c = taskop::parallel(t, y);

    IS_PAR_TYPE(double, c);
    EXPECT_EQ(typeid(std::tuple<double, void*>), typeid(decltype(c)::result_t));
  }
  {
    auto t = taskop::create(r, &d_f_d);
    auto y = taskop::create(r, &v_f_d);

    auto c = t.parallel(y);

    IS_PAR_TYPE(double, c);
    EXPECT_EQ(typeid(std::tuple<double, void*>), typeid(decltype(c)::result_t));
  }
}

TEST(runner, basic_compile_sequential)
{
  auto r = std::make_shared<my_runner>();

  {
    auto t = taskop::create(r, &v_f);
    auto y = taskop::create(r, &d_f);

    auto c = taskop::sequential(t, y);
    IS_PAR_TYPE(void, c);
    IS_RET_TYPE(double, c);
  }
  {
    auto t = taskop::create(r, &v_f);
    auto y = taskop::create(r, &d_f);

    auto c = t.sequential(y);
    IS_PAR_TYPE(void, c);
    IS_RET_TYPE(double, c);
  }
  {
    auto t = taskop::create(r, &d_f_d);
    auto y = taskop::create(r, &v_f_d);

    auto c = taskop::sequential(t, y);
    IS_PAR_TYPE(double, c);
    IS_RET_TYPE(void, c);
  }
  {
    auto t = taskop::create(r, &d_f_d);
    auto y = taskop::create(r, &v_f_d);

    auto c = t.sequential(y);
    IS_PAR_TYPE(double, c);
    IS_RET_TYPE(void, c);
  }
}

TEST(runner, parallel_return_value)
{
  auto r = std::make_shared<my_runner>();

  try
  {
    cool::basis::vow<void> v_;
    auto a_ = v_.get_aim();

    auto a = taskop::create(r , [](const runner::weak_ptr_t& r) -> double { return 42.0; });
    auto b = taskop::create(r , [](const runner::weak_ptr_t& r) -> int { return 21; });
    auto c = a.parallel(b);
    auto d = taskop::create(
        r
      , [&v_] (const runner::weak_ptr_t& r, const decltype(c)::result_t& res)
        {
          EXPECT_EQ(42.0, std::get<0>(res));
          EXPECT_EQ(21, std::get<1>(res));
          v_.set();
        }
    );
    c.sequential(d).run();
    a_.get(ms(100));
  }
  catch (...)
  {
    EXPECT_TRUE(false);
  }
}
#endif

TEST(runner, basic_task_non_void_non_void)
{
  auto r = std::make_shared<my_runner>();

  // std::string ret, int parameter
  {
    cool::basis::vow<void> v_;
    auto a_ = v_.get_aim();

    auto t = taskop::create(r, [&v_] (const runner::ptr_t& r_, int n_) { v_.set(); return std::to_string(n_); });
    t.run();

    EXPECT_NO_THROW(a_.get(ms(100)));
  }
  
  // int ret void parameter
  {
    cool::basis::vow<void> v_;
    auto a_ = v_.get_aim();

    auto t = taskop::create(r, [&v_] (const runner::ptr_t& r_) -> int { v_.set(); return 5; });
    t.run();

    EXPECT_NO_THROW(a_.get(ms(100)));
  }

  // void ret int parameter
  {
    cool::basis::vow<void> v_;
    auto a_ = v_.get_aim();

    auto t = taskop::create(r, [&v_] (const runner::ptr_t& r_, int n_) -> void { v_.set(); });
    t.run();

    EXPECT_NO_THROW(a_.get(ms(100)));
  }
  // void ret void paramter
  {
    cool::basis::vow<void> v_;
    auto a_ = v_.get_aim();

    auto t = taskop::create(r, [&v_] (const runner::ptr_t& r_) -> void { v_.set(); });
    t.run();

    EXPECT_NO_THROW(a_.get(ms(100)));
  }
}

