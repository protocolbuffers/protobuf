// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// -----------------------------------------------------------------------------
// File: access_guard.h
// -----------------------------------------------------------------------------
//
// This header file defines `AccessGuard`, which provides non-blocking,
// fail-fast concurrency detection for Python protobuf objects under
// free-threaded Python (Py_GIL_DISABLED).
//
// Python protobuf messages are not safe for concurrent mutation across threads,
// nor for concurrent reading and writing of the same message hierarchy. Under
// free-threaded Python (PEP 703), there is no Global Interpreter Lock (GIL) to
// serialize C-API operations on a message tree. Rather than paying the cost of
// blocking mutexes or risking deadlocks across complex message graphs,
// `AccessGuard` acts as a lightweight, non-blocking read/write guard shared by
// all Python wrapper objects in a message tree:
//
//   * Multiple threads may hold shared read locks simultaneously.
//   * Only a single thread may hold an exclusive write lock, and only when no
//     shared read locks are held.
//   * If any lock acquisition encounters contention (read vs. write, or write
//     vs. write), acquisition fails immediately without blocking, increments a
//     diagnostic counter, and sets a Python RuntimeError exception
//     ("Concurrent message access detected") on the calling thread.
//
// In standard GIL-enabled builds (when Py_GIL_DISABLED is not defined), all
// `AccessGuard` operations compile down to inline no-ops that always succeed
// with zero runtime overhead.
//
// Users construct an `AccessGuard` inside their Python object (or inside a
// shared hierarchy object such as `MessageTree`) and access guarded state via
// scoped callbacks:
//
//   * `AccessGuard::WithRead()` / `WithWrite()` acquire a single guard and pass
//     a `ReadAccess` / `WriteAccess` token to the callback.
//   * `AccessGuard::WithWriteRead()` / `WithReadRead()` acquire two guards
//     (which may be the same guard) and pass two tokens.
//
// Tokens are proof that a lock is held. They are passed to `Guarded<T>::Get()`
// and `Guarded<T>::Set()` (see guarded.h) to access any value protected by the
// same `AccessGuard`, with zero additional atomic operations. A single guard
// typically protects many `Guarded<T>` values, e.g. every object in a message
// tree.

#ifndef GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_ACCESS_GUARD_H__
#define GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_ACCESS_GUARD_H__

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"

namespace google {
namespace protobuf {
namespace python {

class AccessGuard;

namespace free_threading_internal {

// Returns on_fail() if on_fail is callable, otherwise on_fail itself.
template <typename OnFail>
auto InvokeOrValue(OnFail&& on_fail) {
  if constexpr (std::is_invocable_v<OnFail>) {
    return on_fail();
  } else {
    return on_fail;
  }
}

// Produces the result of a scoped access call whose lock acquisition (or
// subsequent validation) failed: on_fail() if on_fail is callable, otherwise
// on_fail itself, implicitly converted to RetT. For RetT == void, a callable
// on_fail is invoked for its side effects.
template <typename RetT, typename OnFail>
RetT Fail(OnFail&& on_fail) {
  if constexpr (std::is_void_v<RetT>) {
    if constexpr (std::is_invocable_v<OnFail>) {
      std::forward<OnFail>(on_fail)();
    }
  } else {
    return InvokeOrValue(std::forward<OnFail>(on_fail));
  }
}

// Whether `Guarded<T>` records and checks which `AccessGuard` protects it.
//
// This changes the layout of `Guarded<T>`, so it must be uniform across a whole
// binary. It is therefore keyed on sanitizers (which are all-or-nothing) rather
// than NDEBUG (which is commonly mixed within one binary).
#if defined(ABSL_HAVE_ADDRESS_SANITIZER) || \
    defined(ABSL_HAVE_MEMORY_SANITIZER) || defined(ABSL_HAVE_THREAD_SANITIZER)
inline constexpr bool kTrackGuards = true;
#else
inline constexpr bool kTrackGuards = false;
#endif

// Records which `AccessGuard` protects a guarded value, and CHECKs that tokens
// presented for it refer to that guard. Atomics use relaxed ordering: these are
// only identity checks and must not add happens-before edges that would hide
// races from TSan.
class ABSL_ATTRIBUTE_TRIVIAL_ABI GuardTracker {
 public:
  GuardTracker() = default;
  explicit GuardTracker(const AccessGuard* guard) : guard_(guard) {}

