#ifndef GOOGLE_PROTOBUF_FIELD_WITH_ARENA_H__
#define GOOGLE_PROTOBUF_FIELD_WITH_ARENA_H__

#include <cstddef>
#include <type_traits>

#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arena_cleanup.h"
#include "google/protobuf/internal_metadata_locator.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/metadata_lite.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

class MapFieldBase;

template <typename T, bool Trivial = Arena::is_destructor_skippable<T>::value>
struct FieldWithArenaDestructorSkippableBase {
  static constexpr bool kDestructorSkippable = false;
};

template <typename T>
struct FieldWithArenaDestructorSkippableBase<T, true> {
  using DestructorSkippable_ = void;
  static constexpr bool kDestructorSkippable = true;
};

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
class FieldWithArena : public FieldWithArenaDestructorSkippableBase<T> {
 public:
  using InternalArenaConstructable_ = void;

  constexpr FieldWithArena() : field_() {}

  template <typename... Args>
  explicit FieldWithArena(Arena* arena, Args&&... args)
#ifdef PROTOBUF_INTERNAL_REMOVE_ARENA_PTRS_REPEATED_PTR_FIELD
      : _internal_metadata_(arena)
#endif
  {
    StaticallyVerifyLayout();
    // Construct `T` after setting `_internal_metadata_` so that `T` can safely
    // call ResolveArena().
#ifdef PROTOBUF_INTERNAL_REMOVE_ARENA_PTRS_REPEATED_PTR_FIELD
    new (&field_) T(kOffset, std::forward<Args>(args)...);
#else
    new (&field_) T(arena, std::forward<Args>(args)...);
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
  ~FieldWithArena() {}

  // Manually destroy the underlying `T`. This should be called only when a
  // `FieldWithArena` is being destroyed, and the associated arena is nullptr.
  template <typename U = T,
            std::enable_if_t<
                FieldWithArenaDestructorSkippableBase<U>::kDestructorSkippable,
                bool> = true>
  void Destroy() {
    ABSL_DCHECK_EQ(GetArena(), nullptr);
    field_.~T();
  }

  // Manually destroy the underlying `T`, but specialized for types that aren't
  // destructor-skippable on arenas (e.g. absl::Cord). This specialization does
  // not require that the arena is nullptr.
  template <typename U = T,
            std::enable_if_t<
                !FieldWithArenaDestructorSkippableBase<U>::kDestructorSkippable,
                bool> = true>
  void Destroy() {
    field_.~T();
  }

  const T& field() const { return field_; }
  T& field() { return field_; }

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

  // We can't have a plain member with type `T` here because generated code
  // sometimes doesn't have a complete type for `T` (for example, split repeated
  // message fields). When the constructor of `FieldWithArena` is invoked, both
  // the constructor and destructor of all members are pulled in. By placing the
  // field in a union, we manually invoke its destructor and can prevent it from
  // being pulled in except in the `Destroy()` method.
  union {
    T field_;
  };

#ifdef PROTOBUF_INTERNAL_REMOVE_ARENA_PTRS_REPEATED_PTR_FIELD
  const InternalMetadata _internal_metadata_;
#endif
};

#ifdef PROTOBUF_INTERNAL_REMOVE_ARENA_PTRS_REPEATED_PTR_FIELD
template <typename T>
constexpr internal::InternalMetadataOffset FieldWithArena<T>::kOffset =
    internal::InternalMetadataOffset::Build<
        FieldWithArena<T>, offsetof(FieldWithArena<T>, field_)>();
#endif

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

namespace cleanup {

template <typename T>
struct ArenaDestructObject<FieldWithArena<T>> {
  static void Destroy(void* object) {
    reinterpret_cast<FieldWithArena<T>*>(object)->Destroy();
  }
};

}  // namespace cleanup

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_FIELD_WITH_ARENA_H__
