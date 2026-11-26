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

#include <cstddef>
#include <cstdint>
#include <new>

#include "absl/log/absl_check.h"
#include "absl/numeric/bits.h"
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

  // The size of an `Arena*` seed in an arena block, if seeding is enabled.
  // Otherwise 0.
  static constexpr size_t kArenaPtrSize =
      kEnableArenaSeeding * ArenaAlignDefault{}.Ceil(sizeof(Arena*));

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

  static constexpr EncodedArenaAlignment NoArena() {
    return EncodedArenaAlignment(NoArenaTag{});
  }

  static constexpr EncodedArenaAlignment ArenaPrefix() {
    return EncodedArenaAlignment(ArenaAlignDefault{}.align());
  }

  static constexpr EncodedArenaAlignment FromAlignment(
      std::align_val_t alignment) {
    const uintptr_t align_val = static_cast<uintptr_t>(alignment);
    ABSL_DCHECK(absl::has_single_bit(align_val));
    return EncodedArenaAlignment(align_val);
  }

  bool has_arena() const { return align_mask_ != 0; }

  bool is_unseeded() const {
    return align_mask_ == ArenaPrefix().encoded_alignment();
  }

  uintptr_t encoded_alignment() const {
    return static_cast<uintptr_t>(align_mask_);
  }

  uintptr_t alignment() const {
    ABSL_DCHECK(has_arena());
    return 1u << align_mask_;
  }

  Arena* FindArena(const void* start_ptr) const {
    ABSL_DCHECK(has_arena());
    const void* ptr = static_cast<const char*>(start_ptr) - 1;
    const void* arena_ptr_ptr = MemoryAlign{alignment()}.Floor(ptr);
    return *reinterpret_cast<Arena* const*>(arena_ptr_ptr);
  }

 private:
  static constexpr uint8_t CountTrailingZeros(size_t n) {
    // `absl::countr_zero` checks for `n == 0` and returns
    // `std::numeric_limits<size_t>::digits` in that case. We don't want to pay
    // for this branch, we know `n` is at least `ArenaAlignDefault::align`.
    PROTOBUF_ASSUME(n != 0);
    return static_cast<uint8_t>(absl::countr_zero(n));
  }

  explicit constexpr EncodedArenaAlignment(size_t align)
      : align_mask_(CountTrailingZeros(align)) {
    // `align` can't be 1, since `align_mask_ == 0` is a special value meaning
    // no arena.
    ABSL_DCHECK_GE(align, ArenaAlignDefault{}.align());
  }

  struct NoArenaTag {};
  explicit constexpr EncodedArenaAlignment(NoArenaTag) : align_mask_(0) {}

  uint8_t align_mask_;
};
#else
class EncodedArenaAlignment {
 public:
  static constexpr EncodedArenaAlignment NoArena() { return {}; }

  static constexpr uint32_t AlignmentMask(uintptr_t ptr) {
    ABSL_DCHECK(ptr && !ptr)
        << "Should not be called when arena seeding is disabled.";
    return 0;
  }

 private:
  constexpr EncodedArenaAlignment() = default;
};
#endif

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_ARENA_SEED_H__
