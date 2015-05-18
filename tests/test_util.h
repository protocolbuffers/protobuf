/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2014 Google Inc.  See LICENSE for details.
 *
 * Common functionality for tests.
 */

#ifndef UPB_TEST_UTIL_H_
#define UPB_TEST_UTIL_H_

#include <stdio.h>
#include <math.h>
#include "tests/upb_test.h"
#include "upb/sink.h"

upb::BufferHandle global_handle;

// Puts a region of the given buffer [start, end) into the given sink (which
// probably represents a parser.  Can gracefully handle the case where the
// parser returns a "parsed" length that is less or greater than the input
// buffer length, and tracks the overall parse offset in *ofs.
//
// Pass verbose=true to print detailed diagnostics to stderr.
bool parse_buffer(upb::BytesSink* sink, void* subc, const char* buf,
                  size_t start, size_t end, size_t* ofs,
                  upb::Status* status, bool verbose) {
  start = UPB_MAX(start, *ofs);

  if (start <= end) {
    size_t len = end - start;

    // Copy buffer into a separate, temporary buffer.
    // This is necessary to verify that the parser is not erroneously
    // reading outside the specified bounds.
    char *buf2 = (char*)malloc(len);
    assert(buf2);
    memcpy(buf2, buf + start, len);

    if (verbose) {
      fprintf(stderr, "Calling parse(%u) for bytes %u-%u of the input\n",
              (unsigned)len, (unsigned)start, (unsigned)end);
    }

    size_t parsed = sink->PutBuffer(subc, buf2, len, &global_handle);
    free(buf2);

    if (verbose) {
      if (parsed == len) {
        fprintf(stderr,
                "parse(%u) = %u, complete byte count indicates success\n",
                (unsigned)len, (unsigned)len);
      } else if (parsed > len) {
        fprintf(stderr,
                "parse(%u) = %u, long byte count indicates success and skip"
                "of the next %u bytes\n",
                (unsigned)len, (unsigned)parsed, (unsigned)(parsed - len));
      } else {
        fprintf(stderr,
                "parse(%u) = %u, short byte count indicates failure; "
                "last %u bytes were not consumed\n",
                (unsigned)len, (unsigned)parsed, (unsigned)(len - parsed));
      }
    }

    if (status->ok() != (parsed >= len)) {
      if (status->ok()) {
        fprintf(stderr,
                "Error: decode function returned short byte count but set no "
                "error status\n");
      } else {
        fprintf(stderr,
                "Error: decode function returned complete byte count but set "
                "error status\n");
      }
      fprintf(stderr, "Status: %s, parsed=%u, len=%u\n",
              status->error_message(), (unsigned)parsed, (unsigned)len);
      ASSERT(false);
    }

    if (!status->ok())
      return false;

    *ofs += parsed;
  }
  return true;
}

#endif
