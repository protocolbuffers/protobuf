// -*- c++ -*-
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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// This file needs to be included as .inc as it depends on certain macros being
// defined prior to its inclusion.

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cmath>
#include <functional>
#include <limits>
#include <vector>

#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <fstream>
#include <sstream>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/log/scoped_mock_log.h"
#include "absl/strings/cord.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/io_win32.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/message.h"
#include "google/protobuf/test_util2.h"


namespace google {
namespace protobuf {

#if defined(_WIN32)
// DO NOT include <io.h>, instead create functions in io_win32.{h,cc} and import
// them like we do below.
using google::protobuf::io::win32::close;
using google::protobuf::io::win32::open;
#endif

#ifndef O_BINARY
#ifdef _O_BINARY
#define O_BINARY _O_BINARY
#else
#define O_BINARY 0  // If this isn't defined, the platform doesn't need it.
#endif
#endif

TEST(MESSAGE_TEST_NAME, SerializeHelpers) {
  // TODO(kenton):  Test more helpers?  They're all two-liners so it seems
  //   like a waste of time.

  UNITTEST::TestAllTypes message;
  TestUtil::SetAllFields(&message);
  std::stringstream stream;

  std::string str1("foo");
  std::string str2("bar");

  EXPECT_TRUE(message.SerializeToString(&str1));
  EXPECT_TRUE(message.AppendToString(&str2));
  EXPECT_TRUE(message.SerializeToOstream(&stream));

  EXPECT_EQ(str1.size() + 3, str2.size());
  EXPECT_EQ("bar", str2.substr(0, 3));
  // Don't use EXPECT_EQ because we don't want to dump raw binary data to
  // stdout.
  EXPECT_TRUE(str2.substr(3) == str1);

  // GCC gives some sort of error if we try to just do stream.str() == str1.
  std::string temp = stream.str();
  EXPECT_TRUE(temp == str1);

  EXPECT_TRUE(message.SerializeAsString() == str1);

}

TEST(MESSAGE_TEST_NAME, SerializeToBrokenOstream) {
  std::ofstream out;
  UNITTEST::TestAllTypes message;
  message.set_optional_int32(123);

  EXPECT_FALSE(message.SerializeToOstream(&out));
}

TEST(MESSAGE_TEST_NAME, ParseFromFileDescriptor) {
  std::string filename =
      TestUtil::GetTestDataPath("third_party/protobuf/testdata/golden_message");
  int file = open(filename.c_str(), O_RDONLY | O_BINARY);
  ASSERT_GE(file, 0);

  UNITTEST::TestAllTypes message;
  EXPECT_TRUE(message.ParseFromFileDescriptor(file));
  TestUtil::ExpectAllFieldsSet(message);

  EXPECT_GE(close(file), 0);
}

TEST(MESSAGE_TEST_NAME, ParsePackedFromFileDescriptor) {
  std::string filename = TestUtil::GetTestDataPath(
      "third_party/protobuf/testdata/golden_packed_fields_message");
  int file = open(filename.c_str(), O_RDONLY | O_BINARY);
  ASSERT_GE(file, 0);

  UNITTEST::TestPackedTypes message;
  EXPECT_TRUE(message.ParseFromFileDescriptor(file));
  TestUtil::ExpectPackedFieldsSet(message);

  EXPECT_GE(close(file), 0);
}

TEST(MESSAGE_TEST_NAME, ParseHelpers) {
  // TODO(kenton):  Test more helpers?  They're all two-liners so it seems
  //   like a waste of time.
  std::string data;

  {
    // Set up.
    UNITTEST::TestAllTypes message;
    TestUtil::SetAllFields(&message);
    message.SerializeToString(&data);
  }

  {
    // Test ParseFromString.
    UNITTEST::TestAllTypes message;
    EXPECT_TRUE(message.ParseFromString(data));
    TestUtil::ExpectAllFieldsSet(message);
  }

  {
    // Test ParseFromIstream.
    UNITTEST::TestAllTypes message;
    std::stringstream stream(data);
    EXPECT_TRUE(message.ParseFromIstream(&stream));
    EXPECT_TRUE(stream.eof());
    TestUtil::ExpectAllFieldsSet(message);
  }

  {
    // Test ParseFromBoundedZeroCopyStream.
    std::string data_with_junk(data);
    data_with_junk.append("some junk on the end");
    io::ArrayInputStream stream(data_with_junk.data(), data_with_junk.size());
    UNITTEST::TestAllTypes message;
    EXPECT_TRUE(message.ParseFromBoundedZeroCopyStream(&stream, data.size()));
    TestUtil::ExpectAllFieldsSet(message);
  }

  {
    // Test that ParseFromBoundedZeroCopyStream fails (but doesn't crash) if
    // EOF is reached before the expected number of bytes.
    io::ArrayInputStream stream(data.data(), data.size());
    UNITTEST::TestAllTypes message;
    EXPECT_FALSE(
        message.ParseFromBoundedZeroCopyStream(&stream, data.size() + 1));
  }

  // Test bytes cord
  {
    UNITTEST::TestCord cord_message;
    cord_message.set_optional_bytes_cord("bytes_cord");
    EXPECT_TRUE(cord_message.SerializeToString(&data));
    EXPECT_TRUE(cord_message.SerializeAsString() == data);
  }
  {
    UNITTEST::TestCord cord_message;
    EXPECT_TRUE(cord_message.ParseFromString(data));
    EXPECT_EQ("bytes_cord", cord_message.optional_bytes_cord());
  }
}

TEST(MESSAGE_TEST_NAME, ParseFailsIfNotInitialized) {
  UNITTEST::TestRequired message;

  {
    absl::ScopedMockLog log(absl::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_, absl::StrCat(
            "Can't parse message of type \"", UNITTEST_PACKAGE_NAME,
            ".TestRequired\" because it is missing required fields: a, b, c")));
    log.StartCapturingLogs();
    EXPECT_FALSE(message.ParseFromString(""));
  }
}

TEST(MESSAGE_TEST_NAME, ParseFailsIfSubmessageNotInitialized) {
  UNITTEST::TestRequiredForeign source, message;
  source.mutable_optional_message()->set_dummy2(100);
  std::string serialized = source.SerializePartialAsString();

  EXPECT_TRUE(message.ParsePartialFromString(serialized));
  EXPECT_FALSE(message.IsInitialized());

  {
    absl::ScopedMockLog log(absl::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_, absl::StrCat(
          "Can't parse message of type \"", UNITTEST_PACKAGE_NAME,
          ".TestRequiredForeign\" because it is missing required fields: "
          "optional_message.a, optional_message.b, optional_message.c")));
    log.StartCapturingLogs();
    EXPECT_FALSE(message.ParseFromString(source.SerializePartialAsString()));
  }
}

TEST(MESSAGE_TEST_NAME, ParseFailsIfExtensionNotInitialized) {
  UNITTEST::TestChildExtension source, message;
  auto* r = source.mutable_optional_extension()->MutableExtension(
      UNITTEST::TestRequired::single);
  r->set_dummy2(100);
  std::string serialized = source.SerializePartialAsString();

  EXPECT_TRUE(message.ParsePartialFromString(serialized));
  EXPECT_FALSE(message.IsInitialized());

{
    absl::ScopedMockLog log(absl::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_, absl::Substitute(
                  "Can't parse message of type \"$0.TestChildExtension\" "
                  "because it is missing required fields: "
                  "optional_extension.($0.TestRequired.single).a, "
                  "optional_extension.($0.TestRequired.single).b, "
                  "optional_extension.($0.TestRequired.single).c",
                  UNITTEST_PACKAGE_NAME)));
    log.StartCapturingLogs();
    EXPECT_FALSE(message.ParseFromString(source.SerializePartialAsString()));
  }
}

TEST(MESSAGE_TEST_NAME, MergeFromUninitialized) {
  UNITTEST::TestNestedRequiredForeign o, p, q;
  UNITTEST::TestNestedRequiredForeign* child = o.mutable_child();
  constexpr int kDepth = 2;
  for (int i = 0; i < kDepth; i++) {
    child->set_dummy(i);
    child = child->mutable_child();
  }
  UNITTEST::TestRequiredForeign* payload = child->mutable_payload();
  payload->mutable_optional_message()->set_a(1);
  payload->mutable_optional_message()->set_dummy2(100);
  payload->mutable_optional_message()->set_dummy4(200);
  ASSERT_TRUE(p.ParsePartialFromString(o.SerializePartialAsString()));

  q.mutable_child()->set_dummy(500);
  q = p;
  q.ParsePartialFromString(q.SerializePartialAsString());
  EXPECT_TRUE(TestUtil::EqualsToSerialized(q, o.SerializePartialAsString()));
  EXPECT_TRUE(TestUtil::EqualsToSerialized(q, p.SerializePartialAsString()));
}

