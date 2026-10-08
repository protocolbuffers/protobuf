// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// -----------------------------------------------------------------------------
// File: multi_lock.h
// -----------------------------------------------------------------------------
//
// This header file defines the dual-lock helpers `LockWriteRead()` and
// `LockReadRead()` for operations that simultaneously access two guarded
// objects (such as MergeFrom, CopyFrom, RichCompare, and
// RepeatedScalarContainer::Extend).

#ifndef GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_MULTI_LOCK_H__
#define GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_MULTI_LOCK_H__

#include <functional>
#include <optional>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/base/optimization.h"
#include "google/protobuf/pyext/free_threading/access_guard.h"
#include "google/protobuf/pyext/free_threading/guarded_ptr.h"
#include "google/protobuf/pyext/free_threading/locked_ptr.h"

namespace google {
namespace protobuf {
namespace python {

// Internal helper used by `LockWriteRead()` and `MessageTree` two-phase
// locking. Acquires an exclusive write lock on dst_guard and a shared read
// lock on src_guard (either atomically if &dst_guard == &src_guard, or in
// canonical pointer order if distinct).
inline std::pair<AccessGuard::WriteLock, AccessGuard::ReadLock>
AccessGuard::TryAcquireWriteReadGuards(AccessGuard& dst_guard,
                                       AccessGuard& src_guard) {
  if (&dst_guard == &src_guard) {
    if (ABSL_PREDICT_FALSE(!dst_guard.TryAcquireWriteAndRead())) {
      return {};
    }
    return {AccessGuard::WriteLock(&dst_guard),
            AccessGuard::ReadLock(&src_guard)};
  }
  if (std::less<const AccessGuard*>()(&dst_guard, &src_guard)) {
    AccessGuard::WriteLock dst_lock(dst_guard);
    if (ABSL_PREDICT_FALSE(!dst_lock)) return {};
    AccessGuard::ReadLock src_lock(src_guard);
    if (ABSL_PREDICT_FALSE(!src_lock)) return {};
    return {std::move(dst_lock), std::move(src_lock)};
  } else {
    AccessGuard::ReadLock src_lock(src_guard);
    if (ABSL_PREDICT_FALSE(!src_lock)) return {};
    AccessGuard::WriteLock dst_lock(dst_guard);
    if (ABSL_PREDICT_FALSE(!dst_lock)) return {};
    return {std::move(dst_lock), std::move(src_lock)};
  }
}

// -----------------------------------------------------------------------------
// LockWriteRead()
// -----------------------------------------------------------------------------
//
// Acquires an exclusive write lock on dst and a shared read lock on src
// across potentially distinct trees (or the same tree), returning a pair of
// RAII smart pointers (`LockedPtr<DstT>` and `LockedPtr<const SrcT>`).
//
//   * If &dst_guard == &src_guard (same tree, e.g. msg.MergeFrom(msg) or
//     copying between two submessages of the same root message), atomically
//     acquires both a write lock and a read lock on dst_guard so that both
//     returned `LockedPtr` instances independently release their lock on
//     destruction.
//   * If &dst_guard != &src_guard (different trees), acquires both locks in
//     canonical pointer address order (`std::less<const AccessGuard*>()`) and
//     rolls back if the second lock fails.
//   * On contention, sets a Python RuntimeError and returns `std::nullopt`.
//
// Example:
//
//   PyObject* CopyData(MyPyWrapper* self, MyPyWrapper* other) {
//     PYPROTO_ASSIGN_OR_RETURN(
//         (auto [dst_obj, src_obj]),
//         LockWriteRead(self->cpp_obj, self->guard,
//                       other->cpp_obj, other->guard),
//         nullptr);
//     dst_obj->CopyFrom(*src_obj);
//     Py_RETURN_NONE;
//   }
template <typename DstT, typename SrcT>
ABSL_MUST_USE_RESULT
    std::optional<std::pair<LockedPtr<DstT>, LockedPtr<const SrcT>>>
    LockWriteRead(const GuardedPtr<DstT>& dst, AccessGuard& dst_guard,
                  const GuardedPtr<SrcT>& src, AccessGuard& src_guard) {
  auto [dst_lock, src_lock] =
      AccessGuard::TryAcquireWriteReadGuards(dst_guard, src_guard);
  if (ABSL_PREDICT_FALSE(!dst_lock || !src_lock)) {
    return std::nullopt;
  }
  return std::make_pair(dst.AdoptWriteLock(std::move(dst_lock)),
                        src.AdoptReadLock(std::move(src_lock)));
}

// -----------------------------------------------------------------------------
// LockReadRead()
// -----------------------------------------------------------------------------
//
// Acquires shared read locks on both first and second (which may belong to
// the same tree or different trees), returning a pair of RAII smart pointers
// (`LockedPtr<const FirstT>` and `LockedPtr<const SecondT>`).
//
// Because shared read locks commute with other shared read locks (including on
// the same `AccessGuard`), no pointer-order sorting is required. If acquiring
// the second read lock fails due to a concurrent writer on second_guard, the
// first read lock is automatically released before returning `std::nullopt`.
//
// Example:
//
//   PyObject* CompareEquals(MyPyWrapper* a, MyPyWrapper* b) {
//     PYPROTO_ASSIGN_OR_RETURN(
//         (auto [obj_a, obj_b]),
//         LockReadRead(a->cpp_obj, a->guard,
//                      b->cpp_obj, b->guard),
//         nullptr);
//     return PyBool_FromLong(*obj_a == *obj_b);
//   }
template <typename FirstT, typename SecondT>
ABSL_MUST_USE_RESULT
    std::optional<std::pair<LockedPtr<const FirstT>, LockedPtr<const SecondT>>>
    LockReadRead(const GuardedPtr<FirstT>& first, AccessGuard& first_guard,
                 const GuardedPtr<SecondT>& second, AccessGuard& second_guard) {
  PYPROTO_ASSIGN_OR_RETURN(LockedPtr<const FirstT> first_lock,
                           first.LockRead(first_guard), std::nullopt);
  PYPROTO_ASSIGN_OR_RETURN(LockedPtr<const SecondT> second_lock,
                           second.LockRead(second_guard), std::nullopt);
  return std::make_pair(std::move(first_lock), std::move(second_lock));
}

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_MULTI_LOCK_H__