  GuardTracker(const GuardTracker& other)
      : guard_(other.guard_.load(std::memory_order_relaxed)) {}

  GuardTracker& operator=(const GuardTracker& other) {
    guard_.store(other.guard_.load(std::memory_order_relaxed),
                 std::memory_order_relaxed);
    return *this;
  }

  void Set(const AccessGuard* guard) {
    guard_.store(guard, std::memory_order_relaxed);
  }

  void CheckMatches(const AccessGuard* expected) const {
    ABSL_CHECK_EQ(guard_.load(std::memory_order_relaxed), expected)
        << "Access token is for a different AccessGuard than the one "
           "protecting this value";
  }

 private:
  std::atomic<const AccessGuard*> guard_{nullptr};
};

// Zero-size stand-in for GuardTracker when guard tracking is disabled.
class NoGuardTracker {
 public:
  NoGuardTracker() = default;
  explicit NoGuardTracker(const AccessGuard* /*guard*/) {}
  void Set(const AccessGuard* /*guard*/) {}
  void CheckMatches(const AccessGuard* /*expected*/) const {}
};

using DebugGuardTracker =
    std::conditional_t<kTrackGuards, GuardTracker, NoGuardTracker>;

}  // namespace free_threading_internal

// -----------------------------------------------------------------------------
// ReadAccess / WriteAccess
// -----------------------------------------------------------------------------
//
// Proof that the calling thread currently holds a shared read lock
// (`ReadAccess`) or an exclusive write lock (`WriteAccess`) on a particular
// `AccessGuard`. A `WriteAccess` is-a `ReadAccess`, since a write lock also
// grants read access.
//
// Tokens are created only by `AccessGuard::WithRead()`, `WithWrite()`,
// `WithWriteRead()`, and `WithReadRead()`, and are passed by const reference to
// the caller's callback. They are non-copyable and non-movable, so they cannot
// outlive the callback. Tokens do not own the lock; the scoped `With*()` call
// that created them releases it when the callback returns.
//
// Tokens carry no pointer to any guarded object. Guarded state is accessed by
// passing a token to `Guarded<T>::Get()` / `Set()` (see guarded.h).
class ReadAccess {
 public:
  ReadAccess(const ReadAccess&) = delete;
  ReadAccess& operator=(const ReadAccess&) = delete;

  // The guard this token proves is held. Used for debug-mode identity checks.
  const AccessGuard* guard() const { return guard_; }

 private:
  friend class AccessGuard;
  friend class WriteAccess;

  explicit ReadAccess(const AccessGuard* guard) : guard_(guard) {
    ABSL_DCHECK(guard != nullptr);
  }

  const AccessGuard* guard_;
};

class WriteAccess : public ReadAccess {
 private:
  friend class AccessGuard;

  explicit WriteAccess(const AccessGuard* guard) : ReadAccess(guard) {}
};

// -----------------------------------------------------------------------------
// AccessGuard
// -----------------------------------------------------------------------------
//
// `AccessGuard` maintains a single atomic word encoding both the exclusive
// write bit (bit 0) and the active reader count (bits 1..N):
//
//   * Uncontended read lock acquisition and release perform a single atomic
//     fetch_add followed by fetch_sub.
//   * Uncontended write lock acquisition and release perform a single atomic
//     compare_exchange_strong followed by fetch_and.
//   * On contention, acquisition fails (rolling back any state change) and
//     sets a Python RuntimeError ("Concurrent message access detected").
//
// Example:
//
//   struct MyPyWrapper {
//     PyObject_HEAD
//     AccessGuard guard;
//     Guarded<MyCppObject*> cpp_obj;  // Protected by guard.
//     Guarded<int> version;           // Protected by guard.
//   };
//
//   PyObject* GetValue(MyPyWrapper* self) {
//     return self->guard.WithRead([&](const ReadAccess& r) {
//       return PyLong_FromLong(self->cpp_obj.Get(r)->value());
//     });
//   }
//
//   PyObject* Bump(MyPyWrapper* self) {
//     return self->guard.WithWrite([&](const WriteAccess& w) {
//       self->cpp_obj.Get(w)->Mutate();
//       self->version.Set(self->version.Get(w) + 1, w);
//       Py_RETURN_NONE;
//     });
//   }
class AccessGuard {
 public:
  AccessGuard() = default;
  AccessGuard(const AccessGuard&) = delete;
  AccessGuard& operator=(const AccessGuard&) = delete;

