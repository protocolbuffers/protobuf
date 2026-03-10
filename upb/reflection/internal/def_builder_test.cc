// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/internal/def_builder.h"

#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "upb/mem/arena.hpp"

// Must be last.
#include "upb/port/def.inc"

struct IdentTestData {
  absl::string_view text;
  bool ok;
};

class FullIdentTestBase : public testing::TestWithParam<IdentTestData> {};

TEST_P(FullIdentTestBase, CheckFullIdent) {
  upb_Status status;
  upb_DefBuilder ctx;
  upb::Arena arena;
  ctx.status = &status;
  ctx.arena = arena.ptr();
  upb_Status_Clear(&status);

  if (UPB_SETJMP(ctx.err)) {
    EXPECT_FALSE(GetParam().ok);
  } else {
    _upb_DefBuilder_CheckIdentFull(
        &ctx, upb_StringView_FromDataAndSize(GetParam().text.data(),
                                             GetParam().text.size()));
    EXPECT_TRUE(GetParam().ok);
  }
}

INSTANTIATE_TEST_SUITE_P(FullIdentTest, FullIdentTestBase,
                         testing::ValuesIn(std::vector<IdentTestData>{
                             {"foo.bar", true},
                             {"foo.", true},
                             {"foo", true},

                             {"foo.7bar", false},
                             {".foo", false},
                             {"#", false},
                             {".", false},
                             {"", false}}));

class PartIdentTestBase : public testing::TestWithParam<IdentTestData> {};

TEST_P(PartIdentTestBase, TestNotFullIdent) {
  upb_Status status;
  upb_DefBuilder ctx;
  upb::Arena arena;
  ctx.status = &status;
  ctx.arena = arena.ptr();
  upb_Status_Clear(&status);

  if (UPB_SETJMP(ctx.err)) {
    EXPECT_FALSE(GetParam().ok);
  } else {
    _upb_DefBuilder_MakeFullName(
        &ctx, "abc",
        upb_StringView_FromDataAndSize(GetParam().text.data(),
                                       GetParam().text.size()));
    EXPECT_TRUE(GetParam().ok);
  }
}

INSTANTIATE_TEST_SUITE_P(PartIdentTest, PartIdentTestBase,
                         testing::ValuesIn(std::vector<IdentTestData>{
                             {"foo", true},
                             {"foo1", true},

                             {"foo.bar", false},
                             {"1foo", false},
                             {"#", false},
                             {".", false},
                             {"", false}}));
