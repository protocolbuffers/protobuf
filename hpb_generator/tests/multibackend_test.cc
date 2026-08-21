// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
#include "hpb_generator/tests/test_model.hpb.h"
#include "hpb/arena.h"
#include "hpb/hpb.h"

namespace {
using ::hpb_unittest::protos::TestModel;

TEST(MultiBackend, CreateMessage) {
  hpb::Arena arena;
  auto msg = hpb::CreateMessage<TestModel>(arena);
  (void)msg;
}
}  // namespace
