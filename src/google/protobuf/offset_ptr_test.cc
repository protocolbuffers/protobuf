// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/offset_ptr.h"

#include <cstdint>
#include <limits>
#include <type_traits>

#include <gtest/gtest.h>
#include "google/protobuf/unittest.pb.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

int array[10];

TEST(BasePtrTest, Basic) {
  BasePointer<int, false> b(&array[5], &array);
  EXPECT_EQ(&array[5], b.Resolve(&array));
}

TEST(BasePtrTest, Copyable) {
  EXPECT_TRUE(
      (std::is_trivially_copy_constructible_v<BasePointer<int, false>>));
  EXPECT_TRUE((std::is_trivially_copy_assignable_v<BasePointer<int, false>>));
  EXPECT_TRUE((std::is_trivially_destructible_v<BasePointer<int, false>>));

  BasePointer<int, false> b(&array[7], &array);
  EXPECT_EQ(&array[7], b.Resolve(&array));

  auto b2 = b;
  EXPECT_EQ(&array[7], b2.Resolve(&array));
}

TEST(BasePtrTest, BaseCanBeOnEitherSide) {
  EXPECT_EQ(&array[7],
            (BasePointer<int, false>(&array[7], &array[3])).Resolve(&array[3]));
  EXPECT_EQ(&array[4],
            (BasePointer<int, false>(&array[4], &array[7])).Resolve(&array[7]));
}

TEST(BasePtrTest, BaseCanBeSameAsPointer) {
  int var = 0;

  EXPECT_EQ(&var, (BasePointer<int, false>(&var, &var)).Resolve(&var));
}

TEST(BasePtrTest, NullIsAllowed) {
  bool dummy = false;
  BasePointer<int, true> b(nullptr, &dummy);
  EXPECT_EQ(nullptr, b.Resolve(&dummy));

  int p = 0;
  EXPECT_DEATH((BasePointer<int, true>(&p, &p)),
               "Nullable base pointer can't self reference");

  EXPECT_DEATH((BasePointer<int, false>(nullptr, &dummy)),
               "Non-nullable base pointer constructed with a null pointer");
}

TEST(BasePtrTest, OutOfScopeFails) {
  // For this one we manufacture pointers to make sure they trigger out of scope
  // failures.
  if (sizeof(void*) == 4) {
    GTEST_SKIP() << "We need 64-bit to have out-of-scope pointers.";
  }

  void* base = reinterpret_cast<void*>(uintptr_t{0x123457890abcdef});

  const auto make_ptr = [&](ptrdiff_t diff) {
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(base) + diff);
  };

  using T = BasePointer<void, false>;

  // INT_MAX is fine.
  EXPECT_EQ(T(make_ptr(std::numeric_limits<int>::max()), base).Resolve(base),
            make_ptr(std::numeric_limits<int>::max()));

  // INT_MAX+1 is too much.
  EXPECT_DEATH(
      T(make_ptr(ptrdiff_t{std::numeric_limits<int>::max()} + 1), base),
      "Pointer out of scope for offset pointer");

  // INT_MIN is fine.
  EXPECT_EQ(T(make_ptr(std::numeric_limits<int>::min()), base).Resolve(base),
            make_ptr(std::numeric_limits<int>::min()));

  // INT_MIN-1 is too much.
  EXPECT_DEATH(
      T(make_ptr(ptrdiff_t{std::numeric_limits<int>::min()} - 1), base),
      "Pointer out of scope for offset pointer");
}

TEST(OffsetPtrTest, Basic) {
  struct Object {
    int array[10];
    OffsetPtr<int, false> ptr;
  } object{};

  object.ptr = object.array + 3;
  EXPECT_EQ(object.ptr, object.array + 3);
}

TEST(OffsetProtoPtr, Basic) {
  using P = proto2_unittest::TestAllTypes;
  OffsetProtoPtr<const P> p;

  p = nullptr;
  EXPECT_EQ(&P::default_instance(), p);

  P msg;
  p = &msg;
  EXPECT_EQ(&msg, p);

  p = &P::default_instance();
  EXPECT_EQ(&P::default_instance(), p);
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
