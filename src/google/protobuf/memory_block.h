// Protocol Buffers - Google's data interchange format
// Copyright 2021 Google Inc.  All rights reserved.
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

#ifndef GOOGLE_PROTOBUF_MEMORY_BLOCK_H__
#define GOOGLE_PROTOBUF_MEMORY_BLOCK_H__

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "google/protobuf/stubs/logging.h"
#include "google/protobuf/stubs/common.h"
#include "google/protobuf/arena_align.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

#ifdef __cpp_lib_byte
using Byte = std::byte;
#else
enum class Byte : uint8_t { one };
#endif

using Ptr = Byte*;

class alignas(ArenaAlignDefault::align) MemoryBlock {
 public:
  MemoryBlock(void* mem, size_t size, MemoryBlock* next = sentinel());

  static constexpr MemoryBlock* sentinel();

  constexpr Ptr head() const;
  constexpr Ptr limit() const;
  constexpr Ptr tail() const;
  constexpr MemoryBlock* next() const;

  constexpr size_t size() const;
  constexpr size_t allocated_size() const;

  void set_limit(Ptr limit);

 private:
  size_t available(Ptr ptr) const { return ptr - head(); }

  constexpr MemoryBlock();

  struct Sentinel;

  union {
    Byte raw_[sizeof(void*) * 3];
    struct {
      MemoryBlock* const next_;
      Ptr limit_;
      Ptr const tail_;
    };
  };
};

inline constexpr MemoryBlock::MemoryBlock()
    : next_(this), limit_(raw_ + sizeof(raw_)), tail_(raw_ + sizeof(raw_)) {}

inline MemoryBlock::MemoryBlock(void* mem, size_t size, MemoryBlock* next)
    : next_(next),
      limit_(static_cast<Ptr>(mem) + size),
      tail_(static_cast<Ptr>(mem) + size) {}

struct MemoryBlock::Sentinel {
  static constexpr MemoryBlock sentinel;
};

constexpr MemoryBlock* MemoryBlock::sentinel() {
  return const_cast<MemoryBlock*>(&Sentinel::sentinel);
}

inline constexpr Ptr MemoryBlock::head() const {
  return const_cast<Ptr>(raw_ + sizeof(raw_));
}
inline constexpr Ptr MemoryBlock::limit() const {
  return const_cast<Ptr>(limit_);
}
inline constexpr Ptr MemoryBlock::tail() const {
  return const_cast<Ptr>(tail_);
}

inline void MemoryBlock::set_limit(Ptr limit) {
  // Avoid sentinel by avoiding const update
  if (limit != limit_) {
    GOOGLE_DCHECK_NE(this, sentinel());
    limit_ = limit;
  }
}

constexpr MemoryBlock* MemoryBlock::next() const { return next_; }

constexpr size_t MemoryBlock::size() const { return tail_ - raw_; }

constexpr size_t MemoryBlock::allocated_size() const {
  return this == sentinel() ? 0 : size();
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_MEMORY_BLOCK_H__