  static void SetEnabledInTsanForTesting(bool enabled) {
#if defined(Py_GIL_DISABLED) && defined(ABSL_HAVE_THREAD_SANITIZER)
    enabled_in_tsan_.store(enabled, std::memory_order_relaxed);
#endif
  }

  static uintptr_t GetConcurrentAccessCountForTesting() {
    return concurrent_access_count_.load(std::memory_order_relaxed);
  }

  static void ResetConcurrentAccessCountForTesting() {
    concurrent_access_count_.store(0, std::memory_order_relaxed);
  }

  // AccessGuard::WithRead() / AccessGuard::WithWrite()
  //
  // Acquires a shared read (or exclusive write) lock on this guard, invokes
  // f(const ReadAccess&) (or f(const WriteAccess&)), releases the lock, and
  // returns f's result. If the lock cannot be acquired, a Python RuntimeError
  // is set, f is not invoked, and on_fail is returned (or on_fail() is called
  // and its result returned).
  //
  // The (f) overloads are only for callbacks returning void. They return true
  // if f was invoked, or false (with a Python exception set) if the lock could
  // not be acquired. Callbacks that return a value must always say explicitly
  // what to return on failure.
  template <typename OnFail, typename F>
  [[nodiscard]] auto WithRead(OnFail&& on_fail, F&& f);
  template <typename F>
  [[nodiscard]] bool WithRead(F&& f);

  template <typename OnFail, typename F>
  [[nodiscard]] auto WithWrite(OnFail&& on_fail, F&& f);
  template <typename F>
  [[nodiscard]] bool WithWrite(F&& f);

  // AccessGuard::WithWriteRead()
  //
  // Acquires a write lock on dst and a read lock on src, and invokes
  // f(const WriteAccess& dst_access, const ReadAccess& src_access).
  //
  //   * If &dst == &src, acquires a single write lock and passes the same
  //     WriteAccess as both arguments (a write lock implies read access).
  //   * Otherwise acquires both locks in canonical address order, rolling back
  //     the first if the second fails.
  template <typename OnFail, typename F>
  [[nodiscard]] static auto WithWriteRead(AccessGuard& dst, AccessGuard& src,
                                          OnFail&& on_fail, F&& f);
  template <typename F>
  [[nodiscard]] static bool WithWriteRead(AccessGuard& dst, AccessGuard& src,
                                          F&& f);

  // AccessGuard::WithReadRead()
  //
  // Acquires read locks on a and b (a single read lock if &a == &b) and invokes
  // f(const ReadAccess& a_access, const ReadAccess& b_access).
  template <typename OnFail, typename F>
  [[nodiscard]] static auto WithReadRead(AccessGuard& a, AccessGuard& b,
                                         OnFail&& on_fail, F&& f);
  template <typename F>
  [[nodiscard]] static bool WithReadRead(AccessGuard& a, AccessGuard& b, F&& f);

  // Records a concurrent access and raises a Python RuntimeError ("Concurrent
  // message access detected") on the current thread. For callers that detect
  // a concurrent modification through means other than lock acquisition (for
  // example, an object moving to a different guard while acquiring).
  static void ReportConcurrentAccess() {
    concurrent_access_count_.fetch_add(1, std::memory_order_relaxed);
    PyErr_SetString(PyExc_RuntimeError, "Concurrent message access detected");
  }

 private:
  struct ReadReleaser {
    AccessGuard* guard;
    ~ReadReleaser() { guard->ReleaseRead(); }
  };
  struct WriteReleaser {
    AccessGuard* guard;
    ~WriteReleaser() { guard->ReleaseWrite(); }
  };

