// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_METADATA_LITE_H__
#define GOOGLE_PROTOBUF_METADATA_LITE_H__

#include <string>

#include "absl/base/optimization.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/port.h"

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
  constexpr InternalMetadata() : ptr_(0) {}
  explicit InternalMetadata(Arena* arena) {
    ptr_ = reinterpret_cast<intptr_t>(arena);
  }

  // Delete will delete the unknown fields only if they weren't allocated on an
  // arena.  Then it updates the flags so that if you call
  // have_unknown_fields(), it will return false.
  //
  // It is designed to be used as part of a Message class's destructor call, so
  // that when control eventually gets to ~InternalMetadata(), we don't need to
  // check for have_unknown_fields() again.
  void Delete() {
    // Note that Delete<> should be called not more than once.
    if (have_unknown_fields()) {
      vtable()->delete_instance(this);
    }
  }

  PROTOBUF_NDEBUG_INLINE Arena* arena() const {
    if (ABSL_PREDICT_FALSE(have_unknown_fields())) {
      return PtrValue<ContainerBase>()->arena;
    } else {
      return PtrValue<Arena>();
    }
  }

  PROTOBUF_NDEBUG_INLINE bool have_unknown_fields() const {
    return HasUnknownFieldsTag();
  }

  PROTOBUF_NDEBUG_INLINE void* raw_arena_ptr() const {
    return reinterpret_cast<void*>(ptr_);
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

  PROTOBUF_NDEBUG_INLINE void InternalSwap(
      InternalMetadata* PROTOBUF_RESTRICT other) {
    std::swap(ptr_, other->ptr_);
  }

  PROTOBUF_NDEBUG_INLINE void MergeFrom(const InternalMetadata& other) {
    if (other.have_unknown_fields()) {
      // Use other.vtable because we might not have one yet.
      other.vtable()->merge_from(this, other);
    }
  }

  PROTOBUF_NDEBUG_INLINE void Clear() {
    if (have_unknown_fields()) {
      vtable()->clear(this);
    }
  }

 private:
  intptr_t ptr_;

  // Tagged pointer implementation.
  static constexpr intptr_t kUnknownFieldsTagMask = 1;
  static constexpr intptr_t kPtrTagMask = kUnknownFieldsTagMask;
  static constexpr intptr_t kPtrValueMask = ~kPtrTagMask;

  // Accessors for pointer tag and pointer value.
  PROTOBUF_ALWAYS_INLINE bool HasUnknownFieldsTag() const {
    return ptr_ & kUnknownFieldsTagMask;
  }

  template <typename U>
  U* PtrValue() const {
    return reinterpret_cast<U*>(ptr_ & kPtrValueMask);
  }

  // If ptr_'s tag is kTagContainer, it points to an instance of this struct.
  struct VTable {
    void (*delete_instance)(InternalMetadata*);
    void (*clear)(InternalMetadata*);
    void (*merge_from)(InternalMetadata*, const InternalMetadata&);
  };

  struct ContainerBase {
    Arena* arena;
    const VTable* vtable;
  };

  const VTable* vtable() const {
    ABSL_DCHECK(have_unknown_fields());
    return PtrValue<ContainerBase>()->vtable;
  }

  template <typename T>
  static void DeleteImpl(InternalMetadata* m) {
    delete m->PtrValue<Container<T>>();
    // TODO:  This store is load-bearing.  Since we are
    // destructing the message at this point, see if we can eliminate it.
    m->ptr_ = 0;
  }

  template <typename T>
  static void ClearImpl(InternalMetadata* m);

  template <typename T>
  static void MergeFromImpl(InternalMetadata* m, const InternalMetadata& from);

  template <typename T>
  static inline constexpr VTable kVTable = {
      DeleteImpl<T>,
      ClearImpl<T>,
      MergeFromImpl<T>,
  };

  template <typename T>
  struct Container : public ContainerBase {
    T unknown_fields;
  };

  template <typename T>
  PROTOBUF_NOINLINE T* mutable_unknown_fields_slow() {
    Arena* my_arena = arena();
    Container<T>* container = Arena::Create<Container<T>>(my_arena);
    // Two-step assignment works around a bug in clang's static analyzer:
    // https://bugs.llvm.org/show_bug.cgi?id=34198.
    ptr_ = reinterpret_cast<intptr_t>(container);
    ptr_ |= kUnknownFieldsTagMask;
    container->arena = my_arena;
    container->vtable = &kVTable<T>;
    return &(container->unknown_fields);
  }

  // Private helper with debug checks for ~InternalMetadata()
  void CheckedDestruct();
};

// Instantiated once in message_lite.cc to prevent much duplication across
// translation units of a large build.
template <>
PROTOBUF_EXPORT void InternalMetadata::ClearImpl<std::string>(
    InternalMetadata*);
template <>
PROTOBUF_EXPORT void InternalMetadata::MergeFromImpl<std::string>(
    InternalMetadata*, const InternalMetadata&);
extern template PROTOBUF_EXPORT const InternalMetadata::VTable
    InternalMetadata::kVTable<std::string>;

// Instantiated once in message.cc (where the definition of UnknownFieldSet is
// known) to prevent much duplication across translation units of a large build.
template <>
PROTOBUF_EXPORT void InternalMetadata::ClearImpl<UnknownFieldSet>(
    InternalMetadata*);
template <>
PROTOBUF_EXPORT void InternalMetadata::MergeFromImpl<UnknownFieldSet>(
    InternalMetadata*, const InternalMetadata&);
extern template PROTOBUF_EXPORT const InternalMetadata::VTable
    InternalMetadata::kVTable<UnknownFieldSet>;

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
