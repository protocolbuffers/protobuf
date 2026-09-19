// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "recording_test_runner.h"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "test_runner.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ElementsAre;
using ::testing::Pair;

// Fake delegate that records every call and returns a canned response derived
// from the arguments, so pass-through can be verified.
class FakeTestRunner : public ConformanceTestRunner {
 public:
  std::string RunTest(absl::string_view test_name,
                      absl::string_view input) override {
    calls_.emplace_back(std::string(test_name), std::string(input));
    return absl::StrCat("response:", test_name, ":", input);
  }

  const std::vector<std::pair<std::string, std::string>>& calls() const {
    return calls_;
  }

 private:
  std::vector<std::pair<std::string, std::string>> calls_;
};

TEST(Fnv1a64Test, KnownVectors) {
  EXPECT_EQ(internal::Fnv1a64(""), 0xcbf29ce484222325u);
  EXPECT_EQ(internal::Fnv1a64("a"), 0xaf63dc4c8601ec8cu);
  EXPECT_EQ(internal::Fnv1a64("foobar"), 0x85944171f73967e8u);
}

TEST(Fnv1a64Test, HandlesEmbeddedNulAndHighBytes) {
  // Bytes >= 0x80 must be treated as unsigned, and NUL must not terminate.
  const std::string with_nul("\0\xff", 2);
  EXPECT_NE(internal::Fnv1a64(with_nul), internal::Fnv1a64(""));
  EXPECT_NE(internal::Fnv1a64(with_nul), internal::Fnv1a64("\xff"));
}

// A ConformanceRequest with protobuf_payload (1) = "ab",
// requested_output_format (3) = PROTOBUF and message_type (4) = "x", serialized
// with its fields in ascending and in descending field-number order.  Both are
// valid encodings of the same message.
constexpr absl::string_view kAscending = "\x0a\x02\x61\x62\x18\x01\x22\x01\x78";
constexpr absl::string_view kDescending =
    "\x22\x01\x78\x18\x01\x0a\x02\x61\x62";

TEST(CanonicalizeRequestTest, IsIndependentOfFieldOrder) {
  EXPECT_EQ(internal::CanonicalizeRequest(kAscending),
            internal::CanonicalizeRequest(kDescending));
  EXPECT_EQ(internal::CanonicalizeRequest(kAscending),
            "protobuf_payload: \"ab\"\n"
            "requested_output_format: PROTOBUF\n"
            "message_type: \"x\"\n");
}

TEST(CanonicalizeRequestTest, DistinguishesDifferentRequests) {
  // Same as kAscending but with message_type = "y".
  constexpr absl::string_view kOther = "\x0a\x02\x61\x62\x18\x01\x22\x01\x79";
  EXPECT_NE(internal::CanonicalizeRequest(kAscending),
            internal::CanonicalizeRequest(kOther));
}

TEST(CanonicalizeRequestTest, ReturnsInvalidWireFormatVerbatim) {
  // Neither is valid wire format at all; such input is hashed as-is rather
  // than dropped.
  EXPECT_EQ(internal::CanonicalizeRequest("a"), "a");
  const std::string binary("\x00\x01\xff\n", 4);
  EXPECT_EQ(internal::CanonicalizeRequest(binary), binary);
}

TEST(CanonicalizeRequestTest, FallsBackToSortedWireFieldsWhenParseFails) {
  // json_payload (2) = "\xff" is invalid UTF-8, which proto3 rejects on parse,
  // followed by message_type (4) = "x"; and the same two fields in the other
  // order.  The canonical form must still not depend on that order.
  constexpr absl::string_view kUnparseableAscending =
      "\x12\x01\xff\x22\x01\x78";
  constexpr absl::string_view kUnparseableDescending =
      "\x22\x01\x78\x12\x01\xff";
  ASSERT_FALSE(::conformance::ConformanceRequest().ParseFromString(
      kUnparseableAscending));

  EXPECT_EQ(internal::CanonicalizeRequest(kUnparseableAscending),
            internal::CanonicalizeRequest(kUnparseableDescending));
  EXPECT_EQ(internal::CanonicalizeRequest(kUnparseableDescending),
            kUnparseableAscending);
}

