#ifndef GOOGLE_PROTOBUF_FIELD_WITH_ARENA_H__
#define GOOGLE_PROTOBUF_FIELD_WITH_ARENA_H__

#include <cstddef>
#include <cstdint>

#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/internal_metadata_locator.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/metadata_lite.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

// A container that holds a `T` and an arena pointer, where `T` has an
// `InternalMetadataResolver` member. This is used for both directly
// arena-allocated `T`'s and split `T`'s. Both cases need to return the correct
// thing when a user calls `GetArena()` on `T`, or when internal code needs the
// arena to do memory allocation.
//
// This class is used to store `InternalMetadata` alongside `T` with an
// `InternalMetadataResolver`, since `InternalMetadataResolver`s can only point
// to an existing arena pointer "nearby" in memory.
template <typename T>
class FieldWithArena {
 public:
  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;

  FieldWithArena() : FieldWithArena(/*arena=*/nullptr) {}

  template <typename... Args>
  explicit FieldWithArena(Arena* arena, Args&&... args)
#ifdef PROTOBUF_INTERNAL_REMOVE_ARENA_PTRS_REPEATED_PTR_FIELD
      : _internal_metadata_(arena)
#endif
  {
    StaticallyVerifyLayout();
#ifdef PROTOBUF_INTERNAL_REMOVE_ARENA_PTRS_REPEATED_PTR_FIELD
    new (field_storage_) T(kOffset, std::forward<Args>(args)...);
#else
    new (field_storage_) T(arena, std::forward<Args>(args)...);
#endif
  }

  // The destructor of `FieldWithArena` can't call the destructor of `T` because
  // the destructor of `FieldWithArena` is pulled in when the constructor of any
  // subclass of `FieldWithArena` is invoked, and we would like to be able to
  // subclass `FieldWithArena` to add behavior for specific field types. We
  // expect that constructors of these subclasses may be called with incomplete
  // element types (e.g. for split repeated message fields), but `~T()` requires
  // `T` to be complete.
  //
  // Instead, we provide a `Destroy()` method that must be called explicitly
  // from the destructor of subclasses of `FieldWithArena`.
  ~FieldWithArena() = default;

  // Manually destroy the underlying `T`. This should be called only when a
  // `FieldWithArena` is being destroyed, and the associated arena is nullptr.
  void Destroy() {
    ABSL_DCHECK_EQ(GetArena(), nullptr);
    field().~T();
  }

  const T& field() const { return *reinterpret_cast<const T*>(field_storage_); }
  T& field() { return *reinterpret_cast<T*>(field_storage_); }

  // Returns the arena that the field is allocated on. This is cheaper than
  // calling `field().GetArena()`.
  Arena* GetArena() const {
#ifdef PROTOBUF_INTERNAL_REMOVE_ARENA_PTRS_REPEATED_PTR_FIELD
    return _internal_metadata_.arena();
#else
    return field().GetArena();
#endif
  }

 private:
  friend InternalMetadataOffset;

#ifdef PROTOBUF_INTERNAL_REMOVE_ARENA_PTRS_REPEATED_PTR_FIELD
  static const InternalMetadataOffset kOffset;
#endif

  // A method to statically verify the offset of `field_storage_`. We need to
  // define this in a member function out of line because `FieldWithArena` needs
  // to be fully defined to call `offsetof`, but `field_storage_` is private.
  [[maybe_unused]] static constexpr void StaticallyVerifyLayout();

  // We can't have a member with type `T` here because generated code sometimes
  // doesn't have a complete type for `T` (for example, split repeated message
  // fields). When the constructor of `FieldWithArena` is invoked, both the
  // constructor and destructor of all members are pulled in. By managing the
  // lifetime of `T` manually, we can avoid pulling in the destructor of `T`
  // except in the `Destroy()` method.
  alignas(T) uint8_t field_storage_[sizeof(T)];

#ifdef PROTOBUF_INTERNAL_REMOVE_ARENA_PTRS_REPEATED_PTR_FIELD
  const InternalMetadata _internal_metadata_;
#endif
};

#ifdef PROTOBUF_INTERNAL_REMOVE_ARENA_PTRS_REPEATED_PTR_FIELD
template <typename T>
constexpr internal::InternalMetadataOffset FieldWithArena<T>::kOffset =
    internal::InternalMetadataOffset::Build<
        FieldWithArena<T>, offsetof(FieldWithArena<T>, field_storage_)>();
#endif

template <typename Element>
constexpr void FieldWithArena<Element>::StaticallyVerifyLayout() {
  static_assert(
      offsetof(FieldWithArena, field_storage_) == 0,
      "field_storage_ must be at offset 0 in FieldWithArena. There are "
      "multiple places throughout the code (e.g. reflection, "
      "VerifyHasBitConsistency) which assume that you can find the wrapped "
      "field by interpreting a pointer as the wrapped field type, and aren't "
      "aware of this wrapper class. By placing `field_storage_` at offset 0 in "
      "this struct, this assumption holds.");
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_FIELD_WITH_ARENA_H__
