#ifndef GOOGLE_PROTOBUF_INTERNAL_METADATA_LOCATOR_H__
#define GOOGLE_PROTOBUF_INTERNAL_METADATA_LOCATOR_H__

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/metadata_lite.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

// A wrapper around the offset to internal metadata from the address of another
// field in the same class/struct. This is used to reduce the size of fields
// that need access to an Arena which can be found in the containing object.
class InternalMetadataOffset {
  // The offset to arena to use when there is no arena.
  static constexpr int32_t kSentinelInternalMetadataOffset = 0;

 public:
  // A sentinel `InternalMetadataOffset`, which does not point to any metadata.
  constexpr PROTOBUF_ALWAYS_INLINE InternalMetadataOffset() = default;

  // Constructs an `InternalMetadataOffset` which can recover the
  // `InternalMetadata` from a containing type `T` given the starting address of
  // the field at offset `FieldOffset` within `T`.
  //
  // This method expects to find a field with name `_internal_metadata_` in `T`,
  // and the type of that field should be `InternalMetadata`.
  template <typename T, size_t kFieldOffset>
  static constexpr PROTOBUF_ALWAYS_INLINE InternalMetadataOffset Build() {
    static_assert(
        std::is_same_v<std::remove_const_t<decltype(T::_internal_metadata_)>,
                       InternalMetadata>,
        "Field `_internal_metadata_ is not of type `InternalMetadata`");

    constexpr int64_t kInternalMetadataOffset =
        static_cast<int64_t>(PROTOBUF_FIELD_OFFSET(T, _internal_metadata_));

    static_assert(
        kInternalMetadataOffset - static_cast<int64_t>(kFieldOffset) >=
            int64_t{INT32_MIN},
        "Offset from `_internal_metadata_` is underflowing an int32_t, "
        "likely meaning your message body is too large.");
    static_assert(
        kInternalMetadataOffset - static_cast<int64_t>(kFieldOffset) <=
            int64_t{INT32_MAX},
        "Offset from `_internal_metadata_` is overflowing an int32_t, "
        "likely meaning your message body is too large.");

    return InternalMetadataOffset(
        static_cast<int32_t>(kInternalMetadataOffset - kFieldOffset));
  }

  // Builds an `InternalMetadataOffset` from a dynamic offset from the start of
  // `T`. This is used by `DynamicMessage` to build an `InternalMetadataOffset`
  // for a field at a given runtime-derived offset from the start of the
  // message.
  //
  // This function performs runtime checks to ensure that the offset from
  // `_internal_metadata_` to the field is within the range of an int32_t. This
  // is necessary to prevent integer overflow when calculating the offset.
  template <typename T>
  static InternalMetadataOffset BuildFromDynamicOffset(size_t field_offset) {
    static_assert(
        std::is_base_of_v<MessageLite, T>,
        "BuildFromDynamicOffset can only be used for `DynamicMessage`");

    constexpr int64_t kInternalMetadataOffset =
        static_cast<int64_t>(PROTOBUF_FIELD_OFFSET(T, _internal_metadata_));

    ABSL_DCHECK_GE(kInternalMetadataOffset - static_cast<int64_t>(field_offset),
                   int64_t{INT32_MIN})
        << "Offset from `_internal_metadata_` to the field at offset "
        << field_offset
        << " is underflowing an int32_t, likely meaning your message body is "
           "too large.";
    ABSL_DCHECK_LE(kInternalMetadataOffset - static_cast<int64_t>(field_offset),
                   int64_t{INT32_MAX})
        << "Offset from `_internal_metadata_` to the field at offset "
        << field_offset
        << " is overflowing an int32_t, likely meaning your message body is "
           "too large.";

    return InternalMetadataOffset(
        static_cast<int32_t>(kInternalMetadataOffset - field_offset));
  }

  // Translates an offset relative to some class `T` to an offset relative to
  // the member at offset `kMemberOffset` within `T`. This is used when passing
  // `InternalMetadataOffset`s to members of a class where the offset was
  // constructed relative to the start of `T`.
  //
  // For example, here is how you would pass an `InternalMetadataOffset` to a
  // member `Baz` of a class `Bar`, which itself is a member of `Foo`.
  //
  // ```cc
  // struct Baz {
  //   int some_value;
  //   InternalMetadataResolver resolver;
  //
  //   Baz(int value, InternalMetadataOffset offset)
  //       : some_value(value), resolver(offset) {}
  // };
  //
  // struct Bar {
  //   int some_value;
  //   Baz baz;
  //
  //   Bar(int value, InternalMetadataOffset offset)
  //       : some_value(value),
  //         baz(2 * value, offset.TranslateForMember<offsetof(Bar, baz)>()) {}
  // };
  //
  // struct Foo {
  //   InternalMetadata _internal_metadata_;
  //   Bar field1;
  //
  //   explicit Foo(Arena* arena)
  //       : _internal_metadata_(arena),
  //         field1(123,
  //                InternalMetadataOffset::Build<Foo, PROTOBUF_FIELD_OFFSET(
  //                                                       Foo, field1)>()) {}
  // };
  // ```
  template <size_t kMemberOffset>
  constexpr InternalMetadataOffset TranslateForMember() const {
    if (IsSentinel()) {
      return InternalMetadataOffset();
    }
    return InternalMetadataOffset(offset_ -
                                  static_cast<int32_t>(kMemberOffset));
  }

