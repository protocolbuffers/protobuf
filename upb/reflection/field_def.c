// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/internal/field_def.h"

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_descriptor/internal/encode.h"
#include "upb/mini_descriptor/internal/modifiers.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/mini_table/sub.h"
#include "upb/reflection/def.h"
#include "upb/reflection/def_type.h"
#include "upb/reflection/internal/def_builder.h"
#include "upb/reflection/internal/def_pool.h"
#include "upb/reflection/internal/desc_state.h"
#include "upb/reflection/internal/enum_def.h"
#include "upb/reflection/internal/file_def.h"
#include "upb/reflection/internal/message_def.h"
#include "upb/reflection/internal/oneof_def.h"
#include "upb/reflection/internal/strdup2.h"

// Must be last.
#include "upb/port/def.inc"

#define UPB_FIELD_TYPE_UNSPECIFIED 0

typedef struct {
  size_t len;
  char str[1];  // Null-terminated string data follows.
} str_t;

struct upb_FieldDef {
  UPB_ALIGN_AS(8) const UPB_DESC(FieldOptions*) opts;
  const UPB_DESC(FeatureSet*) resolved_features;
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
    void* msg;  // Always NULL.
  } defaultval;
  union {
    const upb_OneofDef* oneof;
    const upb_MessageDef* extension_scope;
  } scope;
  union {
    const upb_MessageDef* msgdef;
    const upb_EnumDef* enumdef;
    const UPB_DESC(FieldDescriptorProto) * unresolved;
  } sub;
  uint32_t number_;
  uint16_t index_;
  uint16_t layout_index;  // Index into msgdef->layout->fields or file->exts
  bool has_default;
  bool has_json_name;
  bool has_presence;
  bool is_extension;
  bool is_proto3_optional;
  upb_FieldType type_;
  upb_Label label_;
};

upb_FieldDef* _upb_FieldDef_At(const upb_FieldDef* f, int i) {
  return (upb_FieldDef*)&f[i];
}

const UPB_DESC(FieldOptions) * upb_FieldDef_Options(const upb_FieldDef* f) {
  return f->opts;
}

bool upb_FieldDef_HasOptions(const upb_FieldDef* f) {
  return f->opts != (void*)kUpbDefOptDefault;
}

const UPB_DESC(FeatureSet) *
    upb_FieldDef_ResolvedFeatures(const upb_FieldDef* f) {
  return f->resolved_features;
}

const char* upb_FieldDef_FullName(const upb_FieldDef* f) {
  return f->full_name;
}

upb_CType upb_FieldDef_CType(const upb_FieldDef* f) {
  return upb_FieldType_CType(f->type_);
}

upb_FieldType upb_FieldDef_Type(const upb_FieldDef* f) { return f->type_; }

uint32_t upb_FieldDef_Index(const upb_FieldDef* f) { return f->index_; }

uint32_t upb_FieldDef_LayoutIndex(const upb_FieldDef* f) {
  return f->layout_index;
}

upb_Label upb_FieldDef_Label(const upb_FieldDef* f) { return f->label_; }

uint32_t upb_FieldDef_Number(const upb_FieldDef* f) { return f->number_; }

bool upb_FieldDef_IsExtension(const upb_FieldDef* f) { return f->is_extension; }

bool _upb_FieldDef_IsPackable(const upb_FieldDef* f) {
  return upb_FieldDef_IsRepeated(f) && upb_FieldDef_IsPrimitive(f);
}

bool upb_FieldDef_IsPacked(const upb_FieldDef* f) {
  return _upb_FieldDef_IsPackable(f) &&
         UPB_DESC(FeatureSet_repeated_field_encoding(f->resolved_features)) ==
             UPB_DESC(FeatureSet_PACKED);
}

const char* upb_FieldDef_Name(const upb_FieldDef* f) {
  return _upb_DefBuilder_FullToShort(f->full_name);
}

const char* upb_FieldDef_JsonName(const upb_FieldDef* f) {
  return f->json_name;
}

bool upb_FieldDef_HasJsonName(const upb_FieldDef* f) {
  return f->has_json_name;
}

const upb_FileDef* upb_FieldDef_File(const upb_FieldDef* f) { return f->file; }

