// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
#ifndef GOOGLE_PROTOBUF_DESCRIPTOR_TEST_UTILS_H__
#define GOOGLE_PROTOBUF_DESCRIPTOR_TEST_UTILS_H__

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/tokenizer.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

// Can't use an anonymous namespace here due to brokenness of Tru64 compiler.
namespace descriptor_unittest {

class MockErrorCollector : public DescriptorPool::ErrorCollector {
 public:
  MockErrorCollector() = default;
  ~MockErrorCollector() override = default;

  std::string text_;
  std::string warning_text_;

  // implements ErrorCollector ---------------------------------------
  void RecordError(absl::string_view filename, absl::string_view element_name,
                   const Message* descriptor, ErrorLocation location,
                   absl::string_view message) override;

  // implements ErrorCollector ---------------------------------------
  void RecordWarning(absl::string_view filename, absl::string_view element_name,
                     const Message* descriptor, ErrorLocation location,
                     absl::string_view message) override;
};

class SimpleErrorCollector : public io::ErrorCollector {
 public:
  // implements ErrorCollector ---------------------------------------
  void RecordError(int line, int column, absl::string_view message) override {
    last_error_ = absl::StrFormat("%d:%d:%s", line, column, message);
  }

  const std::string& last_error() { return last_error_; }

 private:
  std::string last_error_;
};


class ValidationErrorTest : public testing::Test {
 protected:
  void SetUp() override;

  // Parse file_text as a FileDescriptorProto in text format and add it
  // to the DescriptorPool.  Expect no errors.
  const FileDescriptor* BuildFile(absl::string_view file_text);

  FileDescriptorProto ParseFile(absl::string_view file_name,
                                absl::string_view file_text);

  const FileDescriptor* ParseAndBuildFile(absl::string_view file_name,
                                          absl::string_view file_text);


  // Add file_proto to the DescriptorPool. Expect errors to be produced which
  // match the given error text.
  void BuildFileWithErrors(const FileDescriptorProto& file_proto,
                           testing::Matcher<std::string> expected_errors);

  // Parse file_text as a FileDescriptorProto in text format and add it
  // to the DescriptorPool.  Expect errors to be produced which match the
  // given error text.
  void BuildFileWithErrors(const std::string& file_text,
                           testing::Matcher<std::string> expected_errors);

  // Parse a proto file and build it.  Expect errors to be produced which match
  // the given error text.
  void ParseAndBuildFileWithErrors(absl::string_view file_name,
                                   absl::string_view file_text,
                                   absl::string_view expected_errors);

  void ParseAndBuildFileWithErrorSubstr(absl::string_view file_name,
                                        absl::string_view file_text,
                                        absl::string_view expected_errors);

  void ParseAndBuildFileWithWarningSubstr(absl::string_view file_name,
                                          absl::string_view file_text,
                                          absl::string_view expected_warning);

  // Parse file_text as a FileDescriptorProto in text format and add it
  // to the DescriptorPool.  Expect errors to be produced which match the
  // given warning text.
  void BuildFileWithWarnings(const std::string& file_text,
                             const std::string& expected_warnings);

  // Builds some already-parsed file in our test pool.
  void BuildFileInTestPool(const FileDescriptor* file);

  // Build descriptor.proto in our test pool. This allows us to extend it in
  // the test pool, so we can test custom options.
  void BuildDescriptorMessagesInTestPool();

  void BuildDescriptorMessagesInTestPoolWithErrors(
      absl::string_view expected_errors);

  DescriptorPool pool_;
};

}  // namespace descriptor_unittest
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_DESCRIPTOR_TEST_UTILS_H__
