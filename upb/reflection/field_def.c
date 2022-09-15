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

#include "upb/reflection/field_def.h"

#include <ctype.h>
#include <errno.h>

#include "upb/mini_table.h"
#include "upb/reflection/def_builder.h"
#include "upb/reflection/def_pool.h"
#include "upb/reflection/def_type.h"
#include "upb/reflection/enum_def.h"
#include "upb/reflection/enum_value_def.h"
#include "upb/reflection/extension_range.h"
#include "upb/reflection/file_def.h"
#include "upb/reflection/message_def.h"
#include "upb/reflection/oneof_def.h"

// Must be last.
#include "upb/port_def.inc"

#define UPB_FIELD_TYPE_UNSPECIFIED 0

typedef struct {
  size_t len;
  char str[1];  // Null-terminated string data follows.
} str_t;

struct upb_FieldDef {
  const google_protobuf_FieldOptions* opts;
  const upb_FileDef* file;
  const upb_MessageDef* msgdef;
  const char* full_name;
  const char* json_name;
  union {
    int64_t sint;
    uint64_t uint;
    double dbl;
    float flt;
    bool boolean;
    str_t* str;
  } defaultval;
  union {
    const upb_OneofDef* oneof;
    const upb_MessageDef* extension_scope;
  } scope;
  union {
    const upb_MessageDef* msgdef;
    const upb_EnumDef* enumdef;
    const google_protobuf_FieldDescriptorProto* unresolved;
  } sub;
  uint32_t number_;
  uint16_t index_;
  uint16_t layout_index; /* Index into msgdef->layout->fields or file->exts */
  bool has_default;
  bool is_extension_;
  bool packed_;
  bool proto3_optional_;
  bool has_json_name_;
  upb_FieldType type_;
  upb_Label label_;
#if UINTPTR_MAX == 0xffffffff
  uint32_t padding;  // Increase size to a multiple of 8.
#endif
};

upb_FieldDef* _upb_FieldDef_At(const upb_FieldDef* f, int i) {
  return (upb_FieldDef*)&f[i];
}

const google_protobuf_FieldOptions* upb_FieldDef_Options(const upb_FieldDef* f) {
  return f->opts;
}

bool upb_FieldDef_HasOptions(const upb_FieldDef* f) {
  return f->opts != (void*)kUpbDefOptDefault;
}

const char* upb_FieldDef_FullName(const upb_FieldDef* f) {
  return f->full_name;
}

upb_CType upb_FieldDef_CType(const upb_FieldDef* f) {
  switch (f->type_) {
    case kUpb_FieldType_Double:
      return kUpb_CType_Double;
    case kUpb_FieldType_Float:
      return kUpb_CType_Float;
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_SInt64:
    case kUpb_FieldType_SFixed64:
      return kUpb_CType_Int64;
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_SFixed32:
    case kUpb_FieldType_SInt32:
      return kUpb_CType_Int32;
    case kUpb_FieldType_UInt64:
    case kUpb_FieldType_Fixed64:
      return kUpb_CType_UInt64;
    case kUpb_FieldType_UInt32:
    case kUpb_FieldType_Fixed32:
      return kUpb_CType_UInt32;
    case kUpb_FieldType_Enum:
      return kUpb_CType_Enum;
    case kUpb_FieldType_Bool:
      return kUpb_CType_Bool;
    case kUpb_FieldType_String:
      return kUpb_CType_String;
    case kUpb_FieldType_Bytes:
      return kUpb_CType_Bytes;
    case kUpb_FieldType_Group:
    case kUpb_FieldType_Message:
      return kUpb_CType_Message;
  }
  UPB_UNREACHABLE();
}

upb_FieldType upb_FieldDef_Type(const upb_FieldDef* f) { return f->type_; }

uint32_t upb_FieldDef_Index(const upb_FieldDef* f) { return f->index_; }

upb_Label upb_FieldDef_Label(const upb_FieldDef* f) { return f->label_; }

uint32_t upb_FieldDef_Number(const upb_FieldDef* f) { return f->number_; }

bool upb_FieldDef_IsExtension(const upb_FieldDef* f) {
  return f->is_extension_;
}

bool upb_FieldDef_IsPacked(const upb_FieldDef* f) { return f->packed_; }

const char* upb_FieldDef_Name(const upb_FieldDef* f) {
  return _upb_DefBuilder_FullToShort(f->full_name);
}

const char* upb_FieldDef_JsonName(const upb_FieldDef* f) {
  return f->json_name;
}

bool upb_FieldDef_HasJsonName(const upb_FieldDef* f) {
  return f->has_json_name_;
}

const upb_FileDef* upb_FieldDef_File(const upb_FieldDef* f) { return f->file; }

const upb_MessageDef* upb_FieldDef_ContainingType(const upb_FieldDef* f) {
  return f->msgdef;
}

const upb_MessageDef* upb_FieldDef_ExtensionScope(const upb_FieldDef* f) {
  return f->is_extension_ ? f->scope.extension_scope : NULL;
}

const upb_OneofDef* upb_FieldDef_ContainingOneof(const upb_FieldDef* f) {
  return f->is_extension_ ? NULL : f->scope.oneof;
}

const upb_OneofDef* upb_FieldDef_RealContainingOneof(const upb_FieldDef* f) {
  const upb_OneofDef* oneof = upb_FieldDef_ContainingOneof(f);
  if (!oneof || upb_OneofDef_IsSynthetic(oneof)) return NULL;
  return oneof;
}

