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

#ifdef Py_GIL_DISABLED
using FreeThreadingMutex = absl::Mutex;
using FreeThreadingLockGuard = absl::MutexLock;
#define FREE_THREADING_PT_GUARDED_BY(mutex) ABSL_PT_GUARDED_BY(mutex)
#else

class FreeThreadingMutex {
 public:
  FreeThreadingMutex() = default;
  explicit constexpr FreeThreadingMutex(absl::ConstInitType) {}
};

class FreeThreadingLockGuard {
 public:
  explicit FreeThreadingLockGuard(FreeThreadingMutex& mutex) {}
};

#define FREE_THREADING_PT_GUARDED_BY(mutex)

#endif

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_FREE_THREADING_MUTEX_H__
