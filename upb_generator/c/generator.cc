// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "absl/base/macros.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/memory/memory.h"
#include "absl/strings/cord.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/plugin.h"
#include "upb/base/descriptor_constants.h"
#include "upb/base/status.hpp"
#include "upb/base/string_view.h"
#include "upb/mem/arena.hpp"
#include "upb/mini_table/field.h"
#include "upb/reflection/def.hpp"
#include "upb_generator/c/names.h"
#include "upb_generator/c/names_internal.h"
#include "upb_generator/common.h"
#include "upb_generator/common/names.h"
#include "upb_generator/file_layout.h"
#include "upb_generator/minitable/names.h"
#include "upb_generator/minitable/names_internal.h"
#include "upb_generator/plugin.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace generator {
namespace {

struct Options {
  int bootstrap_stage = -1;  // -1 means not bootstrapped.
  bool strip_nonfunctional_codegen = false;
};

// Local convenience aliases for the public names.h header files.

std::string ExtensionIdentBase(upb::FieldDefPtr field) {
  return CApiExtensionIdentBase(field.full_name());
}

std::string MessageType(upb::MessageDefPtr descriptor) {
  return CApiMessageType(descriptor.full_name());
}

std::string EnumType(upb::EnumDefPtr descriptor) {
  return CApiEnumType(descriptor.full_name());
}

std::string EnumValueSymbol(upb::EnumValDefPtr value) {
  return CApiEnumValueSymbol(value.full_name());
}

std::string SourceFilename(upb::FileDefPtr file) {
  return StripExtension(file.name()) + ".upb.c";
}

std::string MessageMiniTableRef(upb::MessageDefPtr descriptor,
                                const Options& options) {
  if (options.bootstrap_stage == 0) {
    return absl::StrCat(MiniTableMessageVarName(descriptor.full_name()), "()");
  } else {
    return absl::StrCat("&", MiniTableMessageVarName(descriptor.full_name()));
  }
}

std::string EnumMiniTableRef(upb::EnumDefPtr descriptor,
                             const Options& options) {
  if (options.bootstrap_stage == 0) {
    return absl::StrCat(MiniTableEnumVarName(descriptor.full_name()), "()");
  } else {
    return absl::StrCat("&", MiniTableEnumVarName(descriptor.full_name()));
  }
}

std::string CTypeInternal(upb::FieldDefPtr field, bool is_const) {
  std::string maybe_const = is_const ? "const " : "";
  switch (field.ctype()) {
    case kUpb_CType_Message: {
      std::string maybe_struct =
          field.file() != field.message_type().file() ? "struct " : "";
      return maybe_const + maybe_struct + MessageType(field.message_type()) +
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

std::string MapValueCTypeConst(upb::FieldDefPtr map_field) {
  return CTypeConst(map_field.message_type().map_value());
}

std::string MapKeyValueSize(upb_CType ctype, absl::string_view expr) {
  return ctype == kUpb_CType_String || ctype == kUpb_CType_Bytes
             ? "0"
             : absl::StrCat("sizeof(", expr, ")");
}

std::string MapKeySize(upb::FieldDefPtr map_field, absl::string_view expr) {
  const upb_CType ctype = map_field.message_type().map_key().ctype();
  return MapKeyValueSize(ctype, expr);
}

std::string MapValueSize(upb::FieldDefPtr map_field, absl::string_view expr) {
  const upb_CType ctype = map_field.message_type().map_value().ctype();
  return MapKeyValueSize(ctype, expr);
}

std::string FieldInitializer(const DefPoolPair& pools, upb::FieldDefPtr field,
                             const Options& options);
std::string FieldInitializerStrong(const DefPoolPair& pools,
                                   upb::FieldDefPtr field,
                                   const Options& options);

void DumpEnumValues(upb::EnumDefPtr desc, Output& output) {
  std::vector<upb::EnumValDefPtr> values;
  values.reserve(desc.value_count());
  for (int i = 0; i < desc.value_count(); i++) {
    values.push_back(desc.value(i));
  }
  std::stable_sort(values.begin(), values.end(),
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
                               const Options& options, Output& output) {
  output(
      R"cc(
        UPB_INLINE bool $0_has_$1(const struct $2* msg) {
          return upb_Message_HasExtension((upb_Message*)msg, &$3);
        }
      )cc",
      ExtensionIdentBase(ext), ext.name(), MessageType(ext.containing_type()),
      MiniTableExtensionVarName(ext.full_name()));

  output(
      R"cc(
        UPB_INLINE void $0_clear_$1(struct $2* msg) {
          upb_Message_ClearExtension((upb_Message*)msg, &$3);
        }
      )cc",
      ExtensionIdentBase(ext), ext.name(), MessageType(ext.containing_type()),
      MiniTableExtensionVarName(ext.full_name()));

  if (ext.IsSequence()) {
    // TODO: We need generated accessors for repeated extensions.
  } else {
    output(
        R"cc(
          UPB_INLINE $0 $1_$2(const struct $3* msg) {
            const upb_MiniTableExtension* ext = &$4;
            UPB_ASSUME(upb_MiniTableField_IsScalar(&ext->UPB_PRIVATE(field)));
            UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(
                           &ext->UPB_PRIVATE(field)) == $5);
            $0 default_val = $6;
            $0 ret;
            _upb_Message_GetExtensionField((upb_Message*)msg, ext, &default_val, &ret);
            return ret;
          }
        )cc",
        CTypeConst(ext), ExtensionIdentBase(ext), ext.name(),
        MessageType(ext.containing_type()),
        MiniTableExtensionVarName(ext.full_name()), GetFieldRep(pools, ext),
        FieldDefault(ext));
    output(
        R"cc(
          UPB_INLINE void $1_set_$2(struct $3* msg, $0 val, upb_Arena* arena) {
            const upb_MiniTableExtension* ext = &$4;
            UPB_ASSUME(upb_MiniTableField_IsScalar(&ext->UPB_PRIVATE(field)));
            UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(
                           &ext->UPB_PRIVATE(field)) == $5);
            bool ok = upb_Message_SetExtension((upb_Message*)msg, ext, &val, arena);
            UPB_ASSERT(ok);
          }
        )cc",
        CTypeConst(ext), ExtensionIdentBase(ext), ext.name(),
        MessageType(ext.containing_type()),
        MiniTableExtensionVarName(ext.full_name()), GetFieldRep(pools, ext));

