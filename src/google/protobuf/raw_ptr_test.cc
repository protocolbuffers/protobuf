// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/raw_ptr.h"

#include <cstdint>

#include <gtest/gtest.h>
#include "absl/base/optimization.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

TEST(ZeroCacheline, Basic) {
  for (int i = 0; i < ABSL_CACHELINE_SIZE; ++i) {
    EXPECT_EQ(kZeroBuffer[i], 0) << i;
  }
  EXPECT_EQ(reinterpret_cast<uintptr_t>(kZeroBuffer) % ABSL_CACHELINE_SIZE, 0);
}

struct Obj {
  int i;
};

TEST(RawPtr, Basic) {
  RawPtr<Obj> raw;
  EXPECT_EQ(raw->i, 0);
  EXPECT_EQ((*raw).i, 0);
  EXPECT_EQ(static_cast<void*>(&raw->i), kZeroBuffer);
  EXPECT_EQ(static_cast<void*>(raw.Get()), kZeroBuffer);
  EXPECT_TRUE(raw.IsDefault());

  Obj obj = {1};
  raw.Set(&obj);
  EXPECT_EQ(raw->i, 1);
  EXPECT_EQ(static_cast<void*>(raw.Get()), &obj);
  EXPECT_FALSE(raw.IsDefault());
}

TEST(RawPtr, Constexpr) {
  constexpr RawPtr<Obj> raw;
  EXPECT_EQ(raw->i, 0);
  EXPECT_EQ((*raw).i, 0);
  EXPECT_EQ(static_cast<void*>(&raw->i), kZeroBuffer);
  EXPECT_EQ(static_cast<void*>(raw.Get()), kZeroBuffer);
  EXPECT_TRUE(raw.IsDefault());

  static constexpr Obj obj = {1};
  constexpr RawPtr<Obj> raw2(&obj);
  EXPECT_EQ(raw2->i, 1);
  EXPECT_EQ((*raw2).i, 1);
  EXPECT_EQ(static_cast<void*>(raw2.Get()), &obj);
  EXPECT_FALSE(raw2.IsDefault());
}

TEST(RawPtr, DeleteIfNotDefault) {
  RawPtr<Obj> raw;
  EXPECT_TRUE(raw.IsDefault());

  // Shouldn't trigger an allocator problem by deallocating default ptr.
  raw.DeleteIfNotDefault();

  raw.Set(new Obj());
  EXPECT_FALSE(raw.IsDefault());

  // Shouldn't leak.
  raw.DeleteIfNotDefault();
}

TEST(RawPtr, ClearIfNotDefault) {
  struct ObjectWithClear {
    int called = 0;
    void Clear() { ++called; }
  };
  RawPtr<ObjectWithClear> raw;
  EXPECT_TRUE(raw.IsDefault());

  // Shouldn't trigger / crash
  raw.ClearIfNotDefault();
  EXPECT_EQ(raw.Get()->called, 0);

  raw.Set(new ObjectWithClear());
  EXPECT_FALSE(raw.IsDefault());

  // Should invoke Clear
  raw.ClearIfNotDefault();
  EXPECT_EQ(raw.Get()->called, 1);

  raw.DeleteIfNotDefault();
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
