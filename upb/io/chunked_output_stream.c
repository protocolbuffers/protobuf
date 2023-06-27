/*
 * Copyright (c) 2009-2022, Google LLC
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

#include "upb/io/chunked_output_stream.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  upb_ZeroCopyOutputStream base;

  char* data;
  size_t size;
  size_t limit;
  size_t position;
  size_t last_returned_size;
} upb_ChunkedOutputStream;

static void* upb_ChunkedOutputStream_Next(upb_ZeroCopyOutputStream* z,
                                          size_t* count, upb_Status* status) {
  upb_ChunkedOutputStream* c = (upb_ChunkedOutputStream*)z;
  UPB_ASSERT(c->position <= c->size);

  char* out = c->data + c->position;

  const size_t chunk = UPB_MIN(c->limit, c->size - c->position);
  c->position += chunk;
  c->last_returned_size = chunk;
  *count = chunk;

  return chunk ? out : NULL;
}

static void upb_ChunkedOutputStream_BackUp(upb_ZeroCopyOutputStream* z,
                                           size_t count) {
  upb_ChunkedOutputStream* c = (upb_ChunkedOutputStream*)z;

  UPB_ASSERT(c->last_returned_size >= count);
  c->position -= count;
  c->last_returned_size -= count;
}

static size_t upb_ChunkedOutputStream_ByteCount(
    const upb_ZeroCopyOutputStream* z) {
  const upb_ChunkedOutputStream* c = (const upb_ChunkedOutputStream*)z;

  return c->position;
}

static const _upb_ZeroCopyOutputStream_VTable upb_ChunkedOutputStream_vtable = {
    upb_ChunkedOutputStream_Next,
    upb_ChunkedOutputStream_BackUp,
    upb_ChunkedOutputStream_ByteCount,
};

upb_ZeroCopyOutputStream* upb_ChunkedOutputStream_New(void* data, size_t size,
                                                      size_t limit,
                                                      upb_Arena* arena) {
  upb_ChunkedOutputStream* c = upb_Arena_Malloc(arena, sizeof(*c));
  if (!c || !limit) return NULL;

  c->base.vtable = &upb_ChunkedOutputStream_vtable;
  c->data = data;
  c->size = size;
  c->limit = limit;
  c->position = 0;
  c->last_returned_size = 0;

  return (upb_ZeroCopyOutputStream*)c;
}
