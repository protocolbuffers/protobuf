/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 */

#ifndef UPB_TEST_H_
#define UPB_TEST_H_

#ifdef __cplusplus
extern "C" {
#endif

int num_assertions = 0;
#define ASSERT(expr) do { \
  ++num_assertions; \
  if (!(expr)) { \
    fprintf(stderr, "Assertion failed: %s:%d\n", __FILE__, __LINE__); \
    abort(); \
  } \
} while(0)

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DECODER_H_ */
