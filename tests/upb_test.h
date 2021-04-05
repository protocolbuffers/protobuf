/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UPB_TEST_H_
#define UPB_TEST_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int num_assertions = 0;
uint32_t testhash = 0;

#define PRINT_FAILURE(expr) \
  fprintf(stderr, "Assertion failed: %s:%d\n", __FILE__, __LINE__); \
  fprintf(stderr, "expr: %s\n", #expr); \
  if (testhash) { \
    fprintf(stderr, "assertion failed running test %x.  " \
                    "Run with the arg %x to run only this test.\n", \
                    testhash, testhash); \
  }

#define ASSERT(expr) do { \
  ++num_assertions; \
  if (!(expr)) { \
    PRINT_FAILURE(expr) \
    abort(); \
  } \
} while (0)

#define ASSERT_NOCOUNT(expr) do { \
  if (!(expr)) { \
    PRINT_FAILURE(expr) \
    abort(); \
  } \
} while (0)

#define ASSERT_STATUS(expr, status) do { \
  ++num_assertions; \
  if (!(expr)) { \
    PRINT_FAILURE(expr) \
    fprintf(stderr, "failed status: %s\n", upb_status_errmsg(status)); \
    abort(); \
  } \
} while (0)

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DECODER_H_ */