  // If true, this `InternalMetadataOffset` does not point to any metadata.
  constexpr bool IsSentinel() const {
    return offset_ == kSentinelInternalMetadataOffset;
  }

  // The offset from the start of the field to the internal metadata of the
  // containing type (either a `MessageLite` or some other internal class, like
  // `RepeatedPtrFieldWithArena`).
  constexpr int32_t Offset() const { return offset_; }

 private:
  // A private constructor for non-sentinel offsets which can only be called
  // from the static build methods.
  explicit constexpr InternalMetadataOffset(int32_t offset) : offset_(offset) {}

  int32_t offset_ = kSentinelInternalMetadataOffset;
};

// A class which can recover the `InternalMetadata` field from a containing type
// given a pointer to another field contained by that type.
template <uint32_t kTaggedBits>
class TaggedInternalMetadataResolver {
 public:
  static_assert(kTaggedBits < std::numeric_limits<uint32_t>::digits);
  static constexpr uint32_t kTagMask = (uint32_t{1} << kTaggedBits) - 1;

  // Builds an `InternalMetadataResolver` which points to no metadata.
  constexpr TaggedInternalMetadataResolver() = default;

  constexpr explicit TaggedInternalMetadataResolver(
      InternalMetadataOffset offset)
      : offset_(static_cast<uint32_t>(offset.Offset())) {
    ABSL_DCHECK_EQ(offset_ & kTagMask, uint32_t{0});
  }

  constexpr int32_t Offset() const {
    return static_cast<int32_t>(offset_ & ~kTagMask);
  }

  constexpr void SetTag(uint32_t tag) {
    ABSL_DCHECK_EQ(tag & ~kTagMask, uint32_t{0});
    offset_ = (offset_ & ~kTagMask) | tag;
  }

  constexpr uint32_t Tag() const { return offset_ & kTagMask; }

  // Swaps only the tags of the two resolvers, leaving their offsets unchanged.
  void SwapTags(TaggedInternalMetadataResolver& other) {
    const uint32_t swap_tag = Tag() ^ other.Tag();
    offset_ ^= swap_tag;
    other.offset_ ^= swap_tag;
  }

 private:
  template <auto Resolver, typename T>
  friend inline Arena* ResolveArena(const T* object);
  template <auto Resolver, uint32_t kTaggedBits_, typename T>
  friend inline Arena* ResolveTaggedArena(const T* object);

  // Finds the `Arena*` from the `InternalMetadata` of the containing type given
  // the `this` pointer to the field contained by that type.
  template <typename T,
            TaggedInternalMetadataResolver<kTaggedBits> T::* Resolver>
  static inline Arena* FindArena(const T* object) {
    auto& resolver = object->*Resolver;
    if (resolver.Offset() == 0) {
      return nullptr;
    }
    return resolver.FindInternalMetadata(object).arena();
  }

  // Finds the `InternalMetadata` by adding the offset to the address of the
  // start of the field.
  inline const InternalMetadata& FindInternalMetadata(
      const void* object) const {
    ABSL_DCHECK_NE(Offset(), 0);
    return *reinterpret_cast<const InternalMetadata*>(
        reinterpret_cast<const char*>(object) + Offset());
  }

  uint32_t offset_ = InternalMetadataOffset().Offset();
};

using InternalMetadataResolver = TaggedInternalMetadataResolver<0>;

// Resolves an `Arena*` from the `InternalMetadata` of a containing type (which
// has a member `InternalMetadata _internal_metadata_`) given a reference to a
// field of type `T` contained by that type.
//
// The template parameter `Resolver` is a pointer-to-member to the
// `InternalMetadataResolver` field of `object`.
//
// `object` must have been constructed by the containing type, which is
// responsible for correctly constructing the `InternalMetadataOffset` for
// `object`.
//
// This function exists as a standalone function and not a member of
// `InternalMetadataResolver` because the offset must be computed relative to
// the address of the field containing the resolver, not the resolver itself.
// This pattern is easy to get wrong from the caller, so we force callers to
// give a pointer-to-member to the resolver as a type argument, then require
// that the pointer passed to `ResolveArena` is of the containing type of the
// resolver field. With the pointer-to-member type, we can load the resolver
// directly from the passed object, thereby ensuring we are using the correct
// offset for the object.
//
// Example usage:
//
// ```cc
// struct Bar {
//   int some_value;
//   InternalMetadataResolver resolver;
//
//   Bar(int value, InternalMetadataOffset offset)
//       : some_value(value), resolver(offset) {}
//
//   Arena* GetArena() const {
//     return ResolveArena<&Bar::resolver>(this);
//   }
// };
//
// struct Foo {
//   InternalMetadata _internal_metadata_;
//   Bar field1;
//
//   Foo(Arena* arena)
//       : _internal_metadata_(arena),
//         field1(123,
//                    InternalMetadataOffset::Build<Foo, PROTOBUF_FIELD_OFFSET(
//                                                           Foo, field1)>()) {}
// };
// ```
template <auto Resolver, typename T>
inline Arena* ResolveArena(const T* object) {
  return InternalMetadataResolver::FindArena<T, Resolver>(object);
}

template <auto Resolver, uint32_t kTaggedBits, typename T>
inline Arena* ResolveTaggedArena(const T* object) {
  return TaggedInternalMetadataResolver<kTaggedBits>::template FindArena<
      T, Resolver>(object);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_INTERNAL_METADATA_LOCATOR_H__
