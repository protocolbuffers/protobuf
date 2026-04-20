// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "testing/fuzzing/fuzztest.h"
#include "absl/strings/string_view.h"
#include "upb/test/fuzz_util.h"
#include "upb/wire/fuzz_impl.h"

// Test with:
//   bazel run --config=fuzztest //third_party/upb/upb/wire:fuzz_test --
//   --fuzztest_fuzz=
FUZZ_TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayload);
