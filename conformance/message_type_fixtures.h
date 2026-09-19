// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_MESSAGE_TYPE_FIXTURES_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_MESSAGE_TYPE_FIXTURES_H__

#include <string>

#include <gtest/gtest.h>
#include "conformance/binary_test_util.h"
#include "conformance/test_fixture.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "google/protobuf/descriptor.h"

// Fixtures for conformance tests that know which message type they exercise.
// They report the message type through ConformanceTest::MessageUnderTest().
// The base SetUp() then skips the test when --maximum_edition doesn't cover
// the message's edition, so test bodies never have to gate themselves.
//
// Suites parameterized over more than the message type choose every value,
// including the message type, with PB_TEST_CHOOSE_VALUE (see protobuf_test.h):
// they derive from ConformanceTest and override MessageUnderTest() themselves
// (see binary_premature_eof_test.cc).
// TODO: make MessageTypeConformanceTest choose the message type
// that way too, and migrate the remaining TEST_P suites to PB_TEST_F.

namespace google {
namespace protobuf {
namespace conformance {

// The fixture for conformance tests parameterized over the test message type
// only.  Give each group its own suite name with a type alias.  Instantiate it
// over AllTestMessageTypes() with MessageTypeParamName():
//
//   using FooTest = MessageTypeConformanceTest;
//
//   TEST_P(FooTest, Bar) {
//     EXPECT_THAT(Testee("Bar").ParseBinary(message(), ...).ParseOnly(),
//                 Yields(IsParseError()));
//   }
//
//   INSTANTIATE_TEST_SUITE_P(All, FooTest, ValuesIn(AllTestMessageTypes()),
//                            MessageTypeParamName);
class MessageTypeConformanceTest
    : public ConformanceTest,
      public testing::WithParamInterface<const Descriptor*> {
 protected:
  const Descriptor* message() const { return GetParam(); }

 private:
  const Descriptor* MessageUnderTest() const override { return GetParam(); }
};

// The fixture for conformance tests that only exist for
// TestAllTypesEdition2023, such as extensions and delimited fields.  Like
// MessageTypeConformanceTest, alias it per group:
//
//   using DelimitedExtensionTest = Edition2023ConformanceTest;
class Edition2023ConformanceTest : public ConformanceTest {
 protected:
  static const Descriptor* message() {
    return protobuf_test_messages::editions::TestAllTypesEdition2023::
        descriptor();
  }

 private:
  const Descriptor* MessageUnderTest() const override { return message(); }
};

// The INSTANTIATE_TEST_SUITE_P name generator for suites parameterized over
// the message type only: MessageTypeName() of the descriptor, e.g.
// "EditionsProto2".
std::string MessageTypeParamName(
    const testing::TestParamInfo<const Descriptor*>& info);

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_MESSAGE_TYPE_FIXTURES_H__
