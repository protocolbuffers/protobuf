#ifndef PERL_PROTOBUF_T_C_TEST_UTIL_H_
#define PERL_PROTOBUF_T_C_TEST_UTIL_H_

#include <stdio.h>
#include <string.h>

#include "EXTERN.h"
#include "perl.h"  // NOLINT(build/include)

// Forward declarations for test runners from other files
extern void run_utils_tests(pTHX);

// Simplified test functions for C tests
#define is_s(got, expected, name) \
  printf("%s: %s\n", strcmp(got, expected) == 0 ? "ok" : "not ok", name);

#define is_null(ptr, name) \
  printf("%s: %s\n", ptr == NULL ? "ok" : "not ok", name);

#define done_testing() \
  printf("1..0\n");  // Placeholder, Test::More in Perl handles the count

#endif  // PERL_PROTOBUF_T_C_TEST_UTIL_H_
