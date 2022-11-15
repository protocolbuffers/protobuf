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

#ifndef GOOGLE_PROTOBUF_ARENA_CLEANUP_H__
#define GOOGLE_PROTOBUF_ARENA_CLEANUP_H__

#include <cstddef>
#include <cstdint>
#include <string>

#include "google/protobuf/stubs/logging.h"
#include "google/protobuf/stubs/common.h"
#include "absl/base/attributes.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
namespace cleanup {

// Helper function invoking the destructor of `object`
template <typename T>
void arena_destruct_object(void* object) {
  reinterpret_cast<T*>(object)->~T();
}

// DynamicNode contains the object (`elem`) that needs to be
// destroyed, and the function to destroy it (`destructor`)
// elem must be aligned at minimum on a 4 byte boundary.
struct DynamicNode {
  uintptr_t elem;
  void (*destructor)(void*);
};

// Adds a cleanup entry at memory location `pos`.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE void CreateNode(void* pos,
                                                    const void* elem_raw,
                                                    void (*destructor)(void*)) {
  auto elem = reinterpret_cast<uintptr_t>(elem_raw);
  DynamicNode n = {elem, destructor};
  memcpy(pos, &n, sizeof(n));
}

// Optimization: performs a prefetch on `elem_address`.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE void PrefetchNode(
    const void* elem_address) {
  (void)elem_address;
}

// Destroys the node stored at memory location `pos`.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE void DestroyNode(const void* pos) {
  DynamicNode n;
  memcpy(&n, pos, sizeof(n));
  n.destructor(reinterpret_cast<void*>(n.elem));
}

// Returns the required size in bytes for a DynamicNode.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE size_t Size() {
  return sizeof(DynamicNode);
}

// Block on the arena for std::strings so that we can destruct them efficiently.
// This is a linked list. The first block may be partially full, but all other
// blocks are full.
struct StringBlock {
  static constexpr size_t kMaxSize = 512;
  static constexpr size_t kCapacity =
      (kMaxSize - sizeof(StringBlock*)) / sizeof(std::string);

  ABSL_ATTRIBUTE_ALWAYS_INLINE void* GetSpace(size_t i) {
    static_assert(sizeof(StringBlock) <= kMaxSize, "");
    return space + sizeof(std::string) * i;
  }
  ABSL_ATTRIBUTE_ALWAYS_INLINE std::string* Get(size_t i) {
    return static_cast<std::string*>(GetSpace(i));
  }

  StringBlock* next = nullptr;
  alignas(std::string) char space[kCapacity * sizeof(std::string)];
};

inline ABSL_ATTRIBUTE_ALWAYS_INLINE void DestroyStrings(
    StringBlock* block, size_t first_block_size) {
  // Some compilers don't like fully qualified explicit dtor calls,
  // so use an alias to avoid having to type `::`.
  using string_type = std::string;

#ifndef PROTO2_OPENSOURCE
  // Prefetch the next block if non-null.
  if (block->next != nullptr) ::compiler::PrefetchT0(block->next);
#endif
  for (size_t i = 0; i < first_block_size; ++i) {
    block->Get(i)->~string_type();
  }

  // All blocks other than the first are full.
  while (block->next != nullptr) {
    block = block->next;
#ifndef PROTO2_OPENSOURCE
    // Prefetch the next block if non-null.
    if (block->next != nullptr) ::compiler::PrefetchT0(block->next);
#endif
    for (size_t i = 0; i < StringBlock::kCapacity; ++i) {
      block->Get(i)->~string_type();
    }
  }
}

}  // namespace cleanup
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_ARENA_CLEANUP_H__
