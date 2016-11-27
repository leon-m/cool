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

#if !defined(cool_1c6a4535_705a_4686_a315_30e4583a7a5b)
#define cool_1c6a4535_705a_4686_a315_30e4583a7a5b

#define WIN32_MEAN_AND_LEAN
#include <windows.h>

#include <atomic>
#include <mutex>
#include <memory>
#include <deque>

#include "cool2/async.h"
#include "cool2/named.h"

namespace cool { namespace async { namespace entrails {

class poolmgr;

class runner : public misc::named
{
  using queue_t = HANDLE;

 public:
  runner(RunPolicy policy_);
  ~runner();

  void start();
  void stop();
  void run(cool::async::impl::context*);

 private:
  static VOID CALLBACK task_executor(PTP_CALLBACK_INSTANCE instance_, PVOID pv_, PTP_WORK work_);
  void task_executor();
  void start_work();

 private:
  PTP_WORK          m_work;
  queue_t           m_fifo;
  std::atomic<bool> m_work_in_progress;

  static std::atomic<unsigned int> m_refcnt;
  static std::unique_ptr<poolmgr>  m_pool;


  const bool        m_is_system;
  std::atomic<bool> m_active;

};

} } } // namespace

#endif