const upb_MessageDef* upb_FieldDef_ContainingType(const upb_FieldDef* f) {
  return f->msgdef;
}

const upb_MessageDef* upb_FieldDef_ExtensionScope(const upb_FieldDef* f) {
  return f->is_extension ? f->scope.extension_scope : NULL;
}

const upb_OneofDef* upb_FieldDef_ContainingOneof(const upb_FieldDef* f) {
  return f->is_extension ? NULL : f->scope.oneof;
}

const upb_OneofDef* upb_FieldDef_RealContainingOneof(const upb_FieldDef* f) {
  const upb_OneofDef* oneof = upb_FieldDef_ContainingOneof(f);
  if (!oneof || upb_OneofDef_IsSynthetic(oneof)) return NULL;
  return oneof;
}

upb_MessageValue upb_FieldDef_Default(const upb_FieldDef* f) {
  upb_MessageValue ret;

  if (upb_FieldDef_IsRepeated(f) || upb_FieldDef_IsSubMessage(f)) {
    return (upb_MessageValue){.msg_val = NULL};
  }

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
  return upb_FieldDef_IsSubMessage(f) ? f->sub.msgdef : NULL;
}

const upb_EnumDef* upb_FieldDef_EnumSubDef(const upb_FieldDef* f) {
  return upb_FieldDef_IsEnum(f) ? f->sub.enumdef : NULL;
}

const upb_MiniTableField* upb_FieldDef_MiniTable(const upb_FieldDef* f) {
  if (upb_FieldDef_IsExtension(f)) {
    const upb_FileDef* file = upb_FieldDef_File(f);
    return (upb_MiniTableField*)_upb_FileDef_ExtensionMiniTable(
        file, f->layout_index);
  } else {
    const upb_MiniTable* layout = upb_MessageDef_MiniTable(f->msgdef);
    return &layout->UPB_PRIVATE(fields)[f->layout_index];
  }
}

const upb_MiniTableExtension* upb_FieldDef_MiniTableExtension(
    const upb_FieldDef* f) {
  UPB_ASSERT(upb_FieldDef_IsExtension(f));
  const upb_FileDef* file = upb_FieldDef_File(f);
  return _upb_FileDef_ExtensionMiniTable(file, f->layout_index);
}

bool _upb_FieldDef_IsClosedEnum(const upb_FieldDef* f) {
  if (f->type_ != kUpb_FieldType_Enum) return false;
  return upb_EnumDef_IsClosed(f->sub.enumdef);
}

bool _upb_FieldDef_IsProto3Optional(const upb_FieldDef* f) {
  return f->is_proto3_optional;
}

int _upb_FieldDef_LayoutIndex(const upb_FieldDef* f) { return f->layout_index; }

bool _upb_FieldDef_ValidateUtf8(const upb_FieldDef* f) {
  if (upb_FieldDef_Type(f) != kUpb_FieldType_String) return false;
  return UPB_DESC(FeatureSet_utf8_validation(f->resolved_features)) ==
         UPB_DESC(FeatureSet_VERIFY);
}

bool _upb_FieldDef_IsGroupLike(const upb_FieldDef* f) {
  // Groups are always tag-delimited.
  if (f->type_ != kUpb_FieldType_Group) {
    return false;
  }

  const upb_MessageDef* msg = upb_FieldDef_MessageSubDef(f);

  // Group fields always are always the lowercase type name.
  const char* mname = upb_MessageDef_Name(msg);
  const char* fname = upb_FieldDef_Name(f);
  size_t name_size = strlen(fname);
  if (name_size != strlen(mname)) return false;
  for (size_t i = 0; i < name_size; ++i) {
    if ((mname[i] | 0x20) != fname[i]) {
      // Case-insensitive ascii comparison.
      return false;
    }
  }

  if (upb_MessageDef_File(msg) != upb_FieldDef_File(f)) {
    return false;
  }

  // Group messages are always defined in the same scope as the field.  File
  // level extensions will compare NULL == NULL here, which is why the file
  // comparison above is necessary to ensure both come from the same file.
  return upb_FieldDef_IsExtension(f) ? upb_FieldDef_ExtensionScope(f) ==
                                           upb_MessageDef_ContainingType(msg)
                                     : upb_FieldDef_ContainingType(f) ==
                                           upb_MessageDef_ContainingType(msg);
}

