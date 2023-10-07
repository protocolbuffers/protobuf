// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/reflection/def.hpp"
#include "upb/wire/types.h"
#include "upb_generator/common.h"
#include "upb_generator/file_layout.h"
#include "upb_generator/names.h"
#include "upb_generator/plugin.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace generator {
namespace {

struct Options {
  bool bootstrap = false;
};

std::string SourceFilename(upb::FileDefPtr file) {
  return StripExtension(file.name()) + ".upb.c";
}

std::string MessageMiniTableRef(upb::MessageDefPtr descriptor,
                                const Options& options) {
  if (options.bootstrap) {
    return absl::StrCat(MessageInitName(descriptor), "()");
  } else {
    return absl::StrCat("&", MessageInitName(descriptor));
  }
}

std::string EnumInitName(upb::EnumDefPtr descriptor) {
  return ToCIdent(descriptor.full_name()) + "_enum_init";
}

std::string EnumMiniTableRef(upb::EnumDefPtr descriptor,
                             const Options& options) {
  if (options.bootstrap) {
    return absl::StrCat(EnumInitName(descriptor), "()");
  } else {
    return absl::StrCat("&", EnumInitName(descriptor));
  }
}

std::string ExtensionIdentBase(upb::FieldDefPtr ext) {
  assert(ext.is_extension());
  std::string ext_scope;
  if (ext.extension_scope()) {
    return MessageName(ext.extension_scope());
  } else {
    return ToCIdent(ext.file().package());
  }
}

std::string ExtensionLayout(upb::FieldDefPtr ext) {
  return absl::StrCat(ExtensionIdentBase(ext), "_", ext.name(), "_ext");
}

std::string EnumValueSymbol(upb::EnumValDefPtr value) {
  return ToCIdent(value.full_name());
}

std::string CTypeInternal(upb::FieldDefPtr field, bool is_const) {
  std::string maybe_const = is_const ? "const " : "";
  switch (field.ctype()) {
    case kUpb_CType_Message: {
      std::string maybe_struct =
          field.file() != field.message_type().file() ? "struct " : "";
      return maybe_const + maybe_struct + MessageName(field.message_type()) +
             "*";
    }
    case kUpb_CType_Bool:
      return "bool";
    case kUpb_CType_Float:
      return "float";
    case kUpb_CType_Int32:
    case kUpb_CType_Enum:
      return "int32_t";
    case kUpb_CType_UInt32:
      return "uint32_t";
    case kUpb_CType_Double:
      return "double";
    case kUpb_CType_Int64:
      return "int64_t";
    case kUpb_CType_UInt64:
      return "uint64_t";
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      return "upb_StringView";
    default:
      abort();
  }
}

std::string FloatToCLiteral(float value) {
  if (value == std::numeric_limits<float>::infinity()) {
    return "kUpb_FltInfinity";
  } else if (value == -std::numeric_limits<float>::infinity()) {
    return "-kUpb_FltInfinity";
  } else if (std::isnan(value)) {
    return "kUpb_NaN";
  } else {
    return absl::StrCat(value);
  }
}

std::string DoubleToCLiteral(double value) {
  if (value == std::numeric_limits<double>::infinity()) {
    return "kUpb_Infinity";
  } else if (value == -std::numeric_limits<double>::infinity()) {
    return "-kUpb_Infinity";
  } else if (std::isnan(value)) {
    return "kUpb_NaN";
  } else {
    return absl::StrCat(value);
  }
}

// Escape trigraphs by escaping question marks to \?
std::string EscapeTrigraphs(absl::string_view to_escape) {
  return absl::StrReplaceAll(to_escape, {{"?", "\\?"}});
}

std::string FieldDefault(upb::FieldDefPtr field) {
  switch (field.ctype()) {
    case kUpb_CType_Message:
      return "NULL";
    case kUpb_CType_Bytes:
    case kUpb_CType_String: {
      upb_StringView str = field.default_value().str_val;
      return absl::Substitute("upb_StringView_FromString(\"$0\")",
                              EscapeTrigraphs(absl::CEscape(
                                  absl::string_view(str.data, str.size))));
    }
    case kUpb_CType_Int32:
      return absl::Substitute("(int32_t)$0", field.default_value().int32_val);
    case kUpb_CType_Int64:
      if (field.default_value().int64_val == INT64_MIN) {
        // Special-case to avoid:
        //   integer literal is too large to be represented in a signed integer
        //   type, interpreting as unsigned
        //   [-Werror,-Wimplicitly-unsigned-literal]
        //   int64_t default_val = (int64_t)-9223372036854775808ll;
        //
        // More info here: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52661
        return "INT64_MIN";
      } else {
        return absl::Substitute("(int64_t)$0ll",
                                field.default_value().int64_val);
      }
    case kUpb_CType_UInt32:
      return absl::Substitute("(uint32_t)$0u",
                              field.default_value().uint32_val);
    case kUpb_CType_UInt64:
      return absl::Substitute("(uint64_t)$0ull",
                              field.default_value().uint64_val);
    case kUpb_CType_Float:
      return FloatToCLiteral(field.default_value().float_val);
    case kUpb_CType_Double:
      return DoubleToCLiteral(field.default_value().double_val);
    case kUpb_CType_Bool:
      return field.default_value().bool_val ? "true" : "false";
    case kUpb_CType_Enum:
      // Use a number instead of a symbolic name so that we don't require
      // this enum's header to be included.
      return absl::StrCat(field.default_value().int32_val);
  }
  ABSL_ASSERT(false);
  return "XXX";
}

std::string CType(upb::FieldDefPtr field) {
  return CTypeInternal(field, false);
}

std::string CTypeConst(upb::FieldDefPtr field) {
  return CTypeInternal(field, true);
}

std::string MapKeyCType(upb::FieldDefPtr map_field) {
  return CType(map_field.message_type().map_key());
}

std::string MapValueCType(upb::FieldDefPtr map_field) {
  return CType(map_field.message_type().map_value());
}

std::string MapKeySize(upb::FieldDefPtr map_field, absl::string_view expr) {
  return map_field.message_type().map_key().ctype() == kUpb_CType_String
             ? "0"
             : absl::StrCat("sizeof(", expr, ")");
}

std::string MapValueSize(upb::FieldDefPtr map_field, absl::string_view expr) {
  return map_field.message_type().map_value().ctype() == kUpb_CType_String
             ? "0"
             : absl::StrCat("sizeof(", expr, ")");
}

std::string FieldInitializer(const DefPoolPair& pools, upb::FieldDefPtr field,
                             const Options& options);

void DumpEnumValues(upb::EnumDefPtr desc, Output& output) {
  std::vector<upb::EnumValDefPtr> values;
  values.reserve(desc.value_count());
  for (int i = 0; i < desc.value_count(); i++) {
    values.push_back(desc.value(i));
  }
  std::sort(values.begin(), values.end(),
            [](upb::EnumValDefPtr a, upb::EnumValDefPtr b) {
              return a.number() < b.number();
            });

  for (size_t i = 0; i < values.size(); i++) {
    auto value = values[i];
    output("  $0 = $1", EnumValueSymbol(value), value.number());
    if (i != values.size() - 1) {
      output(",");
    }
    output("\n");
  }
}

std::string GetFieldRep(const DefPoolPair& pools, upb::FieldDefPtr field) {
  return upb::generator::GetFieldRep(pools.GetField32(field),
                                     pools.GetField64(field));
}

void GenerateExtensionInHeader(const DefPoolPair& pools, upb::FieldDefPtr ext,
                               Output& output) {
  output(
      R"cc(
        UPB_INLINE bool $0_has_$1(const struct $2* msg) {
          return _upb_Message_HasExtensionField(msg, &$3);
        }
      )cc",
      ExtensionIdentBase(ext), ext.name(), MessageName(ext.containing_type()),
      ExtensionLayout(ext));