upb_MessageValue upb_FieldDef_Default(const upb_FieldDef* f) {
  UPB_ASSERT(!upb_FieldDef_IsSubMessage(f));
  upb_MessageValue ret;

  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Bool:
      return (upb_MessageValue){.bool_val = f->defaultval.boolean};
    case kUpb_CType_Int64:
      return (upb_MessageValue){.int64_val = f->defaultval.sint};
    case kUpb_CType_UInt64:
      return (upb_MessageValue){.uint64_val = f->defaultval.uint};
    case kUpb_CType_Enum:
    case kUpb_CType_Int32:
      return (upb_MessageValue){.int32_val = (int32_t)f->defaultval.sint};
    case kUpb_CType_UInt32:
      return (upb_MessageValue){.uint32_val = (uint32_t)f->defaultval.uint};
    case kUpb_CType_Float:
      return (upb_MessageValue){.float_val = f->defaultval.flt};
    case kUpb_CType_Double:
      return (upb_MessageValue){.double_val = f->defaultval.dbl};
    case kUpb_CType_String:
    case kUpb_CType_Bytes: {
      str_t* str = f->defaultval.str;
      if (str) {
        return (upb_MessageValue){
            .str_val = (upb_StringView){.data = str->str, .size = str->len}};
      } else {
        return (upb_MessageValue){
            .str_val = (upb_StringView){.data = NULL, .size = 0}};
      }
    }
    default:
      UPB_UNREACHABLE();
  }

  return ret;
}

const upb_MessageDef* upb_FieldDef_MessageSubDef(const upb_FieldDef* f) {
  return upb_FieldDef_CType(f) == kUpb_CType_Message ? f->sub.msgdef : NULL;
}

const upb_EnumDef* upb_FieldDef_EnumSubDef(const upb_FieldDef* f) {
  return upb_FieldDef_CType(f) == kUpb_CType_Enum ? f->sub.enumdef : NULL;
}

const upb_MiniTable_Field* upb_FieldDef_MiniTable(const upb_FieldDef* f) {
  UPB_ASSERT(!upb_FieldDef_IsExtension(f));
  const upb_MiniTable* layout = upb_MessageDef_MiniTable(f->msgdef);
  return &layout->fields[f->layout_index];
}

const upb_MiniTable_Extension* _upb_FieldDef_ExtensionMiniTable(
    const upb_FieldDef* f) {
  UPB_ASSERT(upb_FieldDef_IsExtension(f));
  const upb_FileDef* file = upb_FieldDef_File(f);
  return _upb_FileDef_ExtensionMiniTable(file, f->layout_index);
}

bool _upb_FieldDef_IsProto3Optional(const upb_FieldDef* f) {
  return f->proto3_optional_;
}

bool upb_FieldDef_IsSubMessage(const upb_FieldDef* f) {
  return upb_FieldDef_CType(f) == kUpb_CType_Message;
}

bool upb_FieldDef_IsString(const upb_FieldDef* f) {
  return upb_FieldDef_CType(f) == kUpb_CType_String ||
         upb_FieldDef_CType(f) == kUpb_CType_Bytes;
}

bool upb_FieldDef_IsOptional(const upb_FieldDef* f) {
  return upb_FieldDef_Label(f) == kUpb_Label_Optional;
}

bool upb_FieldDef_IsRequired(const upb_FieldDef* f) {
  return upb_FieldDef_Label(f) == kUpb_Label_Required;
}

bool upb_FieldDef_IsRepeated(const upb_FieldDef* f) {
  return upb_FieldDef_Label(f) == kUpb_Label_Repeated;
}

bool upb_FieldDef_IsPrimitive(const upb_FieldDef* f) {
  return !upb_FieldDef_IsString(f) && !upb_FieldDef_IsSubMessage(f);
}

bool upb_FieldDef_IsMap(const upb_FieldDef* f) {
  return upb_FieldDef_IsRepeated(f) && upb_FieldDef_IsSubMessage(f) &&
         upb_MessageDef_IsMapEntry(upb_FieldDef_MessageSubDef(f));
}

bool upb_FieldDef_HasDefault(const upb_FieldDef* f) { return f->has_default; }

bool upb_FieldDef_HasSubDef(const upb_FieldDef* f) {
  return upb_FieldDef_IsSubMessage(f) ||
         upb_FieldDef_CType(f) == kUpb_CType_Enum;
}

bool upb_FieldDef_HasPresence(const upb_FieldDef* f) {
  if (upb_FieldDef_IsRepeated(f)) return false;
  const upb_FileDef* file = upb_FieldDef_File(f);
  return upb_FieldDef_IsSubMessage(f) || upb_FieldDef_ContainingOneof(f) ||
         upb_FileDef_Syntax(file) == kUpb_Syntax_Proto2;
}

static bool between(int32_t x, int32_t low, int32_t high) {
  return x >= low && x <= high;
}

bool upb_FieldDef_checklabel(int32_t label) { return between(label, 1, 3); }
bool upb_FieldDef_checktype(int32_t type) { return between(type, 1, 11); }
bool upb_FieldDef_checkintfmt(int32_t fmt) { return between(fmt, 1, 3); }

bool upb_FieldDef_checkdescriptortype(int32_t type) {
  return between(type, 1, 18);
}

/* Code to build defs from descriptor protos. *********************************/

/* There is a question of how much validation to do here.  It will be difficult
 * to perfectly match the amount of validation performed by proto2.  But since
 * this code is used to directly build defs from Ruby (for example) we do need
 * to validate important constraints like uniqueness of names and numbers. */

static size_t div_round_up(size_t n, size_t d) { return (n + d - 1) / d; }