TEST(MESSAGE_TEST_NAME, UninitializedAndTooDeep) {
  UNITTEST::TestRequiredForeign original;
  original.mutable_optional_message()->set_a(1);
  original.mutable_optional_lazy_message()
      ->mutable_child()
      ->mutable_payload()
      ->set_optional_int64(0);

  std::string data;
  ASSERT_TRUE(original.SerializePartialToString(&data));

  UNITTEST::TestRequiredForeign pass;
  ASSERT_TRUE(pass.ParsePartialFromString(data));
  ASSERT_FALSE(pass.IsInitialized());

  io::ArrayInputStream array_stream(data.data(), data.size());
  io::CodedInputStream input_stream(&array_stream);
  input_stream.SetRecursionLimit(2);

  UNITTEST::TestRequiredForeign fail;
  EXPECT_FALSE(fail.ParsePartialFromCodedStream(&input_stream));

  UNITTEST::TestRequiredForeign fail_uninitialized;
  EXPECT_FALSE(fail_uninitialized.ParseFromString(data));
}

TEST(MESSAGE_TEST_NAME, ExplicitLazyExceedRecursionLimit) {
  UNITTEST::NestedTestAllTypes original, parsed;
  // Build proto with recursion depth of 3.
  original.mutable_lazy_child()
      ->mutable_child()
      ->mutable_payload()
      ->set_optional_int32(-1);
  std::string serialized;
  EXPECT_TRUE(original.SerializeToString(&serialized));

  // User annotated LazyField ([lazy = true]) is eagerly verified and should
  // catch the recursion limit violation.
  io::ArrayInputStream array_stream(serialized.data(), serialized.size());
  io::CodedInputStream input_stream(&array_stream);
  input_stream.SetRecursionLimit(2);
  EXPECT_FALSE(parsed.ParseFromCodedStream(&input_stream));

  // Lazy read results in parsing error which can be verified by not having
  // expected value.
  EXPECT_NE(parsed.lazy_child().child().payload().optional_int32(), -1);
}

TEST(MESSAGE_TEST_NAME, ParseFailNonCanonicalZeroTag) {
  const char encoded[] = {"\n\x3\x80\0\0"};
  UNITTEST::NestedTestAllTypes parsed;
  EXPECT_FALSE(parsed.ParsePartialFromString(
      absl::string_view{encoded, sizeof(encoded) - 1}));
}

TEST(MESSAGE_TEST_NAME, ParseFailNonCanonicalZeroField) {
  const char encoded[] = {"\012\x6\205\0\0\0\0\0"};
  UNITTEST::NestedTestAllTypes parsed;
  EXPECT_FALSE(parsed.ParsePartialFromString(
      absl::string_view{encoded, sizeof(encoded) - 1}));
}

TEST(MESSAGE_TEST_NAME, NestedExplicitLazyExceedRecursionLimit) {
  UNITTEST::NestedTestAllTypes original, parsed;
  // Build proto with recursion depth of 5, with nested annotated LazyField.
  original.mutable_lazy_child()
      ->mutable_child()
      ->mutable_lazy_child()
      ->mutable_child()
      ->mutable_payload()
      ->set_optional_int32(-1);
  std::string serialized;
  EXPECT_TRUE(original.SerializeToString(&serialized));

  // User annotated LazyField ([lazy = true]) is eagerly verified and should
  // catch the recursion limit violation.
  io::ArrayInputStream array_stream(serialized.data(), serialized.size());
  io::CodedInputStream input_stream(&array_stream);
  input_stream.SetRecursionLimit(4);
  EXPECT_FALSE(parsed.ParseFromCodedStream(&input_stream));

  // Lazy read results in parsing error which can be verified by not having
  // expected value.
  EXPECT_NE(parsed.lazy_child()
                .child()
                .lazy_child()
                .child()
                .payload()
                .optional_int32(),
            -1);
}

TEST(MESSAGE_TEST_NAME, ParseFailsIfSubmessageTruncated) {
  UNITTEST::NestedTestAllTypes o, p;
  constexpr int kDepth = 5;
  auto* child = o.mutable_child();
  for (int i = 0; i < kDepth; i++) {
    child = child->mutable_child();
  }
  TestUtil::SetAllFields(child->mutable_payload());

  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  // Should parse correctly.
  EXPECT_TRUE(p.ParseFromString(serialized));

  constexpr int kMaxTruncate = 50;
  ASSERT_GT(serialized.size(), kMaxTruncate);

  for (int i = 1; i < kMaxTruncate; i += 3) {
    EXPECT_FALSE(
        p.ParseFromString(serialized.substr(0, serialized.size() - i)));
  }
}

TEST(MESSAGE_TEST_NAME, ParseFailsIfWireMalformed) {
  UNITTEST::NestedTestAllTypes o, p;
  constexpr int kDepth = 5;
  auto* child = o.mutable_child();
  for (int i = 0; i < kDepth; i++) {
    child = child->mutable_child();
  }
  // -1 becomes \xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x1
  child->mutable_payload()->set_optional_int32(-1);

  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  // Should parse correctly.
  EXPECT_TRUE(p.ParseFromString(serialized));

  // Overwriting the last byte to 0xFF results in malformed wire.
  serialized[serialized.size() - 1] = 0xFF;
  EXPECT_FALSE(p.ParseFromString(serialized));
}

TEST(MESSAGE_TEST_NAME, ParseFailsIfOneofWireMalformed) {
  UNITTEST::NestedTestAllTypes o, p;
  constexpr int kDepth = 5;
  auto* child = o.mutable_child();
  for (int i = 0; i < kDepth; i++) {
    child = child->mutable_child();
  }
  // -1 becomes \xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x1
  child->mutable_payload()->mutable_oneof_nested_message()->set_bb(-1);

  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  // Should parse correctly.
  EXPECT_TRUE(p.ParseFromString(serialized));

  // Overwriting the last byte to 0xFF results in malformed wire.
  serialized[serialized.size() - 1] = 0xFF;
  EXPECT_FALSE(p.ParseFromString(serialized));
}

TEST(MESSAGE_TEST_NAME, ParseFailsIfExtensionWireMalformed) {
  UNITTEST::TestChildExtension o, p;
  auto* m = o.mutable_optional_extension()->MutableExtension(
      UNITTEST::optional_nested_message_extension);

  // -1 becomes \xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x1
  m->set_bb(-1);

  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  // Should parse correctly.
  EXPECT_TRUE(p.ParseFromString(serialized));

  // Overwriting the last byte to 0xFF results in malformed wire.
  serialized[serialized.size() - 1] = 0xFF;
  EXPECT_FALSE(p.ParseFromString(serialized));
}

TEST(MESSAGE_TEST_NAME, ParseFailsIfGroupFieldMalformed) {
  UNITTEST::TestMutualRecursionA original, parsed;
  original.mutable_bb()
      ->mutable_a()
      ->mutable_subgroup()
      ->mutable_sub_message()
      ->mutable_b()
      ->set_optional_int32(-1);

  std::string data;
  ASSERT_TRUE(original.SerializeToString(&data));
  // Should parse correctly.
  ASSERT_TRUE(parsed.ParseFromString(data));
  // Overwriting the last byte of varint (-1) to 0xFF results in malformed wire.
  data[data.size() - 2] = 0xFF;

  EXPECT_FALSE(parsed.ParseFromString(data));
}

TEST(MESSAGE_TEST_NAME, ParseFailsIfRepeatedGroupFieldMalformed) {
  UNITTEST::TestMutualRecursionA original, parsed;
  original.mutable_bb()
      ->mutable_a()
      ->add_subgroupr()
      ->mutable_payload()
      ->set_optional_int64(-1);

  std::string data;
  ASSERT_TRUE(original.SerializeToString(&data));
  // Should parse correctly.
  ASSERT_TRUE(parsed.ParseFromString(data));
  // Overwriting the last byte of varint (-1) to 0xFF results in malformed wire.
  data[data.size() - 2] = 0xFF;

  EXPECT_FALSE(parsed.ParseFromString(data));
}

TEST(MESSAGE_TEST_NAME, UninitializedAndMalformed) {
  UNITTEST::TestRequiredForeign o, p1, p2;
  o.mutable_optional_message()->set_a(-1);

  // -1 becomes \xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x1
  std::string serialized;
  EXPECT_TRUE(o.SerializePartialToString(&serialized));

  // Should parse correctly.
  EXPECT_TRUE(p1.ParsePartialFromString(serialized));
  EXPECT_FALSE(p1.IsInitialized());

  // Overwriting the last byte to 0xFF results in malformed wire.
  serialized[serialized.size() - 1] = 0xFF;
  EXPECT_FALSE(p2.ParseFromString(serialized));
  EXPECT_FALSE(p2.IsInitialized());
}

inline UNITTEST::NestedTestAllTypes InitNestedProto(int depth) {
  UNITTEST::NestedTestAllTypes p;
  auto* child = p.mutable_child();
  for (int i = 0; i < depth; i++) {
    child->mutable_payload()->set_optional_int32(i);
    child = child->mutable_child();
  }
  // -1 becomes \xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x1
  child->mutable_payload()->set_optional_int32(-1);
  return p;
}

// Parsing proto must not access beyond the bound.
TEST(MESSAGE_TEST_NAME, ParseStrictlyBoundedStream) {
  UNITTEST::NestedTestAllTypes o, p;
  constexpr int kDepth = 2;
  o = InitNestedProto(kDepth);
  TestUtil::SetAllFields(o.mutable_child()->mutable_payload());
  o.mutable_child()->mutable_child()->mutable_payload()->set_optional_string(
      std::string(1024, 'a'));

  std::string data;
  EXPECT_TRUE(o.SerializeToString(&data));

  TestUtil::BoundedArrayInputStream stream(data.data(), data.size());
  EXPECT_TRUE(p.ParseFromBoundedZeroCopyStream(&stream, data.size()));
  TestUtil::ExpectAllFieldsSet(p.child().payload());
}

