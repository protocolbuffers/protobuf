// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