TEST(CanonicalizeRequestTest, EmptyRequestIsEmpty) {
  EXPECT_EQ(internal::CanonicalizeRequest(""), "");
}

TEST(RecordingTestRunnerTest, HashIsIndependentOfFieldOrder) {
  FakeTestRunner fake;
  std::ostringstream ascending;
  std::ostringstream descending;
  RecordingTestRunner(&fake, &ascending).RunTest("Suite.Test", kAscending);
  RecordingTestRunner(&fake, &descending).RunTest("Suite.Test", kDescending);

  EXPECT_EQ(ascending.str(), descending.str());
  // The size is that of the wire bytes, the hash that of the canonical form.
  EXPECT_EQ(
      ascending.str(),
      absl::StrCat("Suite.Test 9 ",
                   absl::Hex(internal::Fnv1a64(
                                 internal::CanonicalizeRequest(kAscending)),
                             absl::kZeroPad16),
                   "\n"));
}

TEST(RecordingTestRunnerTest, DelegatesAndPassesThroughResponse) {
  FakeTestRunner fake;
  std::ostringstream out;
  RecordingTestRunner runner(&fake, &out);

  EXPECT_EQ(runner.RunTest("Test.One", "abc"), "response:Test.One:abc");
  EXPECT_EQ(runner.RunTest("Test.Two", ""), "response:Test.Two:");

  EXPECT_THAT(fake.calls(),
              ElementsAre(Pair("Test.One", "abc"), Pair("Test.Two", "")));
}

// "a" and "foobar" are not valid wire format, so they are hashed verbatim (see
// CanonicalizeRequestTest).
TEST(RecordingTestRunnerTest, WritesOneLinePerRequest) {
  FakeTestRunner fake;
  std::ostringstream out;
  {
    RecordingTestRunner runner(&fake, &out);
    runner.RunTest("Suite.First", "a");
    runner.RunTest("Suite.Second", "foobar");
  }

  EXPECT_EQ(out.str(),
            "Suite.First 1 af63dc4c8601ec8c\n"
            "Suite.Second 6 85944171f73967e8\n");
}

TEST(RecordingTestRunnerTest, EmptyInputYieldsSizeZero) {
  FakeTestRunner fake;
  std::ostringstream out;
  RecordingTestRunner runner(&fake, &out);

  runner.RunTest("Suite.Empty", "");

  EXPECT_EQ(out.str(), "Suite.Empty 0 cbf29ce484222325\n");
}

TEST(RecordingTestRunnerTest, RecordsBinaryInputByHashOnly) {
  FakeTestRunner fake;
  std::ostringstream out;
  RecordingTestRunner runner(&fake, &out);

  const std::string binary("\x00\x01\xff\n", 4);
  runner.RunTest("Suite.Binary", binary);

  // The raw bytes (including the newline) must not leak into the output; only
  // the size and hash are recorded.
  EXPECT_EQ(out.str(),
            absl::StrCat("Suite.Binary 4 ",
                         absl::Hex(internal::Fnv1a64(binary), absl::kZeroPad16),
                         "\n"));
}

// Delegate that snapshots the recording stream at the moment it is invoked,
// so the test can verify the line was written (and flushed) beforehand.
class SnapshottingTestRunner : public ConformanceTestRunner {
 public:
  explicit SnapshottingTestRunner(std::ostringstream* out) : out_(out) {}

  std::string RunTest(absl::string_view test_name,
                      absl::string_view input) override {
    snapshot_ = out_->str();
    return "";
  }

  const std::string& snapshot() const { return snapshot_; }

 private:
  std::ostringstream* out_;
  std::string snapshot_;
};

TEST(RecordingTestRunnerTest, RecordsBeforeDelegating) {
  std::ostringstream out;
  SnapshottingTestRunner fake(&out);
  RecordingTestRunner runner(&fake, &out);

  runner.RunTest("Suite.First", "a");

  // The line must already be on the stream when the delegate runs, so that a
  // crash inside the delegate cannot lose it.
  EXPECT_EQ(fake.snapshot(), "Suite.First 1 af63dc4c8601ec8c\n");
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