TEST(MESSAGE_TEST_NAME, AllSetMethodsOnStringField) {
  UNITTEST::TestAllTypes msg;

  msg.set_optional_string(absl::string_view("Abcdef"));
  EXPECT_EQ(msg.optional_string(), "Abcdef");

  msg.set_optional_string("Asciiz");
  EXPECT_EQ(msg.optional_string(), "Asciiz");

  msg.set_optional_string("Length delimited", 6);
  EXPECT_EQ(msg.optional_string(), "Length");

  std::string value = "std::string value 1";
  msg.set_optional_string(value);
  EXPECT_EQ(msg.optional_string(), "std::string value 1");

  value = "std::string value 2";
  msg.set_optional_string(std::cref(value));
  EXPECT_EQ(msg.optional_string(), "std::string value 2");

  value = "std::string value 3";
  msg.set_optional_string(std::move(value));
  EXPECT_EQ(msg.optional_string(), "std::string value 3");
}


TEST(MESSAGE_TEST_NAME, AllAddMethodsOnRepeatedStringField) {
  UNITTEST::TestAllTypes msg;

  msg.add_repeated_string(absl::string_view("Abcdef"));
  EXPECT_EQ(msg.repeated_string(0), "Abcdef");
  msg.clear_repeated_string();

  msg.add_repeated_string("Asciiz");
  EXPECT_EQ(msg.repeated_string(0), "Asciiz");
  msg.clear_repeated_string();

  msg.add_repeated_string("Length delimited", 6);
  EXPECT_EQ(msg.repeated_string(0), "Length");
  msg.clear_repeated_string();

  std::string value = "std::string value 1";
  msg.add_repeated_string(value);
  EXPECT_EQ(msg.repeated_string(0), "std::string value 1");
  msg.clear_repeated_string();

  value = "std::string value 2";
  msg.add_repeated_string(std::cref(value));
  EXPECT_EQ(msg.repeated_string(0), "std::string value 2");
  msg.clear_repeated_string();

  value = "std::string value 3";
  msg.add_repeated_string(std::move(value));
  EXPECT_EQ(msg.repeated_string(0), "std::string value 3");
  msg.clear_repeated_string();
}

// Helper functions to touch any nested lazy field
void TouchLazy(UNITTEST::NestedTestAllTypes* msg);
void TouchLazy(UNITTEST::TestAllTypes* msg);
void TouchLazy(UNITTEST::TestAllTypes::NestedMessage* msg) {}

void TouchLazy(UNITTEST::TestAllTypes* msg) {
  if (msg->has_optional_lazy_message()) {
    TouchLazy(msg->mutable_optional_lazy_message());
  }
  if (msg->has_optional_unverified_lazy_message()) {
    TouchLazy(msg->mutable_optional_unverified_lazy_message());
  }
  for (auto& child : *msg->mutable_repeated_lazy_message()) {
    TouchLazy(&child);
  }
}

void TouchLazy(UNITTEST::NestedTestAllTypes* msg) {
  if (msg->has_child()) TouchLazy(msg->mutable_child());
  if (msg->has_payload()) TouchLazy(msg->mutable_payload());
  for (auto& child : *msg->mutable_repeated_child()) {
    TouchLazy(&child);
  }
  if (msg->has_lazy_child()) TouchLazy(msg->mutable_lazy_child());
  if (msg->has_eager_child()) TouchLazy(msg->mutable_eager_child());
}

TEST(MESSAGE_TEST_NAME, SuccessAfterParsingFailure) {
  UNITTEST::NestedTestAllTypes o, p, q;
  constexpr int kDepth = 5;
  o = InitNestedProto(kDepth);
  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  // Should parse correctly.
  EXPECT_TRUE(p.ParseFromString(serialized));

  // Overwriting the last byte to 0xFF results in malformed wire.
  serialized[serialized.size() - 1] = 0xFF;
  EXPECT_FALSE(p.ParseFromString(serialized));

  // If the affected byte is inside a lazy message, we have no guarantee that it
  // serializes into error free data because serialization needs to preserve
  // const correctness on lazy fields: `touch` all lazy fields.
  TouchLazy(&p);
  EXPECT_TRUE(q.ParseFromString(p.SerializeAsString()));
}

TEST(MESSAGE_TEST_NAME, ExceedRecursionLimit) {
  UNITTEST::NestedTestAllTypes o, p;
  const int kDepth = io::CodedInputStream::GetDefaultRecursionLimit() + 10;
  o = InitNestedProto(kDepth);
  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  // Recursion level deeper than the default.
  EXPECT_FALSE(p.ParseFromString(serialized));
}

TEST(MESSAGE_TEST_NAME, SupportCustomRecursionLimitRead) {
  UNITTEST::NestedTestAllTypes o, p;
  const int kDepth = io::CodedInputStream::GetDefaultRecursionLimit() + 10;
  o = InitNestedProto(kDepth);
  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  // Should pass with custom limit + reads.
  io::ArrayInputStream raw_input(serialized.data(), serialized.size());
  io::CodedInputStream input(&raw_input);
  input.SetRecursionLimit(kDepth + 10);
  EXPECT_TRUE(p.ParseFromCodedStream(&input));

  EXPECT_EQ(p.child().payload().optional_int32(), 0);
  EXPECT_EQ(p.child().child().payload().optional_int32(), 1);

  // Verify p serializes successfully (survives VerifyConsistency).
  std::string result;
  EXPECT_TRUE(p.SerializeToString(&result));
}

TEST(MESSAGE_TEST_NAME, SupportCustomRecursionLimitWrite) {
  UNITTEST::NestedTestAllTypes o, p;
  const int kDepth = io::CodedInputStream::GetDefaultRecursionLimit() + 10;
  o = InitNestedProto(kDepth);
  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  // Should pass with custom limit + writes.
  io::ArrayInputStream raw_input(serialized.data(), serialized.size());
  io::CodedInputStream input(&raw_input);
  input.SetRecursionLimit(kDepth + 10);
  EXPECT_TRUE(p.ParseFromCodedStream(&input));

  EXPECT_EQ(p.mutable_child()->mutable_payload()->optional_int32(), 0);
  EXPECT_EQ(
      p.mutable_child()->mutable_child()->mutable_payload()->optional_int32(),
      1);
}

// While deep recursion is never guaranteed, this test aims to catch potential
// issues with very deep recursion.
TEST(MESSAGE_TEST_NAME, SupportDeepRecursionLimit) {
  UNITTEST::NestedTestAllTypes o, p;
  constexpr int kDepth = 1000;
  auto* child = o.mutable_child();
  for (int i = 0; i < kDepth; i++) {
    child = child->mutable_child();
  }
  child->mutable_payload()->set_optional_int32(100);

  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  io::ArrayInputStream raw_input(serialized.data(), serialized.size());
  io::CodedInputStream input(&raw_input);
  input.SetRecursionLimit(1100);
  EXPECT_TRUE(p.ParseFromCodedStream(&input));
}

TEST(MESSAGE_TEST_NAME, Swap) {
  UNITTEST::NestedTestAllTypes o;
  constexpr int kDepth = 5;
  auto* child = o.mutable_child();
  for (int i = 0; i < kDepth; i++) {
    child = child->mutable_child();
  }
  TestUtil::SetAllFields(child->mutable_payload());

  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  {
    Arena arena;
    UNITTEST::NestedTestAllTypes* p1 =
        Arena::CreateMessage<UNITTEST::NestedTestAllTypes>(&arena);

    // Should parse correctly.
    EXPECT_TRUE(p1->ParseFromString(serialized));

    UNITTEST::NestedTestAllTypes* p2 =
        Arena::CreateMessage<UNITTEST::NestedTestAllTypes>(&arena);

    p1->Swap(p2);

    EXPECT_EQ(o.SerializeAsString(), p2->SerializeAsString());
  }
}

TEST(MESSAGE_TEST_NAME, BypassInitializationCheckOnParse) {
  UNITTEST::TestRequired message;
  io::ArrayInputStream raw_input(nullptr, 0);
  io::CodedInputStream input(&raw_input);
  EXPECT_TRUE(message.MergePartialFromCodedStream(&input));
}

TEST(MESSAGE_TEST_NAME, InitializationErrorString) {
  UNITTEST::TestRequired message;
  EXPECT_EQ("a, b, c", message.InitializationErrorString());
}

TEST(MESSAGE_TEST_NAME, DynamicCastToGenerated) {
  UNITTEST::TestAllTypes test_all_types;

  Message* test_all_types_pointer = &test_all_types;
  EXPECT_EQ(&test_all_types, DynamicCastToGenerated<UNITTEST::TestAllTypes>(
                                 test_all_types_pointer));
  EXPECT_EQ(nullptr, DynamicCastToGenerated<UNITTEST::TestRequired>(
                         test_all_types_pointer));

  const Message* test_all_types_pointer_const = &test_all_types;
  EXPECT_EQ(&test_all_types,
            DynamicCastToGenerated<const UNITTEST::TestAllTypes>(
                test_all_types_pointer_const));
  EXPECT_EQ(nullptr, DynamicCastToGenerated<const UNITTEST::TestRequired>(
                         test_all_types_pointer_const));

  Message* test_all_types_pointer_nullptr = nullptr;
  EXPECT_EQ(nullptr, DynamicCastToGenerated<UNITTEST::TestAllTypes>(
                         test_all_types_pointer_nullptr));
}

