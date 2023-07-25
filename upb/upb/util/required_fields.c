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

#include "upb/util/required_fields.h"

#include <inttypes.h>
#include <stdarg.h>

#include "upb/collections/map.h"
#include "upb/port/vsnprintf_compat.h"
#include "upb/reflection/message.h"

// Must be last.
#include "upb/port/def.inc"

////////////////////////////////////////////////////////////////////////////////
// upb_FieldPath_ToText()
////////////////////////////////////////////////////////////////////////////////

typedef struct {
  char* buf;
  char* ptr;
  char* end;
  size_t overflow;
} upb_PrintfAppender;

UPB_PRINTF(2, 3)
static void upb_FieldPath_Printf(upb_PrintfAppender* a, const char* fmt, ...) {
  size_t n;
  size_t have = a->end - a->ptr;
  va_list args;

  va_start(args, fmt);
  n = _upb_vsnprintf(a->ptr, have, fmt, args);
  va_end(args);

  if (UPB_LIKELY(have > n)) {
    // We can't end up here if the user passed (NULL, 0), therefore ptr is known
    // to be non-NULL, and UPB_PTRADD() is not necessary.
    assert(a->ptr);
    a->ptr += n;
  } else {
    a->ptr = UPB_PTRADD(a->ptr, have);
    a->overflow += (n - have);
  }
}

static size_t upb_FieldPath_NullTerminate(upb_PrintfAppender* d, size_t size) {
  size_t ret = d->ptr - d->buf + d->overflow;

  if (size > 0) {
    if (d->ptr == d->end) d->ptr--;
    *d->ptr = '\0';
  }

  return ret;
}

static void upb_FieldPath_PutMapKey(upb_PrintfAppender* a,
                                    upb_MessageValue map_key,
                                    const upb_FieldDef* key_f) {
  switch (upb_FieldDef_CType(key_f)) {
    case kUpb_CType_Int32:
      upb_FieldPath_Printf(a, "[%" PRId32 "]", map_key.int32_val);
      break;
    case kUpb_CType_Int64:
      upb_FieldPath_Printf(a, "[%" PRId64 "]", map_key.int64_val);
      break;
    case kUpb_CType_UInt32:
      upb_FieldPath_Printf(a, "[%" PRIu32 "]", map_key.uint32_val);
      break;
    case kUpb_CType_UInt64:
      upb_FieldPath_Printf(a, "[%" PRIu64 "]", map_key.uint64_val);
      break;
    case kUpb_CType_Bool:
      upb_FieldPath_Printf(a, "[%s]", map_key.bool_val ? "true" : "false");
      break;
    case kUpb_CType_String:
      upb_FieldPath_Printf(a, "[\"");
      for (size_t i = 0; i < map_key.str_val.size; i++) {
        char ch = map_key.str_val.data[i];
        if (ch == '"') {
          upb_FieldPath_Printf(a, "\\\"");
        } else {
          upb_FieldPath_Printf(a, "%c", ch);
        }
      }
      upb_FieldPath_Printf(a, "\"]");
      break;
    default:
      UPB_UNREACHABLE();  // Other types can't be map keys.
  }
}

size_t upb_FieldPath_ToText(upb_FieldPathEntry** path, char* buf, size_t size) {
  upb_FieldPathEntry* ptr = *path;
  upb_PrintfAppender appender;
  appender.buf = buf;
  appender.ptr = buf;
  appender.end = UPB_PTRADD(buf, size);
  appender.overflow = 0;
  bool first = true;

  while (ptr->field) {
    const upb_FieldDef* f = ptr->field;

    upb_FieldPath_Printf(&appender, first ? "%s" : ".%s", upb_FieldDef_Name(f));
    first = false;
    ptr++;

    if (upb_FieldDef_IsMap(f)) {
      const upb_FieldDef* key_f =
          upb_MessageDef_Field(upb_FieldDef_MessageSubDef(f), 0);
      upb_FieldPath_PutMapKey(&appender, ptr->map_key, key_f);
      ptr++;
    } else if (upb_FieldDef_IsRepeated(f)) {
      upb_FieldPath_Printf(&appender, "[%zu]", ptr->array_index);
      ptr++;
    }
  }

  // Advance beyond terminating NULL.
  ptr++;
  *path = ptr;
  return upb_FieldPath_NullTerminate(&appender, size);
}

////////////////////////////////////////////////////////////////////////////////
// upb_util_HasUnsetRequired()
////////////////////////////////////////////////////////////////////////////////

typedef struct {
  upb_FieldPathEntry* path;
  size_t size;
  size_t cap;
} upb_FieldPathVector;

typedef struct {
  upb_FieldPathVector stack;
  upb_FieldPathVector out_fields;
  const upb_DefPool* ext_pool;
  jmp_buf err;
  bool has_unset_required;
  bool save_paths;
} upb_FindContext;

static void upb_FieldPathVector_Init(upb_FieldPathVector* vec) {
  vec->path = NULL;
  vec->size = 0;
  vec->cap = 0;
}

static void upb_FieldPathVector_Reserve(upb_FindContext* ctx,
                                        upb_FieldPathVector* vec,
                                        size_t elems) {
  if (vec->cap - vec->size < elems) {
    size_t need = vec->size + elems;
    vec->cap = UPB_MAX(4, vec->cap);
    while (vec->cap < need) vec->cap *= 2;
    vec->path = realloc(vec->path, vec->cap * sizeof(*vec->path));
    if (!vec->path) {
      UPB_LONGJMP(ctx->err, 1);
    }
  }
}

