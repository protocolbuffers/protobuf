// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

// This file provides alignment utilities for use in arenas.
//
// `ArenaAlign` contains a single `align` data member and provides
// the below functions which operate on the given alignment.
//
//   Ceil(size_t n)      - rounds `n` up to the nearest `align` boundary.
//   Floor(size_t n)     - rounds `n` down to the nearest `align` boundary.
//   Ceil(T* P)          - rounds `p` up to the nearest `align` boundary.
//   IsAligned(size_t n) - returns true if `n` is aligned to `align`
//   IsAligned(T* p)     - returns true if `p` is aligned to `align`
//   CheckAligned(T* p)  - returns `p`. Checks alignment of `p` in debug.
//
// Additionally there is an optimized `CeilDefaultAligned(T*)` method which is
// equivalent to `Ceil(ArenaAlignDefault().CheckAlign(p))` but more efficiently
// implemented as a 'check only' for ArenaAlignDefault.
//
// These classes allow for generic arena logic using 'alignment policies'.
//
// For example:
//
//  template <Align>
//  void* NaiveAlloc(size_t n, Align align) {
//    align.CheckAligned(n);
//    uint8_t* ptr = align.CeilDefaultAligned(ptr_);
//    ptr_ += n;
//    return ptr;
//  }
//
//  void CallSites() {
//    void *p1 = NaiveAlloc(n, ArenaAlignDefault());
//    void *p2 = NaiveAlloc(n, ArenaAlignAs(32));
//  }
//
#ifndef GOOGLE_PROTOBUF_ARENA_ALIGN_H__
#define GOOGLE_PROTOBUF_ARENA_ALIGN_H__

#include <cstddef>
#include <cstdint>

#include "google/protobuf/stubs/logging.h"
#include "google/protobuf/stubs/common.h"
#include "absl/numeric/bits.h"

namespace google {
namespace protobuf {
namespace internal {

struct ArenaAlignDefault {
  static constexpr size_t align = 8;  // NOLINT

  static constexpr bool IsAligned(size_t n) { return (n & (align - 1)) == 0; }

  template <typename T>
  static bool IsAligned(T* ptr) {
    return (reinterpret_cast<uintptr_t>(ptr) & (align - 1)) == 0;
  }

  static constexpr size_t Ceil(size_t n) { return (n + align - 1) & -align; }
  static constexpr size_t Floor(size_t n) { return (n & ~(align - 1)); }

  template <typename T>
  T* Ceil(T* ptr) const {
    uintptr_t intptr = reinterpret_cast<uintptr_t>(ptr);
    return reinterpret_cast<T*>((intptr + align - 1) & -align);
  }

  template <typename T>
  T* CeilDefaultAligned(T* ptr) const {
    return ArenaAlignDefault().CheckAligned(ptr);
  }

  // Address sanitizer enabled alignment check
  template <typename T>
  static T* CheckAligned(T* ptr) {
    GOOGLE_DCHECK(IsAligned(ptr)) << static_cast<void*>(ptr);
    return ptr;
  }
};

struct ArenaAlign {
  static constexpr bool IsDefault() { return false; };

  size_t align;

  constexpr bool IsAligned(size_t n) const { return (n & (align - 1)) == 0; }

  template <typename T>
  bool IsAligned(T* ptr) const {
    return (reinterpret_cast<uintptr_t>(ptr) & (align - 1)) == 0;
  }

  constexpr size_t Ceil(size_t n) const { return (n + align - 1) & -align; }
  constexpr size_t Floor(size_t n) const { return (n & ~(align - 1)); }

  template <typename T>
  T* Ceil(T* ptr) const {
    uintptr_t intptr = reinterpret_cast<uintptr_t>(ptr);
    return reinterpret_cast<T*>((intptr + align - 1) & -align);
  }

  template <typename T>
  T* CeilDefaultAligned(T* ptr) const {
    return Ceil(ArenaAlignDefault().CheckAligned(ptr));
  }

  // Address sanitizer enabled alignment check
  template <typename T>
  T* CheckAligned(T* ptr) const {
    GOOGLE_DCHECK(IsAligned(ptr)) << static_cast<void*>(ptr);
    return ptr;
  }
};

inline ArenaAlign ArenaAlignAs(size_t align) {
  // align must be a non zero power of 2 >= 8
  GOOGLE_DCHECK_NE(align, 0);
  GOOGLE_DCHECK(absl::has_single_bit(align)) << "Invalid alignment " << align;
  return ArenaAlign{align};
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_ARENA_ALIGN_H__