#if GTEST_HAS_DEATH_TEST  // death tests do not work on Windows yet.

TEST(MESSAGE_TEST_NAME, SerializeFailsIfNotInitialized) {
  UNITTEST::TestRequired message;
  std::string data;
  EXPECT_DEBUG_DEATH(
      EXPECT_TRUE(message.SerializeToString(&data)),
      absl::StrCat("Can't serialize message of type \"", UNITTEST_PACKAGE_NAME,
                   ".TestRequired\" because "
                   "it is missing required fields: a, b, c"));
}

TEST(MESSAGE_TEST_NAME, CheckInitialized) {
  UNITTEST::TestRequired message;
  EXPECT_DEATH(message.CheckInitialized(),
               absl::StrCat("Message of type \"", UNITTEST_PACKAGE_NAME,
                            ".TestRequired\" is missing required "
                            "fields: a, b, c"));
}

#endif  // GTEST_HAS_DEATH_TEST

namespace {
// An input stream that repeats a std::string's content for a number of times.
// It helps us create a really large input without consuming too much memory.
// Used to test the parsing behavior when the input size exceeds 2G or close to
// it.
class RepeatedInputStream : public io::ZeroCopyInputStream {
 public:
  RepeatedInputStream(const std::string& data, size_t count)
      : data_(data), count_(count), position_(0), total_byte_count_(0) {}

  bool Next(const void** data, int* size) override {
    if (position_ == data_.size()) {
      if (--count_ == 0) {
        return false;
      }
      position_ = 0;
    }
    *data = &data_[position_];
    *size = static_cast<int>(data_.size() - position_);
    position_ = data_.size();
    total_byte_count_ += *size;
    return true;
  }

  void BackUp(int count) override {
    position_ -= static_cast<size_t>(count);
    total_byte_count_ -= count;
  }

  bool Skip(int count) override {
    while (count > 0) {
      const void* data;
      int size;
      if (!Next(&data, &size)) {
        break;
      }
      if (size >= count) {
        BackUp(size - count);
        return true;
      } else {
        count -= size;
      }
    }
    return false;
  }

  int64_t ByteCount() const override { return total_byte_count_; }

 private:
  std::string data_;
  size_t count_;     // The number of strings that haven't been consumed.
  size_t position_;  // Position in the std::string for the next read.
  int64_t total_byte_count_;
};
}  // namespace

TEST(MESSAGE_TEST_NAME, TestParseMessagesCloseTo2G) {
  constexpr int32_t kint32max = std::numeric_limits<int32_t>::max();

  // Create a message with a large std::string field.
  std::string value = std::string(64 * 1024 * 1024, 'x');
  UNITTEST::TestAllTypes message;
  message.set_optional_string(value);

  // Repeat this message in the input stream to make the total input size
  // close to 2G.
  std::string data = message.SerializeAsString();
  size_t count = static_cast<size_t>(kint32max) / data.size();
  RepeatedInputStream input(data, count);

  // The parsing should succeed.
  UNITTEST::TestAllTypes result;
  EXPECT_TRUE(result.ParseFromZeroCopyStream(&input));

  // When there are multiple occurrences of a singular field, the last one
  // should win.
  EXPECT_EQ(value, result.optional_string());
}

TEST(MESSAGE_TEST_NAME, TestParseMessagesOver2G) {
  constexpr int32_t kint32max = std::numeric_limits<int32_t>::max();

  // Create a message with a large std::string field.
  std::string value = std::string(64 * 1024 * 1024, 'x');
  UNITTEST::TestAllTypes message;
  message.set_optional_string(value);

  // Repeat this message in the input stream to make the total input size
  // larger than 2G.
  std::string data = message.SerializeAsString();
  size_t count = static_cast<size_t>(kint32max) / data.size() + 1;
  RepeatedInputStream input(data, count);

  // The parsing should fail.
  UNITTEST::TestAllTypes result;
  EXPECT_FALSE(result.ParseFromZeroCopyStream(&input));
}

TEST(MESSAGE_TEST_NAME, BypassInitializationCheckOnSerialize) {
  UNITTEST::TestRequired message;
  io::ArrayOutputStream raw_output(nullptr, 0);
  io::CodedOutputStream output(&raw_output);
  EXPECT_TRUE(message.SerializePartialToCodedStream(&output));
}

TEST(MESSAGE_TEST_NAME, FindInitializationErrors) {
  UNITTEST::TestRequired message;
  std::vector<std::string> errors;
  message.FindInitializationErrors(&errors);
  ASSERT_EQ(3, errors.size());
  EXPECT_EQ("a", errors[0]);
  EXPECT_EQ("b", errors[1]);
  EXPECT_EQ("c", errors[2]);
}

TEST(MESSAGE_TEST_NAME, ReleaseMustUseResult) {
  UNITTEST::TestAllTypes message;
  auto* f = new UNITTEST::ForeignMessage();
  f->set_c(1000);
  message.set_allocated_optional_foreign_message(f);
  auto* mf = message.mutable_optional_foreign_message();
  EXPECT_EQ(mf, f);
  std::unique_ptr<UNITTEST::ForeignMessage> rf(
      message.release_optional_foreign_message());
  EXPECT_NE(rf.get(), nullptr);
}

TEST(MESSAGE_TEST_NAME, ParseFailsOnInvalidMessageEnd) {
  UNITTEST::TestAllTypes message;

  // Control case.
  EXPECT_TRUE(message.ParseFromArray("", 0));

  // The byte is a valid varint, but not a valid tag (zero).
  EXPECT_FALSE(message.ParseFromArray("\0", 1));

  // The byte is a malformed varint.
  EXPECT_FALSE(message.ParseFromArray("\200", 1));

  // The byte is an endgroup tag, but we aren't parsing a group.
  EXPECT_FALSE(message.ParseFromArray("\014", 1));
}

// Regression test for b/23630858
TEST(MESSAGE_TEST_NAME, MessageIsStillValidAfterParseFails) {
  UNITTEST::TestAllTypes message;

  // 9 0xFFs for the "optional_uint64" field.
  std::string invalid_data = "\x20\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";

  EXPECT_FALSE(message.ParseFromString(invalid_data));
  message.Clear();
  EXPECT_EQ(0, message.optional_uint64());

  // invalid data for field "optional_string". Length prefix is 1 but no
  // payload.
  std::string invalid_string_data = "\x72\x01";
  {
    Arena arena;
    UNITTEST::TestAllTypes* arena_message =
        Arena::CreateMessage<UNITTEST::TestAllTypes>(&arena);
    EXPECT_FALSE(arena_message->ParseFromString(invalid_string_data));
    arena_message->Clear();
    EXPECT_EQ("", arena_message->optional_string());
  }
}


namespace {

void ExpectMessageMerged(const UNITTEST::TestAllTypes& message) {
  EXPECT_EQ(3, message.optional_int32());
  EXPECT_EQ(2, message.optional_int64());
  EXPECT_EQ("hello", message.optional_string());
}

void AssignParsingMergeMessages(UNITTEST::TestAllTypes* msg1,
                                UNITTEST::TestAllTypes* msg2,
                                UNITTEST::TestAllTypes* msg3) {
  msg1->set_optional_int32(1);
  msg2->set_optional_int64(2);
  msg3->set_optional_int32(3);
  msg3->set_optional_string("hello");
}

}  // namespace

// Test that if an optional or required message/group field appears multiple
// times in the input, they need to be merged.
TEST(MESSAGE_TEST_NAME, ParsingMerge) {
  UNITTEST::TestParsingMerge::RepeatedFieldsGenerator generator;
  UNITTEST::TestAllTypes* msg1;
  UNITTEST::TestAllTypes* msg2;
  UNITTEST::TestAllTypes* msg3;

#define ASSIGN_REPEATED_FIELD(FIELD) \
  msg1 = generator.add_##FIELD();    \
  msg2 = generator.add_##FIELD();    \
  msg3 = generator.add_##FIELD();    \
  AssignParsingMergeMessages(msg1, msg2, msg3)

  ASSIGN_REPEATED_FIELD(field1);
  ASSIGN_REPEATED_FIELD(field2);
  ASSIGN_REPEATED_FIELD(field3);
  ASSIGN_REPEATED_FIELD(ext1);
  ASSIGN_REPEATED_FIELD(ext2);

#undef ASSIGN_REPEATED_FIELD
#define ASSIGN_REPEATED_GROUP(FIELD)                \
  msg1 = generator.add_##FIELD()->mutable_field1(); \
  msg2 = generator.add_##FIELD()->mutable_field1(); \
  msg3 = generator.add_##FIELD()->mutable_field1(); \
  AssignParsingMergeMessages(msg1, msg2, msg3)

  ASSIGN_REPEATED_GROUP(group1);
  ASSIGN_REPEATED_GROUP(group2);

#undef ASSIGN_REPEATED_GROUP

  std::string buffer;
  generator.SerializeToString(&buffer);
  UNITTEST::TestParsingMerge parsing_merge;
  parsing_merge.ParseFromString(buffer);

  // Required and optional fields should be merged.
  ExpectMessageMerged(parsing_merge.required_all_types());
  ExpectMessageMerged(parsing_merge.optional_all_types());
  ExpectMessageMerged(parsing_merge.optionalgroup().optional_group_all_types());
  ExpectMessageMerged(
      parsing_merge.GetExtension(UNITTEST::TestParsingMerge::optional_ext));

  // Repeated fields should not be merged.
  EXPECT_EQ(3, parsing_merge.repeated_all_types_size());
  EXPECT_EQ(3, parsing_merge.repeatedgroup_size());
  EXPECT_EQ(
      3, parsing_merge.ExtensionSize(UNITTEST::TestParsingMerge::repeated_ext));
}

