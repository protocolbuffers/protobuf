/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 */

#ifndef UPB_TEST_H_
#define UPB_TEST_H_

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int num_assertions = 0;
#define ASSERT(expr) do { \
  ++num_assertions; \
  if (!(expr)) { \
    fprintf(stderr, "Assertion failed: %s:%d\n", __FILE__, __LINE__); \
    fprintf(stderr, "expr: %s\n", #expr); \
    abort(); \
  } \
} while (0)

#define ASSERT_NOCOUNT(expr) do { \
  if (!(expr)) { \
    fprintf(stderr, "Assertion failed: %s:%d\n", __FILE__, __LINE__); \
    fprintf(stderr, "expr: %s\n", #expr); \
    abort(); \
  } \
} while (0)

#define ASSERT_STATUS(expr, status) do { \
  ++num_assertions; \
  if (!(expr)) { \
    fprintf(stderr, "Assertion failed: %s:%d\n", __FILE__, __LINE__); \
    fprintf(stderr, "expr: %s\n", #expr); \
    fprintf(stderr, "failed status: %s\n", upb_status_getstr(status)); \
    abort(); \
  } \
} while (0)

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DECODER_H_ */
