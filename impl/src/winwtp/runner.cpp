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
  }

  ~poolmgr()
  {
    if (m_pool != NULL)
      CloseThreadpool(m_pool);

    DestroyThreadpoolEnvironment(&m_environ);
  }

  PTP_POOL            m_pool;
  TP_CALLBACK_ENVIRON m_environ;
};

std::unique_ptr<poolmgr> runner::m_pool;

runner::runner(RunPolicy policy_)
    : named("si.digiverse.cool2.runner")
    , m_is_system(false)
    , m_active(true)
{
  if (m_refcnt++ == 0)
    m_pool.reset(new poolmgr());
}

runner::~runner()
{
  if (--m_refcnt == 0)
    m_pool.reset();
}

void runner::start()
{
}

void runner::stop()
{
}

} } } // namespace