TEST(MESSAGE_TEST_NAME, MergeFrom) {
  UNITTEST::TestAllTypes source, dest;

  // Optional fields
  source.set_optional_int32(1);  // only source
  source.set_optional_int64(2);  // both source and dest
  dest.set_optional_int64(3);
  dest.set_optional_uint32(4);  // only dest

  // Optional fields with defaults
  source.set_default_int32(13);  // only source
  source.set_default_int64(14);  // both source and dest
  dest.set_default_int64(15);
  dest.set_default_uint32(16);  // only dest

  // Repeated fields
  source.add_repeated_int32(5);  // only source
  source.add_repeated_int32(6);
  source.add_repeated_int64(7);  // both source and dest
  source.add_repeated_int64(8);
  dest.add_repeated_int64(9);
  dest.add_repeated_int64(10);
  dest.add_repeated_uint32(11);  // only dest
  dest.add_repeated_uint32(12);

  dest.MergeFrom(source);

  // Optional fields: source overwrites dest if source is specified
  EXPECT_EQ(1, dest.optional_int32());   // only source: use source
  EXPECT_EQ(2, dest.optional_int64());   // source and dest: use source
  EXPECT_EQ(4, dest.optional_uint32());  // only dest: use dest
  EXPECT_EQ(0, dest.optional_uint64());  // neither: use default

  // Optional fields with defaults
  EXPECT_EQ(13, dest.default_int32());   // only source: use source
  EXPECT_EQ(14, dest.default_int64());   // source and dest: use source
  EXPECT_EQ(16, dest.default_uint32());  // only dest: use dest
  EXPECT_EQ(44, dest.default_uint64());  // neither: use default

  // Repeated fields: concatenate source onto the end of dest
  ASSERT_EQ(2, dest.repeated_int32_size());
  EXPECT_EQ(5, dest.repeated_int32(0));
  EXPECT_EQ(6, dest.repeated_int32(1));
  ASSERT_EQ(4, dest.repeated_int64_size());
  EXPECT_EQ(9, dest.repeated_int64(0));
  EXPECT_EQ(10, dest.repeated_int64(1));
  EXPECT_EQ(7, dest.repeated_int64(2));
  EXPECT_EQ(8, dest.repeated_int64(3));
  ASSERT_EQ(2, dest.repeated_uint32_size());
  EXPECT_EQ(11, dest.repeated_uint32(0));
  EXPECT_EQ(12, dest.repeated_uint32(1));
  ASSERT_EQ(0, dest.repeated_uint64_size());
}

TEST(MESSAGE_TEST_NAME, IsInitialized) {
  UNITTEST::TestIsInitialized msg;
  EXPECT_TRUE(msg.IsInitialized());
  UNITTEST::TestIsInitialized::SubMessage* sub_message =
      msg.mutable_sub_message();
  EXPECT_TRUE(msg.IsInitialized());
  UNITTEST::TestIsInitialized::SubMessage::SubGroup* sub_group =
      sub_message->mutable_subgroup();
  EXPECT_FALSE(msg.IsInitialized());
  sub_group->set_i(1);
  EXPECT_TRUE(msg.IsInitialized());
}

TEST(MESSAGE_TEST_NAME, IsInitializedSplitBytestream) {
  UNITTEST::TestRequired ab, c;
  ab.set_a(1);
  ab.set_b(2);
  c.set_c(3);

  // The protobuf represented by the concatenated string has all required
  // fields (a,b,c) set.
  std::string bytes =
      ab.SerializePartialAsString() + c.SerializePartialAsString();

  UNITTEST::TestRequired concatenated;
  EXPECT_TRUE(concatenated.ParsePartialFromString(bytes));
  EXPECT_TRUE(concatenated.IsInitialized());

  UNITTEST::TestRequiredForeign fab, fc;
  fab.mutable_optional_message()->set_a(1);
  fab.mutable_optional_message()->set_b(2);
  fc.mutable_optional_message()->set_c(3);

  bytes = fab.SerializePartialAsString() + fc.SerializePartialAsString();

  UNITTEST::TestRequiredForeign fconcatenated;
  EXPECT_TRUE(fconcatenated.ParsePartialFromString(bytes));
  EXPECT_TRUE(fconcatenated.IsInitialized());
}

TEST(MESSAGE_FACTORY_TEST_NAME, GeneratedFactoryLookup) {
  EXPECT_EQ(MessageFactory::generated_factory()->GetPrototype(
                UNITTEST::TestAllTypes::descriptor()),
            &UNITTEST::TestAllTypes::default_instance());
}

TEST(MESSAGE_FACTORY_TEST_NAME, GeneratedFactoryUnknownType) {
  // Construct a new descriptor.
  DescriptorPool pool;
  FileDescriptorProto file;
  file.set_name("foo.proto");
  file.add_message_type()->set_name("Foo");
  const Descriptor* descriptor = pool.BuildFile(file)->message_type(0);

  // Trying to construct it should return nullptr.
  EXPECT_TRUE(MessageFactory::generated_factory()->GetPrototype(descriptor) ==
              nullptr);
}

TEST(MESSAGE_TEST_NAME, MOMIParserEdgeCases) {
  {
    UNITTEST::TestAllTypes msg;
    // Parser ends in last 16 bytes of buffer due to a 0.
    std::string data;
    // 12 bytes of data
    for (int i = 0; i < 4; i++) absl::StrAppend(&data, "\370\1\1");
    // 13 byte is terminator
    data += '\0';  // Terminator
    // followed by the rest of the stream
    // space is ascii 32 so no end group
    data += std::string(30, ' ');
    io::ArrayInputStream zcis(data.data(), data.size(), 17);
    io::CodedInputStream cis(&zcis);
    EXPECT_TRUE(msg.MergePartialFromCodedStream(&cis));
    EXPECT_EQ(cis.CurrentPosition(), 3 * 4 + 1);
  }
  {
    // Parser ends in last 16 bytes of buffer due to a end-group.
    // Must use a message that is a group. Otherwise ending on a group end is
    // a failure.
    UNITTEST::TestAllTypes::OptionalGroup msg;
    std::string data;
    for (int i = 0; i < 3; i++) absl::StrAppend(&data, "\370\1\1");
    data += '\14';  // Octal end-group tag 12 (1 * 8 + 4(
    data += std::string(30, ' ');
    io::ArrayInputStream zcis(data.data(), data.size(), 17);
    io::CodedInputStream cis(&zcis);
    EXPECT_TRUE(msg.MergePartialFromCodedStream(&cis));
    EXPECT_EQ(cis.CurrentPosition(), 3 * 3 + 1);
    EXPECT_TRUE(cis.LastTagWas(12));
  }
  {
    // Parser ends in last 16 bytes of buffer due to a end-group. But is inside
    // a length delimited field.
    // a failure.
    UNITTEST::TestAllTypes::OptionalGroup msg;
    std::string data = "\22\3foo";
    data += '\14';  // Octal end-group tag 12 (1 * 8 + 4(
    data += std::string(30, ' ');
    io::ArrayInputStream zcis(data.data(), data.size(), 17);
    io::CodedInputStream cis(&zcis);
    EXPECT_TRUE(msg.MergePartialFromCodedStream(&cis));
    EXPECT_EQ(cis.CurrentPosition(), 6);
    EXPECT_TRUE(cis.LastTagWas(12));
  }
  {
    // Parser fails when ending on 0 if from ZeroCopyInputStream
    UNITTEST::TestAllTypes msg;
    std::string data;
    // 12 bytes of data
    for (int i = 0; i < 4; i++) absl::StrAppend(&data, "\370\1\1");
    // 13 byte is terminator
    data += '\0';  // Terminator
    data += std::string(30, ' ');
    io::ArrayInputStream zcis(data.data(), data.size(), 17);
    EXPECT_FALSE(msg.ParsePartialFromZeroCopyStream(&zcis));
  }
}


