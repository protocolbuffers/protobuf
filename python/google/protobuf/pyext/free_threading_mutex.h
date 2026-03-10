// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_PYTHON_CPP_FREE_THREADING_MUTEX_H__
#define GOOGLE_PROTOBUF_PYTHON_CPP_FREE_THREADING_MUTEX_H__

#include "absl/base/attributes.h"
#include "absl/base/const_init.h"
#include "absl/base/thread_annotations.h"
#ifdef Py_GIL_DISABLED
// Only include mutex for free-threaded builds
#include "absl/synchronization/mutex.h"
#endif

namespace google {
namespace protobuf {
namespace python {

// Zero-cost mutex wrapper that compiles away to nothing in GIL-enabled builds.
// Similar to nanobind's ft_mutex pattern.
// NOTE: Protobuf Free-threading support is still experimental.
class ABSL_LOCKABLE ABSL_ATTRIBUTE_WARN_UNUSED FreeThreadingMutex {
 public:
  FreeThreadingMutex() = default;
  explicit constexpr FreeThreadingMutex(absl::ConstInitType)
#ifdef Py_GIL_DISABLED
      : mutex_(absl::kConstInit)
#endif
  {
  }
  FreeThreadingMutex(const FreeThreadingMutex&) = delete;
  FreeThreadingMutex& operator=(const FreeThreadingMutex&) = delete;

#ifndef Py_GIL_DISABLED
  // GIL-enabled build: no-op mutex (zero cost)
  void Lock() {}
  void Unlock() {}
#else
  // Free-threaded build: real mutex
  void Lock() ABSL_EXCLUSIVE_LOCK_FUNCTION() { mutex_.Lock(); }
  void Unlock() ABSL_UNLOCK_FUNCTION() { mutex_.Unlock(); }

 private:
  absl::Mutex mutex_;
#endif
};

// RAII lock guard for FreeThreadingMutex
class ABSL_SCOPED_LOCKABLE FreeThreadingLockGuard {
 public:
  explicit FreeThreadingLockGuard(FreeThreadingMutex& mutex)
      ABSL_EXCLUSIVE_LOCK_FUNCTION(mutex)
      : mutex_(mutex) {
    mutex_.Lock();
  }
  ~FreeThreadingLockGuard() ABSL_UNLOCK_FUNCTION() { mutex_.Unlock(); }

  FreeThreadingLockGuard(const FreeThreadingLockGuard&) = delete;
  FreeThreadingLockGuard& operator=(const FreeThreadingLockGuard&) = delete;

 private:
  FreeThreadingMutex& mutex_;
};

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_FREE_THREADING_MUTEX_H__
