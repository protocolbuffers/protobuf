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

#include "upb/def.h"

#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "google/protobuf/descriptor.upb.h"
#include "upb/mini_table.h"
#include "upb/reflection.h"

/* Must be last. */
#include "upb/port_def.inc"

typedef struct {
  size_t len;
  char str[1]; /* Null-terminated string data follows. */
} str_t;

/* The upb core does not generally have a concept of default instances. However
 * for descriptor options we make an exception since the max size is known and
 * modest (<200 bytes). All types can share a default instance since it is
 * initialized to zeroes.
 *
 * We have to allocate an extra pointer for upb's internal metadata. */
static const char opt_default_buf[_UPB_MAXOPT_SIZE + sizeof(void*)] = {0};
static const char* opt_default = &opt_default_buf[sizeof(void*)];

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

struct upb_ExtensionRange {
  const google_protobuf_ExtensionRangeOptions* opts;
  int32_t start;
  int32_t end;
};

struct upb_MessageDef {
  const google_protobuf_MessageOptions* opts;
  const upb_MiniTable* layout;
  const upb_FileDef* file;
  const upb_MessageDef* containing_type;
  const char* full_name;

  /* Tables for looking up fields by number and name. */
  upb_inttable itof;
  upb_strtable ntof;

  /* All nested defs.
   * MEM: We could save some space here by putting nested defs in a contiguous
   * region and calculating counts from offsets or vice-versa. */
  const upb_FieldDef* fields;
  const upb_OneofDef* oneofs;
  const upb_ExtensionRange* ext_ranges;
  const upb_MessageDef* nested_msgs;
  const upb_EnumDef* nested_enums;
  const upb_FieldDef* nested_exts;
  int field_count;
  int real_oneof_count;
  int oneof_count;
  int ext_range_count;
  int nested_msg_count;
  int nested_enum_count;
  int nested_ext_count;
  bool in_message_set;
  upb_WellKnown well_known_type;
#if UINTPTR_MAX == 0xffffffff
  uint32_t padding;  // Increase size to a multiple of 8.
#endif
};

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
#if UINTPTR_MAX == 0xffffffff
  uint32_t padding;  // Increase size to a multiple of 8.
#endif
};

struct upb_EnumValueDef {
  const google_protobuf_EnumValueOptions* opts;
  const upb_EnumDef* parent;
  const char* full_name;
  int32_t number;
};

struct upb_OneofDef {
  const google_protobuf_OneofOptions* opts;
  const upb_MessageDef* parent;
  const char* full_name;
  int field_count;
  bool synthetic;
  const upb_FieldDef** fields;
  upb_strtable ntof;
  upb_inttable itof;
#if UINTPTR_MAX == 0xffffffff
  uint32_t padding;  // Increase size to a multiple of 8.
#endif
};

struct upb_FileDef {
  const google_protobuf_FileOptions* opts;
  const char* name;
  const char* package;

  const upb_FileDef** deps;
  const int32_t* public_deps;
  const int32_t* weak_deps;
  const upb_MessageDef* top_lvl_msgs;
  const upb_EnumDef* top_lvl_enums;
  const upb_FieldDef* top_lvl_exts;
  const upb_ServiceDef* services;
  const upb_MiniTable_Extension** ext_layouts;
  const upb_DefPool* symtab;

  int dep_count;
  int public_dep_count;
  int weak_dep_count;
  int top_lvl_msg_count;
  int top_lvl_enum_count;
  int top_lvl_ext_count;
  int service_count;
  int ext_count; /* All exts in the file. */
  upb_Syntax syntax;
};

struct upb_MethodDef {
  const google_protobuf_MethodOptions* opts;
  upb_ServiceDef* service;
  const char* full_name;
  const upb_MessageDef* input_type;
  const upb_MessageDef* output_type;
  int index;
  bool client_streaming;
  bool server_streaming;
};

struct upb_ServiceDef {
  const google_protobuf_ServiceOptions* opts;
  const upb_FileDef* file;
  const char* full_name;
  upb_MethodDef* methods;
  int method_count;
  int index;
};

struct upb_DefPool {
  upb_Arena* arena;
  upb_strtable syms;  /* full_name -> packed def ptr */
  upb_strtable files; /* file_name -> upb_FileDef* */
  upb_inttable exts;  /* upb_MiniTable_Extension* -> upb_FieldDef* */
  upb_ExtensionRegistry* extreg;
  size_t bytes_loaded;
};

/* Inside a symtab we store tagged pointers to specific def types. */
typedef enum {
  UPB_DEFTYPE_MASK = 7,

  /* Only inside symtab table. */
  UPB_DEFTYPE_EXT = 0,
  UPB_DEFTYPE_MSG = 1,
  UPB_DEFTYPE_ENUM = 2,
  UPB_DEFTYPE_ENUMVAL = 3,
  UPB_DEFTYPE_SERVICE = 4,

  /* Only inside message table. */
  UPB_DEFTYPE_FIELD = 0,
  UPB_DEFTYPE_ONEOF = 1,
  UPB_DEFTYPE_FIELD_JSONNAME = 2,

  /* Only inside file table. */
  UPB_DEFTYPE_FILE = 0,
  UPB_DEFTYPE_LAYOUT = 1
} upb_deftype_t;

#define FIELD_TYPE_UNSPECIFIED 0

static upb_deftype_t deftype(upb_value v) {
  uintptr_t num = (uintptr_t)upb_value_getconstptr(v);
  return num & UPB_DEFTYPE_MASK;
}

static const void* unpack_def(upb_value v, upb_deftype_t type) {
  uintptr_t num = (uintptr_t)upb_value_getconstptr(v);
  return (num & UPB_DEFTYPE_MASK) == type
             ? (const void*)(num & ~UPB_DEFTYPE_MASK)
             : NULL;
}

static upb_value pack_def(const void* ptr, upb_deftype_t type) {
  // Our 3-bit pointer tagging requires all pointers to be multiples of 8.
  // The arena will always yield 8-byte-aligned addresses, however we put
  // the defs into arrays.  For each element in the array to be 8-byte-aligned,
  // the sizes of each def type must also be a multiple of 8.
  //
  // If any of these asserts fail, we need to add or remove padding on 32-bit
  // machines (64-bit machines will have 8-byte alignment already due to
  // pointers, which all of these structs have).
  UPB_ASSERT((sizeof(upb_FieldDef) & UPB_DEFTYPE_MASK) == 0);
  UPB_ASSERT((sizeof(upb_MessageDef) & UPB_DEFTYPE_MASK) == 0);
  UPB_ASSERT((sizeof(upb_EnumDef) & UPB_DEFTYPE_MASK) == 0);
  UPB_ASSERT((sizeof(upb_EnumValueDef) & UPB_DEFTYPE_MASK) == 0);
  UPB_ASSERT((sizeof(upb_ServiceDef) & UPB_DEFTYPE_MASK) == 0);
  UPB_ASSERT((sizeof(upb_OneofDef) & UPB_DEFTYPE_MASK) == 0);
  uintptr_t num = (uintptr_t)ptr;
  UPB_ASSERT((num & UPB_DEFTYPE_MASK) == 0);
  num |= type;
  return upb_value_constptr((const void*)num);
}

/* isalpha() etc. from <ctype.h> are locale-dependent, which we don't want. */
static bool upb_isbetween(uint8_t c, uint8_t low, uint8_t high) {
  return c >= low && c <= high;
}

static char upb_ascii_lower(char ch) {
  // Per ASCII this will lower-case a letter.  If the result is a letter, the
  // input was definitely a letter.  If the output is not a letter, this may
  // have transformed the character unpredictably.
  return ch | 0x20;
}

static bool upb_isletter(char c) {
  char lower = upb_ascii_lower(c);
  return upb_isbetween(lower, 'a', 'z') || c == '_';
}

static bool upb_isalphanum(char c) {
  return upb_isletter(c) || upb_isbetween(c, '0', '9');
}

static const char* shortdefname(const char* fullname) {
  const char* p;

  if (fullname == NULL) {
    return NULL;
  } else if ((p = strrchr(fullname, '.')) == NULL) {
    /* No '.' in the name, return the full string. */
    return fullname;
  } else {
    /* Return one past the last '.'. */
    return p + 1;
  }
}

/* All submessage fields are lower than all other fields.
 * Secondly, fields are increasing in order. */
uint32_t field_rank(const upb_FieldDef* f) {
  uint32_t ret = upb_FieldDef_Number(f);
  const uint32_t high_bit = 1 << 30;
  UPB_ASSERT(ret < high_bit);
  if (!upb_FieldDef_IsSubMessage(f)) ret |= high_bit;
  return ret;
}

int cmp_fields(const void* p1, const void* p2) {
  const upb_FieldDef* f1 = *(upb_FieldDef* const*)p1;
  const upb_FieldDef* f2 = *(upb_FieldDef* const*)p2;
  return field_rank(f1) - field_rank(f2);
}

static void upb_Status_setoom(upb_Status* status) {
  upb_Status_SetErrorMessage(status, "out of memory");
}

static void assign_msg_wellknowntype(upb_MessageDef* m) {
  const char* name = upb_MessageDef_FullName(m);
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

/* upb_EnumDef ****************************************************************/

const google_protobuf_EnumOptions* upb_EnumDef_Options(const upb_EnumDef* e) {
  return e->opts;
}

bool upb_EnumDef_HasOptions(const upb_EnumDef* e) {
  return e->opts != (void*)opt_default;
}

const char* upb_EnumDef_FullName(const upb_EnumDef* e) { return e->full_name; }

const char* upb_EnumDef_Name(const upb_EnumDef* e) {
  return shortdefname(e->full_name);
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

const upb_EnumValueDef* upb_EnumDef_FindValueByNameWithSize(
    const upb_EnumDef* def, const char* name, size_t len) {
  upb_value v;
  return upb_strtable_lookup2(&def->ntoi, name, len, &v)
             ? upb_value_getconstptr(v)
             : NULL;
}

const upb_EnumValueDef* upb_EnumDef_FindValueByNumber(const upb_EnumDef* def,
                                                      int32_t num) {
  upb_value v;
  return upb_inttable_lookup(&def->iton, num, &v) ? upb_value_getconstptr(v)
                                                  : NULL;
}

bool upb_EnumDef_CheckNumber(const upb_EnumDef* e, int32_t num) {
  // We could use upb_EnumDef_FindValueByNumber(e, num) != NULL, but we expect
  // this to be faster (especially for small numbers).
  return upb_MiniTable_Enum_CheckValue(e->layout, num);
}

const upb_EnumValueDef* upb_EnumDef_Value(const upb_EnumDef* e, int i) {
  UPB_ASSERT(0 <= i && i < e->value_count);
  return &e->values[i];
}

/* upb_EnumValueDef ***********************************************************/

const google_protobuf_EnumValueOptions* upb_EnumValueDef_Options(
    const upb_EnumValueDef* e) {
  return e->opts;
}

bool upb_EnumValueDef_HasOptions(const upb_EnumValueDef* e) {
  return e->opts != (void*)opt_default;
}

const upb_EnumDef* upb_EnumValueDef_Enum(const upb_EnumValueDef* ev) {
  return ev->parent;
}

const char* upb_EnumValueDef_FullName(const upb_EnumValueDef* ev) {
  return ev->full_name;
}

const char* upb_EnumValueDef_Name(const upb_EnumValueDef* ev) {
  return shortdefname(ev->full_name);
}

int32_t upb_EnumValueDef_Number(const upb_EnumValueDef* ev) {
  return ev->number;
}

uint32_t upb_EnumValueDef_Index(const upb_EnumValueDef* ev) {
  // Compute index in our parent's array.
  return ev - ev->parent->values;
}

/* upb_ExtensionRange
 * ***************************************************************/

const google_protobuf_ExtensionRangeOptions* upb_ExtensionRange_Options(
    const upb_ExtensionRange* r) {
  return r->opts;
}

bool upb_ExtensionRange_HasOptions(const upb_ExtensionRange* r) {
  return r->opts != (void*)opt_default;
}

int32_t upb_ExtensionRange_Start(const upb_ExtensionRange* e) {
  return e->start;
}

int32_t upb_ExtensionRange_End(const upb_ExtensionRange* e) { return e->end; }

/* upb_FieldDef ***************************************************************/

const google_protobuf_FieldOptions* upb_FieldDef_Options(
    const upb_FieldDef* f) {
  return f->opts;
}

bool upb_FieldDef_HasOptions(const upb_FieldDef* f) {
  return f->opts != (void*)opt_default;
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
  return shortdefname(f->full_name);
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
  return &f->msgdef->layout->fields[f->layout_index];
}

const upb_MiniTable_Extension* _upb_FieldDef_ExtensionMiniTable(
    const upb_FieldDef* f) {
  UPB_ASSERT(upb_FieldDef_IsExtension(f));
  return f->file->ext_layouts[f->layout_index];
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
  return upb_FieldDef_IsSubMessage(f) || upb_FieldDef_ContainingOneof(f) ||
         f->file->syntax == kUpb_Syntax_Proto2;
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

/* upb_MessageDef
 * *****************************************************************/

const google_protobuf_MessageOptions* upb_MessageDef_Options(
    const upb_MessageDef* m) {
  return m->opts;
}

bool upb_MessageDef_HasOptions(const upb_MessageDef* m) {
  return m->opts != (void*)opt_default;
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
  return shortdefname(m->full_name);
}

upb_Syntax upb_MessageDef_Syntax(const upb_MessageDef* m) {
  return m->file->syntax;
}

const upb_FieldDef* upb_MessageDef_FindFieldByNumber(const upb_MessageDef* m,
                                                     uint32_t i) {
  upb_value val;
  return upb_inttable_lookup(&m->itof, i, &val) ? upb_value_getconstptr(val)
                                                : NULL;
}

const upb_FieldDef* upb_MessageDef_FindFieldByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t len) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  return unpack_def(val, UPB_DEFTYPE_FIELD);
}

const upb_OneofDef* upb_MessageDef_FindOneofByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t len) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  return unpack_def(val, UPB_DEFTYPE_ONEOF);
}