TEST(MESSAGE_TEST_NAME, CheckSerializationWhenInterleavedExtensions) {
  UNITTEST::TestExtensionRangeSerialize in_message;

  in_message.set_foo_one(1);
  in_message.set_foo_two(2);
  in_message.set_foo_three(3);
  in_message.set_foo_four(4);

  in_message.SetExtension(UNITTEST::TestExtensionRangeSerialize::bar_one, 1);
  in_message.SetExtension(UNITTEST::TestExtensionRangeSerialize::bar_two, 2);
  in_message.SetExtension(UNITTEST::TestExtensionRangeSerialize::bar_three, 3);
  in_message.SetExtension(UNITTEST::TestExtensionRangeSerialize::bar_four, 4);
  in_message.SetExtension(UNITTEST::TestExtensionRangeSerialize::bar_five, 5);

  std::string buffer;
  in_message.SerializeToString(&buffer);

  UNITTEST::TestExtensionRangeSerialize out_message;
  out_message.ParseFromString(buffer);

  EXPECT_EQ(1, out_message.foo_one());
  EXPECT_EQ(2, out_message.foo_two());
  EXPECT_EQ(3, out_message.foo_three());
  EXPECT_EQ(4, out_message.foo_four());

  EXPECT_EQ(1, out_message.GetExtension(
                   UNITTEST::TestExtensionRangeSerialize::bar_one));
  EXPECT_EQ(2, out_message.GetExtension(
                   UNITTEST::TestExtensionRangeSerialize::bar_two));
  EXPECT_EQ(3, out_message.GetExtension(
                   UNITTEST::TestExtensionRangeSerialize::bar_three));
  EXPECT_EQ(4, out_message.GetExtension(
                   UNITTEST::TestExtensionRangeSerialize::bar_four));
  EXPECT_EQ(5, out_message.GetExtension(
                   UNITTEST::TestExtensionRangeSerialize::bar_five));
}

TEST(MESSAGE_TEST_NAME, PreservesFloatingPointNegative0) {
  UNITTEST::TestAllTypes in_message;
  in_message.set_optional_float(-0.0f);
  in_message.set_optional_double(-0.0);
  std::string serialized;
  EXPECT_TRUE(in_message.SerializeToString(&serialized));
  UNITTEST::TestAllTypes out_message;
  EXPECT_TRUE(out_message.ParseFromString(serialized));
  EXPECT_EQ(in_message.optional_float(), out_message.optional_float());
  EXPECT_EQ(std::signbit(in_message.optional_float()),
            std::signbit(out_message.optional_float()));
  EXPECT_EQ(in_message.optional_double(), out_message.optional_double());
  EXPECT_EQ(std::signbit(in_message.optional_double()),
            std::signbit(out_message.optional_double()));
}

TEST(MESSAGE_TEST_NAME,
     RegressionTestForParseMessageReadingUninitializedLimit) {
  UNITTEST::TestAllTypes in_message;
  in_message.mutable_optional_nested_message();
  std::string serialized = in_message.SerializeAsString();
  // We expect this to have 3 bytes: two for the tag, and one for the zero size.
  // Break the size by making it overlong.
  ASSERT_EQ(serialized.size(), 3);
  serialized.back() = '\200';
  serialized += std::string(10, '\200');
  EXPECT_FALSE(in_message.ParseFromString(serialized));
}

TEST(MESSAGE_TEST_NAME,
     RegressionTestForParseMessageWithSizeBeyondInputFailsToPopLimit) {
  UNITTEST::TestAllTypes in_message;
  in_message.mutable_optional_nested_message();
  std::string serialized = in_message.SerializeAsString();
  // We expect this to have 3 bytes: two for the tag, and one for the zero size.
  // Make the size a valid varint, but it overflows in the input.
  ASSERT_EQ(serialized.size(), 3);
  serialized.back() = 10;
  EXPECT_FALSE(in_message.ParseFromString(serialized));
}

const uint8_t* SkipTag(const uint8_t* buf) {
  while (*buf & 0x80) ++buf;
  ++buf;
  return buf;
}

// Adds `non_canonical_bytes` bytes to the varint representation at the tail of
// the buffer.
// `buf` points to the start of the buffer, `p` points to one-past-the-end.
// Returns the new one-past-the-end pointer.
uint8_t* AddNonCanonicalBytes(const uint8_t* buf, uint8_t* p,
                              int non_canonical_bytes) {
  // varint can have a max of 10 bytes.
  while (non_canonical_bytes-- > 0 && p - buf < 10) {
    // Add a dummy byte at the end.
    p[-1] |= 0x80;
    p[0] = 0;
    ++p;
  }
  return p;
}

std::string EncodeEnumValue(int number, int value, int non_canonical_bytes,
                            bool use_packed) {
  uint8_t buf[100];
  uint8_t* p = buf;

  if (use_packed) {
    p = internal::WireFormatLite::WriteEnumNoTagToArray(value, p);
    p = AddNonCanonicalBytes(buf, p, non_canonical_bytes);

    std::string payload(buf, p);
    p = buf;
    p = internal::WireFormatLite::WriteStringToArray(number, payload, p);
    return std::string(buf, p);

  } else {
    p = internal::WireFormatLite::WriteEnumToArray(number, value, p);
    p = AddNonCanonicalBytes(SkipTag(buf), p, non_canonical_bytes);
    return std::string(buf, p);
  }
}

std::string EncodeOverlongEnum(int number, bool use_packed) {
  uint8_t buf[100];
  uint8_t* p = buf;

  std::string overlong(16, static_cast<char>(0x80));
  if (use_packed) {
    p = internal::WireFormatLite::WriteStringToArray(number, overlong, p);
    return std::string(buf, p);
  } else {
    p = internal::WireFormatLite::WriteTagToArray(
        number, internal::WireFormatLite::WIRETYPE_VARINT, p);
    p = std::copy(overlong.begin(), overlong.end(), p);
    return std::string(buf, p);
  }
}

std::string EncodeInt32Value(int number, int32_t value,
                             int non_canonical_bytes) {
  uint8_t buf[100];
  uint8_t* p = buf;

  p = internal::WireFormatLite::WriteInt32ToArray(number, value, p);
  p = AddNonCanonicalBytes(SkipTag(buf), p, non_canonical_bytes);
  return std::string(buf, p);
}

std::string EncodeInt64Value(int number, int64_t value, int non_canonical_bytes,
                             bool use_packed = false) {
  uint8_t buf[100];
  uint8_t* p = buf;

  if (use_packed) {
    p = internal::WireFormatLite::WriteInt64NoTagToArray(value, p);
    p = AddNonCanonicalBytes(buf, p, non_canonical_bytes);

    std::string payload(buf, p);
    p = buf;
    p = internal::WireFormatLite::WriteStringToArray(number, payload, p);
    return std::string(buf, p);

  } else {
    p = internal::WireFormatLite::WriteInt64ToArray(number, value, p);
    p = AddNonCanonicalBytes(SkipTag(buf), p, non_canonical_bytes);
    return std::string(buf, p);
  }
}

std::string EncodeOtherField() {
  UNITTEST::EnumParseTester obj;
  obj.set_other_field(1);
  return obj.SerializeAsString();
}

template <typename T>
static std::vector<const FieldDescriptor*> GetFields() {
  auto* descriptor = T::descriptor();
  std::vector<const FieldDescriptor*> fields;
  for (int i = 0; i < descriptor->field_count(); ++i) {
    fields.push_back(descriptor->field(i));
  }
  for (int i = 0; i < descriptor->extension_count(); ++i) {
    fields.push_back(descriptor->extension(i));
  }
  return fields;
}

TEST(MESSAGE_TEST_NAME, TestEnumParsers) {
  UNITTEST::EnumParseTester obj;

  const auto other_field = EncodeOtherField();

  // Encode an enum field for many different cases and verify that it can be
  // parsed as expected.
  // There are:
  //  - optional/repeated/packed fields
  //  - field tags that encode in 1/2/3 bytes
  //  - canonical and non-canonical encodings of the varint
  //  - last vs not last field
  //  - label combinations to trigger different parsers: sequential, small
  //  sequential, non-validated.

  const std::vector<const FieldDescriptor*> fields =
      GetFields<UNITTEST::EnumParseTester>();

  constexpr int kInvalidValue = 0x900913;
  auto* ref = obj.GetReflection();
  auto* descriptor = obj.descriptor();
  for (bool use_packed : {false, true}) {
    SCOPED_TRACE(use_packed);
    for (bool use_tail_field : {false, true}) {
      SCOPED_TRACE(use_tail_field);
      for (int non_canonical_bytes = 0; non_canonical_bytes < 9;
           ++non_canonical_bytes) {
        SCOPED_TRACE(non_canonical_bytes);
        for (bool add_garbage_bits : {false, true}) {
          if (add_garbage_bits && non_canonical_bytes != 9) {
            // We only add garbage on the 10th byte.
            continue;
          }
          SCOPED_TRACE(add_garbage_bits);
          for (auto field : fields) {
            if (field->name() == "other_field") continue;
            if (!field->is_repeated() && use_packed) continue;
            SCOPED_TRACE(field->full_name());
            const auto* enum_desc = field->enum_type();
            for (int e = 0; e < enum_desc->value_count(); ++e) {
              const auto* value_desc = enum_desc->value(e);
              if (value_desc->number() < 0 && non_canonical_bytes > 0) {
                // Negative numbers only have a canonical representation.
                continue;
              }
              SCOPED_TRACE(value_desc->number());
              ABSL_CHECK_NE(value_desc->number(), kInvalidValue)
                  << "Invalid value is a real label.";
              auto encoded =
                  EncodeEnumValue(field->number(), value_desc->number(),
                                  non_canonical_bytes, use_packed);
              if (add_garbage_bits) {
                // These bits should be discarded even in the `false` case.
                encoded.back() |= 0b0111'1110;
              }
              if (use_tail_field) {
                // Make sure that fields after this one can be parsed too. ie
                // test that the "next" jump is correct too.
                encoded += other_field;
              }

              EXPECT_TRUE(obj.ParseFromString(encoded));
              if (field->is_repeated()) {
                ASSERT_EQ(ref->FieldSize(obj, field), 1);
                EXPECT_EQ(ref->GetRepeatedEnumValue(obj, field, 0),
                          value_desc->number());
              } else {
                EXPECT_TRUE(ref->HasField(obj, field));
                EXPECT_EQ(ref->GetEnumValue(obj, field), value_desc->number());
              }
              auto& unknown = ref->GetUnknownFields(obj);
              ASSERT_EQ(unknown.field_count(), 0);
            }

            {
              SCOPED_TRACE("Invalid value");
              // Try an invalid value, which should go to the unknown fields.
              EXPECT_TRUE(obj.ParseFromString(
                  EncodeEnumValue(field->number(), kInvalidValue,
                                  non_canonical_bytes, use_packed)));
              if (field->is_repeated()) {
                ASSERT_EQ(ref->FieldSize(obj, field), 0);
              } else {
                EXPECT_FALSE(ref->HasField(obj, field));
                EXPECT_EQ(ref->GetEnumValue(obj, field),
                          enum_desc->value(0)->number());
              }
              auto& unknown = ref->GetUnknownFields(obj);
              ASSERT_EQ(unknown.field_count(), 1);
              EXPECT_EQ(unknown.field(0).number(), field->number());
              EXPECT_EQ(unknown.field(0).type(), unknown.field(0).TYPE_VARINT);
              EXPECT_EQ(unknown.field(0).varint(), kInvalidValue);
            }
            {
              SCOPED_TRACE("Overlong varint");
              // Try an overlong varint. It should fail parsing, but not trigger
              // any sanitizer warning.
              EXPECT_FALSE(obj.ParseFromString(
                  EncodeOverlongEnum(field->number(), use_packed)));
            }
          }
        }
      }
    }
  }
}

