// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/internal/def_builder.h"

#include <cstddef>
#include <vector>

#include <gtest/gtest.h>
#include "absl/cleanup/cleanup.h"
#include "absl/strings/string_view.h"
#include "upb/base/status.h"
#include "upb/base/string_view.h"
#include "upb/mem/alloc.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.h"
#include "upb/reflection/def_pool.h"
#include "upb/reflection/def_type.h"

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
                             {"foo", true},
                             {"foo.7bar", true},

                             {"foo.", false},
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
                             {"1foo", true},

                             {"foo.bar", false},
                             {"#", false},
                             {".", false},
                             {"", false}}));

TEST(DefBuilderTest, AllocationFailure) {
  if (!upb_AllocationCount_IsAvailable()) return;

  auto RunScenario = [&]() -> bool {
    upb_Arena* arena = upb_Arena_New();
    if (!arena) return false;
    auto cleanup = absl::MakeCleanup([arena] { upb_Arena_Free(arena); });

    upb_Status status;
    upb_DefBuilder ctx;
    ctx.status = &status;
    ctx.arena = arena;
    upb_Status_Clear(&status);

    if (UPB_SETJMP(ctx.err)) {
      return false;
    }

    _upb_DefBuilder_MakeFullName(&ctx, "abc",
                                 upb_StringView_FromDataAndSize("foo", 3));
    return true;
  };

  upb_AllocationCount_Reset();
  if (RunScenario()) {
    size_t total_allocations = upb_AllocationCount_Get();
    for (size_t i = 0; i < total_allocations; ++i) {
      upb_AllocationCount_Reset();
      upb_AllocationCount_FailOn(i);
      bool success_with_fail = RunScenario();
      EXPECT_FALSE(success_with_fail)
          << "DefBuilder unexpectedly succeeded when allocation "
          << "number " << i << " was failed.";
    }
  }
  upb_AllocationCount_Reset();
}

TEST(DefBuilderTest, ResolveAllocationFailure) {
  if (!upb_AllocationCount_IsAvailable()) return;

  auto RunScenario = [&]() -> bool {
    upb_Arena* arena = upb_Arena_New();
    if (!arena) return false;
    auto cleanup = absl::MakeCleanup([arena] { upb_Arena_Free(arena); });

    upb_DefPool* symtab = upb_DefPool_New();
    if (!symtab) return false;
    auto cleanup_symtab =
        absl::MakeCleanup([symtab] { upb_DefPool_Free(symtab); });

    upb_Status status;
    upb_DefBuilder ctx;
    ctx.status = &status;
    ctx.arena = arena;
    ctx.symtab = symtab;
    upb_Status_Clear(&status);

    if (UPB_SETJMP(ctx.err)) {
      return false;
    }

    upb_deftype_t found_type;
    _upb_DefBuilder_ResolveAny(&ctx, "dbg", "base.package",
                               upb_StringView_FromDataAndSize("foo", 3),
                               &found_type);
    return true;
  };

  upb_AllocationCount_Reset();
  if (RunScenario()) {
    size_t total_allocations = upb_AllocationCount_Get();
    for (size_t i = 0; i < total_allocations; ++i) {
      upb_AllocationCount_Reset();
      upb_AllocationCount_FailOn(i);
      bool success_with_fail = RunScenario();
      EXPECT_FALSE(success_with_fail)
          << "DefBuilder Resolve unexpectedly succeeded when allocation "
          << "number " << i << " was failed.";
    }
  }
  upb_AllocationCount_Reset();
}
