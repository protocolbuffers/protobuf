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

static size_t upb_TryGrowToUsable(void** ptr, size_t size) {
  size_t usable_size = size;
#ifdef UPB_MALLOC_USABLE_SIZE
  usable_size = UPB_MALLOC_USABLE_SIZE(*ptr);
  size_t aligned_usable_size = UPB_ALIGN_DOWN(usable_size, UPB_MALLOC_ALIGN);

  // Don't bother trying to realloc if it's only for an insignificant amount
  if (aligned_usable_size <= size || aligned_usable_size - size <= 64)
    return size;

  // It's called usable size, but we can't actually use it without a realloc -
  // glibc forbids it, presumably the allocator could subdivide it or we could
  // trip sanitizers. This call is not guaranteed to succeed, some other thread
  // could in theory have grabbed a subsection of our block; but if it fails
  // the original allocation is preserved.
  void* resized = realloc(*ptr, usable_size);
  if (!resized) return size;

  *ptr = resized;
#endif
  return usable_size;
}

static size_t upb_RoundUpToBlockSize(size_t size) {
#ifdef UPB_MALLOC_GOOD_SIZE
  size = UPB_MALLOC_GOOD_SIZE(size);
#endif
  return size;
}

static void* upb_global_allocfunc(upb_alloc* alloc, void* ptr, size_t oldsize,
                                  size_t size, size_t* actual_size) {
  UPB_UNUSED(alloc);
  UPB_UNUSED(oldsize);
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  size_t allocated_size = size;
  if (actual_size) {
    allocated_size = upb_RoundUpToBlockSize(size);
  }

  void* ret = realloc(ptr, allocated_size);
  if (ret && actual_size) {
    size_t grown = upb_TryGrowToUsable(&ret, allocated_size);
    if (size != grown) {
      *actual_size = grown;
    }
  }
  return ret;
}

upb_alloc upb_alloc_global = {&upb_global_allocfunc};
