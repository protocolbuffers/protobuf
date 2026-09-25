// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/map.h"

#include <cstdint>
#include <string>

#include <gtest/gtest.h>
#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.hpp"
#include "upb/message/array.h"
#include "upb/message/internal/map.h"

TEST(MapTest, DeleteRegression) {
  upb::Arena arena;
  upb_Map* map = upb_Map_New(arena.ptr(), kUpb_CType_Int32, kUpb_CType_String);
  ASSERT_NE(map, nullptr);

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

TEST(MapTest, DeleteMissingKeyStringValue) {
  upb::Arena arena;
  upb_Map* map = upb_Map_New(arena.ptr(), kUpb_CType_Int32, kUpb_CType_String);
  ASSERT_NE(map, nullptr);

  upb_MessageValue key;
  key.int32_val = 42;

  upb_MessageValue delete_value;
  bool removed = upb_Map_Delete(map, key, &delete_value);
  EXPECT_FALSE(removed);
}

TEST(MapTest, ReserveEmptyMap) {
  upb::Arena arena;
  upb_Map* map = upb_Map_New(arena.ptr(), kUpb_CType_Int32, kUpb_CType_Int32);
  ASSERT_NE(map, nullptr);
  EXPECT_EQ(_upb_Map_Capacity(map), 0);

  EXPECT_TRUE(upb_Map_Reserve(map, 20, arena.ptr()));
  EXPECT_EQ(_upb_Map_Capacity(map), 32);

  for (int32_t i = 1; i <= 20; i++) {
    upb_MessageValue k{.int32_val = i};
    upb_MessageValue v{.int32_val = i * 10};
    EXPECT_EQ(kUpb_MapInsertStatus_Inserted,
              upb_Map_Insert(map, k, v, arena.ptr()));
    EXPECT_EQ(_upb_Map_Capacity(map), 32);
  }
  EXPECT_EQ(upb_Map_Size(map), 20);
  for (int32_t i = 1; i <= 20; i++) {
    upb_MessageValue k{.int32_val = i};
    upb_MessageValue got;
    EXPECT_TRUE(upb_Map_Get(map, k, &got));
    EXPECT_EQ(got.int32_val, i * 10);
  }
}

TEST(MapTest, ReserveStringMap) {
  upb::Arena arena;
  upb_Map* map = upb_Map_New(arena.ptr(), kUpb_CType_String, kUpb_CType_Int32);
  ASSERT_NE(map, nullptr);
  EXPECT_EQ(_upb_Map_Capacity(map), 0);

  EXPECT_TRUE(upb_Map_Reserve(map, 50, arena.ptr()));
  EXPECT_EQ(_upb_Map_Capacity(map), 64);

  for (int i = 0; i < 50; i++) {
    std::string s = "key-" + std::to_string(i);
    upb_MessageValue k{.str_val =
                           upb_StringView_FromDataAndSize(s.data(), s.size())};
    upb_MessageValue v{.int32_val = i};
    EXPECT_EQ(kUpb_MapInsertStatus_Inserted,
              upb_Map_Insert(map, k, v, arena.ptr()));
    EXPECT_EQ(_upb_Map_Capacity(map), 64);
  }
  EXPECT_EQ(upb_Map_Size(map), 50);
}

TEST(MapTest, ReserveGrowExistingMap) {
  upb::Arena arena;
  upb_Map* map = upb_Map_New(arena.ptr(), kUpb_CType_Int32, kUpb_CType_Int32);
  ASSERT_NE(map, nullptr);
  for (int32_t i = 1; i <= 3; i++) {
    upb_MessageValue k{.int32_val = i};
    upb_MessageValue v{.int32_val = i * 10};
    EXPECT_EQ(kUpb_MapInsertStatus_Inserted,
              upb_Map_Insert(map, k, v, arena.ptr()));
  }
  EXPECT_EQ(_upb_Map_Capacity(map), 8);

  EXPECT_TRUE(upb_Map_Reserve(map, 50, arena.ptr()));
  EXPECT_GE(_upb_Map_Capacity(map), 64);
  EXPECT_EQ(upb_Map_Size(map), 3);
  for (int32_t i = 1; i <= 3; i++) {
    upb_MessageValue k{.int32_val = i};
    upb_MessageValue got;
    EXPECT_TRUE(upb_Map_Get(map, k, &got));
    EXPECT_EQ(got.int32_val, i * 10);
  }
}
