// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
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

#ifndef GOOGLE_PROTOBUF_ARENA_CLEANUPX_H__
#define GOOGLE_PROTOBUF_ARENA_CLEANUPX_H__

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iosfwd>
#include <string>

#include "google/protobuf/stubs/logging.h"
#include "google/protobuf/stubs/common.h"
#include "absl/strings/cord.h"
#include "google/protobuf/arena_align.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
namespace cleanupx {

// Pseudo object destructor
template <typename T>
void destruct_object(void* object) {
  reinterpret_cast<T*>(object)->~T();
}

// ---------------------------------
// Tag definition
// ---------------------------------
enum class Tag : uintptr_t {
  kDynamic = 0,
  kDynamicPointer = 1,
  kString = 2,
  kCord = 3,
};
inline uintptr_t Cast(Tag tag) { return static_cast<uintptr_t>(tag); }

inline constexpr bool IsTypedTag(Tag tag) {
  return tag == Tag::kString || tag == Tag::kCord;
}

template <typename T>
inline constexpr Tag TagFor() {
  return Tag::kDynamic;
}

template <>
inline constexpr Tag TagFor<std::string>() {
  return Tag::kString;
}

template <>
inline constexpr Tag TagFor<absl::Cord>() {
  return Tag::kCord;
}

inline const void* AsPointer(void (*dtor)(void*)) {
  const void* p;
  memcpy(&p, &dtor, sizeof(const void*));
  return p;
}

std::ostream& operator<<(std::ostream&, Tag);  // NOLINT

// ---------------------------------
// Meta information nodes
// ---------------------------------
struct alignas(ArenaAlignDefault::align) TaggedNode {
  uintptr_t object;
};

struct alignas(ArenaAlignDefault::align) DynamicNode {
  uintptr_t ptr_or_size;
  void (*dtor)(void*);
};

struct NodeRef {
  const void* address;
};

std::ostream& operator<<(std::ostream&, NodeRef);      // NOLINT
std::ostream& operator<<(std::ostream&, DynamicNode);  // NOLINT
std::ostream& operator<<(std::ostream&, TaggedNode);   // NOLINT

// ---------------------------------
// Type reflection
// ---------------------------------
template <typename T>
struct TypeInfo {
  static constexpr Tag tag = TagFor<T>();
  static constexpr size_t align = ArenaAlignDefault::Ceil(alignof(T));
  static constexpr auto align_as = ArenaAlignAs<align>();
  static constexpr size_t size = align_as.Ceil(sizeof(T));
  static constexpr auto destructor = &destruct_object<T>;
};

// ---------------------------------
// Cleanup allocation arguments
// --------------------------------
template <typename T>
struct TypedAllocation {
  using NodeType = TaggedNode;
  static constexpr size_t meta_size = sizeof(TaggedNode);
  static constexpr size_t allocation_size() { return TypeInfo<T>::size; }

  TaggedNode CreateMeta() { return TaggedNode{Cast(TagFor<T>())}; };
};

using StringAllocation = TypedAllocation<std::string>;
using CordAllocation = TypedAllocation<absl::Cord>;

struct TaggedAllocation {
  using NodeType = TaggedNode;
  static constexpr size_t meta_size = sizeof(TaggedNode);

  Tag tag;

  constexpr size_t allocation_size() const {
    return tag == Tag::kString ? TypeInfo<std::string>::size
                               : TypeInfo<absl::Cord>::size;
  }

  TaggedNode CreateMeta() { return TaggedNode{Cast(tag)}; }
};

struct TaggedCleanup {
  using NodeType = TaggedNode;
  static constexpr size_t meta_size = sizeof(TaggedNode);

  void* object;
  Tag tag;

  constexpr size_t allocation_size() const { return 0; }

  TaggedNode CreateMeta() {
    return TaggedNode{reinterpret_cast<uintptr_t>(object) + Cast(tag)};
  }
};

struct DynamicAllocation {
  using NodeType = DynamicNode;
  static constexpr size_t meta_size = sizeof(DynamicNode);

  size_t size;
  void (*dtor)(void*);

  constexpr size_t allocation_size() const { return size; }

  DynamicNode CreateMeta() { return DynamicNode{size, dtor}; }
};

struct DynamicCleanup {
  using NodeType = DynamicNode;
  static constexpr size_t meta_size = sizeof(DynamicNode);

  void* object;
  void (*dtor)(void*);

  constexpr size_t allocation_size() const { return 0; }

