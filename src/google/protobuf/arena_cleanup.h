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
  kDynamic = 0,   // DynamicNode
  kEmbedded = 1,  // Embedded node
  kString = 2,    // TaggedNode (std::string) (pointer or embedded)
  kCord = 3,      // TaggedNode (absl::Cord)  (pointer or embedded)
};

enum { kTagMask = 0x3 };

// DynamicNode contains the object (`elem`) that needs to be
// destroyed, and the function to destroy it (`destructor`)
// elem must be aligned at minimum on a 4 byte boundary.
struct DynamicNode {
  uintptr_t elem;
  void (*destructor)(void*);
};

// TaggedNode describes a well known cleanup instance identified by the non-zero
// tag value stored in the lowest 2 bits of `elem`.
struct TaggedNode {
  uintptr_t elem;
};

// EnableSpecializedTags() return true if the alignment of tagged objects
// such as std::string allow us to poke tags in the 2 LSB bits.
inline constexpr bool EnableSpecializedTags() {
  // For now we require 2 bits
  return alignof(std::string) > kTagMask;
}

inline ABSL_ATTRIBUTE_ALWAYS_INLINE void CreateNode(void* pos,
                                                    const void* object,
                                                    void (*dtor)(void*)) {
  auto elem = reinterpret_cast<uintptr_t>(object);
  DynamicNode n = {elem, dtor};
  memcpy(pos, &n, sizeof(n));
}

inline ABSL_ATTRIBUTE_ALWAYS_INLINE void CreateNode(void* pos, size_t size,
                                                    void (*dtor)(void*)) {
  GOOGLE_DCHECK_GT(size, 0);
  GOOGLE_DCHECK(ArenaAlignDefault::IsAligned(size));
  auto elem = static_cast<uintptr_t>(size);
  DynamicNode n = {elem + static_cast<uintptr_t>(Tag::kEmbedded), dtor};
  memcpy(pos, &n, sizeof(n));
}

inline ABSL_ATTRIBUTE_ALWAYS_INLINE void CreateNode(void* pos,
                                                    const void* object,
                                                    Tag tag) {
  GOOGLE_DCHECK(tag != Tag::kDynamic && tag != Tag::kEmbedded);
  auto elem = reinterpret_cast<uintptr_t>(object);
  GOOGLE_DCHECK_EQ(elem & 3, 0ULL);  // Must be aligned
  TaggedNode n = {elem + static_cast<uintptr_t>(tag)};
  memcpy(pos, &n, sizeof(n));
}

inline ABSL_ATTRIBUTE_ALWAYS_INLINE void CreateNode(void* pos, Tag tag) {
  GOOGLE_DCHECK(tag != Tag::kDynamic && tag != Tag::kEmbedded);
  TaggedNode n = {static_cast<uintptr_t>(tag)};
  memcpy(pos, &n, sizeof(n));
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
        TaggedNode n = {elem | static_cast<uintptr_t>(Tag::kString)};
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

template <typename T>
inline ABSL_ATTRIBUTE_ALWAYS_INLINE size_t DestroyTaggedObject(void* pos,
                                                               uintptr_t elem) {
  // elem == 0 identifies embedded, elem != 0 pointer cleanup
  if (elem) {
    reinterpret_cast<T*>(elem)->~T();
    return sizeof(TaggedNode);
  } else {
    reinterpret_cast<T*>(static_cast<TaggedNode*>(pos) + 1)->~T();
    return ArenaAlignDefault::Ceil(sizeof(TaggedNode) + sizeof(T));
  }
}

// Destroys the node idenitfied by `tag` stored at memory location `pos`.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE size_t DestroyNodeAt(void* pos) {
  uintptr_t head = *reinterpret_cast<uintptr_t*>(pos);
  uintptr_t tag = head & kTagMask;
  switch (static_cast<Tag>(tag)) {
    case Tag::kDynamic: {
      auto* node = static_cast<DynamicNode*>(pos);
      GOOGLE_DCHECK(node->destructor != nullptr);
      GOOGLE_DCHECK(head);
      node->destructor(reinterpret_cast<void*>(head));
      return sizeof(DynamicNode);
    }

    case Tag::kEmbedded: {
      auto* node = static_cast<DynamicNode*>(pos);
      GOOGLE_DCHECK(node->destructor != nullptr);
      GOOGLE_DCHECK_GT(head - tag, 0);
      node->destructor(node + 1);
      return head - tag;
    }

    case Tag::kString:
      return DestroyTaggedObject<std::string>(pos, head - tag);

    case Tag::kCord:
      return DestroyTaggedObject<absl::Cord>(pos, head - tag);
  }
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
    case Tag::kEmbedded:
      return sizeof(DynamicNode);
    case Tag::kString:
    case Tag::kCord:
      return sizeof(TaggedNode);
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

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_ARENA_CLEANUP_H__