uint64_t _upb_FieldDef_Modifiers(const upb_FieldDef* f) {
  uint64_t out = upb_FieldDef_IsPacked(f) ? kUpb_FieldModifier_IsPacked : 0;

  if (upb_FieldDef_IsRepeated(f)) {
    out |= kUpb_FieldModifier_IsRepeated;
  } else if (upb_FieldDef_IsRequired(f)) {
    out |= kUpb_FieldModifier_IsRequired;
  } else if (!upb_FieldDef_HasPresence(f)) {
    out |= kUpb_FieldModifier_IsProto3Singular;
  }

  if (_upb_FieldDef_IsClosedEnum(f)) {
    out |= kUpb_FieldModifier_IsClosedEnum;
  }

  if (_upb_FieldDef_ValidateUtf8(f)) {
    out |= kUpb_FieldModifier_ValidateUtf8;
  }

  return out;
}

bool upb_FieldDef_HasDefault(const upb_FieldDef* f) { return f->has_default; }
bool upb_FieldDef_HasPresence(const upb_FieldDef* f) { return f->has_presence; }

bool upb_FieldDef_HasSubDef(const upb_FieldDef* f) {
  return upb_FieldDef_IsSubMessage(f) || upb_FieldDef_IsEnum(f);
}

bool upb_FieldDef_IsEnum(const upb_FieldDef* f) {
  return upb_FieldDef_CType(f) == kUpb_CType_Enum;
}

bool upb_FieldDef_IsMap(const upb_FieldDef* f) {
  return upb_FieldDef_IsRepeated(f) && upb_FieldDef_IsSubMessage(f) &&
         upb_MessageDef_IsMapEntry(upb_FieldDef_MessageSubDef(f));
}

bool upb_FieldDef_IsOptional(const upb_FieldDef* f) {
  return upb_FieldDef_Label(f) == kUpb_Label_Optional;
}

bool upb_FieldDef_IsPrimitive(const upb_FieldDef* f) {
  return !upb_FieldDef_IsString(f) && !upb_FieldDef_IsSubMessage(f);
}

bool upb_FieldDef_IsRepeated(const upb_FieldDef* f) {
  return upb_FieldDef_Label(f) == kUpb_Label_Repeated;
}

bool upb_FieldDef_IsRequired(const upb_FieldDef* f) {
  return UPB_DESC(FeatureSet_field_presence)(f->resolved_features) ==
         UPB_DESC(FeatureSet_LEGACY_REQUIRED);
}

bool upb_FieldDef_IsString(const upb_FieldDef* f) {
  return upb_FieldDef_CType(f) == kUpb_CType_String ||
         upb_FieldDef_CType(f) == kUpb_CType_Bytes;
}

bool upb_FieldDef_IsSubMessage(const upb_FieldDef* f) {
  return upb_FieldDef_CType(f) == kUpb_CType_Message;
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
      // Standard C number parsing functions expect null-terminated strings.
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
      break;
    }
    case kUpb_CType_Message:
      break;
  }
}

static bool _upb_FieldDef_InferLegacyFeatures(
    upb_DefBuilder* ctx, upb_FieldDef* f,
    const UPB_DESC(FieldDescriptorProto*) proto,
    const UPB_DESC(FieldOptions*) options, upb_Syntax syntax,
    UPB_DESC(FeatureSet*) features) {
  bool ret = false;

  if (UPB_DESC(FieldDescriptorProto_label)(proto) == kUpb_Label_Required) {
    if (syntax == kUpb_Syntax_Proto3) {
      _upb_DefBuilder_Errf(ctx, "proto3 fields cannot be required (%s)",
                           f->full_name);
    }
    int val = UPB_DESC(FeatureSet_LEGACY_REQUIRED);
    UPB_DESC(FeatureSet_set_field_presence(features, val));
    ret = true;
  }

  if (UPB_DESC(FieldDescriptorProto_type)(proto) == kUpb_FieldType_Group) {
    int val = UPB_DESC(FeatureSet_DELIMITED);
    UPB_DESC(FeatureSet_set_message_encoding(features, val));
    ret = true;
  }

  if (UPB_DESC(FieldOptions_has_packed)(options)) {
    int val = UPB_DESC(FieldOptions_packed)(options)
                  ? UPB_DESC(FeatureSet_PACKED)
                  : UPB_DESC(FeatureSet_EXPANDED);
    UPB_DESC(FeatureSet_set_repeated_field_encoding(features, val));
    ret = true;
  }

  return ret;
}

