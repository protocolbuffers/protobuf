// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file contains code to support StringPiece fields (ctype=STRING_PIECE)
// in generated code, and should not be used directly by users.

#ifndef GOOGLE_PROTOBUF_STRING_PIECE_FIELD_SUPPORT_H__
#define GOOGLE_PROTOBUF_STRING_PIECE_FIELD_SUPPORT_H__

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>

#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/field_with_arena.h"
#include "google/protobuf/internal_metadata_locator.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/port.h"

// clang-format off
#include "google/protobuf/port_def.inc"
// clang-format on

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {
namespace io {
class CodedInputStream;  // coded_stream.h
}
class Arena;       // arena.h
class Reflection;  // message.h
namespace internal {
class RepeatedPtrFieldBase;  // repeated_ptr_field.h
}  // namespace internal
}  // namespace protobuf
}  // namespace google

namespace google {
namespace protobuf {
namespace internal {

// Fields with ctype=STRING_PIECE need to have an associated string object that
// they can store to when parsing from an input that doesn't support aliasing.
// We put them together in this here structure so that Reflection can read these
// fields given an offset.
//
// See also io::CodedInputStream::ReadStringPiece().
class PROTOBUF_EXPORT StringPieceField {
 public:
  constexpr StringPieceField() = default;

  constexpr explicit StringPieceField(InternalMetadataOffset offset)
      : resolver_(offset) {}

  StringPieceField(InternalMetadataOffset offset, Arena* arena,
                   const StringPieceField& rhs)
      : resolver_(offset) {
    CopyFromWithArena(arena, rhs.Get());
  }

  StringPieceField(InternalMetadataOffset offset, Arena* arena,
                   absl::string_view value)
      : resolver_(offset) {
    CopyFromWithArena(arena, value);
  }

  StringPieceField(const StringPieceField& rhs)
      : StringPieceField(InternalMetadataOffset(), /*arena=*/nullptr, rhs) {}
  StringPieceField& operator=(const StringPieceField&) = delete;
  StringPieceField(StringPieceField&& other) noexcept {
    // Let `other`'s dtor clean up our old data.
    Swap(GetArena(), &other, other.GetArena());
  }
  StringPieceField& operator=(StringPieceField&& other) noexcept {
    // Let `other`'s dtor clean up our old data.
    Swap(GetArena(), &other, other.GetArena());
    return *this;
  }

  ~StringPieceField();

  explicit StringPieceField(absl::string_view value) { CopyFrom(value); }

  // This constructor is only ever used to initialize to the default value,
  // so we want it to alias, because the default value string is part of the
  // descriptor and thus doesn't need to be copied.
  constexpr StringPieceField(absl::string_view value, ConstantInitialized)
      : data_(value.data()), size_(value.size()) {}

  constexpr StringPieceField(InternalMetadataOffset offset,
                             absl::string_view value, ConstantInitialized)
      : data_(value.data()), size_(value.size()), resolver_(offset) {}

  const char* data() const { return data_; }
  size_t size() const { return size_; }

  absl::string_view Get() const { return absl::string_view(data_, size_); }
  void Set(absl::string_view value) {
    data_ = value.data();
    size_ = value.size();
  }

  void CopyFrom(absl::string_view value);
  void CopyFrom(const absl::Cord& value);

  // These are only here so that RepeatedPtrField doesn't have to be
  // explicitly specialized for StringPieceField.
  void Clear() {
    size_ = 0;
    // To catch lifetime issues for StringPiece fields let's clear and release
    // the buffer.
    if (scratch_) {
#ifndef NDEBUG
      std::memset(scratch_, 0, scratch_size_);
#endif
      if (GetArena() == nullptr) {
        std::allocator<char>().deallocate(scratch_, scratch_size_);
        scratch_ = nullptr;
        scratch_size_ = 0;
      }
    }
  }

  // This is used to implement messages' Swap() method. The caller is
  // responsible for ensuring arena safety; the two fields must be on arenas
  // with equivalent lifetimes or else both must be on the heap.
  void UnsafeArenaSwap(StringPieceField* x) {
    using std::swap;  // enable ADL with fallback
    swap(data_, x->data_);
    swap(size_, x->size_);
    swap(scratch_, x->scratch_);
    swap(scratch_size_, x->scratch_size_);
  }

  // Performs a swap. Unlike the UnsafeArenaSwap() method above, this version is
  // safe to call even if the two fields are on arenas with different lifetimes
  // or if one is on an arena and the other is on the heap.
  void Swap(Arena* arena, StringPieceField* x, Arena* x_arena);

  // Returns (an estimate of) the number of bytes used.
  size_t SpaceUsedLong() const { return sizeof(*this) + scratch_size_; }

  // Returns (an estimate of) the number of bytes used, not including *this.
  size_t SpaceUsedExcludingSelfLong() const { return scratch_size_; }


  // Returns an empty StringPieceField.
  static const StringPieceField& default_instance();

  // Defined in parse_context.cc because that's where these are called.
  void Append(const char* ptr, int chunk_size);
  void ClearAndReserve(int size);

 private:
  void CopyFromWithArena(Arena* arena, absl::string_view value);
  void CopyFromWithArena(Arena* arena, const absl::Cord& value);

  // Either points at scratch_ or aliases the parse buffer.
  const char* data_ = nullptr;
  size_t size_ = 0;
  char* scratch_ = nullptr;
  size_t scratch_size_ = 0;
  InternalMetadataResolver resolver_;

  // When constructed in an Arena, we want our destructor to be skipped.
  friend class ::google::protobuf::Arena;

  // For calling the private CopyFromWithArena() method.
  friend class ::google::protobuf::Reflection;
  friend class ::google::protobuf::internal::RepeatedPtrFieldBase;

  typedef void DestructorSkippable_;
  typedef void InternalArenaConstructable_;

  // Accessor required by Arena::Create.
  ::google::protobuf::Arena* GetArena() const {
    return ResolveArena<&StringPieceField::resolver_>(this);
  }
};

using StringPieceFieldWithArena = FieldWithArena<StringPieceField>;

template <>
struct FieldArenaRep<StringPieceField> {
  using Type = StringPieceFieldWithArena;

  static StringPieceField* Get(Type* arena_rep) { return &arena_rep->field(); }
};

template <>
struct FieldArenaRep<const StringPieceField> {
  using Type = const StringPieceFieldWithArena;

  static const StringPieceField* Get(const Type* arena_rep) {
    return &arena_rep->field();
  }
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_STRING_PIECE_FIELD_SUPPORT_H__
