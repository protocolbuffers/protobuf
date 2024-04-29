// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Tests for upb_table.

#include <limits.h>
#include <string.h>

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "absl/container/flat_hash_map.h"
#include "upb/hash/int_table.h"
#include "upb/hash/str_table.h"
#include "upb/mem/arena.hpp"

// Must be last.
#include "upb/port/def.inc"

using std::vector;

TEST(Table, StringTable) {
  vector<std::string> keys;
  keys.push_back("google.protobuf.FileDescriptorSet");
  keys.push_back("google.protobuf.FileDescriptorProto");
  keys.push_back("google.protobuf.DescriptorProto");
  keys.push_back("google.protobuf.DescriptorProto.ExtensionRange");
  keys.push_back("google.protobuf.FieldDescriptorProto");
  keys.push_back("google.protobuf.EnumDescriptorProto");
  keys.push_back("google.protobuf.EnumValueDescriptorProto");
  keys.push_back("google.protobuf.ServiceDescriptorProto");
  keys.push_back("google.protobuf.MethodDescriptorProto");
  keys.push_back("google.protobuf.FileOptions");
  keys.push_back("google.protobuf.MessageOptions");
  keys.push_back("google.protobuf.FieldOptions");
  keys.push_back("google.protobuf.EnumOptions");
  keys.push_back("google.protobuf.EnumValueOptions");
  keys.push_back("google.protobuf.ServiceOptions");
  keys.push_back("google.protobuf.MethodOptions");
  keys.push_back("google.protobuf.UninterpretedOption");
  keys.push_back("google.protobuf.UninterpretedOption.NamePart");

  /* Initialize structures. */
  upb::Arena arena;
  upb_strtable t;
  upb_strtable_init(&t, keys.size(), arena.ptr());
  std::map<std::string, int32_t> m;
  std::set<std::string> all;
  for (const auto& key : keys) {
    all.insert(key);
    upb_value val = {uint64_t(key[0])};
    upb_strtable_insert(&t, key.data(), key.size(), val, arena.ptr());
    m[key] = key[0];
  }

  /* Test correctness. */
  for (const auto& key : keys) {
    upb_value val;
    bool ok = upb_strtable_lookup2(&t, key.data(), key.size(), &val);
    EXPECT_TRUE(ok);
    EXPECT_EQ(val.val, uint64_t(key[0]));
    EXPECT_EQ(m[key], key[0]);
  }

  intptr_t iter = UPB_STRTABLE_BEGIN;
  upb_StringView key;
  upb_value val;
  while (upb_strtable_next2(&t, &key, &val, &iter)) {
    std::set<std::string>::iterator i = all.find(key.data);
    EXPECT_NE(i, all.end());
    all.erase(i);
  }
  EXPECT_TRUE(all.empty());

  // Test iteration with resizes.

  for (int i = 0; i < 10; i++) {
    intptr_t iter = UPB_STRTABLE_BEGIN;
    while (upb_strtable_next2(&t, &key, &val, &iter)) {
      // Even if we invalidate the iterator it should only return real elements.
      EXPECT_EQ(val.val, m[key.data]);

      // Force a resize even though the size isn't changing.
      // Also forces the table size to grow so some new buckets end up empty.
      bool ok = upb_strtable_resize(&t, 5 + i, arena.ptr());
      EXPECT_TRUE(ok);
    }
  }
}

class IntTableTest : public testing::TestWithParam<int> {
  void SetUp() override {
    if (GetParam() > 0) {
      for (int i = 0; i < GetParam(); i++) {
        keys_.push_back(i + 1);
      }
    } else {
      for (int32_t i = 0; i < 64; i++) {
        if (i < 32)
          keys_.push_back(i + 1);
        else
          keys_.push_back(10101 + i);
      }
    }
  }

 protected:
  std::vector<int32_t> keys_;
};

