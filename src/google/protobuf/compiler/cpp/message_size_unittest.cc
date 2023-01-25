// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/unittest.pb.h"

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
  void* arena;       // 8 bytes
  int16_t capacity;  // 4 bytes
  int16_t size;      // 4 bytes
  void* data;        // 8 bytes
};
ABSL_CHECK_MESSAGE_SIZE(MockExtensionSet, 24);

struct MockRepeatedPtrField {
  void* arena;       // 8 bytes
  int current_size;  // 4 bytes
  int total_size;    // 4 bytes
  void* data;        // 8 bytes
};
ABSL_CHECK_MESSAGE_SIZE(MockRepeatedPtrField, 24);

struct MockRepeatedField {
  int current_size;  // 4 bytes
  int total_size;    // 4 bytes
  void* data;        // 8 bytes
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
  EXPECT_EQ(sizeof(protobuf_unittest::TestEmptyMessage),
            sizeof(MockZeroFieldsBase));
}

TEST(GeneratedMessageTest, ReservedSize) {
  EXPECT_EQ(sizeof(protobuf_unittest::TestReservedFields),
            sizeof(MockZeroFieldsBase));
}

TEST(GeneratedMessageTest, EmptyMessageWithExtensionsSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    MockExtensionSet extensions;                   // 24 bytes
    int cached_size;                               // 4 bytes
    // + 4 bytes of padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 48);
  EXPECT_EQ(sizeof(protobuf_unittest::TestEmptyMessageWithExtensions),
            sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, RecursiveMessageSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    void* a;                                       // 8 bytes
    int32_t i;                                     // 4 bytes
    // + 4 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 40);
  EXPECT_EQ(sizeof(protobuf_unittest::TestRecursiveMessage),
            sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, OneStringSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    void* data;                                    // 8 bytes
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 32);
  EXPECT_EQ(sizeof(protobuf_unittest::OneString), sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, MoreStringSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int cached_size;                               // 4 bytes
    MockRepeatedPtrField data;                     // 24 bytes
    // + 4 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 48);
  EXPECT_EQ(sizeof(protobuf_unittest::MoreString), sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, Int32MessageSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    int32_t data;                                  // 4 bytes
    // + 4 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 32);
  EXPECT_EQ(sizeof(protobuf_unittest::Int32Message), sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, Int64MessageSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    int64_t data;                                  // 8 bytes
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 32);
  EXPECT_EQ(sizeof(protobuf_unittest::Int64Message), sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, BoolMessageSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    bool data;                                     // 1 byte
    // + 3 bytes padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 32);
  EXPECT_EQ(sizeof(protobuf_unittest::BoolMessage), sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, OneofSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    void* foo;                                     // 8 bytes
    int cached_size;                               // 4 bytes
    uint32_t oneof_case[1];                        // 4 bytes
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 32);
  EXPECT_EQ(sizeof(protobuf_unittest::TestOneof), sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, Oneof2Size) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    void* baz_string;                              // 8 bytes
    int32_t baz_int;                               // 4 bytes
                                                   // + 4 bytes padding
    void* foo;                                     // 8 bytes
    void* bar;                                     // 8 bytes
    uint32_t oneof_case[2];                        // 8 bytes
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 64);
  EXPECT_EQ(sizeof(protobuf_unittest::TestOneof2), sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, FieldOrderingsSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    int has_bits[1];                               // 4 bytes
    int cached_size;                               // 4 bytes
    MockExtensionSet extensions;                   // 24 bytes
    void* my_string;                               // 8 bytes
    void* optional_nested_message;                 // 8 bytes
    int64_t my_int;                                // 8 bytes
    float my_float;                                // 4 bytes
    // + 4 bytes of padding
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 80);
  EXPECT_EQ(sizeof(protobuf_unittest::TestFieldOrderings), sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, TestMessageSize) {
  // We expect the message to contain (not in this order):
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
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 56);
  EXPECT_EQ(sizeof(protobuf_unittest::TestMessageSize), sizeof(MockGenerated));
}

TEST(GeneratedMessageTest, PackedTypesSize) {
  struct MockGenerated : public MockMessageBase {  // 16 bytes
    MockRepeatedField packed_int32;                // 16 bytes
    int packed_int32_cached_byte_size;             // 4 bytes + 4 bytes padding
    MockRepeatedField packed_int64;                // 16 bytes
    int packed_int64_cached_byte_size;             // 4 bytes + 4 bytes padding
    MockRepeatedField packed_uint32;               // 16 bytes
    int packed_uint32_cached_byte_size;            // 4 bytes + 4 bytes padding
    MockRepeatedField packed_uint64;               // 16 bytes
    int packed_uint64_cached_byte_size;            // 4 bytes + 4 bytes padding
    MockRepeatedField packed_sint32;               // 16 bytes
    int packed_sint32_cached_byte_size;            // 4 bytes + 4 bytes padding
    MockRepeatedField packed_sint64;               // 16 bytes
    int packed_sint64_cached_byte_size;            // 4 bytes + 4 bytes padding
    MockRepeatedField packed_fixed32;              // 16 bytes
    MockRepeatedField packed_fixed64;              // 16 bytes
    MockRepeatedField packed_sfixed32;             // 16 bytes
    MockRepeatedField packed_sfixed64;             // 16 bytes
    MockRepeatedField packed_float;                // 16 bytes
    MockRepeatedField packed_double;               // 16 bytes
    MockRepeatedField packed_bool;                 // 16 bytes
    MockRepeatedField packed_enum;                 // 16 bytes
    int packed_enum_cached_byte_size;              // 4 bytes
    int cached_size;                               // 4 bytes
  };
  ABSL_CHECK_MESSAGE_SIZE(MockGenerated, 16 * 15 + 8 * 6 + 8);
  EXPECT_EQ(sizeof(protobuf_unittest::TestPackedTypes), sizeof(MockGenerated));
}

}  // namespace cpp_unittest
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
