#ifndef GOOGLE_PROTOBUF_FIELD_WITH_ARENA_H__
#define GOOGLE_PROTOBUF_FIELD_WITH_ARENA_H__

#include <cstddef>

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
//
// Note that `FieldWithArena<T>` is destructor-skippable if and only if `T` is
// destructor-skippable.
template <typename T>
class FieldWithArena : public ContainerDestructorSkippableBase<T> {
 public:
  using InternalArenaConstructable_ = void;

  constexpr FieldWithArena() : field_() {}

  template <typename... Args>
  explicit FieldWithArena(Arena* arena, Args&&... args)
      : _internal_metadata_(arena) {
    StaticallyVerifyLayout();
    // Construct `T` after setting `_internal_metadata_` so that `T` can safely
    // call ResolveArena().
    new (&field_) T(BuildOffset(), std::forward<Args>(args)...);
  }

  ~FieldWithArena() {
    // The destructor of `FieldWithArena` for destructor-skippable types must
    // only be called if the field is not allocated on an arena.
    if constexpr (Arena::is_destructor_skippable<T>::value) {
      ABSL_DCHECK_EQ(GetArena(), nullptr);
    }
    field_.~T();
  }

  const T& field() const { return field_; }
  T& field() { return field_; }

  // Returns the arena that the field is allocated on. This is cheaper than
  // calling `field().GetArena()`.
  Arena* GetArena() const { return _internal_metadata_.arena(); }

 private:
  friend InternalMetadataOffset;

  static constexpr InternalMetadataOffset BuildOffset();

  // A method to statically verify the offset of `field_storage_`. We need to
  // define this in a member function out of line because `FieldWithArena` needs
  // to be fully defined to call `offsetof`, but `field_storage_` is private.
  static constexpr void StaticallyVerifyLayout();

  // Generated code sometimes doesn't have a complete type for `T` (for example,
  // split repeated message fields). When the constructor of `FieldWithArena` is
  // invoked, both the constructor and destructor of all members are pulled in.
  // By placing the field in a union, we manually invoke its destructor and can
  // prevent it from being pulled in except in the `Destroy()` method.
  union {
    T field_;
  };

  // Note that the name of this field must be `_internal_metadata_`, as
  // `InternalMetadataOffset` expects a field with this name.
  const InternalMetadata _internal_metadata_;
};

template <typename T>
constexpr InternalMetadataOffset FieldWithArena<T>::BuildOffset() {
  return InternalMetadataOffset::Build<FieldWithArena,
                                       offsetof(FieldWithArena, field_)>();
}

template <typename Element>
constexpr void FieldWithArena<Element>::StaticallyVerifyLayout() {
  static_assert(
      offsetof(FieldWithArena, field_) == 0,
      "field_ must be at offset 0 in FieldWithArena. There are multiple places "
      "throughout the code (e.g. reflection, VerifyHasBitConsistency) which "
      "assume that you can find the wrapped field by interpreting a pointer as "
      "the wrapped field type, and aren't aware of this wrapper class. By "
      "placing `field_` at offset 0 in this struct, this assumption holds.");
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_FIELD_WITH_ARENA_H__