  output(
      R"cc(
        UPB_INLINE void $0_clear_$1(struct $2* msg) {
          _upb_Message_ClearExtensionField(msg, &$3);
        }
      )cc",
      ExtensionIdentBase(ext), ext.name(), MessageName(ext.containing_type()),
      ExtensionLayout(ext));

  if (ext.IsSequence()) {
    // TODO: We need generated accessors for repeated extensions.
  } else {
    output(
        R"cc(
          UPB_INLINE $0 $1_$2(const struct $3* msg) {
            const upb_MiniTableExtension* ext = &$4;
            UPB_ASSUME(!upb_IsRepeatedOrMap(&ext->field));
            UPB_ASSUME(_upb_MiniTableField_GetRep(&ext->field) == $5);
            $0 default_val = $6;
            $0 ret;
            _upb_Message_GetExtensionField(msg, ext, &default_val, &ret);
            return ret;
          }
        )cc",
        CTypeConst(ext), ExtensionIdentBase(ext), ext.name(),
        MessageName(ext.containing_type()), ExtensionLayout(ext),
        GetFieldRep(pools, ext), FieldDefault(ext));
    output(
        R"cc(
          UPB_INLINE void $1_set_$2(struct $3* msg, $0 val, upb_Arena* arena) {
            const upb_MiniTableExtension* ext = &$4;
            UPB_ASSUME(!upb_IsRepeatedOrMap(&ext->field));
            UPB_ASSUME(_upb_MiniTableField_GetRep(&ext->field) == $5);
            bool ok = _upb_Message_SetExtensionField(msg, ext, &val, arena);
            UPB_ASSERT(ok);
          }
        )cc",
        CTypeConst(ext), ExtensionIdentBase(ext), ext.name(),
        MessageName(ext.containing_type()), ExtensionLayout(ext),
        GetFieldRep(pools, ext));
  }
}

