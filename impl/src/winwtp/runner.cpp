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
#include "cool/exception.h"
#include "entrails/winwtp/runner.h"

namespace cool { namespace async { namespace entrails {

class poolmgr
{
 public:
  poolmgr() 
    : m_pool(NULL)
  {
    InitializeThreadpoolEnvironment(&m_environ);
    m_pool = CreateThreadpool(NULL);
    if (m_pool == NULL)
      throw cool::exception::operation_failed("failed to create thread pool");

    // Associate the callback environment with our thread pool.
    SetThreadpoolCallbackPool(&m_environ, m_pool);
  }

  ~poolmgr()
  {
    if (m_pool != NULL)
      CloseThreadpool(m_pool);

    DestroyThreadpoolEnvironment(&m_environ);
  }

  PTP_CALLBACK_ENVIRON get_environ()
  {
    return &m_environ;
  }

 private:
  PTP_POOL            m_pool;
  TP_CALLBACK_ENVIRON m_environ;
};

std::unique_ptr<poolmgr> runner::m_pool;
std::atomic<unsigned int> runner::m_refcnt(0);

runner::runner(RunPolicy policy_)
    : named("si.digiverse.cool2.runner")
    , m_work(NULL)
    , m_busy(false)
    , m_is_system(false)
    , m_active(true)
{
  if (m_refcnt++ == 0)
    m_pool.reset(new poolmgr());
  // Create work with the callback environment.
  m_work = CreateThreadpoolWork(task_executor, this, m_pool->get_environ());
  if (m_work == NULL)
    throw exception::operation_failed("failed to create work");
}

runner::~runner()
{
	{
		std::unique_lock<std::mutex> l(m_);
  if  (m_work != NULL)
    CloseThreadpoolWork(m_work);
  }

  if (--m_refcnt == 0)
    m_pool.reset();
}

void runner::start()
{
}

void runner::stop()
{
}

VOID CALLBACK runner::task_executor(PTP_CALLBACK_INSTANCE instance_, PVOID pv_, PTP_WORK work_)
{
  static_cast<runner*>(pv_)->task_executor();
}

void runner::task_executor()
{
  impl::context_ptr ctx;
  {
    std::unique_lock<std::mutex> l(m_);
    ctx = m_fifo.front();
    m_fifo.pop_front();
  }

  auto r = ctx->m_runner.lock();
  if (r)
    ctx->m_ctx.simple()->entry_point()(r, ctx);

  {
    std::unique_lock<std::mutex> l(m_);

    if (m_fifo.empty())
      m_busy = false;
    else
      start_work();
  }
}

void runner::run(const impl::context_ptr& ctx_)
{
  {
    std::unique_lock<std::mutex> l(m_);
    m_fifo.push_back(ctx_);
  }
  boolean expected = false;
  if (m_busy.compare_exchange_strong(expected, true))
     start_work();
}

void runner::start_work()
{
  SubmitThreadpoolWork(m_work);
}

} } } // namespace