static void _upb_FieldDef_Create(upb_DefBuilder* ctx, const char* prefix,
                                 const UPB_DESC(FeatureSet*) parent_features,
                                 const UPB_DESC(FieldDescriptorProto*)
                                     field_proto,
                                 upb_MessageDef* m, upb_FieldDef* f) {
  // Must happen before _upb_DefBuilder_Add()
  f->file = _upb_DefBuilder_File(ctx);

  const upb_StringView name = UPB_DESC(FieldDescriptorProto_name)(field_proto);
  f->full_name = _upb_DefBuilder_MakeFullName(ctx, prefix, name);
  f->number_ = UPB_DESC(FieldDescriptorProto_number)(field_proto);
  f->is_proto3_optional =
      UPB_DESC(FieldDescriptorProto_proto3_optional)(field_proto);
  f->msgdef = m;
  f->scope.oneof = NULL;

  UPB_DEF_SET_OPTIONS(f->opts, FieldDescriptorProto, FieldOptions, field_proto);

  upb_Syntax syntax = upb_FileDef_Syntax(f->file);
  const UPB_DESC(FeatureSet*) unresolved_features =
      UPB_DESC(FieldOptions_features)(f->opts);
  bool implicit = false;

  if (syntax != kUpb_Syntax_Editions) {
    upb_Message_Clear(UPB_UPCAST(ctx->legacy_features),
                      UPB_DESC_MINITABLE(FeatureSet));
    if (_upb_FieldDef_InferLegacyFeatures(ctx, f, field_proto, f->opts, syntax,
                                          ctx->legacy_features)) {
      implicit = true;
      unresolved_features = ctx->legacy_features;
    }
  }

  if (UPB_DESC(FieldDescriptorProto_has_oneof_index)(field_proto)) {
    int oneof_index = UPB_DESC(FieldDescriptorProto_oneof_index)(field_proto);

    if (!m) {
      _upb_DefBuilder_Errf(ctx, "oneof field (%s) has no containing msg",
                           f->full_name);
    }

    if (oneof_index < 0 || oneof_index >= upb_MessageDef_OneofCount(m)) {
      _upb_DefBuilder_Errf(ctx, "oneof_index out of range (%s)", f->full_name);
    }

    upb_OneofDef* oneof = (upb_OneofDef*)upb_MessageDef_Oneof(m, oneof_index);
    f->scope.oneof = oneof;
    parent_features = upb_OneofDef_ResolvedFeatures(oneof);

    _upb_OneofDef_Insert(ctx, oneof, f, name.data, name.size);
  }

  f->resolved_features = _upb_DefBuilder_DoResolveFeatures(
      ctx, parent_features, unresolved_features, implicit);

  f->label_ = (int)UPB_DESC(FieldDescriptorProto_label)(field_proto);
  if (f->label_ == kUpb_Label_Optional &&
      // TODO: remove once we can deprecate kUpb_Label_Required.
      UPB_DESC(FeatureSet_field_presence)(f->resolved_features) ==
          UPB_DESC(FeatureSet_LEGACY_REQUIRED)) {
    f->label_ = kUpb_Label_Required;
  }

  if (!UPB_DESC(FieldDescriptorProto_has_name)(field_proto)) {
    _upb_DefBuilder_Errf(ctx, "field has no name");
  }

  f->has_json_name = UPB_DESC(FieldDescriptorProto_has_json_name)(field_proto);
  if (f->has_json_name) {
    const upb_StringView sv =
        UPB_DESC(FieldDescriptorProto_json_name)(field_proto);
    f->json_name = upb_strdup2(sv.data, sv.size, ctx->arena);
  } else {
    f->json_name = make_json_name(name.data, name.size, ctx->arena);
  }
  if (!f->json_name) _upb_DefBuilder_OomErr(ctx);

  const bool has_type = UPB_DESC(FieldDescriptorProto_has_type)(field_proto);
  const bool has_type_name =
      UPB_DESC(FieldDescriptorProto_has_type_name)(field_proto);

  f->type_ = (int)UPB_DESC(FieldDescriptorProto_type)(field_proto);

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
  }

  if ((!has_type && has_type_name) || f->type_ == kUpb_FieldType_Message) {
    f->type_ =
        UPB_FIELD_TYPE_UNSPECIFIED;  // We'll assign this in resolve_subdef()
  } else {
    if (f->type_ < kUpb_FieldType_Double || f->type_ > kUpb_FieldType_SInt64) {
      _upb_DefBuilder_Errf(ctx, "invalid type for field %s (%d)", f->full_name,
                           f->type_);
    }
  }

  if (f->label_ < kUpb_Label_Optional || f->label_ > kUpb_Label_Repeated) {
    _upb_DefBuilder_Errf(ctx, "invalid label for field %s (%d)", f->full_name,
                         f->label_);
  }

  /* We can't resolve the subdef or (in the case of extensions) the containing
   * message yet, because it may not have been defined yet.  We stash a pointer
   * to the field_proto until later when we can properly resolve it. */
  f->sub.unresolved = field_proto;

  if (UPB_DESC(FieldDescriptorProto_has_oneof_index)(field_proto)) {
    if (upb_FieldDef_Label(f) != kUpb_Label_Optional) {
      _upb_DefBuilder_Errf(ctx, "fields in oneof must have OPTIONAL label (%s)",
                           f->full_name);
    }
  }

  f->has_presence =
      (!upb_FieldDef_IsRepeated(f)) &&
      (f->is_extension ||
       (f->type_ == kUpb_FieldType_Message ||
        f->type_ == kUpb_FieldType_Group || upb_FieldDef_ContainingOneof(f) ||
        UPB_DESC(FeatureSet_field_presence)(f->resolved_features) !=
            UPB_DESC(FeatureSet_IMPLICIT)));
}

