// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
