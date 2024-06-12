// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mem/debug/ref.h"

#include <gtest/gtest.h>

// Must be last.
#include "upb/port/def.inc"

#ifdef UPB_ARENA_DEBUG

namespace {

// Important: Each unit test needs to use its own unique arena value.

static const char* arena = (char*)0xdeadbeef;
static const void* tag0 = (void*)3;
static const void* tag1 = (void*)7;

TEST(RefTest, Correct1) {
  upb_Debug_IncRef(arena + 0, tag0);
  upb_Debug_DecRef(arena + 0, tag0);
}

TEST(RefTest, Correct2) {
  upb_Debug_IncRef(arena + 1, tag0);
  upb_Debug_IncRef(arena + 1, tag1);
  upb_Debug_DecRef(arena + 1, tag0);
  upb_Debug_DecRef(arena + 1, tag1);
}

TEST(RefTest, Correct3) {
  upb_Debug_IncRef(arena + 2, tag0);
  upb_Debug_IncRef(arena + 2, tag1);
  upb_Debug_DecRef(arena + 2, tag1);
  upb_Debug_DecRef(arena + 2, tag0);
}

TEST(RefTest, Recycle) {
  upb_Debug_IncRef(arena + 8, tag0);
  upb_Debug_DecRef(arena + 8, tag0);
  upb_Debug_IncRef(arena + 8, tag0);
  upb_Debug_DecRef(arena + 8, tag0);
}

TEST(RefTest, DoubleInc) {
  upb_Debug_IncRef(arena + 3, tag0);
  ASSERT_DEATH({ upb_Debug_IncRef(arena + 3, tag0); }, "arena owner exists");
}

TEST(RefTest, EmptyDec) {
  ASSERT_DEATH(
      { upb_Debug_DecRef(arena + 5, tag0); }, "arena owner does not exist");
}

}  // namespace

#endif  // UPB_ARENA_DEBUG
