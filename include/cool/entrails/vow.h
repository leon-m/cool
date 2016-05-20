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

#if !defined(ENTRAILS_VOW_H_HEADER_GUARD)
#define ENTRAILS_VOW_H_HEADER_GUARD

#include <atomic>

namespace cool { namespace basis {

template <typename T> class vow;

namespace entrails {

class state_base
{
 protected:
  enum States { EMPTY, FAILURE, SUCCESS, DEPLETED };

 public:
  typedef state_base              this_t;
  typedef std::shared_ptr<this_t> ptr_t;

 public:
  typedef std::function<void (const std::exception_ptr&)> failure_cb_t;

  state_base(const state_base&)             = delete;
  state_base(state_base&&)                  = delete;
  state_base& operator= (const state_base&) = delete;
  state_base& operator= (state_base&&)      = delete;

  std::mutex& mutex();
  const std::mutex& mutex() const;
  std::condition_variable& cv();
  const std::condition_variable& cv() const;

  failure_cb_t& on_failure();
  const failure_cb_t& on_failure() const;
  void on_failure(const failure_cb_t&);
  const std::exception_ptr& failure();
  dlldecl void failure(const std::exception_ptr& err);

  bool is_set() const;
  bool is_success() const;
  bool is_failure() const;
  bool is_empty() const;
  bool is_depleted() const;

  void deplete();
  int dec_vow_ref_cnt();
  void inc_vow_ref_cnt();

 protected:
  state_base();

 protected:
  States                  m_state;
  std::atomic<int>        m_vow_ref_cnt;
  std::exception_ptr      m_error;
  failure_cb_t            m_failure_cb;
  std::mutex              m_lock;
  std::condition_variable m_cv;
};

template <typename T> class state : public state_base
{
 public:
  typedef state<T>                                        this_t;
  typedef T                                               result_t;
  typedef std::shared_ptr<this_t>                         ptr_t;
  typedef std::function<void (const T&)>                  success_cb_t;
  typedef std::function<void (const vow<T> &, const T&)>  chained_cb_t;

 public:
  state();
  ~state();

  success_cb_t& on_success();
  const success_cb_t& on_success() const;
  void on_success(const success_cb_t&);
  const T& get_result();
  void set_result(const T& value);

 private:
  result_t     m_result;
  success_cb_t m_success_cb;
};

template <> class state<void> : public state_base
{
 public:
  typedef state                                   this_t;
  typedef void                                    result_t;
  typedef std::shared_ptr<this_t>                 ptr_t;
  typedef std::function<void (void)>              success_cb_t;
  typedef std::function<void (const vow<void> &)> chained_cb_t;

 public:
  state();
  ~state();

  success_cb_t& on_success();
  const success_cb_t& on_success() const;
  void on_success(const success_cb_t&);
  void get_result();
  void set_result();

 private:
  success_cb_t m_success_cb;
};


// --------------------------------------------------------------------------
//
// state implementation
//
// --------------------------------------------------------------------------

inline state_base::state_base() : m_state(EMPTY), m_vow_ref_cnt(1)
{ /* noop */ }

inline const std::mutex& state_base::mutex() const
{
  return m_lock;
}

inline std::mutex& state_base::mutex()
{
  return m_lock;
}

inline std::condition_variable& state_base::cv()
{
  return m_cv;
}

inline const std::condition_variable& state_base::cv() const
{
  return m_cv;
}

inline state_base::failure_cb_t& state_base::on_failure()
{
  return m_failure_cb;
}

inline const state_base::failure_cb_t& state_base::on_failure() const
{
  return m_failure_cb;
}

inline void state_base::on_failure(const state_base::failure_cb_t& fcb)
{
  m_failure_cb = fcb;
}

inline const std::exception_ptr& state_base::failure()
{
  deplete();
  return m_error;
}

inline bool state_base::is_empty() const
{
  return m_state == EMPTY;
}
inline bool state_base::is_depleted() const
{
  return m_state == DEPLETED;
}
inline bool state_base::is_set() const
{
  return m_state == SUCCESS || m_state == FAILURE;
}
inline bool state_base::is_failure() const
{
  return m_state == FAILURE;
}
inline bool state_base::is_success() const
{
  return m_state == SUCCESS;
}
inline void state_base::deplete()
{
  m_state = DEPLETED;
}
inline int state_base::dec_vow_ref_cnt()
{
  return --m_vow_ref_cnt;
}
inline void state_base::inc_vow_ref_cnt()
{
  ++m_vow_ref_cnt;
}
// -------------------------
template <typename T>
state<T>::state()
{ /* noop */ }

template <typename T>
state<T>::~state()
{ /* noop */ }

template <typename T>
typename state<T>::success_cb_t& state<T>::on_success()
{
  return m_success_cb;
}

template <typename T>
const typename state<T>::success_cb_t& state<T>::on_success() const
{
  return m_success_cb;
}

template <typename T>
void state<T>::on_success(const typename state<T>::success_cb_t& scb)
{
  m_success_cb = scb;
}

template <typename T>
const T& state<T>::get_result()
{
  deplete();
  return m_result;
}

template <typename T>
void state<T>::set_result(const T& arg)
{
  if (!is_empty())
    throw exception::illegal_state("This state was already set.");
  m_state = SUCCESS;
  m_result = arg;
}

// -------------------------
inline state<void>::state()
{ /* noop */ }

inline state<void>::~state()
{ /* noop */ }

inline state<void>::success_cb_t& state<void>::on_success()
{
  return m_success_cb;
}

inline const state<void>::success_cb_t& state<void>::on_success() const
{
  return m_success_cb;
}

inline void state<void>::on_success(const state<void>::success_cb_t& scb)
{
  m_success_cb = scb;
}

inline void state<void>::get_result()
{
  deplete();
}

} // namespace

} } // namespace

#endif