TEST_P(IntTableTest, TestIntTable) {
  /* Initialize structures. */
  upb::Arena arena;
  upb_inttable t;
  upb_inttable_init(&t, arena.ptr());
  uint32_t largest_key = 0;
  std::map<uint32_t, uint32_t> m;
  absl::flat_hash_map<uint32_t, uint32_t> hm;
  for (const auto& key : keys_) {
    largest_key = UPB_MAX((int32_t)largest_key, key);
    upb_value val = upb_value_uint32(key * 2);
    bool ok = upb_inttable_insert(&t, key, val, arena.ptr());
    EXPECT_TRUE(ok);
    m[key] = key * 2;
    hm[key] = key * 2;
  }
  EXPECT_EQ(upb_inttable_count(&t), keys_.size());

  /* Test correctness. */
  int count = 0;
  for (uint32_t i = 0; i <= largest_key; i++) {
    upb_value val;
    bool ok = upb_inttable_lookup(&t, i, &val);
    if (ok) { /* Assume map implementation is correct. */
      EXPECT_EQ(val.val, i * 2);
      EXPECT_EQ(m[i], i * 2);
      EXPECT_EQ(hm[i], i * 2);
      count++;
    }
  }
  EXPECT_EQ(count, keys_.size());
  EXPECT_EQ(count, upb_inttable_count(&t));

  // Test replace.
  count = 0;
  for (uint32_t i = 0; i <= largest_key; i++) {
    upb_value val = upb_value_uint32(i * 3);
    bool ok = upb_inttable_replace(&t, i, val);
    if (ok) { /* Assume map implementation is correct. */
      m[i] = i * 3;
      hm[i] = i * 3;
      count++;
    }
  }
  EXPECT_EQ(count, keys_.size());
  EXPECT_EQ(count, upb_inttable_count(&t));

  // Compact and test correctness again.
  upb_inttable_compact(&t, arena.ptr());
  count = 0;
  for (uint32_t i = 0; i <= largest_key; i++) {
    upb_value val;
    bool ok = upb_inttable_lookup(&t, i, &val);
    if (ok) { /* Assume map implementation is correct. */
      EXPECT_EQ(val.val, i * 3);
      EXPECT_EQ(m[i], i * 3);
      EXPECT_EQ(hm[i], i * 3);
      count++;
    }
  }
  EXPECT_EQ(count, keys_.size());
  EXPECT_EQ(count, upb_inttable_count(&t));

  for (const auto& key : keys_) {
    upb_value val;
    bool ok = upb_inttable_remove(&t, key, &val);
    EXPECT_TRUE(ok);
    EXPECT_EQ(val.val, (uint32_t)key * 3);
    count--;
    EXPECT_EQ(count, upb_inttable_count(&t));
  }
  EXPECT_EQ(0, upb_inttable_count(&t));
}

INSTANTIATE_TEST_SUITE_P(IntTableParams, IntTableTest,
                         testing::Values(8, 64, 512, -32));

/*
 * This test can't pass right now because the table can't store a value of
 * (uint64_t)-1.
 */
TEST(Table, MaxValue) {
  /*
    typedef upb::TypedIntTable<uint64_t> Table;
    Table table;
    uintptr_t uint64_max = (uint64_t)-1;
    table.Insert(1, uint64_max);
    std::pair<bool, uint64_t> found = table.Lookup(1);
    ASSERT(found.first);
    ASSERT(found.second == uint64_max);
  */
}

TEST(Table, Delete) {
  upb::Arena arena;
  upb_inttable t;
  upb_inttable_init(&t, arena.ptr());
  upb_inttable_insert(&t, 0, upb_value_bool(true), arena.ptr());
  upb_inttable_insert(&t, 2, upb_value_bool(true), arena.ptr());
  upb_inttable_insert(&t, 4, upb_value_bool(true), arena.ptr());
  upb_inttable_compact(&t, arena.ptr());
  upb_inttable_remove(&t, 0, nullptr);
  upb_inttable_remove(&t, 2, nullptr);
  upb_inttable_remove(&t, 4, nullptr);

  intptr_t iter = UPB_INTTABLE_BEGIN;
  uintptr_t key;
  upb_value val;
  while (upb_inttable_next(&t, &key, &val, &iter)) {
    FAIL();
  }
}

TEST(Table, Init) {
  for (int i = 0; i < 2048; i++) {
    /* Tests that the size calculations in init() (lg2 size for target load)
     * work for all expected sizes. */
    upb::Arena arena;
    upb_strtable t;
    upb_strtable_init(&t, i, arena.ptr());
  }
}
