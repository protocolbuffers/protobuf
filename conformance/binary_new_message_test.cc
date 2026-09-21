// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests of a message the testee creates empty (Test::New()
// on the testee API, a NewAction on the wire) rather than parses: the default
// instance, serialized as is.  No round trip can express this: every protocol
// version 1 request starts from a payload, so the shortest one, an empty
// payload, already goes through the parser.  These tests check the other end,
// that a message nothing was ever written to serializes to nothing, in every
// format and all of them at once.  They need protocol version 2; a testee that
// speaks version 1 never sees them and the runner reports them as unsupported
// rather than failed.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::_;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::IsEmpty;
using ::testing::Property;
using ::testing::ValuesIn;

using NewMessageTest = MessageTypeConformanceTest;

// An empty message serializes to no bytes at all.
TEST_P(NewMessageTest, SerializesToNothing) {
  EXPECT_THAT(Testee().New(message()).SerializeBinary(),
              Yields(Payload(Wire())));
}

// The same message asked for in every format at once: nothing in binary,
// nothing in text format and an empty object in JSON.  The extra outputs
// follow the terminal SerializeBinary()'s in TestResult::outputs(), in the
// order they were queued (see InMemoryMessage::AlsoText()), which is what
// makes them addressable here; the matchers of matchers.h only look at the
// first output, so the others are matched on outputs() directly.
TEST_P(NewMessageTest, SerializesToNothingInEveryFormat) {
  using Output = internal::TestResult::Output;
  EXPECT_THAT(
      Testee().New(message()).AlsoText().AlsoJson().SerializeBinary(),
      Yields(AllOf(
          Payload(Wire()),
          Property(&internal::TestResult::outputs,
                   ElementsAre(
                       _,
                       AllOf(Field(&Output::format, ::conformance::TEXT_FORMAT),
                             Field(&Output::payload, IsEmpty())),
                       AllOf(Field(&Output::format, ::conformance::JSON),
                             Field(&Output::payload, "{}")))))));
}

INSTANTIATE_TEST_SUITE_P(All, NewMessageTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
