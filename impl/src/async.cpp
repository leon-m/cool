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

#include <memory>

#include "cool2/async.h"
#include "entrails/runner.h"

namespace cool { namespace async {

// ----- implementation of cool::async::runner
//
runner::runner(RunPolicy policy_)
{
  m_impl = std::make_shared<entrails::runner>(policy_);
}

runner::~runner()
{ /* noop */ }

const std::string& runner::name() const
{
  return m_impl->name();
}

const std::shared_ptr<entrails::runner> runner::impl() const
{
  return m_impl;
}

std::shared_ptr<entrails::runner> runner::impl()
{
  return m_impl;
}

void runner::start()
{
  m_impl->start();
}

void runner::stop()
{
  m_impl->stop();
}

namespace entrails
{

void kick(const impl::context_ptr& ctx_)
{
  if (!ctx_)
    throw cool::exception::illegal_state("this task object is in undefined state");

  auto aux = ctx_->m_runner.lock();
  if (!aux)
    throw runner_not_available();

  aux->impl()->run(ctx_);

}

} // namespace

// ----- implementation of impl::task helpers
namespace impl {

u_one_of::u_one_of() : m_type(context_type::not_set)
{ /* noop */ }

u_one_of::~u_one_of()
{
  release();
}

void u_one_of::release()
{
  switch (m_type)
  {
    case context_type::simple:    m_u.m_simple.~shared_ptr(); break;
    case context_type::intercept: m_u.m_intercept.~shared_ptr(); break;
    case context_type::serial:    m_u.m_serial.~shared_ptr(); break;
    case context_type::parallel:  m_u.m_parallel.~shared_ptr(); break;
    case context_type::not_set:   break;
  }
  m_type = context_type::not_set;
}

void u_one_of::set(const std::shared_ptr<simple::context>& arg_)
{
  release();
  m_type = context_type::simple;
  m_u.m_simple = arg_;
}
void u_one_of::set(const std::shared_ptr<serial::context>& arg_)
{
  release();
  m_type = context_type::serial;
  m_u.m_serial = arg_;
}
void u_one_of::set(const std::shared_ptr<parallel::context>& arg_)
{
  release();
  m_type = context_type::parallel;
  m_u.m_parallel = arg_;
}
void u_one_of::set(const std::shared_ptr<intercept::context>& arg_)
{
  release();
  m_type = context_type::intercept;
  m_u.m_intercept = arg_;
}
std::shared_ptr<simple::context> u_one_of::simple()
{
  if (m_type != context_type::simple)
    throw cool::exception::bad_conversion("wrong content type");
  return m_u.m_simple;
}
std::shared_ptr<serial::context> u_one_of::serial()
{
  if (m_type != context_type::serial)
    throw cool::exception::bad_conversion("wrong content type");
  return m_u.m_serial;
}
std::shared_ptr<parallel::context> u_one_of::parallel()
{
  if (m_type != context_type::parallel)
    throw cool::exception::bad_conversion("wrong content type");
  return m_u.m_parallel;
}
std::shared_ptr<intercept::context> u_one_of::intercept()
{
  if (m_type != context_type::intercept)
    throw cool::exception::bad_conversion("wrong content type");
  return m_u.m_intercept;
}


#if 0





info::info() : m_type(TaskType::Unknown)
{ /* noop */
  std::cout << "+++++ info" << std::endl;
}

info::~info()
{
  std::cout << "----- info" << std::endl;
  release();
}

void info::release()
{
  switch (m_type)
  {
    case TaskType::Simple:           u.m_simple.~shared_ptr(); break;
    case TaskType::Intercept:        u.m_intercept.~shared_ptr(); break;
    case TaskType::CompoundSerial:   u.m_serial.~shared_ptr(); break;
    case TaskType::CompoundParallel: u.m_parallel.~shared_ptr(); break;
    case TaskType::Unknown: break;
  }
  m_type = TaskType::Unknown;
}

void info::set(const std::shared_ptr<simple::info>& arg_)
{
  release();
  m_type = TaskType::Simple;
  u.m_simple = arg_;
}
void info::set(const std::shared_ptr<serial::info>& arg_)
{
  release();
  m_type = TaskType::CompoundSerial;
  u.m_serial = arg_;
}
void info::set(const std::shared_ptr<parallel::info>& arg_)
{
  release();
  m_type = TaskType::CompoundParallel;
  u.m_parallel = arg_;
}
void info::set(const std::shared_ptr<intercept::info>& arg_)
{
  release();
  m_type = TaskType::Intercept;
  u.m_intercept = arg_;
}

const task_callback_t& info::callable() const
{
  switch (m_type)
  {
    case TaskType::Simple:
      return u.m_simple->m_callable.bound();
    case TaskType::CompoundSerial:
    case TaskType::CompoundParallel:
      break;
  }
}

std::weak_ptr<runner> info::runner() const
{
  switch (m_type)
  {
    case TaskType::Simple:
      return u.m_simple->m_runner;
    case TaskType::CompoundSerial:
    case TaskType::CompoundParallel:
      break;
  }

}
#endif

} // namespace



} } //namespace