void GenerateMessageFunctionsInHeader(upb::MessageDefPtr message,
                                      const Options& options, Output& output) {
  // TODO: The generated code here does not check the return values
  // from upb_Encode(). How can we even fix this without breaking other things?
  output(
      R"cc(
        UPB_INLINE $0* $0_new(upb_Arena* arena) {
          return ($0*)_upb_Message_New($1, arena);
        }
        UPB_INLINE $0* $0_parse(const char* buf, size_t size, upb_Arena* arena) {
          $0* ret = $0_new(arena);
          if (!ret) return NULL;
          if (upb_Decode(buf, size, ret, $1, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
            return NULL;
          }
          return ret;
        }
        UPB_INLINE $0* $0_parse_ex(const char* buf, size_t size,
                                   const upb_ExtensionRegistry* extreg,
                                   int options, upb_Arena* arena) {
          $0* ret = $0_new(arena);
          if (!ret) return NULL;
          if (upb_Decode(buf, size, ret, $1, extreg, options, arena) !=
              kUpb_DecodeStatus_Ok) {
            return NULL;
          }
          return ret;
        }
        UPB_INLINE char* $0_serialize(const $0* msg, upb_Arena* arena, size_t* len) {
          char* ptr;
          (void)upb_Encode(msg, $1, 0, arena, &ptr, len);
          return ptr;
        }
        UPB_INLINE char* $0_serialize_ex(const $0* msg, int options,
                                         upb_Arena* arena, size_t* len) {
          char* ptr;
          (void)upb_Encode(msg, $1, options, arena, &ptr, len);
          return ptr;
        }
      )cc",
      MessageName(message), MessageMiniTableRef(message, options));
}

void GenerateOneofInHeader(upb::OneofDefPtr oneof, const DefPoolPair& pools,
                           absl::string_view msg_name, const Options& options,
                           Output& output) {
  std::string fullname = ToCIdent(oneof.full_name());
  output("typedef enum {\n");
  for (int j = 0; j < oneof.field_count(); j++) {
    upb::FieldDefPtr field = oneof.field(j);
    output("  $0_$1 = $2,\n", fullname, field.name(), field.number());
  }
  output(
      "  $0_NOT_SET = 0\n"
      "} $0_oneofcases;\n",
      fullname);
  output(
      R"cc(
        UPB_INLINE $0_oneofcases $1_$2_case(const $1* msg) {
          const upb_MiniTableField field = $3;
          return ($0_oneofcases)upb_Message_WhichOneofFieldNumber(msg, &field);
        }
      )cc",
      fullname, msg_name, oneof.name(),
      FieldInitializer(pools, oneof.field(0), options));
}

void GenerateHazzer(upb::FieldDefPtr field, const DefPoolPair& pools,
                    absl::string_view msg_name,
                    const NameToFieldDefMap& field_names,
                    const Options& options, Output& output) {
  std::string resolved_name = ResolveFieldName(field, field_names);
  if (field.has_presence()) {
    output(
        R"cc(
          UPB_INLINE bool $0_has_$1(const $0* msg) {
            const upb_MiniTableField field = $2;
            return _upb_Message_HasNonExtensionField(msg, &field);
          }
        )cc",
        msg_name, resolved_name, FieldInitializer(pools, field, options));
  } else if (field.IsMap()) {
    // Do nothing.
  } else if (field.IsSequence()) {
    // TODO: remove.
    output(
        R"cc(
          UPB_INLINE bool $0_has_$1(const $0* msg) {
            size_t size;
            $0_$1(msg, &size);
            return size != 0;
          }
        )cc",
        msg_name, resolved_name);
  }
}

void GenerateClear(upb::FieldDefPtr field, const DefPoolPair& pools,
                   absl::string_view msg_name,
                   const NameToFieldDefMap& field_names, const Options& options,
                   Output& output) {
  if (field == field.containing_type().map_key() ||
      field == field.containing_type().map_value()) {
    // Cannot be cleared.
    return;
  }
  std::string resolved_name = ResolveFieldName(field, field_names);
  output(
      R"cc(
        UPB_INLINE void $0_clear_$1($0* msg) {
          const upb_MiniTableField field = $2;
          _upb_Message_ClearNonExtensionField(msg, &field);
        }
      )cc",
      msg_name, resolved_name, FieldInitializer(pools, field, options));
}

