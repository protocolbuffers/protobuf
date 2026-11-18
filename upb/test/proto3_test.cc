// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
#include "upb/reflection/def.hpp"
#include "upb/test/proto3_test.upb.h"
#include "upb/test/proto3_test.upbdefs.h"

TEST(Proto3Test, SyntheticOneofExtension) {
  upb::DefPool defpool;
  upb::MessageDefPtr md(upb_test_TestMessage3_getmsgdef(defpool.ptr()));
  ASSERT_EQ(md.field_count(), 6);
}