TEST(MESSAGE_TEST_NAME, TestEnumParserForUnknownEnumValue) {
  DynamicMessageFactory factory;
  std::unique_ptr<Message> dynamic(
      factory.GetPrototype(UNITTEST::EnumParseTester::descriptor())->New());

  UNITTEST::EnumParseTester non_dynamic;

  // For unknown enum values, for consistency we must include the
  // int32_t enum value in the unknown field set, which might not be exactly the
  // same as the input.
  auto* descriptor = non_dynamic.descriptor();

  const std::vector<const FieldDescriptor*> fields =
      GetFields<UNITTEST::EnumParseTester>();

  for (bool use_dynamic : {false, true}) {
    SCOPED_TRACE(use_dynamic);
    for (auto field : fields) {
      if (field->name() == "other_field") continue;
      SCOPED_TRACE(field->full_name());
      for (bool use_packed : {false, true}) {
        SCOPED_TRACE(use_packed);
        if (!field->is_repeated() && use_packed) continue;

        // -2 is an invalid enum value on all the tests here.
        // We will encode -2 as a positive int64 that is equivalent to
        // int32_t{-2} when truncated.
        constexpr int64_t minus_2_non_canonical =
            static_cast<int64_t>(static_cast<uint32_t>(int32_t{-2}));
        static_assert(minus_2_non_canonical != -2, "");
        std::string encoded = EncodeInt64Value(
            field->number(), minus_2_non_canonical, 0, use_packed);

        auto& obj = use_dynamic ? *dynamic : non_dynamic;
        ASSERT_TRUE(obj.ParseFromString(encoded));

        auto& unknown = obj.GetReflection()->GetUnknownFields(obj);
        ASSERT_EQ(unknown.field_count(), 1);
        EXPECT_EQ(unknown.field(0).number(), field->number());
        EXPECT_EQ(unknown.field(0).type(), unknown.field(0).TYPE_VARINT);
        EXPECT_EQ(unknown.field(0).varint(), int64_t{-2});
      }
    }
  }
}

std::string EncodeBoolValue(int number, bool value, int non_canonical_bytes) {
  uint8_t buf[100];
  uint8_t* p = buf;

  p = internal::WireFormatLite::WriteBoolToArray(number, value, p);
  p = AddNonCanonicalBytes(SkipTag(buf), p, non_canonical_bytes);
  return std::string(buf, p);
}

TEST(MESSAGE_TEST_NAME, TestBoolParsers) {
  UNITTEST::BoolParseTester obj;

  const auto other_field = EncodeOtherField();

  // Encode a boolean field for many different cases and verify that it can be
  // parsed as expected.
  // There are:
  //  - optional/repeated/packed fields
  //  - field tags that encode in 1/2/3 bytes
  //  - canonical and non-canonical encodings of the varint
  //  - last vs not last field

  const std::vector<const FieldDescriptor*> fields =
      GetFields<UNITTEST::BoolParseTester>();

  auto* ref = obj.GetReflection();
  auto* descriptor = obj.descriptor();
  for (bool use_tail_field : {false, true}) {
    SCOPED_TRACE(use_tail_field);
    for (int non_canonical_bytes = 0; non_canonical_bytes < 10;
         ++non_canonical_bytes) {
      SCOPED_TRACE(non_canonical_bytes);
      for (bool add_garbage_bits : {false, true}) {
        if (add_garbage_bits && non_canonical_bytes != 9) {
          // We only add garbage on the 10th byte.
          continue;
        }
        SCOPED_TRACE(add_garbage_bits);
        for (auto field : fields) {
          if (field->name() == "other_field") continue;
          SCOPED_TRACE(field->full_name());
          for (bool value : {false, true}) {
            SCOPED_TRACE(value);
            auto encoded =
                EncodeBoolValue(field->number(), value, non_canonical_bytes);
            if (add_garbage_bits) {
              // These bits should be discarded even in the `false` case.
              encoded.back() |= 0b0111'1110;
            }
            if (use_tail_field) {
              // Make sure that fields after this one can be parsed too. ie test
              // that the "next" jump is correct too.
              encoded += other_field;
            }

            EXPECT_TRUE(obj.ParseFromString(encoded));
            if (field->is_repeated()) {
              ASSERT_EQ(ref->FieldSize(obj, field), 1);
              EXPECT_EQ(ref->GetRepeatedBool(obj, field, 0), value);
            } else {
              EXPECT_TRUE(ref->HasField(obj, field));
              EXPECT_EQ(ref->GetBool(obj, field), value)
                  << testing::PrintToString(encoded);
            }
            auto& unknown = ref->GetUnknownFields(obj);
            ASSERT_EQ(unknown.field_count(), 0);
          }
        }
      }
    }
  }
}

TEST(MESSAGE_TEST_NAME, TestInt32Parsers) {
  UNITTEST::Int32ParseTester obj;

  const auto other_field = EncodeOtherField();

  // Encode an int32 field for many different cases and verify that it can be
  // parsed as expected.
  // There are:
  //  - optional/repeated/packed fields
  //  - field tags that encode in 1/2/3 bytes
  //  - canonical and non-canonical encodings of the varint
  //  - last vs not last field

  const std::vector<const FieldDescriptor*> fields =
      GetFields<UNITTEST::Int32ParseTester>();

  auto* ref = obj.GetReflection();
  auto* descriptor = obj.descriptor();
  for (bool use_tail_field : {false, true}) {
    SCOPED_TRACE(use_tail_field);
    for (int non_canonical_bytes = 0; non_canonical_bytes < 10;
         ++non_canonical_bytes) {
      SCOPED_TRACE(non_canonical_bytes);
      for (bool add_garbage_bits : {false, true}) {
        if (add_garbage_bits && non_canonical_bytes != 9) {
          // We only add garbage on the 10th byte.
          continue;
        }
        SCOPED_TRACE(add_garbage_bits);
        for (auto field : fields) {
          if (field->name() == "other_field") continue;
          SCOPED_TRACE(field->full_name());
          for (int32_t value : {1, 0, -1, (std::numeric_limits<int32_t>::min)(),
                              (std::numeric_limits<int32_t>::max)()}) {
            SCOPED_TRACE(value);
            auto encoded =
                EncodeInt32Value(field->number(), value, non_canonical_bytes);
            if (add_garbage_bits) {
              // These bits should be discarded even in the `false` case.
              encoded.back() |= 0b0111'1110;
            }
            if (use_tail_field) {
              // Make sure that fields after this one can be parsed too. ie test
              // that the "next" jump is correct too.
              encoded += other_field;
            }

            EXPECT_TRUE(obj.ParseFromString(encoded));
            if (field->is_repeated()) {
              ASSERT_EQ(ref->FieldSize(obj, field), 1);
              EXPECT_EQ(ref->GetRepeatedInt32(obj, field, 0), value);
            } else {
              EXPECT_TRUE(ref->HasField(obj, field));
              EXPECT_EQ(ref->GetInt32(obj, field), value)
                  << testing::PrintToString(encoded);
            }
            auto& unknown = ref->GetUnknownFields(obj);
            ASSERT_EQ(unknown.field_count(), 0);
          }
        }
      }
    }
  }
}

