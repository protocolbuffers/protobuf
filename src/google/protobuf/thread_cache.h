// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef GOOGLE_PROTOBUF_THREAD_CACHE_H__
#define GOOGLE_PROTOBUF_THREAD_CACHE_H__

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "absl/base/optimization.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

#ifdef __cpp_aligned_new
enum { kThreadCacheAlignment = ABSL_CACHELINE_SIZE };
#else
enum { kThreadCacheAlignment = alignof(max_align_t) };  // do the best we can
#endif

template <typename T>
class alignas(kThreadCacheAlignment) ThreadCache {
 public:
  inline int64_t thread_id() const { return thread_id_; }
  inline T* value() const { return value_; }

  inline void Set(int64_t thread_id, T* value) {
    thread_id_ = thread_id;
    value_ = value;
  }

  inline int64_t GetUniqueId() {
    static constexpr size_t kPerThreadIds = 256;
    if ((next_id_ & (kPerThreadIds - 1)) == 0) {
      static std::atomic<int64_t> global_id{0};
      next_id_ = global_id.fetch_add(kPerThreadIds, std::memory_order_relaxed);
    }
    return ++next_id_;
  }

 private:
  int64_t thread_id_ = -1;
  T* value_ = nullptr;
  int64_t next_id_ = 0;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_THREAD_CACHE_H__
