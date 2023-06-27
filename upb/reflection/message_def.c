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

#include "upb/hash/int_table.h"
#include "upb/hash/str_table.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_descriptor/internal/modifiers.h"
#include "upb/reflection/def.h"
#include "upb/reflection/def_builder_internal.h"
#include "upb/reflection/def_type.h"
#include "upb/reflection/desc_state_internal.h"
#include "upb/reflection/enum_def_internal.h"
#include "upb/reflection/extension_range_internal.h"
#include "upb/reflection/field_def_internal.h"
#include "upb/reflection/file_def_internal.h"
#include "upb/reflection/message_def_internal.h"
#include "upb/reflection/message_reserved_range_internal.h"
#include "upb/reflection/oneof_def_internal.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_MessageDef {
  const UPB_DESC(MessageOptions) * opts;
  const upb_MiniTable* layout;
  const upb_FileDef* file;
  const upb_MessageDef* containing_type;
  const char* full_name;

  // Tables for looking up fields by number and name.
  upb_inttable itof;
  upb_strtable ntof;

  /* All nested defs.
   * MEM: We could save some space here by putting nested defs in a contiguous
   * region and calculating counts from offsets or vice-versa. */
  const upb_FieldDef* fields;
  const upb_OneofDef* oneofs;
  const upb_ExtensionRange* ext_ranges;
  const upb_StringView* res_names;
  const upb_MessageDef* nested_msgs;
  const upb_MessageReservedRange* res_ranges;
  const upb_EnumDef* nested_enums;
  const upb_FieldDef* nested_exts;

  // TODO(salo): These counters don't need anywhere near 32 bits.
  int field_count;
  int real_oneof_count;
  int oneof_count;
  int ext_range_count;
  int res_range_count;
  int res_name_count;
  int nested_msg_count;
  int nested_enum_count;
  int nested_ext_count;
  bool in_message_set;
  bool is_sorted;
  upb_WellKnown well_known_type;
#if UINTPTR_MAX == 0xffffffff
  uint32_t padding;  // Increase size to a multiple of 8.
#endif
};

static void assign_msg_wellknowntype(upb_MessageDef* m) {
  const char* name = m->full_name;
  if (name == NULL) {
    m->well_known_type = kUpb_WellKnown_Unspecified;
    return;
  }
  if (!strcmp(name, "google.protobuf.Any")) {
    m->well_known_type = kUpb_WellKnown_Any;
  } else if (!strcmp(name, "google.protobuf.FieldMask")) {
    m->well_known_type = kUpb_WellKnown_FieldMask;
  } else if (!strcmp(name, "google.protobuf.Duration")) {
    m->well_known_type = kUpb_WellKnown_Duration;
  } else if (!strcmp(name, "google.protobuf.Timestamp")) {
    m->well_known_type = kUpb_WellKnown_Timestamp;
  } else if (!strcmp(name, "google.protobuf.DoubleValue")) {
    m->well_known_type = kUpb_WellKnown_DoubleValue;
  } else if (!strcmp(name, "google.protobuf.FloatValue")) {
    m->well_known_type = kUpb_WellKnown_FloatValue;
  } else if (!strcmp(name, "google.protobuf.Int64Value")) {
    m->well_known_type = kUpb_WellKnown_Int64Value;
  } else if (!strcmp(name, "google.protobuf.UInt64Value")) {
    m->well_known_type = kUpb_WellKnown_UInt64Value;
  } else if (!strcmp(name, "google.protobuf.Int32Value")) {
    m->well_known_type = kUpb_WellKnown_Int32Value;
  } else if (!strcmp(name, "google.protobuf.UInt32Value")) {
    m->well_known_type = kUpb_WellKnown_UInt32Value;
  } else if (!strcmp(name, "google.protobuf.BoolValue")) {
    m->well_known_type = kUpb_WellKnown_BoolValue;
  } else if (!strcmp(name, "google.protobuf.StringValue")) {
    m->well_known_type = kUpb_WellKnown_StringValue;
  } else if (!strcmp(name, "google.protobuf.BytesValue")) {
    m->well_known_type = kUpb_WellKnown_BytesValue;
  } else if (!strcmp(name, "google.protobuf.Value")) {
    m->well_known_type = kUpb_WellKnown_Value;
  } else if (!strcmp(name, "google.protobuf.ListValue")) {
    m->well_known_type = kUpb_WellKnown_ListValue;
  } else if (!strcmp(name, "google.protobuf.Struct")) {
    m->well_known_type = kUpb_WellKnown_Struct;
  } else {
    m->well_known_type = kUpb_WellKnown_Unspecified;
  }
}

