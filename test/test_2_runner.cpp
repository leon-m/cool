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
  using result_type = T;
};
template <typename T, typename Y>
struct C
{
  using result_type = T;
  using input_type = Y;
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
{
  const int m_id = 1;
  using ptr = std::shared_ptr<my_runner>;
};

struct my_other_runner : public runner
{
  const int m_id = 100000;
  using ptr = std::shared_ptr<my_other_runner>;
};

TEST(runner, basic)
{
  const std::string base = "si.digiverse.cool2.runner";
  my_runner r;
  EXPECT_EQ(base, r.name().substr(0,base.length()));
}


#if 0

TEST(runner, sequence_of_tasks)
{
  {
    auto r = std::make_shared<runner>(RunPolicy::SEQUENTIAL);
    cool::basis::vow<void> v_;
    auto a_ = v_.get_aim();
    int step = 0;
    factory::create(r,
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
    factory::create(r,
      [&v_, &step] (const runner::ptr& r_)
      {
        if (step == 2)
          step = 3;
      }
    ).run();
    factory::create(r,
      [&v_, &step] (const runner::ptr& r_)
      {
        if (step == 3)
          step = 4;
      }
    ).run();
    factory::create(r,
      [&v_, &step] (const runner::ptr& r_)
      {
        if (step == 4)
          step = 5;
      }
    ).run();
    factory::create(r,
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

#if 0
TEST(runner, sequential_tasks)
{
  auto r = std::make_shared<runner>(RunPolicy::SEQUENTIAL);

  {
    cool::basis::vow<std::string> v_;
    auto a_ = v_.get_aim();

    auto t1 = factory::create(
        r
      , [] (const runner::ptr&, int n)
        {
          return std::to_string(n);
        }
    );
    auto t2 = factory::create(
        r
      , [&v_](const runner::ptr&, const std::string arg)
        {
          v_.set(arg);
        }
    );

    auto t = factory::sequential(r, t1, t2);
//    t.run(42);

    std::string res;
    EXPECT_NO_THROW(res = a_.get(ms(100)));
    EXPECT_EQ("84", res);
  }

}
#endif
#endif
// --------------------------------------------------------------------------
//
//
// Task creation and composition tests
//
//
//

double d_f_d(const runner::ptr& r, double d) { return d; }
void v_f_d(const runner::ptr& r, double d) {  }
double d_f(const runner::ptr&) { return 3; }
void v_f(const runner::ptr&) {  }
double d_f_d_i(const runner::ptr& r, double d, int n) { return d; }
void v_f_d_i(const runner::ptr& r, double d, int n) {  }
double d_f_i(const runner::ptr& r, int n) { return 3; }
void v_f_i(const runner::ptr& r, int n) {  }

struct c_int_int  { int  operator ()(const runner::ptr& r, int n) { return 3; } };
struct c_void_int { void operator ()(const runner::ptr& r, int n) { } };
struct c_int      { int  operator ()(const runner::ptr& r) { return 3; } };
struct c_void     { void operator ()(const runner::ptr& r) { } };

#define IS_RET_TYPE(type_, task_) \
   EXPECT_EQ(std::string(typeid(type_).name()), \
             std::string(typeid(decltype(task_)::result_type).name()))
#define IS_PAR_TYPE(type_, task_) \
   EXPECT_EQ(std::string(typeid(type_).name()), \
             std::string(typeid(decltype(task_)::input_type).name()))

// This tests si basically compilation test to see whether the stuff compiles
// with different Callable types
TEST(compile, task_simple)
{
  auto r = std::make_shared<my_runner>();

#if 1
  // lambdas
  {
    auto t = factory::create(r, [] (const runner::ptr& r, int n) { return 3; });
    auto y = factory::create(r, [] (const runner::ptr& r, int n) {  });
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

    auto t = factory::create(r, f1);
    auto y = factory::create(r, f2);
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
    auto t = factory::create(r, c_int_int());
    auto y = factory::create(r, c_void_int());
    IS_RET_TYPE(int, t);
    IS_RET_TYPE(void, y);
    IS_PAR_TYPE(int, t);
    IS_PAR_TYPE(int, y);
  }
#endif
#if 1
  // function pointers
  {
    auto t = factory::create(r, &d_f_d);
    auto y = factory::create(r, &v_f_d);
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
    auto t = factory::create(r, [] (const runner::ptr& r) { return 3; });
    auto y = factory::create(r, [] (const runner::ptr& r) {  });
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

    auto t = factory::create(r, f1);
    auto y = factory::create(r, f2);
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
    auto t = factory::create(r, c_int());
    auto y = factory::create(r, c_void());
    IS_RET_TYPE(int, t);
    IS_RET_TYPE(void, y);
    IS_PAR_TYPE(void, t);
    IS_PAR_TYPE(void, y);
  }
#endif
#if 1
  // function pointers
  {
    auto t = factory::create(r, &d_f);
    auto y = factory::create(r, &v_f);
    IS_RET_TYPE(double, t);
    IS_RET_TYPE(void, y);
    IS_PAR_TYPE(void, t);
    IS_PAR_TYPE(void, y);
  }
#endif
}

TEST(compile, task_parallel)
{

  auto r = std::make_shared<my_runner>();

  {
    auto t = factory::create(r, &d_f);
    auto y = factory::create(r, &d_f);

    auto c = factory::parallel(t, y);

    IS_PAR_TYPE(void, c);
    EXPECT_EQ(typeid(std::tuple<double, double>), typeid(decltype(c)::result_type));
  }
  {
    auto t = factory::create(r, &d_f);
    auto y = factory::create(r, &d_f);

    auto c = factory::parallel(t, y);

    IS_PAR_TYPE(void, c);
    EXPECT_EQ(typeid(std::tuple<double, double>), typeid(decltype(c)::result_type));
  }
  {
    auto t = factory::create(r, &d_f_d);
    auto y = factory::create(r, &v_f_d);

    auto c = factory::parallel(t, y);

    IS_PAR_TYPE(double, c);
    EXPECT_EQ(typeid(std::tuple<double, void*>), typeid(decltype(c)::result_type));
  }
  {
    auto t = factory::create(r, &d_f_d);
    auto y = factory::create(r, &v_f_d);

    auto c = factory::parallel(t, y);

    IS_PAR_TYPE(double, c);
    EXPECT_EQ(typeid(std::tuple<double, void*>), typeid(decltype(c)::result_type));
  }
}

TEST(compile, task_sequential)
{
  auto r = std::make_shared<my_runner>();
  {
    auto t = factory::create(r, &v_f);
    auto y = factory::create(r, &d_f);

    auto c = factory::sequential(t, y);
    IS_PAR_TYPE(void, c);
    IS_RET_TYPE(double, c);
  }
  {
    auto t = factory::create(r, &v_f);
    auto y = factory::create(r, &d_f);

    auto c = factory::sequential(t, y);
    IS_PAR_TYPE(void, c);
    IS_RET_TYPE(double, c);
  }
  {
    auto t = factory::create(r, &d_f_d);
    auto y = factory::create(r, &v_f_d);

    auto c = factory::sequential(t, y);
    IS_PAR_TYPE(double, c);
    IS_RET_TYPE(void, c);
  }
  {
    auto t = factory::create(r, &d_f_d);
    auto y = factory::create(r, &v_f_d);

    auto c = factory::sequential(t, y);
    IS_PAR_TYPE(double, c);
    IS_RET_TYPE(void, c);
  }
}

TEST(compile, task_conditional)
{
  auto r = std::make_shared<my_runner>();
  {
    auto t = factory::create(r, &d_f_d);
    auto y = factory::create(r, &d_f_d);

    auto c = factory::conditional([](double x) { return false; }, t, y);
    IS_PAR_TYPE(double, c);
    IS_RET_TYPE(double, c);
  }
}

TEST(compile, task_oneof)
{
  auto r = std::make_shared<my_runner>();
  {
    auto t = factory::create(r, &d_f_d);
    auto y = factory::create(r, &d_f_d);
    auto z = factory::create(r, &d_f_d);

    auto c = factory::oneof(
        z
      , std::make_tuple([](double) -> bool { return false; }, t)
      , std::make_tuple([](double) -> bool { return false; }, y)
    );
    IS_PAR_TYPE(double, c);
    IS_RET_TYPE(double, c);
  }
}

TEST(compile, task_loop)
{
  auto r = std::make_shared<my_runner>();
  {
    auto t = factory::create(r, &d_f_d);

    auto c = factory::loop(
        [](double x) { return x == 0; }
      , t
    );
    IS_PAR_TYPE(double, c);
    IS_RET_TYPE(double, c);
  }
}

TEST(compile, task_repeat)
{
  auto r = std::make_shared<my_runner>();
  {
    auto t = factory::create(r, &v_f_i);

    auto c = factory::repeat(t);
    IS_PAR_TYPE(int, c);
    IS_RET_TYPE(void, c);
  }
}


TEST(run, task_simple_task)
{
	auto r = std::make_shared<my_runner>();

	// std::string ret, int parameter
	{
		cool::basis::vow<void> v_;
		auto a_ = v_.get_aim();

		auto t = factory::create(
			r
			, [&v_](const my_runner::ptr& r_, int n_)
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

		auto t = factory::create(r, [&v_](const my_runner::ptr& r_) -> int { v_.set(); return 5; });
		t.run();

		EXPECT_NO_THROW(a_.get(ms(100)));
	}

	// void ret int parameter
	{
		cool::basis::vow<void> v_;
		auto a_ = v_.get_aim();

		auto t = factory::create(r, [&v_](const my_runner::ptr& r_, int n_) -> void { v_.set(); });
		t.run(42);

		EXPECT_NO_THROW(a_.get(ms(100)));
	}
	// void ret void paramter
	{
		cool::basis::vow<void> v_;
		auto a_ = v_.get_aim();

		auto t = factory::create(
			r
			, [&v_](const my_runner::ptr& r_) -> void
		{
			v_.set();
		}
		);
		t.run();
		EXPECT_NO_THROW(a_.get(ms(100)));
	}
}

TEST(run, task_conditional)
{
  auto r1 = std::make_shared<my_runner>();
  auto r2 = std::make_shared<my_other_runner>();
  {
    int runner_id = 0;
    int parameter = 0;
    std::unique_ptr<cool::basis::vow<int>> v_(new cool::basis::vow<int>);

    auto t1 = factory::create(
        r1
      , [&runner_id, &v_, &parameter](const my_runner::ptr& r_, int arg_)
        {
          runner_id = r_->m_id;
          parameter = arg_;
          v_->set(1);
        }
    );
    auto t2 = factory::create(
        r2
      , [&runner_id, &v_, &parameter](const my_other_runner::ptr& r_, int arg_)
        {
          runner_id = r_->m_id;
          parameter = arg_;
          v_->set(2);
        }
    );
    auto c = factory::conditional([](int arg_) { return arg_ % 2 == 0; }, t1, t2);

    {
      auto a_ = v_->get_aim();
      int res = 0;

      c.run(12);
      EXPECT_NO_THROW(res = a_.get(ms(100)));
      EXPECT_EQ(1, res);
      EXPECT_EQ(1, runner_id);
      EXPECT_EQ(12, parameter);
    }

    v_.reset((new cool::basis::vow<int>));
    {
      auto a_ = v_->get_aim();
      int res = 0;

      c.run(13);
      EXPECT_NO_THROW(res = a_.get(ms(100)));
      EXPECT_EQ(2, res);
      EXPECT_EQ(100000, runner_id);
      EXPECT_EQ(13, parameter);
    }
  }
}

TEST(run, task_loop)
{
  auto r = std::make_shared<my_runner>();
  {
    cool::basis::vow<void> v_;
    auto a_ = v_.get_aim();

    const int iterations = 1000;
    int count = 0;

    factory::loop(
        [&iterations](int step) { return step < iterations; }
      , factory::create(
            r
          , [&count, &v_, &iterations] (const std::shared_ptr<my_runner>& r_, int step_)
            {
              ++count;
              if (count == iterations)
                v_.set();
              return step_ + 1;
            }
        )
    ).run(0);

    EXPECT_NO_THROW(a_.get(ms(1000)));
    EXPECT_EQ(iterations, count);
  }
}

TEST(run, task_repeat)
{
  auto r = std::make_shared<my_runner>();
  {
    cool::basis::vow<void> v_;
    auto a_ = v_.get_aim();

    const std::size_t iterations = 1000;
    std::size_t count = 0;
    factory::repeat(
      factory::create(
          r
        , [&count, &v_, &iterations](const std::shared_ptr<my_runner>& r_, const unsigned int arg_)
          {
            ++count;
            if (arg_ == iterations)
              v_.set();
          }
      )
    ).run(iterations);

    EXPECT_NO_THROW(a_.get(ms(1000)));
    EXPECT_EQ(iterations, count);
  }
  {
    // throw exception after 500-th iteration
    std::size_t count = 0;
    auto t = factory::create(r, [&count](const std::shared_ptr<my_runner>& r_, const std::size_t arg_) { ++count; if (count == 500) throw std::runtime_error(""); });
    auto c = factory::repeat(t);
    c.run(1000);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(500, count);
  }
  {
    cool::basis::vow<void> v_;
    auto a_ = v_.get_aim();
    const std::size_t iterations = 1000;
    std::size_t count = 0;
    factory::repeat(
      factory::repeat(
        factory::create(
            r
            , [&count, &v_, &iterations](const std::shared_ptr<my_runner>& r_, const std::size_t arg_)
              {
                ++count;
                if (count == 500500)
                  v_.set();
              }
        )
      )
    ).run(iterations);

    EXPECT_NO_THROW(a_.get(ms(4000)));
    EXPECT_EQ(500500, count);
  }
}

TEST(run, task_repeat_conditional)
{
  auto r1 = std::make_shared<my_runner>();
  auto r2 = std::make_shared<my_other_runner>();

  {
    int low = 0, high = 0;
    cool::basis::vow<void> v_;
    auto a_ = v_.get_aim();
    factory::repeat(
      factory::conditional(
          [](int n) { return n <= 500; }
        , factory::create(
            r1
          , [&low] (const std::shared_ptr<runner>&, int n) { ++low; })
        , factory::create(
            r1
          , [&high, &v_] (const std::shared_ptr<runner>&, int n) { ++high; if (n == 1000) v_.set(); })
      )
    ).run(1000);
    EXPECT_NO_THROW(a_.get(ms(100)));
    EXPECT_EQ(500, low);
    EXPECT_EQ(500, high);
  }
}

TEST(run, task_loop_conditional)
{
  auto r1 = std::make_shared<my_runner>();
  auto r2 = std::make_shared<my_other_runner>();
  {
    const int max_repeat = 1000;
    const int lim_true   = 900;

    int count = 0;
    int id_sum = 0;
    cool::basis::vow<void> v_;
    auto a_ = v_.get_aim();
    auto t = factory::loop(
        [&max_repeat](int n_)
        {
          return n_ < max_repeat;
        }
      , factory::conditional(
            [&lim_true] (int n_)
            {
              return n_ < lim_true;
            }
          , factory::create(
                r1
              , [&count, &id_sum] (const std::shared_ptr<my_runner>& r_, int n_)
                {
                  ++count;
                  id_sum += r_->m_id;
                  return n_ + 1;
                }
            )
          , factory::create(
                r2
              , [&count, &v_, &id_sum, &max_repeat] (const std::shared_ptr<my_other_runner>& r_, int n_)
                {
                  id_sum += r_->m_id;
                  v_.set();
                  return max_repeat;
                }
            )
        )
    );
    t.run(0);
    EXPECT_NO_THROW(a_.get(ms(1000)));
    EXPECT_EQ(lim_true, count);
    EXPECT_EQ(100900, id_sum);
  }
}


