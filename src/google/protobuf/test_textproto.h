// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_TEST_TEXTPROTO_H__
#define GOOGLE_PROTOBUF_TEST_TEXTPROTO_H__

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/field_comparator.h"
#include "google/protobuf/util/message_differencer.h"

// This file contains private helpers for dealing with textprotos in our
// tests.  We make no guarantees about the behavior in real-world situations,
// and these are only meant for basic unit-tests of protobuf internals.
namespace google {
namespace protobuf {
namespace internal {

inline bool EqualsProtoImpl(const Message& actual, const Message& expected,
                            ::testing::MatchResultListener* result_listener) {
  util::MessageDifferencer differencer;
  util::DefaultFieldComparator field_comparator;
  field_comparator.set_treat_nan_as_equal(true);
  differencer.set_field_comparator(&field_comparator);
  std::string differences;
  differencer.ReportDifferencesToString(&differences);

  differencer.Compare(actual, expected);
  if (!differences.empty()) {
    *result_listener << "protos were not equivalent:\n" << differences;
    return false;
  }
  return true;
}

MATCHER_P(EqualsTextProto, textproto, "") {
  auto msg = absl::WrapUnique(arg.New());
  if (!TextFormat::ParseFromString(textproto, msg.get())) {
    *result_listener << "failed to parse textproto";
    return false;
  }
  return EqualsProtoImpl(arg, *msg, result_listener);
}

MATCHER_P(EqualsMessage, message, "") {
  return EqualsProtoImpl(arg, *message, result_listener);
}
}  // namespace internal

inline auto EqualsProto(absl::string_view textproto) {
  return internal::EqualsTextProto(textproto);
}
inline auto EqualsProto(const Message& message) {
  return internal::EqualsMessage(&message);
}

MATCHER_P3(EqualsProtoSerialized, pool, type, textproto, "") {
  const Descriptor* desc = pool->FindMessageTypeByName(type);
  DynamicMessageFactory factory(pool);
  auto msg = absl::WrapUnique(factory.GetPrototype(desc)->New());
  return TextFormat::ParseFromString(textproto, msg.get()) &&
         arg.SerializeAsString() == msg->SerializeAsString();
}

class ParseTextOrDie {
 public:
  explicit ParseTextOrDie(absl::string_view text) : text_(text) {}
  template <typename Proto>
  operator Proto() {  // NOLINT(google-explicit-constructor)
    Proto ret;
    ABSL_CHECK(TextFormat::ParseFromString(text_, &ret));
    return ret;
  }

 private:
  absl::string_view text_;
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_TEST_TEXTPROTO_H__
