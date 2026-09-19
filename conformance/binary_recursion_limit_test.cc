// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary performance conformance tests checking that absurdly deeply nested
// messages are rejected rather than parsed.  Part of the performance suite
// (see conformance.bzl).  This replaces the legacy suite-level
// BinaryAndJsonConformanceSuite::RunRecursionLimitTests(); the test names and
// the requests sent to the testee are identical to the legacy ones.
//
// Unlike the legacy suite, which ran the TestAllTypesEdition2023 tests here
// regardless of --maximum_edition, these are edition-gated like every other
// editions test (through MessageUnderTest(), see message_type_fixtures.h).

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "binary_wireformat.h"
#include "matchers.h"
#include "message_type_fixtures.h"
#include "test_environment.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "google/protobuf/test_messages_proto2.pb.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::protobuf_test_messages::editions::TestAllTypesEdition2023;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;

// The default recursion limit is 100 for most languages. 10,000 for golang. We
// use a larger number here for test.
constexpr int kMapDepth = 20000;
constexpr int kMapStringKeyDepth = 10000;
constexpr int kMessageSetDepth = 20000;

// The payloads are built directly on the wire rather than serialized from a
// message object of that depth: ByteSizeLong(), the serializer and the
// destructor recurse once per nesting level, which overflows the stack under
// the sanitizers' larger frames (b/563731788).  The bytes are identical to
// what the message objects serialize, see PayloadsMatchTheMessageObjects
// below and the request goldens in migration/.

// One nesting level of such a payload, seen from outside: the bytes before the
// level below it (the level's own fields, then the tag of the field that holds
// the next level; its length prefix is added by Nested()) and the bytes after
// it (e.g. the end tag of a group).  The innermost level holds no next level.
struct Level {
  Wire before;
  Wire after;
};

// The serialization of `levels[0]`, which holds the serialization of
// `levels[1]` as a length-prefixed submessage, and so on down to
// `levels.back()`.  Two passes and no recursion: the sizes inside out, the
// bytes outside in.
Wire Nested(const std::vector<Level>& levels) {
  std::vector<size_t> sizes(levels.size());
  for (size_t i = levels.size(); i-- > 0;) {
    sizes[i] = levels[i].before.size() + levels[i].after.size();
    if (i + 1 < levels.size()) {
      sizes[i] += Varint(sizes[i + 1]).size() + sizes[i + 1];
    }
  }
  std::string bytes;
  bytes.reserve(sizes.empty() ? 0 : sizes[0]);
  for (size_t i = 0; i < levels.size(); ++i) {
    absl::StrAppend(&bytes, levels[i].before.data());
    if (i + 1 < levels.size()) {
      absl::StrAppend(&bytes, Varint(sizes[i + 1]).data());
    }
  }
  for (size_t i = levels.size(); i-- > 0;) {
    absl::StrAppend(&bytes, levels[i].after.data());
  }
  return Wire(std::move(bytes));
}

// TestAllTypesEdition2023 whose map_recursive (301) entry {key 0, value}
// holds a TestAllTypesEdition2023 with optional_int32 (1) = 123 and, `depth`
// times in all, its own such entry.
Wire MapPayload(int depth) {
  std::vector<Level> levels;
  levels.push_back({Tag(301, WireType::kLengthPrefixed)});
  for (int i = 0; i < depth; ++i) {
    // The map entry: C++ writes the key even when it is the default.
    levels.push_back(
        {Wire(VarintField(1, 0), Tag(2, WireType::kLengthPrefixed))});
    // The nested message.
    Wire fields = VarintField(1, 123);
    if (i + 1 < depth) {
      fields = Wire(fields, Tag(301, WireType::kLengthPrefixed));
    }
    levels.push_back({fields});
  }
  return Nested(levels);
}

// TestAllTypesEdition2023 whose map_string_nested_message (71) entry {key "",
// value} holds a NestedMessage whose corecursive (2) is a
// TestAllTypesEdition2023 with optional_int32 (1) = 123 and, `depth` times in
// all, its own such entry.
Wire MapStringKeyPayload(int depth) {
  std::vector<Level> levels;
  levels.push_back({Tag(71, WireType::kLengthPrefixed)});
  for (int i = 0; i < depth; ++i) {
    // The map entry: C++ writes the key even when it is empty.
    levels.push_back(
        {Wire(LengthPrefixedField(1, ""), Tag(2, WireType::kLengthPrefixed))});
    // The NestedMessage.
    levels.push_back({Tag(2, WireType::kLengthPrefixed)});
    // The nested TestAllTypesEdition2023.
    Wire fields = VarintField(1, 123);
    if (i + 1 < depth) {
      fields = Wire(fields, Tag(71, WireType::kLengthPrefixed));
    }
    levels.push_back({fields});
  }
  return Nested(levels);
}