static void _upb_FieldDef_CreateExt(upb_DefBuilder* ctx, const char* prefix,
                                    const UPB_DESC(FeatureSet*) parent_features,
                                    const UPB_DESC(FieldDescriptorProto*)
                                        field_proto,
                                    upb_MessageDef* m, upb_FieldDef* f) {
  f->is_extension = true;
  _upb_FieldDef_Create(ctx, prefix, parent_features, field_proto, m, f);

  if (UPB_DESC(FieldDescriptorProto_has_oneof_index)(field_proto)) {
    _upb_DefBuilder_Errf(ctx, "oneof_index provided for extension field (%s)",
                         f->full_name);
  }

  f->scope.extension_scope = m;
  _upb_DefBuilder_Add(ctx, f->full_name, _upb_DefType_Pack(f, UPB_DEFTYPE_EXT));
  f->layout_index = ctx->ext_count++;

  if (ctx->layout) {
    UPB_ASSERT(upb_MiniTableExtension_Number(
                   upb_FieldDef_MiniTableExtension(f)) == f->number_);
  }
}

static void _upb_FieldDef_CreateNotExt(upb_DefBuilder* ctx, const char* prefix,
                                       const UPB_DESC(FeatureSet*)
                                           parent_features,
                                       const UPB_DESC(FieldDescriptorProto*)
                                           field_proto,
                                       upb_MessageDef* m, upb_FieldDef* f) {
  f->is_extension = false;
  _upb_FieldDef_Create(ctx, prefix, parent_features, field_proto, m, f);

  if (!UPB_DESC(FieldDescriptorProto_has_oneof_index)(field_proto)) {
    if (f->is_proto3_optional) {
      _upb_DefBuilder_Errf(
          ctx,
          "non-extension field (%s) with proto3_optional was not in a oneof",
          f->full_name);
    }
  }

  _upb_MessageDef_InsertField(ctx, m, f);
}

