// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mem/alloc.h"

#include <stdlib.h>

// Must be last.
#include "upb/port/def.inc"

static void* upb_global_allocfunc(upb_alloc* alloc, void* ptr, size_t oldsize,
                                  size_t size, size_t* actual_size) {
  UPB_UNUSED(alloc);
  UPB_UNUSED(oldsize);
  UPB_UNUSED(actual_size);
  if (size == 0) {
    free(ptr);
    return NULL;
  } else {
    return realloc(ptr, size);
  }
}

UPB_NODISCARD bool upb_AllocationCount_IsAvailable(void) {
#ifdef UPB_ARENA_ALLOCATION_COUNT
  return true;
#else
  return false;
#endif
}

UPB_NODISCARD size_t upb_AllocationCount_Get(void) {
#ifdef UPB_ARENA_ALLOCATION_COUNT
  return upb_arena_alloc_count;
#else
  return 0;
#endif
}

void upb_AllocationCount_Reset(void) {
#ifdef UPB_ARENA_ALLOCATION_COUNT
  upb_arena_alloc_count = 0;
  upb_arena_alloc_fail_on = SIZE_MAX;
#endif
}

void upb_AllocationCount_FailOn(size_t n) {
#ifdef UPB_ARENA_ALLOCATION_COUNT
  upb_arena_alloc_fail_on = n;
#endif
}

upb_alloc upb_alloc_global = {&upb_global_allocfunc};
