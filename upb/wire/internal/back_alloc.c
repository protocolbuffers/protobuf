// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/internal/back_alloc.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/internal/log2.h"
#include "upb/mem/internal/arena.h"

// Must be last.
#include "upb/port/def.inc"

char upb_BackAlloc_sentinel;

static size_t upb_BackAlloc_CalcBlockSize(upb_BackAlloc* a, size_t required,
                                          bool* one_off) {
#if UPB_HWASAN
  required = UPB_ALIGN_UP(required, UPB_MALLOC_ALIGN);
#endif
  size_t organic_block_size =
      UPB_PRIVATE(_upb_Arena_NextBlockSize)(a->arena, required, one_off);

  // We want to offer amortized linear time, which means we must grow block
  // sizes exponentially. However, merely allocating a power of 2 is
  // pathological for allocators that use a header with mmap for large
  // contiguous allocations. Instead, we want to allocate based on a power of
  // 2, but request slightly less to leave room for backing allocator
  // metadata. If we had universal size feedback this would not be necessary.
  size_t scaled_block_size = upb_RoundUpToPowerOfTwo(required);

  // Estimated value such that 128 bytes of possible overhead is not
  // significant. 128 bytes should be enough for whatever metadata is needed.
  if (scaled_block_size >= 4096 * 4) {
    scaled_block_size = upb_RoundUpToPowerOfTwo(required + 128) - 128;
  }

  // Scaled block size calculations could overflow, but that's OK as it's
  // unsigned and won't be used if it's less than the organic block size
  if (scaled_block_size > organic_block_size) {
    return UPB_PRIVATE(_upb_Arena_NextBlockSize)(a->arena, scaled_block_size,
                                                 one_off);
  }

  return organic_block_size;
}

static char* upb_BackAlloc_Realloc(upb_BackAlloc* a, char* ptr, size_t n) {
  size_t copy = a->limit - ptr;
  if (SIZE_MAX - copy < n) {
    return NULL;
  }

  bool one_off = false;
  size_t required_block_size = copy + n;
  size_t size = upb_BackAlloc_CalcBlockSize(a, required_block_size, &one_off);

  char* block = UPB_PRIVATE(_upb_Arena_AllocBlock)(a->arena, &size);

  if (!block) {
    return NULL;
  }

  UPB_PRIVATE(_upb_Arena_UpdateGrowthState)(a->arena, required_block_size, size,
                                            one_off);

  char* dst = block + size - copy;
  memcpy(dst, ptr, copy);

  if (a->limit != a->buf) {
    // Dispose of the old block.
    if (a->standalone) {
      // Note: while it would technically be possible to give this block to the
      // arena to use for allocations, this could lead to a lot of garbage
      // blocks that never get used.
      UPB_PRIVATE(_upb_Arena_FreeBlock)(a->arena, a->buf);
    } else {
      UPB_PRIVATE(_upb_Arena_UseBlock)(a->arena, a->buf, a->limit - a->buf);
    }
  }

  a->buf = block;
  a->limit = block + size;
  a->standalone = true;
  return dst - n;
}

char* upb_BackAlloc_Grow(upb_BackAlloc* a, char* ptr, size_t n) {
  if (a->limit == a->buf) {
    // First allocation: try to steal a block.
    size_t size = n;
    char* block = UPB_PRIVATE(_upb_Arena_Steal)(a->arena, &size);
    if (block) {
      UPB_ASSERT(size >= n);
      UPB_ASSERT(a->standalone == false);
      a->buf = block;
      a->limit = block + size;
      return a->limit - n;
    }
  }

  return upb_BackAlloc_Realloc(a, ptr, n);
}

#include "upb/port/undef.inc"
