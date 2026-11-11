// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_METADATA_LITE_H__
#define GOOGLE_PROTOBUF_METADATA_LITE_H__

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/class_data.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {

class UnknownFieldSet;

namespace internal {

// This is the representation for messages that support arena allocation. It
// uses a tagged pointer to either store the owning Arena pointer, if there are
// no unknown fields, or a pointer to a block of memory with both the owning
// Arena pointer and the UnknownFieldSet, if there are unknown fields. Besides,
// it also uses the tag to distinguish whether the owning Arena pointer is also
// used by sub-structure allocation. This optimization allows for
// "zero-overhead" storage of the Arena pointer, relative to the above baseline
// implementation.
//
// The tagged pointer uses the least two significant bits to disambiguate cases.
// It uses bit 0 == 0 to indicate an arena pointer and bit 0 == 1 to indicate a
// UFS+Arena-container pointer. Besides it uses bit 1 == 0 to indicate arena
// allocation and bit 1 == 1 to indicate heap allocation.
class PROTOBUF_EXPORT InternalMetadata {
 public:
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
  constexpr explicit InternalMetadata(const ClassData* class_data)
      : ptr_(class_data) {}
#else
  constexpr InternalMetadata() : ptr_(nullptr) {}
  explicit InternalMetadata(Arena* arena) : ptr_(arena) {}
#endif

  // Delete will delete the unknown fields only if they weren't allocated on an
  // arena.  Then it updates the flags so that if you call
  // have_unknown_fields(), it will return false.
  //
  // It is designed to be used as part of a Message class's destructor call, so
  // that when control eventually gets to ~InternalMetadata(), we don't need to
  // check for have_unknown_fields() again.
  template <typename T>
  void Delete() {
    // Note that Delete<> should be called not more than once.
    if (have_unknown_fields()) {
      DeleteOutOfLineHelper<T>();
    }
  }

#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
  const ClassData* class_data() const {
    if (ABSL_PREDICT_FALSE(have_unknown_fields())) {
      return PtrValue<ContainerBase>()->class_data;
    } else {
      return PtrValue<const ClassData>();
    }
  }
#else
  PROTOBUF_NDEBUG_INLINE Arena* arena() const {
    if (ABSL_PREDICT_FALSE(have_unknown_fields())) {
      return PtrValue<ContainerBase>()->arena;
    } else {
      return PtrValue<Arena>();
    }
  }
#endif

  PROTOBUF_NDEBUG_INLINE bool have_unknown_fields() const {
    return HasUnknownFieldsTag();
  }

  template <typename T>
  PROTOBUF_NDEBUG_INLINE const T& unknown_fields(
      const T& (*default_instance)()) const {
    if (ABSL_PREDICT_FALSE(have_unknown_fields())) {
      return PtrValue<Container<T>>()->unknown_fields;
    } else {
      return default_instance();
    }
  }

  template <typename T>
  PROTOBUF_NDEBUG_INLINE T* mutable_unknown_fields() {
    if (ABSL_PREDICT_TRUE(have_unknown_fields())) {
      return &PtrValue<Container<T>>()->unknown_fields;
    } else {
      return mutable_unknown_fields_slow<T>();
    }
  }

  template <typename T>
  PROTOBUF_NDEBUG_INLINE void Swap(InternalMetadata* other) {
    // Semantics here are that we swap only the unknown fields, not the arena
    // pointer. We cannot simply swap ptr_ with other->ptr_ because we need to
    // maintain our own arena ptr. Also, our ptr_ and other's ptr_ may be in
    // different states (direct arena pointer vs. container with UFS) so we
    // cannot simply swap ptr_ and then restore the arena pointers. We reuse
    // UFS's swap implementation instead.
    if (have_unknown_fields() || other->have_unknown_fields()) {
      DoSwap<T>(other->mutable_unknown_fields<T>());
    }
  }

  PROTOBUF_NDEBUG_INLINE void InternalSwap(
      InternalMetadata* PROTOBUF_RESTRICT other) {
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
    ABSL_DCHECK_EQ(class_data(), other->class_data());
#endif
    std::swap(ptr_, other->ptr_);
  }

  template <typename T>
  PROTOBUF_NDEBUG_INLINE void MergeFrom(const InternalMetadata& other) {
    if (ABSL_PREDICT_FALSE(other.have_unknown_fields())) {
      DoMergeFrom<T>(other.unknown_fields<T>(nullptr));
    }
  }

  template <typename T>
  PROTOBUF_NDEBUG_INLINE void Clear() {
    if (ABSL_PREDICT_FALSE(have_unknown_fields())) {
      DoClear<T>();
    }
  }

 private:
  const void* ptr_;

  // Tagged pointer implementation.
  static constexpr uintptr_t kUnknownFieldsTagMask = 1;
  static constexpr uintptr_t kPtrTagMask = kUnknownFieldsTagMask;
  static constexpr uintptr_t kPtrValueMask = ~kPtrTagMask;

  uintptr_t uptr() const { return reinterpret_cast<uintptr_t>(ptr_); }

  // Accessors for pointer tag and pointer value.
  PROTOBUF_ALWAYS_INLINE bool HasUnknownFieldsTag() const {
    return uptr() & kUnknownFieldsTagMask;
  }

  template <typename U>
  U* PtrValue() const {
    if constexpr (std::is_same_v<U, Arena> ||
                  std::is_same_v<U, const ClassData>) {
      // No mask to remove.
      ABSL_DCHECK_EQ(uptr() & kPtrTagMask, 0u);
      return reinterpret_cast<U*>(uptr());
    } else {
      static_assert(kPtrTagMask == 1);
      ABSL_DCHECK_EQ(uptr() & kPtrTagMask, kPtrTagMask);
      // We can remove the mask via -1, which is smaller asm and can be merged
      // with other arithmetic operations.
      // Eg PtrValue<Container>()->unknown_fields can merge the offset into the
      // mask removal.
      return reinterpret_cast<U*>(uptr() - kPtrTagMask);
    }
  }

#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
  Arena* arena() const;
#endif

  // If ptr_'s tag is kTagContainer, it points to an instance of this struct.
  struct ContainerBase {
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
    const ClassData* class_data;

    explicit ContainerBase(const ClassData* input_class_data)
        : class_data(input_class_data) {}
#else
    Arena* arena;

    explicit ContainerBase(Arena* input_arena) : arena(input_arena) {}
#endif
  };

  template <typename T>
  struct Container;

  template <typename T>
  PROTOBUF_NOINLINE void DeleteOutOfLineHelper();

  template <typename T>
  PROTOBUF_NOINLINE T* mutable_unknown_fields_slow();

  // Templated functions.

  template <typename T>
  PROTOBUF_NOINLINE void DoClear();

  template <typename T>
  PROTOBUF_NOINLINE void DoMergeFrom(const T& other);

  template <typename T>
  PROTOBUF_NOINLINE void DoSwap(T* other);

  // Private helper with debug checks for ~InternalMetadata()
  void CheckedDestruct();
};

// String Template specializations.

template <>
struct InternalMetadata::Container<std::string> : public ContainerBase {
  std::string unknown_fields;

#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
  Container(Arena*, const ClassData* input_class_data)
      : ContainerBase(input_class_data) {}
#else
  explicit Container(Arena* arena_param) : ContainerBase(arena_param) {}
#endif

  using InternalArenaConstructable_ = void;
};

template <>
PROTOBUF_EXPORT void InternalMetadata::DoClear<std::string>();
template <>
PROTOBUF_EXPORT void InternalMetadata::DoMergeFrom<std::string>(
    const std::string& other);
template <>
PROTOBUF_EXPORT void InternalMetadata::DoSwap<std::string>(std::string* other);
template <>
PROTOBUF_EXPORT void InternalMetadata::DeleteOutOfLineHelper<std::string>();
extern template PROTOBUF_EXPORT std::string*
InternalMetadata::mutable_unknown_fields_slow<std::string>();

// Instantiated once in message.cc (where the definition of UnknownFieldSet is
// known) to prevent much duplication across translation units of a large build.
template <>
PROTOBUF_EXPORT void InternalMetadata::DoClear<UnknownFieldSet>();
template <>
PROTOBUF_EXPORT void InternalMetadata::DoMergeFrom<UnknownFieldSet>(
    const UnknownFieldSet& other);
template <>
PROTOBUF_EXPORT void InternalMetadata::DoSwap<UnknownFieldSet>(
    UnknownFieldSet* other);
template <>
PROTOBUF_EXPORT void InternalMetadata::DeleteOutOfLineHelper<UnknownFieldSet>();
extern template PROTOBUF_EXPORT UnknownFieldSet*
InternalMetadata::mutable_unknown_fields_slow<UnknownFieldSet>();

// This helper RAII class is needed to efficiently parse unknown fields. We
// should only call mutable_unknown_fields if there are actual unknown fields.
// The obvious thing to just use a stack string and swap it at the end of
// the parse won't work, because the destructor of StringOutputStream needs to
// be called before we can modify the string (it check-fails). Using
// LiteUnknownFieldSetter setter(&_internal_metadata_);
// StringOutputStream stream(setter.buffer());
// guarantees that the string is only swapped after stream is destroyed.
class PROTOBUF_EXPORT LiteUnknownFieldSetter {
 public:
  explicit LiteUnknownFieldSetter(InternalMetadata* metadata)
      : metadata_(metadata) {
    if (metadata->have_unknown_fields()) {
      buffer_.swap(*metadata->mutable_unknown_fields<std::string>());
    }
  }
  ~LiteUnknownFieldSetter() {
    if (!buffer_.empty())
      metadata_->mutable_unknown_fields<std::string>()->swap(buffer_);
  }
  std::string* buffer() { return &buffer_; }

 private:
  InternalMetadata* metadata_;
  std::string buffer_;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_METADATA_LITE_H__