upb_MessageDef* _upb_MessageDef_At(const upb_MessageDef* m, int i) {
  return (upb_MessageDef*)&m[i];
}

bool _upb_MessageDef_IsValidExtensionNumber(const upb_MessageDef* m, int n) {
  for (int i = 0; i < m->ext_range_count; i++) {
    const upb_ExtensionRange* r = upb_MessageDef_ExtensionRange(m, i);
    if (upb_ExtensionRange_Start(r) <= n && n < upb_ExtensionRange_End(r)) {
      return true;
    }
  }
  return false;
}

const UPB_DESC(MessageOptions) *
    upb_MessageDef_Options(const upb_MessageDef* m) {
  return m->opts;
}

bool upb_MessageDef_HasOptions(const upb_MessageDef* m) {
  return m->opts != (void*)kUpbDefOptDefault;
}

const char* upb_MessageDef_FullName(const upb_MessageDef* m) {
  return m->full_name;
}

const upb_FileDef* upb_MessageDef_File(const upb_MessageDef* m) {
  return m->file;
}

const upb_MessageDef* upb_MessageDef_ContainingType(const upb_MessageDef* m) {
  return m->containing_type;
}

const char* upb_MessageDef_Name(const upb_MessageDef* m) {
  return _upb_DefBuilder_FullToShort(m->full_name);
}

upb_Syntax upb_MessageDef_Syntax(const upb_MessageDef* m) {
  return upb_FileDef_Syntax(m->file);
}

const upb_FieldDef* upb_MessageDef_FindFieldByNumber(const upb_MessageDef* m,
                                                     uint32_t i) {
  upb_value val;
  return upb_inttable_lookup(&m->itof, i, &val) ? upb_value_getconstptr(val)
                                                : NULL;
}

const upb_FieldDef* upb_MessageDef_FindFieldByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t size) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, size, &val)) {
    return NULL;
  }

  return _upb_DefType_Unpack(val, UPB_DEFTYPE_FIELD);
}

const upb_OneofDef* upb_MessageDef_FindOneofByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t size) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, size, &val)) {
    return NULL;
  }

  return _upb_DefType_Unpack(val, UPB_DEFTYPE_ONEOF);
}

bool _upb_MessageDef_Insert(upb_MessageDef* m, const char* name, size_t len,
                            upb_value v, upb_Arena* a) {
  return upb_strtable_insert(&m->ntof, name, len, v, a);
}

bool upb_MessageDef_FindByNameWithSize(const upb_MessageDef* m,
                                       const char* name, size_t len,
                                       const upb_FieldDef** out_f,
                                       const upb_OneofDef** out_o) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return false;
  }

  const upb_FieldDef* f = _upb_DefType_Unpack(val, UPB_DEFTYPE_FIELD);
  const upb_OneofDef* o = _upb_DefType_Unpack(val, UPB_DEFTYPE_ONEOF);
  if (out_f) *out_f = f;
  if (out_o) *out_o = o;
  return f || o; /* False if this was a JSON name. */
}

const upb_FieldDef* upb_MessageDef_FindByJsonNameWithSize(
    const upb_MessageDef* m, const char* name, size_t size) {
  upb_value val;
  const upb_FieldDef* f;

  if (!upb_strtable_lookup2(&m->ntof, name, size, &val)) {
    return NULL;
  }

  f = _upb_DefType_Unpack(val, UPB_DEFTYPE_FIELD);
  if (!f) f = _upb_DefType_Unpack(val, UPB_DEFTYPE_FIELD_JSONNAME);

  return f;
}

int upb_MessageDef_ExtensionRangeCount(const upb_MessageDef* m) {
  return m->ext_range_count;
}

int upb_MessageDef_ReservedRangeCount(const upb_MessageDef* m) {
  return m->res_range_count;
}

int upb_MessageDef_ReservedNameCount(const upb_MessageDef* m) {
  return m->res_name_count;
}

