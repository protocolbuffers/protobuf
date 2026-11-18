// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdint>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_bases.h"
#include "google/protobuf/internal_metadata_locator.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/test_protos/message_padding_test.pb.h"
#include "google/protobuf/unittest.pb.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

// Can't use an anonymous namespace here due to brokenness of Tru64 compiler.
namespace cpp_unittest {


#if !defined(ABSL_CHECK_MESSAGE_SIZE)
#define ABSL_CHECK_MESSAGE_SIZE(t, expected)
#endif

// Mock structures to lock down the size of messages in a platform-independent
// way.  The commented sizes only apply when build with clang x86_64.
struct MockMessageBase {
  virtual ~MockMessageBase() = default;  // 8 bytes vtable
  void* internal_metadata;               // 8 bytes
};
ABSL_CHECK_MESSAGE_SIZE(MockMessageBase, 16);

struct MockZeroFieldsBase : public MockMessageBase {
  int cached_size;  // 4 bytes
  // + 4 bytes padding
};
ABSL_CHECK_MESSAGE_SIZE(MockZeroFieldsBase, 24);

struct MockExtensionSet {
  void* data;        // 8 bytes
};
ABSL_CHECK_MESSAGE_SIZE(MockExtensionSet, 8);

struct MockRepeatedPtrField {
  int current_size;  // 4 bytes
  int total_size;    // 4 bytes
  void* data;        // 8 bytes
};
ABSL_CHECK_MESSAGE_SIZE(MockRepeatedPtrField, 16);

struct MockRepeatedField {
  internal::InternalMetadataResolver resolver;  // 4 bytes
  int size;                                     // 4 bytes
  union {                                       // 8 bytes
    void* heap_rep;
    uint8_t soo_capacity[internal::kSooCapacityBytes];
  };
};
ABSL_CHECK_MESSAGE_SIZE(MockRepeatedField, 16);

TEST(GeneratedMessageTest, MockSizes) {
  // Consistency checks -- if these fail, the tests below will definitely fail.
  ABSL_CHECK_EQ(sizeof(MessageLite), sizeof(MockMessageBase));
  ABSL_CHECK_EQ(sizeof(Message), sizeof(MockMessageBase));
  ABSL_CHECK_EQ(sizeof(internal::ZeroFieldsBase), sizeof(MockZeroFieldsBase));
  ABSL_CHECK_EQ(sizeof(internal::ExtensionSet), sizeof(MockExtensionSet));
  ABSL_CHECK_EQ(sizeof(RepeatedPtrField<std::string>),
                sizeof(MockRepeatedPtrField));
  ABSL_CHECK_EQ(sizeof(RepeatedField<int>), sizeof(MockRepeatedField));
}

TEST(GeneratedMessageTest, EmptyMessageSize) {
  EXPECT_EQ(sizeof(proto2_unittest::TestEmptyMessage),
            sizeof(MockZeroFieldsBase));
}

TEST(GeneratedMessageTest, ReservedSize) {
  EXPECT_EQ(sizeof(proto2_unittest::TestReservedFields),
            sizeof(MockZeroFieldsBase));
}

TEST(GeneratedMessageTest, EmptyMessageWithExtensionsSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    MockExtensionSet extensions;                   // 8 bytes
    int cached_size;                               // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-4 bytes
    // + 0-4 bytes of padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 32);
  EXPECT_EQ(sizeof(proto2_unittest::TestEmptyMessageWithExtensions),
            sizeof(MockGenerated));
}

struct MockLazy {
  void* ptr;
};

TEST(GeneratedMessageTest, RecursiveMessageSize) {
  // TODO: remove once synthetic_pdproto lands.
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    void* a;                                       // 8 bytes
    int32_t i;                                     // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-4 bytes
    // + 0-4 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 40);

  struct MockGeneratedLazy : public MockMessageBase {  // 16 bytes
    int has_bits[1];                                   // 4 bytes
    int cached_size;                                   // 4 bytes
    MockLazy a;                                        // lazy, see above.
    int32_t i;                                         // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                      // 0-4 bytes
    // + 0-4 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGeneratedLazy, 32 + sizeof(MockLazy));

  struct MockGeneratedSplit : public MockMessageBase {  // 16 bytes
    int has_bits[1];                                    // 4 bytes
    int cached_size;                                    // 4 bytes
    void* split;                                        // 8 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                       // 0-4 bytes
    // + 0-4 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGeneratedSplit, 32);

#ifndef PROTOBUF_FORCE_SPLIT
  using Type = std::conditional_t<internal::ForceEagerlyVerifiedLazyInProtoc(),
                                  MockGeneratedLazy, MockGenerated>;
#else   // !PROTOBUF_FORCE_SPLIT
  using Type = MockGeneratedSplit;
#endif  // PROTOBUF_FORCE_SPLIT

  EXPECT_EQ(sizeof(proto2_unittest::TestRecursiveMessage), sizeof(Type));
}

TEST(GeneratedMessageTest, OneStringSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-4 bytes
                                                   // + 0-4 bytes padding
    void* data;                                    // 8 bytes
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 32);

  struct MockGeneratedInlinedString : public MockMessageBase {  // 16 bytes
    int has_bits[1];                                            // 4 bytes
    int cached_size;                                            // 4 bytes
    std::string data;              // sizeof(std::string)
    PROTOBUF_TSAN_DECLARE_MEMBER;  // 0-4 bytes
                                   // + 0-4 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGeneratedInlinedString, 48);

  using Type = std::conditional_t<internal::ForceInlineStringInProtoc(),
                                  MockGeneratedInlinedString, MockGenerated>;
  EXPECT_EQ(sizeof(proto2_unittest::OneString), sizeof(Type));
}

TEST(GeneratedMessageTest, MoreStringSize) {
  // TODO: remove once synthetic_pdproto lands.
#ifndef PROTOBUF_FORCE_SPLIT
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-4 bytes
                                                   // + 0-4 bytes padding
    MockRepeatedPtrField data;                     // 24 bytes
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 40);
  EXPECT_EQ(sizeof(proto2_unittest::MoreString), sizeof(MockGenerated));
#else   // !PROTOBUF_FORCE_SPLIT
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int cached_size;                               // 4 bytes
    void* split;                                   // 8 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-4 bytes
                                                   // + 0-4 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 32);
  EXPECT_EQ(sizeof(proto2_unittest::MoreString), sizeof(MockGenerated));
#endif  // PROTOBUF_FORCE_SPLIT
}

TEST(GeneratedMessageTest, Int32MessageSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-4 bytes
                                                   // + 0-4 bytes padding
    int32_t data;                                  // 4 bytes
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 32);
  EXPECT_EQ(sizeof(proto2_unittest::Int32Message), sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, Int64MessageSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-4 bytes
                                                   // + 0-4 bytes padding
    int64_t data;                                  // 8 bytes
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 32);
  EXPECT_EQ(sizeof(proto2_unittest::Int64Message), sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, BoolMessageSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-4 bytes
                                                   // + 0-4 bytes padding
    bool data;                                     // 1 byte
    // + 3 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 32);
  EXPECT_EQ(sizeof(proto2_unittest::BoolMessage), sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, OneofSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    void* foo;                                     // 8 bytes
    int cached_size;                               // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-4 bytes
                                                   // + 0-4 bytes padding
    uint32_t oneof_case[1];                        // 4 bytes
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 32);
  EXPECT_EQ(sizeof(proto2_unittest::TestOneof), sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, Oneof2Size) {
  // TODO: remove once synthetic_pdproto lands.
#ifndef PROTOBUF_FORCE_SPLIT
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-4 bytes
                                                   // + 0-4 bytes padding
    void* baz_string;                              // 8 bytes
    int32_t baz_int;                               // 4 bytes
                                                   // + 4 bytes padding
    void* foo;                                     // 8 bytes
    void* bar;                                     // 8 bytes
    uint32_t oneof_case[2];                        // 8 bytes
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 64);
#else   // !PROTOBUF_FORCE_SPLIT
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-4 bytes
                                                   // + 0-4 bytes padding
    void* split;                                   // 8 bytes
    void* foo;                                     // 8 bytes
    void* bar;                                     // 8 bytes
    uint32_t oneof_case[2];                        // 8 bytes
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 56);
#endif  // PROTOBUF_FORCE_SPLIT
  EXPECT_EQ(sizeof(proto2_unittest::TestOneof2), sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, FieldOrderingsSize) {
  // TODO: remove once synthetic_pdproto lands.
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    MockExtensionSet extensions;                   // 8 bytes
    void* my_string;                               // 8 bytes
    void* optional_nested_message;                 // 8 bytes
    int64_t my_int;                                // 8 bytes
    float my_float;                                // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-4 bytes
    // + 0-4 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 64);

  struct MockGeneratedExperiments : public MockMessageBase {  // 16 bytes
    int has_bits[1];                                          // 4 bytes
    int cached_size;                                          // 4 bytes
    MockExtensionSet extensions;                              // 8 bytes
    std::string my_string;             // sizeof(std::string)
    MockLazy optional_nested_message;  // lazy, see above.
    int64_t my_int;                    // 8 bytes
    float my_float;                    // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;      // 0-4 bytes
    // + 0-4 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGeneratedExperiments, 72 + sizeof(MockLazy));

  struct MockGeneratedSplit : public MockMessageBase {  // 16 bytes
    int has_bits[1];                                    // 4 bytes
    int cached_size;                                    // 4 bytes
    MockExtensionSet extensions;                        // 8 bytes
    void* split;                                        // 8 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                       // 0-4 bytes
    // + 0-4 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGeneratedSplit, 40);

#ifndef PROTOBUF_FORCE_SPLIT
  // Make sure both or none are on for this test.
  ASSERT_EQ(internal::ForceEagerlyVerifiedLazyInProtoc(),
            internal::ForceInlineStringInProtoc());
  using Type = std::conditional_t<internal::ForceInlineStringInProtoc(),
                                  MockGeneratedExperiments, MockGenerated>;
#else   // !PROTOBUF_FORCE_SPLIT
  using Type = MockGeneratedSplit;
#endif  // PROTOBUF_FORCE_SPLIT
  EXPECT_EQ(sizeof(proto2_unittest::TestFieldOrderings), sizeof(Type));
}

TEST(GeneratedMessageTest, TestMessageSize) {
  // We expect the message to contain (not in this order):
  // TODO: remove once synthetic_pdproto lands.
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    void* m4;                                      // 8 bytes
    int64_t m2;                                    // 8 bytes
    bool m1;                                       // 1 bytes
    bool m3;                                       // 1 bytes
                                                   // + 2 bytes padding
    int m5;                                        // 4 bytes
    int64_t m6;                                    // 8 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-4 bytes
                                                   // + 0-4 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 56);

  struct MockGeneratedInlinedString : public MockMessageBase {  // 16 bytes
    int has_bits[1];                                            // 4 bytes
    int cached_size;                                            // 4 bytes
    std::string m4;                // sizeof(std::string)
    int64_t m2;                    // 8 bytes
    bool m1;                       // 1 bytes
    bool m3;                       // 1 bytes
                                   // + 2 bytes padding
    int m5;                        // 4 bytes
    int64_t m6;                    // 8 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;  // 0-4 bytes
                                   // + 0-4 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGeneratedInlinedString, 72);

  struct MockGeneratedSplit : public MockMessageBase {  // 16 bytes
    int has_bits[1];                                    // 4 bytes
    int cached_size;                                    // 4 bytes
    void* split;                                        // 8 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                       // 0-4 bytes
                                                        // + 0-4 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGeneratedSplit, 32);
#ifndef PROTOBUF_FORCE_SPLIT
  using Type = std::conditional_t<internal::ForceInlineStringInProtoc(),
                                  MockGeneratedInlinedString, MockGenerated>;
#else   // !PROTOBUF_FORCE_SPLIT
  using Type = MockGeneratedSplit;
#endif  // PROTOBUF_FORCE_SPLIT
  EXPECT_EQ(sizeof(proto2_unittest::TestMessageSize), sizeof(Type));
}

TEST(GeneratedMessageTest, PackedTypesSize) {
  // TODO: remove once synthetic_pdproto lands.
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    MockRepeatedField packed_int32;                // 16 bytes
    MockRepeatedField packed_int64;                // 16 bytes
    MockRepeatedField packed_uint32;               // 16 bytes
    MockRepeatedField packed_uint64;               // 16 bytes
    MockRepeatedField packed_sint32;               // 16 bytes
    MockRepeatedField packed_sint64;               // 16 bytes
    MockRepeatedField packed_fixed32;              // 16 bytes
    MockRepeatedField packed_fixed64;              // 16 bytes
    MockRepeatedField packed_sfixed32;             // 16 bytes
    MockRepeatedField packed_sfixed64;             // 16 bytes
    MockRepeatedField packed_float;                // 16 bytes
    MockRepeatedField packed_double;               // 16 bytes
    MockRepeatedField packed_bool;                 // 16 bytes
    MockRepeatedField packed_enum;                 // 16 bytes
    int packed_int32_cached_byte_size;             // 4 bytes
    int packed_int64_cached_byte_size;             // 4 bytes
    int packed_uint32_cached_byte_size;            // 4 bytes
    int packed_uint64_cached_byte_size;            // 4 bytes
    int packed_sint32_cached_byte_size;            // 4 bytes
    int packed_sint64_cached_byte_size;            // 4 bytes
    int packed_enum_cached_byte_size;              // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-4 bytes
    // + 4-0 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 8 + 16 * 15 + 4 * 8);
  struct MockSplitGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes + 4 bytes padding
    void* split;                                   // 8 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-4 bytes
    // + 0-4 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockSplitGenerated, 32);
  using T = std::conditional_t<internal::ForceSplitFieldsInProtoc(),
                               MockSplitGenerated, MockGenerated>;

  EXPECT_EQ(sizeof(proto2_unittest::TestPackedTypes), sizeof(T));
}

TEST(GeneratedMessageTest, EvenHasbitsAndInt64Size) {
  // TODO: Currently 176 bytes; moving cached_size after f[33]
  // eliminates both padding gaps:
  //   struct MockOptimal : public MockMessageBase {  // 16 bytes
  //     int has_bits[2];                               // 8 bytes
  //     int64_t f_int64;                               // 8 bytes
  //     int32_t f[33];                                 // 132 bytes
  //     int cached_size;                               // 4 bytes
  //     PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-1 byte
  //     // + 0-7 bytes padding
  //   };  // 168 bytes
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[2];                               // 8 bytes
    int cached_size;                               // 4 bytes
    // + 4 bytes padding
    int64_t f_int64;               // 8 bytes
    int32_t f[33];                 // 132 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;  // 0-1 byte
    // + 4-3 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 176);

  struct MockSplitGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[2];                                    // 8 bytes
    int cached_size;                                    // 4 bytes
    // + 4 bytes padding
    void* split;                   // 8 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;  // 0-1 byte
    // + 0-7 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockSplitGenerated, 40);

  using Type = std::conditional_t<internal::ForceSplitFieldsInProtoc(),
                                  MockSplitGenerated, MockGenerated>;
  EXPECT_EQ(
      sizeof(protobuf_message_padding_test::MessageWithEvenHasbitsAndInt64),
      sizeof(Type));
}

TEST(GeneratedMessageTest, EvenHasbitsAndOneofSize) {
  // TODO: Currently 184 bytes (56 bytes when split); pairing
  // cached_size with oneof_case[1] eliminates both padding gaps (176 bytes,
  // or 48 bytes when split):
  //   struct MockOptimal : public MockMessageBase {  // 16 bytes
  //     int has_bits[2];                               // 8 bytes
  //     int64_t f_int64;                               // 8 bytes
  //     int32_t f[32];                                 // 128 bytes
  //     int cached_size;                               // 4 bytes
  //     uint32_t oneof_case[1];                        // 4 bytes
  //     int64_t oneof_val;                             // 8 bytes
  //     PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-1 byte
  //     // + 0-7 bytes padding
  //   };  // 176 bytes
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[2];                               // 8 bytes
    int cached_size;                               // 4 bytes
    // + 4 bytes padding
    int64_t f_int64;               // 8 bytes
    int32_t f[32];                 // 128 bytes
    int64_t oneof_val;             // 8 bytes
    uint32_t oneof_case[1];        // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;  // 0-1 byte
    // + 4-3 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 184);

  struct MockSplitGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[2];                                    // 8 bytes
    int cached_size;                                    // 4 bytes
    // + 4 bytes padding
    void* split;                   // 8 bytes
    int64_t oneof_val;             // 8 bytes
    uint32_t oneof_case[1];        // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;  // 0-1 byte
    // + 4-3 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockSplitGenerated, 56);

  using Type = std::conditional_t<internal::ForceSplitFieldsInProtoc(),
                                  MockSplitGenerated, MockGenerated>;
  EXPECT_EQ(
      sizeof(protobuf_message_padding_test::MessageWithEvenHasbitsAndOneof),
      sizeof(Type));
}

TEST(GeneratedMessageTest, EvenHasbitsAndOneofNoFieldPaddingSize) {
  // TODO: Currently 312 bytes (56 bytes when split); pairing
  // cached_size with oneof_case[1] eliminates both padding gaps (304 bytes,
  // or 48 bytes when split):
  //   struct MockOptimal : public MockMessageBase {  // 16 bytes
  //     int has_bits[2];                               // 8 bytes
  //     int64_t f[33];                                 // 264 bytes
  //     int cached_size;                               // 4 bytes
  //     uint32_t oneof_case[1];                        // 4 bytes
  //     int64_t oneof_val;                             // 8 bytes
  //     PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-1 byte
  //     // + 0-7 bytes padding
  //   };  // 304 bytes
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[2];                               // 8 bytes
    int cached_size;                               // 4 bytes
    // + 4 bytes padding
    int64_t f[33];                 // 264 bytes
    int64_t oneof_val;             // 8 bytes
    uint32_t oneof_case[1];        // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;  // 0-1 byte
    // + 4-3 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 312);

  struct MockSplitGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[2];                                    // 8 bytes
    int cached_size;                                    // 4 bytes
    // + 4 bytes padding
    void* split;                   // 8 bytes
    int64_t oneof_val;             // 8 bytes
    uint32_t oneof_case[1];        // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;  // 0-1 byte
    // + 4-3 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockSplitGenerated, 56);

  using Type = std::conditional_t<internal::ForceSplitFieldsInProtoc(),
                                  MockSplitGenerated, MockGenerated>;
  EXPECT_EQ(sizeof(protobuf_message_padding_test::
                       MessageWithEvenHasbitsAndOneofNoFieldPadding),
            sizeof(Type));
}

TEST(GeneratedMessageTest, EvenHasbitsAndPackedFieldSize) {
  // TODO: Currently 192 bytes; pairing cached_size with
  // f_packed_cached_byte_size eliminates both padding gaps:
  //   struct MockOptimal : public MockMessageBase {  // 16 bytes
  //     int has_bits[2];                               // 8 bytes
  //     int64_t f_int64;                               // 8 bytes
  //     MockRepeatedField f_packed;                    // 16 bytes
  //     int32_t f[32];                                 // 128 bytes
  //     int cached_size;                               // 4 bytes
  //     int f_packed_cached_byte_size;                 // 4 bytes
  //     PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-1 byte
  //     // + 0-7 bytes padding
  //   };  // 184 bytes
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[2];                               // 8 bytes
    int cached_size;                               // 4 bytes
    // + 4 bytes padding
    int64_t f_int64;                // 8 bytes
    MockRepeatedField f_packed;     // 16 bytes
    int32_t f[32];                  // 128 bytes
    int f_packed_cached_byte_size;  // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;   // 0-1 byte
    // + 4-3 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 192);

  struct MockSplitGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[2];                                    // 8 bytes
    int cached_size;                                    // 4 bytes
    // + 4 bytes padding
    void* split;                   // 8 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;  // 0-1 byte
    // + 0-7 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockSplitGenerated, 40);

  using Type = std::conditional_t<internal::ForceSplitFieldsInProtoc(),
                                  MockSplitGenerated, MockGenerated>;
  EXPECT_EQ(
      sizeof(
          protobuf_message_padding_test::MessageWithEvenHasbitsAndPackedField),
      sizeof(Type));
}

TEST(GeneratedMessageTest, EvenHasbitsAndTwoOneofsSize) {
  // TODO: Currently 200 bytes; moving cached_size and oneof_case
  // right after f[33] fills the 4B remainder and avoids the 4B gap after
  // cached_size:
  //   struct MockOptimal : public MockMessageBase {  // 16 bytes
  //     int has_bits[2];                               // 8 bytes
  //     int64_t f_int64;                               // 8 bytes
  //     int32_t f[33];                                 // 132 bytes
  //     int cached_size;                               // 4 bytes
  //     uint32_t oneof_case[2];                        // 8 bytes
  //     int64_t oneof_foo;                             // 8 bytes
  //     int64_t oneof_bar;                             // 8 bytes
  //     PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-1 byte
  //     // + 0-7 bytes padding
  //   };  // 192 bytes
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[2];                               // 8 bytes
    int cached_size;                               // 4 bytes
    // + 4 bytes padding
    int64_t f_int64;  // 8 bytes
    int32_t f[33];    // 132 bytes
    // + 4 bytes padding
    int64_t oneof_foo;             // 8 bytes
    int64_t oneof_bar;             // 8 bytes
    uint32_t oneof_case[2];        // 8 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;  // 0-1 byte
    // + 0-7 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 200);

  struct MockSplitGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[2];                                    // 8 bytes
    int cached_size;                                    // 4 bytes
    // + 4 bytes padding
    void* split;                   // 8 bytes
    int64_t oneof_foo;             // 8 bytes
    int64_t oneof_bar;             // 8 bytes
    uint32_t oneof_case[2];        // 8 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;  // 0-1 byte
    // + 0-7 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockSplitGenerated, 64);

  using Type = std::conditional_t<internal::ForceSplitFieldsInProtoc(),
                                  MockSplitGenerated, MockGenerated>;
  EXPECT_EQ(
      sizeof(protobuf_message_padding_test::MessageWithEvenHasbitsAndTwoOneofs),
      sizeof(Type));
}

TEST(GeneratedMessageTest, OddHasbitsAndOneofSize) {
  // TODO: Currently 56 bytes; placing oneof_case[1] right after
  // f_int32 eliminates both internal and tail padding:
  //   struct MockOptimal : public MockMessageBase {  // 16 bytes
  //     int has_bits[1];                               // 4 bytes
  //     int cached_size;                               // 4 bytes
  //     int64_t f_int64;                               // 8 bytes
  //     int32_t f_int32;                               // 4 bytes
  //     uint32_t oneof_case[1];                        // 4 bytes
  //     int64_t oneof_val;                             // 8 bytes
  //     PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-1 byte
  //     // + 0-7 bytes padding
  //   };  // 48 bytes
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    int64_t f_int64;                               // 8 bytes
    int32_t f_int32;                               // 4 bytes
    // + 4 bytes padding
    int64_t oneof_val;             // 8 bytes
    uint32_t oneof_case[1];        // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;  // 0-1 byte
    // + 4-3 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 56);

  struct MockSplitGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                                    // 4 bytes
    int cached_size;                                    // 4 bytes
    void* split;                                        // 8 bytes
    int64_t oneof_val;                                  // 8 bytes
    uint32_t oneof_case[1];                             // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                       // 0-1 byte
    // + 4-3 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockSplitGenerated, 48);

  using Type = std::conditional_t<internal::ForceSplitFieldsInProtoc(),
                                  MockSplitGenerated, MockGenerated>;
  EXPECT_EQ(
      sizeof(protobuf_message_padding_test::MessageWithOddHasbitsAndOneof),
      sizeof(Type));
}

TEST(GeneratedMessageTest, EvenHasbitsAndBoolSize) {
  // TODO: Currently 176 bytes; moving cached_size after f_bool
  // eliminates the 4B padding after has_bits[2]:
  //   struct MockOptimal : public MockMessageBase {  // 16 bytes
  //     int has_bits[2];                               // 8 bytes
  //     int64_t f_int64[2];                            // 16 bytes
  //     int32_t f[30];                                 // 120 bytes
  //     bool f_bool;                                   // 1 byte
  //     // + 3 bytes padding
  //     int cached_size;                               // 4 bytes
  //     PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-1 byte
  //     // + 0-7 bytes padding
  //   };  // 168 bytes
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[2];                               // 8 bytes
    int cached_size;                               // 4 bytes
    // + 4 bytes padding
    int64_t f_int64[2];            // 16 bytes
    int32_t f[30];                 // 120 bytes
    bool f_bool;                   // 1 byte
    PROTOBUF_TSAN_DECLARE_MEMBER;  // 0-1 byte
    // + 7-6 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 176);

  struct MockSplitGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[2];                                    // 8 bytes
    int cached_size;                                    // 4 bytes
    // + 4 bytes padding
    void* split;                   // 8 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;  // 0-1 byte
    // + 0-7 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockSplitGenerated, 40);

  using Type = std::conditional_t<internal::ForceSplitFieldsInProtoc(),
                                  MockSplitGenerated, MockGenerated>;
  EXPECT_EQ(
      sizeof(protobuf_message_padding_test::MessageWithEvenHasbitsAndBool),
      sizeof(Type));
}

TEST(GeneratedMessageTest, OddHasbitsBoolAndOneofSize) {
  // TODO: Currently 56 bytes; placing oneof_case[1] after f_bool
  // eliminates tail padding and reduces internal padding to 3B:
  //   struct MockOptimal : public MockMessageBase {  // 16 bytes
  //     int has_bits[1];                               // 4 bytes
  //     int cached_size;                               // 4 bytes
  //     int64_t f_int64;                               // 8 bytes
  //     bool f_bool;                                   // 1 byte
  //     // + 3 bytes padding
  //     uint32_t oneof_case[1];                        // 4 bytes
  //     int64_t oneof_val;                             // 8 bytes
  //     PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-1 byte
  //     // + 0-7 bytes padding
  //   };  // 48 bytes
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    int64_t f_int64;                               // 8 bytes
    bool f_bool;                                   // 1 byte
    // + 7 bytes padding
    int64_t oneof_val;             // 8 bytes
    uint32_t oneof_case[1];        // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;  // 0-1 byte
    // + 4-3 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 56);

  struct MockSplitGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                                    // 4 bytes
    int cached_size;                                    // 4 bytes
    void* split;                                        // 8 bytes
    int64_t oneof_val;                                  // 8 bytes
    uint32_t oneof_case[1];                             // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                       // 0-1 byte
    // + 4-3 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockSplitGenerated, 48);

  using Type = std::conditional_t<internal::ForceSplitFieldsInProtoc(),
                                  MockSplitGenerated, MockGenerated>;
  EXPECT_EQ(
      sizeof(protobuf_message_padding_test::MessageWithOddHasbitsBoolAndOneof),
      sizeof(Type));
}

TEST(GeneratedMessageTest, OddHasbitsPackedAndTwoOneofsSize) {
  // TODO: Currently 88 bytes; placing f_packed_cached_byte_size
  // and oneof_case[2] right after f_int32 eliminates both padding gaps:
  //   struct MockOptimal : public MockMessageBase {  // 16 bytes
  //     int has_bits[1];                               // 4 bytes
  //     int cached_size;                               // 4 bytes
  //     int64_t f_int64;                               // 8 bytes
  //     MockRepeatedField f_packed;                    // 16 bytes
  //     int32_t f_int32;                               // 4 bytes
  //     int f_packed_cached_byte_size;                 // 4 bytes
  //     uint32_t oneof_case[2];                        // 8 bytes
  //     int64_t foo_int64;                             // 8 bytes
  //     int64_t bar_int64;                             // 8 bytes
  //     PROTOBUF_TSAN_DECLARE_MEMBER;                  // 0-1 byte
  //     // + 0-7 bytes padding
  //   };  // 80 bytes
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    int64_t f_int64;                               // 8 bytes
    MockRepeatedField f_packed;                    // 16 bytes
    int32_t f_int32;                               // 4 bytes
    // + 4 bytes padding
    int64_t foo_int64;              // 8 bytes
    int64_t bar_int64;              // 8 bytes
    uint32_t oneof_case[2];         // 8 bytes
    int f_packed_cached_byte_size;  // 4 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;   // 0-1 byte
    // + 4-3 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 88);

  struct MockSplitGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                                    // 4 bytes
    int cached_size;                                    // 4 bytes
    void* split;                                        // 8 bytes
    int64_t foo_int64;                                  // 8 bytes
    int64_t bar_int64;                                  // 8 bytes
    uint32_t oneof_case[2];                             // 8 bytes
    PROTOBUF_TSAN_DECLARE_MEMBER;                       // 0-1 byte
    // + 0-7 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockSplitGenerated, 56);

  using Type = std::conditional_t<internal::ForceSplitFieldsInProtoc(),
                                  MockSplitGenerated, MockGenerated>;
  EXPECT_EQ(sizeof(protobuf_message_padding_test::
                       MessageWithOddHasbitsPackedAndTwoOneofs),
            sizeof(Type));
}

}  // namespace cpp_unittest
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