bool upb_MessageDef_FindByNameWithSize(const upb_MessageDef* m,
                                       const char* name, size_t len,
                                       const upb_FieldDef** out_f,
                                       const upb_OneofDef** out_o) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return false;
  }

  const upb_FieldDef* f = unpack_def(val, UPB_DEFTYPE_FIELD);
  const upb_OneofDef* o = unpack_def(val, UPB_DEFTYPE_ONEOF);
  if (out_f) *out_f = f;
  if (out_o) *out_o = o;
  return f || o; /* False if this was a JSON name. */
}

const upb_FieldDef* upb_MessageDef_FindByJsonNameWithSize(
    const upb_MessageDef* m, const char* name, size_t len) {
  upb_value val;
  const upb_FieldDef* f;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  f = unpack_def(val, UPB_DEFTYPE_FIELD);
  if (!f) f = unpack_def(val, UPB_DEFTYPE_FIELD_JSONNAME);

  return f;
}

int upb_MessageDef_numfields(const upb_MessageDef* m) { return m->field_count; }

int upb_MessageDef_numoneofs(const upb_MessageDef* m) { return m->oneof_count; }

int upb_MessageDef_numrealoneofs(const upb_MessageDef* m) {
  return m->real_oneof_count;
}

int upb_MessageDef_ExtensionRangeCount(const upb_MessageDef* m) {
  return m->ext_range_count;
}

int upb_MessageDef_FieldCount(const upb_MessageDef* m) {
  return m->field_count;
}

int upb_MessageDef_OneofCount(const upb_MessageDef* m) {
  return m->oneof_count;
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

int upb_MessageDef_realoneofcount(const upb_MessageDef* m) {
  return m->real_oneof_count;
}

const upb_MiniTable* upb_MessageDef_MiniTable(const upb_MessageDef* m) {
  return m->layout;
}

const upb_ExtensionRange* upb_MessageDef_ExtensionRange(const upb_MessageDef* m,
                                                        int i) {
  UPB_ASSERT(0 <= i && i < m->ext_range_count);
  return &m->ext_ranges[i];
}

const upb_FieldDef* upb_MessageDef_Field(const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->field_count);
  return &m->fields[i];
}

const upb_OneofDef* upb_MessageDef_Oneof(const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->oneof_count);
  return &m->oneofs[i];
}

const upb_MessageDef* upb_MessageDef_NestedMessage(const upb_MessageDef* m,
                                                   int i) {
  UPB_ASSERT(0 <= i && i < m->nested_msg_count);
  return &m->nested_msgs[i];
}

const upb_EnumDef* upb_MessageDef_NestedEnum(const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->nested_enum_count);
  return &m->nested_enums[i];
}

const upb_FieldDef* upb_MessageDef_NestedExtension(const upb_MessageDef* m,
                                                   int i) {
  UPB_ASSERT(0 <= i && i < m->nested_ext_count);
  return &m->nested_exts[i];
}

upb_WellKnown upb_MessageDef_WellKnownType(const upb_MessageDef* m) {
  return m->well_known_type;
}

/* upb_OneofDef ***************************************************************/

const google_protobuf_OneofOptions* upb_OneofDef_Options(
    const upb_OneofDef* o) {
  return o->opts;
}

bool upb_OneofDef_HasOptions(const upb_OneofDef* o) {
  return o->opts != (void*)opt_default;
}

const char* upb_OneofDef_Name(const upb_OneofDef* o) {
  return shortdefname(o->full_name);
}

const upb_MessageDef* upb_OneofDef_ContainingType(const upb_OneofDef* o) {
  return o->parent;
}

int upb_OneofDef_FieldCount(const upb_OneofDef* o) { return o->field_count; }

const upb_FieldDef* upb_OneofDef_Field(const upb_OneofDef* o, int i) {
  UPB_ASSERT(i < o->field_count);
  return o->fields[i];
}

int upb_OneofDef_numfields(const upb_OneofDef* o) { return o->field_count; }

uint32_t upb_OneofDef_Index(const upb_OneofDef* o) {
  // Compute index in our parent's array.
  return o - o->parent->oneofs;
}

bool upb_OneofDef_IsSynthetic(const upb_OneofDef* o) { return o->synthetic; }

const upb_FieldDef* upb_OneofDef_LookupNameWithSize(const upb_OneofDef* o,
                                                    const char* name,
                                                    size_t length) {
  upb_value val;
  return upb_strtable_lookup2(&o->ntof, name, length, &val)
             ? upb_value_getptr(val)
             : NULL;
}

const upb_FieldDef* upb_OneofDef_LookupNumber(const upb_OneofDef* o,
                                              uint32_t num) {
  upb_value val;
  return upb_inttable_lookup(&o->itof, num, &val) ? upb_value_getptr(val)
                                                  : NULL;
}

/* upb_FileDef ****************************************************************/

const google_protobuf_FileOptions* upb_FileDef_Options(const upb_FileDef* f) {
  return f->opts;
}

bool upb_FileDef_HasOptions(const upb_FileDef* f) {
  return f->opts != (void*)opt_default;
}

const char* upb_FileDef_Name(const upb_FileDef* f) { return f->name; }

const char* upb_FileDef_Package(const upb_FileDef* f) { return f->package; }

upb_Syntax upb_FileDef_Syntax(const upb_FileDef* f) { return f->syntax; }

int upb_FileDef_TopLevelMessageCount(const upb_FileDef* f) {
  return f->top_lvl_msg_count;
}

int upb_FileDef_DependencyCount(const upb_FileDef* f) { return f->dep_count; }

int upb_FileDef_PublicDependencyCount(const upb_FileDef* f) {
  return f->public_dep_count;
}

int upb_FileDef_WeakDependencyCount(const upb_FileDef* f) {
  return f->weak_dep_count;
}

const int32_t* _upb_FileDef_PublicDependencyIndexes(const upb_FileDef* f) {
  return f->public_deps;
}

const int32_t* _upb_FileDef_WeakDependencyIndexes(const upb_FileDef* f) {
  return f->weak_deps;
}

int upb_FileDef_TopLevelEnumCount(const upb_FileDef* f) {
  return f->top_lvl_enum_count;
}

int upb_FileDef_TopLevelExtensionCount(const upb_FileDef* f) {
  return f->top_lvl_ext_count;
}

int upb_FileDef_ServiceCount(const upb_FileDef* f) { return f->service_count; }

const upb_FileDef* upb_FileDef_Dependency(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->dep_count);
  return f->deps[i];
}

const upb_FileDef* upb_FileDef_PublicDependency(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->public_dep_count);
  return f->deps[f->public_deps[i]];
}

const upb_FileDef* upb_FileDef_WeakDependency(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->public_dep_count);
  return f->deps[f->weak_deps[i]];
}

const upb_MessageDef* upb_FileDef_TopLevelMessage(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->top_lvl_msg_count);
  return &f->top_lvl_msgs[i];
}

const upb_EnumDef* upb_FileDef_TopLevelEnum(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->top_lvl_enum_count);
  return &f->top_lvl_enums[i];
}

const upb_FieldDef* upb_FileDef_TopLevelExtension(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->top_lvl_ext_count);
  return &f->top_lvl_exts[i];
}

const upb_ServiceDef* upb_FileDef_Service(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->service_count);
  return &f->services[i];
}

const upb_DefPool* upb_FileDef_Pool(const upb_FileDef* f) { return f->symtab; }

/* upb_MethodDef **************************************************************/

const google_protobuf_MethodOptions* upb_MethodDef_Options(
    const upb_MethodDef* m) {
  return m->opts;
}

bool upb_MethodDef_HasOptions(const upb_MethodDef* m) {
  return m->opts != (void*)opt_default;
}

const char* upb_MethodDef_FullName(const upb_MethodDef* m) {
  return m->full_name;
}

int upb_MethodDef_Index(const upb_MethodDef* m) { return m->index; }

const char* upb_MethodDef_Name(const upb_MethodDef* m) {
  return shortdefname(m->full_name);
}

const upb_ServiceDef* upb_MethodDef_Service(const upb_MethodDef* m) {
  return m->service;
}

const upb_MessageDef* upb_MethodDef_InputType(const upb_MethodDef* m) {
  return m->input_type;
}

const upb_MessageDef* upb_MethodDef_OutputType(const upb_MethodDef* m) {
  return m->output_type;
}

bool upb_MethodDef_ClientStreaming(const upb_MethodDef* m) {
  return m->client_streaming;
}

bool upb_MethodDef_ServerStreaming(const upb_MethodDef* m) {
  return m->server_streaming;
}

/* upb_ServiceDef *************************************************************/

const google_protobuf_ServiceOptions* upb_ServiceDef_Options(
    const upb_ServiceDef* s) {
  return s->opts;
}

bool upb_ServiceDef_HasOptions(const upb_ServiceDef* s) {
  return s->opts != (void*)opt_default;
}

const char* upb_ServiceDef_FullName(const upb_ServiceDef* s) {
  return s->full_name;
}

const char* upb_ServiceDef_Name(const upb_ServiceDef* s) {
  return shortdefname(s->full_name);
}

int upb_ServiceDef_Index(const upb_ServiceDef* s) { return s->index; }

const upb_FileDef* upb_ServiceDef_File(const upb_ServiceDef* s) {
  return s->file;
}

int upb_ServiceDef_MethodCount(const upb_ServiceDef* s) {
  return s->method_count;
}

const upb_MethodDef* upb_ServiceDef_Method(const upb_ServiceDef* s, int i) {
  return i < 0 || i >= s->method_count ? NULL : &s->methods[i];
}

