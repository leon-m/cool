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

#include <string>
#include <tuple>
#include <type_traits>
#include <memory>
#include <functional>
#include <iostream>

#include "cool2/impl/traits.h"

#define RESULT(a, b) (a == b ? "OK\n" : "Failed\n")


using namespace cool::async::impl;
using namespace std::placeholders;

int f1(const std::weak_ptr<char>&, double)
{ return 3; }
void f2(const std::weak_ptr<char>&)
{  }
int f3(const std::weak_ptr<char>&, double, int n)
{ return 3; }
void f4(const std::weak_ptr<char>&, int n)
{  }

template <typename T>
struct A
{
  using result_t = T;
};

template <typename... Args>
struct B
{
  typename traits::parallel_result<Args...>::type m_a;
};

template <typename T, typename Y>
struct C
{
  using result_t = T;
  using parameter_t = Y;
};

int main(int argc, char* argv[])
{
  B<A<int>, A<char>, A<float>> a;
  B<A<int>, A<const void>, A<char>> b;

  std::get<0>(a.m_a) = 3;
  std::get<1>(b.m_a) = nullptr;
  using tricky_t = void;
  B<A<int>, A<tricky_t>, A<std::string>> c;
  std::get<1>(c.m_a) = nullptr;
  using another_tricky_t = const void;
  B<A<int>, A<another_tricky_t>, A<std::string>> d;
  std::get<1>(c.m_a) = nullptr;

  std::cout << "Tupple type replacement\n"
               "=======================\n\n"
               "  A<int, const void, char>:\n"
               "    get<0> : " << typeid(std::get<0>(b.m_a)).name() << "\n"
               "    get<1> : " << typeid(std::get<1>(b.m_a)).name() << "\n"
               "    get<2> : " << typeid(std::get<2>(b.m_a)).name() << "\n\n"
               "  A<int, another_tricky_t, std::string>:\n"
               "    get<0> : " << typeid(std::get<0>(d.m_a)).name() << "\n"
               "    get<1> : " << typeid(std::get<1>(d.m_a)).name() << "\n"
               "    get<2> : " << typeid(std::get<2>(d.m_a)).name() << "\n\n";

  std::cout << "function_traits  types\n"
               "======================\n\n";
  {
    std::cout << "  int f1(const std::weak_ptr<char>&, double)\n"
                 "    return type: " << typeid(traits::function_traits<decltype(&f1)>::result_type).name() << "\n"
                 "    num params : " << traits::function_traits<decltype(&f1)>::arity::value << "\n"
                 "    param 1    : " << typeid(traits::function_traits<decltype(&f1)>:: template arg<0>::type).name() << "\n"
                 "    param 2    : " << typeid(traits::function_traits<decltype(&f1)>:: template arg<1>::type).name() << "\n\n"
                 "  void f2(const std::weak_ptr<char>&)\n"
                 "    return type: " << typeid(traits::function_traits<decltype(&f2)>::result_type).name() << "\n"
                 "    num params : " << traits::function_traits<decltype(&f2)>::arity::value << "\n"
                 "    param 1    : " << typeid(traits::function_traits<decltype(&f2)>:: template arg<0>::type).name() << "\n\n";
  }

  {
    auto a = [] (const std::weak_ptr<char>&, double) { return 3; };
    auto b = [] (const std::weak_ptr<char>&) {  };

    std::cout <<
      "  lambda [] (const std::weak_ptr<char>&, double) { return 3; }\n"
      "    return type: " << typeid(traits::function_traits<decltype(a)>::result_type).name() << "\n"
      "    num params : " << traits::function_traits<decltype(a)>::arity::value << "\n"
      "    param 1    : " << typeid(traits::function_traits<decltype(a)>:: template arg<0>::type).name() << "\n"
      "    param 2    : " << typeid(traits::function_traits<decltype(a)>:: template arg<1>::type).name() << "\n\n"
      "  lambda [] (const std::weak_ptr<char>&) {  }\n"
      "    return type: " << typeid(traits::function_traits<decltype(b)>::result_type).name() << "\n"
      "    num params : " << traits::function_traits<decltype(b)>::arity::value << "\n"
      "    param 1    : " << typeid(traits::function_traits<decltype(b)>:: template arg<0>::type).name() << "\n\n";
  }

  {
    std::function<int(const std::weak_ptr<char>&, double)> a; // = [] (const std::weak_ptr<char>&, double) { return 3; };
    std::function<void(const std::weak_ptr<char>&)> b = [] (const std::weak_ptr<char>&) {  };

    std::cout <<
        "  std::function<int(const std::weak_ptr<char>&, double)>\n"
        "    return type: " << typeid(traits::function_traits<decltype(a)>::result_type).name() << "\n"
        "    num params : " << traits::function_traits<decltype(a)>::arity::value << "\n"
        "    param 1    : " << typeid(traits::function_traits<decltype(a)>:: template arg<0>::type).name() << "\n"
        "    param 2    : " << typeid(traits::function_traits<decltype(a)>:: template arg<1>::type).name() << "\n\n"
        "  std::function<void(const std::weak_ptr<char>&)>\n"
        "    return type: " << typeid(traits::function_traits<decltype(b)>::result_type).name() << "\n"
        "    num params : " << traits::function_traits<decltype(b)>::arity::value << "\n"
        "    param 1    : " << typeid(traits::function_traits<decltype(b)>:: template arg<0>::type).name() << "\n\n";
  }

  std::cout << "Result type deduction\n"
               "=====================\n\n";
  {
    using D = traits::parallel_result<A<int>, A<void>, A<double>>::type;

    std::cout <<
        "  parallel_result<A<int>, A<void>, A<double>>::type\n"
        "    result type: " << typeid(D).name() << "\n\n";
  }
  {
    using D = traits::sequence_result<A<int>, A<void>, A<double>>::type;

    std::cout <<
        "  sequence_result<A<int>, A<void>, A<double>>::type\n"
        "    result type: " << typeid(D).name() << "\n\n";
  }
  {
    using D = traits::sequence_result<A<int>, A<void>, A<void>>::type;

    std::cout <<
        "  sequence_result<A<int>, A<void>, A<void>>::type\n"
        "    result type: " << typeid(D).name() << "\n\n";
  }
  {
    using D = traits::first_task<C<int, int>, C<void, void>, C<void, double>>::type;

    std::cout <<
        "  first_task<C<int, int>, C<int, void>, C<void, double>>::type\n"
        "    paramter type: " << typeid(D::parameter_t).name() << "\n\n";
  }

  std::cout << "Result of all_same\n"
               "=====================\n\n";
  {
    std::cout <<
        "  all_same<int, int, int>\n"
        "    result: " << (traits::all_same<int,int,int>::value ? "true" : "false" ) << "\n\n";
  }
  {
    std::cout <<
        "  all_same<int, void, int>\n"
        "    result: " << (traits::all_same<int,void,int>::value ? "true" : "false" ) << "\n\n";
  }
  {
    std::cout <<
        "  all_same<const int&, int, int>\n"
        "    result: " << (traits::all_same<std::decay<const int&>::type,int,int>::value ? "true" : "false" ) << "\n\n";
  }
  {
        std::cout <<
        "  all_same<int&, int>\n"
        "    result: " << (traits::all_same<std::decay<int&>::type,int>::value ? "true" : "false" ) << "\n\n";
  }
  {
    std::cout <<
    "  all_same<double, double, double, double, double, double, double>\n"
    "    result: " << (traits::all_same<double, double, double, double, double, double, double>::value ? "true" : "false" ) << "\n\n";
  }
  {
    std::cout <<
    "  all_same<double, double, double, double, double, double, int>\n"
    "    result: " << (traits::all_same<double, double, double, double, double, double, int>::value ? "true" : "false" ) << "\n\n";
  }
  {
    std::cout <<
    "  all_same<double, double, double, double, double, int, double>\n"
    "    result: " << (traits::all_same<double, double, double, double, double, int, double>::value ? "true" : "false" ) << "\n\n";
  }
  {
    std::cout <<
    "  all_same<double, double, double, double, int, double, double>\n"
    "    result: " << (traits::all_same<double, double, double, double, int, double, double>::value ? "true" : "false" ) << "\n\n";
  }
  {
    std::cout <<
    "  all_same<double, double, double, int, double, double, double>\n"
    "    result: " << (traits::all_same<double, double, double, int, double, double, double>::value ? "true" : "false" ) << "\n\n";
  }
  {
    std::cout <<
    "  all_same<double, double, int, double, double, double, double>\n"
    "    result: " << (traits::all_same<double, double, int, double, double, double, double>::value ? "true" : "false" ) << "\n\n";
  }
  {
    std::cout <<
    "  all_same<double, int, double, double, double, double, double>\n"
    "    result: " << (traits::all_same<double, int, double, double, double, double, double>::value ? "true" : "false" ) << "\n\n";
  }
  {
    std::cout <<
    "  all_same<int, double, double, double, double, double, double>\n"
    "    result: " << (traits::all_same<int, double, double, double, double, double, double>::value ? "true" : "false" ) << "\n\n";
  }
  std::cout << "Result of are_chained\n"
               "=====================\n\n";
  {
    std::cout <<
        "  are_chained<C<int, int>, C<int, void>>\n"
        "    result: " << (traits::are_chained<C<int,int>, C<void, int>>::value ? "true" : "false" ) << "\n\n";
  }
  {
    std::cout <<
        "  are_chained<C<int, void>, C<void, int>>\n"
        "    result: " << (traits::are_chained<C<void,int>, C<int,void>>::value ? "true" : "false" ) << "\n\n";
  }
  {
    std::cout <<
    "  are_chained<C<int,double>, C<void,int>, C<char, void>, C<bool, char>, C<double, bool>, C<void, double>>:\n"
    "    result: " << RESULT(true, (traits::are_chained<C<int,double>, C<void,int>, C<char, void>, C<bool, char>, C<double, bool>, C<void, double>>::value)) << "\n\n";
  }
  {
    std::cout <<
    "  are_chained<C<int,double>, C<void,int>, C<char, void>, C<bool, bool>, C<double, bool>, C<void, double>>:\n"
    "    result: " << RESULT(false, (traits::are_chained<C<int,double>, C<void,int>, C<char, void>, C<bool, bool>, C<double, bool>, C<void, double>>::value)) << "\n\n";
  }
  {
    std::cout <<
    "  are_chained<C<int, double>, C<void, int>, C<char, void> C<void, char>\n"
    "    result: " << (traits::are_chained<C<int,double>, C<void,int>, C<char, void>>::value ? "true" : "false" ) << "\n\n";
  }
  {
    std::cout <<
    "  are_chained<C<int, double>, C<int int>, C<char, void>>\n"
    "    result: " << (traits::are_chained<C<int,double>, C<int,int>, C<char, void>>::value ? "true" : "false" ) << "\n\n";
  }



}