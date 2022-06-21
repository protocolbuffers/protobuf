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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UPB_INTERNAL_UPB_H_
#define UPB_INTERNAL_UPB_H_

#include "upb/upb.h"

struct mem_block;
typedef struct mem_block mem_block;

struct upb_Arena {
  _upb_ArenaHead head;
  /* Stores cleanup metadata for this arena.
   * - a pointer to the current cleanup counter.
   * - a boolean indicating if there is an unowned initial block.  */
  uintptr_t cleanup_metadata;

  /* Allocator to allocate arena blocks.  We are responsible for freeing these
   * when we are destroyed. */
  upb_alloc* block_alloc;
  uint32_t last_size;

  /* When multiple arenas are fused together, each arena points to a parent
   * arena (root points to itself). The root tracks how many live arenas
   * reference it. */
  uint32_t refcount; /* Only used when a->parent == a */
  struct upb_Arena* parent;

  /* Linked list of blocks to free/cleanup. */
  mem_block *freelist, *freelist_tail;
};

// Encodes a float or double that is round-trippable, but as short as possible.
// These routines are not fully optimal (not guaranteed to be shortest), but are
// short-ish and match the implementation that has been used in protobuf since
// the beginning.
//
// The given buffer size must be at least kUpb_RoundTripBufferSize.
enum { kUpb_RoundTripBufferSize = 32 };
void _upb_EncodeRoundTripDouble(double val, char* buf, size_t size);
void _upb_EncodeRoundTripFloat(float val, char* buf, size_t size);

#endif /* UPB_INTERNAL_UPB_H_ */