const upb_MethodDef* upb_ServiceDef_FindMethodByName(const upb_ServiceDef* s,
                                                     const char* name) {
  for (int i = 0; i < s->method_count; i++) {
    if (strcmp(name, upb_MethodDef_Name(&s->methods[i])) == 0) {
      return &s->methods[i];
    }
  }
  return NULL;
}

/* upb_DefPool ****************************************************************/

void upb_DefPool_Free(upb_DefPool* s) {
  upb_Arena_Free(s->arena);
  upb_gfree(s);
}

upb_DefPool* upb_DefPool_New(void) {
  upb_DefPool* s = upb_gmalloc(sizeof(*s));

  if (!s) {
    return NULL;
  }

  s->arena = upb_Arena_New();
  s->bytes_loaded = 0;

  if (!upb_strtable_init(&s->syms, 32, s->arena) ||
      !upb_strtable_init(&s->files, 4, s->arena) ||
      !upb_inttable_init(&s->exts, s->arena)) {
    goto err;
  }

  s->extreg = upb_ExtensionRegistry_New(s->arena);
  if (!s->extreg) goto err;
  return s;

err:
  upb_Arena_Free(s->arena);
  upb_gfree(s);
  return NULL;
}

static const void* symtab_lookup(const upb_DefPool* s, const char* sym,
                                 upb_deftype_t type) {
  upb_value v;
  return upb_strtable_lookup(&s->syms, sym, &v) ? unpack_def(v, type) : NULL;
}

static const void* symtab_lookup2(const upb_DefPool* s, const char* sym,
                                  size_t size, upb_deftype_t type) {
  upb_value v;
  return upb_strtable_lookup2(&s->syms, sym, size, &v) ? unpack_def(v, type)
                                                       : NULL;
}

const upb_MessageDef* upb_DefPool_FindMessageByName(const upb_DefPool* s,
                                                    const char* sym) {
  return symtab_lookup(s, sym, UPB_DEFTYPE_MSG);
}

const upb_MessageDef* upb_DefPool_FindMessageByNameWithSize(
    const upb_DefPool* s, const char* sym, size_t len) {
  return symtab_lookup2(s, sym, len, UPB_DEFTYPE_MSG);
}

const upb_EnumDef* upb_DefPool_FindEnumByName(const upb_DefPool* s,
                                              const char* sym) {
  return symtab_lookup(s, sym, UPB_DEFTYPE_ENUM);
}

const upb_EnumValueDef* upb_DefPool_FindEnumByNameval(const upb_DefPool* s,
                                                      const char* sym) {
  return symtab_lookup(s, sym, UPB_DEFTYPE_ENUMVAL);
}

const upb_FileDef* upb_DefPool_FindFileByName(const upb_DefPool* s,
                                              const char* name) {
  upb_value v;
  return upb_strtable_lookup(&s->files, name, &v)
             ? unpack_def(v, UPB_DEFTYPE_FILE)
             : NULL;
}

const upb_FileDef* upb_DefPool_FindFileByNameWithSize(const upb_DefPool* s,
                                                      const char* name,
                                                      size_t len) {
  upb_value v;
  return upb_strtable_lookup2(&s->files, name, len, &v)
             ? unpack_def(v, UPB_DEFTYPE_FILE)
             : NULL;
}

const upb_FieldDef* upb_DefPool_FindExtensionByNameWithSize(
    const upb_DefPool* s, const char* name, size_t size) {
  upb_value v;
  if (!upb_strtable_lookup2(&s->syms, name, size, &v)) return NULL;

  switch (deftype(v)) {
    case UPB_DEFTYPE_FIELD:
      return unpack_def(v, UPB_DEFTYPE_FIELD);
    case UPB_DEFTYPE_MSG: {
      const upb_MessageDef* m = unpack_def(v, UPB_DEFTYPE_MSG);
      return m->in_message_set ? &m->nested_exts[0] : NULL;
    }
    default:
      break;
  }

  return NULL;
}

const upb_FieldDef* upb_DefPool_FindExtensionByName(const upb_DefPool* s,
                                                    const char* sym) {
  return upb_DefPool_FindExtensionByNameWithSize(s, sym, strlen(sym));
}

const upb_ServiceDef* upb_DefPool_FindServiceByName(const upb_DefPool* s,
                                                    const char* name) {
  return symtab_lookup(s, name, UPB_DEFTYPE_SERVICE);
}

const upb_ServiceDef* upb_DefPool_FindServiceByNameWithSize(
    const upb_DefPool* s, const char* name, size_t size) {
  return symtab_lookup2(s, name, size, UPB_DEFTYPE_SERVICE);
}

const upb_FileDef* upb_DefPool_FindFileContainingSymbol(const upb_DefPool* s,
                                                        const char* name) {
  upb_value v;
  // TODO(haberman): non-extension fields and oneofs.
  if (upb_strtable_lookup(&s->syms, name, &v)) {
    switch (deftype(v)) {
      case UPB_DEFTYPE_EXT: {
        const upb_FieldDef* f = unpack_def(v, UPB_DEFTYPE_EXT);
        return upb_FieldDef_File(f);
      }
      case UPB_DEFTYPE_MSG: {
        const upb_MessageDef* m = unpack_def(v, UPB_DEFTYPE_MSG);
        return upb_MessageDef_File(m);
      }
      case UPB_DEFTYPE_ENUM: {
        const upb_EnumDef* e = unpack_def(v, UPB_DEFTYPE_ENUM);
        return upb_EnumDef_File(e);
      }
      case UPB_DEFTYPE_ENUMVAL: {
        const upb_EnumValueDef* ev = unpack_def(v, UPB_DEFTYPE_ENUMVAL);
        return upb_EnumDef_File(upb_EnumValueDef_Enum(ev));
      }
      case UPB_DEFTYPE_SERVICE: {
        const upb_ServiceDef* service = unpack_def(v, UPB_DEFTYPE_SERVICE);
        return upb_ServiceDef_File(service);
      }
      default:
        UPB_UNREACHABLE();
    }
  }

  const char* last_dot = strrchr(name, '.');
  if (last_dot) {
    const upb_MessageDef* parent =
        upb_DefPool_FindMessageByNameWithSize(s, name, last_dot - name);
    if (parent) {
      const char* shortname = last_dot + 1;
      if (upb_MessageDef_FindByNameWithSize(parent, shortname,
                                            strlen(shortname), NULL, NULL)) {
        return upb_MessageDef_File(parent);
      }
    }
  }

  return NULL;
}

/* Code to build defs from descriptor protos. *********************************/

/* There is a question of how much validation to do here.  It will be difficult
 * to perfectly match the amount of validation performed by proto2.  But since
 * this code is used to directly build defs from Ruby (for example) we do need
 * to validate important constraints like uniqueness of names and numbers. */

#define CHK_OOM(x)      \
  if (!(x)) {           \
    symtab_oomerr(ctx); \
  }

typedef struct {
  upb_DefPool* symtab;
  upb_FileDef* file;                /* File we are building. */
  upb_Arena* arena;                 /* Allocate defs here. */
  upb_Arena* tmp_arena;             /* For temporary allocations. */
  const upb_MiniTable_File* layout; /* NULL if we should build layouts. */
  int enum_count;                   /* Count of enums built so far. */
  int msg_count;                    /* Count of messages built so far. */
  int ext_count;                    /* Count of extensions built so far. */
  upb_Status* status;               /* Record errors here. */
  jmp_buf err;                      /* longjmp() on error. */
} symtab_addctx;

UPB_NORETURN UPB_NOINLINE UPB_PRINTF(2, 3) static void symtab_errf(
    symtab_addctx* ctx, const char* fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  upb_Status_VSetErrorFormat(ctx->status, fmt, argp);
  va_end(argp);
  UPB_LONGJMP(ctx->err, 1);
}

UPB_NORETURN UPB_NOINLINE static void symtab_oomerr(symtab_addctx* ctx) {
  upb_Status_setoom(ctx->status);
  UPB_LONGJMP(ctx->err, 1);
}

void* symtab_alloc(symtab_addctx* ctx, size_t bytes) {
  if (bytes == 0) return NULL;
  void* ret = upb_Arena_Malloc(ctx->arena, bytes);
  if (!ret) symtab_oomerr(ctx);
  return ret;
}

