/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2013 Google Inc.  See LICENSE for details.
 *
 * Test of upb_pipeline.
 */

#include "upb/sink.h"
#include "tests/upb_test.h"

static void *count_realloc(void *ud, void *ptr, size_t size) {
  int *count = ud;
  *count += 1;
  return upb_realloc(ud, ptr, size);
}

static void test_empty() {
  // A pipeline with no initial memory or allocation function should return
  // NULL from attempts to allocate.
  upb_pipeline pipeline;
  upb_pipeline_init(&pipeline, NULL, 0, NULL, NULL);
  ASSERT(upb_pipeline_alloc(&pipeline, 1) == NULL);
  ASSERT(upb_pipeline_alloc(&pipeline, 1) == NULL);
  ASSERT(upb_pipeline_realloc(&pipeline, NULL, 0, 1) == NULL);
  upb_pipeline_uninit(&pipeline);
}

static void test_only_initial() {
  upb_pipeline pipeline;
  char initial[152];  // 128 + a conservative 24 bytes overhead.
  upb_pipeline_init(&pipeline, initial, sizeof(initial), NULL, NULL);
  void *p1 = upb_pipeline_alloc(&pipeline, 64);
  void *p2 = upb_pipeline_alloc(&pipeline, 64);
  void *p3 = upb_pipeline_alloc(&pipeline, 64);
  ASSERT(p1);
  ASSERT(p2);
  ASSERT(!p3);
  ASSERT(p1 != p2);
  ASSERT((void*)initial <= p1);
  ASSERT(p1 < p2);
  ASSERT(p2 < (void*)(initial + sizeof(initial)));
  upb_pipeline_uninit(&pipeline);
}

static void test_with_alloc_func() {
  upb_pipeline pipeline;
  char initial[152];  // 128 + a conservative 24 bytes overhead.
  int count = 0;
  upb_pipeline_init(&pipeline, initial, sizeof(initial), count_realloc, &count);
  void *p1 = upb_pipeline_alloc(&pipeline, 64);
  void *p2 = upb_pipeline_alloc(&pipeline, 64);
  ASSERT(p1);
  ASSERT(p2);
  ASSERT(p1 != p2);
  ASSERT(count == 0);

  void *p3 = upb_pipeline_alloc(&pipeline, 64);
  ASSERT(p3);
  ASSERT(p3 != p2);
  ASSERT(count == 1);

  // Allocation larger than internal block size should force another alloc.
  char *p4 = upb_pipeline_alloc(&pipeline, 16384);
  ASSERT(p4);
  p4[16383] = 1;  // Verify memory is writable without crashing.
  ASSERT(p4[16383] == 1);
  ASSERT(count == 2);

  upb_pipeline_uninit(&pipeline);
  ASSERT(count == 4);  // From two calls to free the memory.
}

static void test_realloc() {
  upb_pipeline pipeline;
  char initial[152];  // 128 + a conservative 24 bytes overhead.
  int count = 0;
  upb_pipeline_init(&pipeline, initial, sizeof(initial), count_realloc, &count);
  void *p1 = upb_pipeline_alloc(&pipeline, 64);
  // This realloc should work in-place.
  void *p2 = upb_pipeline_realloc(&pipeline, p1, 64, 128);
  ASSERT(p1);
  ASSERT(p2);
  ASSERT(p1 == p2);
  ASSERT(count == 0);

  // This realloc will *not* work in place, due to size.
  void *p3 = upb_pipeline_realloc(&pipeline, p2, 128, 256);
  ASSERT(p3);
  ASSERT(p3 != p2);
  ASSERT(count == 1);

  void *p4 = upb_pipeline_alloc(&pipeline, 64);
  void *p5 = upb_pipeline_alloc(&pipeline, 64);
  // This realloc will *not* work in place because it was not the last
  // allocation.
  void *p6 = upb_pipeline_realloc(&pipeline, p4, 64, 128);
  ASSERT(p4);
  ASSERT(p5);
  ASSERT(p6);
  ASSERT(p4 != p6);
  ASSERT(p4 < p5);
  ASSERT(p5 < p6);
  ASSERT(count == 1);  // These should all fit in the first dynamic block.

  upb_pipeline_uninit(&pipeline);
  ASSERT(count == 2);
}

int run_tests(int argc, char *argv[]) {
  UPB_UNUSED(argc);
  UPB_UNUSED(argv);
  test_empty();
  test_only_initial();
  test_with_alloc_func();
  test_realloc();
  return 0;
}