// TestAllTypesProto2 whose message_set_correct (500) holds a MessageSetCorrect
// with one MessageSetCorrectExtension2 item (type id 4135312) whose sub_msg
// (10) is again such a MessageSetCorrect, `depth` times, the innermost
// extension holding i (9) = 123 instead.  A message set item is a group:
// start tag (1), type_id (2), message (3), end tag.
Wire MessageSetPayload(int depth) {
  std::vector<Level> levels;
  levels.push_back({Tag(500, WireType::kLengthPrefixed)});
  for (int i = 0; i <= depth; ++i) {
    // The MessageSetCorrect and its item.
    levels.push_back(
        {Wire(Tag(1, WireType::kStartGroup), VarintField(2, 4135312),
              Tag(3, WireType::kLengthPrefixed)),
         Tag(1, WireType::kEndGroup)});
    // The MessageSetCorrectExtension2.
    levels.push_back(
        {i < depth ? Tag(10, WireType::kLengthPrefixed) : VarintField(9, 123)});
  }
  return Nested(levels);
}

// A self-check of the payload builders above, not a conformance request (no
// testee is involved): the same payloads built from message objects the way
// the legacy suite built them, at a depth the stack can take.
TEST(RecursionLimitPayloadPerformanceTest, PayloadsMatchTheMessageObjects) {
  constexpr int kDepth = 3;
  {
    TestAllTypesEdition2023 message;
    TestAllTypesEdition2023* sub = &message;
    for (int i = 0; i < kDepth; i++) {
      sub = &(*sub->mutable_map_recursive())[0];
      sub->set_optional_int32(123);
    }
    EXPECT_EQ(MapPayload(kDepth), Wire(message.SerializeAsString()));
  }
  {
    TestAllTypesEdition2023 message;
    TestAllTypesEdition2023* sub = &message;
    for (int i = 0; i < kDepth; i++) {
      sub =
          (*sub->mutable_map_string_nested_message())[""].mutable_corecursive();
      sub->set_optional_int32(123);
    }
    EXPECT_EQ(MapStringKeyPayload(kDepth), Wire(message.SerializeAsString()));
  }
  {
    TestAllTypesProto2 message;
    TestAllTypesProto2::MessageSetCorrect* sub =
        message.mutable_message_set_correct();
    for (int i = 0; i < kDepth; i++) {
      sub = sub->MutableExtension(
                   TestAllTypesProto2::MessageSetCorrectExtension2::
                       message_set_extension)
                ->mutable_sub_msg();
    }
    sub->MutableExtension(TestAllTypesProto2::MessageSetCorrectExtension2::
                              message_set_extension)
        ->set_i(123);
    EXPECT_EQ(MessageSetPayload(kDepth), Wire(message.SerializeAsString()));
  }
}

// The TestAllTypesProto2 test, which every --maximum_edition covers.
using RecursionLimitPerformanceTest = ConformanceTest;

// The TestAllTypesEdition2023 tests, which the base SetUp() skips when
// --maximum_edition doesn't cover editions.
using Edition2023RecursionLimitPerformanceTest = Edition2023ConformanceTest;

TEST_F(Edition2023RecursionLimitPerformanceTest, Map) {
  EXPECT_THAT(RecommendedTest("EnforceDepthLimit.Map")
                  .ParseBinary(TestAllTypesEdition2023::descriptor(),
                               MapPayload(kMapDepth))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_F(Edition2023RecursionLimitPerformanceTest, MapStringKey) {
  EXPECT_THAT(RecommendedTest("EnforceDepthLimit.MapStringKey")
                  .ParseBinary(TestAllTypesEdition2023::descriptor(),
                               MapStringKeyPayload(kMapStringKeyDepth))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_F(RecursionLimitPerformanceTest, MessageSetExtension) {
  EXPECT_THAT(RecommendedTest("EnforceDepthLimit.MessageSetExtension")
                  .ParseBinary(TestAllTypesProto2::descriptor(),
                               MessageSetPayload(kMessageSetDepth))
                  .ParseOnly(),
              Yields(IsParseError()));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