    // Message extensions also have a Msg_mutable_foo() accessor that will
    // create the sub-message if it doesn't already exist.
    if (ext.IsSubMessage()) {
      output(
          R"cc(
            UPB_INLINE struct $0* $1_mutable_$2(struct $3* msg,
                                                upb_Arena* arena) {
              struct $0* sub = (struct $0*)$1_$2(msg);
              if (sub == NULL) {
                sub = (struct $0*)_upb_Message_New($4, arena);
                if (sub) $1_set_$2(msg, sub, arena);
              }
              return sub;
            }
          )cc",
          MessageType(ext.message_type()), ExtensionIdentBase(ext), ext.name(),
          MessageType(ext.containing_type()),
          MessageMiniTableRef(ext.message_type(), options));
    }
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
          if (upb_Decode(buf, size, UPB_UPCAST(ret), $1, NULL, 0, arena) !=
              kUpb_DecodeStatus_Ok) {
            return NULL;
          }
          return ret;
        }
        UPB_INLINE $0* $0_parse_ex(const char* buf, size_t size,
                                   const upb_ExtensionRegistry* extreg,
                                   int options, upb_Arena* arena) {
          $0* ret = $0_new(arena);
          if (!ret) return NULL;
          if (upb_Decode(buf, size, UPB_UPCAST(ret), $1, extreg, options,
                         arena) != kUpb_DecodeStatus_Ok) {
            return NULL;
          }
          return ret;
        }
        UPB_INLINE char* $0_serialize(const $0* msg, upb_Arena* arena, size_t* len) {
          char* ptr;
          (void)upb_Encode(UPB_UPCAST(msg), $1, 0, arena, &ptr, len);
          return ptr;
        }
        UPB_INLINE char* $0_serialize_ex(const $0* msg, int options,
                                         upb_Arena* arena, size_t* len) {
          char* ptr;
          (void)upb_Encode(UPB_UPCAST(msg), $1, options, arena, &ptr, len);
          return ptr;
        }
      )cc",
      MessageType(message), MessageMiniTableRef(message, options));
}