upb_FieldDef* _upb_Extensions_New(upb_DefBuilder* ctx, int n,
                                  const UPB_DESC(FieldDescriptorProto*)
                                      const* protos,
                                  const UPB_DESC(FeatureSet*) parent_features,
                                  const char* prefix, upb_MessageDef* m) {
  _upb_DefType_CheckPadding(sizeof(upb_FieldDef));
  upb_FieldDef* defs = UPB_DEFBUILDER_ALLOCARRAY(ctx, upb_FieldDef, n);

  for (int i = 0; i < n; i++) {
    upb_FieldDef* f = &defs[i];

    _upb_FieldDef_CreateExt(ctx, prefix, parent_features, protos[i], m, f);
    f->index_ = i;
  }

  return defs;
}

upb_FieldDef* _upb_FieldDefs_New(upb_DefBuilder* ctx, int n,
                                 const UPB_DESC(FieldDescriptorProto*)
                                     const* protos,
                                 const UPB_DESC(FeatureSet*) parent_features,
                                 const char* prefix, upb_MessageDef* m,
                                 bool* is_sorted) {
  _upb_DefType_CheckPadding(sizeof(upb_FieldDef));
  upb_FieldDef* defs = UPB_DEFBUILDER_ALLOCARRAY(ctx, upb_FieldDef, n);

  uint32_t previous = 0;
  for (int i = 0; i < n; i++) {
    upb_FieldDef* f = &defs[i];

    _upb_FieldDef_CreateNotExt(ctx, prefix, parent_features, protos[i], m, f);
    f->index_ = i;
    if (!ctx->layout) {
      // Speculate that the def fields are sorted.  We will always sort the
      // MiniTable fields, so if defs are sorted then indices will match.
      //
      // If this is incorrect, we will overwrite later.
      f->layout_index = i;
    }

    const uint32_t current = f->number_;
    if (previous > current) *is_sorted = false;
    previous = current;
  }

  return defs;
}

static void resolve_subdef(upb_DefBuilder* ctx, const char* prefix,
                           upb_FieldDef* f) {
  const UPB_DESC(FieldDescriptorProto)* field_proto = f->sub.unresolved;
  upb_StringView name = UPB_DESC(FieldDescriptorProto_type_name)(field_proto);
  bool has_name = UPB_DESC(FieldDescriptorProto_has_type_name)(field_proto);
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
          f->type_ = kUpb_FieldType_Message;
          // TODO: remove once we can deprecate
          // kUpb_FieldType_Group.
          if (UPB_DESC(FeatureSet_message_encoding)(f->resolved_features) ==
                  UPB_DESC(FeatureSet_DELIMITED) &&
              !upb_MessageDef_IsMapEntry(def) &&
              !(f->msgdef && upb_MessageDef_IsMapEntry(f->msgdef))) {
            f->type_ = kUpb_FieldType_Group;
          }
          f->has_presence = !upb_FieldDef_IsRepeated(f);
          break;
        default:
          _upb_DefBuilder_Errf(ctx, "Couldn't resolve type name for field %s",
                               f->full_name);
      }
      break;
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

static int _upb_FieldDef_Compare(const void* p1, const void* p2) {
  const uint32_t v1 = (*(upb_FieldDef**)p1)->number_;
  const uint32_t v2 = (*(upb_FieldDef**)p2)->number_;
  return (v1 < v2) ? -1 : (v1 > v2);
}

// _upb_FieldDefs_Sorted() is mostly a pure function of its inputs, but has one
// critical side effect that we depend on: it sets layout_index appropriately
// for non-sorted lists of fields.
const upb_FieldDef** _upb_FieldDefs_Sorted(const upb_FieldDef* f, int n,
                                           upb_Arena* a) {
  // TODO: Replace this arena alloc with a persistent scratch buffer.
  upb_FieldDef** out = (upb_FieldDef**)upb_Arena_Malloc(a, n * sizeof(void*));
  if (!out) return NULL;

  for (int i = 0; i < n; i++) {
    out[i] = (upb_FieldDef*)&f[i];
  }
  qsort(out, n, sizeof(void*), _upb_FieldDef_Compare);

  for (int i = 0; i < n; i++) {
    out[i]->layout_index = i;
  }
  return (const upb_FieldDef**)out;
}

