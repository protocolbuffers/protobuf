// -*- c++ -*-
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
#include "google/protobuf/reflection_ops.h"
#include "google/protobuf/test_util2.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

TEST(MESSAGE_TEST_NAME, LegacySetMethodsOnStringField) {
  UNITTEST::TestAllTypes msg;

  msg.set_optional_string("std::string value 4", 5);
  EXPECT_EQ(msg.optional_string(), "std::");
}

TEST(MESSAGE_TEST_NAME, LegacySetMethodsOnRepeatedStringField) {
  UNITTEST::TestAllTypes msg;

  msg.add_repeated_string("");
  msg.set_repeated_string(0, "std::string value 4", 5);
  EXPECT_EQ(msg.repeated_string(0), "std::");
}

TEST(MESSAGE_TEST_NAME, LegacyAddMethodsOnRepeatedStringField) {
  UNITTEST::TestAllTypes msg;

  msg.add_repeated_string("std::string value 4", 5);
  EXPECT_EQ(msg.repeated_string(0), "std::");
  msg.clear_repeated_string();
}


namespace {
template <typename T>
static const internal::TcParseTableBase* GetTableIfAvailable(...) {
  return nullptr;
}

template <typename T>
static const internal::TcParseTableBase* GetTableIfAvailable(
    decltype(internal::TcParser::GetTable<T>())) {
  return internal::TcParser::GetTable<T>();
}
}  // namespace

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