void GenerateOneofInHeader(upb::OneofDefPtr oneof, const DefPoolPair& pools,
                           absl::string_view msg_name, const Options& options,
                           Output& output) {
  std::string fullname = CApiOneofIdentBase(oneof.full_name());
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
          return ($0_oneofcases)upb_Message_WhichOneofFieldNumber(
              UPB_UPCAST(msg), &field);
        }
      )cc",
      fullname, msg_name, oneof.name(),
      FieldInitializer(pools, oneof.field(0), options));
}

void GenerateHazzer(upb::FieldDefPtr field, const DefPoolPair& pools,
                    absl::string_view msg_name, const NameMangler& mangler,
                    const Options& options, Output& output) {
  std::string resolved_name = mangler.ResolveFieldName(field.name());
  if (field.has_presence()) {
    output(
        R"cc(
          UPB_INLINE bool $0_has_$1(const $0* msg) {
            const upb_MiniTableField field = $2;
            return upb_Message_HasBaseField(UPB_UPCAST(msg), &field);
          }
        )cc",
        msg_name, resolved_name, FieldInitializer(pools, field, options));
  }
}

void GenerateClear(upb::FieldDefPtr field, const DefPoolPair& pools,
                   absl::string_view msg_name, const NameMangler& mangler,
                   const Options& options, Output& output) {
  if (field == field.containing_type().map_key() ||
      field == field.containing_type().map_value()) {
    // Cannot be cleared.
    return;
  }
  std::string resolved_name = mangler.ResolveFieldName(field.name());
  output(
      R"cc(
        UPB_INLINE void $0_clear_$1($0* msg) {
          const upb_MiniTableField field = $2;
          upb_Message_ClearBaseField(UPB_UPCAST(msg), &field);
        }
      )cc",
      msg_name, resolved_name, FieldInitializer(pools, field, options));
}