int upb_MessageDef_FieldCount(const upb_MessageDef* m) {
  return m->field_count;
}

int upb_MessageDef_OneofCount(const upb_MessageDef* m) {
  return m->oneof_count;
}

int upb_MessageDef_RealOneofCount(const upb_MessageDef* m) {
  return m->real_oneof_count;
}

int upb_MessageDef_NestedMessageCount(const upb_MessageDef* m) {
  return m->nested_msg_count;
}

int upb_MessageDef_NestedEnumCount(const upb_MessageDef* m) {
  return m->nested_enum_count;
}

int upb_MessageDef_NestedExtensionCount(const upb_MessageDef* m) {
  return m->nested_ext_count;
}

const upb_MiniTable* upb_MessageDef_MiniTable(const upb_MessageDef* m) {
  return m->layout;
}

const upb_ExtensionRange* upb_MessageDef_ExtensionRange(const upb_MessageDef* m,
                                                        int i) {
  UPB_ASSERT(0 <= i && i < m->ext_range_count);
  return _upb_ExtensionRange_At(m->ext_ranges, i);
}

const upb_MessageReservedRange* upb_MessageDef_ReservedRange(
    const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->res_range_count);
  return _upb_MessageReservedRange_At(m->res_ranges, i);
}

upb_StringView upb_MessageDef_ReservedName(const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->res_name_count);
  return m->res_names[i];
}

const upb_FieldDef* upb_MessageDef_Field(const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->field_count);
  return _upb_FieldDef_At(m->fields, i);
}

const upb_OneofDef* upb_MessageDef_Oneof(const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->oneof_count);
  return _upb_OneofDef_At(m->oneofs, i);
}

const upb_MessageDef* upb_MessageDef_NestedMessage(const upb_MessageDef* m,
                                                   int i) {
  UPB_ASSERT(0 <= i && i < m->nested_msg_count);
  return &m->nested_msgs[i];
}

const upb_EnumDef* upb_MessageDef_NestedEnum(const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->nested_enum_count);
  return _upb_EnumDef_At(m->nested_enums, i);
}

const upb_FieldDef* upb_MessageDef_NestedExtension(const upb_MessageDef* m,
                                                   int i) {
  UPB_ASSERT(0 <= i && i < m->nested_ext_count);
  return _upb_FieldDef_At(m->nested_exts, i);
}

upb_WellKnown upb_MessageDef_WellKnownType(const upb_MessageDef* m) {
  return m->well_known_type;
}

bool _upb_MessageDef_InMessageSet(const upb_MessageDef* m) {
  return m->in_message_set;
}

const upb_FieldDef* upb_MessageDef_FindFieldByName(const upb_MessageDef* m,
                                                   const char* name) {
  return upb_MessageDef_FindFieldByNameWithSize(m, name, strlen(name));
}

const upb_OneofDef* upb_MessageDef_FindOneofByName(const upb_MessageDef* m,
                                                   const char* name) {
  return upb_MessageDef_FindOneofByNameWithSize(m, name, strlen(name));
}

bool upb_MessageDef_IsMapEntry(const upb_MessageDef* m) {
  return UPB_DESC(MessageOptions_map_entry)(m->opts);
}

bool upb_MessageDef_IsMessageSet(const upb_MessageDef* m) {
  return UPB_DESC(MessageOptions_message_set_wire_format)(m->opts);
}

static upb_MiniTable* _upb_MessageDef_MakeMiniTable(upb_DefBuilder* ctx,
                                                    const upb_MessageDef* m) {
  upb_StringView desc;
  // Note: this will assign layout_index for fields, so upb_FieldDef_MiniTable()
  // is safe to call only after this call.
  bool ok = upb_MessageDef_MiniDescriptorEncode(m, ctx->tmp_arena, &desc);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  void** scratch_data = _upb_DefPool_ScratchData(ctx->symtab);
  size_t* scratch_size = _upb_DefPool_ScratchSize(ctx->symtab);
  upb_MiniTable* ret = upb_MiniTable_BuildWithBuf(
      desc.data, desc.size, ctx->platform, ctx->arena, scratch_data,
      scratch_size, ctx->status);
  if (!ret) _upb_DefBuilder_FailJmp(ctx);

  return ret;
}

