// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_REFLECTION_INTERNAL_DEF_POOL_INIT_H_
#define UPB_REFLECTION_INTERNAL_DEF_POOL_INIT_H_

#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/file.h"
#include "upb/mini_table/message.h"
#include "upb/reflection/internal/def_pool.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// The APIs in this file are internal to protobuf.

// Builds a upb_MiniTableFile on `arena`.
//
// The count and order of each array must be correct.
upb_MiniTableFile* upb_MiniTableFile_New(
    upb_Arena* arena, const upb_MiniTable** msgs, int msg_count,
    const upb_MiniTableEnum** enums, int enum_count,
    const upb_MiniTableExtension** exts, int ext_count);

// Builds a _upb_DefPool_Init on `arena`, ready to hand to
// _upb_DefPool_LoadDefInit.
_upb_DefPool_Init* upb_DefPool_Init_New(upb_Arena* arena, const char* filename,
                                        upb_StringView descriptor,
                                        _upb_DefPool_Init** deps,
                                        const upb_MiniTableFile* layout);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_REFLECTION_INTERNAL_DEF_POOL_INIT_H_ */