void GenerateMapGetters(upb::FieldDefPtr field, const DefPoolPair& pools,
                        absl::string_view msg_name, const NameMangler& mangler,
                        const Options& options, Output& output) {
  std::string resolved_name = mangler.ResolveFieldName(field.name());
  output(
      R"cc(
        UPB_INLINE size_t $0_$1_size(const $0* msg) {
          const upb_MiniTableField field = $2;
          const upb_Map* map = upb_Message_GetMap(UPB_UPCAST(msg), &field);
          return map ? _upb_Map_Size(map) : 0;
        }
      )cc",
      msg_name, resolved_name, FieldInitializer(pools, field, options));
  output(
      R"cc(
        UPB_INLINE bool $0_$1_get(const $0* msg, $2 key, $3* val) {
          const upb_MiniTableField field = $4;
          const upb_Map* map = upb_Message_GetMap(UPB_UPCAST(msg), &field);
          if (!map) return false;
          return _upb_Map_Get(map, &key, $5, val, $6);
        }
      )cc",
      msg_name, resolved_name, MapKeyCType(field), MapValueCType(field),
      FieldInitializerStrong(pools, field, options), MapKeySize(field, "key"),
      MapValueSize(field, "*val"));
  output(
      R"cc(
        UPB_INLINE bool $0_$1_next(const $0* msg, $2* key, $3* val,
                                   size_t* iter) {
          const upb_MiniTableField field = $4;
          const upb_Map* map = upb_Message_GetMap(UPB_UPCAST(msg), &field);
          if (!map) return false;
          upb_MessageValue k;
          upb_MessageValue v;
          if (!upb_Map_Next(map, &k, &v, iter)) return false;
          memcpy(key, &k, sizeof(*key));
          memcpy(val, &v, sizeof(*val));
          return true;
        }
      )cc",
      msg_name, resolved_name, MapKeyCType(field), MapValueCTypeConst(field),
      FieldInitializerStrong(pools, field, options));
  // Generate private getter returning a upb_Map or NULL for immutable and
  // a upb_Map for mutable.
  //
  // Example:
  //   UPB_INLINE const upb_Map* _name_immutable_upb_map(Foo* msg)
  //   UPB_INLINE upb_Map* _name_mutable_upb_map(Foo* msg, upb_Arena* a)
  output(
      R"cc(
        UPB_INLINE const upb_Map* _$0_$1_$2($0* msg) {
          const upb_MiniTableField field = $4;
          return upb_Message_GetMap(UPB_UPCAST(msg), &field);
        }
        UPB_INLINE upb_Map* _$0_$1_$3($0* msg, upb_Arena* a) {
          const upb_MiniTableField field = $4;
          return _upb_Message_GetOrCreateMutableMap(UPB_UPCAST(msg), &field, $5, $6, a);
        }
      )cc",
      msg_name, resolved_name, kMapGetterPostfix, kMutableMapGetterPostfix,
      FieldInitializerStrong(pools, field, options),
      MapKeySize(field, MapKeyCType(field)),
      MapValueSize(field, MapValueCType(field)));
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
                             const NameMangler& mangler, const Options& options,
                             Output& output) {
  // Generate getter returning first item and size.
  //
  // Example:
  //   UPB_INLINE const struct Bar* const* name(const Foo* msg, size_t* size)
  output(
      R"cc(
        UPB_INLINE $0 const* $1_$2(const $1* msg, size_t* size) {
          const upb_MiniTableField field = $3;
          const upb_Array* arr = upb_Message_GetArray(UPB_UPCAST(msg), &field);
          if (arr) {
            if (size) *size = arr->UPB_PRIVATE(size);
            return ($0 const*)upb_Array_DataPtr(arr);
          } else {
            if (size) *size = 0;
            return NULL;
          }
        }
      )cc",
      CTypeConst(field),                             // $0
      msg_name,                                      // $1
      mangler.ResolveFieldName(field.name()),        // $2
      FieldInitializerStrong(pools, field, options)  // #3
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
          const upb_Array* arr = upb_Message_GetArray(UPB_UPCAST(msg), &field);
          if (size) {
            *size = arr ? arr->UPB_PRIVATE(size) : 0;
          }
          return arr;
        }
        UPB_INLINE upb_Array* _$1_$2_$5($1* msg, size_t* size, upb_Arena* arena) {
          const upb_MiniTableField field = $3;
          upb_Array* arr = upb_Message_GetOrCreateMutableArray(UPB_UPCAST(msg),
                                                               &field, arena);
          if (size) {
            *size = arr ? arr->UPB_PRIVATE(size) : 0;
          }
          return arr;
        }
      )cc",
      CTypeConst(field),                              // $0
      msg_name,                                       // $1
      mangler.ResolveFieldName(field.name()),         // $2
      FieldInitializerStrong(pools, field, options),  // $3
      kRepeatedFieldArrayGetterPostfix,               // $4
      kRepeatedFieldMutableArrayGetterPostfix         // $5
  );
}

void GenerateScalarGetters(upb::FieldDefPtr field, const DefPoolPair& pools,
                           absl::string_view msg_name,
                           const NameMangler& mangler, const Options& Options,
                           Output& output) {
  std::string field_name = mangler.ResolveFieldName(field.name());
  output(
      R"cc(
        UPB_INLINE $0 $1_$2(const $1* msg) {
          $0 default_val = $3;
          $0 ret;
          const upb_MiniTableField field = $4;
          _upb_Message_GetNonExtensionField(UPB_UPCAST(msg), &field,
                                            &default_val, &ret);
          return ret;
        }
      )cc",
      CTypeConst(field), msg_name, field_name, FieldDefault(field),
      FieldInitializerStrong(pools, field, Options));
}

void GenerateGetters(upb::FieldDefPtr field, const DefPoolPair& pools,
                     absl::string_view msg_name, const NameMangler& mangler,
                     const Options& options, Output& output) {
  if (field.IsMap()) {
    GenerateMapGetters(field, pools, msg_name, mangler, options, output);
  } else if (field.containing_type().mapentry()) {
    GenerateMapEntryGetters(field, msg_name, output);
  } else if (field.IsSequence()) {
    GenerateRepeatedGetters(field, pools, msg_name, mangler, options, output);
  } else {
    GenerateScalarGetters(field, pools, msg_name, mangler, options, output);
  }
}

