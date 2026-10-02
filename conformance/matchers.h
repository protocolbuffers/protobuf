// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// gMock matchers for conformance tests.
//
// The matchers are pure, so they compose with gMock like any other matcher.
// The failure messages they write end up in failure lists, so each matcher
// documents its message and keeps it stable.

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_H__

#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/testee.h"
#include "google/protobuf/message.h"

namespace google {
namespace protobuf {
namespace conformance {

// Matches a message equivalent to `text`, parsed as the actual message's type.
// Messages are compared with MessageDifferencer, with NaN equal to NaN.
//
//   EXPECT_THAT(message, EqualsTextProto("optional_int32: 1"));
testing::Matcher<const Message&> EqualsTextProto(absl::string_view text);

// Like EqualsTextProto(), but the expected message is the binary serialization
// `bytes`.  It takes a Wire rather than a string so that NUL bytes survive.
// Messages are compared with MessageDifferencer, with NaN equal to NaN.
//
//   EXPECT_THAT(message, EqualsBinaryProto(VarintField(1, 1)));
testing::Matcher<const Message&> EqualsBinaryProto(Wire bytes);

// TODO: b/410122158 - Add JSON matchers once JSON support is migrated.

// Matches a response that reports a parse error.
//
//   EXPECT_THAT(result, IsParseError());
//
// Any other response fails with "Should have failed to parse, but didn't."  A
// runtime error fails with "Should have failed to parse, but raised an error
// instead."
testing::Matcher<const internal::TestResult&> IsParseError();

// Matches a response that reports a serialize error.
//
//   EXPECT_THAT(result, IsSerializeError());
//
// Any other response fails like it does for IsParseError(), with the
// corresponding "Should have failed to serialize, ..." messages.
testing::Matcher<const internal::TestResult&> IsSerializeError();

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_H__
