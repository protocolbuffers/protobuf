// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/recording_test_runner.h"

#include <sstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/crc/crc32c.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/mock_test_runner.h"
#include "google/protobuf/test_textproto.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::conformance::ConformanceRequest;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StartsWith;

// Runs a single request through a RecordingTestRunner and returns what it
// recorded.
std::string Record(absl::string_view test_name, absl::string_view input) {
  NiceMock<MockTestRunner> delegate;
  std::ostringstream out;
  RecordingTestRunner(&delegate, &out).RunTest(test_name, input);
  return out.str();
}

// The hash RecordingTestRunner records for `input`: the last field of its line.
std::string RecordedHash(absl::string_view input) {
  std::string line = Record("Suite.Test", input);
  return line.substr(line.rfind(' ') + 1, 8);
}

ConformanceRequest SampleRequest() {
  ConformanceRequest request;
  request.set_protobuf_payload("ab");
  request.set_requested_output_format(::conformance::PROTOBUF);
  request.set_message_type("x");
  return request;
}

// The canonical rendering of SampleRequest(), whose CRC32C the runner records.
constexpr absl::string_view kSampleCanonical =
    "protobuf_payload: \"ab\"\n"
    "requested_output_format: 1\n"
    "message_type: \"x\"\n";

TEST(RecordingTestRunnerTest, WritesOneLinePerRequest) {
  const std::string sample = SampleRequest().SerializeAsString();
  NiceMock<MockTestRunner> delegate;
  std::ostringstream out;
  {
    RecordingTestRunner runner(&delegate, &out);
    runner.RunTest("Suite.First", sample);
    runner.RunTest("Suite.Empty", "");
  }

  // The size is that of the wire bytes, the CRC32C that of the canonical
  // rendering.
  EXPECT_EQ(out.str(), absl::StrCat("Suite.First ", sample.size(), " ",
                                    absl::ComputeCrc32c(kSampleCanonical), "\n",
                                    "Suite.Empty 0 00000000\n"));
}

TEST(RecordingTestRunnerTest, HashIsIndependentOfFieldOrder) {
  // SampleRequest() serializes its fields in field-number order; this is the
  // same message with them in the opposite order.
  Wire descending(
      LengthPrefixedField(ConformanceRequest::kMessageTypeFieldNumber, "x"),
      VarintField(ConformanceRequest::kRequestedOutputFormatFieldNumber,
                  ::conformance::PROTOBUF),
      LengthPrefixedField(ConformanceRequest::kProtobufPayloadFieldNumber,
                          "ab"));
  ConformanceRequest parsed;
  ASSERT_TRUE(parsed.ParseFromString(descending.data()));
  ASSERT_THAT(parsed, EqualsProto(kSampleCanonical));

  EXPECT_EQ(Record("Suite.Test", descending.data()),
            Record("Suite.Test", SampleRequest().SerializeAsString()));
  EXPECT_EQ(Record("Suite.Test", descending.data()), "Suite.Test 9 17dd210d\n");
}

TEST(RecordingTestRunnerTest, HashDistinguishesDifferentRequests) {
  ConformanceRequest other = SampleRequest();
  other.set_message_type("y");
  EXPECT_NE(RecordedHash(SampleRequest().SerializeAsString()),
            RecordedHash(other.SerializeAsString()));
}

TEST(RecordingTestRunnerTest, RendersEveryFieldTheRunnerSends) {
  // SampleRequest() covers protobuf_payload, requested_output_format and
  // message_type; these cover the rest.
  ConformanceRequest json;
  json.set_json_payload("{}");
  json.set_test_category(::conformance::JSON_TEST);
  json.set_print_unknown_fields(true);
  EXPECT_EQ(RecordedHash(json.SerializeAsString()),
            absl::StrCat(absl::ComputeCrc32c("json_payload: \"{}\"\n"
                                             "test_category: 2\n"
                                             "print_unknown_fields: 1\n")));

  ConformanceRequest jspb;
  jspb.set_jspb_payload("[]");
  EXPECT_EQ(RecordedHash(jspb.SerializeAsString()),
            absl::StrCat(absl::ComputeCrc32c("jspb_payload: \"[]\"\n")));

  ConformanceRequest text;
  text.set_text_payload("a: 1");
  EXPECT_EQ(RecordedHash(text.SerializeAsString()),
            absl::StrCat(absl::ComputeCrc32c("text_payload: \"a: 1\"\n")));
}

