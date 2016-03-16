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
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>
#include "cool/vow.h"

class test_vow : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(test_vow);
  CPPUNIT_TEST(test_one);
  CPPUNIT_TEST(test_two);
  CPPUNIT_TEST_SUITE_END();
  
public:
  void test_one();
  void test_two();
};

CPPUNIT_TEST_SUITE_REGISTRATION(test_vow);

using namespace cool::basis;

void test_vow::test_one()
{
  {
    vow<void>* v = new vow<void>();
    auto a = v->get_aim();
    delete v;
    CPPUNIT_ASSERT_THROW(a.get(std::chrono::milliseconds(20)), cool::exception::broken_vow);
  }
  {
    vow<int>* v = new vow<int>();
    auto a = v->get_aim();
    delete v;
    CPPUNIT_ASSERT_THROW(a.get(std::chrono::milliseconds(20)), cool::exception::broken_vow);
  }
  {
    vow<void> v;
    auto a = v.get_aim();
    CPPUNIT_ASSERT_THROW(a.get(std::chrono::milliseconds(20)), cool::exception::timeout);
  }
  {
    vow<int>v;
    auto a = v.get_aim();
    CPPUNIT_ASSERT_THROW(a.get(std::chrono::milliseconds(20)), cool::exception::timeout);
  }
  {
    vow<void> v;
    auto a = v.get_aim();
    v.set();
    CPPUNIT_ASSERT_NO_THROW(a.get(std::chrono::milliseconds(20)));
  }
  {
    vow<int>v;
    auto a = v.get_aim();
    int res = 0;
    CPPUNIT_ASSERT(!v.is_set());
    v.set(10);
    CPPUNIT_ASSERT(v.is_set());
    CPPUNIT_ASSERT_NO_THROW(res = a.get(std::chrono::milliseconds(20)));
    CPPUNIT_ASSERT_EQUAL(10, res);
  }
  {
    vow<void> v;
    auto a = v.get_aim();
    CPPUNIT_ASSERT(!v.is_set());
    v.set();
    CPPUNIT_ASSERT(v.is_set());
    CPPUNIT_ASSERT_NO_THROW(a.get(std::chrono::milliseconds(20)));
  }
  {
    vow<int>v;
    auto a = v.get_aim();
    int res = 0;
    CPPUNIT_ASSERT(!v.is_set());
    v.set(10);
    CPPUNIT_ASSERT(v.is_set());
    CPPUNIT_ASSERT_NO_THROW(res = a.get(std::chrono::milliseconds(20)));
    CPPUNIT_ASSERT_EQUAL(10, res);
  }
  {
    vow<void> v;
    auto a = v.get_aim();
    CPPUNIT_ASSERT(!v.is_set());
    v.set(std::make_exception_ptr(cool::exception::not_found("")));
    CPPUNIT_ASSERT(v.is_set());
    CPPUNIT_ASSERT_THROW(a.get(std::chrono::milliseconds(20)), cool::exception::not_found);
  }
  {
    vow<int>v;
    auto a = v.get_aim();
    int res = 0;
    CPPUNIT_ASSERT(!v.is_set());
    v.set(std::make_exception_ptr(cool::exception::not_found("")));
    CPPUNIT_ASSERT(v.is_set());
    CPPUNIT_ASSERT_THROW(res = a.get(std::chrono::milliseconds(20)), cool::exception::not_found);
    CPPUNIT_ASSERT_EQUAL(0, res);
  }
  {
    vow<void> v;
    auto a = v.get_aim();
    v.set();
    CPPUNIT_ASSERT_NO_THROW(a.get(std::chrono::milliseconds(20)));
//    CPPUNIT_ASSERT_THROW(a.get(std::chrono::milliseconds(20)), cool::exception::illegal_state);
  }
  {
    vow<void> v;
    auto a = v.get_aim();
    v.set(std::make_exception_ptr(cool::exception::not_found("")));
    CPPUNIT_ASSERT_THROW(a.get(std::chrono::milliseconds(20)), cool::exception::not_found);
//    CPPUNIT_ASSERT_THROW(a.get(std::chrono::milliseconds(20)), cool::exception::illegal_state);
  }
  {
    vow<int>v;
    auto a = v.get_aim();
    int res = 0;
    v.set(10);
    CPPUNIT_ASSERT_NO_THROW(res = a.get(std::chrono::milliseconds(20)));
    CPPUNIT_ASSERT_THROW(res = a.get(std::chrono::milliseconds(20)), cool::exception::illegal_state);
    CPPUNIT_ASSERT_EQUAL(10, res);
  }
  {
    vow<int>v;
    auto a = v.get_aim();
    int res = 0;
    v.set(std::make_exception_ptr(cool::exception::not_found("")));
    CPPUNIT_ASSERT_THROW(res = a.get(std::chrono::milliseconds(20)), cool::exception::not_found);
    CPPUNIT_ASSERT_THROW(res = a.get(std::chrono::milliseconds(20)), cool::exception::illegal_state);
    CPPUNIT_ASSERT_EQUAL(0, res);
  }
}