void GenerateMapGetters(upb::FieldDefPtr field, const DefPoolPair& pools,
                        absl::string_view msg_name,
                        const NameToFieldDefMap& field_names,
                        const Options& options, Output& output) {
  std::string resolved_name = ResolveFieldName(field, field_names);
  output(
      R"cc(
        UPB_INLINE size_t $0_$1_size(const $0* msg) {
          const upb_MiniTableField field = $2;
          const upb_Map* map = upb_Message_GetMap(msg, &field);
          return map ? _upb_Map_Size(map) : 0;
        }
      )cc",
      msg_name, resolved_name, FieldInitializer(pools, field, options));
  output(
      R"cc(
        UPB_INLINE bool $0_$1_get(const $0* msg, $2 key, $3* val) {
          const upb_MiniTableField field = $4;
          const upb_Map* map = upb_Message_GetMap(msg, &field);
          if (!map) return false;
          return _upb_Map_Get(map, &key, $5, val, $6);
        }
      )cc",
      msg_name, resolved_name, MapKeyCType(field), MapValueCType(field),
      FieldInitializer(pools, field, options), MapKeySize(field, "key"),
      MapValueSize(field, "*val"));
  output(
      R"cc(
        UPB_INLINE $0 $1_$2_next(const $1* msg, size_t* iter) {
          const upb_MiniTableField field = $3;
          const upb_Map* map = upb_Message_GetMap(msg, &field);
          if (!map) return NULL;
          return ($0)_upb_map_next(map, iter);
        }
      )cc",
      CTypeConst(field), msg_name, resolved_name,
      FieldInitializer(pools, field, options));
}

void GenerateMapEntryGetters(upb::FieldDefPtr field, absl::string_view msg_name,
                             Output& output) {
  output(
      R"cc(
        UPB_INLINE $0 $1_$2(const $1* msg) {
          $3 ret;
          _upb_msg_map_$2(msg, &ret, $4);
          return ret;
        }
      )cc",
      CTypeConst(field), msg_name, field.name(), CType(field),
      field.ctype() == kUpb_CType_String ? "0" : "sizeof(ret)");
}

void GenerateRepeatedGetters(upb::FieldDefPtr field, const DefPoolPair& pools,
                             absl::string_view msg_name,
                             const NameToFieldDefMap& field_names,
                             const Options& options, Output& output) {
  // Generate getter returning first item and size.
  //
  // Example:
  //   UPB_INLINE const struct Bar* const* name(const Foo* msg, size_t* size)
  output(
      R"cc(
        UPB_INLINE $0 const* $1_$2(const $1* msg, size_t* size) {
          const upb_MiniTableField field = $3;
          const upb_Array* arr = upb_Message_GetArray(msg, &field);
          if (arr) {
            if (size) *size = arr->size;
            return ($0 const*)_upb_array_constptr(arr);
          } else {
            if (size) *size = 0;
            return NULL;
          }
        }
      )cc",
      CTypeConst(field),                       // $0
      msg_name,                                // $1
      ResolveFieldName(field, field_names),    // $2
      FieldInitializer(pools, field, options)  // #3
  );
  // Generate private getter returning array or NULL for immutable and upb_Array
  // for mutable.
  //
  // Example:
  //   UPB_INLINE const upb_Array* _name_upbarray(size_t* size)
  //   UPB_INLINE upb_Array* _name_mutable_upbarray(size_t* size)
  output(
      R"cc(
        UPB_INLINE const upb_Array* _$1_$2_$4(const $1* msg, size_t* size) {
          const upb_MiniTableField field = $3;
          const upb_Array* arr = upb_Message_GetArray(msg, &field);
          if (size) {
            *size = arr ? arr->size : 0;
          }
          return arr;
        }
        UPB_INLINE upb_Array* _$1_$2_$5(const $1* msg, size_t* size, upb_Arena* arena) {
          const upb_MiniTableField field = $3;
          upb_Array* arr = upb_Message_GetOrCreateMutableArray(
              (upb_Message*)msg, &field, arena);
          if (size) {
            *size = arr ? arr->size : 0;
          }
          return arr;
        }
      )cc",
      CTypeConst(field),                        // $0
      msg_name,                                 // $1
      ResolveFieldName(field, field_names),     // $2
      FieldInitializer(pools, field, options),  // $3
      kRepeatedFieldArrayGetterPostfix,         // $4
      kRepeatedFieldMutableArrayGetterPostfix   // $5
  );
}

void GenerateScalarGetters(upb::FieldDefPtr field, const DefPoolPair& pools,
                           absl::string_view msg_name,
                           const NameToFieldDefMap& field_names,
                           const Options& Options, Output& output) {
  std::string field_name = ResolveFieldName(field, field_names);
  output(
      R"cc(
        UPB_INLINE $0 $1_$2(const $1* msg) {
          $0 default_val = $3;
          $0 ret;
          const upb_MiniTableField field = $4;
          _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
          return ret;
        }
      )cc",
      CTypeConst(field), msg_name, field_name, FieldDefault(field),
      FieldInitializer(pools, field, Options));
}

void GenerateGetters(upb::FieldDefPtr field, const DefPoolPair& pools,
                     absl::string_view msg_name,
                     const NameToFieldDefMap& field_names,
                     const Options& options, Output& output) {
  if (field.IsMap()) {
    GenerateMapGetters(field, pools, msg_name, field_names, options, output);
  } else if (UPB_DESC(MessageOptions_map_entry)(
                 field.containing_type().options())) {
    GenerateMapEntryGetters(field, msg_name, output);
  } else if (field.IsSequence()) {
    GenerateRepeatedGetters(field, pools, msg_name, field_names, options,
                            output);
  } else {
    GenerateScalarGetters(field, pools, msg_name, field_names, options, output);
  }
}