static void upb_FindContext_Push(upb_FindContext* ctx, upb_FieldPathEntry ent) {
  if (!ctx->save_paths) return;
  upb_FieldPathVector_Reserve(ctx, &ctx->stack, 1);
  ctx->stack.path[ctx->stack.size++] = ent;
}

static void upb_FindContext_Pop(upb_FindContext* ctx) {
  if (!ctx->save_paths) return;
  assert(ctx->stack.size != 0);
  ctx->stack.size--;
}

static void upb_util_FindUnsetInMessage(upb_FindContext* ctx,
                                        const upb_Message* msg,
                                        const upb_MessageDef* m) {
  // Iterate over all fields to see if any required fields are missing.
  for (int i = 0, n = upb_MessageDef_FieldCount(m); i < n; i++) {
    const upb_FieldDef* f = upb_MessageDef_Field(m, i);
    if (upb_FieldDef_Label(f) != kUpb_Label_Required) continue;

    if (!msg || !upb_Message_HasFieldByDef(msg, f)) {
      // A required field is missing.
      ctx->has_unset_required = true;

      if (ctx->save_paths) {
        // Append the contents of the stack to the out array, then
        // NULL-terminate.
        upb_FieldPathVector_Reserve(ctx, &ctx->out_fields, ctx->stack.size + 2);
        if (ctx->stack.size) {
          memcpy(&ctx->out_fields.path[ctx->out_fields.size], ctx->stack.path,
                 ctx->stack.size * sizeof(*ctx->stack.path));
        }
        ctx->out_fields.size += ctx->stack.size;
        ctx->out_fields.path[ctx->out_fields.size++] =
            (upb_FieldPathEntry){.field = f};
        ctx->out_fields.path[ctx->out_fields.size++] =
            (upb_FieldPathEntry){.field = NULL};
      }
    }
  }
}

static void upb_util_FindUnsetRequiredInternal(upb_FindContext* ctx,
                                               const upb_Message* msg,
                                               const upb_MessageDef* m) {
  // OPT: add markers in the schema for where we can avoid iterating:
  // 1. messages with no required fields.
  // 2. messages that cannot possibly reach any required fields.

  upb_util_FindUnsetInMessage(ctx, msg, m);
  if (!msg) return;

  // Iterate over all present fields to find sub-messages that might be missing
  // required fields.  This may revisit some of the fields already inspected
  // in the previous loop.  We do this separately because this loop will also
  // find present extensions, which the previous loop will not.
  //
  // TODO(haberman): consider changing upb_Message_Next() to be capable of
  // visiting extensions only, for example with a kUpb_Message_BeginEXT
  // constant.
  size_t iter = kUpb_Message_Begin;
  const upb_FieldDef* f;
  upb_MessageValue val;
  while (upb_Message_Next(msg, m, ctx->ext_pool, &f, &val, &iter)) {
    // Skip non-submessage fields.
    if (!upb_FieldDef_IsSubMessage(f)) continue;

    upb_FindContext_Push(ctx, (upb_FieldPathEntry){.field = f});
    const upb_MessageDef* sub_m = upb_FieldDef_MessageSubDef(f);

    if (upb_FieldDef_IsMap(f)) {
      // Map field.
      const upb_FieldDef* val_f = upb_MessageDef_Field(sub_m, 1);
      const upb_MessageDef* val_m = upb_FieldDef_MessageSubDef(val_f);
      if (!val_m) continue;
      const upb_Map* map = val.map_val;
      size_t iter = kUpb_Map_Begin;
      upb_MessageValue key, map_val;
      while (upb_Map_Next(map, &key, &map_val, &iter)) {
        upb_FindContext_Push(ctx, (upb_FieldPathEntry){.map_key = key});
        upb_util_FindUnsetRequiredInternal(ctx, map_val.msg_val, val_m);
        upb_FindContext_Pop(ctx);
      }
    } else if (upb_FieldDef_IsRepeated(f)) {
      // Repeated field.
      const upb_Array* arr = val.array_val;
      for (size_t i = 0, n = upb_Array_Size(arr); i < n; i++) {
        upb_MessageValue elem = upb_Array_Get(arr, i);
        upb_FindContext_Push(ctx, (upb_FieldPathEntry){.array_index = i});
        upb_util_FindUnsetRequiredInternal(ctx, elem.msg_val, sub_m);
        upb_FindContext_Pop(ctx);
      }
    } else {
      // Scalar sub-message field.
      upb_util_FindUnsetRequiredInternal(ctx, val.msg_val, sub_m);
    }

    upb_FindContext_Pop(ctx);
  }
}

bool upb_util_HasUnsetRequired(const upb_Message* msg, const upb_MessageDef* m,
                               const upb_DefPool* ext_pool,
                               upb_FieldPathEntry** fields) {
  upb_FindContext ctx;
  ctx.has_unset_required = false;
  ctx.save_paths = fields != NULL;
  ctx.ext_pool = ext_pool;
  upb_FieldPathVector_Init(&ctx.stack);
  upb_FieldPathVector_Init(&ctx.out_fields);
  upb_util_FindUnsetRequiredInternal(&ctx, msg, m);
  free(ctx.stack.path);
  if (fields) {
    upb_FieldPathVector_Reserve(&ctx, &ctx.out_fields, 1);
    ctx.out_fields.path[ctx.out_fields.size] =
        (upb_FieldPathEntry){.field = NULL};
    *fields = ctx.out_fields.path;
  }
  return ctx.has_unset_required;
}