void GenerateMapSetters(upb::FieldDefPtr field, const DefPoolPair& pools,
                        absl::string_view msg_name, const NameMangler& mangler,
                        const Options& options, Output& output) {
  std::string resolved_name = mangler.ResolveFieldName(field.name());
  output(
      R"cc(
        UPB_INLINE void $0_$1_clear($0* msg) {
          const upb_MiniTableField field = $2;
          upb_Map* map = (upb_Map*)upb_Message_GetMap(UPB_UPCAST(msg), &field);
          if (!map) return;
          _upb_Map_Clear(map);
        }
      )cc",
      msg_name, resolved_name, FieldInitializer(pools, field, options));
  output(
      R"cc(
        UPB_INLINE bool $0_$1_set($0* msg, $2 key, $3 val, upb_Arena* a) {
          const upb_MiniTableField field = $4;
          upb_Map* map = _upb_Message_GetOrCreateMutableMap(UPB_UPCAST(msg),
                                                            &field, $5, $6, a);
          return _upb_Map_Insert(map, &key, $5, &val, $6, a) !=
                 kUpb_MapInsertStatus_OutOfMemory;
        }
      )cc",
      msg_name, resolved_name, MapKeyCType(field), MapValueCType(field),
      FieldInitializerStrong(pools, field, options), MapKeySize(field, "key"),
      MapValueSize(field, "val"));
  output(
      R"cc(
        UPB_INLINE bool $0_$1_delete($0* msg, $2 key) {
          const upb_MiniTableField field = $3;
          upb_Map* map = (upb_Map*)upb_Message_GetMap(UPB_UPCAST(msg), &field);
          if (!map) return false;
          return _upb_Map_Delete(map, &key, $4, NULL);
        }
      )cc",
      msg_name, resolved_name, MapKeyCType(field),
      FieldInitializer(pools, field, options), MapKeySize(field, "key"));
}

void GenerateRepeatedSetters(upb::FieldDefPtr field, const DefPoolPair& pools,
                             absl::string_view msg_name,
                             const NameMangler& mangler, const Options& options,
                             Output& output) {
  std::string resolved_name = mangler.ResolveFieldName(field.name());
  output(
      R"cc(
        UPB_INLINE $0* $1_mutable_$2($1* msg, size_t* size) {
          upb_MiniTableField field = $3;
          upb_Array* arr = upb_Message_GetMutableArray(UPB_UPCAST(msg), &field);
          if (arr) {
            if (size) *size = arr->UPB_PRIVATE(size);
            return ($0*)upb_Array_MutableDataPtr(arr);
          } else {
            if (size) *size = 0;
            return NULL;
          }
        }
      )cc",
      CType(field), msg_name, resolved_name,
      FieldInitializerStrong(pools, field, options));
  output(
      R"cc(
        UPB_INLINE $0* $1_resize_$2($1* msg, size_t size, upb_Arena* arena) {
          upb_MiniTableField field = $3;
          return ($0*)upb_Message_ResizeArrayUninitialized(UPB_UPCAST(msg),
                                                           &field, size, arena);
        }
      )cc",
      CType(field), msg_name, resolved_name,
      FieldInitializer(pools, field, options));
  if (field.ctype() == kUpb_CType_Message) {
    output(
        R"cc(
          UPB_INLINE struct $0* $1_add_$2($1* msg, upb_Arena* arena) {
            upb_MiniTableField field = $4;
            upb_Array* arr = upb_Message_GetOrCreateMutableArray(
                UPB_UPCAST(msg), &field, arena);
            if (!arr || !UPB_PRIVATE(_upb_Array_ResizeUninitialized)(
                            arr, arr->UPB_PRIVATE(size) + 1, arena)) {
              return NULL;
            }
            struct $0* sub = (struct $0*)_upb_Message_New($3, arena);
            if (!arr || !sub) return NULL;
            UPB_PRIVATE(_upb_Array_Set)
            (arr, arr->UPB_PRIVATE(size) - 1, &sub, sizeof(sub));
            return sub;
          }
        )cc",
        MessageType(field.message_type()), msg_name, resolved_name,
        MessageMiniTableRef(field.message_type(), options),
        FieldInitializerStrong(pools, field, options));
  } else {
    output(
        R"cc(
          UPB_INLINE bool $1_add_$2($1* msg, $0 val, upb_Arena* arena) {
            upb_MiniTableField field = $3;
            upb_Array* arr = upb_Message_GetOrCreateMutableArray(
                UPB_UPCAST(msg), &field, arena);
            if (!arr || !UPB_PRIVATE(_upb_Array_ResizeUninitialized)(
                            arr, arr->UPB_PRIVATE(size) + 1, arena)) {
              return false;
            }
            UPB_PRIVATE(_upb_Array_Set)
            (arr, arr->UPB_PRIVATE(size) - 1, &val, sizeof(val));
            return true;
          }
        )cc",
        CType(field), msg_name, resolved_name,
        FieldInitializerStrong(pools, field, options));
  }
}