TEST(MESSAGE_TEST_NAME, TestInt64Parsers) {
  UNITTEST::Int64ParseTester obj;

  const auto other_field = EncodeOtherField();

  // Encode an int64 field for many different cases and verify that it can be
  // parsed as expected.
  // There are:
  //  - optional/repeated/packed fields
  //  - field tags that encode in 1/2/3 bytes
  //  - canonical and non-canonical encodings of the varint
  //  - last vs not last field

  const std::vector<const FieldDescriptor*> fields =
      GetFields<UNITTEST::Int64ParseTester>();

  auto* ref = obj.GetReflection();
  auto* descriptor = obj.descriptor();
  for (bool use_tail_field : {false, true}) {
    SCOPED_TRACE(use_tail_field);
    for (int non_canonical_bytes = 0; non_canonical_bytes < 10;
         ++non_canonical_bytes) {
      SCOPED_TRACE(non_canonical_bytes);
      for (bool add_garbage_bits : {false, true}) {
        if (add_garbage_bits && non_canonical_bytes != 9) {
          // We only add garbage on the 10th byte.
          continue;
        }
        SCOPED_TRACE(add_garbage_bits);
        for (auto field : fields) {
          if (field->name() == "other_field") continue;
          SCOPED_TRACE(field->full_name());
          for (int64_t value : {int64_t{1}, int64_t{0}, int64_t{-1},
                                (std::numeric_limits<int64_t>::min)(),
                                (std::numeric_limits<int64_t>::max)()}) {
            SCOPED_TRACE(value);
            auto encoded =
                EncodeInt64Value(field->number(), value, non_canonical_bytes);
            if (add_garbage_bits) {
              // These bits should be discarded even in the `false` case.
              encoded.back() |= 0b0111'1110;
            }
            if (use_tail_field) {
              // Make sure that fields after this one can be parsed too. ie test
              // that the "next" jump is correct too.
              encoded += other_field;
            }

            EXPECT_TRUE(obj.ParseFromString(encoded));
            if (field->is_repeated()) {
              ASSERT_EQ(ref->FieldSize(obj, field), 1);
              EXPECT_EQ(ref->GetRepeatedInt64(obj, field, 0), value);
            } else {
              EXPECT_TRUE(ref->HasField(obj, field));
              EXPECT_EQ(ref->GetInt64(obj, field), value)
                  << testing::PrintToString(encoded);
            }
            auto& unknown = ref->GetUnknownFields(obj);
            ASSERT_EQ(unknown.field_count(), 0);
          }
        }
      }
    }
  }
}

TEST(MESSAGE_TEST_NAME, IsDefaultInstance) {
  UNITTEST::TestAllTypes msg;
  const auto& default_msg = UNITTEST::TestAllTypes::default_instance();
  const auto* r = msg.GetReflection();
  EXPECT_TRUE(r->IsDefaultInstance(default_msg));
  EXPECT_FALSE(r->IsDefaultInstance(msg));
}

std::string EncodeStringValue(int number, const std::string& value) {
  uint8_t buf[100];
  return std::string(
      buf, internal::WireFormatLite::WriteStringToArray(number, value, buf));
}

class TestInputStream final : public io::ZeroCopyInputStream {
 public:
  explicit TestInputStream(absl::string_view payload, size_t break_pos)
      : payload_(payload), break_pos_(break_pos) {}

  bool Next(const void** data, int* size) override {
    if (payload_.empty()) return false;
    const auto to_consume = payload_.substr(0, break_pos_);
    *data = to_consume.data();
    *size = to_consume.size();
    payload_.remove_prefix(to_consume.size());
    // The next time will consume the rest.
    break_pos_ = payload_.npos;

    return true;
  }

  void BackUp(int) override { ABSL_CHECK(false); }
  bool Skip(int) override {
    ABSL_CHECK(false);
    return false;
  }
  int64_t ByteCount() const override {
    ABSL_CHECK(false);
    return 0;
  }

 private:
  absl::string_view payload_;
  size_t break_pos_;
};

template <typename T>
static const internal::TcParseTableBase* GetTableIfAvailable(...) {
  return nullptr;
}

template <typename T>
static const internal::TcParseTableBase* GetTableIfAvailable(
    decltype(internal::TcParser::GetTable<T>())) {
  return internal::TcParser::GetTable<T>();
}

TEST(MESSAGE_TEST_NAME, TestRegressionInlinedStringAuxIdxMismatchOnFastParser) {
  using Proto = UNITTEST::InlinedStringIdxRegressionProto;

  auto* table = GetTableIfAvailable<Proto>(nullptr);
  // Only test when TDP is on, and we have these fields inlined.
  if (table != nullptr &&
      table->fast_entry(1)->target() == internal::TcParser::FastSiS1) {
    // optional string str1 = 1;
    EXPECT_EQ(table->fast_entry(1)->bits.aux_idx(), 1);
    // optional InlinedStringIdxRegressionProto sub = 2;
    EXPECT_EQ(table->fast_entry(2)->bits.aux_idx(), 2);
    // optional string str2 = 3;
    // The aux_idx points to the inlined_string_idx and not the actual aux_idx.
    EXPECT_EQ(table->fast_entry(3)->bits.aux_idx(), 2);
    // optional string str3 = 4;
    // The aux_idx points to the inlined_string_idx and not the actual aux_idx.
    EXPECT_EQ(table->fast_entry(0)->bits.aux_idx(), 3);
  }

  std::string encoded;
  {
    Proto proto;
    // We use strings longer than SSO.
    proto.set_str1(std::string(100, 'a'));
    proto.set_str2(std::string(100, 'a'));
    proto.set_str3(std::string(100, 'a'));
    encoded = proto.SerializeAsString();
  }
  Arena arena;
  auto* proto = Arena::CreateMessage<Proto>(&arena);
  // We don't alter donation here, so it works even if the idx are bad.
  ASSERT_TRUE(proto->ParseFromString(encoded));
  // Now we alter donation bits. str2's bit (#2) will be off, but its aux_idx
  // (#3) will point to a donated string.
  proto = Arena::CreateMessage<Proto>(&arena);
  proto->mutable_str1();
  proto->mutable_str2();
  proto->mutable_str3();
  // With the bug, this breaks the cleanup list, causing UB on arena
  // destruction.
  ASSERT_TRUE(proto->ParseFromString(encoded));
}

TEST(MESSAGE_TEST_NAME, TestRepeatedStringParsers) {
  google::protobuf::Arena arena;

  const std::string sample =
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  const auto* const descriptor = UNITTEST::StringParseTester::descriptor();

  const std::vector<const FieldDescriptor*> fields =
      GetFields<UNITTEST::StringParseTester>();

  static const size_t sso_capacity = std::string().capacity();
  if (sso_capacity == 0) GTEST_SKIP();
  // SSO, !SSO, and off-by-one just in case
  for (size_t size :
       {sso_capacity - 1, sso_capacity, sso_capacity + 1, sso_capacity + 2}) {
    SCOPED_TRACE(size);
    const std::string value = sample.substr(0, size);
    for (auto field : fields) {
      SCOPED_TRACE(field->full_name());
      const auto encoded = EncodeStringValue(field->number(), sample) +
                           EncodeStringValue(field->number(), value);
      // Check for different breaks in the input stream to test cases where
      // the payload can be read and can't be read in one go.
      for (size_t i = 1; i <= encoded.size(); ++i) {
        TestInputStream input_stream(encoded, i);

        auto& obj = *arena.CreateMessage<UNITTEST::StringParseTester>(&arena);
        auto* ref = obj.GetReflection();
        EXPECT_TRUE(obj.ParseFromZeroCopyStream(&input_stream));
        if (field->is_repeated()) {
          ASSERT_EQ(ref->FieldSize(obj, field), 2);
          EXPECT_EQ(ref->GetRepeatedString(obj, field, 0), sample);
          EXPECT_EQ(ref->GetRepeatedString(obj, field, 1), value);
        } else {
          EXPECT_EQ(ref->GetString(obj, field), value);
        }
      }
    }
  }
}

TEST(MESSAGE_TEST_NAME, TestRegressionOnParseFailureNotSettingHasBits) {
  std::string single_field;
  // We use blocks because we want fully new instances of the proto. We are
  // testing .Clear(), so we can't use it to set up the test.
  {
    UNITTEST::TestAllTypes message;
    message.set_optional_int32(17);
    single_field = message.SerializeAsString();
  }
  const auto validate_message = [](auto& message) {
    if (!message.has_optional_int32()) {
      EXPECT_EQ(message.optional_int32(), 0);
    }
    message.Clear();
    EXPECT_FALSE(message.has_optional_int32());
    EXPECT_EQ(message.optional_int32(), 0);
  };
  {
    // Verify the setup is correct.
    UNITTEST::TestAllTypes message;
    EXPECT_FALSE(message.has_optional_int32());
    EXPECT_EQ(message.optional_int32(), 0);
    EXPECT_TRUE(message.ParseFromString(single_field));
    validate_message(message);
  }
  {
    // Run the regression.
    // These are the steps:
    // - The stream contains a fast field, and then a failure in MiniParse
    // - The parsing fails.
    // - We call clear.
    // - The fast field should be reset.
    UNITTEST::TestAllTypes message;
    EXPECT_FALSE(message.has_optional_int32());
    EXPECT_EQ(message.optional_int32(), 0);
    // The second tag will fail to parse because it has too many continuation
    // bits.
    auto with_error =
        absl::StrCat(single_field, std::string(100, static_cast<char>(0x80)));
    EXPECT_FALSE(message.ParseFromString(with_error));
    validate_message(message);
  }
}

}  // namespace protobuf
}  // namespace google