static size_t upb_MessageValue_sizeof(upb_CType type) {
  switch (type) {
    case kUpb_CType_Double:
    case kUpb_CType_Int64:
    case kUpb_CType_UInt64:
      return 8;
    case kUpb_CType_Enum:
    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Float:
      return 4;
    case kUpb_CType_Bool:
      return 1;
    case kUpb_CType_Message:
      return sizeof(void*);
    case kUpb_CType_Bytes:
    case kUpb_CType_String:
      return sizeof(upb_StringView);
  }
  UPB_UNREACHABLE();
}

static uint8_t upb_msg_fielddefsize(const upb_FieldDef* f) {
  if (upb_MessageDef_IsMapEntry(upb_FieldDef_ContainingType(f))) {
    upb_MapEntry ent;
    UPB_ASSERT(sizeof(ent.k) == sizeof(ent.v));
    return sizeof(ent.k);
  } else if (upb_FieldDef_IsRepeated(f)) {
    return sizeof(void*);
  } else {
    return upb_MessageValue_sizeof(upb_FieldDef_CType(f));
  }
}

static uint32_t upb_MiniTable_place(upb_DefBuilder* ctx, upb_MiniTable* l,
                                    size_t size, const upb_MessageDef* m) {
  size_t ofs = UPB_ALIGN_UP(l->size, size);
  size_t next = ofs + size;

  if (next > UINT16_MAX) {
    _upb_DefBuilder_Errf(ctx,
                         "size of message %s exceeded max size of %zu bytes",
                         upb_MessageDef_FullName(m), (size_t)UINT16_MAX);
  }

  l->size = next;
  return ofs;
}

static int field_number_cmp(const void* p1, const void* p2) {
  const upb_MiniTable_Field* f1 = p1;
  const upb_MiniTable_Field* f2 = p2;
  return f1->number - f2->number;
}

static void assign_layout_indices(const upb_MessageDef* m, upb_MiniTable* l,
                                  upb_MiniTable_Field* fields) {
  int i;
  int n = upb_MessageDef_FieldCount(m);
  int dense_below = 0;
  for (i = 0; i < n; i++) {
    upb_FieldDef* f =
        (upb_FieldDef*)upb_MessageDef_FindFieldByNumber(m, fields[i].number);
    UPB_ASSERT(f);
    f->layout_index = i;
    if (i < UINT8_MAX && fields[i].number == i + 1 &&
        (i == 0 || fields[i - 1].number == i)) {
      dense_below = i + 1;
    }
  }
  l->dense_below = dense_below;
}

static uint8_t map_descriptortype(const upb_FieldDef* f) {
  uint8_t type = upb_FieldDef_Type(f);
  /* See TableDescriptorType() in upbc/generator.cc for details and
   * rationale of these exceptions. */
  if (type == kUpb_FieldType_String) {
    const upb_FileDef* file = upb_FieldDef_File(f);
    const upb_Syntax syntax = upb_FileDef_Syntax(file);

    if (syntax == kUpb_Syntax_Proto2) return kUpb_FieldType_Bytes;
  } else if (type == kUpb_FieldType_Enum) {
    const upb_FileDef* file = upb_EnumDef_File(f->sub.enumdef);
    const upb_Syntax syntax = upb_FileDef_Syntax(file);

    if (syntax == kUpb_Syntax_Proto3 || UPB_TREAT_PROTO2_ENUMS_LIKE_PROTO3 ||
        // TODO(https://github.com/protocolbuffers/upb/issues/541):
        // fix map enum values to check for unknown enum values and put
        // them in the unknown field set.
        upb_MessageDef_IsMapEntry(upb_FieldDef_ContainingType(f))) {
      return kUpb_FieldType_Int32;
    }
  }
  return type;
}

static void fill_fieldlayout(upb_MiniTable_Field* field,
                             const upb_FieldDef* f) {
  field->number = upb_FieldDef_Number(f);
  field->descriptortype = map_descriptortype(f);

  if (upb_FieldDef_IsMap(f)) {
    field->mode =
        kUpb_FieldMode_Map | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift);
  } else if (upb_FieldDef_IsRepeated(f)) {
    field->mode =
        kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift);
  } else {
    /* Maps descriptor type -> elem_size_lg2.  */
    static const uint8_t sizes[] = {
        -1,                       /* invalid descriptor type */
        kUpb_FieldRep_8Byte,      /* DOUBLE */
        kUpb_FieldRep_4Byte,      /* FLOAT */
        kUpb_FieldRep_8Byte,      /* INT64 */
        kUpb_FieldRep_8Byte,      /* UINT64 */
        kUpb_FieldRep_4Byte,      /* INT32 */
        kUpb_FieldRep_8Byte,      /* FIXED64 */
        kUpb_FieldRep_4Byte,      /* FIXED32 */
        kUpb_FieldRep_1Byte,      /* BOOL */
        kUpb_FieldRep_StringView, /* STRING */
        kUpb_FieldRep_Pointer,    /* GROUP */
        kUpb_FieldRep_Pointer,    /* MESSAGE */
        kUpb_FieldRep_StringView, /* BYTES */
        kUpb_FieldRep_4Byte,      /* UINT32 */
        kUpb_FieldRep_4Byte,      /* ENUM */
        kUpb_FieldRep_4Byte,      /* SFIXED32 */
        kUpb_FieldRep_8Byte,      /* SFIXED64 */
        kUpb_FieldRep_4Byte,      /* SINT32 */
        kUpb_FieldRep_8Byte,      /* SINT64 */
    };
    field->mode = kUpb_FieldMode_Scalar |
                  (sizes[field->descriptortype] << kUpb_FieldRep_Shift);
  }

  if (upb_FieldDef_IsPacked(f)) {
    field->mode |= kUpb_LabelFlags_IsPacked;
  }

  if (upb_FieldDef_IsExtension(f)) {
    field->mode |= kUpb_LabelFlags_IsExtension;
  }
}