  // Acquires a write lock on dst_guard and a read lock on src_guard, which must
  // be distinct, in canonical address order.
  ABSL_MUST_USE_RESULT static bool TryAcquireWriteReadGuards(
      AccessGuard& dst_guard, AccessGuard& src_guard);

#ifdef Py_GIL_DISABLED
  bool TryAcquireRead() {
#ifdef ABSL_HAVE_THREAD_SANITIZER
    if (!enabled_in_tsan_.load(std::memory_order_relaxed)) return true;
#endif
    uintptr_t prev =
        state_.fetch_add(kReadIncrement, std::memory_order_acquire);
    if (ABSL_PREDICT_FALSE((prev & kWriteBit) != 0)) {
      state_.fetch_sub(kReadIncrement, std::memory_order_relaxed);
      ReportConcurrentAccess();
      return false;
    }
    return true;
  }

  void ReleaseRead() {
#ifdef ABSL_HAVE_THREAD_SANITIZER
    if (!enabled_in_tsan_.load(std::memory_order_relaxed)) return;
#endif
    uintptr_t prev =
        state_.fetch_sub(kReadIncrement, std::memory_order_release);
    ABSL_DCHECK_GT(prev >> 1, 0u);
    // Because TryAcquireWrite() uses CAS, a writer never holds kWriteBit while
    // readers are active, so kWriteBit is never set here.
    ABSL_DCHECK_EQ(prev & kWriteBit, 0u);
  }

  bool TryAcquireWrite() {
#ifdef ABSL_HAVE_THREAD_SANITIZER
    if (!enabled_in_tsan_.load(std::memory_order_relaxed)) return true;
#endif
    uintptr_t expected = 0;
    if (ABSL_PREDICT_FALSE(!state_.compare_exchange_strong(
            expected, kWriteBit, std::memory_order_acquire,
            std::memory_order_relaxed))) {
      ReportConcurrentAccess();
      return false;
    }
    return true;
  }

  void ReleaseWrite() {
#ifdef ABSL_HAVE_THREAD_SANITIZER
    if (!enabled_in_tsan_.load(std::memory_order_relaxed)) return;
#endif
    uintptr_t prev = state_.fetch_and(~kWriteBit, std::memory_order_release);
    ABSL_DCHECK_EQ(prev & kWriteBit, kWriteBit);
    // We can't assert that the read increment is 0 since another thread may be
    // in the TryAcquireRead() failure path.
  }

  static constexpr uintptr_t kWriteBit = 1;
  static constexpr uintptr_t kReadIncrement = 2;
  std::atomic<uintptr_t> state_{0};
#ifdef ABSL_HAVE_THREAD_SANITIZER
  // Disabled by default under TSan so that AccessGuard's release/acquire atomic
  // operations do not create artificial happens-before edges that hide user
  // thread-safety bugs from TSan. Enabled explicitly in AccessGuard tests.
  static inline std::atomic<bool> enabled_in_tsan_{false};
#endif
#else
  bool TryAcquireRead() { return true; }
  void ReleaseRead() {}
  bool TryAcquireWrite() { return true; }
  void ReleaseWrite() {}
#endif  // Py_GIL_DISABLED