// scenario: two threads blocked on get() on clones, one should get result,
// the other illegal_state, no timeout should be received
void test_vow::test_two()
{
  {
    vow<int> v;
    auto a1 = v.get_aim();
    auto a2 = a1;
    auto a3 = a1;
    bool ill = false;
    bool tmo = false;
    bool oth = false;
    int res = 0;
    
    std::thread t1(
      [&] ()
      {
        try { res = a1.get(std::chrono::milliseconds(500)); }
        catch (const cool::exception::illegal_state&) { ill = true; }
        catch (const cool::exception::timeout&) { tmo = true; }
        catch (...) { oth = true; }
      });
    std::thread t2(
      [&] ()
      {
        try { res = a2.get(std::chrono::milliseconds(500)); }
        catch (const cool::exception::illegal_state&) { ill = true; }
        catch (const cool::exception::timeout&) { tmo = true; }
        catch (...) { oth = true; }
      });
    std::thread t3(
      [&] ()
      {
        try { res = a3.get(std::chrono::milliseconds(500)); }
        catch (const cool::exception::illegal_state&) { ill = true; }
        catch (const cool::exception::timeout&) { tmo = true; }
        catch (...) { oth = true; }
      });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    v.set(42);
    t1.join();
    t2.join();
    t3.join();
    
    CPPUNIT_ASSERT_EQUAL(42, res);
    CPPUNIT_ASSERT(!tmo);
    CPPUNIT_ASSERT(ill);
  }
  {
    vow<void> v;
    auto a1 = v.get_aim();
    auto a2 = a1;
    auto a3 = a1;
    bool ill = false;
    bool tmo = false;
    bool oth = false;
    
    std::thread t1(
      [&] ()
      {
        try { a1.get(std::chrono::milliseconds(500)); }
        catch (const cool::exception::illegal_state&) { ill = true; }
        catch (const cool::exception::timeout&) { tmo = true; }
        catch (...) { oth = true; }
      });
    std::thread t2(
      [&] ()
      {
        try { a2.get(std::chrono::milliseconds(500)); }
        catch (const cool::exception::illegal_state&) { ill = true; }
        catch (const cool::exception::timeout&) { tmo = true; }
        catch (...) { oth = true; }
      });
    std::thread t3(
      [&] ()
      {
        try { a3.get(std::chrono::milliseconds(500)); }
        catch (const cool::exception::illegal_state&) { ill = true; }
        catch (const cool::exception::timeout&) { tmo = true; }
        catch (...) { oth = true; }
      });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    v.set();
    t1.join();
    t2.join();
    t3.join();
    
    CPPUNIT_ASSERT(!tmo);
    CPPUNIT_ASSERT(ill);
  }
}

