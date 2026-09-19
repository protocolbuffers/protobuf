// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_MESSAGE_TYPE_FIXTURES_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_MESSAGE_TYPE_FIXTURES_H__

#include <string>
#include <tuple>

#include <gtest/gtest.h>
#include "absl/strings/str_join.h"
#include "binary_test_util.h"
#include "test_environment.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "google/protobuf/descriptor.h"

// Fixtures for conformance tests that know which message type they exercise,
// and the INSTANTIATE_TEST_SUITE_P name generators that go with them.  The
// fixtures report the message type through ConformanceTest::MessageUnderTest(),
// so the base SetUp() skips the test when --maximum_edition doesn't cover the
// message's edition and test bodies never have to gate themselves.
//
// Suites parameterized over more than the message type (a std::tuple whose
// first element is the `const Descriptor*`) derive from ConformanceTest and
// override MessageUnderTest() themselves; see binary_premature_eof_test.cc.

namespace google {
namespace protobuf {
namespace conformance {

// The fixture for conformance tests parameterized over the test message type
// only.  Give each group its own suite name with a type alias and instantiate
// it over AllTestMessageTypes() (or Proto3TestMessageTypes()) with
// MessageTypeParamName():
//
//   using FooTest = MessageTypeConformanceTest;
//
//   TEST_P(FooTest, Bar) {
//     EXPECT_THAT(RequiredTest("Bar").ParseBinary(message(), ...).ParseOnly(),
//                 Yields(IsParseError()));
//   }
//
//   INSTANTIATE_TEST_SUITE_P(All, FooTest, ValuesIn(AllTestMessageTypes()),
//                            MessageTypeParamName);
class MessageTypeConformanceTest
    : public ConformanceTest,
      public testing::WithParamInterface<const Descriptor*> {
 protected:
  const Descriptor* MessageUnderTest() const override { return GetParam(); }

  const Descriptor* message() const { return GetParam(); }
};

// The fixture for conformance tests that only exist for
// TestAllTypesEdition2023 (extensions, delimited fields, ...).  Like
// MessageTypeConformanceTest, alias it per group:
//
//   using DelimitedExtensionTest = Edition2023ConformanceTest;
class Edition2023ConformanceTest : public ConformanceTest {
 protected:
  const Descriptor* MessageUnderTest() const override { return message(); }

  static const Descriptor* message() {
    return protobuf_test_messages::editions::TestAllTypesEdition2023::
        descriptor();
  }
};

// The INSTANTIATE_TEST_SUITE_P name generator for suites parameterized over
// the message type only: ParamName() of the descriptor, e.g. "EditionsProto2".
std::string MessageTypeParamName(
    const testing::TestParamInfo<const Descriptor*>& info);

// The INSTANTIATE_TEST_SUITE_P name generator for suites parameterized over a
// std::tuple: ParamName() of every element, joined with "_", e.g.
// "Proto3_INT32" for a (const Descriptor*, FieldDescriptor::Type) tuple.  Every
// element type needs a ParamName() overload (see binary_test_util.h).
//
//   INSTANTIATE_TEST_SUITE_P(All, FooTest,
//                            Combine(ValuesIn(AllTestMessageTypes()),
//                                    ValuesIn(kTypes)),
//                            TupleParamName<FooTest::ParamType>);
template <typename Tuple>
std::string TupleParamName(const testing::TestParamInfo<Tuple>& info) {
  return std::apply(
      [](const auto&... params) {
        return absl::StrJoin({ParamName(params)...}, "_");
      },
      info.param);
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_MESSAGE_TYPE_FIXTURES_H__