/* This function is the dynamic equivalent of message_layout.{cc,h} in upbc.
 * It computes a dynamic layout for all of the fields in |m|. */
void _upb_FieldDef_MakeLayout(upb_DefBuilder* ctx, const upb_MessageDef* m) {
  upb_MiniTable* l = (upb_MiniTable*)upb_MessageDef_MiniTable(m);
  size_t field_count = upb_MessageDef_FieldCount(m);
  size_t sublayout_count = 0;
  upb_MiniTable_Sub* subs;
  upb_MiniTable_Field* fields;

  memset(l, 0, sizeof(*l) + sizeof(_upb_FastTable_Entry));

  // Count sub-messages.
  for (size_t i = 0; i < field_count; i++) {
    const upb_FieldDef* f = upb_MessageDef_Field(m, i);
    if (upb_FieldDef_IsSubMessage(f)) {
      sublayout_count++;
    }
    if (upb_FieldDef_CType(f) == kUpb_CType_Enum &&
        upb_FileDef_Syntax(upb_EnumDef_File(f->sub.enumdef)) ==
            kUpb_Syntax_Proto2) {
      sublayout_count++;
    }
  }

  fields = _upb_DefBuilder_Alloc(ctx, field_count * sizeof(*fields));
  subs = _upb_DefBuilder_Alloc(ctx, sublayout_count * sizeof(*subs));

  l->field_count = upb_MessageDef_FieldCount(m);
  l->fields = fields;
  l->subs = subs;
  l->table_mask = 0;
  l->required_count = 0;

  if (upb_MessageDef_ExtensionRangeCount(m) > 0) {
    if (google_protobuf_MessageOptions_message_set_wire_format(
            upb_MessageDef_Options(m))) {
      l->ext = kUpb_ExtMode_IsMessageSet;
    } else {
      l->ext = kUpb_ExtMode_Extendable;
    }
  } else {
    l->ext = kUpb_ExtMode_NonExtendable;
  }

  /* TODO(haberman): initialize fast tables so that reflection-based parsing
   * can get the same speeds as linked-in types. */
  l->fasttable[0].field_parser = &_upb_FastDecoder_DecodeGeneric;
  l->fasttable[0].field_data = 0;

  if (upb_MessageDef_IsMapEntry(m)) {
    /* TODO(haberman): refactor this method so this special case is more
     * elegant. */
    const upb_FieldDef* key = upb_MessageDef_FindFieldByNumber(m, 1);
    const upb_FieldDef* val = upb_MessageDef_FindFieldByNumber(m, 2);
    fields[0].number = 1;
    fields[1].number = 2;
    fields[0].mode = kUpb_FieldMode_Scalar;
    fields[1].mode = kUpb_FieldMode_Scalar;
    fields[0].presence = 0;
    fields[1].presence = 0;
    fields[0].descriptortype = map_descriptortype(key);
    fields[1].descriptortype = map_descriptortype(val);
    fields[0].offset = 0;
    fields[1].offset = sizeof(upb_StringView);
    fields[1].submsg_index = 0;

    if (upb_FieldDef_CType(val) == kUpb_CType_Message) {
      subs[0].submsg =
          upb_MessageDef_MiniTable(upb_FieldDef_MessageSubDef(val));
    }

    upb_FieldDef* fielddefs = (upb_FieldDef*)upb_MessageDef_Field(m, 0);
    UPB_ASSERT(fielddefs[0].number_ == 1);
    UPB_ASSERT(fielddefs[1].number_ == 2);
    fielddefs[0].layout_index = 0;
    fielddefs[1].layout_index = 1;

    l->field_count = 2;
    l->size = 2 * sizeof(upb_StringView);
    l->size = UPB_ALIGN_UP(l->size, 8);
    l->dense_below = 2;
    return;
  }

  /* Allocate data offsets in three stages:
   *
   * 1. hasbits.
   * 2. regular fields.
   * 3. oneof fields.
   *
   * OPT: There is a lot of room for optimization here to minimize the size.
   */

  /* Assign hasbits for required fields first. */
  size_t hasbit = 0;

  for (int i = 0; i < upb_MessageDef_FieldCount(m); i++) {
    const upb_FieldDef* f = upb_MessageDef_Field(m, i);
    upb_MiniTable_Field* field = &fields[upb_FieldDef_Index(f)];
    if (upb_FieldDef_Label(f) == kUpb_Label_Required) {
      field->presence = ++hasbit;
      if (hasbit >= 63) {
        _upb_DefBuilder_Errf(ctx, "Message with >=63 required fields: %s",
                             upb_MessageDef_FullName(m));
      }
      l->required_count++;
    }
  }

  /* Allocate hasbits and set basic field attributes. */
  sublayout_count = 0;
  for (int i = 0; i < upb_MessageDef_FieldCount(m); i++) {
    const upb_FieldDef* f = upb_MessageDef_Field(m, i);
    upb_MiniTable_Field* field = &fields[upb_FieldDef_Index(f)];

    fill_fieldlayout(field, f);

    if (field->descriptortype == kUpb_FieldType_Message ||
        field->descriptortype == kUpb_FieldType_Group) {
      field->submsg_index = sublayout_count++;
      subs[field->submsg_index].submsg =
          upb_MessageDef_MiniTable(upb_FieldDef_MessageSubDef(f));
    } else if (field->descriptortype == kUpb_FieldType_Enum) {
      field->submsg_index = sublayout_count++;
      subs[field->submsg_index].subenum =
          _upb_EnumDef_MiniTable(upb_FieldDef_EnumSubDef(f));
      UPB_ASSERT(subs[field->submsg_index].subenum);
    }

    if (upb_FieldDef_Label(f) == kUpb_Label_Required) {
      /* Hasbit was already assigned. */
    } else if (upb_FieldDef_HasPresence(f) &&
               !upb_FieldDef_RealContainingOneof(f)) {
      /* We don't use hasbit 0, so that 0 can indicate "no presence" in the
       * table. This wastes one hasbit, but we don't worry about it for now. */
      field->presence = ++hasbit;
    } else {
      field->presence = 0;
    }
  }

  /* Account for space used by hasbits. */
  l->size = hasbit ? div_round_up(hasbit + 1, 8) : 0;

  /* Allocate non-oneof fields. */
  for (int i = 0; i < upb_MessageDef_FieldCount(m); i++) {
    const upb_FieldDef* f = upb_MessageDef_Field(m, i);
    size_t field_size = upb_msg_fielddefsize(f);
    size_t index = upb_FieldDef_Index(f);

    if (upb_FieldDef_RealContainingOneof(f)) {
      /* Oneofs are handled separately below. */
      continue;
    }

    fields[index].offset = upb_MiniTable_place(ctx, l, field_size, m);
  }

  /* Allocate oneof fields.  Each oneof field consists of a uint32 for the case
   * and space for the actual data. */
  for (int i = 0; i < upb_MessageDef_OneofCount(m); i++) {
    const upb_OneofDef* o = upb_MessageDef_Oneof(m, i);
    size_t case_size = sizeof(uint32_t); /* Could potentially optimize this. */
    size_t field_size = 0;
    uint32_t case_offset;
    uint32_t data_offset;

    if (upb_OneofDef_IsSynthetic(o)) continue;

    if (upb_OneofDef_FieldCount(o) == 0) {
      _upb_DefBuilder_Errf(ctx, "Oneof must have at least one field (%s)",
                           upb_OneofDef_FullName(o));
    }

    /* Calculate field size: the max of all field sizes. */
    for (int j = 0; j < upb_OneofDef_FieldCount(o); j++) {
      const upb_FieldDef* f = upb_OneofDef_Field(o, j);
      field_size = UPB_MAX(field_size, upb_msg_fielddefsize(f));
    }

    /* Align and allocate case offset. */
    case_offset = upb_MiniTable_place(ctx, l, case_size, m);
    data_offset = upb_MiniTable_place(ctx, l, field_size, m);

    for (int i = 0; i < upb_OneofDef_FieldCount(o); i++) {
      const upb_FieldDef* f = upb_OneofDef_Field(o, i);
      fields[upb_FieldDef_Index(f)].offset = data_offset;
      fields[upb_FieldDef_Index(f)].presence = ~case_offset;
    }
  }

  /* Size of the entire structure should be a multiple of its greatest
   * alignment.  TODO: track overall alignment for real? */
  l->size = UPB_ALIGN_UP(l->size, 8);

  /* Sort fields by number. */
  if (fields) {
    qsort(fields, upb_MessageDef_FieldCount(m), sizeof(*fields),
          field_number_cmp);
  }
  assign_layout_indices(m, l, fields);
}