// We want to copy the options verbatim into the destination options proto.
// We use serialize+parse as our deep copy.
#define SET_OPTIONS(target, desc_type, options_type, proto)                   \
  if (google_protobuf_##desc_type##_has_options(proto)) {                     \
    size_t size;                                                              \
    char* pb = google_protobuf_##options_type##_serialize(                    \
        google_protobuf_##desc_type##_options(proto), ctx->tmp_arena, &size); \
    CHK_OOM(pb);                                                              \
    target = google_protobuf_##options_type##_parse(pb, size, ctx->arena);    \
    CHK_OOM(target);                                                          \
  } else {                                                                    \
    target = (const google_protobuf_##options_type*)opt_default;              \
  }

static void check_ident(symtab_addctx* ctx, upb_StringView name, bool full) {
  const char* str = name.data;
  size_t len = name.size;
  bool start = true;
  size_t i;
  for (i = 0; i < len; i++) {
    char c = str[i];
    if (c == '.') {
      if (start || !full) {
        symtab_errf(ctx, "invalid name: unexpected '.' (%.*s)", (int)len, str);
      }
      start = true;
    } else if (start) {
      if (!upb_isletter(c)) {
        symtab_errf(
            ctx,
            "invalid name: path components must start with a letter (%.*s)",
            (int)len, str);
      }
      start = false;
    } else {
      if (!upb_isalphanum(c)) {
        symtab_errf(ctx, "invalid name: non-alphanumeric character (%.*s)",
                    (int)len, str);
      }
    }
  }
  if (start) {
    symtab_errf(ctx, "invalid name: empty part (%.*s)", (int)len, str);
  }
}

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

static uint32_t upb_MiniTable_place(symtab_addctx* ctx, upb_MiniTable* l,
                                    size_t size, const upb_MessageDef* m) {
  size_t ofs = UPB_ALIGN_UP(l->size, size);
  size_t next = ofs + size;

  if (next > UINT16_MAX) {
    symtab_errf(ctx, "size of message %s exceeded max size of %zu bytes",
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
  int n = upb_MessageDef_numfields(m);
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
  if (type == kUpb_FieldType_String && f->file->syntax == kUpb_Syntax_Proto2) {
    return kUpb_FieldType_Bytes;
  } else if (type == kUpb_FieldType_Enum &&
             (f->sub.enumdef->file->syntax == kUpb_Syntax_Proto3 ||
              UPB_TREAT_PROTO2_ENUMS_LIKE_PROTO3 ||
              // TODO(https://github.com/protocolbuffers/upb/issues/541):
              // fix map enum values to check for unknown enum values and put
              // them in the unknown field set.
              upb_MessageDef_IsMapEntry(upb_FieldDef_ContainingType(f)))) {
    return kUpb_FieldType_Int32;
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
static void make_layout(symtab_addctx* ctx, const upb_MessageDef* m) {
  upb_MiniTable* l = (upb_MiniTable*)m->layout;
  size_t field_count = upb_MessageDef_numfields(m);
  size_t sublayout_count = 0;
  upb_MiniTable_Sub* subs;
  upb_MiniTable_Field* fields;

  memset(l, 0, sizeof(*l) + sizeof(_upb_FastTable_Entry));

  /* Count sub-messages. */
  for (size_t i = 0; i < field_count; i++) {
    const upb_FieldDef* f = &m->fields[i];
    if (upb_FieldDef_IsSubMessage(f)) {
      sublayout_count++;
    }
    if (upb_FieldDef_CType(f) == kUpb_CType_Enum &&
        f->sub.enumdef->file->syntax == kUpb_Syntax_Proto2) {
      sublayout_count++;
    }
  }

  fields = symtab_alloc(ctx, field_count * sizeof(*fields));
  subs = symtab_alloc(ctx, sublayout_count * sizeof(*subs));

  l->field_count = upb_MessageDef_numfields(m);
  l->fields = fields;
  l->subs = subs;
  l->table_mask = 0;
  l->required_count = 0;

  if (upb_MessageDef_ExtensionRangeCount(m) > 0) {
    if (google_protobuf_MessageOptions_message_set_wire_format(m->opts)) {
      l->ext = kUpb_ExtMode_IsMessageSet;
    } else {
      l->ext = kUpb_ExtMode_Extendable;
    }
  } else {
    l->ext = kUpb_ExtMode_NonExtendable;
  }

  /* TODO(haberman): initialize fast tables so that reflection-based parsing
   * can get the same speeds as linked-in types. */
  l->fasttable[0].field_parser = &fastdecode_generic;
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
      subs[0].submsg = upb_FieldDef_MessageSubDef(val)->layout;
    }

    upb_FieldDef* fielddefs = (upb_FieldDef*)&m->fields[0];
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

  for (int i = 0; i < m->field_count; i++) {
    const upb_FieldDef* f = &m->fields[i];
    upb_MiniTable_Field* field = &fields[upb_FieldDef_Index(f)];
    if (upb_FieldDef_Label(f) == kUpb_Label_Required) {
      field->presence = ++hasbit;
      if (hasbit >= 63) {
        symtab_errf(ctx, "Message with >=63 required fields: %s",
                    upb_MessageDef_FullName(m));
      }
      l->required_count++;
    }
  }

  /* Allocate hasbits and set basic field attributes. */
  sublayout_count = 0;
  for (int i = 0; i < m->field_count; i++) {
    const upb_FieldDef* f = &m->fields[i];
    upb_MiniTable_Field* field = &fields[upb_FieldDef_Index(f)];

    fill_fieldlayout(field, f);

    if (field->descriptortype == kUpb_FieldType_Message ||
        field->descriptortype == kUpb_FieldType_Group) {
      field->submsg_index = sublayout_count++;
      subs[field->submsg_index].submsg = upb_FieldDef_MessageSubDef(f)->layout;
    } else if (field->descriptortype == kUpb_FieldType_Enum) {
      field->submsg_index = sublayout_count++;
      subs[field->submsg_index].subenum = upb_FieldDef_EnumSubDef(f)->layout;
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
  for (int i = 0; i < m->field_count; i++) {
    const upb_FieldDef* f = &m->fields[i];
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
  for (int i = 0; i < m->oneof_count; i++) {
    const upb_OneofDef* o = &m->oneofs[i];
    size_t case_size = sizeof(uint32_t); /* Could potentially optimize this. */
    size_t field_size = 0;
    uint32_t case_offset;
    uint32_t data_offset;

    if (upb_OneofDef_IsSynthetic(o)) continue;

    if (o->field_count == 0) {
      symtab_errf(ctx, "Oneof must have at least one field (%s)", o->full_name);
    }

    /* Calculate field size: the max of all field sizes. */
    for (int j = 0; j < o->field_count; j++) {
      const upb_FieldDef* f = o->fields[j];
      field_size = UPB_MAX(field_size, upb_msg_fielddefsize(f));
    }

    /* Align and allocate case offset. */
    case_offset = upb_MiniTable_place(ctx, l, case_size, m);
    data_offset = upb_MiniTable_place(ctx, l, field_size, m);

    for (int i = 0; i < o->field_count; i++) {
      const upb_FieldDef* f = o->fields[i];
      fields[upb_FieldDef_Index(f)].offset = data_offset;
      fields[upb_FieldDef_Index(f)].presence = ~case_offset;
    }
  }

  /* Size of the entire structure should be a multiple of its greatest
   * alignment.  TODO: track overall alignment for real? */
  l->size = UPB_ALIGN_UP(l->size, 8);

  /* Sort fields by number. */
  if (fields) {
    qsort(fields, upb_MessageDef_numfields(m), sizeof(*fields),
          field_number_cmp);
  }
  assign_layout_indices(m, l, fields);
}

static char* strviewdup(symtab_addctx* ctx, upb_StringView view) {
  char* ret = upb_strdup2(view.data, view.size, ctx->arena);
  CHK_OOM(ret);
  return ret;
}

static bool streql2(const char* a, size_t n, const char* b) {
  return n == strlen(b) && memcmp(a, b, n) == 0;
}

static bool streql_view(upb_StringView view, const char* b) {
  return streql2(view.data, view.size, b);
}

static const char* makefullname(symtab_addctx* ctx, const char* prefix,
                                upb_StringView name) {
  if (prefix) {
    /* ret = prefix + '.' + name; */
    size_t n = strlen(prefix);
    char* ret = symtab_alloc(ctx, n + name.size + 2);
    strcpy(ret, prefix);
    ret[n] = '.';
    memcpy(&ret[n + 1], name.data, name.size);
    ret[n + 1 + name.size] = '\0';
    return ret;
  } else {
    return strviewdup(ctx, name);
  }
}

static void finalize_oneofs(symtab_addctx* ctx, upb_MessageDef* m) {
  int i;
  int synthetic_count = 0;
  upb_OneofDef* mutable_oneofs = (upb_OneofDef*)m->oneofs;

  for (i = 0; i < m->oneof_count; i++) {
    upb_OneofDef* o = &mutable_oneofs[i];

    if (o->synthetic && o->field_count != 1) {
      symtab_errf(ctx, "Synthetic oneofs must have one field, not %d: %s",
                  o->field_count, upb_OneofDef_Name(o));
    }

    if (o->synthetic) {
      synthetic_count++;
    } else if (synthetic_count != 0) {
      symtab_errf(ctx, "Synthetic oneofs must be after all other oneofs: %s",
                  upb_OneofDef_Name(o));
    }

    o->fields = symtab_alloc(ctx, sizeof(upb_FieldDef*) * o->field_count);
    o->field_count = 0;
  }

  for (i = 0; i < m->field_count; i++) {
    const upb_FieldDef* f = &m->fields[i];
    upb_OneofDef* o = (upb_OneofDef*)upb_FieldDef_ContainingOneof(f);
    if (o) {
      o->fields[o->field_count++] = f;
    }
  }

  m->real_oneof_count = m->oneof_count - synthetic_count;
}

size_t getjsonname(const char* name, char* buf, size_t len) {
  size_t src, dst = 0;
  bool ucase_next = false;

#define WRITE(byte)      \
  ++dst;                 \
  if (dst < len)         \
    buf[dst - 1] = byte; \
  else if (dst == len)   \
  buf[dst - 1] = '\0'

  if (!name) {
    WRITE('\0');
    return 0;
  }

  /* Implement the transformation as described in the spec:
   *   1. upper case all letters after an underscore.
   *   2. remove all underscores.
   */
  for (src = 0; name[src]; src++) {
    if (name[src] == '_') {
      ucase_next = true;
      continue;
    }

    if (ucase_next) {
      WRITE(toupper(name[src]));
      ucase_next = false;
    } else {
      WRITE(name[src]);
    }
  }

  WRITE('\0');
  return dst;

#undef WRITE
}

static char* makejsonname(symtab_addctx* ctx, const char* name) {
  size_t size = getjsonname(name, NULL, 0);
  char* json_name = symtab_alloc(ctx, size);
  getjsonname(name, json_name, size);
  return json_name;
}

/* Adds a symbol |v| to the symtab, which must be a def pointer previously
 * packed with pack_def().  The def's pointer to upb_FileDef* must be set before
 * adding, so we know which entries to remove if building this file fails. */
static void symtab_add(symtab_addctx* ctx, const char* name, upb_value v) {
  // TODO: table should support an operation "tryinsert" to avoid the double
  // lookup.
  if (upb_strtable_lookup(&ctx->symtab->syms, name, NULL)) {
    symtab_errf(ctx, "duplicate symbol '%s'", name);
  }
  size_t len = strlen(name);
  CHK_OOM(upb_strtable_insert(&ctx->symtab->syms, name, len, v,
                              ctx->symtab->arena));
}

static bool remove_component(char* base, size_t* len) {
  if (*len == 0) return false;

  for (size_t i = *len - 1; i > 0; i--) {
    if (base[i] == '.') {
      *len = i;
      return true;
    }
  }

  *len = 0;
  return true;
}

/* Given a symbol and the base symbol inside which it is defined, find the
 * symbol's definition in t. */
static const void* symtab_resolveany(symtab_addctx* ctx,
                                     const char* from_name_dbg,
                                     const char* base, upb_StringView sym,
                                     upb_deftype_t* type) {
  const upb_strtable* t = &ctx->symtab->syms;
  if (sym.size == 0) goto notfound;
  upb_value v;
  if (sym.data[0] == '.') {
    /* Symbols starting with '.' are absolute, so we do a single lookup.
     * Slice to omit the leading '.' */
    if (!upb_strtable_lookup2(t, sym.data + 1, sym.size - 1, &v)) {
      goto notfound;
    }
  } else {
    /* Remove components from base until we find an entry or run out. */
    size_t baselen = base ? strlen(base) : 0;
    char* tmp = malloc(sym.size + baselen + 1);
    while (1) {
      char* p = tmp;
      if (baselen) {
        memcpy(p, base, baselen);
        p[baselen] = '.';
        p += baselen + 1;
      }
      memcpy(p, sym.data, sym.size);
      p += sym.size;
      if (upb_strtable_lookup2(t, tmp, p - tmp, &v)) {
        break;
      }
      if (!remove_component(tmp, &baselen)) {
        free(tmp);
        goto notfound;
      }
    }
    free(tmp);
  }

  *type = deftype(v);
  return unpack_def(v, *type);

notfound:
  symtab_errf(ctx, "couldn't resolve name '" UPB_STRINGVIEW_FORMAT "'",
              UPB_STRINGVIEW_ARGS(sym));
}

static const void* symtab_resolve(symtab_addctx* ctx, const char* from_name_dbg,
                                  const char* base, upb_StringView sym,
                                  upb_deftype_t type) {
  upb_deftype_t found_type;
  const void* ret =
      symtab_resolveany(ctx, from_name_dbg, base, sym, &found_type);
  if (ret && found_type != type) {
    symtab_errf(ctx,
                "type mismatch when resolving %s: couldn't find "
                "name " UPB_STRINGVIEW_FORMAT " with type=%d",
                from_name_dbg, UPB_STRINGVIEW_ARGS(sym), (int)type);
  }
  return ret;
}

static void create_oneofdef(
    symtab_addctx* ctx, upb_MessageDef* m,
    const google_protobuf_OneofDescriptorProto* oneof_proto,
    const upb_OneofDef* _o) {
  upb_OneofDef* o = (upb_OneofDef*)_o;
  upb_StringView name = google_protobuf_OneofDescriptorProto_name(oneof_proto);
  upb_value v;

  o->parent = m;
  o->full_name = makefullname(ctx, m->full_name, name);
  o->field_count = 0;
  o->synthetic = false;

  SET_OPTIONS(o->opts, OneofDescriptorProto, OneofOptions, oneof_proto);

  upb_value existing_v;
  if (upb_strtable_lookup2(&m->ntof, name.data, name.size, &existing_v)) {
    symtab_errf(ctx, "duplicate oneof name (%s)", o->full_name);
  }

  v = pack_def(o, UPB_DEFTYPE_ONEOF);
  CHK_OOM(upb_strtable_insert(&m->ntof, name.data, name.size, v, ctx->arena));

  CHK_OOM(upb_inttable_init(&o->itof, ctx->arena));
  CHK_OOM(upb_strtable_init(&o->ntof, 4, ctx->arena));
}

static str_t* newstr(symtab_addctx* ctx, const char* data, size_t len) {
  str_t* ret = symtab_alloc(ctx, sizeof(*ret) + len);
  CHK_OOM(ret);
  ret->len = len;
  if (len) memcpy(ret->str, data, len);
  ret->str[len] = '\0';
  return ret;
}

static bool upb_DefPool_TryGetChar(const char** src, const char* end,
                                   char* ch) {
  if (*src == end) return false;
  *ch = **src;
  *src += 1;
  return true;
}

static char upb_DefPool_TryGetHexDigit(symtab_addctx* ctx,
                                       const upb_FieldDef* f, const char** src,
                                       const char* end) {
  char ch;
  if (!upb_DefPool_TryGetChar(src, end, &ch)) return -1;
  if ('0' <= ch && ch <= '9') {
    return ch - '0';
  }
  ch = upb_ascii_lower(ch);
  if ('a' <= ch && ch <= 'f') {
    return ch - 'a' + 0xa;
  }
  *src -= 1;  // Char wasn't actually a hex digit.
  return -1;
}

static char upb_DefPool_ParseHexEscape(symtab_addctx* ctx,
                                       const upb_FieldDef* f, const char** src,
                                       const char* end) {
  char hex_digit = upb_DefPool_TryGetHexDigit(ctx, f, src, end);
  if (hex_digit < 0) {
    symtab_errf(ctx,
                "\\x cannot be followed by non-hex digit in field '%s' default",
                upb_FieldDef_FullName(f));
    return 0;
  }
  unsigned int ret = hex_digit;
  while ((hex_digit = upb_DefPool_TryGetHexDigit(ctx, f, src, end)) >= 0) {
    ret = (ret << 4) | hex_digit;
  }
  if (ret > 0xff) {
    symtab_errf(ctx, "Value of hex escape in field %s exceeds 8 bits",
                upb_FieldDef_FullName(f));
    return 0;
  }
  return ret;
}

char upb_DefPool_TryGetOctalDigit(const char** src, const char* end) {
  char ch;
  if (!upb_DefPool_TryGetChar(src, end, &ch)) return -1;
  if ('0' <= ch && ch <= '7') {
    return ch - '0';
  }
  *src -= 1;  // Char wasn't actually an octal digit.
  return -1;
}

static char upb_DefPool_ParseOctalEscape(symtab_addctx* ctx,
                                         const upb_FieldDef* f,
                                         const char** src, const char* end) {
  char ch = 0;
  for (int i = 0; i < 3; i++) {
    char digit;
    if ((digit = upb_DefPool_TryGetOctalDigit(src, end)) >= 0) {
      ch = (ch << 3) | digit;
    }
  }
  return ch;
}

static char upb_DefPool_ParseEscape(symtab_addctx* ctx, const upb_FieldDef* f,
                                    const char** src, const char* end) {
  char ch;
  if (!upb_DefPool_TryGetChar(src, end, &ch)) {
    symtab_errf(ctx, "unterminated escape sequence in field %s",
                upb_FieldDef_FullName(f));
    return 0;
  }
  switch (ch) {
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case 'v':
      return '\v';
    case '\\':
      return '\\';
    case '\'':
      return '\'';
    case '\"':
      return '\"';
    case '?':
      return '\?';
    case 'x':
    case 'X':
      return upb_DefPool_ParseHexEscape(ctx, f, src, end);
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
      *src -= 1;
      return upb_DefPool_ParseOctalEscape(ctx, f, src, end);
  }
  symtab_errf(ctx, "Unknown escape sequence: \\%c", ch);
}

static str_t* unescape(symtab_addctx* ctx, const upb_FieldDef* f,
                       const char* data, size_t len) {
  // Size here is an upper bound; escape sequences could ultimately shrink it.
  str_t* ret = symtab_alloc(ctx, sizeof(*ret) + len);
  char* dst = &ret->str[0];
  const char* src = data;
  const char* end = data + len;

  while (src < end) {
    if (*src == '\\') {
      src++;
      *dst++ = upb_DefPool_ParseEscape(ctx, f, &src, end);
    } else {
      *dst++ = *src++;
    }
  }

  ret->len = dst - &ret->str[0];
  return ret;
}

static void parse_default(symtab_addctx* ctx, const char* str, size_t len,
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
        symtab_errf(ctx, "Default too long: %.*s", (int)len, str);
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
      f->defaultval.sint = ev->number;
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
      symtab_errf(ctx, "Message should not have a default (%s)",
                  upb_FieldDef_FullName(f));
  }

  return;

invalid:
  symtab_errf(ctx, "Invalid default '%.*s' for field %s of type %d", (int)len,
              str, upb_FieldDef_FullName(f), (int)upb_FieldDef_Type(f));
}

static void set_default_default(symtab_addctx* ctx, upb_FieldDef* f) {
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
    case kUpb_CType_Enum:
      f->defaultval.sint = f->sub.enumdef->values[0].number;
    case kUpb_CType_Message:
      break;
  }
}

static void create_fielddef(
    symtab_addctx* ctx, const char* prefix, upb_MessageDef* m,
    const google_protobuf_FieldDescriptorProto* field_proto,
    const upb_FieldDef* _f, bool is_extension) {
  upb_FieldDef* f = (upb_FieldDef*)_f;
  upb_StringView name;
  const char* full_name;
  const char* json_name;
  const char* shortname;
  int32_t field_number;

  f->file = ctx->file; /* Must happen prior to symtab_add(). */

  if (!google_protobuf_FieldDescriptorProto_has_name(field_proto)) {
    symtab_errf(ctx, "field has no name");
  }

  name = google_protobuf_FieldDescriptorProto_name(field_proto);
  check_ident(ctx, name, false);
  full_name = makefullname(ctx, prefix, name);
  shortname = shortdefname(full_name);

  if (google_protobuf_FieldDescriptorProto_has_json_name(field_proto)) {
    json_name = strviewdup(
        ctx, google_protobuf_FieldDescriptorProto_json_name(field_proto));
    f->has_json_name_ = true;
  } else {
    json_name = makejsonname(ctx, shortname);
    f->has_json_name_ = false;
  }

  field_number = google_protobuf_FieldDescriptorProto_number(field_proto);

  f->full_name = full_name;
  f->json_name = json_name;
  f->label_ = (int)google_protobuf_FieldDescriptorProto_label(field_proto);
  f->number_ = field_number;
  f->scope.oneof = NULL;
  f->proto3_optional_ =
      google_protobuf_FieldDescriptorProto_proto3_optional(field_proto);

  bool has_type = google_protobuf_FieldDescriptorProto_has_type(field_proto);
  bool has_type_name =
      google_protobuf_FieldDescriptorProto_has_type_name(field_proto);

  f->type_ = (int)google_protobuf_FieldDescriptorProto_type(field_proto);

  if (has_type) {
    switch (f->type_) {
      case kUpb_FieldType_Message:
      case kUpb_FieldType_Group:
      case kUpb_FieldType_Enum:
        if (!has_type_name) {
          symtab_errf(ctx, "field of type %d requires type name (%s)",
                      (int)f->type_, full_name);
        }
        break;
      default:
        if (has_type_name) {
          symtab_errf(ctx, "invalid type for field with type_name set (%s, %d)",
                      full_name, (int)f->type_);
        }
    }
  } else if (has_type_name) {
    f->type_ =
        FIELD_TYPE_UNSPECIFIED;  // We'll fill this in in resolve_fielddef().
  }

  if (!is_extension) {
    /* direct message field. */
    upb_value v, field_v, json_v, existing_v;
    size_t json_size;

    if (field_number <= 0 || field_number > kUpb_MaxFieldNumber) {
      symtab_errf(ctx, "invalid field number (%u)", field_number);
    }

    f->index_ = f - m->fields;
    f->msgdef = m;
    f->is_extension_ = false;

    field_v = pack_def(f, UPB_DEFTYPE_FIELD);
    json_v = pack_def(f, UPB_DEFTYPE_FIELD_JSONNAME);
    v = upb_value_constptr(f);
    json_size = strlen(json_name);

    if (upb_strtable_lookup(&m->ntof, shortname, &existing_v)) {
      symtab_errf(ctx, "duplicate field name (%s)", shortname);
    }

    CHK_OOM(upb_strtable_insert(&m->ntof, name.data, name.size, field_v,
                                ctx->arena));

    if (strcmp(shortname, json_name) != 0) {
      if (upb_strtable_lookup(&m->ntof, json_name, &v)) {
        symtab_errf(ctx, "duplicate json_name (%s)", json_name);
      } else {
        CHK_OOM(upb_strtable_insert(&m->ntof, json_name, json_size, json_v,
                                    ctx->arena));
      }
    }

    if (upb_inttable_lookup(&m->itof, field_number, NULL)) {
      symtab_errf(ctx, "duplicate field number (%u)", field_number);
    }

    CHK_OOM(upb_inttable_insert(&m->itof, field_number, v, ctx->arena));

    if (ctx->layout) {
      const upb_MiniTable_Field* fields = m->layout->fields;
      int count = m->layout->field_count;
      bool found = false;
      for (int i = 0; i < count; i++) {
        if (fields[i].number == field_number) {
          f->layout_index = i;
          found = true;
          break;
        }
      }
      UPB_ASSERT(found);
    }
  } else {
    /* extension field. */
    f->is_extension_ = true;
    f->scope.extension_scope = m;
    symtab_add(ctx, full_name, pack_def(f, UPB_DEFTYPE_EXT));
    f->layout_index = ctx->ext_count++;
    if (ctx->layout) {
      UPB_ASSERT(ctx->file->ext_layouts[f->layout_index]->field.number ==
                 field_number);
    }
  }

  if (f->type_ < kUpb_FieldType_Double || f->type_ > kUpb_FieldType_SInt64) {
    symtab_errf(ctx, "invalid type for field %s (%d)", f->full_name, f->type_);
  }

  if (f->label_ < kUpb_Label_Optional || f->label_ > kUpb_Label_Repeated) {
    symtab_errf(ctx, "invalid label for field %s (%d)", f->full_name,
                f->label_);
  }

  /* We can't resolve the subdef or (in the case of extensions) the containing
   * message yet, because it may not have been defined yet.  We stash a pointer
   * to the field_proto until later when we can properly resolve it. */
  f->sub.unresolved = field_proto;

  if (f->label_ == kUpb_Label_Required &&
      f->file->syntax == kUpb_Syntax_Proto3) {
    symtab_errf(ctx, "proto3 fields cannot be required (%s)", f->full_name);
  }

  if (google_protobuf_FieldDescriptorProto_has_oneof_index(field_proto)) {
    uint32_t oneof_index = google_protobuf_FieldDescriptorProto_oneof_index(field_proto);
    upb_OneofDef* oneof;
    upb_value v = upb_value_constptr(f);

    if (upb_FieldDef_Label(f) != kUpb_Label_Optional) {
      symtab_errf(ctx, "fields in oneof must have OPTIONAL label (%s)",
                  f->full_name);
    }

    if (!m) {
      symtab_errf(ctx, "oneof_index provided for extension field (%s)",
                  f->full_name);
    }

    if (oneof_index >= m->oneof_count) {
      symtab_errf(ctx, "oneof_index out of range (%s)", f->full_name);
    }

    oneof = (upb_OneofDef*)&m->oneofs[oneof_index];
    f->scope.oneof = oneof;

    oneof->field_count++;
    if (f->proto3_optional_) {
      oneof->synthetic = true;
    }
    CHK_OOM(upb_inttable_insert(&oneof->itof, f->number_, v, ctx->arena));
    CHK_OOM(
        upb_strtable_insert(&oneof->ntof, name.data, name.size, v, ctx->arena));
  } else {
    if (f->proto3_optional_) {
      symtab_errf(ctx, "field with proto3_optional was not in a oneof (%s)",
                  f->full_name);
    }
  }

  SET_OPTIONS(f->opts, FieldDescriptorProto, FieldOptions, field_proto);

  if (google_protobuf_FieldOptions_has_packed(f->opts)) {
    f->packed_ = google_protobuf_FieldOptions_packed(f->opts);
  } else {
    /* Repeated fields default to packed for proto3 only. */
    f->packed_ = upb_FieldDef_IsPrimitive(f) &&
                 f->label_ == kUpb_Label_Repeated &&
                 f->file->syntax == kUpb_Syntax_Proto3;
  }
}

static void create_service(
    symtab_addctx* ctx, const google_protobuf_ServiceDescriptorProto* svc_proto,
    const upb_ServiceDef* _s) {
  upb_ServiceDef* s = (upb_ServiceDef*)_s;
  upb_StringView name;
  const google_protobuf_MethodDescriptorProto* const* methods;
  size_t i, n;

  s->file = ctx->file; /* Must happen prior to symtab_add. */

  name = google_protobuf_ServiceDescriptorProto_name(svc_proto);
  check_ident(ctx, name, false);
  s->full_name = makefullname(ctx, ctx->file->package, name);
  symtab_add(ctx, s->full_name, pack_def(s, UPB_DEFTYPE_SERVICE));

  methods = google_protobuf_ServiceDescriptorProto_method(svc_proto, &n);

  s->method_count = n;
  s->methods = symtab_alloc(ctx, sizeof(*s->methods) * n);

  SET_OPTIONS(s->opts, ServiceDescriptorProto, ServiceOptions, svc_proto);

  for (i = 0; i < n; i++) {
    const google_protobuf_MethodDescriptorProto* method_proto = methods[i];
    upb_MethodDef* m = (upb_MethodDef*)&s->methods[i];
    upb_StringView name =
        google_protobuf_MethodDescriptorProto_name(method_proto);

    m->service = s;
    m->full_name = makefullname(ctx, s->full_name, name);
    m->index = i;
    m->client_streaming =
        google_protobuf_MethodDescriptorProto_client_streaming(method_proto);
    m->server_streaming =
        google_protobuf_MethodDescriptorProto_server_streaming(method_proto);
    m->input_type = symtab_resolve(
        ctx, m->full_name, m->full_name,
        google_protobuf_MethodDescriptorProto_input_type(method_proto),
        UPB_DEFTYPE_MSG);
    m->output_type = symtab_resolve(
        ctx, m->full_name, m->full_name,
        google_protobuf_MethodDescriptorProto_output_type(method_proto),
        UPB_DEFTYPE_MSG);

    SET_OPTIONS(m->opts, MethodDescriptorProto, MethodOptions, method_proto);
  }
}

static int count_bits_debug(uint64_t x) {
  // For assertions only, speed does not matter.
  int n = 0;
  while (x) {
    if (x & 1) n++;
    x >>= 1;
  }
  return n;
}

static int compare_int32(const void* a_ptr, const void* b_ptr) {
  int32_t a = *(int32_t*)a_ptr;
  int32_t b = *(int32_t*)b_ptr;
  return a < b ? -1 : (a == b ? 0 : 1);
}

upb_MiniTable_Enum* create_enumlayout(symtab_addctx* ctx,
                                      const upb_EnumDef* e) {
  int n = 0;
  uint64_t mask = 0;

  for (int i = 0; i < e->value_count; i++) {
    uint32_t val = (uint32_t)e->values[i].number;
    if (val < 64) {
      mask |= 1ULL << val;
    } else {
      n++;
    }
  }

  int32_t* values = symtab_alloc(ctx, sizeof(*values) * n);

  if (n) {
    int32_t* p = values;

    // Add values outside the bitmask range to the list, as described in the
    // comments for upb_MiniTable_Enum.
    for (int i = 0; i < e->value_count; i++) {
      int32_t val = e->values[i].number;
      if ((uint32_t)val >= 64) {
        *p++ = val;
      }
    }
    UPB_ASSERT(p == values + n);
  }

  // Enums can have duplicate values; we must sort+uniq them.
  if (values) qsort(values, n, sizeof(*values), &compare_int32);

  int dst = 0;
  for (int i = 0; i < n; dst++) {
    int32_t val = values[i];
    while (i < n && values[i] == val) i++;  // Skip duplicates.
    values[dst] = val;
  }
  n = dst;

  UPB_ASSERT(upb_inttable_count(&e->iton) == n + count_bits_debug(mask));

  upb_MiniTable_Enum* layout = symtab_alloc(ctx, sizeof(*layout));
  layout->value_count = n;
  layout->mask = mask;
  layout->values = values;

  return layout;
}

static void create_enumvaldef(
    symtab_addctx* ctx, const char* prefix,
    const google_protobuf_EnumValueDescriptorProto* val_proto, upb_EnumDef* e,
    int i) {
  upb_EnumValueDef* val = (upb_EnumValueDef*)&e->values[i];
  upb_StringView name =
      google_protobuf_EnumValueDescriptorProto_name(val_proto);
  upb_value v = upb_value_constptr(val);

  val->parent = e; /* Must happen prior to symtab_add(). */
  val->full_name = makefullname(ctx, prefix, name);
  val->number = google_protobuf_EnumValueDescriptorProto_number(val_proto);
  symtab_add(ctx, val->full_name, pack_def(val, UPB_DEFTYPE_ENUMVAL));

  SET_OPTIONS(val->opts, EnumValueDescriptorProto, EnumValueOptions, val_proto);

  if (i == 0 && e->file->syntax == kUpb_Syntax_Proto3 && val->number != 0) {
    symtab_errf(ctx, "for proto3, the first enum value must be zero (%s)",
                e->full_name);
  }

  CHK_OOM(upb_strtable_insert(&e->ntoi, name.data, name.size, v, ctx->arena));

  // Multiple enumerators can have the same number, first one wins.
  if (!upb_inttable_lookup(&e->iton, val->number, NULL)) {
    CHK_OOM(upb_inttable_insert(&e->iton, val->number, v, ctx->arena));
  }
}

static void create_enumdef(
    symtab_addctx* ctx, const char* prefix,
    const google_protobuf_EnumDescriptorProto* enum_proto,
    const upb_MessageDef* containing_type, const upb_EnumDef* _e) {
  upb_EnumDef* e = (upb_EnumDef*)_e;
  ;
  const google_protobuf_EnumValueDescriptorProto* const* values;
  upb_StringView name;
  size_t i, n;

  e->file = ctx->file; /* Must happen prior to symtab_add() */
  e->containing_type = containing_type;

  name = google_protobuf_EnumDescriptorProto_name(enum_proto);
  check_ident(ctx, name, false);

  e->full_name = makefullname(ctx, prefix, name);
  symtab_add(ctx, e->full_name, pack_def(e, UPB_DEFTYPE_ENUM));

  values = google_protobuf_EnumDescriptorProto_value(enum_proto, &n);
  CHK_OOM(upb_strtable_init(&e->ntoi, n, ctx->arena));
  CHK_OOM(upb_inttable_init(&e->iton, ctx->arena));

  e->defaultval = 0;
  e->value_count = n;
  e->values = symtab_alloc(ctx, sizeof(*e->values) * n);

  if (n == 0) {
    symtab_errf(ctx, "enums must contain at least one value (%s)",
                e->full_name);
  }

  SET_OPTIONS(e->opts, EnumDescriptorProto, EnumOptions, enum_proto);

  for (i = 0; i < n; i++) {
    create_enumvaldef(ctx, prefix, values[i], e, i);
  }

  upb_inttable_compact(&e->iton, ctx->arena);

  if (e->file->syntax == kUpb_Syntax_Proto2) {
    if (ctx->layout) {
      UPB_ASSERT(ctx->enum_count < ctx->layout->enum_count);
      e->layout = ctx->layout->enums[ctx->enum_count++];
      UPB_ASSERT(upb_inttable_count(&e->iton) ==
                 e->layout->value_count + count_bits_debug(e->layout->mask));
    } else {
      e->layout = create_enumlayout(ctx, e);
    }
  } else {
    e->layout = NULL;
  }
}

static void msgdef_create_nested(
    symtab_addctx* ctx, const google_protobuf_DescriptorProto* msg_proto,
    upb_MessageDef* m);

static void create_msgdef(symtab_addctx* ctx, const char* prefix,
                          const google_protobuf_DescriptorProto* msg_proto,
                          const upb_MessageDef* containing_type,
                          const upb_MessageDef* _m) {
  upb_MessageDef* m = (upb_MessageDef*)_m;
  const google_protobuf_OneofDescriptorProto* const* oneofs;
  const google_protobuf_FieldDescriptorProto* const* fields;
  const google_protobuf_DescriptorProto_ExtensionRange* const* ext_ranges;
  size_t i, n_oneof, n_field, n_ext_range;
  upb_StringView name;

  m->file = ctx->file; /* Must happen prior to symtab_add(). */
  m->containing_type = containing_type;

  name = google_protobuf_DescriptorProto_name(msg_proto);
  check_ident(ctx, name, false);

  m->full_name = makefullname(ctx, prefix, name);
  symtab_add(ctx, m->full_name, pack_def(m, UPB_DEFTYPE_MSG));

  oneofs = google_protobuf_DescriptorProto_oneof_decl(msg_proto, &n_oneof);
  fields = google_protobuf_DescriptorProto_field(msg_proto, &n_field);
  ext_ranges =
      google_protobuf_DescriptorProto_extension_range(msg_proto, &n_ext_range);

  CHK_OOM(upb_inttable_init(&m->itof, ctx->arena));
  CHK_OOM(upb_strtable_init(&m->ntof, n_oneof + n_field, ctx->arena));

  if (ctx->layout) {
    /* create_fielddef() below depends on this being set. */
    UPB_ASSERT(ctx->msg_count < ctx->layout->msg_count);
    m->layout = ctx->layout->msgs[ctx->msg_count++];
    UPB_ASSERT(n_field == m->layout->field_count);
  } else {
    /* Allocate now (to allow cross-linking), populate later. */
    m->layout =
        symtab_alloc(ctx, sizeof(*m->layout) + sizeof(_upb_FastTable_Entry));
  }

  SET_OPTIONS(m->opts, DescriptorProto, MessageOptions, msg_proto);

  m->oneof_count = n_oneof;
  m->oneofs = symtab_alloc(ctx, sizeof(*m->oneofs) * n_oneof);
  for (i = 0; i < n_oneof; i++) {
    create_oneofdef(ctx, m, oneofs[i], &m->oneofs[i]);
  }

  m->field_count = n_field;
  m->fields = symtab_alloc(ctx, sizeof(*m->fields) * n_field);
  for (i = 0; i < n_field; i++) {
    create_fielddef(ctx, m->full_name, m, fields[i], &m->fields[i],
                    /* is_extension= */ false);
  }

  m->ext_range_count = n_ext_range;
  m->ext_ranges = symtab_alloc(ctx, sizeof(*m->ext_ranges) * n_ext_range);
  for (i = 0; i < n_ext_range; i++) {
    const google_protobuf_DescriptorProto_ExtensionRange* r = ext_ranges[i];
    upb_ExtensionRange* r_def = (upb_ExtensionRange*)&m->ext_ranges[i];
    int32_t start = google_protobuf_DescriptorProto_ExtensionRange_start(r);
    int32_t end = google_protobuf_DescriptorProto_ExtensionRange_end(r);
    int32_t max =
        google_protobuf_MessageOptions_message_set_wire_format(m->opts)
            ? INT32_MAX
            : kUpb_MaxFieldNumber + 1;

    // A full validation would also check that each range is disjoint, and that
    // none of the fields overlap with the extension ranges, but we are just
    // sanity checking here.
    if (start < 1 || end <= start || end > max) {
      symtab_errf(ctx, "Extension range (%d, %d) is invalid, message=%s\n",
                  (int)start, (int)end, m->full_name);
    }

    r_def->start = start;
    r_def->end = end;
    SET_OPTIONS(r_def->opts, DescriptorProto_ExtensionRange,
                ExtensionRangeOptions, r);
  }

  finalize_oneofs(ctx, m);
  assign_msg_wellknowntype(m);
  upb_inttable_compact(&m->itof, ctx->arena);
  msgdef_create_nested(ctx, msg_proto, m);
}

static void msgdef_create_nested(
    symtab_addctx* ctx, const google_protobuf_DescriptorProto* msg_proto,
    upb_MessageDef* m) {
  size_t n;

  const google_protobuf_EnumDescriptorProto* const* enums =
      google_protobuf_DescriptorProto_enum_type(msg_proto, &n);
  m->nested_enum_count = n;
  m->nested_enums = symtab_alloc(ctx, sizeof(*m->nested_enums) * n);
  for (size_t i = 0; i < n; i++) {
    m->nested_enum_count = i + 1;
    create_enumdef(ctx, m->full_name, enums[i], m, &m->nested_enums[i]);
  }

  const google_protobuf_FieldDescriptorProto* const* exts =
      google_protobuf_DescriptorProto_extension(msg_proto, &n);
  m->nested_ext_count = n;
  m->nested_exts = symtab_alloc(ctx, sizeof(*m->nested_exts) * n);
  for (size_t i = 0; i < n; i++) {
    create_fielddef(ctx, m->full_name, m, exts[i], &m->nested_exts[i],
                    /* is_extension= */ true);
    ((upb_FieldDef*)&m->nested_exts[i])->index_ = i;
  }

  const google_protobuf_DescriptorProto* const* msgs =
      google_protobuf_DescriptorProto_nested_type(msg_proto, &n);
  m->nested_msg_count = n;
  m->nested_msgs = symtab_alloc(ctx, sizeof(*m->nested_msgs) * n);
  for (size_t i = 0; i < n; i++) {
    create_msgdef(ctx, m->full_name, msgs[i], m, &m->nested_msgs[i]);
  }
}

static void resolve_subdef(symtab_addctx* ctx, const char* prefix,
                           upb_FieldDef* f) {
  const google_protobuf_FieldDescriptorProto* field_proto = f->sub.unresolved;
  upb_StringView name =
      google_protobuf_FieldDescriptorProto_type_name(field_proto);
  bool has_name =
      google_protobuf_FieldDescriptorProto_has_type_name(field_proto);
  switch ((int)f->type_) {
    case FIELD_TYPE_UNSPECIFIED: {
      // Type was not specified and must be inferred.
      UPB_ASSERT(has_name);
      upb_deftype_t type;
      const void* def =
          symtab_resolveany(ctx, f->full_name, prefix, name, &type);
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
          symtab_errf(ctx, "Couldn't resolve type name for field %s",
                      f->full_name);
      }
    }
    case kUpb_FieldType_Message:
    case kUpb_FieldType_Group:
      UPB_ASSERT(has_name);
      f->sub.msgdef =
          symtab_resolve(ctx, f->full_name, prefix, name, UPB_DEFTYPE_MSG);
      break;
    case kUpb_FieldType_Enum:
      UPB_ASSERT(has_name);
      f->sub.enumdef =
          symtab_resolve(ctx, f->full_name, prefix, name, UPB_DEFTYPE_ENUM);
      break;
    default:
      // No resolution necessary.
      break;
  }
}

static void resolve_extension(
    symtab_addctx* ctx, const char* prefix, upb_FieldDef* f,
    const google_protobuf_FieldDescriptorProto* field_proto) {
  if (!google_protobuf_FieldDescriptorProto_has_extendee(field_proto)) {
    symtab_errf(ctx, "extension for field '%s' had no extendee", f->full_name);
  }

  upb_StringView name =
      google_protobuf_FieldDescriptorProto_extendee(field_proto);
  const upb_MessageDef* m =
      symtab_resolve(ctx, f->full_name, prefix, name, UPB_DEFTYPE_MSG);
  f->msgdef = m;

  bool found = false;

  for (int i = 0, n = m->ext_range_count; i < n; i++) {
    const upb_ExtensionRange* r = &m->ext_ranges[i];
    if (r->start <= f->number_ && f->number_ < r->end) {
      found = true;
      break;
    }
  }

  if (!found) {
    symtab_errf(ctx,
                "field number %u in extension %s has no extension range in "
                "message %s",
                (unsigned)f->number_, f->full_name, f->msgdef->full_name);
  }

  const upb_MiniTable_Extension* ext = ctx->file->ext_layouts[f->layout_index];
  if (ctx->layout) {
    UPB_ASSERT(upb_FieldDef_Number(f) == ext->field.number);
  } else {
    upb_MiniTable_Extension* mut_ext = (upb_MiniTable_Extension*)ext;
    fill_fieldlayout(&mut_ext->field, f);
    mut_ext->field.presence = 0;
    mut_ext->field.offset = 0;
    mut_ext->field.submsg_index = 0;
    mut_ext->extendee = f->msgdef->layout;
    mut_ext->sub.submsg = f->sub.msgdef->layout;
  }

  CHK_OOM(upb_inttable_insert(&ctx->symtab->exts, (uintptr_t)ext,
                              upb_value_constptr(f), ctx->arena));
}

static void resolve_default(
    symtab_addctx* ctx, upb_FieldDef* f,
    const google_protobuf_FieldDescriptorProto* field_proto) {
  // Have to delay resolving of the default value until now because of the enum
  // case, since enum defaults are specified with a label.
  if (google_protobuf_FieldDescriptorProto_has_default_value(field_proto)) {
    upb_StringView defaultval =
        google_protobuf_FieldDescriptorProto_default_value(field_proto);

    if (f->file->syntax == kUpb_Syntax_Proto3) {
      symtab_errf(ctx, "proto3 fields cannot have explicit defaults (%s)",
                  f->full_name);
    }

    if (upb_FieldDef_IsSubMessage(f)) {
      symtab_errf(ctx, "message fields cannot have explicit defaults (%s)",
                  f->full_name);
    }

    parse_default(ctx, defaultval.data, defaultval.size, f);
    f->has_default = true;
  } else {
    set_default_default(ctx, f);
    f->has_default = false;
  }
}

static void resolve_fielddef(symtab_addctx* ctx, const char* prefix,
                             upb_FieldDef* f) {
  // We have to stash this away since resolve_subdef() may overwrite it.
  const google_protobuf_FieldDescriptorProto* field_proto = f->sub.unresolved;

  resolve_subdef(ctx, prefix, f);
  resolve_default(ctx, f, field_proto);

  if (f->is_extension_) {
    resolve_extension(ctx, prefix, f, field_proto);
  }
}

static void resolve_msgdef(symtab_addctx* ctx, upb_MessageDef* m) {
  for (int i = 0; i < m->field_count; i++) {
    resolve_fielddef(ctx, m->full_name, (upb_FieldDef*)&m->fields[i]);
  }

  m->in_message_set = false;
  for (int i = 0; i < m->nested_ext_count; i++) {
    upb_FieldDef* ext = (upb_FieldDef*)&m->nested_exts[i];
    resolve_fielddef(ctx, m->full_name, ext);
    if (ext->type_ == kUpb_FieldType_Message &&
        ext->label_ == kUpb_Label_Optional && ext->sub.msgdef == m &&
        google_protobuf_MessageOptions_message_set_wire_format(
            ext->msgdef->opts)) {
      m->in_message_set = true;
    }
  }

  if (!ctx->layout) make_layout(ctx, m);

  for (int i = 0; i < m->nested_msg_count; i++) {
    resolve_msgdef(ctx, (upb_MessageDef*)&m->nested_msgs[i]);
  }
}

static int count_exts_in_msg(const google_protobuf_DescriptorProto* msg_proto) {
  size_t n;
  google_protobuf_DescriptorProto_extension(msg_proto, &n);
  int ext_count = n;

  const google_protobuf_DescriptorProto* const* nested_msgs =
      google_protobuf_DescriptorProto_nested_type(msg_proto, &n);
  for (size_t i = 0; i < n; i++) {
    ext_count += count_exts_in_msg(nested_msgs[i]);
  }

  return ext_count;
}

static void build_filedef(
    symtab_addctx* ctx, upb_FileDef* file,
    const google_protobuf_FileDescriptorProto* file_proto) {
  const google_protobuf_DescriptorProto* const* msgs;
  const google_protobuf_EnumDescriptorProto* const* enums;
  const google_protobuf_FieldDescriptorProto* const* exts;
  const google_protobuf_ServiceDescriptorProto* const* services;
  const upb_StringView* strs;
  const int32_t* public_deps;
  const int32_t* weak_deps;
  size_t i, n;

  file->symtab = ctx->symtab;

  /* Count all extensions in the file, to build a flat array of layouts. */
  google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  int ext_count = n;
  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  for (int i = 0; i < n; i++) {
    ext_count += count_exts_in_msg(msgs[i]);
  }
  file->ext_count = ext_count;

  if (ctx->layout) {
    /* We are using the ext layouts that were passed in. */
    file->ext_layouts = ctx->layout->exts;
    if (ctx->layout->ext_count != file->ext_count) {
      symtab_errf(ctx, "Extension count did not match layout (%d vs %d)",
                  ctx->layout->ext_count, file->ext_count);
    }
  } else {
    /* We are building ext layouts from scratch. */
    file->ext_layouts =
        symtab_alloc(ctx, sizeof(*file->ext_layouts) * file->ext_count);
    upb_MiniTable_Extension* ext =
        symtab_alloc(ctx, sizeof(*ext) * file->ext_count);
    for (int i = 0; i < file->ext_count; i++) {
      file->ext_layouts[i] = &ext[i];
    }
  }

  if (!google_protobuf_FileDescriptorProto_has_name(file_proto)) {
    symtab_errf(ctx, "File has no name");
  }

  file->name =
      strviewdup(ctx, google_protobuf_FileDescriptorProto_name(file_proto));

  if (google_protobuf_FileDescriptorProto_has_package(file_proto)) {
    upb_StringView package =
        google_protobuf_FileDescriptorProto_package(file_proto);
    check_ident(ctx, package, true);
    file->package = strviewdup(ctx, package);
  } else {
    file->package = NULL;
  }

  if (google_protobuf_FileDescriptorProto_has_syntax(file_proto)) {
    upb_StringView syntax =
        google_protobuf_FileDescriptorProto_syntax(file_proto);

    if (streql_view(syntax, "proto2")) {
      file->syntax = kUpb_Syntax_Proto2;
    } else if (streql_view(syntax, "proto3")) {
      file->syntax = kUpb_Syntax_Proto3;
    } else {
      symtab_errf(ctx, "Invalid syntax '" UPB_STRINGVIEW_FORMAT "'",
                  UPB_STRINGVIEW_ARGS(syntax));
    }
  } else {
    file->syntax = kUpb_Syntax_Proto2;
  }

  /* Read options. */
  SET_OPTIONS(file->opts, FileDescriptorProto, FileOptions, file_proto);

  /* Verify dependencies. */
  strs = google_protobuf_FileDescriptorProto_dependency(file_proto, &n);
  file->dep_count = n;
  file->deps = symtab_alloc(ctx, sizeof(*file->deps) * n);

  for (i = 0; i < n; i++) {
    upb_StringView str = strs[i];
    file->deps[i] =
        upb_DefPool_FindFileByNameWithSize(ctx->symtab, str.data, str.size);
    if (!file->deps[i]) {
      symtab_errf(ctx,
                  "Depends on file '" UPB_STRINGVIEW_FORMAT
                  "', but it has not been loaded",
                  UPB_STRINGVIEW_ARGS(str));
    }
  }

  public_deps =
      google_protobuf_FileDescriptorProto_public_dependency(file_proto, &n);
  file->public_dep_count = n;
  file->public_deps = symtab_alloc(ctx, sizeof(*file->public_deps) * n);
  int32_t* mutable_public_deps = (int32_t*)file->public_deps;
  for (i = 0; i < n; i++) {
    if (public_deps[i] >= file->dep_count) {
      symtab_errf(ctx, "public_dep %d is out of range", (int)public_deps[i]);
    }
    mutable_public_deps[i] = public_deps[i];
  }

  weak_deps =
      google_protobuf_FileDescriptorProto_weak_dependency(file_proto, &n);
  file->weak_dep_count = n;
  file->weak_deps = symtab_alloc(ctx, sizeof(*file->weak_deps) * n);
  int32_t* mutable_weak_deps = (int32_t*)file->weak_deps;
  for (i = 0; i < n; i++) {
    if (weak_deps[i] >= file->dep_count) {
      symtab_errf(ctx, "weak_dep %d is out of range", (int)weak_deps[i]);
    }
    mutable_weak_deps[i] = weak_deps[i];
  }

  /* Create enums. */
  enums = google_protobuf_FileDescriptorProto_enum_type(file_proto, &n);
  file->top_lvl_enum_count = n;
  file->top_lvl_enums = symtab_alloc(ctx, sizeof(*file->top_lvl_enums) * n);
  for (i = 0; i < n; i++) {
    create_enumdef(ctx, file->package, enums[i], NULL, &file->top_lvl_enums[i]);
  }

  /* Create extensions. */
  exts = google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  file->top_lvl_ext_count = n;
  file->top_lvl_exts = symtab_alloc(ctx, sizeof(*file->top_lvl_exts) * n);
  for (i = 0; i < n; i++) {
    create_fielddef(ctx, file->package, NULL, exts[i], &file->top_lvl_exts[i],
                    /* is_extension= */ true);
    ((upb_FieldDef*)&file->top_lvl_exts[i])->index_ = i;
  }

  /* Create messages. */
  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  file->top_lvl_msg_count = n;
  file->top_lvl_msgs = symtab_alloc(ctx, sizeof(*file->top_lvl_msgs) * n);
  for (i = 0; i < n; i++) {
    create_msgdef(ctx, file->package, msgs[i], NULL, &file->top_lvl_msgs[i]);
  }

  /* Create services. */
  services = google_protobuf_FileDescriptorProto_service(file_proto, &n);
  file->service_count = n;
  file->services = symtab_alloc(ctx, sizeof(*file->services) * n);
  for (i = 0; i < n; i++) {
    create_service(ctx, services[i], &file->services[i]);
    ((upb_ServiceDef*)&file->services[i])->index = i;
  }

  /* Now that all names are in the table, build layouts and resolve refs. */
  for (i = 0; i < (size_t)file->top_lvl_ext_count; i++) {
    resolve_fielddef(ctx, file->package, (upb_FieldDef*)&file->top_lvl_exts[i]);
  }

  for (i = 0; i < (size_t)file->top_lvl_msg_count; i++) {
    resolve_msgdef(ctx, (upb_MessageDef*)&file->top_lvl_msgs[i]);
  }

  if (file->ext_count) {
    CHK_OOM(_upb_extreg_add(ctx->symtab->extreg, file->ext_layouts,
                            file->ext_count));
  }
}

static void remove_filedef(upb_DefPool* s, upb_FileDef* file) {
  intptr_t iter = UPB_INTTABLE_BEGIN;
  upb_StringView key;
  upb_value val;
  while (upb_strtable_next2(&s->syms, &key, &val, &iter)) {
    const upb_FileDef* f;
    switch (deftype(val)) {
      case UPB_DEFTYPE_EXT:
        f = upb_FieldDef_File(unpack_def(val, UPB_DEFTYPE_EXT));
        break;
      case UPB_DEFTYPE_MSG:
        f = upb_MessageDef_File(unpack_def(val, UPB_DEFTYPE_MSG));
        break;
      case UPB_DEFTYPE_ENUM:
        f = upb_EnumDef_File(unpack_def(val, UPB_DEFTYPE_ENUM));
        break;
      case UPB_DEFTYPE_ENUMVAL:
        f = upb_EnumDef_File(
            upb_EnumValueDef_Enum(unpack_def(val, UPB_DEFTYPE_ENUMVAL)));
        break;
      case UPB_DEFTYPE_SERVICE:
        f = upb_ServiceDef_File(unpack_def(val, UPB_DEFTYPE_SERVICE));
        break;
      default:
        UPB_UNREACHABLE();
    }

    if (f == file) upb_strtable_removeiter(&s->syms, &iter);
  }
}

static const upb_FileDef* _upb_DefPool_AddFile(
    upb_DefPool* s, const google_protobuf_FileDescriptorProto* file_proto,
    const upb_MiniTable_File* layout, upb_Status* status) {
  symtab_addctx ctx;
  upb_StringView name = google_protobuf_FileDescriptorProto_name(file_proto);
  upb_value v;

  if (upb_strtable_lookup2(&s->files, name.data, name.size, &v)) {
    if (unpack_def(v, UPB_DEFTYPE_FILE)) {
      upb_Status_SetErrorFormat(status, "duplicate file name (%.*s)",
                                UPB_STRINGVIEW_ARGS(name));
      return NULL;
    }
    const upb_MiniTable_File* registered = unpack_def(v, UPB_DEFTYPE_LAYOUT);
    UPB_ASSERT(registered);
    if (layout && layout != registered) {
      upb_Status_SetErrorFormat(
          status, "tried to build with a different layout (filename=%.*s)",
          UPB_STRINGVIEW_ARGS(name));
      return NULL;
    }
    layout = registered;
  }

  ctx.symtab = s;
  ctx.layout = layout;
  ctx.msg_count = 0;
  ctx.enum_count = 0;
  ctx.ext_count = 0;
  ctx.status = status;
  ctx.file = NULL;
  ctx.arena = upb_Arena_New();
  ctx.tmp_arena = upb_Arena_New();

  if (!ctx.arena || !ctx.tmp_arena) {
    if (ctx.arena) upb_Arena_Free(ctx.arena);
    if (ctx.tmp_arena) upb_Arena_Free(ctx.tmp_arena);
    upb_Status_setoom(status);
    return NULL;
  }

  if (UPB_UNLIKELY(UPB_SETJMP(ctx.err))) {
    UPB_ASSERT(!upb_Status_IsOk(status));
    if (ctx.file) {
      remove_filedef(s, ctx.file);
      ctx.file = NULL;
    }
  } else {
    ctx.file = symtab_alloc(&ctx, sizeof(*ctx.file));
    build_filedef(&ctx, ctx.file, file_proto);
    upb_strtable_insert(&s->files, name.data, name.size,
                        pack_def(ctx.file, UPB_DEFTYPE_FILE), ctx.arena);
    UPB_ASSERT(upb_Status_IsOk(status));
    upb_Arena_Fuse(s->arena, ctx.arena);
  }

  upb_Arena_Free(ctx.arena);
  upb_Arena_Free(ctx.tmp_arena);
  return ctx.file;
}

const upb_FileDef* upb_DefPool_AddFile(
    upb_DefPool* s, const google_protobuf_FileDescriptorProto* file_proto,
    upb_Status* status) {
  return _upb_DefPool_AddFile(s, file_proto, NULL, status);
}

/* Include here since we want most of this file to be stdio-free. */
#include <stdio.h>

bool _upb_DefPool_LoadDefInitEx(upb_DefPool* s, const _upb_DefPool_Init* init,
                                bool rebuild_minitable) {
  /* Since this function should never fail (it would indicate a bug in upb) we
   * print errors to stderr instead of returning error status to the user. */
  _upb_DefPool_Init** deps = init->deps;
  google_protobuf_FileDescriptorProto* file;
  upb_Arena* arena;
  upb_Status status;

  upb_Status_Clear(&status);

  if (upb_DefPool_FindFileByName(s, init->filename)) {
    return true;
  }

  arena = upb_Arena_New();

  for (; *deps; deps++) {
    if (!_upb_DefPool_LoadDefInitEx(s, *deps, rebuild_minitable)) goto err;
  }

  file = google_protobuf_FileDescriptorProto_parse_ex(
      init->descriptor.data, init->descriptor.size, NULL,
      kUpb_DecodeOption_AliasString, arena);
  s->bytes_loaded += init->descriptor.size;

  if (!file) {
    upb_Status_SetErrorFormat(
        &status,
        "Failed to parse compiled-in descriptor for file '%s'. This should "
        "never happen.",
        init->filename);
    goto err;
  }

  const upb_MiniTable_File* mt = rebuild_minitable ? NULL : init->layout;
  if (!_upb_DefPool_AddFile(s, file, mt, &status)) {
    goto err;
  }

  upb_Arena_Free(arena);
  return true;

err:
  fprintf(stderr,
          "Error loading compiled-in descriptor for file '%s' (this should "
          "never happen): %s\n",
          init->filename, upb_Status_ErrorMessage(&status));
  upb_Arena_Free(arena);
  return false;
}

size_t _upb_DefPool_BytesLoaded(const upb_DefPool* s) {
  return s->bytes_loaded;
}

upb_Arena* _upb_DefPool_Arena(const upb_DefPool* s) { return s->arena; }

const upb_FieldDef* _upb_DefPool_FindExtensionByMiniTable(
    const upb_DefPool* s, const upb_MiniTable_Extension* ext) {
  upb_value v;
  bool ok = upb_inttable_lookup(&s->exts, (uintptr_t)ext, &v);
  UPB_ASSERT(ok);
  return upb_value_getconstptr(v);
}

const upb_FieldDef* upb_DefPool_FindExtensionByNumber(const upb_DefPool* s,
                                                      const upb_MessageDef* m,
                                                      int32_t fieldnum) {
  const upb_MiniTable* l = upb_MessageDef_MiniTable(m);
  const upb_MiniTable_Extension* ext = _upb_extreg_get(s->extreg, l, fieldnum);
  return ext ? _upb_DefPool_FindExtensionByMiniTable(s, ext) : NULL;
}

bool _upb_DefPool_registerlayout(upb_DefPool* s, const char* filename,
                                 const upb_MiniTable_File* file) {
  if (upb_DefPool_FindFileByName(s, filename)) return false;
  upb_value v = pack_def(file, UPB_DEFTYPE_LAYOUT);
  return upb_strtable_insert(&s->files, filename, strlen(filename), v,
                             s->arena);
}

const upb_ExtensionRegistry* upb_DefPool_ExtensionRegistry(
    const upb_DefPool* s) {
  return s->extreg;
}

const upb_FieldDef** upb_DefPool_GetAllExtensions(const upb_DefPool* s,
                                                  const upb_MessageDef* m,
                                                  size_t* count) {
  size_t n = 0;
  intptr_t iter = UPB_INTTABLE_BEGIN;
  uintptr_t key;
  upb_value val;
  // This is O(all exts) instead of O(exts for m).  If we need this to be
  // efficient we may need to make extreg into a two-level table, or have a
  // second per-message index.
  while (upb_inttable_next2(&s->exts, &key, &val, &iter)) {
    const upb_FieldDef* f = upb_value_getconstptr(val);
    if (upb_FieldDef_ContainingType(f) == m) n++;
  }
  const upb_FieldDef** exts = malloc(n * sizeof(*exts));
  iter = UPB_INTTABLE_BEGIN;
  size_t i = 0;
  while (upb_inttable_next2(&s->exts, &key, &val, &iter)) {
    const upb_FieldDef* f = upb_value_getconstptr(val);
    if (upb_FieldDef_ContainingType(f) == m) exts[i++] = f;
  }
  *count = n;
  return exts;
}

#undef CHK_OOM
