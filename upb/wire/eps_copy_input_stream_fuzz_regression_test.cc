// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
#include "upb/wire/eps_copy_input_stream_fuzz_impl.h"

namespace {

TEST(EpsCopyFuzzTest, TestAgainstFakeStreamRegression) {
  TestAgainstFakeStream({299,
                         {
                             PushLimitOp{2},
                             ReadOp{14},
                         }});
}

TEST(EpsCopyFuzzTest, AliasingEnabledZeroSizeReadString) {
  TestAgainstFakeStream({510, {ReadStringAliasOp{0}}});
}

TEST(EpsCopyFuzzTest, AliasingDisabledZeroSizeReadString) {
  TestAgainstFakeStream({510, {ReadStringOp{0}}});
}

TEST(EpsCopyFuzzTest, ReadStringZero) {
  TestAgainstFakeStream({0, {ReadStringAliasOp{0}}});
}

TEST(EpsCopyFuzzTest, ReadZero) {
  TestAgainstFakeStream({0, {ReadOp{0}}});
}

TEST(EpsCopyFuzzTest, ReadZeroTwice) {
  TestAgainstFakeStream({0, {ReadOp{0}, ReadOp{0}}});
}

TEST(EpsCopyFuzzTest, ReadStringZeroThenRead) {
  TestAgainstFakeStream({0, {ReadStringAliasOp{0}, ReadOp{0}}});
}

TEST(EpsCopyFuzzTest, ReadStringOverflowsBufferButNotLimit) {
  TestAgainstFakeStream({351,
                         {
                             ReadOp{7},
                             PushLimitOp{2147483647},
                             ReadStringOp{344},
                         }});
}

TEST(EpsCopyFuzzTest, LastBufferAliasing) {
  TestAgainstFakeStream({27, {ReadOp{12}, ReadStringAliasOp{3}}});
}

TEST(EpsCopyFuzzTest, FirstBufferAliasing) {
  TestAgainstFakeStream({7, {ReadStringAliasOp{3}}});
}

TEST(EpsCopyFuzzTest, LimitPastTheEnd) {
  TestAgainstFakeStream({17, {PushLimitOp{18}, ReadStringOp{17}}});
}

}  // namespace
