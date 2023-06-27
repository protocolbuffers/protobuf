/*
 * Copyright (c) 2009-2022, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "upb/reflection/def_builder_internal.h"
#include "upb/upb.hpp"

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
