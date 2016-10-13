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

#if !defined(cool_41152bf7_f2d7_4732_8200_be1475dc84b2)
#define cool_41152bf7_f2d7_4732_8200_be1475dc84b2

namespace cool { namespace async { namespace impl { namespace traits {


// -------
// function_traits<Callable> determines the following information about
// the Callable type:
//   - result_type the return type of the callable, including void
//   - aritiy::value number of parameters
//   - ::arg<n>::type type of n-th paramter; will static_assert if n >= arity::value
//
// Note: it works for function pointers, functors, member function pointers,
//       member pointers. lambdas and std::function objects. It does not work
//       for std::bind<> produced types as they have multiple operator()
//       overloads. If you need this anyway, assign it to std::function.

template<typename F> struct function_traits;

// function pointer
template<typename R, typename... Args>
struct function_traits<R(*)(Args...)> : public function_traits<R(Args...)>
{};

template<class R, class... Args>
struct function_traits<R(Args...)>
{
  using result_type = R;

  using arity = std::integral_constant<std::size_t, sizeof...(Args) >;
  using arguments = std::tuple<Args...>;

  template <std::size_t N>
  struct arg
  {
    static_assert(N < arity::value, "error: invalid parameter index.");
    using type = typename std::tuple_element<N, arguments>::type;
  };
};

// member function pointer
template<typename C, typename R, typename... Args>
struct function_traits<R(C::*)(Args...)> : public function_traits<R(C&,Args...)>
{};

// const member function pointer
template<typename C, typename R, typename... Args>
struct function_traits<R(C::*)(Args...) const> : public function_traits<R(C&,Args...)>
{};

// member object pointer
template<typename C, typename R>
struct function_traits<R(C::*)> : public function_traits<R(C&)>
{};


// functor
template<typename F>
struct function_traits
{
private:
  using call_type = function_traits<decltype(&F::operator())>;
public:
  using result_type = typename call_type::result_type;

  using arity = std::integral_constant<std::size_t, call_type::arity::value - 1>;

  template <std::size_t N>
  struct arg
  {
    static_assert(N < arity::value, "error: invalid parameter index.");
    using type = typename call_type::template arg<N+1>::type;
  };
};

template<typename F>
struct function_traits<F&> : public function_traits<F>
{};

template<typename F>
struct function_traits<F&&> : public function_traits<F>
{};


// -------
// arg_type<n, CallableT>::type
//     returns type of the n-th parameter if exisst, void otherwise

template <bool is_valid, std::size_t arg_num, typename CallableT>
struct arg_type_extractor
{
  using arg_type = typename function_traits<CallableT>::template arg<arg_num>::type;
};

template <std::size_t arg_num, typename CallableT>
struct arg_type_extractor<false, arg_num, CallableT>
{
  using arg_type = void;
};

template <std::size_t arg_num, typename CallableT> struct arg_type
{
  using type = typename arg_type_extractor<
        arg_num < function_traits<CallableT>::arity::value
      , arg_num
      , CallableT
    >::arg_type;
};

// --------
// calculate result type of parallel tasks as
//     std::tuple<result_type1, result_type2, ...>
// Note: for tasks not returning value it has to replace void with placeholder void*

template <typename... Args>
struct parallel_result
{
  using type = std::tuple<
      typename std::conditional<
          std::is_same<typename std::decay<typename std::decay<Args>::type::result_t>::type, void>::value
        , void*
        , typename std::remove_reference<Args>::type::result_t>::type...
      >;
};
  
// --------
// result type  of sequential tasks is a result of the last task in the sequence
template <typename... Args>
class sequence_result
{
  using sequence = std::tuple<typename std::decay<Args>::type::result_t...>;

 public:
  using type = typename std::tuple_element<std::tuple_size<sequence>::value - 1, sequence>::type;
};

// --------
// parameter type of the first task in the sequence
template <typename... Args>
class first_task
{
  template <typename TaskT, typename... MoreTaskT>
  struct helper
  {
    using type = TaskT;
  };

 public:
  using type = typename std::decay<typename helper<Args...>::type>::type;
};

// --------
// all_same::value is true if all types in paramter pack are the same
// type (after std::decay) and false if not

template<typename... T>
struct all_same : std::false_type
{ };

template<>
struct all_same<> : std::true_type
{ };

template<typename T>
struct all_same<T> : std::true_type
{ };


template<typename T, typename... Ts>
struct all_same<T, T, Ts...> : all_same<T, Ts...>
{ };

// --------
// all_chained::value is true if for all task types in the parameter pack
// the parameter type of the next task is the same as the result type of the
// preceding task

template <typename T, typename Y, typename... Ts>
struct all_chained
{
  using result = std::integral_constant<bool, all_chained<T, Y>::result::value && all_chained<Y, Ts...>::result::value>;
};

template <typename T, typename Y>
struct all_chained<T, Y>
{
  using result = std::integral_constant<bool, std::is_same<typename std::decay<typename T::result_t>::type, typename std::decay<typename Y::parameter_t>::type>::value>;
};

} } } } // namespace

#endif
