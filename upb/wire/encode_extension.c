// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/encode_extension.h"

#include <setjmp.h>
#include <stddef.h>

#include "upb/base/string_view.h"
#include "upb/mem/internal/arena.h"
#include "upb/message/internal/extension.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/message.h"
#include "upb/wire/encode.h"
#include "upb/wire/internal/back_alloc.h"
#include "upb/wire/internal/encoder.h"

// Must be last.
#include "upb/port/def.inc"

static upb_EncodeStatus upb_DoEncodeExtension(upb_encstate* encoder, char* ptr,
                                              const struct upb_Extension* ext,
                                              bool is_message_set,
                                              upb_StringView* view,
                                              int encode_options) {
  if (UPB_SETJMP(*encoder->err) == 0) {
    char* buf = ptr;
    size_t size = 0;
    UPB_PRIVATE(_upb_Encode_Extension)(encoder, ext->ext, ext->data,
                                       is_message_set, &buf, &size,
                                       encode_options);
    view->data = buf;
    view->size = size;
  } else {
    UPB_ASSERT(encoder->status != kUpb_EncodeStatus_Ok);
    upb_BackAlloc_Abort(&encoder->alloc);
    view->data = NULL;
    view->size = 0;
  }
  UPB_PRIVATE(_upb_encstate_destroy)(encoder);
  return encoder->status;
}

upb_EncodeStatus upb_EncodeExtension(const struct upb_Extension* ext,
                                     struct upb_Arena* arena,
                                     upb_StringView* view, int encode_options) {
  const upb_MiniTable* extendee = upb_MiniTableExtension_Extendee(ext->ext);
  bool is_message_set =
      extendee != NULL && upb_MiniTable_IsMessageSet(extendee);
  upb_encstate e;
  jmp_buf err;
  char* ptr = UPB_PRIVATE(_upb_encstate_init)(&e, &err, arena);
  return upb_DoEncodeExtension(&e, ptr, ext, is_message_set, view,
                               encode_options);
}
