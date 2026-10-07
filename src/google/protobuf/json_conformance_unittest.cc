// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_json_conformance_suite.h"
#include "conformance/conformance.pb.h"
#include "conformance/conformance_test.h"
#include "conformance/test_runner.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/json_parser_internal.h"
#include "google/protobuf/message.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"

namespace {

class ConformanceErrorCollector : public google::protobuf::io::ErrorCollector {
 public:
  void RecordError(int line, google::protobuf::io::ColumnNumber column,
                   absl::string_view message) override {
    ABSL_CHECK(!message.empty());
    error_ = absl::StrCat(line, ":", column, " ", message);
  }

  const std::string& error() const { return error_; }

 private:
  std::string error_;
};

class SeamInputStream : public google::protobuf::io::ZeroCopyInputStream {
 public:
  SeamInputStream(absl::string_view data, int block_size)
      : data_(data), block_size_(block_size == -1 ? data.size() : block_size) {
    buf_.resize(block_size_);
  }

  ~SeamInputStream() override = default;

  // implements ZeroCopyInputStream ----------------------------------
  bool Next(const void** data, int* size) override {
    *data = &buf_[0];
    *size = std::min<size_t>(data_.size(), block_size_);
    memcpy(&buf_[0], data_.data(), *size);
    data_.remove_prefix(*size);
    last_size_ = *size;
    return true;
  }

  void BackUp(int count) override {
    ABSL_CHECK(count <= last_size_);
    data_ = absl::string_view(data_.data() - count, data_.size() + count);
  }

  bool Skip(int count) override {
    data_.remove_prefix(count);
    return true;
  }

  int64_t ByteCount() const override { return 0; }

