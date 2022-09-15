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

#include "upb/reflection/enum_value_def.h"

#include "upb/reflection/def_builder.h"
#include "upb/reflection/def_type.h"
#include "upb/reflection/enum_def.h"
#include "upb/reflection/file_def.h"

// Must be last.
#include "upb/port_def.inc"

struct upb_EnumValueDef {
  const google_protobuf_EnumValueOptions* opts;
  const upb_EnumDef* parent;
  const char* full_name;
  int32_t number;
};

upb_EnumValueDef* _upb_EnumValueDef_At(const upb_EnumValueDef* v, int i) {
  return (upb_EnumValueDef*)&v[i];
}

const google_protobuf_EnumValueOptions* upb_EnumValueDef_Options(
    const upb_EnumValueDef* v) {
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
                              const google_protobuf_EnumValueDescriptorProto* val_proto,
                              upb_EnumDef* e, upb_EnumValueDef* v) {
  upb_StringView name = google_protobuf_EnumValueDescriptorProto_name(val_proto);

  v->parent = e;  // Must happen prior to _upb_DefBuilder_Add()
  v->full_name = _upb_DefBuilder_MakeFullName(ctx, prefix, name);
  v->number = google_protobuf_EnumValueDescriptorProto_number(val_proto);
  _upb_DefBuilder_Add(ctx, v->full_name,
                      _upb_DefType_Pack(v, UPB_DEFTYPE_ENUMVAL));

  UBP_DEF_SET_OPTIONS(v->opts, EnumValueDescriptorProto, EnumValueOptions,
                      val_proto);

  bool ok = _upb_EnumDef_Insert(e, v, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);
}

// Allocate and initialize an array of |n| enum value defs owned by |e|.
upb_EnumValueDef* _upb_EnumValueDefs_New(
    upb_DefBuilder* ctx, const char* prefix, int n,
    const google_protobuf_EnumValueDescriptorProto* const* protos, upb_EnumDef* e,
    bool* is_sorted) {
  _upb_DefType_CheckPadding(sizeof(upb_EnumValueDef));

  upb_EnumValueDef* v =
      _upb_DefBuilder_Alloc(ctx, sizeof(upb_EnumValueDef) * n);

  *is_sorted = true;
  uint32_t previous = 0;
  for (size_t i = 0; i < n; i++) {
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