void GenerateMapSetters(upb::FieldDefPtr field, const DefPoolPair& pools,
                        absl::string_view msg_name,
                        const NameToFieldDefMap& field_names,
                        const Options& options, Output& output) {
  std::string resolved_name = ResolveFieldName(field, field_names);
  output(
      R"cc(
        UPB_INLINE void $0_$1_clear($0* msg) {
          const upb_MiniTableField field = $2;
          upb_Map* map = (upb_Map*)upb_Message_GetMap(msg, &field);
          if (!map) return;
          _upb_Map_Clear(map);
        }
      )cc",
      msg_name, resolved_name, FieldInitializer(pools, field, options));
  output(
      R"cc(
        UPB_INLINE bool $0_$1_set($0* msg, $2 key, $3 val, upb_Arena* a) {
          const upb_MiniTableField field = $4;
          upb_Map* map = _upb_Message_GetOrCreateMutableMap(msg, &field, $5, $6, a);
          return _upb_Map_Insert(map, &key, $5, &val, $6, a) !=
                 kUpb_MapInsertStatus_OutOfMemory;
        }
      )cc",
      msg_name, resolved_name, MapKeyCType(field), MapValueCType(field),
      FieldInitializer(pools, field, options), MapKeySize(field, "key"),
      MapValueSize(field, "val"));
  output(
      R"cc(
        UPB_INLINE bool $0_$1_delete($0* msg, $2 key) {
          const upb_MiniTableField field = $3;
          upb_Map* map = (upb_Map*)upb_Message_GetMap(msg, &field);
          if (!map) return false;
          return _upb_Map_Delete(map, &key, $4, NULL);
        }
      )cc",
      msg_name, resolved_name, MapKeyCType(field),
      FieldInitializer(pools, field, options), MapKeySize(field, "key"));
  output(
      R"cc(
        UPB_INLINE $0 $1_$2_nextmutable($1* msg, size_t* iter) {
          const upb_MiniTableField field = $3;
          upb_Map* map = (upb_Map*)upb_Message_GetMap(msg, &field);
          if (!map) return NULL;
          return ($0)_upb_map_next(map, iter);
        }
      )cc",
      CType(field), msg_name, resolved_name,
      FieldInitializer(pools, field, options));
}

void GenerateRepeatedSetters(upb::FieldDefPtr field, const DefPoolPair& pools,
                             absl::string_view msg_name,
                             const NameToFieldDefMap& field_names,
                             const Options& options, Output& output) {
  std::string resolved_name = ResolveFieldName(field, field_names);
  output(
      R"cc(
        UPB_INLINE $0* $1_mutable_$2($1* msg, size_t* size) {
          upb_MiniTableField field = $3;
          upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
          if (arr) {
            if (size) *size = arr->size;
            return ($0*)_upb_array_ptr(arr);
          } else {
            if (size) *size = 0;
            return NULL;
          }
        }
      )cc",
      CType(field), msg_name, resolved_name,
      FieldInitializer(pools, field, options));
  output(
      R"cc(
        UPB_INLINE $0* $1_resize_$2($1* msg, size_t size, upb_Arena* arena) {
          upb_MiniTableField field = $3;
          return ($0*)upb_Message_ResizeArrayUninitialized(msg, &field, size, arena);
        }
      )cc",
      CType(field), msg_name, resolved_name,
      FieldInitializer(pools, field, options));
  if (field.ctype() == kUpb_CType_Message) {
    output(
        R"cc(
          UPB_INLINE struct $0* $1_add_$2($1* msg, upb_Arena* arena) {
            upb_MiniTableField field = $4;
            upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
            if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
              return NULL;
            }
            struct $0* sub = (struct $0*)_upb_Message_New($3, arena);
            if (!arr || !sub) return NULL;
            _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
            return sub;
          }
        )cc",
        MessageName(field.message_type()), msg_name, resolved_name,
        MessageMiniTableRef(field.message_type(), options),
        FieldInitializer(pools, field, options));
  } else {
    output(
        R"cc(
          UPB_INLINE bool $1_add_$2($1* msg, $0 val, upb_Arena* arena) {
            upb_MiniTableField field = $3;
            upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
            if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
              return false;
            }
            _upb_Array_Set(arr, arr->size - 1, &val, sizeof(val));
            return true;
          }
        )cc",
        CType(field), msg_name, resolved_name,
        FieldInitializer(pools, field, options));
  }
}