void GenerateNonRepeatedSetters(upb::FieldDefPtr field,
                                const DefPoolPair& pools,
                                absl::string_view msg_name,
                                const NameMangler& mangler,
                                const Options& options, Output& output) {
  if (field == field.containing_type().map_key()) {
    // Key cannot be mutated.
    return;
  }

  std::string field_name = mangler.ResolveFieldName(field.name());

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
               upb_Message_SetBaseField((upb_Message *)msg, &field, &value);
             }
           )cc",
           msg_name, field_name, CType(field),
           FieldInitializerStrong(pools, field, options));
  }

  // Message fields also have a Msg_mutable_foo() accessor that will create
  // the sub-message if it doesn't already exist.
  if (field.IsSubMessage() && !field.containing_type().mapentry()) {
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
        MessageType(field.message_type()), msg_name, field_name,
        MessageMiniTableRef(field.message_type(), options));
  }
}

void GenerateSetters(upb::FieldDefPtr field, const DefPoolPair& pools,
                     absl::string_view msg_name, const NameMangler& mangler,
                     const Options& options, Output& output) {
  if (field.IsMap()) {
    GenerateMapSetters(field, pools, msg_name, mangler, options, output);
  } else if (field.IsSequence()) {
    GenerateRepeatedSetters(field, pools, msg_name, mangler, options, output);
  } else {
    GenerateNonRepeatedSetters(field, pools, msg_name, mangler, options,
                               output);
  }
}

