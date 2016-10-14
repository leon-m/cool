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
#include <vector>
#include <typeinfo>
#include <tuple>
#include <gtest/gtest.h>
#include <cool/vow.h>
#include "cool2/async.h"
using namespace cool::async;
using namespace cool::async::impl;

using ms = std::chrono::milliseconds;



// --------------------------------------------------------------------------
//
//
// Traits tests
//
//
//

char* g_f(int arg_) { return nullptr; }

TEST(traits, function_traits)
{
  {
    auto f = [] (const std::weak_ptr<char>&, double) { return 3; };
    EXPECT_EQ(typeid(int), typeid(traits::function_traits<decltype(f)>::result_type));
    EXPECT_EQ(2, traits::function_traits<decltype(f)>::arity::value);
    EXPECT_EQ(typeid(std::weak_ptr<char>), typeid(traits::function_traits<decltype(f)>::arg<0>::type));
    EXPECT_EQ(typeid(double), typeid(traits::function_traits<decltype(f)>::arg<1>::type));
  }
  {
    std::function<void()> f;
    EXPECT_EQ(typeid(void), typeid(traits::function_traits<decltype(f)>::result_type));
    EXPECT_EQ(0, traits::function_traits<decltype(f)>::arity::value);
  }
  {
    EXPECT_EQ(typeid(char*), typeid(traits::function_traits<decltype(&g_f)>::result_type));
    EXPECT_EQ(1, traits::function_traits<decltype(&g_f)>::arity::value);
    EXPECT_EQ(typeid(int), typeid(traits::function_traits<decltype(&g_f)>::arg<0>::type));
  }
}

template <typename T>
struct A
{
  using result_t = T;
};
template <typename T, typename Y>
struct C
{
  using result_t = T;
  using parameter_t = Y;
};

TEST(traits, parallel_deduction)
{
  EXPECT_EQ(typeid(std::tuple<int, void*, double>), typeid(traits::parallel_result<A<int>, A<void>, A<double>>::type));
  EXPECT_EQ(typeid(std::tuple<int, void*, double>), typeid(traits::parallel_result<A<int>, A<void*>, A<double>>::type));
  EXPECT_EQ(typeid(std::tuple<int, void**, double>), typeid(traits::parallel_result<A<int>, A<void**>, A<double>>::type));

  using tricky_t = void;
  using const_tricky_t = const tricky_t;
  EXPECT_EQ(typeid(std::tuple<int, void*, double>), typeid(traits::parallel_result<A<int>, A<tricky_t>, A<double>>::type));
  EXPECT_EQ(typeid(std::tuple<int, void*, double>), typeid(traits::parallel_result<A<int>, A<const_tricky_t>, A<double>>::type));
}

TEST(traits, sequential_deduction)
{
  EXPECT_EQ(typeid(double), typeid(traits::sequence_result<A<int>, A<void>, A<double>>::type));
  EXPECT_EQ(typeid(void), typeid(traits::sequence_result<A<int>, A<void>, A<void>>::type));
  EXPECT_EQ(typeid(C<int, int>), typeid(traits::first_task<C<int, int>, C<void, void>, C<void, double>>::type));
}

TEST(traits, all_same)
{
  EXPECT_TRUE((traits::all_same<int,int,int>::value));
  EXPECT_FALSE((traits::all_same<int,void,int>::value));
  EXPECT_TRUE((traits::all_same<std::decay<const int&>::type,int,int>::value));
  EXPECT_TRUE((traits::all_same<std::decay<int&>::type,int>::value));
  EXPECT_TRUE((traits::all_same<double, double, double, double, double, double, double>::value));
  EXPECT_FALSE((traits::all_same<double, double, double, double, double, double, int>::value));
  EXPECT_FALSE((traits::all_same<double, double, double, double, double, int, double>::value));
  EXPECT_FALSE((traits::all_same<double, double, double, double, int, double, double>::value));
  EXPECT_FALSE((traits::all_same<double, double, double, int, double, double, double>::value));
  EXPECT_FALSE((traits::all_same<double, double, int, double, double, double, double>::value));
  EXPECT_FALSE((traits::all_same<double, int, double, double, double, double, double>::value));
  EXPECT_FALSE((traits::all_same<int, double, double, double, double, double, double>::value));
}

