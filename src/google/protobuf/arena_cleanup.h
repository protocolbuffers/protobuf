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
#include "absl/base/attributes.h"
#include "absl/strings/cord.h"


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

// TaggedNode contains a `std::string` or `absl::Cord` object (`elem`) that
// needs to be destroyed. The lowest 2 bits of `elem` contain the non-zero
// `kString` or `kCord` tag.
struct TaggedNode {
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

// Adds a cleanup entry identified by `tag` at memory location `pos`.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE void CreateNode(Tag tag, void* pos,
                                                    const void* elem_raw,
                                                    void (*destructor)(void*)) {
  auto elem = reinterpret_cast<uintptr_t>(elem_raw);
  if (EnableTags()) {
    GOOGLE_DCHECK_EQ(elem & 3, 0ULL);  // Must be aligned
    switch (tag) {
      case Tag::kString: {
        TaggedNode n = {elem | static_cast<uintptr_t>(Tag::kString)};
        memcpy(pos, &n, sizeof(n));
        return;
      }
      case Tag::kCord: {
        TaggedNode n = {elem | static_cast<uintptr_t>(Tag::kCord)};
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
// Returns the size of the cleanup (meta) data at this address, allowing the
// caller to advance cleanup iterators without needing to examine or know
// anything about the underlying cleanup node or cleanup meta data / tags.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE size_t
PrefetchNode(const void* elem_address) {
  if (EnableTags()) {
    uintptr_t elem;
    memcpy(&elem, elem_address, sizeof(elem));
    if (static_cast<Tag>(elem & 3) != Tag::kDynamic) {
      return sizeof(TaggedNode);
    }
  }
  return sizeof(DynamicNode);
}

// Destroys the object referenced by the cleanup node at memory location `pos`.
// Returns the size of the cleanup (meta) data at this address, allowing the
// caller to advance cleanup iterators without needing to examine or know
// anything about the underlying cleanup node or cleanup meta data / tags.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE size_t DestroyNode(const void* pos) {
  uintptr_t elem;
  memcpy(&elem, pos, sizeof(elem));
  if (EnableTags()) {
    switch (static_cast<Tag>(elem & 3)) {
      case Tag::kString: {
        // Some compilers don't like fully qualified explicit dtor calls,
        // so use an alias to avoid having to type `::`.
        using T = std::string;
        reinterpret_cast<T*>(elem - static_cast<uintptr_t>(Tag::kString))->~T();
        return sizeof(TaggedNode);
      }
      case Tag::kCord: {
        using T = absl::Cord;
        reinterpret_cast<T*>(elem - static_cast<uintptr_t>(Tag::kCord))->~T();
        return sizeof(TaggedNode);
      }
      default:
        break;
    }
  }
  static_cast<const DynamicNode*>(pos)->destructor(
      reinterpret_cast<void*>(elem - static_cast<uintptr_t>(Tag::kDynamic)));
  return sizeof(DynamicNode);
}

// Returns the `tag` identifying the type of object stored at memory location
// `elem`, which represents the first uintptr_t value in the node.
inline ABSL_ATTRIBUTE_ALWAYS_INLINE Tag Type(void* raw) {
  if (!EnableTags()) return Tag::kDynamic;

  uintptr_t elem;
  memcpy(&elem, raw, sizeof(elem));
  switch (static_cast<Tag>(elem & 0x7ULL)) {
    case Tag::kDynamic:
      return Tag::kDynamic;
    case Tag::kString:
      return Tag::kString;
    case Tag::kCord:
      return Tag::kCord;
    default:
      GOOGLE_LOG(FATAL) << "Corrupted cleanup tag: " << (elem & 0x7ULL);
      return Tag::kDynamic;
  }
}

inline size_t CleanupSize(void (*)(void*)) { return sizeof(DynamicNode); }
inline size_t CleanupSize(Tag) { return sizeof(TaggedNode); }

}  // namespace cleanup
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_ARENA_CLEANUP_H__
