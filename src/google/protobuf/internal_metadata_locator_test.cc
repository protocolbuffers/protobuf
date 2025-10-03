#include "google/protobuf/internal_metadata_locator.h"

#include <cstddef>
#include <cstdint>

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

  InternalMetadata _internal_metadata_;
  FieldWithInternalMetadataOffset field;
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
  const auto* field = &message.field;
  EXPECT_EQ((ResolveArena<&FieldWithInternalMetadataOffset::resolver>(field)),
            &arena);
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
