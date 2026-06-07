#ifndef PERLUPB_CORO_UTIL_H
#define PERLUPB_CORO_UTIL_H

#include "libcoro/coro.h"

// Boilerplate declarations for all coroutine tests
#define DECLARE_CORO_STATE()                                               \
  coro_context main_ctx;                                                   \
  coro_context* coro_ctxs[NUM_COROS];                                      \
  struct coro_stack coro_stacks[NUM_COROS];                                \
  int coro_active[NUM_COROS];                                              \
  void coro_yield(int id) { coro_transfer(coro_ctxs[id - 1], &main_ctx); } \
  void coro_finish(int id) {                                               \
    coro_active[id - 1] = 0;                                               \
    coro_transfer(coro_ctxs[id - 1], &main_ctx);                           \
  }

// Boilerplate execution and verification logic
#define RUN_CORO_TEST(coro_test_func, args)                                    \
  do {                                                                         \
    coro_create(&main_ctx, NULL, NULL, NULL, 0);                               \
    for (int i = 0; i < NUM_COROS; i++) {                                      \
      if (!coro_stack_alloc(&coro_stacks[i], STACK_SIZE)) {                    \
        perror("coro_stack_alloc");                                            \
        exit(1);                                                               \
      }                                                                        \
      args[i].id = i + 1;                                                      \
      args[i].errors = 0;                                                      \
      coro_active[i] = 1;                                                      \
      coro_ctxs[i] = (coro_context*)malloc(sizeof(coro_context));              \
      coro_create(coro_ctxs[i], coro_test_func, &args[i], coro_stacks[i].sptr, \
                  coro_stacks[i].ssze);                                        \
    }                                                                          \
    cdiag("Running C-level coro stress test...");                              \
    int active_count = NUM_COROS;                                              \
    int j = 0;                                                                 \
    while (active_count > 0) {                                                 \
      int idx = j % NUM_COROS;                                                 \
      if (coro_active[idx]) {                                                  \
        coro_transfer(&main_ctx, coro_ctxs[idx]);                              \
        if (!coro_active[idx]) active_count--;                                 \
      }                                                                        \
      j++;                                                                     \
    }                                                                          \
    cdiag("Finished C-level coro stress test.");                               \
    int total_errors = 0;                                                      \
    for (int i = 0; i < NUM_COROS; i++) {                                      \
      total_errors += args[i].errors;                                          \
      char test_name[100];                                                     \
      snprintf(test_name, sizeof(test_name),                                   \
               "Coro %d completed without errors", i + 1);                     \
      ok(args[i].errors == 0, test_name);                                      \
      coro_stack_free(&coro_stacks[i]);                                        \
      free(coro_ctxs[i]);                                                      \
    }                                                                          \
    is(total_errors, 0, "Total errors from all coroutines");                   \
  } while (0)

#endif  // PERLUPB_CORO_UTIL_H