TEST(RecordingTestRunnerTest, LastOccurrenceOfAFieldWins) {
  // Like a parser, the runner keeps the last of two occurrences of a field.
  Wire twice(
      LengthPrefixedField(ConformanceRequest::kMessageTypeFieldNumber, "x"),
      LengthPrefixedField(ConformanceRequest::kMessageTypeFieldNumber, "y"));
  ConformanceRequest once;
  once.set_message_type("y");
  EXPECT_EQ(RecordedHash(twice.data()), RecordedHash(once.SerializeAsString()));
}

TEST(RecordingTestRunnerDeathTest, RejectsInputThatIsNotARequest) {
  EXPECT_DEATH(Record("Suite.Test", "a"),
               "not a serialized ConformanceRequest");
}

TEST(RecordingTestRunnerDeathTest, RejectsFieldsTheRunnerNeverSends) {
  ConformanceRequest request;
  request.mutable_jspb_encoding_options();
  EXPECT_DEATH(Record("Suite.Test", request.SerializeAsString()),
               "jspb_encoding_options");
  // ConformanceRequest has no field 999.
  EXPECT_DEATH(Record("Suite.Test", VarintField(999, 1).data()),
               "unknown ConformanceRequest field 999");
  // message_type with the wrong wire type parses as an unknown field.
  EXPECT_DEATH(
      Record(
          "Suite.Test",
          VarintField(ConformanceRequest::kMessageTypeFieldNumber, 1).data()),
      "unknown ConformanceRequest field 4");
}

TEST(RecordingTestRunnerTest, DelegatesAndPassesThroughResponse) {
  const std::string sample = SampleRequest().SerializeAsString();
  MockTestRunner delegate;
  std::ostringstream out;
  RecordingTestRunner runner(&delegate, &out);

  InSequence in_sequence;
  EXPECT_CALL(delegate, RunTest("Test.One", sample)).WillOnce(Return("one"));
  EXPECT_CALL(delegate, RunTest("Test.Two", "")).WillOnce(Return("two"));
  EXPECT_EQ(runner.RunTest("Test.One", sample), "one");
  EXPECT_EQ(runner.RunTest("Test.Two", ""), "two");
}

TEST(RecordingTestRunnerTest, RecordsBinaryInputByHashOnly) {
  ConformanceRequest request;
  request.set_protobuf_payload("\n\xff");

  // The payload's bytes (including the newline) must not leak into the line;
  // only the size and hash are recorded.
  EXPECT_EQ(Record("Suite.Binary", request.SerializeAsString()),
            absl::StrCat(
                "Suite.Binary ", request.ByteSizeLong(), " ",
                absl::ComputeCrc32c("protobuf_payload: \"\\n\\xff\"\n"), "\n"));
}

TEST(RecordingTestRunnerTest, RecordsBeforeDelegating) {
  MockTestRunner delegate;
  std::ostringstream out;
  RecordingTestRunner runner(&delegate, &out);

  // The line must already be on the stream when the delegate runs, so that a
  // crash inside the delegate cannot lose it.
  std::string recorded_before_delegating;
  EXPECT_CALL(delegate, RunTest)
      .WillOnce([&](absl::string_view, absl::string_view) {
        recorded_before_delegating = out.str();
        return std::string();
      });
  runner.RunTest("Suite.First", SampleRequest().SerializeAsString());

  EXPECT_EQ(recorded_before_delegating, out.str());
  EXPECT_THAT(out.str(), StartsWith("Suite.First "));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