void _upb_MessageDef_Resolve(upb_DefBuilder* ctx, upb_MessageDef* m) {
  for (int i = 0; i < m->field_count; i++) {
    upb_FieldDef* f = (upb_FieldDef*)upb_MessageDef_Field(m, i);
    _upb_FieldDef_Resolve(ctx, m->full_name, f);
  }

  m->in_message_set = false;
  for (int i = 0; i < upb_MessageDef_NestedExtensionCount(m); i++) {
    upb_FieldDef* ext = (upb_FieldDef*)upb_MessageDef_NestedExtension(m, i);
    _upb_FieldDef_Resolve(ctx, m->full_name, ext);
    if (upb_FieldDef_Type(ext) == kUpb_FieldType_Message &&
        upb_FieldDef_Label(ext) == kUpb_Label_Optional &&
        upb_FieldDef_MessageSubDef(ext) == m &&
        UPB_DESC(MessageOptions_message_set_wire_format)(
            upb_MessageDef_Options(upb_FieldDef_ContainingType(ext)))) {
      m->in_message_set = true;
    }
  }

  for (int i = 0; i < upb_MessageDef_NestedMessageCount(m); i++) {
    upb_MessageDef* n = (upb_MessageDef*)upb_MessageDef_NestedMessage(m, i);
    _upb_MessageDef_Resolve(ctx, n);
  }
}