bool upb_FieldDef_MiniDescriptorEncode(const upb_FieldDef* f, upb_Arena* a,
                                       upb_StringView* out) {
  UPB_ASSERT(f->is_extension);

  upb_DescState s;
  _upb_DescState_Init(&s);

  const int number = upb_FieldDef_Number(f);
  const uint64_t modifiers = _upb_FieldDef_Modifiers(f);

  if (!_upb_DescState_Grow(&s, a)) return false;
  s.ptr = upb_MtDataEncoder_EncodeExtension(&s.e, s.ptr, f->type_, number,
                                            modifiers);
  *s.ptr = '\0';

  out->data = s.buf;
  out->size = s.ptr - s.buf;
  return true;
}

static void resolve_extension(upb_DefBuilder* ctx, const char* prefix,
                              upb_FieldDef* f,
                              const UPB_DESC(FieldDescriptorProto) *
                                  field_proto) {
  if (!UPB_DESC(FieldDescriptorProto_has_extendee)(field_proto)) {
    _upb_DefBuilder_Errf(ctx, "extension for field '%s' had no extendee",
                         f->full_name);
  }

  upb_StringView name = UPB_DESC(FieldDescriptorProto_extendee)(field_proto);
  const upb_MessageDef* m =
      _upb_DefBuilder_Resolve(ctx, f->full_name, prefix, name, UPB_DEFTYPE_MSG);
  f->msgdef = m;

  if (!_upb_MessageDef_IsValidExtensionNumber(m, f->number_)) {
    _upb_DefBuilder_Errf(
        ctx,
        "field number %u in extension %s has no extension range in message %s",
        (unsigned)f->number_, f->full_name, upb_MessageDef_FullName(m));
  }
}

void _upb_FieldDef_BuildMiniTableExtension(upb_DefBuilder* ctx,
                                           const upb_FieldDef* f) {
  const upb_MiniTableExtension* ext = upb_FieldDef_MiniTableExtension(f);

  if (ctx->layout) {
    UPB_ASSERT(upb_FieldDef_Number(f) == upb_MiniTableExtension_Number(ext));
  } else {
    upb_StringView desc;
    if (!upb_FieldDef_MiniDescriptorEncode(f, ctx->tmp_arena, &desc)) {
      _upb_DefBuilder_OomErr(ctx);
    }

    upb_MiniTableExtension* mut_ext = (upb_MiniTableExtension*)ext;
    upb_MiniTableSub sub = {NULL};
    if (upb_FieldDef_IsSubMessage(f)) {
      const upb_MiniTable* submsg = upb_MessageDef_MiniTable(f->sub.msgdef);
      sub = upb_MiniTableSub_FromMessage(submsg);
    } else if (_upb_FieldDef_IsClosedEnum(f)) {
      const upb_MiniTableEnum* subenum = _upb_EnumDef_MiniTable(f->sub.enumdef);
      sub = upb_MiniTableSub_FromEnum(subenum);
    }
    bool ok2 = _upb_MiniTableExtension_Init(desc.data, desc.size, mut_ext,
                                            upb_MessageDef_MiniTable(f->msgdef),
                                            sub, ctx->platform, ctx->status);
    if (!ok2) _upb_DefBuilder_Errf(ctx, "Could not build extension mini table");
  }

  bool ok = _upb_DefPool_InsertExt(ctx->symtab, ext, f);
  if (!ok) _upb_DefBuilder_OomErr(ctx);
}

static void resolve_default(upb_DefBuilder* ctx, upb_FieldDef* f,
                            const UPB_DESC(FieldDescriptorProto) *
                                field_proto) {
  // Have to delay resolving of the default value until now because of the enum
  // case, since enum defaults are specified with a label.
  if (UPB_DESC(FieldDescriptorProto_has_default_value)(field_proto)) {
    upb_StringView defaultval =
        UPB_DESC(FieldDescriptorProto_default_value)(field_proto);

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
  const UPB_DESC(FieldDescriptorProto)* field_proto = f->sub.unresolved;

  resolve_subdef(ctx, prefix, f);
  resolve_default(ctx, f, field_proto);

  if (f->is_extension) {
    resolve_extension(ctx, prefix, f, field_proto);
  }
}
