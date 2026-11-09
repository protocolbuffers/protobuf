// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/recursion_limit_payloads.h"

#include <gtest/gtest.h>
#include "conformance/binary_wireformat.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/testing/protobuf_test.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::protobuf_test_messages::editions::TestAllTypesEdition2023;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;

// The same payloads built from message objects the way the legacy suite built
// them, at depths the stack can take: the shallowest, and one deep enough for
// the length prefixes to need two bytes.
constexpr int kDepths[] = {1, 3, 50};

PB_TEST(RecursionLimitPayloadsTest, DeepMapMatchesTheMessageObject) {
  const int depth = PB_TEST_CHOOSE_VALUE(kDepths, "Depth");
  TestAllTypesEdition2023 message;
  TestAllTypesEdition2023* sub = &message;
  for (int i = 0; i < depth; i++) {
    sub = &(*sub->mutable_map_recursive())[0];
    sub->set_optional_int32(123);
  }
  EXPECT_EQ(DeepMapPayload(depth), Wire(message.SerializeAsString()));
}

PB_TEST(RecursionLimitPayloadsTest, DeepMapStringKeyMatchesTheMessageObject) {
  const int depth = PB_TEST_CHOOSE_VALUE(kDepths, "Depth");
  TestAllTypesEdition2023 message;
  TestAllTypesEdition2023* sub = &message;
  for (int i = 0; i < depth; i++) {
    sub = (*sub->mutable_map_string_nested_message())[""].mutable_corecursive();
    sub->set_optional_int32(123);
  }
  EXPECT_EQ(DeepMapStringKeyPayload(depth), Wire(message.SerializeAsString()));
}

PB_TEST(RecursionLimitPayloadsTest, DeepMessageSetMatchesTheMessageObject) {
  const int depth = PB_TEST_CHOOSE_VALUE(kDepths, "Depth");
  TestAllTypesProto2 message;
  TestAllTypesProto2::MessageSetCorrect* sub =
      message.mutable_message_set_correct();
  for (int i = 0; i < depth; i++) {
    sub =
        sub->MutableExtension(TestAllTypesProto2::MessageSetCorrectExtension2::
                                  message_set_extension)
            ->mutable_sub_msg();
  }
  sub->MutableExtension(
         TestAllTypesProto2::MessageSetCorrectExtension2::message_set_extension)
      ->set_i(123);
  EXPECT_EQ(DeepMessageSetPayload(depth), Wire(message.SerializeAsString()));
}

// A message set of depth 0 is a single extension holding i.
PB_TEST(RecursionLimitPayloadsTest, DepthZeroMessageSet) {
  TestAllTypesProto2 message;
  message.mutable_message_set_correct()
      ->MutableExtension(TestAllTypesProto2::MessageSetCorrectExtension2::
                             message_set_extension)
      ->set_i(123);
  EXPECT_EQ(DeepMessageSetPayload(0), Wire(message.SerializeAsString()));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