TEST(traits, chained)
{
  EXPECT_TRUE((traits::all_chained<C<int,int>, C<void, int>>::result::value));
  EXPECT_TRUE((traits::all_chained<C<void,int>, C<int,void>>::result::value));
  EXPECT_TRUE((traits::all_chained<C<int,double>, C<void,int>, C<char, void>, C<bool, char>, C<double, bool>, C<void, double>>::result::value));
  EXPECT_FALSE((traits::all_chained<C<int,double>, C<void,int>, C<char, void>, C<bool, bool>, C<double, bool>, C<void, double>>::result::value));
  EXPECT_TRUE((traits::all_chained<C<int,double>, C<void,int>, C<char, void>>::result::value));
  EXPECT_FALSE((traits::all_chained<C<int,double>, C<int,int>, C<char, void>>::result::value));
  EXPECT_TRUE((traits::all_chained<C<int,int>, C<int,int>, C<int, int>, C<int, int>, C<int, int>, C<int, int>>::result::value));
  EXPECT_FALSE((traits::all_chained<C<int,int>, C<int,int>, C<int, int>, C<int, int>, C<int, int>, C<int, void>>::result::value));
  EXPECT_FALSE((traits::all_chained<C<int,int>, C<int,int>, C<int, int>, C<int, int>, C<int, void>, C<int, int>>::result::value));
  EXPECT_FALSE((traits::all_chained<C<int,int>, C<int,int>, C<int, int>, C<int, void>, C<int, int>, C<int, int>>::result::value));
  EXPECT_FALSE((traits::all_chained<C<int,int>, C<int,int>, C<int, void>, C<int, int>, C<int, int>, C<int, int>>::result::value));
  EXPECT_FALSE((traits::all_chained<C<int,int>, C<int,void>, C<int, int>, C<int, int>, C<int, int>, C<int, int>>::result::value));

  EXPECT_FALSE((traits::all_chained<C<int,int>, C<int,int>, C<int, int>, C<int, int>, C<void, int>, C<int, int>>::result::value));
  EXPECT_FALSE((traits::all_chained<C<int,int>, C<int,int>, C<int, int>, C<void, int>, C<int, int>, C<int, int>>::result::value));
  EXPECT_FALSE((traits::all_chained<C<int,int>, C<int,int>, C<void, int>, C<int, int>, C<int, int>, C<int, int>>::result::value));
  EXPECT_FALSE((traits::all_chained<C<int,int>, C<void,int>, C<int, int>, C<int, int>, C<int, int>, C<int, int>>::result::value));
  EXPECT_FALSE((traits::all_chained<C<void,int>, C<int,int>, C<int, int>, C<int, int>, C<int, int>, C<int, int>>::result::value));

}

// --------------------------------------------------------------------------
//
//
// Runner tests
//
//
//
struct my_runner : public runner
{ };

TEST(runner, basic)
{
  const std::string base = "si.digiverse.cool2.runner";
  runner r;
  EXPECT_EQ(base, r.name().substr(0,base.length()));
}

TEST(runner, basic_task_non_void_non_void)
{
	auto r = std::make_shared<my_runner>();

	// std::string ret, int parameter
	{
		cool::basis::vow<void> v_;
		auto a_ = v_.get_aim();

		auto t = taskop::create(
			r
			, [&v_](const runner::ptr& r_, int n_)
		{
			v_.set();
			return std::to_string(n_);
		}
		);
		t.run(15);
		EXPECT_NO_THROW(a_.get(ms(100)));
	}

	// int ret void parameter
	{
		cool::basis::vow<void> v_;
		auto a_ = v_.get_aim();

		auto t = taskop::create(r, [&v_](const runner::ptr& r_) -> int { v_.set(); return 5; });
		t.run();

		EXPECT_NO_THROW(a_.get(ms(100)));
	}

	// void ret int parameter
	{
		cool::basis::vow<void> v_;
		auto a_ = v_.get_aim();

		auto t = taskop::create(r, [&v_](const runner::ptr& r_, int n_) -> void { v_.set(); });
		t.run(42);

		EXPECT_NO_THROW(a_.get(ms(100)));
	}
	// void ret void paramter
	{
		cool::basis::vow<void> v_;
		auto a_ = v_.get_aim();

		auto t = taskop::create(
			r
			, [&v_](const runner::ptr& r_) -> void
		{
			v_.set();
		}
		);
		t.run();
		EXPECT_NO_THROW(a_.get(ms(100)));
	}
}

TEST(runner, sequence_of_tasks)
{
  {
    auto r = std::make_shared<runner>(RunPolicy::SEQUENTIAL);
    cool::basis::vow<void> v_;
    auto a_ = v_.get_aim();
    int step = 0;
    taskop::create(r,
      [&step] (const runner::ptr& r_)
      {
        step = 1;
        std::this_thread::sleep_for(ms(200));
        step = 2;
      }
    ).run();

    // the second task will make sure that the first completed the 200ms
    // wait (step value 2) and move the step to 3 if it did .. .and repeat
    // this several times to add more tasks into the queue
    taskop::create(r,
      [&v_, &step] (const runner::ptr& r_)
      {
        if (step == 2)
          step = 3;
      }
    ).run();
    taskop::create(r,
      [&v_, &step] (const runner::ptr& r_)
      {
        if (step == 3)
          step = 4;
      }
    ).run();
    taskop::create(r,
      [&v_, &step] (const runner::ptr& r_)
      {
        if (step == 4)
          step = 5;
      }
    ).run();
    taskop::create(r,
      [&v_, &step] (const runner::ptr& r_)
      {
        if (step == 5)
          step = 6;
        v_.set();
      }
    ).run();
    EXPECT_NO_THROW(a_.get(ms(300)));
    EXPECT_EQ(6, step);
  }
}

