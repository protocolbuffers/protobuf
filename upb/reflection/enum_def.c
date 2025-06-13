// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/internal/enum_def.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/status.h"
#include "upb/base/string_view.h"
#include "upb/hash/common.h"
#include "upb/hash/int_table.h"
#include "upb/hash/str_table.h"
#include "upb/mem/arena.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_descriptor/internal/encode.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/file.h"
#include "upb/reflection/def.h"
#include "upb/reflection/def_type.h"
#include "upb/reflection/internal/def_builder.h"
#include "upb/reflection/internal/desc_state.h"
#include "upb/reflection/internal/enum_reserved_range.h"
#include "upb/reflection/internal/enum_value_def.h"
#include "upb/reflection/internal/file_def.h"
#include "upb/reflection/internal/strdup2.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_EnumDef {
  UPB_ALIGN_AS(8) const UPB_DESC(EnumOptions*) opts;
  const UPB_DESC(FeatureSet*) resolved_features;
  const upb_MiniTableEnum* layout;  // Only for proto2.
  const upb_FileDef* file;
  const upb_MessageDef* containing_type;  // Could be merged with "file".
  const char* full_name;
  upb_strtable ntoi;
  upb_inttable iton;
  const upb_EnumValueDef* values;
  const upb_EnumReservedRange* res_ranges;
  const upb_StringView* res_names;
  int value_count;
  int res_range_count;
  int res_name_count;
  int32_t defaultval;
  bool is_sorted;  // Whether all of the values are defined in ascending order.
#if UINTPTR_MAX == 0xffffffff
  uint32_t padding;  // Increase size to a multiple of 8.
#endif
};

upb_EnumDef* _upb_EnumDef_At(const upb_EnumDef* e, int i) {
  return (upb_EnumDef*)&e[i];
}

