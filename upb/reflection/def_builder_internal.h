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

#ifndef UPB_REFLECTION_DEF_BUILDER_INTERNAL_H_
#define UPB_REFLECTION_DEF_BUILDER_INTERNAL_H_

#include "upb/reflection/common.h"
#include "upb/reflection/def_pool_internal.h"
#include "upb/reflection/def_type.h"

// Must be last.
#include "upb/port/def.inc"

// We want to copy the options verbatim into the destination options proto.
// We use serialize+parse as our deep copy.
#define UPB_DEF_SET_OPTIONS(target, desc_type, options_type, proto)           \
  if (UPB_DESC(desc_type##_has_options)(proto)) {                             \
    size_t size;                                                              \
    char* pb = UPB_DESC(options_type##_serialize)(                            \
        UPB_DESC(desc_type##_options)(proto), ctx->tmp_arena, &size);         \
    if (!pb) _upb_DefBuilder_OomErr(ctx);                                     \
    target =                                                                  \
        UPB_DESC(options_type##_parse)(pb, size, _upb_DefBuilder_Arena(ctx)); \
    if (!target) _upb_DefBuilder_OomErr(ctx);                                 \
  } else {                                                                    \
    target = (const UPB_DESC(options_type)*)kUpbDefOptDefault;                \
  }

#ifdef __cplusplus
extern "C" {
#endif

struct upb_DefBuilder {
  upb_DefPool* symtab;
  upb_FileDef* file;                 // File we are building.
  upb_Arena* arena;                  // Allocate defs here.
  upb_Arena* tmp_arena;              // For temporary allocations.
  upb_Status* status;                // Record errors here.
  const upb_MiniTableFile* layout;   // NULL if we should build layouts.
  upb_MiniTablePlatform platform;    // Platform we are targeting.
  int enum_count;                    // Count of enums built so far.
  int msg_count;                     // Count of messages built so far.
  int ext_count;                     // Count of extensions built so far.
  jmp_buf err;                       // longjmp() on error.
};

extern const char* kUpbDefOptDefault;

// ctx->status has already been set elsewhere so just fail/longjmp()
UPB_NORETURN void _upb_DefBuilder_FailJmp(upb_DefBuilder* ctx);

UPB_NORETURN void _upb_DefBuilder_Errf(upb_DefBuilder* ctx, const char* fmt,
                                       ...) UPB_PRINTF(2, 3);
UPB_NORETURN void _upb_DefBuilder_OomErr(upb_DefBuilder* ctx);

const char* _upb_DefBuilder_MakeFullName(upb_DefBuilder* ctx,
                                         const char* prefix,
                                         upb_StringView name);

// Given a symbol and the base symbol inside which it is defined,
// find the symbol's definition.
const void* _upb_DefBuilder_ResolveAny(upb_DefBuilder* ctx,
                                       const char* from_name_dbg,
                                       const char* base, upb_StringView sym,
                                       upb_deftype_t* type);

const void* _upb_DefBuilder_Resolve(upb_DefBuilder* ctx,
                                    const char* from_name_dbg, const char* base,
                                    upb_StringView sym, upb_deftype_t type);

char _upb_DefBuilder_ParseEscape(upb_DefBuilder* ctx, const upb_FieldDef* f,
                                 const char** src, const char* end);

const char* _upb_DefBuilder_FullToShort(const char* fullname);

UPB_INLINE void* _upb_DefBuilder_Alloc(upb_DefBuilder* ctx, size_t bytes) {
  if (bytes == 0) return NULL;
  void* ret = upb_Arena_Malloc(ctx->arena, bytes);
  if (!ret) _upb_DefBuilder_OomErr(ctx);
  return ret;
}

// Adds a symbol |v| to the symtab, which must be a def pointer previously
// packed with pack_def(). The def's pointer to upb_FileDef* must be set before
// adding, so we know which entries to remove if building this file fails.
UPB_INLINE void _upb_DefBuilder_Add(upb_DefBuilder* ctx, const char* name,
                                    upb_value v) {
  upb_StringView sym = {.data = name, .size = strlen(name)};
  bool ok = _upb_DefPool_InsertSym(ctx->symtab, sym, v, ctx->status);
  if (!ok) _upb_DefBuilder_FailJmp(ctx);
}

UPB_INLINE upb_Arena* _upb_DefBuilder_Arena(const upb_DefBuilder* ctx) {
  return ctx->arena;
}

UPB_INLINE upb_FileDef* _upb_DefBuilder_File(const upb_DefBuilder* ctx) {
  return ctx->file;
}

// This version of CheckIdent() is only called by other, faster versions after
// they detect a parsing error.
void _upb_DefBuilder_CheckIdentSlow(upb_DefBuilder* ctx, upb_StringView name,
                                    bool full);

// Verify a full identifier string. This is slightly more complicated than
// verifying a relative identifier string because we must track '.' chars.
UPB_INLINE void _upb_DefBuilder_CheckIdentFull(upb_DefBuilder* ctx,
                                               upb_StringView name) {
  bool good = name.size > 0;
  bool start = true;

  for (size_t i = 0; i < name.size; i++) {
    const char c = name.data[i];
    const char d = c | 0x20;  // force lowercase
    const bool is_alpha = (('a' <= d) & (d <= 'z')) | (c == '_');
    const bool is_numer = ('0' <= c) & (c <= '9') & !start;
    const bool is_dot = (c == '.') & !start;

    good &= is_alpha | is_numer | is_dot;
    start = is_dot;
  }

  if (!good) _upb_DefBuilder_CheckIdentSlow(ctx, name, true);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_REFLECTION_DEF_BUILDER_INTERNAL_H_ */
