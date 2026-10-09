// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/string_piece_field_support.h"

#include <utility>

#include <gtest/gtest.h>
#include "absl/strings/cord.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

TEST(StringPieceFieldTest, Move) {
  StringPieceField a;
  a.CopyFrom("a");
  StringPieceField b;
  b = std::move(a);
  EXPECT_EQ(b.Get(), "a");
}

TEST(StringPieceFieldTest, Swap) {
  StringPieceField a;
  a.CopyFrom("a");
  StringPieceField b;
  b.CopyFrom("b");
  a.Swap(/*arena=*/nullptr, &b, /*x_arena=*/nullptr);
  EXPECT_EQ(a.Get(), "b");
  EXPECT_EQ(b.Get(), "a");
}

TEST(StringPieceFieldTest, Cord) {
  StringPieceField a;
  a.CopyFrom(absl::Cord("a"));
  EXPECT_EQ(a.Get(), "a");
  StringPieceField b;
  b.CopyFrom(absl::Cord(""));
  EXPECT_EQ(b.Get(), "");
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
