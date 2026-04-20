// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include "testing/fuzzing/fuzztest.h"
#include "upb/wire/eps_copy_input_stream_fuzz_impl.h"

auto ArbitraryEpsCopyTestScript() {
  using ::fuzztest::Arbitrary;
  using ::fuzztest::InRange;
  using ::fuzztest::NonNegative;
  using ::fuzztest::StructOf;
  using ::fuzztest::VariantOf;
  using ::fuzztest::VectorOf;

  int max_data_size = 512;

  return StructOf<EpsCopyTestScript>(
      InRange(0, max_data_size),  // data_size
      VectorOf(VariantOf(
          // ReadOp
          StructOf<ReadOp>(InRange(0, kUpb_ReadOp_MaxBytes)),
          // ReadStringOp
          StructOf<ReadStringOp>(NonNegative<int>()),
          // ReadStringAliasOp
          StructOf<ReadStringAliasOp>(NonNegative<int>()),
          // PushLimitOp
          StructOf<PushLimitOp>(NonNegative<int>()))));
}

// Test with:
//   bazel run --config=fuzztest
//   //third_party/upb/upb/wire:eps_copy_input_stream_fuzz_test --
//   --fuzztest_fuzz=
FUZZ_TEST(EpsCopyFuzzTest, TestAgainstFakeStream)
    .WithDomains(ArbitraryEpsCopyTestScript());
