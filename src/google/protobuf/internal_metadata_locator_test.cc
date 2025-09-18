#include "google/protobuf/internal_metadata_locator.h"

#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>
#include "google/protobuf/arena.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/unittest.pb.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google::protobuf::internal {
namespace {

// Since the `MoreString` message has only one field, the offset of the field is
// sizeof(MessageLite) + sizeof(void*) for hasbits.
static constexpr size_t kMoreStringFieldOffset =
    sizeof(MessageLite) + sizeof(void*);

#ifdef PROTOBUF_CUSTOM_VTABLE
static constexpr size_t kMoreStringInternalMetadataOffset = 0;
#else
static constexpr size_t kMoreStringInternalMetadataOffset = sizeof(void*);
#endif

struct FieldWithInternalMetadataOffset {
  explicit FieldWithInternalMetadataOffset(InternalMetadataOffset offset)
      : resolver(offset) {}

  int field = 0;
  InternalMetadataResolver resolver;
};

struct StructWithInternalMetadata {
  StructWithInternalMetadata() = default;
  explicit StructWithInternalMetadata(Arena* arena)
      : _internal_metadata_(arena),
        field(InternalMetadataOffset::Build<StructWithInternalMetadata,
                                            offsetof(StructWithInternalMetadata,
                                                     field)>()) {}

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
                                    offsetof(StructWithInternalMetadata,
                                             field)>();
  EXPECT_FALSE(offset.IsSentinel());
  EXPECT_EQ(offset.Offset(), -static_cast<int32_t>(sizeof(void*)));
}

TEST(InternalMetadataLocatorTest, BuildFromStaticOffsetForProtoMessage) {
  constexpr auto offset =
      InternalMetadataOffset::Build<proto2_unittest::MoreString,
                                    kMoreStringFieldOffset>();
  EXPECT_FALSE(offset.IsSentinel());
  EXPECT_EQ(offset.Offset(),
            -static_cast<int32_t>(kMoreStringFieldOffset -
                                  kMoreStringInternalMetadataOffset));
}

TEST(InternalMetadataLocatorTest, ReadArenaFromInternalMetadata) {
  Arena arena;
  StructWithInternalMetadata message(&arena);
  const auto* field = &message.field;
  EXPECT_EQ((ResolveArena<&FieldWithInternalMetadataOffset::resolver>(field)),
            &arena);
}

}  // namespace
}  // namespace protobuf
}  // namespace google::internal

#include "google/protobuf/port_undef.inc"
