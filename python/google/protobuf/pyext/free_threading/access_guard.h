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
// shared hierarchy object such as `MessageTree`) and pass it to `GuardedPtr<T>`
// (`LockRead()` and `LockWrite()` in guarded_ptr.h, or `LockWriteRead()` and
// `LockReadRead()` in multi_lock.h). Users should generally not call methods on
// `AccessGuard` directly.
//
// The relationship between `AccessGuard` and `GuardedPtr<T>` can be 1:1 or
// 1:many:
//
//   * 1:1 - A standalone Python wrapper owns one `AccessGuard` and one
//     `GuardedPtr<T>`.
//   * 1:many - A single `AccessGuard` protects multiple `GuardedPtr<T>`
//     instances, either multiple guarded fields on the same struct or
//     `GuardedPtr<T>` fields across many Python wrapper objects in the same
//     message tree (e.g., a root CMessage and all of its submessage CMessages
//     sharing a single `MessageTree`). Once a `LockedPtr` is acquired from any
//     `GuardedPtr` in the group, it can be passed to `Get()` or `GetMutable()`
//     to access any other `GuardedPtr` protected by that same `AccessGuard`
//     with zero additional atomic operations.

#ifndef GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_ACCESS_GUARD_H__
#define GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_ACCESS_GUARD_H__

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <atomic>
#include <cstdint>
#include <optional>
#include <utility>

#include "absl/base/attributes.h"

namespace google {
namespace protobuf {
namespace python {

template <typename T>
class LockedPtr;

template <typename T>
class GuardedPtr;

// -----------------------------------------------------------------------------
// AccessGuard
// -----------------------------------------------------------------------------
//
// `AccessGuard` maintains a single atomic word encoding both the exclusive
// write bit (bit 0) and the active reader count (bits 1..N):
//
//   * Uncontended read lock acquisition and release perform a single atomic
//     fetch_add / fetch_sub.
//   * Uncontended write lock acquisition and release perform a single atomic
//     compare_exchange_strong / fetch_and.
//   * On contention, acquisition fails (rolling back any state change) and
//     sets a Python RuntimeError ("Concurrent message access detected").
//
// A single `AccessGuard` may protect one `GuardedPtr<T>` (1:1) or many
// `GuardedPtr<T>` instances (1:many) that must be synchronized together (see
// guarded_ptr.h and locked_ptr.h).
//
// Example 1 (1:1 - a single `AccessGuard` protecting one `GuardedPtr<T>`):
//
//   struct MyPyWrapper {
//     PyObject_HEAD
//     AccessGuard guard;
//     GuardedPtr<MyCppObject> cpp_obj;
//   };
//
//   // During single-threaded initialization of self:
//   new (&self->guard) AccessGuard();
//   self->cpp_obj = GuardedPtr<MyCppObject>(raw_cpp_obj, self->guard);
//
//   // Reading in a Python C-API method:
//   PyObject* GetValue(MyPyWrapper* self) {
//     PYPROTO_ASSIGN_OR_RETURN(LockedPtr<const MyCppObject> obj,
//                              self->cpp_obj.LockRead(self->guard), nullptr);
//     return PyLong_FromLong(obj->value());
//   }
//
// Example 2 (1:many - a single `AccessGuard` shared across a tree of objects):
//
//   struct MessageTree {
//     AccessGuard guard;
//   };
//
//   struct CMessage {
//     PyObject_HEAD
//     MessageTree* tree;            // Shared by root and all submessages.
//     GuardedPtr<Message> message;  // Protected by tree->guard.
//   };
//
//   // Both parent->message and child->message are bound to the same
//   // tree->guard:
//   parent->message = GuardedPtr<Message>(raw_parent, tree->guard);
//   child->message = GuardedPtr<Message>(raw_child, tree->guard);
//
//   // Locking parent->message acquires tree->guard for the entire tree,
//   // allowing child->message to be unpacked via GetMutable() with zero
//   // additional atomic operations:
//   PyObject* ClearChildViaParent(CMessage* parent, CMessage* child) {
//     PYPROTO_ASSIGN_OR_RETURN(LockedPtr<Message> parent_msg,
//                              parent->message.LockWrite(parent->tree->guard),
//                              nullptr);
//     Message* child_msg = child->message.GetMutable(parent_msg);
//     child_msg->Clear();
//     Py_RETURN_NONE;
//   }
class AccessGuard {
 public:
  AccessGuard() = default;
  AccessGuard(const AccessGuard&) = delete;
  AccessGuard& operator=(const AccessGuard&) = delete;

 private:
  class ReadLock;
  class WriteLock;

  // Internal move-only RAII token holding a shared read lock on an
  // `AccessGuard`. Evaluates to true if the lock was acquired, or false if
  // contention was detected (in which case a Python RuntimeError has been set).
  class ABSL_MUST_USE_RESULT ReadLock {
   public:
    ReadLock() = default;
    explicit ReadLock(AccessGuard& guard)
        : guard_(guard.TryAcquireRead() ? &guard : nullptr) {}

    ~ReadLock() { Reset(); }

    ReadLock(ReadLock&& other) noexcept : guard_(other.guard_) {
      other.guard_ = nullptr;
    }

    ReadLock& operator=(ReadLock&& other) noexcept {
      if (this != &other) {
        Reset();
        guard_ = other.guard_;
        other.guard_ = nullptr;
      }
      return *this;
    }

    ReadLock(const ReadLock&) = delete;
    ReadLock& operator=(const ReadLock&) = delete;

