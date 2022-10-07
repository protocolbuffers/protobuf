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



namespace google {
namespace protobuf {
namespace internal {
namespace cleanup {

// Helper function invoking the destructor of `object`
template <typename T>
void arena_destruct_object(void* object) {
  reinterpret_cast<T*>(object)->~T();
}

// Tag defines the type of cleanup / cleanup object. This tag is stored in the
// lowest 2 bits of the `elem` value identifying the type of node. All node
// types must start with a `uintptr_t` that stores `Tag` in its low two bits.
enum class Tag : uintptr_t {
  kDynamic = 0,  // DynamicNode
  kString = 1,   // StringNode (std::string)
};

// DynamicNode contains the object (`elem`) that needs to be
// destroyed, and the function to destroy it (`destructor`)
// elem must be aligned at minimum on a 4 byte boundary.
struct DynamicNode {
  uintptr_t elem;
  void (*destructor)(void*);
};

// StringNode contains a `std::string` object (`elem`) that needs to be
// destroyed. The lowest 2 bits of `elem` contain the non-zero kString tag.
struct StringNode {
  uintptr_t elem;
};


// EnableSpecializedTags() return true if the alignment of tagged objects
// such as std::string allow us to poke tags in the 2 LSB bits.
inline constexpr bool EnableSpecializedTags() {
  // For now we require 2 bits
  return alignof(std::string) >= 8;
}

// Adds a cleanup entry identified by `tag` at memory location `pos`.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE void CreateNode(Tag tag, void* pos,
                                                    const void* elem_raw,
                                                    void (*destructor)(void*)) {
  auto elem = reinterpret_cast<uintptr_t>(elem_raw);
  if (EnableSpecializedTags()) {
    GOOGLE_DCHECK_EQ(elem & 3, 0ULL);  // Must be aligned
    switch (tag) {
      case Tag::kString: {
        StringNode n = {elem | static_cast<uintptr_t>(Tag::kString)};
        memcpy(pos, &n, sizeof(n));
        return;
      }
      default:
        break;
    }
  }
  DynamicNode n = {elem, destructor};
  memcpy(pos, &n, sizeof(n));
}

// Optimization: performs a prefetch on `elem_address`.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE void PrefetchNode(
    const void* elem_address) {
  (void)elem_address;
}

// Destroys the node idenitfied by `tag` stored at memory location `pos`.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE void DestroyNode(Tag tag, const void* pos) {
  if (EnableSpecializedTags()) {
    switch (tag) {
      case Tag::kString: {
        StringNode n;
        memcpy(&n, pos, sizeof(n));
        auto* s = reinterpret_cast<std::string*>(n.elem & ~0x7ULL);
        // Some compilers don't like fully qualified explicit dtor calls,
        // so use an alias to avoid having to type `::`.
        using string_type = std::string;
        s->~string_type();
        return;
      }
      default:
        break;
    }
  }
  DynamicNode n;
  memcpy(&n, pos, sizeof(n));
  n.destructor(reinterpret_cast<void*>(n.elem));
}

// Returns the `tag` identifying the type of object for `destructor` or
// kDynamic if `destructor` does not identify a well know object type.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE Tag Type(void (*destructor)(void*)) {
  if (EnableSpecializedTags()) {
    if (destructor == &arena_destruct_object<std::string>) {
      return Tag::kString;
    }
  }
  return Tag::kDynamic;
}

// Returns the `tag` identifying the type of object stored at memory location
// `elem`, which represents the first uintptr_t value in the node.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE Tag Type(void* raw) {
  if (!EnableSpecializedTags()) return Tag::kDynamic;

  uintptr_t elem;
  memcpy(&elem, raw, sizeof(elem));
  switch (static_cast<Tag>(elem & 0x7ULL)) {
    case Tag::kDynamic:
      return Tag::kDynamic;
    case Tag::kString:
      return Tag::kString;
    default:
      GOOGLE_LOG(FATAL) << "Corrupted cleanup tag: " << (elem & 0x7ULL);
      return Tag::kDynamic;
  }
}

// Returns the required size in bytes off the node type identified by `tag`.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE size_t Size(Tag tag) {
  if (!EnableSpecializedTags()) return sizeof(DynamicNode);

  switch (tag) {
    case Tag::kDynamic:
      return sizeof(DynamicNode);
    case Tag::kString:
      return sizeof(StringNode);
    default:
      GOOGLE_LOG(FATAL) << "Corrupted cleanup tag: " << static_cast<int>(tag);
      return sizeof(DynamicNode);
  }
}

// Returns the required size in bytes off the node type for `destructor`.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE size_t Size(void (*destructor)(void*)) {
  return destructor == nullptr ? 0 : Size(Type(destructor));
}

}  // namespace cleanup
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_ARENA_CLEANUP_H__
