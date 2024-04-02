// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/map.h"

#include <gtest/gtest.h>
#include "upb/base/string_view.h"
#include "upb/mem/arena.hpp"

TEST(MapTest, DeleteRegression) {
  upb::Arena arena;
  upb_Map* map = upb_Map_New(arena.ptr(), kUpb_CType_Int32, kUpb_CType_String);

  upb_MessageValue key;
  key.int32_val = 0;

  upb_MessageValue insert_value;
  insert_value.str_val = upb_StringView_FromString("abcde");

  upb_MapInsertStatus st = upb_Map_Insert(map, key, insert_value, arena.ptr());
  EXPECT_EQ(kUpb_MapInsertStatus_Inserted, st);

  upb_MessageValue delete_value;
  bool removed = upb_Map_Delete(map, key, &delete_value);
  EXPECT_TRUE(removed);

  EXPECT_TRUE(
      upb_StringView_IsEqual(insert_value.str_val, delete_value.str_val));
}
