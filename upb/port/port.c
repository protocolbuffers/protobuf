// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stdio.h>
#include <stdlib.h>

// Must be last.
#include "upb/port/def.inc"

UPB_NORETURN void _upb_UnreachableFailure(const char* file, int line,
                                          const char* function_name) {
  fprintf(stderr, "%s:%d: Reached unreachable statement in function `%s`.\n",
          file, line, function_name ? function_name : "(unknown)");
  abort();
}
