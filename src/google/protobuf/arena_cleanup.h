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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include "google/protobuf/stubs/logging.h"
#include "absl/base/attributes.h"
#include "absl/strings/cord.h"
#include "google/protobuf/arena_align.h"


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

// Tag defines the type of cleanup / cleanup object. This tag is stored in the
// lowest 2 bits of the `elem` value identifying the type of node. All node
// types must start with a `uintptr_t` that stores `Tag` in its low two bits.
enum class Tag : uintptr_t {
  kDynamic = 0,  // DynamicNode
  kString = 1,   // TaggedNode (std::string)
  kCord = 2,     // TaggedNode (absl::Cord)
};

// DynamicNode contains the object (`elem`) that needs to be
// destroyed, and the function to destroy it (`destructor`)
// elem must be aligned at minimum on a 4 byte boundary.
struct DynamicNode {
  uintptr_t elem;
  void (*destructor)(void*);
};

// std::max is not constexpr in c11
template <typename T>
inline constexpr T Max(T lhs, T rhs) {
  return lhs < rhs ? rhs : lhs;
}

// TaggedNode contains a `std::string` or `absl::Cord` object (`elem`) that
// needs to be destroyed. The lowest 2 bits of `elem` contain the non-zero
// `kString` or `kCord` tag. TaggedNode must have an alignment matching at
// least the alignment of std::string and absl::Cord.
struct alignas(Max(alignof(std::string), alignof(absl::Cord))) TaggedNode {
  uintptr_t elem;
};

// EnableTags() return true if the alignment of tagged objects
// such as std::string allow us to poke tags in the 2 LSB bits.
inline constexpr bool EnableTags() {
  // For now we require 2 bits
  return alignof(std::string) >= 8 && alignof(absl::Cord) >= 8;
}

// Adds a cleanup entry invoking 'cleanup' on `object` at memory location `pos`.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE void CreateNode(void* pos,
                                                    const void* object,
                                                    void (*cleanup)(void*)) {
  auto elem = reinterpret_cast<uintptr_t>(object);
  DynamicNode n = {elem, cleanup};
  memcpy(pos, &n, sizeof(n));
}

// Adds a cleanup entry invoking the destructor of `object`, who's type
// is identified by `tag` at memory location `pos`.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE void CreateNode(void* pos,
                                                    const void* object,
                                                    Tag tag) {
  GOOGLE_DCHECK(tag == Tag::kString || tag == Tag::kCord);
  auto elem = reinterpret_cast<uintptr_t>(object);
  GOOGLE_DCHECK_EQ(elem & 3, 0);
  TaggedNode n = {elem + static_cast<uintptr_t>(tag)};
  memcpy(pos, &n, sizeof(n));
}

inline ABSL_ATTRIBUTE_ALWAYS_INLINE void Prefetch(const void* address) {
}

template <typename T>
inline constexpr size_t NodeSize() {
  return (std::is_same<T, std::string>::value ||
          std::is_same<T, absl::Cord>::value)
             ? sizeof(TaggedNode)
             : sizeof(DynamicNode);
}

inline size_t CleanupSize(void (*)(void*)) {
  return ArenaAlignDefault::Ceil(sizeof(DynamicNode));
}
inline size_t CleanupSize(Tag) {
  return ArenaAlignDefault::Ceil(sizeof(TaggedNode));
}

template <typename T>
inline constexpr size_t AllocationSize() {
  return ArenaAlignDefault::Ceil(NodeSize<T>() + sizeof(T));
}

// Optimization: performs a prefetch on `elem_address`.
// Returns the size of the cleanup (meta) data at this address, allowing the
// caller to advance cleanup iterators without needing to examine or know
// anything about the underlying cleanup node or cleanup meta data / tags.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE size_t
PrefetchNode(const void* elem_address) {
  if (EnableTags()) {
    uintptr_t elem;
    memcpy(&elem, elem_address, sizeof(elem));
    uintptr_t tag = elem & 3U;
    elem -= tag;
    switch (static_cast<Tag>(tag)) {
      case Tag::kString:
        if (elem == 0U) return AllocationSize<std::string>();
        Prefetch(reinterpret_cast<const void*>(elem));
        return ArenaAlignDefault::Ceil(sizeof(TaggedNode));

      case Tag::kCord:
        if (elem == 0U) return AllocationSize<absl::Cord>();
        Prefetch(reinterpret_cast<const void*>(elem));
        return ArenaAlignDefault::Ceil(sizeof(TaggedNode));

      case Tag::kDynamic:
        break;
    }
  }
  return sizeof(DynamicNode);
}

template <typename Node, typename T>
T* NodePointerToObjectPointer(void* pos) {
  return reinterpret_cast<T*>(reinterpret_cast<Node*>(pos) + 1);
}

// Destroys the object referenced by the cleanup node at memory location `pos`.
// Returns the size of the cleanup (meta) data at this address, allowing the
// caller to advance cleanup iterators without needing to examine or know
// anything about the underlying cleanup node or cleanup meta data / tags.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE size_t DestroyNode(void* pos) {
  uintptr_t elem;
  memcpy(&elem, pos, sizeof(elem));
  if (EnableTags()) {
    uintptr_t tag = elem & 3U;
    elem -= tag;
    switch (static_cast<Tag>(tag)) {
      case Tag::kString: {
        using T = std::string;
        if (elem == 0U) {
          NodePointerToObjectPointer<TaggedNode, T>(pos)->~T();
          return AllocationSize<T>();
        }
        reinterpret_cast<T*>(elem)->~T();
        return ArenaAlignDefault::Ceil(sizeof(TaggedNode));
      }
      case Tag::kCord: {
        using T = absl::Cord;
        if (elem == 0U) {
          NodePointerToObjectPointer<TaggedNode, T>(pos)->~T();
          return AllocationSize<T>();
        }
        reinterpret_cast<T*>(elem)->~T();
        return ArenaAlignDefault::Ceil(sizeof(TaggedNode));
      }
      default:
        break;
    }
  }
  static_cast<const DynamicNode*>(pos)->destructor(
      reinterpret_cast<void*>(elem - static_cast<uintptr_t>(Tag::kDynamic)));
  return sizeof(DynamicNode);
}

}  // namespace cleanup
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_ARENA_CLEANUP_H__