void GenerateMessageInHeader(upb::MessageDefPtr message,
                             const DefPoolPair& pools, const Options& options,
                             Output& output) {
  output("/* $0 */\n\n", message.full_name());
  std::string msg_name = MessageType(message);
  if (!message.mapentry()) {
    GenerateMessageFunctionsInHeader(message, options, output);
  }

  for (int i = 0; i < message.real_oneof_count(); i++) {
    GenerateOneofInHeader(message.oneof(i), pools, msg_name, options, output);
  }

  NameMangler mangler(GetUpbFields(message));
  for (auto field : FieldNumberOrder(message)) {
    GenerateClear(field, pools, msg_name, mangler, options, output);
    GenerateGetters(field, pools, msg_name, mangler, options, output);
    GenerateHazzer(field, pools, msg_name, mangler, options, output);
  }

  output("\n");

  for (auto field : FieldNumberOrder(message)) {
    GenerateSetters(field, pools, msg_name, mangler, options, output);
  }

  output("\n");
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
  std::vector<upb::EnumDefPtr> this_file_enums = SortedEnums(file, kAllEnums);
  std::vector<upb::MessageDefPtr> forward_messages =
      SortedForwardMessages(this_file_messages, this_file_exts);

  output(FileWarning(file.name()));
  output(
      "#ifndef $0_UPB_H_\n"
      "#define $0_UPB_H_\n\n"
      "#include \"upb/generated_code_support.h\"\n\n",
      IncludeGuard(file.name()));

  for (int i = 0; i < file.public_dependency_count(); i++) {
    if (i == 0) {
      output("/* Public Imports. */\n");
    }
    output("#include \"$0\"\n",
           CApiHeaderFilename(file.public_dependency(i).name(),
                              options.bootstrap_stage >= 0));
  }
  if (file.public_dependency_count() > 0) {
    output("\n");
  }

  if (options.bootstrap_stage != 0) {
    output("#include \"$0\"\n\n",
           MiniTableHeaderFilename(file.name(), options.bootstrap_stage >= 0));
    for (int i = 0; i < file.dependency_count(); i++) {
      if (options.strip_nonfunctional_codegen &&
          google::protobuf::compiler::IsKnownFeatureProto(file.dependency(i).name())) {
        // Strip feature imports for editions codegen tests.
        continue;
      }
      output("#include \"$0\"\n",
             MiniTableHeaderFilename(file.dependency(i).name(),
                                     options.bootstrap_stage >= 0));
    }
    output("\n");
  }

  output(
      "// Must be last.\n"
      "#include \"upb/port/def.inc\"\n"
      "\n"
      "#ifdef __cplusplus\n"
      "extern \"C\" {\n"
      "#endif\n"
      "\n");

  if (options.bootstrap_stage == 0) {
    for (auto message : this_file_messages) {
      output("extern const upb_MiniTable* $0(void);\n",
             MiniTableMessageVarName(message.full_name()));
    }
    for (auto message : forward_messages) {
      output("extern const upb_MiniTable* $0(void);\n",
             MiniTableMessageVarName(message.full_name()));
    }
    for (auto enumdesc : this_file_enums) {
      output("extern const upb_MiniTableEnum* $0(void);\n",
             MiniTableEnumVarName(enumdesc.full_name()));
    }
    output("\n");
  }

  // Forward-declare types defined in this file.
  for (auto message : this_file_messages) {
    output("typedef struct $0 { upb_Message UPB_PRIVATE(base); } $0;\n",
           MessageType(message));
  }

  // Forward-declare types not in this file, but used as submessages.
  // Order by full name for consistent ordering.
  for (auto msg : forward_messages) {
    output("struct $0;\n", MessageType(msg));
  }

  if (!this_file_messages.empty()) {
    output("\n");
  }

  for (auto enumdesc : this_file_enums) {
    output("typedef enum {\n");
    DumpEnumValues(enumdesc, output);
    output("} $0;\n\n", EnumType(enumdesc));
  }

  output("\n");

  output("\n");
  for (auto message : this_file_messages) {
    GenerateMessageInHeader(message, pools, options, output);
  }

  for (auto ext : this_file_exts) {
    GenerateExtensionInHeader(pools, ext, options, output);
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
        size_t size32 = pools.GetMiniTable32(message)->UPB_PRIVATE(size);
        size_t size64 = pools.GetMiniTable64(message)->UPB_PRIVATE(size);
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
      IncludeGuard(file.name()));
}

std::string FieldInitializer(upb::FieldDefPtr field,
                             const upb_MiniTableField* field64,
                             const upb_MiniTableField* field32,
                             const Options& options) {
  if (options.bootstrap_stage == 0) {
    ABSL_CHECK(!field.is_extension());
    return absl::Substitute(
        "*upb_MiniTable_FindFieldByNumber($0, $1)",
        MessageMiniTableRef(field.containing_type(), options), field.number());
  } else {
    return upb::generator::FieldInitializer(field, field64, field32);
  }
}

std::string StrongReferenceSingle(upb::FieldDefPtr field) {
  if (!field.message_type()) return "";
  return absl::Substitute(
      "  UPB_PRIVATE(_upb_MiniTable_StrongReference)(&$0)",
      MiniTableMessageVarName(field.message_type().full_name()));
}

std::string StrongReference(upb::FieldDefPtr field) {
  if (field.IsMap() &&
      field.message_type().FindFieldByNumber(2).IsSubMessage()) {
    return StrongReferenceSingle(field) + ";\n" +
           StrongReferenceSingle(field.message_type().FindFieldByNumber(2));
  } else {
    return StrongReferenceSingle(field);
  }
}
std::string FieldInitializerStrong(const DefPoolPair& pools,
                                   upb::FieldDefPtr field,
                                   const Options& options) {
  std::string ret = FieldInitializer(pools, field, options);
  if (options.bootstrap_stage != 0 && field.IsSubMessage()) {
    ret += ";\n" + StrongReference(field);
  }
  return ret;
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
      MiniTableMessageVarName(msg.full_name()), msg.MiniDescriptorEncode(),
      resolve_calls.output());
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
      MiniTableEnumVarName(enum_def.full_name()),
      enum_def.MiniDescriptorEncode());
  output("\n");
}