    void Reset() {
      if (guard_ != nullptr) {
        guard_->ReleaseRead();
        guard_ = nullptr;
      }
    }

    explicit operator bool() const { return guard_ != nullptr; }
    const AccessGuard* guard() const { return guard_; }

   private:
    friend class AccessGuard;

    explicit ReadLock(AccessGuard* adopted_guard) : guard_(adopted_guard) {}

    AccessGuard* guard_ = nullptr;
  };

  // Internal move-only RAII token holding an exclusive write lock on an
  // `AccessGuard`. Evaluates to true if the lock was acquired, or false if
  // contention was detected (in which case a Python RuntimeError has been set).
  class ABSL_MUST_USE_RESULT WriteLock {
   public:
    WriteLock() = default;
    explicit WriteLock(AccessGuard& guard)
        : guard_(guard.TryAcquireWrite() ? &guard : nullptr) {}

    ~WriteLock() { Reset(); }

    WriteLock(WriteLock&& other) noexcept : guard_(other.guard_) {
      other.guard_ = nullptr;
    }

    WriteLock& operator=(WriteLock&& other) noexcept {
      if (this != &other) {
        Reset();
        guard_ = other.guard_;
        other.guard_ = nullptr;
      }
      return *this;
    }

    WriteLock(const WriteLock&) = delete;
    WriteLock& operator=(const WriteLock&) = delete;

    void Reset() {
      if (guard_ != nullptr) {
        guard_->ReleaseWrite();
        guard_ = nullptr;
      }
    }

    explicit operator bool() const { return guard_ != nullptr; }
    const AccessGuard* guard() const { return guard_; }

   private:
    friend class AccessGuard;

    explicit WriteLock(AccessGuard* adopted_guard) : guard_(adopted_guard) {}

    AccessGuard* guard_ = nullptr;
  };

 public:
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

 private:
  template <typename T>
  friend class LockedPtr;
  template <typename T>
  friend class GuardedPtr;
  friend struct CMessage;
  friend struct ContainerBase;
  template <typename DstT, typename SrcT>
  friend std::optional<std::pair<LockedPtr<DstT>, LockedPtr<const SrcT>>>
  LockWriteRead(const GuardedPtr<DstT>& dst, AccessGuard& dst_guard,
                const GuardedPtr<SrcT>& src, AccessGuard& src_guard);

  ABSL_MUST_USE_RESULT ReadLock LockRead() { return ReadLock(*this); }
  ABSL_MUST_USE_RESULT WriteLock LockWrite() { return WriteLock(*this); }

  ABSL_MUST_USE_RESULT static std::pair<WriteLock, ReadLock>
  TryAcquireWriteReadGuards(AccessGuard& dst_guard, AccessGuard& src_guard);

#ifdef Py_GIL_DISABLED
  // Increments the process-wide concurrent access counter and raises a Python
  // RuntimeError ("Concurrent message access detected") on the current thread.
  static void SetConcurrentModificationException() {
    concurrent_access_count_.fetch_add(1, std::memory_order_relaxed);
    PyErr_SetString(PyExc_RuntimeError, "Concurrent message access detected");
  }
  bool TryAcquireRead() {
#ifdef ABSL_HAVE_THREAD_SANITIZER
    if (!enabled_in_tsan_.load(std::memory_order_relaxed)) return true;
#endif
    uintptr_t prev =
        state_.fetch_add(kReadIncrement, std::memory_order_acquire);
    if (ABSL_PREDICT_FALSE((prev & kWriteBit) != 0)) {
      state_.fetch_sub(kReadIncrement, std::memory_order_relaxed);
      SetConcurrentModificationException();
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
    // Because TryAcquireWrite() uses CAS, a contending writer never sets
    // kWriteBit while readers are active. The only case where kWriteBit is set
    // here is same-tree LockWriteRead() (via TryAcquireWriteAndRead()), which
    // acquires both WriteLock and ReadLock on the same guard and destroys the
    // ReadLock first.
  }

  bool TryAcquireWrite() {
#ifdef ABSL_HAVE_THREAD_SANITIZER
    if (!enabled_in_tsan_.load(std::memory_order_relaxed)) return true;
#endif
    uintptr_t expected = 0;
    if (ABSL_PREDICT_FALSE(!state_.compare_exchange_strong(
            expected, kWriteBit, std::memory_order_acquire,
            std::memory_order_relaxed))) {
      SetConcurrentModificationException();
      return false;
    }
    return true;
  }

  // Atomically acquires BOTH an exclusive write lock and one shared read lock
  // on this guard if and only if the guard is currently completely unlocked
  // (`state_ == 0`).
  bool TryAcquireWriteAndRead() {
#ifdef ABSL_HAVE_THREAD_SANITIZER
    if (!enabled_in_tsan_.load(std::memory_order_relaxed)) return true;
#endif
    uintptr_t expected = 0;
    if (ABSL_PREDICT_FALSE(!state_.compare_exchange_strong(
            expected, kWriteBit | kReadIncrement, std::memory_order_acquire,
            std::memory_order_relaxed))) {
      SetConcurrentModificationException();
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
    // in the TryAcquireRead() failure path, or a same-tree read lock from
    // TryAcquireWriteAndRead() may still be held.
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
  bool TryAcquireWriteAndRead() { return true; }
  void ReleaseWrite() {}
#endif  // Py_GIL_DISABLED

  static inline std::atomic<uintptr_t> concurrent_access_count_{0};
};

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_ACCESS_GUARD_H__
