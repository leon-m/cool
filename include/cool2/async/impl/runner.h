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

#if !defined(cool_369ae5cf_0587_4275_9725_891ed6de7631)
#define cool_369ae5cf_0587_4275_9725_891ed6de7631

#include <memory>
#include <functional>

namespace cool { namespace async {

class runner;

namespace impl {

class execution_context
{
 public:
  using entry_point = std::function<void(const std::shared_ptr<runner>&, execution_context*)>;
  using exception_reporter  = std::function<void(const std::exception_ptr&)>;
  using generic_deleter     = void(*)(void*);

 public:
  execution_context(const entry_point& ep_) : m_ep(ep_), m_res_reporter(nullptr)
  { /* noop */ }
  execution_context() :  m_res_reporter(nullptr)
  { /* noop */ }
  virtual ~execution_context()
  {
    if (m_res_reporter != nullptr)
      (*m_res_deleter)(m_res_reporter);
  }

  virtual const std::weak_ptr<runner>& get_runner() const = 0;

  const entry_point& get_entry_point() const      { return m_ep; }
  void* get_result_reporter() const               { return m_res_reporter; }
  const exception_reporter& get_exception_reporter() const { return m_exc_reporter; }

  void set_entry_point(const entry_point& ep_)
  {
    m_ep = ep_;
  }
  void set_result_reporter(void* rep_, const generic_deleter& del_)
  {
    m_res_reporter = rep_;
    m_res_deleter = del_;
  }
  void set_excetpion_reporter(const exception_reporter& er_)
  {
    m_exc_reporter = er_;
  }

 private:
  entry_point           m_ep;
  exception_reporter    m_exc_reporter;   // result reporter function
  void*                 m_res_reporter;   // pointer to result reporter function
  generic_deleter       m_res_deleter;    // deleter for result reporter function
};

} } } // namespace

#endif