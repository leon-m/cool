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
#include <thread>
#include <chrono>
#include <iomanip>
#include <iostream>
#include "cool/cool.h"

class test_runner : public cool::gcd::task::runner
{
public:
  cool::basis::aim<int> test_one();
};

int countdown = 10;

int f()
{
  return 8;
}

using namespace cool::basis;

#if !defined(_MSC_VER)
cool::basis::aim<int> test_runner::test_one()
{
  int input = 21;
  return run(
    [](int n)
    {
      std::this_thread::sleep_for(std::chrono::seconds(10));
      return n * 2; 
    },
    input
  );
}
#else
cool::basis::aim<int> test_runner::test_one()
{
  int n = 21;
  return run(
    [n]()
  {
    std::this_thread::sleep_for(std::chrono::seconds(10));
    return n * 2;
  }
    );
}
#endif
cool::basis::aim<void> f00001()
{
  return cool::gcd::task::runner::sys_default().run(
    [] () { }
  );
}
#if defined(WINS32_NO_LONGER_FAILS)
cool::basis::aim<int> f00004()
{
  return cool::gcd::task::runner::sys_default().run(
    [] () { return 0; }
  );
}
#endif

#if defined(NO_LONGER_FAILS)
cool::basis::aim<int> f00002()
{
  return cool::gcd::task::runner::sys_default().run(
    [] (int n) -> int
    {
      return n;
    }
    , 5
  );
}
cool::basis::aim<int> f00010()
{
  return cool::gcd::task::runner::sys_default().run(
    [] (int arg) { return arg; }
    , f()
  );
}
#endif
#if !defined(_MSC_VER)
cool::basis::aim<int> f00006()
{
  const int c = 5;
  return cool::gcd::task::runner::sys_default().run(
    [] (int n) -> int
    {
      return n;
    }
    , c
  );
}
cool::basis::aim<int> f00003(const int& n)
{
  return cool::gcd::task::runner::sys_default().run(
    [] (int arg) { return arg; }
    , n
  );
}
cool::basis::aim<int> f00005(const int& n)
{
  return cool::gcd::task::runner::sys_default().run(
    [] (const int& arg) { return arg; }
    , n
  );
}
cool::basis::aim<int> f00007()
{
  const int c = 5;
  return cool::gcd::task::runner::sys_default().run(
    [] (const int& arg) { return arg; }
    , c
  );
}
#endif
#if defined(WIN32_NO_LONGER_FAILS)
cool::basis::aim<void> f00011()
{
  const int c = 5;
  return cool::gcd::task::runner::sys_default().run(
    [](const int& arg) { }
  , c
    );
}
#endif
#if defined(NO_LONGER_FAILS)
cool::basis::aim<int> f00008()
{
  return cool::gcd::task::runner::sys_default().run(
    [] (const int& arg) { return arg; }
    , 5
  );
}
cool::basis::aim<int> f00009()
{
  return cool::gcd::task::runner::sys_default().run(
    [] (const int& arg) { return arg; }
    , f()
  );
}
#endif


int main(int argc, char* argv[])
{
  test_runner r;

  auto a = r.test_one();
  cool::gcd::async::timer t(
    [] (unsigned long count)
    {
      std::cout << countdown-- << " "; std::flush(std::cout);
    },
    cool::gcd::task::runner::sys_default(),
    std::chrono::seconds(1)
  );
  t.start();

  for (int i = 0; i < 100; ++i)
    std::cout << i;
  std::cout << std::endl;
  std::cout << "Now calculating result: ";
  std::flush(std::cout);
  int res = a.get();
  std::cout << " ======> " << res << std::endl;
}