  inline DynamicNode CreateMeta() {
    uintptr_t ptr = reinterpret_cast<uintptr_t>(object);
    return DynamicNode{ptr + Cast(Tag::kDynamicPointer), dtor};
  }
};

std::ostream& operator<<(std::ostream& s, StringAllocation);       // NOLINT
std::ostream& operator<<(std::ostream& s, CordAllocation);         // NOLINT
std::ostream& operator<<(std::ostream& s, TaggedAllocation arg);   // NOLINT
std::ostream& operator<<(std::ostream& s, TaggedCleanup arg);      // NOLINT
std::ostream& operator<<(std::ostream& s, DynamicAllocation arg);  // NOLINT
std::ostream& operator<<(std::ostream& s, DynamicCleanup arg);     // NOLINT

inline TaggedAllocation CleanupArg(Tag tag) {
  GOOGLE_DCHECK(IsTypedTag(tag));
  return TaggedAllocation{tag};
}

inline TaggedCleanup CleanupArg(void* object, Tag tag) {
  GOOGLE_DCHECK(IsTypedTag(tag));
  return TaggedCleanup{object, tag};
}

inline DynamicAllocation CleanupArg(size_t size, void (*dtor)(void*)) {
  return DynamicAllocation{size, dtor};
}

inline DynamicCleanup CleanupArg(void* object, void (*dtor)(void*)) {
  return DynamicCleanup{object, dtor};
}

template <typename T>
struct CleanupArgFactory {
  static auto Create() {
    return CleanupArg(TypeInfo<T>::size, TypeInfo<T>::destructor);
  }
  static auto Create(void* object) {
    return CleanupArg(object, TypeInfo<T>::destructor);
  }
};

template <>
struct CleanupArgFactory<std::string> {
  static auto Create() { return StringAllocation{}; }
  static auto Create(void* object) { return CleanupArg(object, Tag::kString); }
};

template <>
struct CleanupArgFactory<absl::Cord> {
  static auto Create() { return CordAllocation{}; }
  static auto Create(void* object) { return CleanupArg(object, Tag::kCord); }
};

template <typename T>
inline auto CleanupArgFor() {
  return CleanupArgFactory<T>::Create();
}

template <typename T>
inline auto CleanupArgFor(T* object) {
  return CleanupArgFactory<T>::Create(object);
}

// ---------------------------------
// Write nodes
// ---------------------------------
inline void WriteSkip(void* pos, size_t size) {
  if (size > ArenaAlignDefault::align) {
    DynamicNode n{size - sizeof(DynamicNode), nullptr};
    memcpy(pos, &n, sizeof(n));
  } else if (size) {
    TaggedNode n{Cast(Tag::kDynamicPointer)};
    memcpy(pos, &n, sizeof(n));
  }
}

template <typename Ptr, typename Cleanup, typename Align>
inline Ptr RWriteNode(Ptr ptr, Cleanup cleanup, Align align) {
  if (size_t skip = align.ModDefaultAligned(ptr)) {
    cleanupx::WriteSkip(ptr -= skip, skip);
  }
  ptr -= cleanup.meta_size + cleanup.allocation_size();
  auto node = cleanup.CreateMeta();
  memcpy(ptr, &node, sizeof(node));
  return ptr;
}

template <typename Ptr, typename Cleanup, typename Align>
inline Ptr FWriteNode(Ptr ptr, Cleanup cleanup, Align align) {
  if (size_t mod = align.ModDefaultAligned(ptr + cleanup.meta_size)) {
    cleanupx::WriteSkip(ptr, align.align - mod);
    ptr += align.align - mod;
  }
  auto node = cleanup.CreateMeta();
  memcpy(ptr, &node, sizeof(node));
  return ptr + cleanup.meta_size;
}

// ---------------------------------
// Inspect functions at cleanup time
// ---------------------------------
inline Tag TagAt(const void* pos) {
  uintptr_t header = *reinterpret_cast<const uintptr_t*>(pos);
  return static_cast<Tag>(header & 3);
}

// ---------------------------------
// Cleanup functions
// ---------------------------------
template <typename T>
inline size_t DestroyTaggedObject(void* pos, uintptr_t size) {
  // size == 0 identifies embedded, size != 0 pointer cleanup
  if (size) {
    reinterpret_cast<T*>(size)->~T();
    return sizeof(TaggedNode);
  } else {
    reinterpret_cast<T*>(static_cast<TaggedNode*>(pos) + 1)->~T();
    return sizeof(TaggedNode) + TypeInfo<T>::size;
  }
}

inline PROTOBUF_NDEBUG_INLINE size_t DestroyNodeAt(void* pos) {
  uintptr_t head = *static_cast<const uintptr_t*>(pos);
  uintptr_t tag = head & 3;
  switch (static_cast<Tag>(tag)) {
    case Tag::kDynamic:
      // dtor == null --> Skip 'size' bytes
      if (auto dtor = static_cast<DynamicNode*>(pos)->dtor) {
        dtor(static_cast<uint8_t*>(pos) + sizeof(DynamicNode));
      }
      return sizeof(DynamicNode) + head;

    case Tag::kDynamicPointer:
      // object == null --> Skip one word
      if (auto object = reinterpret_cast<void*>(head - tag)) {
        static_cast<DynamicNode*>(pos)->dtor(object);
        return sizeof(DynamicNode);
      }
      return ArenaAlignDefault::align;

    case Tag::kString:
      return DestroyTaggedObject<std::string>(pos, head - tag);

    case Tag::kCord:
      return DestroyTaggedObject<absl::Cord>(pos, head - tag);
  }
}

template <typename T>
inline size_t PrefetchTaggedObject(uintptr_t size) {
  // size == 0 identifies embedded, size != 0 pointer cleanup
  if (size) {
    ::compiler::PrefetchNta(reinterpret_cast<void*>(size));
    return sizeof(TaggedNode);
  } else {
    return sizeof(TaggedNode) + TypeInfo<T>::size;
  }
}

inline PROTOBUF_NDEBUG_INLINE size_t PrefetchNodeAt(void* pos) {
  uintptr_t head = *static_cast<const uintptr_t*>(pos);
  uintptr_t tag = head & 3;
  switch (static_cast<Tag>(tag)) {
    case Tag::kDynamic:
      return sizeof(DynamicNode) + head;

    case Tag::kDynamicPointer:
      if (auto object = reinterpret_cast<void*>(head - tag)) {
        ::compiler::PrefetchNta(object);
        return sizeof(DynamicNode);
      }
      return ArenaAlignDefault::align;

    case Tag::kString:
      return PrefetchTaggedObject<std::string>(head - tag);

    case Tag::kCord:
      return PrefetchTaggedObject<absl::Cord>(head - tag);
  }
}

}  // namespace cleanupx
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_ARENA_CLEANUPX_H__