void GenerateNonRepeatedSetters(upb::FieldDefPtr field,
                                const DefPoolPair& pools,
                                absl::string_view msg_name,
                                const NameToFieldDefMap& field_names,
                                const Options& options, Output& output) {
  if (field == field.containing_type().map_key()) {
    // Key cannot be mutated.
    return;
  }

  std::string field_name = ResolveFieldName(field, field_names);

  if (field == field.containing_type().map_value()) {
    output(R"cc(
             UPB_INLINE void $0_set_$1($0 *msg, $2 value) {
               _upb_msg_map_set_value(msg, &value, $3);
             }
           )cc",
           msg_name, field_name, CType(field),
           field.ctype() == kUpb_CType_String ? "0"
                                              : "sizeof(" + CType(field) + ")");
  } else {
    output(R"cc(
             UPB_INLINE void $0_set_$1($0 *msg, $2 value) {
               const upb_MiniTableField field = $3;
               _upb_Message_SetNonExtensionField(msg, &field, &value);
             }
           )cc",
           msg_name, field_name, CType(field),
           FieldInitializer(pools, field, options));
  }

  // Message fields also have a Msg_mutable_foo() accessor that will create
  // the sub-message if it doesn't already exist.
  if (field.ctype() == kUpb_CType_Message &&
      !UPB_DESC(MessageOptions_map_entry)(field.containing_type().options())) {
    output(
        R"cc(
          UPB_INLINE struct $0* $1_mutable_$2($1* msg, upb_Arena* arena) {
            struct $0* sub = (struct $0*)$1_$2(msg);
            if (sub == NULL) {
              sub = (struct $0*)_upb_Message_New($3, arena);
              if (sub) $1_set_$2(msg, sub);
            }
            return sub;
          }
        )cc",
        MessageName(field.message_type()), msg_name, field_name,
        MessageMiniTableRef(field.message_type(), options));
  }
}

void GenerateSetters(upb::FieldDefPtr field, const DefPoolPair& pools,
                     absl::string_view msg_name,
                     const NameToFieldDefMap& field_names,
                     const Options& options, Output& output) {
  if (field.IsMap()) {
    GenerateMapSetters(field, pools, msg_name, field_names, options, output);
  } else if (field.IsSequence()) {
    GenerateRepeatedSetters(field, pools, msg_name, field_names, options,
                            output);
  } else {
    GenerateNonRepeatedSetters(field, pools, msg_name, field_names, options,
                               output);
  }
}

void GenerateMessageInHeader(upb::MessageDefPtr message,
                             const DefPoolPair& pools, const Options& options,
                             Output& output) {
  output("/* $0 */\n\n", message.full_name());
  std::string msg_name = ToCIdent(message.full_name());
  if (!UPB_DESC(MessageOptions_map_entry)(message.options())) {
    GenerateMessageFunctionsInHeader(message, options, output);
  }

  for (int i = 0; i < message.real_oneof_count(); i++) {
    GenerateOneofInHeader(message.oneof(i), pools, msg_name, options, output);
  }

  auto field_names = CreateFieldNameMap(message);
  for (auto field : FieldNumberOrder(message)) {
    GenerateClear(field, pools, msg_name, field_names, options, output);
    GenerateGetters(field, pools, msg_name, field_names, options, output);
    GenerateHazzer(field, pools, msg_name, field_names, options, output);
  }

  output("\n");

  for (auto field : FieldNumberOrder(message)) {
    GenerateSetters(field, pools, msg_name, field_names, options, output);
  }

  output("\n");
}

void ForwardDeclareMiniTableInit(upb::MessageDefPtr message,
                                 const Options& options, Output& output) {
  if (options.bootstrap) {
    output("extern const upb_MiniTable* $0();\n", MessageInitName(message));
  } else {
    output("extern const upb_MiniTable $0;\n", MessageInitName(message));
  }
}

std::vector<upb::MessageDefPtr> SortedForwardMessages(
    const std::vector<upb::MessageDefPtr>& this_file_messages,
    const std::vector<upb::FieldDefPtr>& this_file_exts) {
  std::map<std::string, upb::MessageDefPtr> forward_messages;
  for (auto message : this_file_messages) {
    for (int i = 0; i < message.field_count(); i++) {
      upb::FieldDefPtr field = message.field(i);
      if (field.ctype() == kUpb_CType_Message &&
          field.file() != field.message_type().file()) {
        forward_messages[field.message_type().full_name()] =
            field.message_type();
      }
    }
  }
  for (auto ext : this_file_exts) {
    if (ext.file() != ext.containing_type().file()) {
      forward_messages[ext.containing_type().full_name()] =
          ext.containing_type();
    }
  }
  std::vector<upb::MessageDefPtr> ret;
  ret.reserve(forward_messages.size());
  for (const auto& pair : forward_messages) {
    ret.push_back(pair.second);
  }
  return ret;
}

