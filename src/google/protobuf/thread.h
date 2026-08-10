// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_THREAD_H__
#define GOOGLE_PROTOBUF_THREAD_H__

#include <cstddef>

#include "absl/functional/function_ref.h"
#include "absl/types/optional.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

struct StackInfo {
  void* base_ptr;
  size_t size;
};

// If supported, returns the StackInfo for the current thread.
// Otherwise, absl::nullopt.
PROTOBUF_EXPORT absl::optional<StackInfo> GetCurrentStackInfo();

// If supported, returns an estimation of the remaining stack in the current
// thread.
// Otherwise, absl::nullopt.
PROTOBUF_EXPORT absl::optional<size_t> GetEstimatedThreadStackRemaining();

inline bool CanSpawnNewThreads() {
#if defined(__wasm__) && !defined(__EMSCRIPTEN_PTHREADS__)
  // Wasm does not support threads by default.
  return false;
#endif
  return true;
}

// Runs the unit of in a separate thread, blocking until the work is done.
PROTOBUF_EXPORT void RunSyncInSeparateThread(absl::FunctionRef<void()> work);

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
#endif  // GOOGLE_PROTOBUF_THREAD_H__