void _upb_MessageDef_InsertField(upb_DefBuilder* ctx, upb_MessageDef* m,
                                 const upb_FieldDef* f) {
  const int32_t field_number = upb_FieldDef_Number(f);

  if (field_number <= 0 || field_number > kUpb_MaxFieldNumber) {
    _upb_DefBuilder_Errf(ctx, "invalid field number (%u)", field_number);
  }

  const char* json_name = upb_FieldDef_JsonName(f);
  const char* shortname = upb_FieldDef_Name(f);
  const size_t shortnamelen = strlen(shortname);

  upb_value v = upb_value_constptr(f);

  upb_value existing_v;
  if (upb_strtable_lookup(&m->ntof, shortname, &existing_v)) {
    _upb_DefBuilder_Errf(ctx, "duplicate field name (%s)", shortname);
  }

  const upb_value field_v = _upb_DefType_Pack(f, UPB_DEFTYPE_FIELD);
  bool ok =
      _upb_MessageDef_Insert(m, shortname, shortnamelen, field_v, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  if (strcmp(shortname, json_name) != 0) {
    if (upb_strtable_lookup(&m->ntof, json_name, &v)) {
      _upb_DefBuilder_Errf(ctx, "duplicate json_name (%s)", json_name);
    }

    const size_t json_size = strlen(json_name);
    const upb_value json_v = _upb_DefType_Pack(f, UPB_DEFTYPE_FIELD_JSONNAME);
    ok = _upb_MessageDef_Insert(m, json_name, json_size, json_v, ctx->arena);
    if (!ok) _upb_DefBuilder_OomErr(ctx);
  }

  if (upb_inttable_lookup(&m->itof, field_number, NULL)) {
    _upb_DefBuilder_Errf(ctx, "duplicate field number (%u)", field_number);
  }

  ok = upb_inttable_insert(&m->itof, field_number, v, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);
}

void _upb_MessageDef_CreateMiniTable(upb_DefBuilder* ctx, upb_MessageDef* m) {
  if (ctx->layout == NULL) {
    m->layout = _upb_MessageDef_MakeMiniTable(ctx, m);
  } else {
    UPB_ASSERT(ctx->msg_count < ctx->layout->msg_count);
    m->layout = ctx->layout->msgs[ctx->msg_count++];
    UPB_ASSERT(m->field_count == m->layout->field_count);

    // We don't need the result of this call, but it will assign layout_index
    // for all the fields in O(n lg n) time.
    _upb_FieldDefs_Sorted(m->fields, m->field_count, ctx->tmp_arena);
  }

  for (int i = 0; i < m->nested_msg_count; i++) {
    upb_MessageDef* nested =
        (upb_MessageDef*)upb_MessageDef_NestedMessage(m, i);
    _upb_MessageDef_CreateMiniTable(ctx, nested);
  }
}

void _upb_MessageDef_LinkMiniTable(upb_DefBuilder* ctx,
                                   const upb_MessageDef* m) {
  for (int i = 0; i < upb_MessageDef_NestedExtensionCount(m); i++) {
    const upb_FieldDef* ext = upb_MessageDef_NestedExtension(m, i);
    _upb_FieldDef_BuildMiniTableExtension(ctx, ext);
  }

  for (int i = 0; i < m->nested_msg_count; i++) {
    _upb_MessageDef_LinkMiniTable(ctx, upb_MessageDef_NestedMessage(m, i));
  }

  if (ctx->layout) return;

  for (int i = 0; i < m->field_count; i++) {
    const upb_FieldDef* f = upb_MessageDef_Field(m, i);
    const upb_MessageDef* sub_m = upb_FieldDef_MessageSubDef(f);
    const upb_EnumDef* sub_e = upb_FieldDef_EnumSubDef(f);
    const int layout_index = _upb_FieldDef_LayoutIndex(f);
    upb_MiniTable* mt = (upb_MiniTable*)upb_MessageDef_MiniTable(m);

    UPB_ASSERT(layout_index < m->field_count);
    upb_MiniTableField* mt_f =
        (upb_MiniTableField*)&m->layout->fields[layout_index];
    if (sub_m) {
      if (!mt->subs) {
        _upb_DefBuilder_Errf(ctx, "unexpected submsg for (%s)", m->full_name);
      }
      UPB_ASSERT(mt_f);
      UPB_ASSERT(sub_m->layout);
      if (UPB_UNLIKELY(!upb_MiniTable_SetSubMessage(mt, mt_f, sub_m->layout))) {
        _upb_DefBuilder_Errf(ctx, "invalid submsg for (%s)", m->full_name);
      }
    } else if (_upb_FieldDef_IsClosedEnum(f)) {
      const upb_MiniTableEnum* mt_e = _upb_EnumDef_MiniTable(sub_e);
      if (UPB_UNLIKELY(!upb_MiniTable_SetSubEnum(mt, mt_f, mt_e))) {
        _upb_DefBuilder_Errf(ctx, "invalid subenum for (%s)", m->full_name);
      }
    }
  }

#ifndef NDEBUG
  for (int i = 0; i < m->field_count; i++) {
    const upb_FieldDef* f = upb_MessageDef_Field(m, i);
    const int layout_index = _upb_FieldDef_LayoutIndex(f);
    UPB_ASSERT(layout_index < m->layout->field_count);
    const upb_MiniTableField* mt_f = &m->layout->fields[layout_index];
    UPB_ASSERT(upb_FieldDef_Type(f) == upb_MiniTableField_Type(mt_f));
    UPB_ASSERT(upb_FieldDef_CType(f) == upb_MiniTableField_CType(mt_f));
    UPB_ASSERT(upb_FieldDef_HasPresence(f) ==
               upb_MiniTableField_HasPresence(mt_f));
  }
#endif
}

static uint64_t _upb_MessageDef_Modifiers(const upb_MessageDef* m) {
  uint64_t out = 0;
  if (upb_FileDef_Syntax(m->file) == kUpb_Syntax_Proto3) {
    out |= kUpb_MessageModifier_ValidateUtf8;
    out |= kUpb_MessageModifier_DefaultIsPacked;
  }
  if (m->ext_range_count) {
    out |= kUpb_MessageModifier_IsExtendable;
  }
  return out;
}

static bool _upb_MessageDef_EncodeMap(upb_DescState* s, const upb_MessageDef* m,
                                      upb_Arena* a) {
  if (m->field_count != 2) return false;

  const upb_FieldDef* key_field = upb_MessageDef_Field(m, 0);
  const upb_FieldDef* val_field = upb_MessageDef_Field(m, 1);
  if (key_field == NULL || val_field == NULL) return false;

  UPB_ASSERT(_upb_FieldDef_LayoutIndex(key_field) == 0);
  UPB_ASSERT(_upb_FieldDef_LayoutIndex(val_field) == 1);

  s->ptr = upb_MtDataEncoder_EncodeMap(
      &s->e, s->ptr, upb_FieldDef_Type(key_field), upb_FieldDef_Type(val_field),
      _upb_FieldDef_Modifiers(key_field), _upb_FieldDef_Modifiers(val_field));
  return true;
}

static bool _upb_MessageDef_EncodeMessage(upb_DescState* s,
                                          const upb_MessageDef* m,
                                          upb_Arena* a) {
  const upb_FieldDef** sorted = NULL;
  if (!m->is_sorted) {
    sorted = _upb_FieldDefs_Sorted(m->fields, m->field_count, a);
    if (!sorted) return false;
  }

  s->ptr = upb_MtDataEncoder_StartMessage(&s->e, s->ptr,
                                          _upb_MessageDef_Modifiers(m));

  for (int i = 0; i < m->field_count; i++) {
    const upb_FieldDef* f = sorted ? sorted[i] : upb_MessageDef_Field(m, i);
    const upb_FieldType type = upb_FieldDef_Type(f);
    const int number = upb_FieldDef_Number(f);
    const uint64_t modifiers = _upb_FieldDef_Modifiers(f);

    if (!_upb_DescState_Grow(s, a)) return false;
    s->ptr = upb_MtDataEncoder_PutField(&s->e, s->ptr, type, number, modifiers);
  }

  for (int i = 0; i < m->real_oneof_count; i++) {
    if (!_upb_DescState_Grow(s, a)) return false;
    s->ptr = upb_MtDataEncoder_StartOneof(&s->e, s->ptr);

    const upb_OneofDef* o = upb_MessageDef_Oneof(m, i);
    const int field_count = upb_OneofDef_FieldCount(o);
    for (int j = 0; j < field_count; j++) {
      const int number = upb_FieldDef_Number(upb_OneofDef_Field(o, j));

      if (!_upb_DescState_Grow(s, a)) return false;
      s->ptr = upb_MtDataEncoder_PutOneofField(&s->e, s->ptr, number);
    }
  }

  return true;
}

static bool _upb_MessageDef_EncodeMessageSet(upb_DescState* s,
                                             const upb_MessageDef* m,
                                             upb_Arena* a) {
  s->ptr = upb_MtDataEncoder_EncodeMessageSet(&s->e, s->ptr);

  return true;
}

bool upb_MessageDef_MiniDescriptorEncode(const upb_MessageDef* m, upb_Arena* a,
                                         upb_StringView* out) {
  upb_DescState s;
  _upb_DescState_Init(&s);

  if (!_upb_DescState_Grow(&s, a)) return false;

  if (upb_MessageDef_IsMapEntry(m)) {
    if (!_upb_MessageDef_EncodeMap(&s, m, a)) return false;
  } else if (UPB_DESC(MessageOptions_message_set_wire_format)(m->opts)) {
    if (!_upb_MessageDef_EncodeMessageSet(&s, m, a)) return false;
  } else {
    if (!_upb_MessageDef_EncodeMessage(&s, m, a)) return false;
  }

  if (!_upb_DescState_Grow(&s, a)) return false;
  *s.ptr = '\0';

  out->data = s.buf;
  out->size = s.ptr - s.buf;
  return true;
}

static upb_StringView* _upb_ReservedNames_New(upb_DefBuilder* ctx, int n,
                                              const upb_StringView* protos) {
  upb_StringView* sv = _upb_DefBuilder_Alloc(ctx, sizeof(upb_StringView) * n);
  for (size_t i = 0; i < n; i++) {
    sv[i].data =
        upb_strdup2(protos[i].data, protos[i].size, _upb_DefBuilder_Arena(ctx));
    sv[i].size = protos[i].size;
  }
  return sv;
}

static void create_msgdef(upb_DefBuilder* ctx, const char* prefix,
                          const UPB_DESC(DescriptorProto) * msg_proto,
                          const upb_MessageDef* containing_type,
                          upb_MessageDef* m) {
  const UPB_DESC(OneofDescriptorProto)* const* oneofs;
  const UPB_DESC(FieldDescriptorProto)* const* fields;
  const UPB_DESC(DescriptorProto_ExtensionRange)* const* ext_ranges;
  const UPB_DESC(DescriptorProto_ReservedRange)* const* res_ranges;
  const upb_StringView* res_names;
  size_t n_oneof, n_field, n_enum, n_ext, n_msg;
  size_t n_ext_range, n_res_range, n_res_name;
  upb_StringView name;

  // Must happen before _upb_DefBuilder_Add()
  m->file = _upb_DefBuilder_File(ctx);

  m->containing_type = containing_type;
  m->is_sorted = true;

  name = UPB_DESC(DescriptorProto_name)(msg_proto);

  m->full_name = _upb_DefBuilder_MakeFullName(ctx, prefix, name);
  _upb_DefBuilder_Add(ctx, m->full_name, _upb_DefType_Pack(m, UPB_DEFTYPE_MSG));

  oneofs = UPB_DESC(DescriptorProto_oneof_decl)(msg_proto, &n_oneof);
  fields = UPB_DESC(DescriptorProto_field)(msg_proto, &n_field);
  ext_ranges =
      UPB_DESC(DescriptorProto_extension_range)(msg_proto, &n_ext_range);
  res_ranges =
      UPB_DESC(DescriptorProto_reserved_range)(msg_proto, &n_res_range);
  res_names = UPB_DESC(DescriptorProto_reserved_name)(msg_proto, &n_res_name);

  bool ok = upb_inttable_init(&m->itof, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  ok = upb_strtable_init(&m->ntof, n_oneof + n_field, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  UPB_DEF_SET_OPTIONS(m->opts, DescriptorProto, MessageOptions, msg_proto);

  m->oneof_count = n_oneof;
  m->oneofs = _upb_OneofDefs_New(ctx, n_oneof, oneofs, m);

  m->field_count = n_field;
  m->fields =
      _upb_FieldDefs_New(ctx, n_field, fields, m->full_name, m, &m->is_sorted);

  // Message Sets may not contain fields.
  if (UPB_UNLIKELY(UPB_DESC(MessageOptions_message_set_wire_format)(m->opts))) {
    if (UPB_UNLIKELY(n_field > 0)) {
      _upb_DefBuilder_Errf(ctx, "invalid message set (%s)", m->full_name);
    }
  }

  m->ext_range_count = n_ext_range;
  m->ext_ranges = _upb_ExtensionRanges_New(ctx, n_ext_range, ext_ranges, m);

  m->res_range_count = n_res_range;
  m->res_ranges =
      _upb_MessageReservedRanges_New(ctx, n_res_range, res_ranges, m);

  m->res_name_count = n_res_name;
  m->res_names = _upb_ReservedNames_New(ctx, n_res_name, res_names);

  const size_t synthetic_count = _upb_OneofDefs_Finalize(ctx, m);
  m->real_oneof_count = m->oneof_count - synthetic_count;

  assign_msg_wellknowntype(m);
  upb_inttable_compact(&m->itof, ctx->arena);

  const UPB_DESC(EnumDescriptorProto)* const* enums =
      UPB_DESC(DescriptorProto_enum_type)(msg_proto, &n_enum);
  m->nested_enum_count = n_enum;
  m->nested_enums = _upb_EnumDefs_New(ctx, n_enum, enums, m);

  const UPB_DESC(FieldDescriptorProto)* const* exts =
      UPB_DESC(DescriptorProto_extension)(msg_proto, &n_ext);
  m->nested_ext_count = n_ext;
  m->nested_exts = _upb_Extensions_New(ctx, n_ext, exts, m->full_name, m);

  const UPB_DESC(DescriptorProto)* const* msgs =
      UPB_DESC(DescriptorProto_nested_type)(msg_proto, &n_msg);
  m->nested_msg_count = n_msg;
  m->nested_msgs = _upb_MessageDefs_New(ctx, n_msg, msgs, m);
}

// Allocate and initialize an array of |n| message defs.
upb_MessageDef* _upb_MessageDefs_New(
    upb_DefBuilder* ctx, int n, const UPB_DESC(DescriptorProto) * const* protos,
    const upb_MessageDef* containing_type) {
  _upb_DefType_CheckPadding(sizeof(upb_MessageDef));

  const char* name = containing_type ? containing_type->full_name
                                     : _upb_FileDef_RawPackage(ctx->file);

  upb_MessageDef* m = _upb_DefBuilder_Alloc(ctx, sizeof(upb_MessageDef) * n);
  for (int i = 0; i < n; i++) {
    create_msgdef(ctx, name, protos[i], containing_type, &m[i]);
  }
  return m;
}
