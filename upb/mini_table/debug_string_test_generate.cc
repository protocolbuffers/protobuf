// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdio>

#include "upb/mini_table/debug_string.h"
#include "upb/mini_table/debug_string_test.upb_minitable.h"

int main(int argc, char** argv) {
  char buf[65536];
  upb_MiniTable_DebugString(&upb_0test__DebugStringTestMessage_msg_init, buf,
                            sizeof(buf));
  puts(buf);
  return 0;
}