static bool streql2(const char* a, size_t n, const char* b) {
  return n == strlen(b) && memcmp(a, b, n) == 0;
}

// Implement the transformation as described in the spec:
//   1. upper case all letters after an underscore.
//   2. remove all underscores.
static char* make_json_name(const char* name, size_t size, upb_Arena* a) {
  char* out = upb_Arena_Malloc(a, size + 1);  // +1 is to add a trailing '\0'
  if (out == NULL) return NULL;

  bool ucase_next = false;
  char* des = out;
  for (size_t i = 0; i < size; i++) {
    if (name[i] == '_') {
      ucase_next = true;
    } else {
      *des++ = ucase_next ? toupper(name[i]) : name[i];
      ucase_next = false;
    }
  }
  *des++ = '\0';
  return out;
}

static str_t* newstr(upb_DefBuilder* ctx, const char* data, size_t len) {
  str_t* ret = _upb_DefBuilder_Alloc(ctx, sizeof(*ret) + len);
  if (!ret) _upb_DefBuilder_OomErr(ctx);
  ret->len = len;
  if (len) memcpy(ret->str, data, len);
  ret->str[len] = '\0';
  return ret;
}

static str_t* unescape(upb_DefBuilder* ctx, const upb_FieldDef* f,
                       const char* data, size_t len) {
  // Size here is an upper bound; escape sequences could ultimately shrink it.
  str_t* ret = _upb_DefBuilder_Alloc(ctx, sizeof(*ret) + len);
  char* dst = &ret->str[0];
  const char* src = data;
  const char* end = data + len;

  while (src < end) {
    if (*src == '\\') {
      src++;
      *dst++ = _upb_DefBuilder_ParseEscape(ctx, f, &src, end);
    } else {
      *dst++ = *src++;
    }
  }

  ret->len = dst - &ret->str[0];
  return ret;
}

