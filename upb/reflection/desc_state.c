// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/internal/desc_state.h"

#include "upb/port/overflow.h"

// Must be last.
#include "upb/port/def.inc"

bool _upb_DescState_Grow(upb_DescState* d, upb_Arena* a) {
  const size_t oldbufsize = d->bufsize;

  if (!d->buf) {
    d->buf = upb_Arena_Malloc(a, d->bufsize);
    if (!d->buf) return false;
    d->ptr = d->buf;
    d->e.end = d->buf + d->bufsize;
  }

  const size_t used = d->ptr - d->buf;
  UPB_ASSERT(used <= oldbufsize);

  if (oldbufsize - used < kUpb_MtDataEncoder_MinSize) {
    size_t newbufsize;
    if (upb_MulOverflow(oldbufsize, (size_t)2, &newbufsize)) return false;
    d->bufsize = newbufsize;
    d->buf = upb_Arena_Realloc(a, d->buf, oldbufsize, d->bufsize);
    if (!d->buf) return false;
    d->ptr = d->buf + used;
    d->e.end = d->buf + d->bufsize;
  }

  return true;
}
