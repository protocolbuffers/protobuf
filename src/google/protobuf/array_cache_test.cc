// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
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
//     * Neither the name of Google Inc. nor the names of its
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

#include "google/protobuf/array_cache.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
namespace {

using ::testing::ElementsAre;
using ::testing::Eq;

void* AsPtr(intptr_t n) { return reinterpret_cast<void*>(n); }

std::vector<void*> Crop(void** p, size_t n) {
  ABSL_ANNOTATE_MEMORY_IS_INITIALIZED(p, n * 8);
  PROTOBUF_UNPOISON_MEMORY_REGION(p, n * 8);
  return std::vector<void*>(p, p + n);
}

TEST(ArrayCacheTest, DonateArrayOnEmptyList) {
  for (size_t pow2 = 0; pow2 <= 20; ++pow2) {
    ArrayCache cache;

    size_t n = 2 << pow2;
    size_t size = n * sizeof(void*);
    std::vector<void*> pointer(n, reinterpret_cast<void*>(0xDEAD));
    cache.DonateArray(pointer.data(), size);
    EXPECT_THAT(cache.GetArrayCacheForTesting(), Eq(pointer.data()));

    ASSERT_THAT(pointer[0], Eq(AsPtr(pow2 + 1))) << " at pow2 " << pow2;

    std::vector<void*> cropped = Crop(pointer.data(), pow2 + 2);
    ASSERT_THAT(cropped[0], Eq(AsPtr(pow2 + 1))) << " at pow2 " << pow2;

    for (size_t index = 1; index <= pow2 + 1; ++index) {
      EXPECT_THAT(pointer[index], Eq(nullptr))
          << " at pow2 " << pow2 << ", index " << index;
      EXPECT_THAT(cropped[index], Eq(nullptr))
          << " at pow2 " << pow2 << ", index " << index;
    }
  }
}

TEST(ArrayCacheTest, DonateArrayToList) {
  ArrayCache cache;

  void* p_32_1[4];
  void* p_32_2[4];
  void* p_32_3[4];
  cache.DonateArray(p_32_1, 32);
  cache.DonateArray(p_32_2, 32);
  EXPECT_THAT(Crop(p_32_1, 3), ElementsAre(AsPtr(2), nullptr, p_32_2));
  EXPECT_THAT(Crop(p_32_2, 1), ElementsAre(nullptr));

  cache.DonateArray(p_32_3, 32);
  EXPECT_THAT(Crop(p_32_1, 3), ElementsAre(AsPtr(2), nullptr, p_32_3));
  EXPECT_THAT(Crop(p_32_3, 1), ElementsAre(p_32_2));
  EXPECT_THAT(Crop(p_32_2, 1), ElementsAre(nullptr));
}

TEST(ArrayCacheTest, DonateLargerArrayToList) {
  ArrayCache cache;

  void* p_32_1[4];
  void* p_64_1[8];
  cache.DonateArray(p_32_1, 32);
  cache.DonateArray(p_64_1, 64);
  EXPECT_THAT(Crop(p_64_1, 4), ElementsAre(AsPtr(3), nullptr, p_32_1, nullptr));
  EXPECT_THAT(Crop(p_32_1, 1), ElementsAre(nullptr));
}

TEST(ArrayCacheTest, DonateArray) {
  ArrayCache cache;

  void* p_32_1[4];
  void* p_32_2[4];
  void* p_32_3[4];

  void* p_64_1[8];
  void* p_64_2[8];
  void* p_64_3[8];

  cache.DonateArray(p_32_1, 32);
  EXPECT_THAT(Crop(p_32_1, 3), ElementsAre(AsPtr(2), nullptr, nullptr));

  cache.DonateArray(p_32_2, 32);
  EXPECT_THAT(Crop(p_32_1, 3), ElementsAre(AsPtr(2), nullptr, p_32_2));
  EXPECT_THAT(p_32_2[0], Eq(nullptr));

  cache.DonateArray(p_64_1, 64);
  EXPECT_THAT(Crop(p_64_1, 4), ElementsAre(AsPtr(3), nullptr, p_32_1, nullptr));
  EXPECT_THAT(Crop(p_32_1, 1), ElementsAre(p_32_2));
  EXPECT_THAT(p_32_2[0], Eq(nullptr));

  cache.DonateArray(p_64_2, 64);
  EXPECT_THAT(Crop(p_64_1, 4), ElementsAre(AsPtr(3), nullptr, p_32_1, p_64_2));
  EXPECT_THAT(Crop(p_64_2, 1), ElementsAre(nullptr));

  cache.DonateArray(p_64_3, 64);
  EXPECT_THAT(Crop(p_64_1, 4), ElementsAre(AsPtr(3), nullptr, p_32_1, p_64_3));
  EXPECT_THAT(Crop(p_64_3, 1), ElementsAre(p_64_2));
  EXPECT_THAT(Crop(p_64_2, 1), ElementsAre(nullptr));

  cache.DonateArray(p_32_3, 32);
  EXPECT_THAT(Crop(p_64_1, 4), ElementsAre(AsPtr(3), nullptr, p_32_3, p_64_3));
  EXPECT_THAT(Crop(p_32_3, 1), ElementsAre(p_32_1));
  EXPECT_THAT(Crop(p_32_1, 1), ElementsAre(p_32_2));
  EXPECT_THAT(Crop(p_32_2, 1), ElementsAre(nullptr));
}

TEST(ArrayCacheTest, AllocateArrayFromCache) {
  ArrayCache cache;

  void* p_64_1[8] = {nullptr, nullptr, nullptr, p_64_1};
  void* p_64_2[8] = {nullptr, nullptr, nullptr, p_64_2};
  void* p_64_3[8] = {nullptr, nullptr, nullptr, p_64_3};
  cache.DonateArray(p_64_1, 64);
  cache.DonateArray(p_64_2, 64);
  cache.DonateArray(p_64_3, 64);

  void* p_32_1[4] = {nullptr, nullptr, nullptr, p_32_1};
  void* p_32_2[4] = {nullptr, nullptr, nullptr, p_32_2};
  void* p_32_3[4] = {nullptr, nullptr, nullptr, p_32_3};
  cache.DonateArray(p_32_1, 32);
  cache.DonateArray(p_32_2, 32);
  cache.DonateArray(p_32_3, 32);

  ASSERT_THAT(p_64_1, ElementsAre(AsPtr(3), nullptr, p_32_3, p_64_3, nullptr,
                                  nullptr, nullptr, nullptr));

  EXPECT_THAT(cache.AllocateArray(32), Eq(p_32_3));
  ASSERT_THAT(p_64_1, ElementsAre(AsPtr(3), nullptr, p_32_2, p_64_3, nullptr,
                                  nullptr, nullptr, nullptr));
  EXPECT_THAT(cache.AllocateArray(24), Eq(p_32_2));
  EXPECT_THAT(cache.AllocateArray(32), Eq(p_32_1));
  EXPECT_THAT(cache.AllocateArray(32), Eq(nullptr));

  EXPECT_THAT(cache.AllocateArray(64), Eq(p_64_3));
  EXPECT_THAT(cache.AllocateArray(56), Eq(p_64_2));
  EXPECT_THAT(cache.AllocateArray(64), Eq(nullptr));
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
