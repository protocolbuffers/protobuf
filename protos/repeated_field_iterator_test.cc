// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "protos/repeated_field_iterator.h"

#include <algorithm>
#include <array>
#include <numeric>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::ElementsAre;

namespace protos {
namespace internal {

template <typename T>
using ScalarRef = ReferenceProxy<ScalarIteratorPolicy<T>>;
template <typename T>
using ScalarIterator = Iterator<ScalarIteratorPolicy<T>>;

template <typename T>
using StringRef = ReferenceProxy<StringIteratorPolicy<T>>;
template <typename T>
using StringIterator = Iterator<StringIteratorPolicy<T>>;

struct IteratorTestPeer {
  template <typename T>
  static ScalarRef<T> MakeScalarRefProxy(T& ref) {
    return ScalarRef<T>({&ref});
  }

  template <typename T>
  static ScalarIterator<T> MakeScalarIterator(T* ptr) {
    return ScalarIterator<T>({ptr});
  }

  template <typename T>
  static StringRef<T> MakeStringRefProxy(upb_Array* arr, protos::Arena& arena) {
    return StringRef<T>({arr, arena.ptr(), 0});
  }

  template <typename T>
  static StringIterator<T> MakeStringIterator(upb_Array* arr,
                                              protos::Arena& arena) {
    return StringIterator<T>({arr, arena.ptr()});
  }
};

namespace {

TEST(ScalarReferenceTest, BasicOperationsWork) {
  int i = 0;
  ScalarRef<int> p = IteratorTestPeer::MakeScalarRefProxy(i);
  ScalarRef<const int> cp =
      IteratorTestPeer::MakeScalarRefProxy(std::as_const(i));
  EXPECT_EQ(i, 0);
  p = 17;
  EXPECT_EQ(i, 17);
  EXPECT_EQ(p, 17);
  EXPECT_EQ(cp, 17);
  i = 13;
  EXPECT_EQ(p, 13);
  EXPECT_EQ(cp, 13);

  EXPECT_FALSE((std::is_assignable<decltype(cp), int>::value));

  // Check that implicit conversion works T -> const T
  ScalarRef<const int> cp2 = p;
  EXPECT_EQ(cp2, 13);

  EXPECT_FALSE((std::is_convertible<decltype(cp), ScalarRef<int>>::value));
}

TEST(ScalarReferenceTest, AssignmentAndSwap) {
  int i = 3;
  int j = 5;
  ScalarRef<int> p = IteratorTestPeer::MakeScalarRefProxy(i);
  ScalarRef<int> p2 = IteratorTestPeer::MakeScalarRefProxy(j);

  EXPECT_EQ(p, 3);
  EXPECT_EQ(p2, 5);
  swap(p, p2);
  EXPECT_EQ(p, 5);
  EXPECT_EQ(p2, 3);

  p = p2;
  EXPECT_EQ(p, 3);
  EXPECT_EQ(p2, 3);
}

template <typename T, typename U>
std::array<bool, 6> RunCompares(const T& a, const U& b) {
  // Verify some basic properties here.
  // Equivalencies
  EXPECT_EQ((a == b), (b == a));
  EXPECT_EQ((a != b), (b != a));
  EXPECT_EQ((a < b), (b > a));
  EXPECT_EQ((a > b), (b < a));
  EXPECT_EQ((a <= b), (b >= a));
  EXPECT_EQ((a >= b), (b <= a));

  // Opposites
  EXPECT_NE((a == b), (a != b));
  EXPECT_NE((a < b), (a >= b));
  EXPECT_NE((a > b), (a <= b));

  return {{
      (a == b),
      (a != b),
      (a < b),
      (a <= b),
      (a > b),
      (a >= b),
  }};
}

template <typename T>
void TestScalarIterator(T* array) {
  ScalarIterator<T> it = IteratorTestPeer::MakeScalarIterator(array);
  // Copy
  auto it2 = it;

  EXPECT_THAT(RunCompares(it, it2),
              ElementsAre(true, false, false, true, false, true));

  // Increment
  EXPECT_EQ(*++it, 11);
  EXPECT_EQ(*it2, 10);
  EXPECT_EQ(*it++, 11);
  EXPECT_EQ(*it2, 10);
  EXPECT_EQ(*it, 12);
  EXPECT_EQ(*it2, 10);

  EXPECT_THAT(RunCompares(it, it2),
              ElementsAre(false, true, false, false, true, true));

  // Assign
  it2 = it;
  EXPECT_EQ(*it, 12);
  EXPECT_EQ(*it2, 12);

  // Decrement
  EXPECT_EQ(*--it, 11);
  EXPECT_EQ(*it--, 11);
  EXPECT_EQ(*it, 10);

  it += 5;
  EXPECT_EQ(*it, 15);
  EXPECT_EQ(it - it2, 3);
  EXPECT_EQ(it2 - it, -3);
  it -= 3;
  EXPECT_EQ(*it, 12);
  EXPECT_EQ(it[6], 18);
  EXPECT_EQ(it[-1], 11);
}

TEST(ScalarIteratorTest, BasicOperationsWork) {
  int array[10] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
  TestScalarIterator<const int>(array);
  TestScalarIterator<int>(array);
}

TEST(ScalarIteratorTest, Convertibility) {
  int array[10] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
  ScalarIterator<int> it = IteratorTestPeer::MakeScalarIterator(array);
  it += 4;
  ScalarIterator<const int> cit = it;
  EXPECT_EQ(*it, 14);
  EXPECT_EQ(*cit, 14);
  it += 2;
  EXPECT_EQ(*it, 16);
  EXPECT_EQ(*cit, 14);
  cit = it;
  EXPECT_EQ(*it, 16);
  EXPECT_EQ(*cit, 16);

  EXPECT_FALSE((std::is_convertible<ScalarIterator<const int>,
                                    ScalarIterator<int>>::value));
  EXPECT_FALSE((std::is_assignable<ScalarIterator<int>,
                                   ScalarIterator<const int>>::value));
}

TEST(ScalarIteratorTest, MutabilityOnlyWorksOnMutable) {
  int array[10] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
  ScalarIterator<int> it = IteratorTestPeer::MakeScalarIterator(array);
  EXPECT_EQ(array[3], 13);
  it[3] = 113;
  EXPECT_EQ(array[3], 113);
  ScalarIterator<const int> cit = it;
  EXPECT_FALSE((std::is_assignable<decltype(*cit), int>::value));
  EXPECT_FALSE((std::is_assignable<decltype(cit[1]), int>::value));
}

TEST(ScalarIteratorTest, IteratorReferenceInteraction) {
  int array[10] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
  ScalarIterator<int> it = IteratorTestPeer::MakeScalarIterator(array);
  EXPECT_EQ(it[4], 14);
  // op& from references goes back to iterator.
  ScalarIterator<int> it2 = &it[4];
  EXPECT_EQ(it + 4, it2);
}

TEST(ScalarIteratorTest, IteratorBasedAlgorithmsWork) {
  // We use a vector here to make testing it easier.
  std::vector<int> v(10, 0);
  ScalarIterator<int> it = IteratorTestPeer::MakeScalarIterator(v.data());
  EXPECT_THAT(v, ElementsAre(0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
  std::iota(it, it + 10, 10);
  EXPECT_THAT(v, ElementsAre(10, 11, 12, 13, 14, 15, 16, 17, 18, 19));
  EXPECT_EQ(it + 5, std::find(it, it + 10, 15));
  EXPECT_EQ(145, std::accumulate(it, it + 10, 0));
  std::sort(it, it + 10, [](int a, int b) {
    return std::tuple(a % 2, a) < std::tuple(b % 2, b);
  });
  EXPECT_THAT(v, ElementsAre(10, 12, 14, 16, 18, 11, 13, 15, 17, 19));
}

const char* CloneString(protos::Arena& arena, absl::string_view str) {
  char* data = (char*)upb_Arena_Malloc(arena.ptr(), str.size());
  memcpy(data, str.data(), str.size());
  return data;
}
upb_Array* MakeStringArray(protos::Arena& arena,
                           const std::vector<std::string>& input) {
  upb_Array* arr = upb_Array_New(arena.ptr(), kUpb_CType_String);
  for (absl::string_view str : input) {
    upb_MessageValue message_value;
    message_value.str_val =
        upb_StringView_FromDataAndSize(CloneString(arena, str), str.size());
    upb_Array_Append(arr, message_value, arena.ptr());
  }
  return arr;
}

TEST(StringReferenceTest, BasicOperationsWork) {
  protos::Arena arena;
  upb_Array* arr = MakeStringArray(arena, {""});

  auto read = [&] {
    upb_MessageValue message_value = upb_Array_Get(arr, 0);
    return absl::string_view(message_value.str_val.data,
                             message_value.str_val.size);
  };

  StringRef<absl::string_view> p =
      IteratorTestPeer::MakeStringRefProxy<absl::string_view>(arr, arena);
  StringRef<const absl::string_view> cp =
      IteratorTestPeer::MakeStringRefProxy<const absl::string_view>(arr, arena);
  EXPECT_EQ(read(), "");
  EXPECT_EQ(p, "");
  p = "ABC";
  EXPECT_EQ(read(), "ABC");
  EXPECT_EQ(p, "ABC");
  EXPECT_EQ(cp, "ABC");
  const_cast<char*>(read().data())[0] = 'X';
  EXPECT_EQ(read(), "XBC");
  EXPECT_EQ(p, "XBC");
  EXPECT_EQ(cp, "XBC");

  EXPECT_FALSE((std::is_assignable<decltype(cp), int>::value));

  // Check that implicit conversion works T -> const T
  StringRef<const absl::string_view> cp2 = p;
  EXPECT_EQ(cp2, "XBC");

  EXPECT_FALSE(
      (std::is_convertible<decltype(cp), StringRef<absl::string_view>>::value));

  EXPECT_THAT(RunCompares(p, "XBC"),
              ElementsAre(true, false, false, true, false, true));
  EXPECT_THAT(RunCompares(p, "YBC"),
              ElementsAre(false, true, true, true, false, false));
  EXPECT_THAT(RunCompares(p, "RBC"),
              ElementsAre(false, true, false, false, true, true));
  EXPECT_THAT(RunCompares(p, "XB"),
              ElementsAre(false, true, false, false, true, true));
  EXPECT_THAT(RunCompares(p, "XBCD"),
              ElementsAre(false, true, true, true, false, false));
}

TEST(StringReferenceTest, AssignmentAndSwap) {
  protos::Arena arena;
  upb_Array* arr1 = MakeStringArray(arena, {"ABC"});
  upb_Array* arr2 = MakeStringArray(arena, {"DEF"});

  auto p = IteratorTestPeer::MakeStringRefProxy<absl::string_view>(arr1, arena);
  auto p2 =
      IteratorTestPeer::MakeStringRefProxy<absl::string_view>(arr2, arena);

  EXPECT_EQ(p, "ABC");
  EXPECT_EQ(p2, "DEF");
  swap(p, p2);
  EXPECT_EQ(p, "DEF");
  EXPECT_EQ(p2, "ABC");

  p = p2;
  EXPECT_EQ(p, "ABC");
  EXPECT_EQ(p2, "ABC");
}

template <typename T>
void TestStringIterator(protos::Arena& arena, upb_Array* array) {
  StringIterator<T> it = IteratorTestPeer::MakeStringIterator<T>(array, arena);
  // Copy
  auto it2 = it;

  EXPECT_THAT(RunCompares(it, it2),
              ElementsAre(true, false, false, true, false, true));

  // Increment
  EXPECT_EQ(*++it, "11");
  EXPECT_EQ(*it2, "10");
  EXPECT_EQ(*it++, "11");
  EXPECT_EQ(*it2, "10");
  EXPECT_EQ(*it, "12");
  EXPECT_EQ(*it2, "10");

  EXPECT_THAT(RunCompares(it, it2),
              ElementsAre(false, true, false, false, true, true));

  // Assign
  it2 = it;
  EXPECT_EQ(*it, "12");
  EXPECT_EQ(*it2, "12");

  // Decrement
  EXPECT_EQ(*--it, "11");
  EXPECT_EQ(*it--, "11");
  EXPECT_EQ(*it, "10");

  it += 5;
  EXPECT_EQ(*it, "15");
  EXPECT_EQ(it - it2, 3);
  EXPECT_EQ(it2 - it, -3);
  it -= 3;
  EXPECT_EQ(*it, "12");
  EXPECT_EQ(it[6], "18");
  EXPECT_EQ(it[-1], "11");
}

TEST(StringIteratorTest, BasicOperationsWork) {
  protos::Arena arena;
  auto* array = MakeStringArray(
      arena, {"10", "11", "12", "13", "14", "15", "16", "17", "18", "19"});
  TestStringIterator<const absl::string_view>(arena, array);
  TestStringIterator<absl::string_view>(arena, array);
}

TEST(StringIteratorTest, Convertibility) {
  protos::Arena arena;
  auto* array = MakeStringArray(
      arena, {"10", "11", "12", "13", "14", "15", "16", "17", "18", "19"});
  StringIterator<absl::string_view> it =
      IteratorTestPeer::MakeStringIterator<absl::string_view>(array, arena);
  it += 4;
  StringIterator<const absl::string_view> cit = it;
  EXPECT_EQ(*it, "14");
  EXPECT_EQ(*cit, "14");
  it += 2;
  EXPECT_EQ(*it, "16");
  EXPECT_EQ(*cit, "14");
  cit = it;
  EXPECT_EQ(*it, "16");
  EXPECT_EQ(*cit, "16");

  EXPECT_FALSE((std::is_convertible<StringIterator<const absl::string_view>,
                                    StringIterator<absl::string_view>>::value));
  EXPECT_FALSE(
      (std::is_assignable<StringIterator<absl::string_view>,
                          StringIterator<const absl::string_view>>::value));
}

TEST(StringIteratorTest, MutabilityOnlyWorksOnMutable) {
  protos::Arena arena;
  auto* array = MakeStringArray(
      arena, {"10", "11", "12", "13", "14", "15", "16", "17", "18", "19"});
  StringIterator<absl::string_view> it =
      IteratorTestPeer::MakeStringIterator<absl::string_view>(array, arena);

  auto read = [&] {
    upb_MessageValue message_value = upb_Array_Get(array, 3);
    return absl::string_view(message_value.str_val.data,
                             message_value.str_val.size);
  };

  EXPECT_EQ(read(), "13");
  it[3] = "113";
  EXPECT_EQ(read(), "113");
  StringIterator<const absl::string_view> cit = it;
  EXPECT_FALSE((std::is_assignable<decltype(*cit), absl::string_view>::value));
  EXPECT_FALSE(
      (std::is_assignable<decltype(cit[1]), absl::string_view>::value));
}

TEST(StringIteratorTest, IteratorReferenceInteraction) {
  protos::Arena arena;
  auto* array = MakeStringArray(
      arena, {"10", "11", "12", "13", "14", "15", "16", "17", "18", "19"});
  StringIterator<absl::string_view> it =
      IteratorTestPeer::MakeStringIterator<absl::string_view>(array, arena);
  EXPECT_EQ(it[4], "14");
  // op& from references goes back to iterator.
  StringIterator<absl::string_view> it2 = &it[4];
  EXPECT_EQ(it + 4, it2);
}

TEST(StringIteratorTest, IteratorBasedAlgorithmsWork) {
  protos::Arena arena;
  auto* array = MakeStringArray(
      arena, {"10", "11", "12", "13", "14", "15", "16", "17", "18", "19"});
  StringIterator<absl::string_view> it =
      IteratorTestPeer::MakeStringIterator<absl::string_view>(array, arena);

  auto read = [&] {
    std::vector<absl::string_view> v;
    for (int i = 0; i < 10; ++i) {
      upb_MessageValue message_value = upb_Array_Get(array, i);
      v.emplace_back(message_value.str_val.data, message_value.str_val.size);
    }
    return v;
  };

  EXPECT_THAT(read(), ElementsAre("10", "11", "12", "13", "14",  //
                                  "15", "16", "17", "18", "19"));
  std::sort(it, it + 10, [](absl::string_view a, absl::string_view b) {
    return std::tuple(a[1] % 2, a) < std::tuple(b[1] % 2, b);
  });
  EXPECT_THAT(read(), ElementsAre("10", "12", "14", "16", "18",  //
                                  "11", "13", "15", "17", "19"));
  // Now sort with the default less.
  std::sort(it, it + 10);
  EXPECT_THAT(read(), ElementsAre("10", "11", "12", "13", "14",  //
                                  "15", "16", "17", "18", "19"));

  // Mutable algorithm
  std::generate(it, it + 10,
                [i = 0]() mutable { return std::string(i++, 'x'); });
  EXPECT_THAT(read(),
              ElementsAre("", "x", "xx", "xxx", "xxxx", "xxxxx", "xxxxxx",
                          "xxxxxxx", "xxxxxxxx", "xxxxxxxxx"));
}

}  // namespace
}  // namespace internal
}  // namespace protos
