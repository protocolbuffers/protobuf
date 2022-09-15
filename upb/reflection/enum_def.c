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

#include "upb/reflection/enum_def.h"

#include <stdio.h>

#include "upb/mini_table.h"
#include "upb/reflection/def_builder.h"
#include "upb/reflection/def_type.h"
#include "upb/reflection/enum_value_def.h"
#include "upb/reflection/file_def.h"
#include "upb/reflection/message_def.h"
#include "upb/reflection/mini_descriptor_encode.h"

// Must be last.
#include "upb/port_def.inc"

struct upb_EnumDef {
  const google_protobuf_EnumOptions* opts;
  const upb_MiniTable_Enum* layout;  // Only for proto2.
  const upb_FileDef* file;
  const upb_MessageDef* containing_type;  // Could be merged with "file".
  const char* full_name;
  upb_strtable ntoi;
  upb_inttable iton;
  const upb_EnumValueDef* values;
  int value_count;
  int32_t defaultval;
  bool is_sorted;  // Whether all of the values are defined in ascending order.
};

upb_EnumDef* _upb_EnumDef_At(const upb_EnumDef* e, int i) {
  return (upb_EnumDef*)&e[i];
}

// TODO: Maybe implement this on top of a ZCOS instead?
void _upb_EnumDef_Debug(const upb_EnumDef* e) {
  fprintf(stderr, "enum %s (%p) {\n", e->full_name, e);
  fprintf(stderr, " value_count: %d\n", e->value_count);
  fprintf(stderr, " default:     %d\n", e->defaultval);
  fprintf(stderr, " is_sorted:   %d\n", e->is_sorted);
  fprintf(stderr, "}\n");
}

const upb_MiniTable_Enum* _upb_EnumDef_MiniTable(const upb_EnumDef* e) {
  return e->layout;
}

bool _upb_EnumDef_Insert(upb_EnumDef* e, upb_EnumValueDef* v, upb_Arena* a) {
  const char* name = upb_EnumValueDef_Name(v);
  const upb_value val = upb_value_constptr(v);
  bool ok = upb_strtable_insert(&e->ntoi, name, strlen(name), val, a);
  if (!ok) return false;

  // Multiple enumerators can have the same number, first one wins.
  const int number = upb_EnumValueDef_Number(v);
  if (!upb_inttable_lookup(&e->iton, number, NULL)) {
    return upb_inttable_insert(&e->iton, number, val, a);
  }
  return true;
}

static int cmp_values(const void* a, const void* b) {
  const uint32_t A = upb_EnumValueDef_Number(*(const upb_EnumValueDef**)a);
  const uint32_t B = upb_EnumValueDef_Number(*(const upb_EnumValueDef**)b);
  return (A < B) ? -1 : (A > B);
}

const char* _upb_EnumDef_MiniDescriptor(const upb_EnumDef* e, upb_Arena* a) {
  if (e->is_sorted) return _upb_MiniDescriptor_EncodeEnum(e, NULL, a);

  const upb_EnumValueDef** sorted = (const upb_EnumValueDef**)upb_Arena_Malloc(
      a, e->value_count * sizeof(void*));
  if (!sorted) return NULL;

  for (size_t i = 0; i < e->value_count; i++) {
    sorted[i] = upb_EnumDef_Value(e, i);
  }
  qsort(sorted, e->value_count, sizeof(void*), cmp_values);

  return _upb_MiniDescriptor_EncodeEnum(e, sorted, a);
}

const google_protobuf_EnumOptions* upb_EnumDef_Options(const upb_EnumDef* e) {
  return e->opts;
}

bool upb_EnumDef_HasOptions(const upb_EnumDef* e) {
  return e->opts != (void*)kUpbDefOptDefault;
}

const char* upb_EnumDef_FullName(const upb_EnumDef* e) { return e->full_name; }

const char* upb_EnumDef_Name(const upb_EnumDef* e) {
  return _upb_DefBuilder_FullToShort(e->full_name);
}

const upb_FileDef* upb_EnumDef_File(const upb_EnumDef* e) { return e->file; }

const upb_MessageDef* upb_EnumDef_ContainingType(const upb_EnumDef* e) {
  return e->containing_type;
}

int32_t upb_EnumDef_Default(const upb_EnumDef* e) {
  UPB_ASSERT(upb_EnumDef_FindValueByNumber(e, e->defaultval));
  return e->defaultval;
}

int upb_EnumDef_ValueCount(const upb_EnumDef* e) { return e->value_count; }

const upb_EnumValueDef* upb_EnumDef_FindValueByName(const upb_EnumDef* e,
                                                    const char* name) {
  return upb_EnumDef_FindValueByNameWithSize(e, name, strlen(name));
}

