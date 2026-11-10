// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// `MemoryAlign` contains a single `align` data member and provides
// the below functions which operate on the given alignment.
//
//   Ceil(size_t n)      - rounds `n` up to the nearest `align` boundary.
//   Floor(size_t n)     - rounds `n` down to the nearest `align` boundary.
//   Padded(size_t n)    - returns the unaligned size to align 'n' bytes. (1)

//   Ceil(T* P)          - rounds `p` up to the nearest `align` boundary. (2)
//   IsAligned(size_t n) - returns true if `n` is aligned to `align`
//   IsAligned(T* p)     - returns true if `p` is aligned to `align`
//   CheckAligned(T* p)  - returns `p`. Checks alignment of `p` in debug.
//
// 1) `Padded(n)` returns the minimum size needed to align an object of size 'n'
//    into a memory area that is default aligned. For example, allocating 'n'
//    bytes aligned at 32 bytes requires a size of 'n + 32 - 8' to align at 32
//    bytes for any 8 byte boundary.
//
// 2) There is an optimized `CeilDefaultAligned(T*)` method which is equivalent
//    to `Ceil(ArenaAlignDefault{}.CheckAlign(p))` but more efficiently
//    implemented as a 'check only' for MemoryAlign.
//
// These classes allow for generic arena logic using 'alignment policies'.
//
// For example:
//
//  template <Align>
//  void* NaiveAlloc(size_t n, Align align) {
//    ABSL_DCHECK(align.IsAligned(n));
//    const size_t required = align.Padded(n);
//    if (required <= static_cast<size_t>(ptr_ - limit_)) {
//      uint8_t* ptr = align.CeilDefaultAligned(ptr_);
//      ptr_ = ptr + n;
//      return ptr;
//    }
//    return nullptr;
//  }
//
//  void CallSites() {
//    void *p1 = NaiveAlloc(n, ArenaAlignDefault());
//    void *p2 = NaiveAlloc(n, ArenaAlignAs(32));
//  }
//
#ifndef GOOGLE_PROTOBUF_MEMORY_ALIGN_H__
#define GOOGLE_PROTOBUF_MEMORY_ALIGN_H__

#include <cstddef>
#include <cstdint>

#include "absl/log/absl_check.h"
#include "absl/numeric/bits.h"

namespace google {
namespace protobuf {
namespace internal {

struct MemoryAlign {
  static constexpr size_t kMinArenaAlignment = 8;

  size_t align;

  constexpr explicit MemoryAlign(size_t align) : align(align) {
    ABSL_DCHECK(absl::has_single_bit(align))
        << "Alignment not a power of 2: " << align;
  }

  template <typename T>
  static constexpr MemoryAlign Of() {
    return MemoryAlign{alignof(T)};
  }

  constexpr bool IsAligned(size_t n) const { return (n & (align - 1)) == 0U; }

  template <typename T>
  bool IsAligned(T* ptr) const {
    return (reinterpret_cast<uintptr_t>(ptr) & (align - 1)) == 0U;
  }

  constexpr size_t Ceil(size_t n) const {
    return (n + align - 1) & ~(align - 1);
  }
  constexpr size_t Floor(size_t n) const { return (n & ~(align - 1)); }

  constexpr size_t Padded(size_t n) const;

  template <typename T>
  T* Ceil(T* ptr) const {
    uintptr_t intptr = reinterpret_cast<uintptr_t>(ptr);
    return reinterpret_cast<T*>((intptr + align - 1) & ~(align - 1));
  }

  template <typename T>
  T* CeilDefaultAligned(T* ptr) const;

  // Address sanitizer enabled alignment check
  template <typename T>
  T* CheckAligned(T* ptr) const {
    ABSL_DCHECK(IsAligned(ptr));
    return ptr;
  }
};

struct ArenaAlignDefault : MemoryAlign {
  static constexpr size_t align = 8;  // NOLINT

  constexpr ArenaAlignDefault() : MemoryAlign{align} {}
};

constexpr MemoryAlign ArenaAlignAs(size_t align) {
  ABSL_DCHECK_GE(align, MemoryAlign::kMinArenaAlignment)
      << "Alignment less than the minimum arena alignment";
  return MemoryAlign{align};
}

constexpr size_t MemoryAlign::Padded(size_t n) const {
  ABSL_DCHECK(ArenaAlignDefault{}.IsAligned(align))
      << "`Padded` is only meant to be used for arena allocations with an "
         "alignment >= `kMinArenaAlignment`.";
  ABSL_DCHECK(ArenaAlignDefault{}.IsAligned(n));
  return n + align - ArenaAlignDefault{}.align;
}

template <typename T>
inline T* MemoryAlign::CeilDefaultAligned(T* ptr) const {
  ABSL_DCHECK(ArenaAlignDefault{}.IsAligned(ptr));
  return Ceil(ptr);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_MEMORY_ALIGN_H__
