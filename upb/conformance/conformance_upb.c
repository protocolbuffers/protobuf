// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/* The upb conformance testee binary: speaks the length-prefixed stdin/stdout
 * conformance protocol around the harness in conformance_upb_harness.h, see:
 *   https://github.com/google/protobuf/tree/master/conformance
 * Built twice: as conformance_upb, and with -DREBUILD_MINITABLES as
 * conformance_upb_dynamic_minitable.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "upb/base/string_view.h"
#include "upb/conformance/conformance_upb_harness.h"
#include "upb/mem/arena.h"

int test_count = 0;

bool CheckedRead(int fd, void* buf, size_t len) {
  size_t ofs = 0;
  while (len > 0) {
    ssize_t bytes_read = read(fd, (char*)buf + ofs, len);

    if (bytes_read == 0) return false;

    if (bytes_read < 0) {
      perror("reading from test runner");
      exit(1);
    }

    len -= bytes_read;
    ofs += bytes_read;
  }

  return true;
}

void CheckedWrite(int fd, const void* buf, size_t len) {
  if ((size_t)write(fd, buf, len) != len) {
    perror("writing to test runner");
    exit(1);
  }
}

bool DoTestIo(const upb_ConformanceHarness* harness) {
  char* input;
  uint32_t input_size;
  upb_StringView output;
  upb_Arena* arena;

  if (!CheckedRead(STDIN_FILENO, &input_size, sizeof(uint32_t))) {
    /* EOF. */
    return false;
  }

  arena = upb_Arena_New();
  input = upb_Arena_Malloc(arena, input_size);

  if (!CheckedRead(STDIN_FILENO, input, input_size)) {
    fprintf(stderr, "conformance_upb: unexpected EOF on stdin.\n");
    exit(1);
  }

  output = upb_ConformanceHarness_Run(
      harness, upb_StringView_FromDataAndSize(input, input_size), arena);

  uint32_t network_out = (uint32_t)output.size;
  CheckedWrite(STDOUT_FILENO, &network_out, sizeof(uint32_t));
  CheckedWrite(STDOUT_FILENO, output.data, output.size);

  test_count++;

  upb_Arena_Free(arena);

  return true;
}

int main(void) {
#ifdef REBUILD_MINITABLES
  const bool rebuild_minitables = true;
#else
  const bool rebuild_minitables = false;
#endif
  upb_ConformanceHarness* harness =
      upb_ConformanceHarness_New(rebuild_minitables);

  while (1) {
    if (!DoTestIo(harness)) {
      fprintf(stderr,
              "conformance_upb: received EOF from test runner "
              "after %d tests, exiting\n",
              test_count);
      upb_ConformanceHarness_Free(harness);
      return 0;
    }
  }
}
