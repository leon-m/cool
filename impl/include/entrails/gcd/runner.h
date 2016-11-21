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

#if !defined(cool_d2aa9442_15ec_4748_9d69_a7d096d1b861)
#define cool_d2aa9442_15ec_4748_9d69_a7d096d1b861

#include <atomic>
#include <memory>
#include <dispatch/dispatch.h>
#include "cool2/async.h"
#include "cool2/named.h"
#include "entrails/runner.h"

namespace cool { namespace async { namespace entrails {

class runner : public misc::named
{
 public:
  using ptr_t = std::shared_ptr<runner>;
  using weak_ptr_t = std::weak_ptr<runner>;

 public:
  runner(RunPolicy policy_);
  ~runner();

  void start();
  void stop();
  void run(cool::async::impl::execution_context*);

 private:
  static void task_executor(void*);

 private:
  const bool        m_is_system;
  std::atomic<bool> m_active;
  dispatch_queue_t  m_queue;
};

} } } // namespace

#endif

