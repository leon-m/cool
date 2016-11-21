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
#include "cool2/async/impl/runner.h"
#include "entrails/winwtp/runner.h"

namespace cool { namespace async { namespace entrails {

/*constexpr*/ const int TASK = 1;

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
    , m_fifo(NULL)
    , m_work_in_progress(false)
    , m_is_system(false)
    , m_active(true)
{
  if (m_refcnt++ == 0)
    m_pool.reset(new poolmgr());

  try {
    m_fifo = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    if (m_fifo == NULL)
      throw cool::exception::operation_failed("failed to create i/o completion port");

    try {
      // Create work with the callback environment.
      m_work = CreateThreadpoolWork(task_executor, this, m_pool->get_environ());
      if (m_work == NULL)
        throw exception::operation_failed("failed to create work");
    }
    catch (...) {
      CloseHandle(m_fifo);
      throw;
    }
  }
  catch (...) {
    if (--m_refcnt == 0)
      m_pool.reset();
    throw;
  }

}

runner::~runner()
{
  if  (m_work != NULL)
    CloseThreadpoolWork(m_work);

  if (m_fifo != NULL)
    CloseHandle(m_fifo);

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
  LPOVERLAPPED aux;
  DWORD        cmd;
  ULONG_PTR    key;

  if (!GetQueuedCompletionStatus(m_fifo, &cmd, &key, &aux, 0))
  {
    m_work_in_progress = false;
    return;
  }

  if (cmd != TASK)
    return;

  auto ctx = reinterpret_cast<cool::async::impl::execution_context*>(aux);
  auto r = ctx->get_runner().lock();

  if (r)
    ctx->get_entry_point()(r, ctx);
  else
    delete ctx;

  start_work();
}

void runner::run(cool::async::impl::execution_context* ctx_)
{
  PostQueuedCompletionStatus(m_fifo, TASK, NULL, reinterpret_cast<LPOVERLAPPED>(ctx_));

  bool expected = false;
  if (m_work_in_progress.compare_exchange_strong(expected, true))
    start_work();
}

void runner::start_work()
{
  SubmitThreadpoolWork(m_work);
}

} } } // namespace
