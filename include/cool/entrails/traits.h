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

#if !defined(COOL_TRAITS_H_HEADER_GUARD)
#define COOL_TRAITS_H_HEADER_GUARD

namespace cool { namespace gcd { namespace traits {

template <typename...>
struct arg_type;

template <typename Arg>
struct arg_type<Arg>
{
  typedef Arg first_arg;
  typedef std::true_type has_one_arg;
};

template <typename Arg, typename... Args>
struct arg_type<Arg, Args...>
{
  typedef Arg first_arg;
  typedef std::false_type has_one_arg;
};

template <>
struct arg_type<>
{
  typedef void first_arg;
  typedef std::false_type has_one_arg;
};


template <typename L>
struct info : info<decltype(&L::operator())> { };

template <typename Class, typename R, typename... Args>
struct info<R(Class::*)(Args...) const>
{
  typedef R result;
  typedef typename arg_type<Args...>::first_arg first_arg;
  typedef typename arg_type<Args...>::has_one_arg has_one_arg;
};
template <typename Class, typename R, typename... Args>
struct info<R(Class::*)(Args...)>
{
  typedef R result;
  typedef typename arg_type<Args...>::first_arg first_arg;
  typedef typename arg_type<Args...>::has_one_arg has_one_arg;
};

}}}

#endif
