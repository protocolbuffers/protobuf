#include "google/protobuf/internal_metadata_locator.h"

#include <cstddef>
#include <cstdint>
#include <utility>

#include <gtest/gtest.h>
#include "google/protobuf/arena.h"
#include "google/protobuf/internal_metadata_locator_test.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/metadata_lite.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
namespace {

// Since the `TestOneRepeatedField` message has only one field, the offset of
// the field is sizeof(MessageLite) + sizeof(void*) for hasbits.
static constexpr size_t kTestOneRepeatedFieldFieldOffset =
    sizeof(MessageLite) + sizeof(void*);

#ifdef PROTOBUF_CUSTOM_VTABLE
static constexpr size_t kTestOneRepeatedFieldInternalMetadataOffset = 0;
#else
static constexpr size_t kTestOneRepeatedFieldInternalMetadataOffset =
    sizeof(void*);
#endif

struct FieldWithInternalMetadataOffset {
  explicit FieldWithInternalMetadataOffset(InternalMetadataOffset offset)
      : resolver(offset) {}

  int field = 0;
  InternalMetadataResolver resolver;
};

struct StructWithInternalMetadata {
  explicit StructWithInternalMetadata(Arena* arena)
      : _internal_metadata_(arena),
        field(InternalMetadataOffset::Build<
              StructWithInternalMetadata,
              PROTOBUF_FIELD_OFFSET(StructWithInternalMetadata, field)>()) {}

  Arena* GetArena() const {
    return ResolveArena<&FieldWithInternalMetadataOffset::resolver>(&field);
  }

  InternalMetadata _internal_metadata_;
  FieldWithInternalMetadataOffset field;
};

template <uint32_t kTaggedBits>
struct TaggedFieldWithInternalMetadataOffset {
  constexpr TaggedFieldWithInternalMetadataOffset() = default;
  TaggedFieldWithInternalMetadataOffset(InternalMetadataOffset offset,
                                        int value)
      : field(value), resolver(offset) {}

  uint32_t Tag() const { return resolver.Tag(); }
  void SetTag(uint32_t tag) { resolver.SetTag(tag); }

  void Swap(TaggedFieldWithInternalMetadataOffset<kTaggedBits>& other) {
    std::swap(field, other.field);
    resolver.SwapTags(other.resolver);
  }

  int field = 0;
  TaggedInternalMetadataResolver<kTaggedBits> resolver;
};

template <uint32_t kTaggedBits, size_t kPadding = 8>
struct TaggedStructWithInternalMetadata {
  constexpr TaggedStructWithInternalMetadata() = default;
  explicit TaggedStructWithInternalMetadata(Arena* arena)
      : TaggedStructWithInternalMetadata(arena, 0) {}
  TaggedStructWithInternalMetadata(Arena* arena, int value)
      : _internal_metadata_(arena),
        field(
            InternalMetadataOffset::Build<TaggedStructWithInternalMetadata,
                                          PROTOBUF_FIELD_OFFSET(
                                              TaggedStructWithInternalMetadata,
                                              field)>(),
            value) {}

  Arena* GetArena() const {
    return ResolveTaggedArena<
        &TaggedFieldWithInternalMetadataOffset<kTaggedBits>::resolver,
        kTaggedBits>(&field);
  }

  template <size_t kOtherPadding>
  void Swap(
      TaggedStructWithInternalMetadata<kTaggedBits, kOtherPadding>& other) {
    field.Swap(other.field);
  }

  InternalMetadata _internal_metadata_;
  // Add padding to the struct to manipulate the offset of the
  // `InternalMetadataResolver` in `field`.
  uint8_t padding[kPadding] = {};
  TaggedFieldWithInternalMetadataOffset<kTaggedBits> field;
};

TEST(InternalMetadataLocatorTest, Sentinel) {
  constexpr InternalMetadataOffset offset;
  EXPECT_TRUE(offset.IsSentinel());
}

TEST(InternalMetadataLocatorTest, BuildFromStaticOffset) {
  constexpr auto offset =
      InternalMetadataOffset::Build<StructWithInternalMetadata,
                                    PROTOBUF_FIELD_OFFSET(
                                        StructWithInternalMetadata, field)>();
  EXPECT_FALSE(offset.IsSentinel());
  EXPECT_EQ(offset.Offset(), -static_cast<int32_t>(sizeof(void*)));
}

TEST(InternalMetadataLocatorTest, BuildFromStaticOffsetForProtoMessage) {
  constexpr auto offset =
      InternalMetadataOffset::Build<proto2_unittest::TestOneRepeatedField,
                                    kTestOneRepeatedFieldFieldOffset>();
  EXPECT_FALSE(offset.IsSentinel());
  EXPECT_EQ(offset.Offset(),
            -static_cast<int32_t>(kTestOneRepeatedFieldFieldOffset -
                                  kTestOneRepeatedFieldInternalMetadataOffset));
}

TEST(InternalMetadataLocatorTest, ReadArenaFromInternalMetadata) {
  Arena arena;
  StructWithInternalMetadata message(&arena);
  EXPECT_EQ(message.GetArena(), &arena);
}

TEST(TaggedInternalMetadataLocatorTest, ReadTag) {
  static constexpr uint32_t kTaggedBits = 1;
  TaggedStructWithInternalMetadata<kTaggedBits> message;

  EXPECT_EQ(message.GetArena(), nullptr);
  EXPECT_EQ(message.field.Tag(), 0);

  message.field.SetTag(1);
  EXPECT_EQ(message.GetArena(), nullptr);
  EXPECT_EQ(message.field.Tag(), 1);
}

TEST(TaggedInternalMetadataLocatorTest, ReadTagWithArena) {
  static constexpr uint32_t kTaggedBits = 1;
  Arena arena;
  TaggedStructWithInternalMetadata<kTaggedBits> message(&arena);

  EXPECT_EQ(message.GetArena(), &arena);
  EXPECT_EQ(message.field.Tag(), 0);

  message.field.SetTag(1);
  EXPECT_EQ(message.GetArena(), &arena);
  EXPECT_EQ(message.field.Tag(), 1);
}

TEST(TaggedInternalMetadataLocatorTest, Swap) {
  static constexpr uint32_t kTaggedBits = 2;

  Arena arena1, arena2;
  // Use different amounts of padding to ensure that the swap works correctly
  // when the internal metadata offsets are different.
  TaggedStructWithInternalMetadata<kTaggedBits, /*kPadding=*/16> container1(
      &arena1, 10);
  TaggedStructWithInternalMetadata<kTaggedBits, /*kPadding=*/32> container2(
      &arena2, 20);

  container1.field.SetTag(1);
  container2.field.SetTag(2);

  // Verify the contents of both fields are correct before the swap.
  EXPECT_EQ(container1.GetArena(), &arena1);
  EXPECT_EQ(container1.field.field, 10);
  EXPECT_EQ(container1.field.Tag(), 1);

  EXPECT_EQ(container2.GetArena(), &arena2);
  EXPECT_EQ(container2.field.field, 20);
  EXPECT_EQ(container2.field.Tag(), 2);

  // Swap the containers. This should swap everything except the arenas and the
  // offsets. If the offsets were not swapped correctly, calling `GetArena()`
  // should crash or return the wrong value.
  container1.Swap(container2);
  EXPECT_EQ(container1.GetArena(), &arena1);
  EXPECT_EQ(container1.field.field, 20);
  EXPECT_EQ(container1.field.Tag(), 2);

  EXPECT_EQ(container2.GetArena(), &arena2);
  EXPECT_EQ(container2.field.field, 10);
  EXPECT_EQ(container2.field.Tag(), 1);
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