void WriteHeader(const DefPoolPair& pools, upb::FileDefPtr file,
                 const Options& options, Output& output) {
  const std::vector<upb::MessageDefPtr> this_file_messages =
      SortedMessages(file);
  const std::vector<upb::FieldDefPtr> this_file_exts = SortedExtensions(file);
  std::vector<upb::EnumDefPtr> this_file_enums = SortedEnums(file);
  std::vector<upb::MessageDefPtr> forward_messages =
      SortedForwardMessages(this_file_messages, this_file_exts);

  EmitFileWarning(file.name(), output);
  output(
      "#ifndef $0_UPB_H_\n"
      "#define $0_UPB_H_\n\n"
      "#include \"upb/generated_code_support.h\"\n\n",
      ToPreproc(file.name()));

  for (int i = 0; i < file.public_dependency_count(); i++) {
    if (i == 0) {
      output("/* Public Imports. */\n");
    }
    output("#include \"$0\"\n", CApiHeaderFilename(file.public_dependency(i)));
  }
  if (file.public_dependency_count() > 0) {
    output("\n");
  }

  if (!options.bootstrap) {
    output("#include \"$0\"\n\n", MiniTableHeaderFilename(file));
    for (int i = 0; i < file.dependency_count(); i++) {
      output("#include \"$0\"\n", MiniTableHeaderFilename(file.dependency(i)));
    }
    if (file.dependency_count() > 0) {
      output("\n");
    }
  }

  output(
      "// Must be last.\n"
      "#include \"upb/port/def.inc\"\n"
      "\n"
      "#ifdef __cplusplus\n"
      "extern \"C\" {\n"
      "#endif\n"
      "\n");

  if (options.bootstrap) {
    for (auto message : this_file_messages) {
      output("extern const upb_MiniTable* $0();\n", MessageInitName(message));
    }
    for (auto message : forward_messages) {
      output("extern const upb_MiniTable* $0();\n", MessageInitName(message));
    }
    for (auto enumdesc : this_file_enums) {
      output("extern const upb_MiniTableEnum* $0();\n", EnumInit(enumdesc));
    }
    output("\n");
  }

  // Forward-declare types defined in this file.
  for (auto message : this_file_messages) {
    output("typedef struct $0 $0;\n", ToCIdent(message.full_name()));
  }

  // Forward-declare types not in this file, but used as submessages.
  // Order by full name for consistent ordering.
  for (auto msg : forward_messages) {
    output("struct $0;\n", MessageName(msg));
  }

  if (!this_file_messages.empty()) {
    output("\n");
  }

  for (auto enumdesc : this_file_enums) {
    output("typedef enum {\n");
    DumpEnumValues(enumdesc, output);
    output("} $0;\n\n", ToCIdent(enumdesc.full_name()));
  }

  output("\n");

  output("\n");
  for (auto message : this_file_messages) {
    GenerateMessageInHeader(message, pools, options, output);
  }

  for (auto ext : this_file_exts) {
    GenerateExtensionInHeader(pools, ext, output);
  }

  if (absl::string_view(file.name()) == "google/protobuf/descriptor.proto" ||
      absl::string_view(file.name()) == "net/proto2/proto/descriptor.proto") {
    // This is gratuitously inefficient with how many times it rebuilds
    // MessageLayout objects for the same message. But we only do this for one
    // proto (descriptor.proto) so we don't worry about it.
    upb::MessageDefPtr max32_message;
    upb::MessageDefPtr max64_message;
    size_t max32 = 0;
    size_t max64 = 0;
    for (const auto message : this_file_messages) {
      if (absl::EndsWith(message.name(), "Options")) {
        size_t size32 = pools.GetMiniTable32(message)->size;
        size_t size64 = pools.GetMiniTable64(message)->size;
        if (size32 > max32) {
          max32 = size32;
          max32_message = message;
        }
        if (size64 > max64) {
          max64 = size64;
          max64_message = message;
        }
      }
    }

    output("/* Max size 32 is $0 */\n", max32_message.full_name());
    output("/* Max size 64 is $0 */\n", max64_message.full_name());
    output("#define _UPB_MAXOPT_SIZE UPB_SIZE($0, $1)\n\n", max32, max64);
  }

  output(
      "#ifdef __cplusplus\n"
      "}  /* extern \"C\" */\n"
      "#endif\n"
      "\n"
      "#include \"upb/port/undef.inc\"\n"
      "\n"
      "#endif  /* $0_UPB_H_ */\n",
      ToPreproc(file.name()));
}

std::string FieldInitializer(upb::FieldDefPtr field,
                             const upb_MiniTableField* field64,
                             const upb_MiniTableField* field32,
                             const Options& options) {
  if (options.bootstrap) {
    ABSL_CHECK(!field.is_extension());
    return absl::Substitute(
        "*upb_MiniTable_FindFieldByNumber($0, $1)",
        MessageMiniTableRef(field.containing_type(), options), field.number());
  } else {
    return upb::generator::FieldInitializer(field, field64, field32);
  }
}

std::string FieldInitializer(const DefPoolPair& pools, upb::FieldDefPtr field,
                             const Options& options) {
  return FieldInitializer(field, pools.GetField64(field),
                          pools.GetField32(field), options);
}

