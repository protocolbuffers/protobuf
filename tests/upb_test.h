
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
