// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/internal/enum_value_def.h"

#include "upb/reflection/def_type.h"
#include "upb/reflection/internal/def_builder.h"
#include "upb/reflection/internal/enum_def.h"
#include "upb/reflection/internal/file_def.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_EnumValueDef {
  const UPB_DESC(EnumValueOptions) * opts;
  const upb_EnumDef* parent;
  const char* full_name;
  int32_t number;
};

upb_EnumValueDef* _upb_EnumValueDef_At(const upb_EnumValueDef* v, int i) {
  return (upb_EnumValueDef*)&v[i];
}

static int _upb_EnumValueDef_Compare(const void* p1, const void* p2) {
  const uint32_t v1 = (*(const upb_EnumValueDef**)p1)->number;
  const uint32_t v2 = (*(const upb_EnumValueDef**)p2)->number;
  return (v1 < v2) ? -1 : (v1 > v2);
}

const upb_EnumValueDef** _upb_EnumValueDefs_Sorted(const upb_EnumValueDef* v,
                                                   int n, upb_Arena* a) {
  // TODO: Try to replace this arena alloc with a persistent scratch buffer.
  upb_EnumValueDef** out =
      (upb_EnumValueDef**)upb_Arena_Malloc(a, n * sizeof(void*));
  if (!out) return NULL;

  for (int i = 0; i < n; i++) {
    out[i] = (upb_EnumValueDef*)&v[i];
  }
  qsort(out, n, sizeof(void*), _upb_EnumValueDef_Compare);

  return (const upb_EnumValueDef**)out;
}

const UPB_DESC(EnumValueOptions) *
    upb_EnumValueDef_Options(const upb_EnumValueDef* v) {
  return v->opts;
}

bool upb_EnumValueDef_HasOptions(const upb_EnumValueDef* v) {
  return v->opts != (void*)kUpbDefOptDefault;
}

const upb_EnumDef* upb_EnumValueDef_Enum(const upb_EnumValueDef* v) {
  return v->parent;
}

const char* upb_EnumValueDef_FullName(const upb_EnumValueDef* v) {
  return v->full_name;
}

const char* upb_EnumValueDef_Name(const upb_EnumValueDef* v) {
  return _upb_DefBuilder_FullToShort(v->full_name);
}

int32_t upb_EnumValueDef_Number(const upb_EnumValueDef* v) { return v->number; }

uint32_t upb_EnumValueDef_Index(const upb_EnumValueDef* v) {
  // Compute index in our parent's array.
  return v - upb_EnumDef_Value(v->parent, 0);
}

static void create_enumvaldef(upb_DefBuilder* ctx, const char* prefix,
                              const UPB_DESC(EnumValueDescriptorProto) *
                                  val_proto,
                              upb_EnumDef* e, upb_EnumValueDef* v) {
  upb_StringView name = UPB_DESC(EnumValueDescriptorProto_name)(val_proto);

  v->parent = e;  // Must happen prior to _upb_DefBuilder_Add()
  v->full_name = _upb_DefBuilder_MakeFullName(ctx, prefix, name);
  v->number = UPB_DESC(EnumValueDescriptorProto_number)(val_proto);
  _upb_DefBuilder_Add(ctx, v->full_name,
                      _upb_DefType_Pack(v, UPB_DEFTYPE_ENUMVAL));

  UPB_DEF_SET_OPTIONS(v->opts, EnumValueDescriptorProto, EnumValueOptions,
                      val_proto);

  bool ok = _upb_EnumDef_Insert(e, v, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);
}

// Allocate and initialize an array of |n| enum value defs owned by |e|.
upb_EnumValueDef* _upb_EnumValueDefs_New(
    upb_DefBuilder* ctx, const char* prefix, int n,
    const UPB_DESC(EnumValueDescriptorProto) * const* protos, upb_EnumDef* e,
    bool* is_sorted) {
  _upb_DefType_CheckPadding(sizeof(upb_EnumValueDef));

  upb_EnumValueDef* v =
      _upb_DefBuilder_Alloc(ctx, sizeof(upb_EnumValueDef) * n);

  *is_sorted = true;
  uint32_t previous = 0;
  for (int i = 0; i < n; i++) {
    create_enumvaldef(ctx, prefix, protos[i], e, &v[i]);

    const uint32_t current = v[i].number;
    if (previous > current) *is_sorted = false;
    previous = current;
  }

  if (upb_FileDef_Syntax(ctx->file) == kUpb_Syntax_Proto3 && n > 0 &&
      v[0].number != 0) {
    _upb_DefBuilder_Errf(ctx,
                         "for proto3, the first enum value must be zero (%s)",
                         upb_EnumDef_FullName(e));
  }

  return v;
}
