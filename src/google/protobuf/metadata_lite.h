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
#include <utility>

#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {

class UnknownFieldSet;

namespace internal {

struct ClassData;

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
  constexpr InternalMetadata() : ptr_(0) {}
#if defined(PROTOBUF_CUSTOM_VTABLE)
  constexpr explicit InternalMetadata(const ClassData* class_data)
      : ptr_(class_data) {}
  InternalMetadata(const ClassData* class_data, bool on_arena)
      : ptr_(reinterpret_cast<const ClassData*>(
            reinterpret_cast<uintptr_t>(class_data) |
            (on_arena ? kOnArenaTagMask : 0))) {}
#else
  explicit InternalMetadata(Arena* arena) {
    ptr_ = reinterpret_cast<intptr_t>(arena);
  }
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

#if defined(PROTOBUF_CUSTOM_VTABLE)
  bool on_arena() const { return OnArena(); }
#endif

#if defined(PROTOBUF_CUSTOM_VTABLE)
  PROTOBUF_NDEBUG_INLINE const ClassData* class_data() const {
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

#if defined(PROTOBUF_CUSTOM_VTABLE)
  PROTOBUF_NDEBUG_INLINE const void* raw_class_data() const { return ptr_; }
#else
  PROTOBUF_NDEBUG_INLINE void* raw_arena_ptr() const {
    return reinterpret_cast<void*>(ptr_);
  }
#endif

  template <typename T>
  PROTOBUF_NDEBUG_INLINE const T& unknown_fields(
      const T& (*default_instance)()) const {
    if (ABSL_PREDICT_FALSE(have_unknown_fields())) {
      return PtrValue<Container<T>>()->unknown_fields;
    } else {
      return default_instance();
    }
  }

#if defined(PROTOBUF_CUSTOM_VTABLE)
  template <typename T>
  PROTOBUF_NDEBUG_INLINE T* mutable_unknown_fields(google::protobuf::Arena* arena) {
    if (ABSL_PREDICT_TRUE(have_unknown_fields())) {
      return &PtrValue<Container<T>>()->unknown_fields;
    } else {
      return mutable_unknown_fields_slow<T>(arena);
    }
  }
#else
  template <typename T>
  PROTOBUF_NDEBUG_INLINE T* mutable_unknown_fields() {
    if (ABSL_PREDICT_TRUE(have_unknown_fields())) {
      return &PtrValue<Container<T>>()->unknown_fields;
    } else {
      return mutable_unknown_fields_slow<T>();
    }
  }
#endif

  template <typename T>
  PROTOBUF_NDEBUG_INLINE void Swap(
#if defined(PROTOBUF_CUSTOM_VTABLE)
      google::protobuf::Arena* arena,
#endif
      InternalMetadata* other) {
    // Semantics here are that we swap only the unknown fields, not the arena
    // pointer. We cannot simply swap ptr_ with other->ptr_ because we need to
    // maintain our own arena ptr. Also, our ptr_ and other's ptr_ may be in
    // different states (direct arena pointer vs. container with UFS) so we
    // cannot simply swap ptr_ and then restore the arena pointers. We reuse
    // UFS's swap implementation instead.
    if (have_unknown_fields() || other->have_unknown_fields()) {
      DoSwap<T>(
#if defined(PROTOBUF_CUSTOM_VTABLE)
          arena,
#endif
          other->mutable_unknown_fields<T>(
#if defined(PROTOBUF_CUSTOM_VTABLE)
              arena
#endif
              ));
    }
  }

  PROTOBUF_NDEBUG_INLINE void InternalSwap(
      InternalMetadata* PROTOBUF_RESTRICT other) {
    std::swap(ptr_, other->ptr_);
  }

  template <typename T>
  PROTOBUF_NDEBUG_INLINE void MergeFrom(
#if defined(PROTOBUF_CUSTOM_VTABLE)
      google::protobuf::Arena* arena,
#endif
      const InternalMetadata& other) {
    if (ABSL_PREDICT_FALSE(other.have_unknown_fields())) {
      DoMergeFrom<T>(
#if defined(PROTOBUF_CUSTOM_VTABLE)
          arena,
#endif
          other.unknown_fields<T>(nullptr));
    }
  }

  template <typename T>
  PROTOBUF_NDEBUG_INLINE void Clear(
#if defined(PROTOBUF_CUSTOM_VTABLE)
      google::protobuf::Arena* arena
#endif
  ) {
    if (ABSL_PREDICT_FALSE(have_unknown_fields())) {
      DoClear<T>(
#if defined(PROTOBUF_CUSTOM_VTABLE)
          arena
#endif
      );
    }
  }

 private:
#if defined(PROTOBUF_CUSTOM_VTABLE)
  const ClassData* ptr_;
#else
  intptr_t ptr_;
#endif

  // Tagged pointer implementation.
  static constexpr intptr_t kUnknownFieldsTagMask = 0x1;
#if defined(PROTOBUF_CUSTOM_VTABLE)
  static constexpr intptr_t kOnArenaTagMask = 0x2;
  static constexpr intptr_t kPtrTagMask =
      kUnknownFieldsTagMask | kOnArenaTagMask;
#else
  static constexpr intptr_t kPtrTagMask = kUnknownFieldsTagMask;
#endif
  static constexpr intptr_t kPtrValueMask = ~kPtrTagMask;

  // Accessors for pointer tag and pointer value.
  PROTOBUF_ALWAYS_INLINE bool HasUnknownFieldsTag() const {
#if defined(PROTOBUF_CUSTOM_VTABLE)
    return reinterpret_cast<uintptr_t>(ptr_) & kUnknownFieldsTagMask;
#else
    return ptr_ & kUnknownFieldsTagMask;
#endif
  }

#if defined(PROTOBUF_CUSTOM_VTABLE)
  PROTOBUF_ALWAYS_INLINE bool OnArena() const {
    return (reinterpret_cast<uintptr_t>(ptr_) & kOnArenaTagMask) != 0;
  }
#endif

  template <typename U>
  U* PtrValue() const {
#if defined(PROTOBUF_CUSTOM_VTABLE)
    return reinterpret_cast<U*>(reinterpret_cast<intptr_t>(ptr_) &
                                kPtrValueMask);
#else
    if constexpr (std::is_same_v<U, Arena>) {
      // No mask to remove.
      ABSL_DCHECK_EQ(ptr_ & kPtrTagMask, 0);
      return reinterpret_cast<U*>(ptr_);
    } else {
      static_assert(kPtrTagMask == 1);
      ABSL_DCHECK_EQ(ptr_ & kPtrTagMask, kPtrTagMask);
      // We can remove the mask via -1, which is smaller asm and can be merged
      // with other arithmetic operations.
      // Eg PtrValue<Container>()->unknown_fields can merge the offset into the
      // mask removal.
      return reinterpret_cast<U*>(ptr_ - kPtrTagMask);
    }
#endif
  }

  // If ptr_'s tag is kTagContainer, it points to an instance of this struct.
  struct ContainerBase {
#if defined(PROTOBUF_CUSTOM_VTABLE)
    const ClassData* class_data;
#else
    Arena* arena;
#endif
  };

  template <typename T>
  struct Container : public ContainerBase {
    T unknown_fields;
  };

  template <typename T>
  PROTOBUF_NOINLINE void DeleteOutOfLineHelper() {
    delete PtrValue<Container<T>>();
    // TODO:  This store is load-bearing.  Since we are destructing
    // the message at this point, see if we can eliminate it.
    ptr_ = 0;
  }

#if defined(PROTOBUF_CUSTOM_VTABLE)
  template <typename T>
  PROTOBUF_NOINLINE T* mutable_unknown_fields_slow(google::protobuf::Arena* arena) {
    const intptr_t on_arena =
        reinterpret_cast<intptr_t>(ptr_) & kOnArenaTagMask;
    const ClassData* class_data = this->class_data();
    Container<T>* container = Arena::Create<Container<T>>(arena);
    // Two-step assignment works around a bug in clang's static analyzer:
    // https://bugs.llvm.org/show_bug.cgi?id=34198.
#if defined(PROTOBUF_CUSTOM_VTABLE)
    ptr_ = reinterpret_cast<ClassData*>(reinterpret_cast<intptr_t>(container) |
                                        kUnknownFieldsTagMask | on_arena);
#else
    ptr_ = reinterpret_cast<intptr_t>(container);
    ptr_ |= kUnknownFieldsTagMask;
#endif
    container->class_data = class_data;
    return &(container->unknown_fields);
  }
#else
  template <typename T>
  PROTOBUF_NOINLINE T* mutable_unknown_fields_slow() {
    Arena* my_arena = arena();
    Container<T>* container = Arena::Create<Container<T>>(my_arena);
    // Two-step assignment works around a bug in clang's static analyzer:
    // https://bugs.llvm.org/show_bug.cgi?id=34198.
    ptr_ = reinterpret_cast<intptr_t>(container);
    ptr_ |= kUnknownFieldsTagMask;
    container->arena = my_arena;
    return &(container->unknown_fields);
  }
#endif

  // Templated functions.

  template <typename T>
  PROTOBUF_NOINLINE void DoClear(
#if defined(PROTOBUF_CUSTOM_VTABLE)
      google::protobuf::Arena* arena
#endif
  ) {
    mutable_unknown_fields<T>(
#if defined(PROTOBUF_CUSTOM_VTABLE)
        arena
#endif
        )
        ->Clear();
  }

  template <typename T>
  PROTOBUF_NOINLINE void DoMergeFrom(
#if defined(PROTOBUF_CUSTOM_VTABLE)
      google::protobuf::Arena* arena,
#endif
      const T& other) {
    mutable_unknown_fields<T>(
#if defined(PROTOBUF_CUSTOM_VTABLE)
        arena
#endif
        )
        ->MergeFrom(other);
  }

  template <typename T>
  PROTOBUF_NOINLINE void DoSwap(
#if defined(PROTOBUF_CUSTOM_VTABLE)
      google::protobuf::Arena* arena,
#endif
      T* other) {
    mutable_unknown_fields<T>(
#if defined(PROTOBUF_CUSTOM_VTABLE)
        arena
#endif
        )
        ->Swap(other);
  }

  // Private helper with debug checks for ~InternalMetadata()
  void CheckedDestruct();
};

// String Template specializations.

template <>
PROTOBUF_EXPORT void InternalMetadata::DoClear<std::string>(
#if defined(PROTOBUF_CUSTOM_VTABLE)
    google::protobuf::Arena* arena
#endif
);
template <>
PROTOBUF_EXPORT void InternalMetadata::DoMergeFrom<std::string>(
#if defined(PROTOBUF_CUSTOM_VTABLE)
    google::protobuf::Arena* arena,
#endif
    const std::string& other);
template <>
PROTOBUF_EXPORT void InternalMetadata::DoSwap<std::string>(
#if defined(PROTOBUF_CUSTOM_VTABLE)
    google::protobuf::Arena* arena,
#endif
    std::string* other);

// Instantiated once in message.cc (where the definition of UnknownFieldSet is
// known) to prevent much duplication across translation units of a large build.
extern template PROTOBUF_EXPORT void InternalMetadata::DoClear<UnknownFieldSet>(
#if defined(PROTOBUF_CUSTOM_VTABLE)
    google::protobuf::Arena* arena
#endif
);
extern template PROTOBUF_EXPORT void
InternalMetadata::DoMergeFrom<UnknownFieldSet>(
#if defined(PROTOBUF_CUSTOM_VTABLE)
    google::protobuf::Arena* arena,
#endif
    const UnknownFieldSet& other);
extern template PROTOBUF_EXPORT void InternalMetadata::DoSwap<UnknownFieldSet>(
#if defined(PROTOBUF_CUSTOM_VTABLE)
    google::protobuf::Arena* arena,
#endif
    UnknownFieldSet* other);
extern template PROTOBUF_EXPORT void
InternalMetadata::DeleteOutOfLineHelper<UnknownFieldSet>();
extern template PROTOBUF_EXPORT UnknownFieldSet*
#if defined(PROTOBUF_CUSTOM_VTABLE)
InternalMetadata::mutable_unknown_fields_slow<UnknownFieldSet>(
    google::protobuf::Arena* arena);
#else
InternalMetadata::mutable_unknown_fields_slow<UnknownFieldSet>();
#endif

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
#if defined(PROTOBUF_CUSTOM_VTABLE)
  LiteUnknownFieldSetter(InternalMetadata* metadata, google::protobuf::Arena* arena)
      : metadata_(metadata), arena_(arena) {
    if (metadata->have_unknown_fields()) {
      buffer_.swap(*metadata->mutable_unknown_fields<std::string>(arena_));
    }
  }
  ~LiteUnknownFieldSetter() {
    if (!buffer_.empty())
      metadata_->mutable_unknown_fields<std::string>(arena_)->swap(buffer_);
  }
#else
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
#endif

  std::string* buffer() { return &buffer_; }

 private:
  InternalMetadata* metadata_;
#if defined(PROTOBUF_CUSTOM_VTABLE)
  google::protobuf::Arena* arena_;
#endif
  std::string buffer_;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_METADATA_LITE_H__
