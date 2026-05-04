// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
#include "upb/json/fuzz_impl.h"

namespace {

using ::upb_test::DecodeEncodeArbitraryJson;

TEST(FuzzTest, UnclosedObjectKey) { DecodeEncodeArbitraryJson("{\" "); }

TEST(FuzzTest, MalformedExponent) {
  DecodeEncodeArbitraryJson(R"({"val":0XE$})");
}

}  // namespace