  static inline std::atomic<uintptr_t> concurrent_access_count_{0};
};

inline bool AccessGuard::TryAcquireWriteReadGuards(AccessGuard& dst_guard,
                                                   AccessGuard& src_guard) {
  ABSL_DCHECK_NE(&dst_guard, &src_guard);
  if (std::less<const AccessGuard*>()(&dst_guard, &src_guard)) {
    if (ABSL_PREDICT_TRUE(dst_guard.TryAcquireWrite())) {
      if (ABSL_PREDICT_TRUE(src_guard.TryAcquireRead())) {
        return true;
      }
      dst_guard.ReleaseWrite();
    }
    return false;
  } else {
    if (ABSL_PREDICT_TRUE(src_guard.TryAcquireRead())) {
      if (ABSL_PREDICT_TRUE(dst_guard.TryAcquireWrite())) {
        return true;
      }
      src_guard.ReleaseRead();
    }
    return false;
  }
}

template <typename OnFail, typename F>
auto AccessGuard::WithRead(OnFail&& on_fail, F&& f) {
  using RetT = std::invoke_result_t<F, const ReadAccess&>;
  if (ABSL_PREDICT_FALSE(!TryAcquireRead())) {
    return free_threading_internal::Fail<RetT>(std::forward<OnFail>(on_fail));
  }
  ReadReleaser release{this};
  const ReadAccess access(this);
  return std::forward<F>(f)(access);
}

template <typename F>
bool AccessGuard::WithRead(F&& f) {
  static_assert(std::is_void_v<std::invoke_result_t<F, const ReadAccess&>>,
                "Callbacks that return a value must pass an explicit on_fail.");
  return WithRead(false, [&](const ReadAccess& access) {
    f(access);
    return true;
  });
}

template <typename OnFail, typename F>
auto AccessGuard::WithWrite(OnFail&& on_fail, F&& f) {
  using RetT = std::invoke_result_t<F, const WriteAccess&>;
  if (ABSL_PREDICT_FALSE(!TryAcquireWrite())) {
    return free_threading_internal::Fail<RetT>(std::forward<OnFail>(on_fail));
  }
  WriteReleaser release{this};
  const WriteAccess access(this);
  return std::forward<F>(f)(access);
}

template <typename F>
bool AccessGuard::WithWrite(F&& f) {
  static_assert(std::is_void_v<std::invoke_result_t<F, const WriteAccess&>>,
                "Callbacks that return a value must pass an explicit on_fail.");
  return WithWrite(false, [&](const WriteAccess& access) {
    f(access);
    return true;
  });
}

template <typename OnFail, typename F>
auto AccessGuard::WithWriteRead(AccessGuard& dst, AccessGuard& src,
                                OnFail&& on_fail, F&& f) {
  using RetT = std::invoke_result_t<F, const WriteAccess&, const ReadAccess&>;
  if (&dst == &src) {
    return dst.WithWrite(
        std::forward<OnFail>(on_fail),
        [&](const WriteAccess& access) -> RetT { return f(access, access); });
  }
  if (ABSL_PREDICT_FALSE(!TryAcquireWriteReadGuards(dst, src))) {
    return free_threading_internal::Fail<RetT>(std::forward<OnFail>(on_fail));
  }
  WriteReleaser release_dst{&dst};
  ReadReleaser release_src{&src};
  const WriteAccess dst_access(&dst);
  const ReadAccess src_access(&src);
  return std::forward<F>(f)(dst_access, src_access);
}

template <typename F>
bool AccessGuard::WithWriteRead(AccessGuard& dst, AccessGuard& src, F&& f) {
  static_assert(
      std::is_void_v<
          std::invoke_result_t<F, const WriteAccess&, const ReadAccess&>>,
      "Callbacks that return a value must pass an explicit on_fail.");
  return WithWriteRead(
      dst, src, false,
      [&](const WriteAccess& dst_access, const ReadAccess& src_access) {
        f(dst_access, src_access);
        return true;
      });
}

template <typename OnFail, typename F>
auto AccessGuard::WithReadRead(AccessGuard& a, AccessGuard& b, OnFail&& on_fail,
                               F&& f) {
  using RetT = std::invoke_result_t<F, const ReadAccess&, const ReadAccess&>;
  if (&a == &b) {
    return a.WithRead(
        std::forward<OnFail>(on_fail),
        [&](const ReadAccess& access) -> RetT { return f(access, access); });
  }
  if (ABSL_PREDICT_FALSE(!a.TryAcquireRead())) {
    return free_threading_internal::Fail<RetT>(std::forward<OnFail>(on_fail));
  }
  ReadReleaser release_a{&a};
  if (ABSL_PREDICT_FALSE(!b.TryAcquireRead())) {
    return free_threading_internal::Fail<RetT>(std::forward<OnFail>(on_fail));
  }
  ReadReleaser release_b{&b};
  const ReadAccess a_access(&a);
  const ReadAccess b_access(&b);
  return std::forward<F>(f)(a_access, b_access);
}

template <typename F>
bool AccessGuard::WithReadRead(AccessGuard& a, AccessGuard& b, F&& f) {
  static_assert(
      std::is_void_v<
          std::invoke_result_t<F, const ReadAccess&, const ReadAccess&>>,
      "Callbacks that return a value must pass an explicit on_fail.");
  return WithReadRead(
      a, b, false, [&](const ReadAccess& a_access, const ReadAccess& b_access) {
        f(a_access, b_access);
        return true;
      });
}

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_ACCESS_GUARD_H__
