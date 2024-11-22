// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stddef.h>

#include <string>

#include <gtest/gtest.h>
#include "absl/log/absl_log.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/message.h"
#include "upb/test/test.upb.h"
#include "upb/test/test.upb_minitable.h"
#include "upb/text/debug_string.h"

TEST(TextNoReflection, Extensions) {
  const upb_MiniTable* mt_main = upb_0test__ModelWithExtensions_msg_init_ptr;
  upb_Arena* arena = upb_Arena_New();

  upb_test_ModelExtension1* extension1 = upb_test_ModelExtension1_new(arena);
  upb_test_ModelExtension1_set_str(extension1,
                                   upb_StringView_FromString("Hello"));

  upb_test_ModelExtension2* extension2 = upb_test_ModelExtension2_new(arena);
  upb_test_ModelExtension2_set_i(extension2, 5);

  upb_test_ModelWithExtensions* msg = upb_test_ModelWithExtensions_new(arena);

  upb_test_ModelExtension1_set_model_ext(msg, extension1, arena);
  upb_test_ModelExtension2_set_model_ext(msg, extension2, arena);

  // Convert to a type of upb_Message*
  upb_Message* input = UPB_UPCAST(msg);
  // Resizing/reallocation of the buffer is not necessary since we're only
  // testing that we get the expected debug string.
  char* buf = new char[100];
  int options =
      UPB_TXTENC_NOSORT;  // Does not matter, but maps will not be sorted.
  size_t size = 100;
  size_t real_size = upb_DebugString(input, mt_main, options, buf, size);
  ABSL_LOG(INFO) << "Buffer: \n"
                 << buf << "\n"
                 << "Size:" << real_size << "\n";
  std::string golden = R"([4135] {
  9: 5
}
[1547] {
  25: "Hello"
}
)";
  ASSERT_EQ(buf[real_size], '\0');
  std::string str(buf);
  ASSERT_EQ(buf, golden);
  delete[] buf;
  upb_Arena_Free(arena);
}