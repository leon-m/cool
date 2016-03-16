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

#include <iostream>
#include <sstream>

#if defined(WIN32_TARGET)
#define WIN32_COOL_BUILD
#endif

#include "cool/gcd_task.h"
#include "cool/gcd_async.h"

namespace cool { namespace gcd { namespace task {

namespace entrails
{

void cleanup_reverse(taskinfo* info_)
{
  for ( ; info_->m_prev != nullptr; info_ = info_->m_prev)
  ;
  cleanup(info_);
}

void cleanup(taskinfo* info_)
{
  entrails::taskinfo* aux;
  
  while (info_ != nullptr)
  {
    aux = info_->m_next;
    delete info_;
    info_ = aux;
  }
}

taskinfo::~taskinfo()
{
  if (m_u.task != nullptr)
  {
    if (m_deleter)
      m_deleter();
    else
      delete m_u.task;
  }
}

void kickstart(taskinfo* info_)
{
  if (info_ == nullptr)
    throw cool::exception::illegal_state("this task object is in undefined state");
    
  auto&& aux = info_->m_runner.lock();
  if (!aux)
    throw runner_not_available();
    
  aux->task_run(info_);
}

void kickstart(taskinfo* info_, const std::exception_ptr& e_)
{
  auto aux = info_->m_runner.lock();
  if (aux)
  {
    aux->task_run(new task_t(std::bind(info_->m_eh,  e_)));
  }
}

// executor for task::run()
void task_executor(void* ctx)
{
  (*static_cast<taskinfo*>(ctx)->m_u.task)();
}

// executor for runner::run()
void executor(void* ctx)
{
  std::unique_ptr<task_t> task(static_cast<task_t*>(ctx));
  try { (*task)(); } catch (...) { }
}

} // namespace

#if defined(APPLE_TARGET)
runner::runner(dispatch_queue_attr_t queue_type)
    : named("si.digiverse.cool.runner")
    , m_active(true)
    , m_data(std::make_shared<entrails::queue>(
        ::dispatch_queue_create(name().c_str(), queue_type)
      , false))
{ /* noop */ }
#else
runner::runner()
    : named("si.digiverse.cool.runner")
    , m_active(true)
    , m_data(
          std::make_shared<entrails::queue>(::dispatch_queue_create(name().c_str(), NULL)
        , false))
{ /* noop */ }
#endif

runner::runner(const std::string& name, dispatch_queue_priority_t priority)
    : named(name)
    , m_active(true)
    , m_data(std::make_shared<entrails::queue>(
        ::dispatch_get_global_queue(priority, 0)
      , true))
{ /* noop */ }

runner::~runner()
{
  if (!m_data->m_is_system)
    start();
}

void runner::stop()
{
  if (!m_data->m_is_system)
  {
    bool expected = false;
    if (m_data->m_suspended.compare_exchange_strong(expected, true))
      ::dispatch_suspend(m_data->m_queue);
  }
}

void runner::task_run(entrails::taskinfo* info_)
{
  ::dispatch_async_f(m_data->m_queue, info_, entrails::task_executor);
}

void runner::task_run(entrails::task_t* task)
{
  ::dispatch_async_f(m_data->m_queue, task, entrails::executor);
}

void runner::start()
{
  bool expected = true;
  if (m_data->m_suspended.compare_exchange_strong(expected, false))
    ::dispatch_resume(m_data->m_queue);
}

// --------------- global variables ---------------------------
class system_runner : public runner
{
 public:
  system_runner(const std::string& name, dispatch_queue_priority_t priority)
      : runner(name, priority)
  { /* noop */ }
};

std::shared_ptr<runner> runner::sys_high_ptr()
{
  static const std::shared_ptr<system_runner> sys_runner_high(
      new system_runner("prio-high", DISPATCH_QUEUE_PRIORITY_HIGH));

  return sys_runner_high;
}
const runner& runner::sys_high()
{
  return *sys_high_ptr();
}

std::shared_ptr<runner> runner::sys_default_ptr()
{
  static const std::shared_ptr<system_runner> sys_runner_default(
      new system_runner("prio-def-", DISPATCH_QUEUE_PRIORITY_DEFAULT));

  return sys_runner_default;
}
const runner& runner::sys_default()
{
  return *sys_default_ptr();
}

const runner& runner::sys_low()
{
  static const std::shared_ptr<system_runner> sys_runner_low(
      new system_runner("prio-low-", DISPATCH_QUEUE_PRIORITY_LOW));
  return *sys_runner_low;
}

#if !defined(WIN32_TARGET)
std::shared_ptr<runner> runner::sys_background_ptr()
{
  static const std::shared_ptr<system_runner> sys_runner_background(
      new system_runner("prio-bg-", DISPATCH_QUEUE_PRIORITY_BACKGROUND));
  return sys_runner_background;
}
const runner& runner::sys_background()
{
  return *sys_background_ptr();
}
#endif

std::shared_ptr<runner> runner::cool_default_ptr()
{
  static const std::shared_ptr<runner> cool_serial_runner(new runner);

  return cool_serial_runner;
}
const runner& runner::cool_default()
{
  return *cool_default_ptr();
}

// --------------- group --------------------------------------

group::group()
{
  auto g = ::dispatch_group_create();
  if (g == NULL)
    throw exception::create_failure("Failed to greate task group");

  m_group = g;
}

group::group(const group& original)
{
  m_group = original.m_group;
  ::dispatch_retain(m_group);
}

group& group::operator=(const cool::gcd::task::group &original)
{
  m_group = original.m_group;
  ::dispatch_retain(m_group);
  return *this;
}

group::~group()
{
  ::dispatch_group_notify_f(m_group, runner::cool_default(), m_group, &group::finalizer);
}

void group::executor(void* ctx)
{
  std::unique_ptr<entrails::task_t> task(static_cast<entrails::task_t*>(ctx));
  try
  {
    (*task)();
  }
  catch (...)
  {
    // TODO: how to handle this?
  }
}

void group::finalizer(void *ctx)
{
  dispatch_group_t g = static_cast<dispatch_group_t>(ctx);
  ::dispatch_release(g);
}

void group::then(const handler_t& handler)
{
  void* ctx = new entrails::task_t(handler);
  ::dispatch_group_notify_f(m_group, runner::cool_default(), ctx, &group::executor);
}

void group::wait()
{
  ::dispatch_group_wait(m_group, ::dispatch_time(DISPATCH_TIME_FOREVER, 0));
}

void group::wait(int64_t interval)
{
  if (interval < 0)
    throw cool::exception::illegal_argument("Cannot use negative wait interval.");
  
  if (::dispatch_group_wait(m_group, ::dispatch_time(DISPATCH_TIME_NOW, interval)) == 0)
    return;
  
  throw cool::exception::timeout("Timeout while waiting for tasks to complete");
}

} } } // namespace