static void parse_default(upb_DefBuilder* ctx, const char* str, size_t len,
                          upb_FieldDef* f) {
  char* end;
  char nullz[64];
  errno = 0;

  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Int32:
    case kUpb_CType_Int64:
    case kUpb_CType_UInt32:
    case kUpb_CType_UInt64:
    case kUpb_CType_Double:
    case kUpb_CType_Float:
      /* Standard C number parsing functions expect null-terminated strings. */
      if (len >= sizeof(nullz) - 1) {
        _upb_DefBuilder_Errf(ctx, "Default too long: %.*s", (int)len, str);
      }
      memcpy(nullz, str, len);
      nullz[len] = '\0';
      str = nullz;
      break;
    default:
      break;
  }

  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Int32: {
      long val = strtol(str, &end, 0);
      if (val > INT32_MAX || val < INT32_MIN || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.sint = val;
      break;
    }
    case kUpb_CType_Enum: {
      const upb_EnumDef* e = f->sub.enumdef;
      const upb_EnumValueDef* ev =
          upb_EnumDef_FindValueByNameWithSize(e, str, len);
      if (!ev) {
        goto invalid;
      }
      f->defaultval.sint = upb_EnumValueDef_Number(ev);
      break;
    }
    case kUpb_CType_Int64: {
      long long val = strtoll(str, &end, 0);
      if (val > INT64_MAX || val < INT64_MIN || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.sint = val;
      break;
    }
    case kUpb_CType_UInt32: {
      unsigned long val = strtoul(str, &end, 0);
      if (val > UINT32_MAX || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.uint = val;
      break;
    }
    case kUpb_CType_UInt64: {
      unsigned long long val = strtoull(str, &end, 0);
      if (val > UINT64_MAX || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.uint = val;
      break;
    }
    case kUpb_CType_Double: {
      double val = strtod(str, &end);
      if (errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.dbl = val;
      break;
    }
    case kUpb_CType_Float: {
      float val = strtof(str, &end);
      if (errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.flt = val;
      break;
    }
    case kUpb_CType_Bool: {
      if (streql2(str, len, "false")) {
        f->defaultval.boolean = false;
      } else if (streql2(str, len, "true")) {
        f->defaultval.boolean = true;
      } else {
        goto invalid;
      }
      break;
    }
    case kUpb_CType_String:
      f->defaultval.str = newstr(ctx, str, len);
      break;
    case kUpb_CType_Bytes:
      f->defaultval.str = unescape(ctx, f, str, len);
      break;
    case kUpb_CType_Message:
      /* Should not have a default value. */
      _upb_DefBuilder_Errf(ctx, "Message should not have a default (%s)",
                           upb_FieldDef_FullName(f));
  }

  return;

invalid:
  _upb_DefBuilder_Errf(ctx, "Invalid default '%.*s' for field %s of type %d",
                       (int)len, str, upb_FieldDef_FullName(f),
                       (int)upb_FieldDef_Type(f));
}

static void set_default_default(upb_DefBuilder* ctx, upb_FieldDef* f) {
  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Int32:
    case kUpb_CType_Int64:
      f->defaultval.sint = 0;
      break;
    case kUpb_CType_UInt64:
    case kUpb_CType_UInt32:
      f->defaultval.uint = 0;
      break;
    case kUpb_CType_Double:
    case kUpb_CType_Float:
      f->defaultval.dbl = 0;
      break;
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      f->defaultval.str = newstr(ctx, NULL, 0);
      break;
    case kUpb_CType_Bool:
      f->defaultval.boolean = false;
      break;
    case kUpb_CType_Enum: {
      const upb_EnumValueDef* v = upb_EnumDef_Value(f->sub.enumdef, 0);
      f->defaultval.sint = upb_EnumValueDef_Number(v);
    }
    case kUpb_CType_Message:
      break;
  }
}

static void _upb_FieldDef_Create(upb_DefBuilder* ctx, const char* prefix,
                                 upb_MessageDef* m,
                                 const google_protobuf_FieldDescriptorProto* field_proto,
                                 upb_FieldDef* f) {
  // Must happen before _upb_DefBuilder_Add()
  f->file = _upb_DefBuilder_File(ctx);

  if (!google_protobuf_FieldDescriptorProto_has_name(field_proto)) {
    _upb_DefBuilder_Errf(ctx, "field has no name");
  }

  const upb_StringView name = google_protobuf_FieldDescriptorProto_name(field_proto);
  _upb_DefBuilder_CheckIdentNotFull(ctx, name);

  f->has_json_name_ = google_protobuf_FieldDescriptorProto_has_json_name(field_proto);
  if (f->has_json_name_) {
    const upb_StringView sv =
        google_protobuf_FieldDescriptorProto_json_name(field_proto);
    f->json_name = upb_strdup2(sv.data, sv.size, ctx->arena);
  } else {
    f->json_name = make_json_name(name.data, name.size, ctx->arena);
  }
  if (!f->json_name) _upb_DefBuilder_OomErr(ctx);

  f->full_name = _upb_DefBuilder_MakeFullName(ctx, prefix, name);
  f->label_ = (int)google_protobuf_FieldDescriptorProto_label(field_proto);
  f->number_ = google_protobuf_FieldDescriptorProto_number(field_proto);
  f->proto3_optional_ =
      google_protobuf_FieldDescriptorProto_proto3_optional(field_proto);
  f->msgdef = m;
  f->scope.oneof = NULL;

  const bool has_type = google_protobuf_FieldDescriptorProto_has_type(field_proto);
  const bool has_type_name =
      google_protobuf_FieldDescriptorProto_has_type_name(field_proto);

  f->type_ = (int)google_protobuf_FieldDescriptorProto_type(field_proto);

  if (has_type) {
    switch (f->type_) {
      case kUpb_FieldType_Message:
      case kUpb_FieldType_Group:
      case kUpb_FieldType_Enum:
        if (!has_type_name) {
          _upb_DefBuilder_Errf(ctx, "field of type %d requires type name (%s)",
                               (int)f->type_, f->full_name);
        }
        break;
      default:
        if (has_type_name) {
          _upb_DefBuilder_Errf(
              ctx, "invalid type for field with type_name set (%s, %d)",
              f->full_name, (int)f->type_);
        }
    }
  } else if (has_type_name) {
    f->type_ =
        UPB_FIELD_TYPE_UNSPECIFIED;  // We'll fill this in in resolve_fielddef()
  }

  if (f->type_ < kUpb_FieldType_Double || f->type_ > kUpb_FieldType_SInt64) {
    _upb_DefBuilder_Errf(ctx, "invalid type for field %s (%d)", f->full_name,
                         f->type_);
  }

  if (f->label_ < kUpb_Label_Optional || f->label_ > kUpb_Label_Repeated) {
    _upb_DefBuilder_Errf(ctx, "invalid label for field %s (%d)", f->full_name,
                         f->label_);
  }

  /* We can't resolve the subdef or (in the case of extensions) the containing
   * message yet, because it may not have been defined yet.  We stash a pointer
   * to the field_proto until later when we can properly resolve it. */
  f->sub.unresolved = field_proto;

  if (f->label_ == kUpb_Label_Required &&
      upb_FileDef_Syntax(f->file) == kUpb_Syntax_Proto3) {
    _upb_DefBuilder_Errf(ctx, "proto3 fields cannot be required (%s)",
                         f->full_name);
  }

  if (google_protobuf_FieldDescriptorProto_has_oneof_index(field_proto)) {
    uint32_t oneof_index = google_protobuf_FieldDescriptorProto_oneof_index(field_proto);

    if (upb_FieldDef_Label(f) != kUpb_Label_Optional) {
      _upb_DefBuilder_Errf(ctx, "fields in oneof must have OPTIONAL label (%s)",
                           f->full_name);
    }

    if (!m) {
      _upb_DefBuilder_Errf(ctx, "oneof_index provided for extension field (%s)",
                           f->full_name);
    }

    if (oneof_index >= upb_MessageDef_OneofCount(m)) {
      _upb_DefBuilder_Errf(ctx, "oneof_index out of range (%s)", f->full_name);
    }

    upb_OneofDef* oneof = (upb_OneofDef*)upb_MessageDef_Oneof(m, oneof_index);
    f->scope.oneof = oneof;

    bool ok = _upb_OneofDef_Insert(oneof, f, name.data, name.size, ctx->arena);
    if (!ok) _upb_DefBuilder_OomErr(ctx);
  } else {
    if (f->proto3_optional_) {
      _upb_DefBuilder_Errf(ctx,
                           "field with proto3_optional was not in a oneof (%s)",
                           f->full_name);
    }
  }

  UBP_DEF_SET_OPTIONS(f->opts, FieldDescriptorProto, FieldOptions, field_proto);

  if (google_protobuf_FieldOptions_has_packed(f->opts)) {
    f->packed_ = google_protobuf_FieldOptions_packed(f->opts);
  } else {
    // Repeated fields default to packed for proto3 only.
    f->packed_ = upb_FieldDef_IsPrimitive(f) &&
                 f->label_ == kUpb_Label_Repeated &&
                 upb_FileDef_Syntax(f->file) == kUpb_Syntax_Proto3;
  }
}

static void _upb_FieldDef_CreateExt(
    upb_DefBuilder* ctx, const char* prefix,
    const google_protobuf_FieldDescriptorProto* field_proto, upb_MessageDef* m,
    upb_FieldDef* f) {
  _upb_FieldDef_Create(ctx, prefix, m, field_proto, f);
  f->is_extension_ = true;

  f->scope.extension_scope = m;
  _upb_DefBuilder_Add(ctx, f->full_name, _upb_DefType_Pack(f, UPB_DEFTYPE_EXT));
  f->layout_index = ctx->ext_count++;

  if (ctx->layout) {
    UPB_ASSERT(_upb_FieldDef_ExtensionMiniTable(f)->field.number == f->number_);
  }
}

static void _upb_FieldDef_CreateNotExt(
    upb_DefBuilder* ctx, const char* prefix,
    const google_protobuf_FieldDescriptorProto* field_proto, upb_MessageDef* m,
    upb_FieldDef* f) {
  _upb_FieldDef_Create(ctx, prefix, m, field_proto, f);
  f->is_extension_ = false;

  _upb_MessageDef_InsertField(ctx, m, f);

  if (ctx->layout) {
    const upb_MiniTable* mt = upb_MessageDef_MiniTable(m);
    const upb_MiniTable_Field* fields = mt->fields;
    const int count = mt->field_count;
    bool found = false;
    for (int i = 0; i < count; i++) {
      if (fields[i].number == f->number_) {
        f->layout_index = i;
        found = true;
        break;
      }
    }
    UPB_ASSERT(found);
  }
}

upb_FieldDef* _upb_FieldDefs_New(
    upb_DefBuilder* ctx, int n,
    const google_protobuf_FieldDescriptorProto* const* protos, const char* prefix,
    upb_MessageDef* m, bool is_ext) {
  _upb_DefType_CheckPadding(sizeof(upb_FieldDef));
  upb_FieldDef* f =
      (upb_FieldDef*)_upb_DefBuilder_Alloc(ctx, sizeof(upb_FieldDef) * n);

  if (is_ext) {
    for (size_t i = 0; i < n; i++) {
      _upb_FieldDef_CreateExt(ctx, prefix, protos[i], m, &f[i]);
      f[i].index_ = i;
    }
  } else {
    for (size_t i = 0; i < n; i++) {
      _upb_FieldDef_CreateNotExt(ctx, prefix, protos[i], m, &f[i]);
      f[i].index_ = i;
    }
  }
  return f;
}

static void resolve_subdef(upb_DefBuilder* ctx, const char* prefix,
                           upb_FieldDef* f) {
  const google_protobuf_FieldDescriptorProto* field_proto = f->sub.unresolved;
  upb_StringView name =
      google_protobuf_FieldDescriptorProto_type_name(field_proto);
  bool has_name =
      google_protobuf_FieldDescriptorProto_has_type_name(field_proto);
  switch ((int)f->type_) {
    case UPB_FIELD_TYPE_UNSPECIFIED: {
      // Type was not specified and must be inferred.
      UPB_ASSERT(has_name);
      upb_deftype_t type;
      const void* def =
          _upb_DefBuilder_ResolveAny(ctx, f->full_name, prefix, name, &type);
      switch (type) {
        case UPB_DEFTYPE_ENUM:
          f->sub.enumdef = def;
          f->type_ = kUpb_FieldType_Enum;
          break;
        case UPB_DEFTYPE_MSG:
          f->sub.msgdef = def;
          f->type_ = kUpb_FieldType_Message;  // It appears there is no way of
                                              // this being a group.
          break;
        default:
          _upb_DefBuilder_Errf(ctx, "Couldn't resolve type name for field %s",
                               f->full_name);
      }
    }
    case kUpb_FieldType_Message:
    case kUpb_FieldType_Group:
      UPB_ASSERT(has_name);
      f->sub.msgdef = _upb_DefBuilder_Resolve(ctx, f->full_name, prefix, name,
                                              UPB_DEFTYPE_MSG);
      break;
    case kUpb_FieldType_Enum:
      UPB_ASSERT(has_name);
      f->sub.enumdef = _upb_DefBuilder_Resolve(ctx, f->full_name, prefix, name,
                                               UPB_DEFTYPE_ENUM);
      break;
    default:
      // No resolution necessary.
      break;
  }
}

static void resolve_extension(upb_DefBuilder* ctx, const char* prefix,
                              upb_FieldDef* f,
                              const google_protobuf_FieldDescriptorProto* field_proto) {
  if (!google_protobuf_FieldDescriptorProto_has_extendee(field_proto)) {
    _upb_DefBuilder_Errf(ctx, "extension for field '%s' had no extendee",
                         f->full_name);
  }

  upb_StringView name = google_protobuf_FieldDescriptorProto_extendee(field_proto);
  const upb_MessageDef* m =
      _upb_DefBuilder_Resolve(ctx, f->full_name, prefix, name, UPB_DEFTYPE_MSG);
  f->msgdef = m;

  bool found = false;

  for (int i = 0, n = upb_MessageDef_ExtensionRangeCount(m); i < n; i++) {
    const upb_ExtensionRange* r = upb_MessageDef_ExtensionRange(m, i);
    if (upb_ExtensionRange_Start(r) <= f->number_ &&
        f->number_ < upb_ExtensionRange_End(r)) {
      found = true;
      break;
    }
  }

  if (!found) {
    _upb_DefBuilder_Errf(
        ctx,
        "field number %u in extension %s has no extension range in "
        "message %s",
        (unsigned)f->number_, f->full_name, upb_MessageDef_FullName(f->msgdef));
  }

  const upb_MiniTable_Extension* ext = _upb_FieldDef_ExtensionMiniTable(f);
  if (ctx->layout) {
    UPB_ASSERT(upb_FieldDef_Number(f) == ext->field.number);
  } else {
    upb_MiniTable_Extension* mut_ext = (upb_MiniTable_Extension*)ext;
    fill_fieldlayout(&mut_ext->field, f);
    mut_ext->field.presence = 0;
    mut_ext->field.offset = 0;
    mut_ext->field.submsg_index = 0;
    mut_ext->extendee = upb_MessageDef_MiniTable(f->msgdef);
    mut_ext->sub.submsg = upb_MessageDef_MiniTable(f->sub.msgdef);
  }

  bool ok = _upb_DefPool_InsertExt(ctx->symtab, ext, f, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);
}

static void resolve_default(upb_DefBuilder* ctx, upb_FieldDef* f,
                            const google_protobuf_FieldDescriptorProto* field_proto) {
  // Have to delay resolving of the default value until now because of the enum
  // case, since enum defaults are specified with a label.
  if (google_protobuf_FieldDescriptorProto_has_default_value(field_proto)) {
    upb_StringView defaultval =
        google_protobuf_FieldDescriptorProto_default_value(field_proto);

    if (upb_FileDef_Syntax(f->file) == kUpb_Syntax_Proto3) {
      _upb_DefBuilder_Errf(ctx,
                           "proto3 fields cannot have explicit defaults (%s)",
                           f->full_name);
    }

    if (upb_FieldDef_IsSubMessage(f)) {
      _upb_DefBuilder_Errf(ctx,
                           "message fields cannot have explicit defaults (%s)",
                           f->full_name);
    }

    parse_default(ctx, defaultval.data, defaultval.size, f);
    f->has_default = true;
  } else {
    set_default_default(ctx, f);
    f->has_default = false;
  }
}

void _upb_FieldDef_Resolve(upb_DefBuilder* ctx, const char* prefix,
                           upb_FieldDef* f) {
  // We have to stash this away since resolve_subdef() may overwrite it.
  const google_protobuf_FieldDescriptorProto* field_proto = f->sub.unresolved;

  resolve_subdef(ctx, prefix, f);
  resolve_default(ctx, f, field_proto);

  if (f->is_extension_) {
    resolve_extension(ctx, prefix, f, field_proto);
  }
}
