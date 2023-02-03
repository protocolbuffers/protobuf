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
#include "upb/reflection/def.hpp"
#include "upb/reflection/def_builder_internal.h"

// Must be last.
#include "upb/port/def.inc"

struct IdentTest {
  const char* text;
  bool ok;
};

static const std::vector<IdentTest> FullIdentTests = {
    {"foo.bar", true},   {"foo.", true},  {"foo", true},

    {"foo.7bar", false}, {".foo", false}, {"#", false},
    {".", false},        {"", false},
};

static const std::vector<IdentTest> NotFullIdentTests = {
    {"foo", true},      {"foo1", true},

    {"foo.bar", false}, {"1foo", false}, {"#", false},
    {".", false},       {"", false},
};

TEST(DefBuilder, TestIdents) {
  upb_StringView sv;
  upb_Status status;
  upb_DefBuilder ctx;
  upb::Arena arena;
  ctx.status = &status;
  ctx.arena = arena.ptr();
  upb_Status_Clear(&status);

  for (const auto& test : FullIdentTests) {
    sv.data = test.text;
    sv.size = strlen(test.text);

    if (UPB_SETJMP(ctx.err)) {
      EXPECT_FALSE(test.ok);
    } else {
      _upb_DefBuilder_CheckIdentFull(&ctx, sv);
      EXPECT_TRUE(test.ok);
    }
  }

  for (const auto& test : NotFullIdentTests) {
    sv.data = test.text;
    sv.size = strlen(test.text);

    if (UPB_SETJMP(ctx.err)) {
      EXPECT_FALSE(test.ok);
    } else {
      _upb_DefBuilder_MakeFullName(&ctx, "abc", sv);
      EXPECT_TRUE(test.ok);
    }
  }
}