void WriteMessageMiniDescriptorInitializer(upb::MessageDefPtr msg,
                                           const Options& options,
                                           Output& output) {
  Output resolve_calls;
  for (int i = 0; i < msg.field_count(); i++) {
    upb::FieldDefPtr field = msg.field(i);
    if (!field.message_type() && !field.enum_subdef()) continue;
    if (field.message_type()) {
      resolve_calls(
          "upb_MiniTable_SetSubMessage(mini_table, "
          "(upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, "
          "$0), $1);\n  ",
          field.number(), MessageMiniTableRef(field.message_type(), options));
    } else if (field.enum_subdef() && field.enum_subdef().is_closed()) {
      resolve_calls(
          "upb_MiniTable_SetSubEnum(mini_table, "
          "(upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, "
          "$0), $1);\n  ",
          field.number(), EnumMiniTableRef(field.enum_subdef(), options));
    }
  }

  output(
      R"cc(
        const upb_MiniTable* $0() {
          static upb_MiniTable* mini_table = NULL;
          static const char* mini_descriptor = "$1";
          if (mini_table) return mini_table;
          mini_table =
              upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                                  upb_BootstrapArena(), NULL);
          $2return mini_table;
        }
      )cc",
      MessageInitName(msg), msg.MiniDescriptorEncode(), resolve_calls.output());
  output("\n");
}

void WriteEnumMiniDescriptorInitializer(upb::EnumDefPtr enum_def,
                                        const Options& options,
                                        Output& output) {
  output(
      R"cc(
        const upb_MiniTableEnum* $0() {
          static const upb_MiniTableEnum* mini_table = NULL;
          static const char* mini_descriptor = "$1";
          if (mini_table) return mini_table;
          mini_table =
              upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                                      upb_BootstrapArena(), NULL);
          return mini_table;
        }
      )cc",
      EnumInitName(enum_def), enum_def.MiniDescriptorEncode());
  output("\n");
}

void WriteMiniDescriptorSource(const DefPoolPair& pools, upb::FileDefPtr file,
                               const Options& options, Output& output) {
  output(
      "#include <stddef.h>\n"
      "#include \"upb/generated_code_support.h\"\n"
      "#include \"$0\"\n\n",
      CApiHeaderFilename(file));

  for (int i = 0; i < file.dependency_count(); i++) {
    output("#include \"$0\"\n", CApiHeaderFilename(file.dependency(i)));
  }

  output(
      R"cc(
        static upb_Arena* upb_BootstrapArena() {
          static upb_Arena* arena = NULL;
          if (!arena) arena = upb_Arena_New();
          return arena;
        }
      )cc");

  output("\n");

  for (const auto msg : SortedMessages(file)) {
    WriteMessageMiniDescriptorInitializer(msg, options, output);
  }

  for (const auto msg : SortedEnums(file)) {
    WriteEnumMiniDescriptorInitializer(msg, options, output);
  }
}

void GenerateFile(const DefPoolPair& pools, upb::FileDefPtr file,
                  const Options& options, Plugin* plugin) {
  Output h_output;
  WriteHeader(pools, file, options, h_output);
  plugin->AddOutputFile(CApiHeaderFilename(file), h_output.output());

  if (options.bootstrap) {
    Output c_output;
    WriteMiniDescriptorSource(pools, file, options, c_output);
    plugin->AddOutputFile(SourceFilename(file), c_output.output());
  } else {
    // TODO: remove once we can figure out how to make both Blaze
    // and Bazel happy with header-only libraries.

    // begin:github_only
    plugin->AddOutputFile(SourceFilename(file), "\n");
    // end:github_only
  }
}

bool ParseOptions(Plugin* plugin, Options* options) {
  for (const auto& pair : ParseGeneratorParameter(plugin->parameter())) {
    if (pair.first == "bootstrap_upb") {
      options->bootstrap = true;
    } else {
      plugin->SetError(absl::Substitute("Unknown parameter: $0", pair.first));
      return false;
    }
  }

  return true;
}

absl::string_view ToStringView(upb_StringView str) {
  return absl::string_view(str.data, str.size);
}

}  // namespace

}  // namespace generator
}  // namespace upb

int main(int argc, char** argv) {
  upb::generator::DefPoolPair pools;
  upb::generator::Plugin plugin;
  upb::generator::Options options;
  if (!ParseOptions(&plugin, &options)) return 0;
  plugin.GenerateFilesRaw([&](const UPB_DESC(FileDescriptorProto) * file_proto,
                              bool generate) {
    upb::Status status;
    upb::FileDefPtr file = pools.AddFile(file_proto, &status);
    if (!file) {
      absl::string_view name = upb::generator::ToStringView(
          UPB_DESC(FileDescriptorProto_name)(file_proto));
      ABSL_LOG(FATAL) << "Couldn't add file " << name
                      << " to DefPool: " << status.error_message();
    }
    if (generate) GenerateFile(pools, file, options, &plugin);
  });
  return 0;
}
