// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/descriptor_test_utils.h"

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/log/die_if_null.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/parser.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/text_format.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

using ::testing::AtLeast;
using ::testing::HasSubstr;
using ::testing::Return;

namespace google {
namespace protobuf {

// Can't use an anonymous namespace here due to brokenness of Tru64 compiler.
namespace descriptor_unittest {

// implements ErrorCollector ---------------------------------------
void MockErrorCollector::RecordError(absl::string_view filename,
                                     absl::string_view element_name,
                                     const Message* descriptor,
                                     ErrorLocation location,
                                     absl::string_view message) {
  absl::SubstituteAndAppend(&text_, "$0: $1: $2: $3\n", filename, element_name,
                            ErrorLocationName(location), message);
}

// implements ErrorCollector ---------------------------------------
void MockErrorCollector::RecordWarning(absl::string_view filename,
                                       absl::string_view element_name,
                                       const Message* descriptor,
                                       ErrorLocation location,
                                       absl::string_view message) {
  absl::SubstituteAndAppend(&warning_text_, "$0: $1: $2: $3\n", filename,
                            element_name, ErrorLocationName(location), message);
}

// ===================================================================

void ValidationErrorTest::SetUp() {
  // Enable extension declaration enforcement since most test cases want to
  // exercise the full validation.
  pool_.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kAllExtensions);
}

const FileDescriptor* ValidationErrorTest::BuildFile(
    absl::string_view file_text) {
  FileDescriptorProto file_proto;
  EXPECT_TRUE(TextFormat::ParseFromString(file_text, &file_proto));
  return ABSL_DIE_IF_NULL(pool_.BuildFile(file_proto));
}

FileDescriptorProto ValidationErrorTest::ParseFile(
    absl::string_view file_name, absl::string_view file_text) {
  io::ArrayInputStream input_stream(file_text.data(), file_text.size());
  SimpleErrorCollector error_collector;
  io::Tokenizer tokenizer(&input_stream, &error_collector);
  compiler::Parser parser;
  parser.RecordErrorsTo(&error_collector);
  FileDescriptorProto proto;
  ABSL_CHECK(parser.Parse(&tokenizer, &proto))
      << error_collector.last_error() << "\n"
      << file_text;
  ABSL_CHECK_EQ("", error_collector.last_error());
  proto.set_name(file_name);
  return proto;
}

const FileDescriptor* ValidationErrorTest::ParseAndBuildFile(
    absl::string_view file_name, absl::string_view file_text) {
  return pool_.BuildFile(ParseFile(file_name, file_text));
}


// Add file_proto to the DescriptorPool. Expect errors to be produced which
// match the given error text.
void ValidationErrorTest::BuildFileWithErrors(
    const FileDescriptorProto& file_proto,
    testing::Matcher<std::string> expected_errors) {
  MockErrorCollector error_collector;
  EXPECT_TRUE(pool_.BuildFileCollectingErrors(file_proto, &error_collector) ==
              nullptr);
  EXPECT_THAT(error_collector.text_, expected_errors);
}

// Parse file_text as a FileDescriptorProto in text format and add it
// to the DescriptorPool.  Expect errors to be produced which match the
// given error text.
void ValidationErrorTest::BuildFileWithErrors(
    const std::string& file_text,
    testing::Matcher<std::string> expected_errors) {
  FileDescriptorProto file_proto;
  ASSERT_TRUE(TextFormat::ParseFromString(file_text, &file_proto));
  BuildFileWithErrors(file_proto, expected_errors);
}

// Parse a proto file and build it.  Expect errors to be produced which match
// the given error text.
void ValidationErrorTest::ParseAndBuildFileWithErrors(
    absl::string_view file_name, absl::string_view file_text,
    absl::string_view expected_errors) {
  MockErrorCollector error_collector;
  EXPECT_TRUE(pool_.BuildFileCollectingErrors(ParseFile(file_name, file_text),
                                              &error_collector) == nullptr);
  EXPECT_EQ(expected_errors, error_collector.text_);
}

void ValidationErrorTest::ParseAndBuildFileWithErrorSubstr(
    absl::string_view file_name, absl::string_view file_text,
    absl::string_view expected_errors) {
  MockErrorCollector error_collector;
  EXPECT_TRUE(pool_.BuildFileCollectingErrors(ParseFile(file_name, file_text),
                                              &error_collector) == nullptr);
  EXPECT_THAT(error_collector.text_, HasSubstr(expected_errors));
}

void ValidationErrorTest::ParseAndBuildFileWithWarningSubstr(
    absl::string_view file_name, absl::string_view file_text,
    absl::string_view expected_warning) {
  MockErrorCollector error_collector;
  EXPECT_TRUE(pool_.BuildFileCollectingErrors(ParseFile(file_name, file_text),
                                              &error_collector));
  EXPECT_THAT(error_collector.warning_text_, HasSubstr(expected_warning));
}

// Parse file_text as a FileDescriptorProto in text format and add it
// to the DescriptorPool.  Expect errors to be produced which match the
// given warning text.
void ValidationErrorTest::BuildFileWithWarnings(
    const std::string& file_text, const std::string& expected_warnings) {
  FileDescriptorProto file_proto;
  ASSERT_TRUE(TextFormat::ParseFromString(file_text, &file_proto));

  MockErrorCollector error_collector;
  EXPECT_TRUE(pool_.BuildFileCollectingErrors(file_proto, &error_collector));
  EXPECT_EQ(expected_warnings, error_collector.warning_text_);
}

// Builds some already-parsed file in our test pool.
void ValidationErrorTest::BuildFileInTestPool(const FileDescriptor* file) {
  FileDescriptorProto file_proto;
  file->CopyTo(&file_proto);
  ASSERT_TRUE(pool_.BuildFile(file_proto) != nullptr);
}

// Build descriptor.proto in our test pool. This allows us to extend it in
// the test pool, so we can test custom options.
void ValidationErrorTest::BuildDescriptorMessagesInTestPool() {
  BuildFileInTestPool(DescriptorProto::descriptor()->file());
}

void ValidationErrorTest::BuildDescriptorMessagesInTestPoolWithErrors(
    absl::string_view expected_errors) {
  FileDescriptorProto file_proto;
  DescriptorProto::descriptor()->file()->CopyTo(&file_proto);
  MockErrorCollector error_collector;
  EXPECT_TRUE(pool_.BuildFileCollectingErrors(file_proto, &error_collector) ==
              nullptr);
  EXPECT_EQ(error_collector.text_, expected_errors);
}

}  // namespace descriptor_unittest
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
