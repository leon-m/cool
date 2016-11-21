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

#include "cool2/async.h"
#include "cool2/async/impl/runner.h"
#include "cool/exception.h"
#include "entrails/gcd/runner.h"

namespace cool { namespace async { namespace entrails {

runner::runner(RunPolicy policy_)
    : named("si.digiverse.cool2.runner")
    , m_is_system(false)
    , m_active(true)
{
#if !defined(LINUX_TARGET)
  if (policy_ == RunPolicy::CONCURRENT)
    m_queue = ::dispatch_queue_create(name().c_str(), DISPATCH_QUEUE_CONCURRENT);
  else
#endif
    m_queue = ::dispatch_queue_create(name().c_str(), NULL);
}

runner::~runner()
{
  start();
  if (!m_is_system)
    dispatch_release(m_queue);
}

void runner::start()
{
  if (!m_is_system)
  {
    bool expect = false;
    if (m_active.compare_exchange_strong(expect, true))
    {
      ::dispatch_resume(m_queue);
    }
  }
}

void runner::stop()
{
  if (!m_is_system)
  {
    bool expect = true;
    if (m_active.compare_exchange_strong(expect, false))
    {
      ::dispatch_suspend(m_queue);
    }
  }
}

void runner::run(cool::async::impl::execution_context* ctx_)
{
  ::dispatch_async_f(m_queue, ctx_, task_executor);
}

// executor for task::run()
void runner::task_executor(void* arg_)
{
  auto ctx = static_cast<cool::async::impl::execution_context*>(arg_);

  auto r = ctx->get_runner().lock();
  if (r)
    ctx->get_entry_point()(r, ctx);
  else
    delete ctx;
}
  

} } } // namespace