TEST(runner, big_sequence_of_tasks)
{
  {
    auto r = std::make_shared<runner>(RunPolicy::SEQUENTIAL);
    cool::basis::vow<void> v_;
    auto a_ = v_.get_aim();
    int step = 0;

    std::vector <task<impl::tag::simple, void, void>> tasks;
    for (int i = 0; i < 1000000; ++i)
      tasks.push_back(taskop::create(
          r
        , [i, &step](const runner::ptr& r_)
          {
            if (i == step)
              ++step;
          }
      ));

    tasks.push_back(taskop::create(
      r
      , [&v_](const runner::ptr&)
        {
          v_.set();
        }
    ));

    auto t_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < tasks.size(); ++i)
    {
      tasks[i].run();
    }
    EXPECT_NO_THROW(a_.get(ms(30000)));
    auto t_stop = std::chrono::high_resolution_clock::now();
    std::cout << "Time units: " << std::chrono::duration_cast<std::chrono::milliseconds>(t_stop - t_start).count() << "\n";
    EXPECT_EQ(1000000, step);
  }
}
// --------------------------------------------------------------------------
//
//
// Task creation and composition tests
//
//
//

double d_f_d(runner::ptr r, double d) { return d; }
void v_f_d(runner::ptr r, double d) {  }
double d_f(runner::ptr r) { return 3; }
void v_f(runner::ptr r) {  }
double d_f_d_i(runner::ptr r, double d, int n) { return d; }
void v_f_d_i(runner::ptr r, double d, int n) {  }
double d_f_i(runner::ptr r, int n) { return 3; }
void v_f_i(runner::ptr r, int n) {  }

struct c_int_int  { int  operator ()(const runner::ptr& r, int n) { return 3; } };
struct c_void_int { void operator ()(const runner::ptr& r, int n) { } };
struct c_int      { int  operator ()(const runner::ptr& r) { return 3; } };
struct c_void     { void operator ()(const runner::ptr& r) { } };

#define IS_RET_TYPE(type_, task_) \
   EXPECT_EQ(std::string(typeid(type_).name()), \
             std::string(typeid(decltype(task_)::result_t).name()))
#define IS_PAR_TYPE(type_, task_) \
   EXPECT_EQ(std::string(typeid(type_).name()), \
             std::string(typeid(decltype(task_)::parameter_t).name()))

// This tests si basically compilation test to see whether the stuff compiles
// with different Callable types
TEST(task, basic_compile_task)
{
  auto r = std::make_shared<my_runner>();

#if 1
  // lambdas
  {
    auto t = taskop::create(r, [] (const runner::ptr& r, int n) { return 3; });
    auto y = taskop::create(r, [] (const runner::ptr& r, int n) {  });
    IS_RET_TYPE(int, t);
    IS_RET_TYPE(void, y);
    IS_PAR_TYPE(int, t);
    IS_PAR_TYPE(int, y);
  }
#endif
  // std::function
#if 1
  {
    std::function<double(const runner::ptr&, double)> f1 = [] (const runner::ptr& r, double n) { return 3; };
    std::function<void(const runner::ptr&, double)> f2 = [] (const runner::ptr& r, double n) { };

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
    auto t = taskop::create(r, [] (const runner::ptr& r) { return 3; });
    auto y = taskop::create(r, [] (const runner::ptr& r) {  });
    IS_RET_TYPE(int, t);
    IS_RET_TYPE(void, y);
    IS_PAR_TYPE(void, t);
    IS_PAR_TYPE(void, y);
  }
#endif
  // std::function
#if 1
  {
    std::function<double(const runner::ptr&)> f1 = [] (const runner::ptr& r) { return 3; };
    std::function<void(const runner::ptr&)> f2 = [] (const runner::ptr& r) { };

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

TEST(task, basic_compile_parallel)
{
#if 0
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
#endif
}

TEST(task, basic_compile_sequential)
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

TEST(task, parallel_return_value)
{
#if 0
  auto r = std::make_shared<my_runner>();

  cool::basis::vow<void> v_;
  auto a_ = v_.get_aim();

  auto a = taskop::create(r , [](const runner::ptr& r) -> double { return 42.0; });
  auto b = taskop::create(r , [](const runner::ptr& r) -> int { return 21; });
  auto c = a.parallel(b);
  auto d = taskop::create(
      r
    , [&v_] (const runner::ptr& r, const decltype(c)::result_t& res)
      {
        EXPECT_EQ(42.0, std::get<0>(res));
        EXPECT_EQ(21, std::get<1>(res));
        v_.set();
      }
  );
  c.sequential(d).run();
  EXPECT_NO_THROW(a_.get(ms(100)));
#endif
}


