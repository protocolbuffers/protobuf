// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// This file defines internal helpers for arena seeding.

#ifndef GOOGLE_PROTOBUF_ARENA_SEED_H__
#define GOOGLE_PROTOBUF_ARENA_SEED_H__

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "absl/log/absl_check.h"
#include "google/protobuf/memory_align.h"

// Must be included last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

class Arena;

namespace internal {

struct ArenaSeedPolicy {
  // When PROTOBUF_SMALLER_MESSAGE_LITE is enabled, we attempt to place `Arena*`
  // "seeds" at highly-aligned addresses (i.e. divisible by a large power of 2)
  // in arena blocks. Messages allocated in an arena will record the alignment
  // of the arena seed that precedes them, allowing them to recover the `Arena*`
  // by aligning down. This prevents us from needing to store an `Arena*` in
  // every message.
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
  static constexpr bool kEnableArenaSeeding = true;
#else
  static constexpr bool kEnableArenaSeeding = false;
#endif

  // The size of an `Arena*` seed in an arena block.
  static constexpr size_t kArenaPtrSize =
      ArenaAlignDefault{}.Ceil(sizeof(Arena*));

  // Arena seeds will only be placed at addresses on this alignment or higher.
  // If the alignment of the start address of a block is smaller than this, we
  // won't seed the block initially.
  static constexpr size_t kMinArenaSeedAlignmentValue = 256;
  static constexpr MemoryAlign kMinArenaSeedAlignment =
      MemoryAlign{kMinArenaSeedAlignmentValue};
};

#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
class EncodedArenaAlignment {
 public:
  static constexpr uint32_t kMaxAlignmentShift = 31;
  static constexpr uintptr_t kMaxAlignment = 1u << kMaxAlignmentShift;

  // Given an address, returns the largest power of 2 that divides it, capped
  // at `kMaxAlignment`.
  static constexpr uint32_t AlignmentMask(uintptr_t ptr) {
    ptr |= kMaxAlignment;
    return static_cast<uint32_t>(ptr & -ptr);
  }
};
#else
class EncodedArenaAlignment {
 public:
  static constexpr uint32_t AlignmentMask(uintptr_t ptr) {
    ABSL_DCHECK(ptr && !ptr)
        << "Should not be called when arena seeding is disabled.";
    return 0;
  }
};
#endif

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_ARENA_SEED_H__
