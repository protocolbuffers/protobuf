// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mem/debug/ref.h"

#ifdef __cplusplus

#include <utility>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_log.h"
#include "absl/synchronization/mutex.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef UPB_ARENA_DEBUG

class upb_Debug_Context {
  using Key = std::pair<const void*, const void*>;

 public:
  void IncRef(const void* arena, const void* owner) {
    const Key key = std::make_pair(arena, owner);
    absl::MutexLock lock(&mutex_);

    if (keys_.contains(key)) {
      ABSL_LOG(FATAL) << "arena owner exists";
    }
    keys_.insert(key);
  }

  void DecRef(const void* arena, const void* owner) {
    const Key key = std::make_pair(arena, owner);
    absl::MutexLock lock(&mutex_);

    if (keys_.erase(key) != 1) {
      ABSL_LOG(FATAL) << "arena owner does not exist";
    }
  }

 private:
  absl::Mutex mutex_;
  absl::flat_hash_set<Key> keys_ ABSL_GUARDED_BY(mutex_);
};

static upb_Debug_Context* global_context = new upb_Debug_Context();

extern "C" {

void upb_Debug_IncRef(const void* arena, const void* owner) {
  global_context->IncRef(arena, owner);
}

void upb_Debug_DecRef(const void* arena, const void* owner) {
  global_context->DecRef(arena, owner);
}

} /* extern "C" */

#include "upb/port/undef.inc"

#endif  // __cplusplus

#endif  // UPB_ARENA_DEBUG