const upb_EnumValueDef* upb_EnumDef_FindValueByNameWithSize(
    const upb_EnumDef* e, const char* name, size_t size) {
  upb_value v;
  return upb_strtable_lookup2(&e->ntoi, name, size, &v)
             ? upb_value_getconstptr(v)
             : NULL;
}

const upb_EnumValueDef* upb_EnumDef_FindValueByNumber(const upb_EnumDef* e,
                                                      int32_t num) {
  upb_value v;
  return upb_inttable_lookup(&e->iton, num, &v) ? upb_value_getconstptr(v)
                                                : NULL;
}

bool upb_EnumDef_CheckNumber(const upb_EnumDef* e, int32_t num) {
  // We could use upb_EnumDef_FindValueByNumber(e, num) != NULL, but we expect
  // this to be faster (especially for small numbers).
  return upb_MiniTable_Enum_CheckValue(e->layout, num);
}

const upb_EnumValueDef* upb_EnumDef_Value(const upb_EnumDef* e, int i) {
  UPB_ASSERT(0 <= i && i < e->value_count);
  return _upb_EnumValueDef_At(e->values, i);
}

static upb_MiniTable_Enum* create_enumlayout(upb_DefBuilder* ctx,
                                             const upb_EnumDef* e) {
  const char* desc = _upb_EnumDef_MiniDescriptor(e, ctx->tmp_arena);
  if (!desc)
    _upb_DefBuilder_Errf(ctx, "OOM while building enum MiniDescriptor");

  upb_Status status;
  upb_MiniTable_Enum* layout =
      upb_MiniTable_BuildEnum(desc, strlen(desc), ctx->arena, &status);
  if (!layout)
    _upb_DefBuilder_Errf(ctx, "Error building enum MiniTable: %s", status.msg);
  return layout;
}

static void create_enumdef(upb_DefBuilder* ctx, const char* prefix,
                           const google_protobuf_EnumDescriptorProto* enum_proto,
                           upb_EnumDef* e) {
  const google_protobuf_EnumValueDescriptorProto* const* values;
  upb_StringView name;
  size_t n;

  // Must happen before _upb_DefBuilder_Add()
  e->file = _upb_DefBuilder_File(ctx);

  name = google_protobuf_EnumDescriptorProto_name(enum_proto);
  _upb_DefBuilder_CheckIdentNotFull(ctx, name);

  e->full_name = _upb_DefBuilder_MakeFullName(ctx, prefix, name);
  _upb_DefBuilder_Add(ctx, e->full_name,
                      _upb_DefType_Pack(e, UPB_DEFTYPE_ENUM));

  values = google_protobuf_EnumDescriptorProto_value(enum_proto, &n);

  bool ok = upb_strtable_init(&e->ntoi, n, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  ok = upb_inttable_init(&e->iton, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  e->defaultval = 0;
  e->value_count = n;
  e->values = _upb_EnumValueDefs_New(ctx, prefix, n, values, e, &e->is_sorted);

  if (n == 0) {
    _upb_DefBuilder_Errf(ctx, "enums must contain at least one value (%s)",
                         e->full_name);
  }

  UBP_DEF_SET_OPTIONS(e->opts, EnumDescriptorProto, EnumOptions, enum_proto);

  upb_inttable_compact(&e->iton, ctx->arena);

  if (upb_FileDef_Syntax(e->file) == kUpb_Syntax_Proto2) {
    if (ctx->layout) {
      UPB_ASSERT(ctx->enum_count < ctx->layout->enum_count);
      e->layout = ctx->layout->enums[ctx->enum_count++];
    } else {
      e->layout = create_enumlayout(ctx, e);
    }
  } else {
    e->layout = NULL;
  }
}

upb_EnumDef* _upb_EnumDefs_New(upb_DefBuilder* ctx, int n,
                               const google_protobuf_EnumDescriptorProto* const* protos,
                               const upb_MessageDef* containing_type) {
  _upb_DefType_CheckPadding(sizeof(upb_EnumDef));

  // If a containing type is defined then get the full name from that.
  // Otherwise use the package name from the file def.
  const char* name = containing_type ? upb_MessageDef_FullName(containing_type)
                                     : _upb_FileDef_RawPackage(ctx->file);

  upb_EnumDef* e = _upb_DefBuilder_Alloc(ctx, sizeof(upb_EnumDef) * n);
  for (size_t i = 0; i < n; i++) {
    create_enumdef(ctx, name, protos[i], &e[i]);
    e[i].containing_type = containing_type;
  }
  return e;
}
