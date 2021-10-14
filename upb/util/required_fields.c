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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "upb/util/required_fields.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>

#include "upb/reflection.h"

// Must be last.
#include "upb/port_def.inc"

typedef struct {
  upb_arena *arena;
  char **fields;
  size_t count;
  size_t size;
  const upb_symtab *ext_pool;
  jmp_buf err;
} Find_Ctx;

UPB_PRINTF(2, 3)
static char *upb_arena_printf(upb_arena *arena, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  size_t n = vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  char *p = upb_arena_malloc(arena, n + 1);
  if (!p) return NULL;

  va_start(args, fmt);
  vsnprintf(p, n + 1, fmt, args);
  va_end(args);
  return p;
}

UPB_PRINTF(2, 3)
static char *upb_asprintf(char *old, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  size_t n = vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  size_t old_size = old ? strlen(old) : 0;
  char *p = realloc(old, old_size + n + 1);
  if (!p) return NULL;

  va_start(args, fmt);
  vsnprintf(p + old_size, n + 1, fmt, args);
  va_end(args);
  return p;
}

// Allocate on the heap instead of the arena, since the accumulated
// allocations can grow quite large (O(all fields), not just set).
bool append_str(Find_Ctx *ctx, const char *prefix, const upb_fielddef *f) {
  if (ctx->count == ctx->size) {
    size_t new_size = UPB_MAX(4, ctx->size * 2);
    char **new_fields = upb_arena_realloc(ctx->arena, ctx->fields,
                                          ctx->size * sizeof(*ctx->fields),
                                          new_size * sizeof(*ctx->fields));
    if (!new_fields) return false;
    ctx->fields = new_fields;
    ctx->size = new_size;
  }
  char *s = upb_arena_printf(ctx->arena, "%s.%s", prefix, upb_fielddef_name(f));
  if (!s) return false;
  ctx->fields[ctx->count++] = s;
  return true;
}

// Really we should use a map key for maps, but proto2 doesn't so we don't.
char *alloc_prefix(const char *prefix, const upb_fielddef *f, int index) {
  char *ret = NULL;
  if (upb_fielddef_isextension(f)) {
    ret = upb_asprintf(ret, "(%s)", upb_fielddef_fullname(f));
  } else {
    ret = upb_asprintf(ret, "%s", upb_fielddef_name(f));
  }

  if (index >= 0) {
    ret = upb_asprintf(ret, "[%d]", index);
  }

  ret = upb_asprintf(ret, ".");
  return ret;
}

bool find_in_msg(Find_Ctx *ctx, const char *prefix, const upb_msg *msg,
                 const upb_msgdef *m) {
  // OPT: add markers in the schema for where we can avoid iterating:
  // 1. messages with no required fields.
  // 2. messages that cannot possibly reach any required fields.
  for (int i = 0, n = upb_msgdef_fieldcount(m); i < n; i++) {
    const upb_fielddef *f = upb_msgdef_field(m, i);
    if (upb_fielddef_label(f) != UPB_LABEL_REQUIRED) continue;
    if (!upb_msg_has(msg, f)) {
      if (!append_str(ctx, prefix, f)) return false;;
    }
  }

  size_t iter = UPB_MSG_BEGIN;
  const upb_fielddef *f;
  upb_msgval val;
  while (upb_msg_next(msg, m, ctx->ext_pool, &f, &val, &iter)) {
    // Skip non-submessage fields.
    if (!upb_fielddef_issubmsg(f)) continue;
    const upb_msgdef *sub_m = upb_fielddef_msgsubdef(f);
    if (upb_fielddef_ismap(f)) {
      const upb_fielddef *val_f = upb_msgdef_field(sub_m, 1);
      const upb_msgdef *val_m = upb_fielddef_msgsubdef(val_f);
      if (!val_m) continue;
      const upb_map *map = val.map_val;
      size_t iter = UPB_MAP_BEGIN;
      int i = 0;
      while (upb_mapiter_next(map, &iter)) {
        char *sub_prefix = alloc_prefix(prefix, f, i++);
        if (!sub_prefix) return false;
        upb_msgval map_val = upb_mapiter_value(map, iter);
        bool ok = find_in_msg(ctx, sub_prefix, map_val.msg_val, val_m);
        free(sub_prefix);
        if (!ok) return false;
      }
    } else if (upb_fielddef_isseq(f)) {
      const upb_array *arr = val.array_val;
      for (size_t i = 0, n = upb_array_size(arr); i < n; i++) {
        upb_msgval elem = upb_array_get(arr, i);
        char *sub_prefix = alloc_prefix(prefix, f, i);
        if (!sub_prefix) return false;
        bool ok = find_in_msg(ctx, sub_prefix, elem.msg_val, sub_m);
        free(sub_prefix);
        if (!ok) return false;
      }
    } else {
      char *sub_prefix = alloc_prefix(prefix, f, -1);
      if (!sub_prefix) return false;
      bool ok = find_in_msg(ctx, sub_prefix, val.msg_val, sub_m);
      free(sub_prefix);
      if (!ok) return false;
    }
  }

  return true;
}

bool upb_msg_findunsetrequired(const upb_msg *msg, const upb_msgdef *m,
                               const upb_symtab *ext_pool, char ***fields,
                               size_t *count, upb_arena *arena) {
  Find_Ctx ctx = {arena, NULL, 0, 0, ext_pool};
  if (!find_in_msg(&ctx, "", msg, m)) return false;
  *count = ctx.count;
  *fields = ctx.fields;
  return true;
}
