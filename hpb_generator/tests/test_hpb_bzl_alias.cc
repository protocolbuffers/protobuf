// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
#include "google/protobuf/compiler/hpb/tests/set_alias.upb.proto.h"
#include "google/protobuf/hpb/arena.h"
#include "google/protobuf/hpb/hpb.h"

namespace {
using hpb_unittest::protos::Child;

TEST(BzlCode, CheckBzlAlias) {
  hpb::Arena arena;
  auto child = hpb::CreateMessage<Child>(arena);
  child.set_peeps(12);
  ASSERT_EQ(child.peeps(), 12);
}
}  // namespace