 private:
  absl::string_view data_;
  std::vector<char> buf_;
  const int block_size_;  // How many bytes to return at a time.
  int last_size_;
};

class ConformanceRunner : public google::protobuf::conformance::ConformanceTestRunner {
  std::string RunTest(absl::string_view test_name,
                      absl::string_view input) override;
  void RunTestWithSeam(const google::protobuf::Descriptor* descriptor,
                       const google::protobuf::internal::JsonParseOptions& json_options,
                       absl::string_view input, int seam,
                       conformance::ConformanceResponse* response) {
    SeamInputStream in_stream(input, seam);
    google::protobuf::io::StringOutputStream out_stream(
        response->mutable_protobuf_payload());
    ConformanceErrorCollector collector;
    bool success = google::protobuf::internal::JsonToBinaryStream(
        descriptor, &in_stream, &out_stream, &collector, json_options,
        google::protobuf::DescriptorPool::generated_pool());
    ASSERT_EQ(success, collector.error().empty());
    if (!success) {
      response->clear_protobuf_payload();
      response->set_parse_error(collector.error());
    }
  }
};

std::string ConformanceRunner::RunTest(absl::string_view test_name,
                                       absl::string_view input) {
  google::protobuf::LinkMessageReflection<
      protobuf_test_messages::proto2::TestAllTypesProto2>();
  google::protobuf::LinkMessageReflection<
      protobuf_test_messages::proto3::TestAllTypesProto3>();
  conformance::ConformanceRequest request;
  conformance::ConformanceResponse response;
  google::protobuf::internal::JsonParseOptions json_options;

  ABSL_CHECK(request.ParseFromString(input));
  const google::protobuf::Descriptor* d =
      google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(
          request.message_type());

  if (request.test_category() ==
      conformance::JSON_IGNORE_UNKNOWN_PARSING_TEST) {
    json_options.ignore_unknown_fields = true;
  }

  if (d &&
      request.payload_case() == conformance::ConformanceRequest::kJsonPayload &&
      request.requested_output_format() == conformance::PROTOBUF) {
    RunTestWithSeam(d, json_options, request.json_payload(), -1, &response);

    // Verify that we get the same output no matter where we put a seam.
    for (int i = 1; i < request.json_payload().size(); i++) {
      conformance::ConformanceResponse compare;
      RunTestWithSeam(d, json_options, request.json_payload(), i, &compare);
      EXPECT_EQ(compare.parse_error(), response.parse_error())
          << google::protobuf::ShortFormat(request);
      EXPECT_EQ(compare.protobuf_payload(), response.protobuf_payload())
          << google::protobuf::ShortFormat(request);
    }
  } else {
    response.set_skipped("Skipped");
  }
  return response.SerializeAsString();
}

TEST(JsonConformance, Conformance) {
  google::protobuf::conformance::BinaryAndJsonConformanceSuite suite;
  suite.SetEnforceRecommended(true);
  ConformanceRunner runner;
  conformance::FailureSet failures;

  // These are failures until we can get Descriptor to respect json_name.
  absl::flat_hash_map<std::string, std::string> failure_map = {
      {"Required.Proto3.JsonInput.FieldNameInSnakeCase.ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Required.Proto3.JsonInput.FieldNameWithMixedCases.ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Recommended.Proto3.JsonInput.FieldNameWithDoubleUnderscores."
       "ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Recommended.Proto3.JsonInput.IgnoreUnknownEnumStringValueInMapValue."
       "ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Recommended.Proto3.JsonInput."
       "IgnoreUnknownEnumStringValueInOptionalField.ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Recommended.Proto3.JsonInput."
       "IgnoreUnknownEnumStringValueInRepeatedField.ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Required.Proto2.JsonInput.FieldNameInSnakeCase.ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Required.Proto2.JsonInput.FieldNameWithMixedCases.ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Recommended.Proto2.JsonInput.FieldNameWithDoubleUnderscores."
       "ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Recommended.Proto2.JsonInput.IgnoreUnknownEnumStringValueInMapValue."
       "ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Recommended.Proto2.JsonInput."
       "IgnoreUnknownEnumStringValueInOptionalField.ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Recommended.Proto2.JsonInput."
       "IgnoreUnknownEnumStringValueInRepeatedField.ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Recommended.Proto2.JsonInput.IgnoreUnknownEnumStringValueInMapPart."
       "ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Recommended.Proto2.JsonInput."
       "IgnoreUnknownEnumStringValueInRepeatedPart.ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Recommended.Proto3.JsonInput.IgnoreUnknownEnumStringValueInMapPart."
       "ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Recommended.Proto3.JsonInput."
       "IgnoreUnknownEnumStringValueInRepeatedPart.ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Required.*.JsonInput.Int32FieldQuotedExponentialValue.ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Required.*.JsonInput."
       "DoubleFieldQuotedExponentialValue.ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Required.Proto3.JsonInput.TimestampWithComplexOffset.ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Required.Proto3.JsonInput.TimestampWithOffsetBoundaryInBoundsMax."
       "ProtobufOutput",
       "Failed to parse input or produce output."},
      {"Required.Proto3.JsonInput.TimestampWithOffsetBoundaryInBoundsMin."
       "ProtobufOutput",
       "Failed to parse input or produce output."},
      // Duplicate field name tests merge values in C++ instead of doing
      // last-wins.
      {"Recommended.Proto2.JsonInput.FieldNameDuplicate",
       "Should have failed to parse or matched expected output but did not."},
      {"Recommended.Proto2.JsonInput.FieldNameDuplicateDifferentCasing1",
       "Should have failed to parse or matched expected output but did not."},
      {"Recommended.Proto2.JsonInput.FieldNameDuplicateDifferentCasing2",
       "Should have failed to parse or matched expected output but did not."},
      {"Recommended.Proto3.JsonInput.FieldNameDuplicate",
       "Should have failed to parse or matched expected output but did not."},
      {"Recommended.Proto3.JsonInput.FieldNameDuplicateDifferentCasing1",
       "Should have failed to parse or matched expected output but did not."},
      {"Recommended.Proto3.JsonInput.FieldNameDuplicateDifferentCasing2",
       "Should have failed to parse or matched expected output but did not."},
  };

  for (const auto& [test_name, message] : failure_map) {
    conformance::TestStatus* test_failure = failures.add_test();
    test_failure->set_name(test_name);
    test_failure->set_failure_message(message);
  }

  std::string output;
  ASSERT_TRUE(suite.RunSuite(&runner, &output, "json_conformance_unittest.cc",
                             &failures))
      << output;
  ABSL_LOG(INFO) << output;
}

}  // namespace
