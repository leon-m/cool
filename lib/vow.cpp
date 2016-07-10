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
#if defined(WIN32_TARGET)
#define WIN32_COOL_BUILD
#endif

#include "cool/vow.h"

namespace cool { namespace basis {

void aim<void>::then(const aim_base<void>::state_t::success_cb_t& scb,
                     const aim_base<void>::state_t::failure_cb_t& fcb)
{
  bool fail;
  // if the result is already available, fire immediately
  if (then_base(fail, scb, fcb))
  {
    try
    {
      if (fail)
      {
        if (fcb)
          fcb(aim_base<void>::m_state->failure());
      }
      else
      {
        if (scb)
          scb();
      }
    }
    catch (...)
    { /* noop */ }
  }
}

aim<void>::this_t aim<void>::then(const aim_base<void>::state_t::chained_cb_t& ccb)
{
  vow_t* p = new vow_t();
  this_t h = p->get_aim();

  then(
    [=] ()
    {
      try {
        ccb(*p);
      }
      catch (...) {
        p->set(std::current_exception());
      }
      delete p;
    },
    [=] (const std::exception_ptr& err)
    {
      p->set(err);
      delete p;
    }
  );

  return h;
}

void vow<void>::set() const
{
  vow_base<void>::state_t::success_cb_t scb;

  {
    std::unique_lock<std::mutex> l (vow_base<void>::m_state->mutex());

    if (!vow_base<void>::m_state->is_empty())
      throw exception::illegal_state("This vow was already set.");

    vow_base<void>::m_state->set_result();
    scb = std::move(vow_base<void>::m_state->on_success());
    if (scb)
      vow_base<void>::m_state->deplete();
    else
      vow_base<void>::m_state->cv().notify_all();
  }

  if (scb)
  {
    scb();
  }
}


namespace entrails {

void state_base::failure(const std::exception_ptr& err)
{
  if (!is_empty())
    throw exception::illegal_state("This state was already set.");
  m_state = FAILURE;
  m_error = err;
}

void state<void>::set_result()
{
  if (!is_empty())
    throw exception::illegal_state("This state was already set.");
  m_state = SUCCESS;
}

} } // namespace

} // namespace

