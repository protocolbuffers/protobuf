// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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

#include "google/protobuf/intrinsic.h"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// clang-format off
#include "google/protobuf/port_def.inc"
// clang-format on

using testing::ElementsAre;

namespace google {
namespace protobuf {
namespace {

TEST(BitSet, WorksWithSmallOffset) {
  uint32_t v32 = 0;
  internal::BitSet<uint32_t>(&v32, 7);
  EXPECT_EQ(v32, 1 << 7);
  internal::BitSet<uint32_t>(&v32, 15);
  EXPECT_EQ(v32, (1 << 7) | (1 << 15));

  uint8_t v8 = 0;
  internal::BitSet<uint8_t>(&v8, 7);
  EXPECT_EQ(v8, 1 << 7);
  internal::BitSet<uint8_t>(&v8, 3);
  EXPECT_EQ(v8, (1 << 7) | (1 << 3));
}

TEST(BitSet, WorksWithLargeOffset) {
  struct S {
    double d = 1.5;
    std::string str = "Hello";
    uint32_t value = 0b10001;
    uint8_t after = 0x12;
  } s;
  internal::BitSet<uint32_t>(&s, 8 * PROTOBUF_FIELD_OFFSET(S, value) + 10);
  EXPECT_EQ(s.value, 0b10000010001);
  // and verify the rest is fine.
  EXPECT_EQ(s.d, 1.5);
  EXPECT_EQ(s.str, "Hello");
  EXPECT_EQ(s.after, 0x12);
}

template <typename T, typename U>
auto TestAllBits(const U& v, int start = 0) {
  std::vector<int> out;
  for (size_t i = 0; i < sizeof(T) * 8; ++i) {
    out.push_back(internal::BitTest<T>(v, start + i));
  }
  return out;
}

TEST(BitTest, WorksWithSmallOffset) {
  EXPECT_THAT(TestAllBits<uint32_t>(uint32_t{0}),
              ElementsAre(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
  EXPECT_THAT(
      TestAllBits<uint32_t>(uint32_t{0b10000000000000000010001100100010}),
      ElementsAre(0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0,  //
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1));
}

TEST(BitTest, WorksWithLargeOffset) {
  struct S {
    double d = 1.5;
    std::string str = "Hello";
    uint32_t value = 0;
    uint8_t after = 0x12;
  } s;

  constexpr size_t offset = 8 * PROTOBUF_FIELD_OFFSET(S, value);
  EXPECT_THAT(TestAllBits<uint32_t>(s, offset),
              ElementsAre(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
  s.value = 0b10000000000000000010001100100010;
  EXPECT_THAT(TestAllBits<uint32_t>(s, offset),
              ElementsAre(0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0,  //
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1));
}

}  // namespace
}  // namespace protobuf
}  // namespace google
