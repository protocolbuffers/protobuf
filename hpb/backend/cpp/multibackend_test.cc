// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
#include "hpb_generator/tests/trivial.hpb.h"
#include "hpb/arena.h"
#include "hpb/hpb.h"
#include "hpb/ptr.h"

// Tests in this file are run against both backends {upb, cpp} to ensure
// api conformance, compatibility, and correctness.
// We eventually expect this file to disappear as we approach full parity.

namespace hpb::testing {
namespace {

using ::hpb_unittest::protos::Trivial;
using ::hpb_unittest::protos::TRIVIAL_PAINTING_SARGENT;

TEST(MultiBackend, CanCreateMessage) {
  hpb::Arena arena;
  hpb::Ptr<Trivial> trivial_ptr = hpb::CreateMessage<Trivial>(arena);
  (void)trivial_ptr;
}

TEST(MultiBackend, Enums) { EXPECT_EQ(2, TRIVIAL_PAINTING_SARGENT); }

TEST(MultiBackend, Booleans) {
  Trivial trivial;
  EXPECT_FALSE(trivial.b());
  // TODO: set, clear
}

}  // namespace
}  // namespace hpb::testing
