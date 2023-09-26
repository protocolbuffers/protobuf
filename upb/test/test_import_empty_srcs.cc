// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
#include "upb/test/test_import_empty_srcs.upb.h"

TEST(Test, Reexport) {
  // This test really just ensures that compilation succeeds.
  ASSERT_GT(sizeof(upb_test_ContainsImported_msg_init), 0);
}