void WriteMiniDescriptorSource(const DefPoolPair& pools, upb::FileDefPtr file,
                               const Options& options, Output& output) {
  output(
      "#include <stddef.h>\n"
      "#include \"upb/generated_code_support.h\"\n"
      "#include \"$0\"\n\n",
      CApiHeaderFilename(file.name(), options.bootstrap_stage >= 0));

  for (int i = 0; i < file.dependency_count(); i++) {
    if (options.strip_nonfunctional_codegen &&
        google::protobuf::compiler::IsKnownFeatureProto(file.dependency(i).name())) {
      continue;
    }
    output("#include \"$0\"\n",
           CApiHeaderFilename(file.dependency(i).name(),
                              options.bootstrap_stage >= 0));
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

  for (const auto msg : SortedEnums(file, kClosedEnums)) {
    WriteEnumMiniDescriptorInitializer(msg, options, output);
  }
}

void GenerateFile(const DefPoolPair& pools, upb::FileDefPtr file,
                  const Options& options,
                  google::protobuf::compiler::GeneratorContext* context) {
  Output h_output;
  WriteHeader(pools, file, options, h_output);
  {
    auto stream =
        absl::WrapUnique(context->Open(CApiHeaderFilename(file.name(), false)));
    ABSL_CHECK(stream->WriteCord(absl::Cord(h_output.output())));
  }

  if (options.bootstrap_stage == 0) {
    Output c_output;
    WriteMiniDescriptorSource(pools, file, options, c_output);
    auto stream = absl::WrapUnique(context->Open(SourceFilename(file)));
    ABSL_CHECK(stream->WriteCord(absl::Cord(c_output.output())));
  } else {
    // TODO: remove once we can figure out how to make both Blaze
    // and Bazel happy with header-only libraries.

    auto stream = absl::WrapUnique(context->Open(SourceFilename(file)));
    ABSL_CHECK(stream->WriteCord(absl::Cord("\n")));
  }
}

bool ParseOptions(absl::string_view parameter, Options* options,
                  std::string* error) {
  for (const auto& pair : ParseGeneratorParameter(parameter)) {
    if (pair.first == "bootstrap_stage") {
      if (!absl::SimpleAtoi(pair.second, &options->bootstrap_stage)) {
        *error = absl::Substitute("Bad stage: $0", pair.second);
        return false;
      }
    } else if (pair.first == "experimental_strip_nonfunctional_codegen") {
      options->strip_nonfunctional_codegen = true;
    } else {
      *error = absl::Substitute("Unknown parameter: $0", pair.first);
      return false;
    }
  }

  return true;
}

class CGenerator : public google::protobuf::compiler::CodeGenerator {
  bool Generate(const google::protobuf::FileDescriptor* file,
                const std::string& parameter,
                google::protobuf::compiler::GeneratorContext* generator_context,
                std::string* error) const override {
    std::vector<const google::protobuf::FileDescriptor*> files{file};
    return GenerateAll(files, parameter, generator_context, error);
  }

  bool GenerateAll(const std::vector<const google::protobuf::FileDescriptor*>& files,
                   const std::string& parameter,
                   google::protobuf::compiler::GeneratorContext* generator_context,
                   std::string* error) const override {
    Options options;
    if (!ParseOptions(parameter, &options, error)) {
      return false;
    }

    upb::Arena arena;
    DefPoolPair pools;
    absl::flat_hash_set<std::string> files_seen;
    for (const auto* file : files) {
      PopulateDefPool(file, &arena, &pools, &files_seen);
      upb::FileDefPtr upb_file = pools.GetFile(file->name());
      GenerateFile(pools, upb_file, options, generator_context);
    }

    return true;
  }

  uint64_t GetSupportedFeatures() const override {
    return FEATURE_PROTO3_OPTIONAL | FEATURE_SUPPORTS_EDITIONS;
  }
  google::protobuf::Edition GetMinimumEdition() const override {
    return google::protobuf::Edition::EDITION_PROTO2;
  }
  google::protobuf::Edition GetMaximumEdition() const override {
    return google::protobuf::Edition::EDITION_2023;
  }
};

}  // namespace

}  // namespace generator
}  // namespace upb

int main(int argc, char** argv) {
  upb::generator::CGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