const upb_MiniTableEnum* _upb_EnumDef_MiniTable(const upb_EnumDef* e) {
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

const UPB_DESC(EnumOptions) * upb_EnumDef_Options(const upb_EnumDef* e) {
  return e->opts;
}

bool upb_EnumDef_HasOptions(const upb_EnumDef* e) {
  return e->opts != (void*)kUpbDefOptDefault;
}

const UPB_DESC(FeatureSet) *
    upb_EnumDef_ResolvedFeatures(const upb_EnumDef* e) {
  return e->resolved_features;
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

int upb_EnumDef_ReservedRangeCount(const upb_EnumDef* e) {
  return e->res_range_count;
}

const upb_EnumReservedRange* upb_EnumDef_ReservedRange(const upb_EnumDef* e,
                                                       int i) {
  UPB_ASSERT(0 <= i && i < e->res_range_count);
  return _upb_EnumReservedRange_At(e->res_ranges, i);
}

int upb_EnumDef_ReservedNameCount(const upb_EnumDef* e) {
  return e->res_name_count;
}

upb_StringView upb_EnumDef_ReservedName(const upb_EnumDef* e, int i) {
  UPB_ASSERT(0 <= i && i < e->res_name_count);
  return e->res_names[i];
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
  return upb_MiniTableEnum_CheckValue(e->layout, num);
}

const upb_EnumValueDef* upb_EnumDef_Value(const upb_EnumDef* e, int i) {
  UPB_ASSERT(0 <= i && i < e->value_count);
  return _upb_EnumValueDef_At(e->values, i);
}

bool upb_EnumDef_IsClosed(const upb_EnumDef* e) {
  if (UPB_TREAT_CLOSED_ENUMS_LIKE_OPEN) return false;
  return upb_EnumDef_IsSpecifiedAsClosed(e);
}

bool upb_EnumDef_IsSpecifiedAsClosed(const upb_EnumDef* e) {
  return UPB_DESC(FeatureSet_enum_type)(e->resolved_features) ==
         UPB_DESC(FeatureSet_CLOSED);
}

bool upb_EnumDef_MiniDescriptorEncode(const upb_EnumDef* e, upb_Arena* a,
                                      upb_StringView* out) {
  upb_DescState s;
  _upb_DescState_Init(&s);

  const upb_EnumValueDef** sorted = NULL;
  if (!e->is_sorted) {
    sorted = _upb_EnumValueDefs_Sorted(e->values, e->value_count, a);
    if (!sorted) return false;
  }

  if (!_upb_DescState_Grow(&s, a)) return false;
  s.ptr = upb_MtDataEncoder_StartEnum(&s.e, s.ptr);

  // Duplicate values are allowed but we only encode each value once.
  uint32_t previous = 0;

  for (int i = 0; i < e->value_count; i++) {
    const uint32_t current =
        upb_EnumValueDef_Number(sorted ? sorted[i] : upb_EnumDef_Value(e, i));
    if (i != 0 && previous == current) continue;

    if (!_upb_DescState_Grow(&s, a)) return false;
    s.ptr = upb_MtDataEncoder_PutEnumValue(&s.e, s.ptr, current);
    previous = current;
  }

  if (!_upb_DescState_Grow(&s, a)) return false;
  s.ptr = upb_MtDataEncoder_EndEnum(&s.e, s.ptr);

  // There will always be room for this '\0' in the encoder buffer because
  // kUpb_MtDataEncoder_MinSize is overkill for upb_MtDataEncoder_EndEnum().
  UPB_ASSERT(s.ptr < s.buf + s.bufsize);
  *s.ptr = '\0';

  out->data = s.buf;
  out->size = s.ptr - s.buf;
  return true;
}

static upb_MiniTableEnum* create_enumlayout(upb_DefBuilder* ctx,
                                            const upb_EnumDef* e) {
  upb_StringView sv;
  bool ok = upb_EnumDef_MiniDescriptorEncode(e, ctx->tmp_arena, &sv);
  if (!ok) _upb_DefBuilder_Errf(ctx, "OOM while building enum MiniDescriptor");

  upb_Status status;
  upb_MiniTableEnum* layout =
      upb_MiniTableEnum_Build(sv.data, sv.size, ctx->arena, &status);
  if (!layout)
    _upb_DefBuilder_Errf(ctx, "Error building enum MiniTable: %s", status.msg);
  return layout;
}

static upb_StringView* _upb_EnumReservedNames_New(
    upb_DefBuilder* ctx, int n, const upb_StringView* protos) {
  upb_StringView* sv = UPB_DEFBUILDER_ALLOCARRAY(ctx, upb_StringView, n);
  for (int i = 0; i < n; i++) {
    sv[i].data =
        upb_strdup2(protos[i].data, protos[i].size, _upb_DefBuilder_Arena(ctx));
    sv[i].size = protos[i].size;
  }
  return sv;
}

static void create_enumdef(upb_DefBuilder* ctx, const char* prefix,
                           const UPB_DESC(EnumDescriptorProto) * enum_proto,
                           const UPB_DESC(FeatureSet*) parent_features,
                           upb_EnumDef* e) {
  const UPB_DESC(EnumValueDescriptorProto)* const* values;
  const UPB_DESC(EnumDescriptorProto_EnumReservedRange)* const* res_ranges;
  const upb_StringView* res_names;
  upb_StringView name;
  size_t n_value, n_res_range, n_res_name;

  UPB_DEF_SET_OPTIONS(e->opts, EnumDescriptorProto, EnumOptions, enum_proto);
  e->resolved_features = _upb_DefBuilder_ResolveFeatures(
      ctx, parent_features, UPB_DESC(EnumOptions_features)(e->opts));

  // Must happen before _upb_DefBuilder_Add()
  e->file = _upb_DefBuilder_File(ctx);

  name = UPB_DESC(EnumDescriptorProto_name)(enum_proto);

  e->full_name = _upb_DefBuilder_MakeFullName(ctx, prefix, name);
  _upb_DefBuilder_Add(ctx, e->full_name,
                      _upb_DefType_Pack(e, UPB_DEFTYPE_ENUM));

  values = UPB_DESC(EnumDescriptorProto_value)(enum_proto, &n_value);

  bool ok = upb_strtable_init(&e->ntoi, n_value, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  ok = upb_inttable_init(&e->iton, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  e->defaultval = 0;
  e->value_count = n_value;
  e->values = _upb_EnumValueDefs_New(ctx, prefix, n_value, values,
                                     e->resolved_features, e, &e->is_sorted);

  if (n_value == 0) {
    _upb_DefBuilder_Errf(ctx, "enums must contain at least one value (%s)",
                         e->full_name);
  }

  res_ranges =
      UPB_DESC(EnumDescriptorProto_reserved_range)(enum_proto, &n_res_range);
  e->res_range_count = n_res_range;
  e->res_ranges = _upb_EnumReservedRanges_New(ctx, n_res_range, res_ranges, e);

  res_names =
      UPB_DESC(EnumDescriptorProto_reserved_name)(enum_proto, &n_res_name);
  e->res_name_count = n_res_name;
  e->res_names = _upb_EnumReservedNames_New(ctx, n_res_name, res_names);

  if (!upb_inttable_compact(&e->iton, ctx->arena)) _upb_DefBuilder_OomErr(ctx);

  if (upb_EnumDef_IsClosed(e)) {
    if (ctx->layout) {
      e->layout = upb_MiniTableFile_Enum(ctx->layout, ctx->enum_count++);
    } else {
      e->layout = create_enumlayout(ctx, e);
    }
  } else {
    e->layout = NULL;
  }
}

upb_EnumDef* _upb_EnumDefs_New(upb_DefBuilder* ctx, int n,
                               const UPB_DESC(EnumDescriptorProto*)
                                   const* protos,
                               const UPB_DESC(FeatureSet*) parent_features,
                               const upb_MessageDef* containing_type) {
  _upb_DefType_CheckPadding(sizeof(upb_EnumDef));

  // If a containing type is defined then get the full name from that.
  // Otherwise use the package name from the file def.
  const char* name = containing_type ? upb_MessageDef_FullName(containing_type)
                                     : _upb_FileDef_RawPackage(ctx->file);

  upb_EnumDef* e = UPB_DEFBUILDER_ALLOCARRAY(ctx, upb_EnumDef, n);
  for (int i = 0; i < n; i++) {
    create_enumdef(ctx, name, protos[i], parent_features, &e[i]);
    e[i].containing_type = containing_type;
  }
  return e;
}
