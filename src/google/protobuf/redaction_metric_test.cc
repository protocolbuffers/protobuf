// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Test for the redaction metric.
// Because the metric is a global variable, we have to put the tests in a
// separate file to more accurately test their values.

#include <cstdint>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unittest.pb.h"

namespace google {
namespace protobuf {

namespace {

using ::testing::HasSubstr;

TEST(TextFormatParsingMetricsTest, MetricsTest) {
  std::string value_replacement = "[REDACTED]";
  protobuf_unittest::RedactedFields proto;
  proto.set_optional_redacted_string("foo");
  int64_t before = internal::GetRedactedFieldCount();
  EXPECT_THAT(absl::StrCat(proto), HasSubstr(value_replacement));
  int64_t after = internal::GetRedactedFieldCount();
  EXPECT_EQ(after, before + 1);
}

}  // namespace
}  // namespace protobuf
}  // namespace google
