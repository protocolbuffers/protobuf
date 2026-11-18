// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/internal/def_pool_init.h"

#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/file.h"
#include "upb/mini_table/message.h"
#include "upb/reflection/internal/def_pool.h"

// Must be last.
#include "upb/port/def.inc"

upb_MiniTableFile* upb_MiniTableFile_New(
    upb_Arena* arena, const upb_MiniTable** msgs, int msg_count,
    const upb_MiniTableEnum** enums, int enum_count,
    const upb_MiniTableExtension** exts, int ext_count) {
  upb_MiniTableFile* layout = upb_Arena_Malloc(arena, sizeof(*layout));
  if (!layout) return NULL;

  layout->UPB_PRIVATE(msgs) = msgs;
  layout->UPB_PRIVATE(enums) = enums;
  layout->UPB_PRIVATE(exts) = exts;
  layout->UPB_PRIVATE(msg_count) = msg_count;
  layout->UPB_PRIVATE(enum_count) = enum_count;
  layout->UPB_PRIVATE(ext_count) = ext_count;
  return layout;
}

_upb_DefPool_Init* upb_DefPool_Init_New(upb_Arena* arena, const char* filename,
                                        upb_StringView descriptor,
                                        _upb_DefPool_Init** deps,
                                        const upb_MiniTableFile* layout) {
  if (!deps) {
    deps = upb_Arena_Malloc(arena, sizeof(*deps));
    if (!deps) return NULL;
    deps[0] = NULL;
  }

  _upb_DefPool_Init* init = upb_Arena_Malloc(arena, sizeof(*init));
  if (!init) return NULL;

  init->deps = deps;
  init->layout = layout;
  init->filename = filename;
  init->descriptor = descriptor;
  return init;
}
