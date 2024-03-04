// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include <gtest/gtest.h>
#include "upb/mini_table/message.h"
#include "upb/test/test.upb_minitable.h"

TEST(Test, SupportsFullName) {
  // This test really just ensures that compilation succeeds.
#if (defined(UPB_TRACING_ENABLED) && !defined(NDEBUG))
  std::string name =
      std::string(upb_MiniTable_FullName(&upb_0test__HelloRequest_msg_init));
  EXPECT_EQ(name, "upb_test.HelloRequest");
#else
  ADD_FAILURE()
      << "Please use -c dbg --copts=-DUPB_TRACING_ENABLED to run this test";
#endif
}
