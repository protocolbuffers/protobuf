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
#include <iterator>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "absl/base/macros.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/plugin.h"
#include "google/protobuf/io/printer.h"
#include "upb/base/descriptor_constants.h"
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

using Printer = google::protobuf::io::Printer;
using Sub = google::protobuf::io::Printer::Sub;

struct Options {
  int bootstrap_stage = -1;  // -1 means not bootstrapped.
  bool strip_nonfunctional_codegen = false;
};

class Context {
 public:
  Context(const DefPoolPair& pools, const Options& options,
          google::protobuf::io::Printer& printer)
      : pools_(pools), options_(options), printer_(printer) {}

  const DefPoolPair& pools() const { return pools_; }
  const Options& options() const { return options_; }

  // A few convenience wrappers so that users can write `ctx.Emit()` instead of
  // `ctx.printer().Emit()`.
  void Emit(absl::Span<const google::protobuf::io::Printer::Sub> vars,
            absl::string_view format,
            Printer::SourceLocation loc = Printer::SourceLocation::current()) {
    printer_.Emit(vars, format, loc);
  }

  void Emit(absl::string_view format,
            Printer::SourceLocation loc = Printer::SourceLocation::current()) {
    printer_.Emit(format, loc);
  }

 private:
  const DefPoolPair& pools_;
  const Options& options_;
  Printer& printer_;
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

void DumpEnumValues(Context& c, upb::EnumDefPtr desc) {
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
    c.Emit(
        {
            {"name", EnumValueSymbol(value)},
            {"number", absl::StrCat(value.number())},
            {"comma", i == values.size() - 1 ? "" : ","},
        },
        R"cc(
          $name$ = $number$$comma$
        )cc");
  }
}

std::string GetFieldRep(const DefPoolPair& pools, upb::FieldDefPtr field) {
  return upb::generator::GetFieldRep(pools.GetField32(field),
                                     pools.GetField64(field));
}

void GenerateExtensionInHeader(Context& c, upb::FieldDefPtr ext) {
  c.Emit(
      {
          {"ident_base", ExtensionIdentBase(ext)},
          {"name", ext.name()},
          {"ctype", MessageType(ext.containing_type())},
          {"ext_var", MiniTableExtensionVarName(ext.full_name())},
      },
      R"cc(
        UPB_INLINE bool $ident_base$_has_$name$(const struct $ctype$* msg) {
          return upb_Message_HasExtension((upb_Message*)msg, &$ext_var$);
        }

        UPB_INLINE void $ident_base$_clear_$name$(struct $ctype$* msg) {
          upb_Message_ClearExtension((upb_Message*)msg, &$ext_var$);
        }
      )cc");

  if (ext.IsSequence()) {
    // TODO: We need generated accessors for repeated extensions.
  } else {
    c.Emit(
        {
            {"ctype_const", CTypeConst(ext)},
            {"ident_base", ExtensionIdentBase(ext)},
            {"name", ext.name()},
            {"ctype", MessageType(ext.containing_type())},
            {"ext_var", MiniTableExtensionVarName(ext.full_name())},
            {"rep", GetFieldRep(c.pools(), ext)},
            {"default", FieldDefault(ext)},
        },
        R"cc(
          UPB_INLINE $ctype_const$
          $ident_base$_$name$(const struct $ctype$* msg) {
            const upb_MiniTableExtension* ext = &$ext_var$;
            UPB_ASSUME(upb_MiniTableField_IsScalar(&ext->UPB_PRIVATE(field)));
            UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(
                           &ext->UPB_PRIVATE(field)) == $rep$);
            $ctype_const$ default_val = $default$;
            $ctype_const$ ret;
            _upb_Message_GetExtensionField((upb_Message*)msg, ext, &default_val, &ret);
            return ret;
          }

          UPB_INLINE void $ident_base$_set_$name$(struct $ctype$* msg,
                                                  $ctype_const$ val,
                                                  upb_Arena* arena) {
            const upb_MiniTableExtension* ext = &$ext_var$;
            UPB_ASSUME(upb_MiniTableField_IsScalar(&ext->UPB_PRIVATE(field)));
            UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(
                           &ext->UPB_PRIVATE(field)) == $rep$);
            bool ok = upb_Message_SetExtension((upb_Message*)msg, ext, &val, arena);
            UPB_ASSERT(ok);
          }
        )cc");

    // Message extensions also have a Msg_mutable_foo() accessor that will
    // create the sub-message if it doesn't already exist.
    if (ext.IsSubMessage()) {
      c.Emit(
          {
              {"sub_ctype", MessageType(ext.message_type())},
              {"ident_base", ExtensionIdentBase(ext)},
              {"name", ext.name()},
              {"ctype", MessageType(ext.containing_type())},
              {"mini_table",
               MessageMiniTableRef(ext.message_type(), c.options())},
          },
          R"cc(
            UPB_INLINE struct $sub_ctype$* $ident_base$_mutable_$name$(
                struct $ctype$* msg, upb_Arena* arena) {
              struct $sub_ctype$* sub = (struct $sub_ctype$*)$ident_base$_$name$(msg);
              if (sub == NULL) {
                sub = (struct $sub_ctype$*)_upb_Message_New($mini_table$, arena);
                if (sub) $ident_base$_set_$name$(msg, sub, arena);
              }
              return sub;
            }
          )cc");
    }
  }
}

void GenerateMessageFunctionsInHeader(Context& c, upb::MessageDefPtr message) {
  // TODO: The generated code here does not check the return values
  // from upb_Encode(). How can we even fix this without breaking other things?
  c.Emit(
      {
          {"msg_type", MessageType(message)},
          {"mini_table", MessageMiniTableRef(message, c.options())},
      },
      R"cc(
        UPB_INLINE $msg_type$* $msg_type$_new(upb_Arena* arena) {
          return ($msg_type$*)_upb_Message_New($mini_table$, arena);
        }
        UPB_INLINE $msg_type$* $msg_type$_parse(const char* buf, size_t size,
                                                upb_Arena* arena) {
          $msg_type$* ret = $msg_type$_new(arena);
          if (!ret) return NULL;
          if (upb_Decode(buf, size, UPB_UPCAST(ret), $mini_table$, NULL, 0,
                         arena) != kUpb_DecodeStatus_Ok) {
            return NULL;
          }
          return ret;
        }
        UPB_INLINE $msg_type$* $msg_type$_parse_ex(
            const char* buf, size_t size, const upb_ExtensionRegistry* extreg,
            int options, upb_Arena* arena) {
          $msg_type$* ret = $msg_type$_new(arena);
          if (!ret) return NULL;
          if (upb_Decode(buf, size, UPB_UPCAST(ret), $mini_table$, extreg,
                         options, arena) != kUpb_DecodeStatus_Ok) {
            return NULL;
          }
          return ret;
        }
        UPB_INLINE char* $msg_type$_serialize(const $msg_type$* msg,
                                              upb_Arena* arena, size_t* len) {
          char* ptr;
          (void)upb_Encode(UPB_UPCAST(msg), $mini_table$, 0, arena, &ptr, len);
          return ptr;
        }
        UPB_INLINE char* $msg_type$_serialize_ex(const $msg_type$* msg,
                                                 int options, upb_Arena* arena,
                                                 size_t* len) {
          char* ptr;
          (void)upb_Encode(UPB_UPCAST(msg), $mini_table$, options, arena, &ptr, len);
          return ptr;
        }
      )cc");
}

void GenerateOneofCases(Context& c, upb::OneofDefPtr oneof) {
  std::string base = CApiOneofIdentBase(oneof.full_name());
  for (int i = 0; i < oneof.field_count(); i++) {
    upb::FieldDefPtr field = oneof.field(i);
    c.Emit(
        {
            {"base", base},
            {"name", field.name()},
            {"number", absl::StrCat(field.number())},
        },
        R"cc(
          $base$_$name$ = $number$,
        )cc");
  }
}

void GenerateOneofInHeader(Context& c, upb::OneofDefPtr oneof,
                           absl::string_view msg_name) {
  c.Emit(
      {
          {"fullname", CApiOneofIdentBase(oneof.full_name())},
          {"msg_name", msg_name},
          {"oneof_name", oneof.name()},
          {"field_init",
           FieldInitializer(c.pools(), oneof.field(0), c.options())},
          {"mini_table",
           MessageMiniTableRef(oneof.containing_type(), c.options())},
          Sub("cases", [&]() { GenerateOneofCases(c, oneof); }).WithSuffix(","),
      },
      R"cc(
        typedef enum {
          $cases$,
          $fullname$_NOT_SET = 0
        } $fullname$_oneofcases;

        UPB_INLINE $fullname$_oneofcases
        $msg_name$_$oneof_name$_case(const $msg_name$* msg) {
          const upb_MiniTableField field = $field_init$;
          return ($fullname$_oneofcases)upb_Message_WhichOneofFieldNumber(
              UPB_UPCAST(msg), &field);
        }
        UPB_INLINE void $msg_name$_clear_$oneof_name$($msg_name$* msg) {
          const upb_MiniTableField field = $field_init$;
          upb_Message_ClearOneof(UPB_UPCAST(msg), $mini_table$, &field);
        }
      )cc");
}

void GenerateHazzer(Context& c, upb::FieldDefPtr field,
                    absl::string_view msg_name, const NameMangler& mangler) {
  std::string resolved_name = mangler.ResolveFieldName(field.name());
  if (field.has_presence()) {
    c.Emit(
        {
            {"msg_name", msg_name},
            {"name", resolved_name},
            {"field_init", FieldInitializer(c.pools(), field, c.options())},
        },
        R"cc(
          UPB_INLINE bool $msg_name$_has_$name$(const $msg_name$* msg) {
            const upb_MiniTableField field = $field_init$;
            return upb_Message_HasBaseField(UPB_UPCAST(msg), &field);
          }
        )cc");
  }
}

void GenerateClear(Context& c, upb::FieldDefPtr field,
                   absl::string_view msg_name, const NameMangler& mangler) {
  if (field == field.containing_type().map_key() ||
      field == field.containing_type().map_value()) {
    // Cannot be cleared.
    return;
  }
  c.Emit(
      {
          {"msg_name", msg_name},
          {"name", mangler.ResolveFieldName(field.name())},
          {"field_init", FieldInitializer(c.pools(), field, c.options())},
      },
      R"cc(
        UPB_INLINE void $msg_name$_clear_$name$($msg_name$* msg) {
          const upb_MiniTableField field = $field_init$;
          upb_Message_ClearBaseField(UPB_UPCAST(msg), &field);
        }
      )cc");
}

void GenerateMapGetters(Context& c, upb::FieldDefPtr field,
                        absl::string_view msg_name,
                        const NameMangler& mangler) {
  c.Emit(
      {
          {"msg_name", msg_name},
          {"name", mangler.ResolveFieldName(field.name())},
          {"key_type", MapKeyCType(field)},
          {"val_type", MapValueCType(field)},
          {"val_type_const", MapValueCTypeConst(field)},
          {"key_size", MapKeySize(field, MapKeyCType(field))},
          {"val_size", MapValueSize(field, MapValueCType(field))},
          {"field_init", FieldInitializerStrong(c.pools(), field, c.options())},
          {"map_getter", kMapGetterPostfix},
          {"mutable_map_getter", kMutableMapGetterPostfix},
      },
      R"cc(
        UPB_INLINE size_t $msg_name$_$name$_size(const $msg_name$* msg) {
          const upb_MiniTableField field = $field_init$;
          const upb_Map* map = upb_Message_GetMap(UPB_UPCAST(msg), &field);
          return map ? _upb_Map_Size(map) : 0;
        }

        UPB_INLINE bool $msg_name$_$name$_get(const $msg_name$* msg,
                                              $key_type$ key, $val_type$* val) {
          const upb_MiniTableField field = $field_init$;
          const upb_Map* map = upb_Message_GetMap(UPB_UPCAST(msg), &field);
          if (!map) return false;
          return _upb_Map_Get(map, &key, $key_size$, val, $val_size$);
        }

        UPB_INLINE bool $msg_name$_$name$_next(const $msg_name$* msg,
                                               $key_type$* key,
                                               $val_type_const$* val,
                                               size_t* iter) {
          const upb_MiniTableField field = $field_init$;
          const upb_Map* map = upb_Message_GetMap(UPB_UPCAST(msg), &field);
          if (!map) return false;
          upb_MessageValue k;
          upb_MessageValue v;
          if (!upb_Map_Next(map, &k, &v, iter)) return false;
          memcpy(key, &k, sizeof(*key));
          memcpy(val, &v, sizeof(*val));
          return true;
        }

        //~ Generate private getter returning a upb_Map or NULL for immutable
        // and ~ a upb_Map for mutable.
        //
        //~ Example:
        //~   UPB_INLINE const upb_Map* _name_immutable_upb_map(Foo* msg)
        //~   UPB_INLINE upb_Map* _name_mutable_upb_map(Foo* msg, upb_Arena* a)
        UPB_INLINE const upb_Map* _$msg_name$_$name$_$map_getter$($msg_name$* msg) {
          const upb_MiniTableField field = $field_init$;
          return upb_Message_GetMap(UPB_UPCAST(msg), &field);
        }

        UPB_INLINE upb_Map* _$msg_name$_$name$_$mutable_map_getter$(
            $msg_name$* msg, upb_Arena* a) {
          const upb_MiniTableField field = $field_init$;
          return _upb_Message_GetOrCreateMutableMap(UPB_UPCAST(msg), &field,
                                                    $key_size$, $val_size$, a);
        }
      )cc");
}

void GenerateRepeatedGetters(Context& c, upb::FieldDefPtr field,
                             absl::string_view msg_name,
                             const NameMangler& mangler) {
  c.Emit(
      {
          {"msg_name", msg_name},
          {"name", mangler.ResolveFieldName(field.name())},
          {"ctype_const", CTypeConst(field)},
          {"field_init", FieldInitializerStrong(c.pools(), field, c.options())},
          {"array_getter", kRepeatedFieldArrayGetterPostfix},
          {"mutable_array_getter", kRepeatedFieldMutableArrayGetterPostfix},
      },
      R"cc(
        //~ Generate getter returning first item and size.
        //~
        //~ Example:
        //~   UPB_INLINE const struct Bar* const* name(const Foo* msg,
        //~                                            size_t* size)
        UPB_INLINE $ctype_const$ const* $msg_name$_$name$(const $msg_name$* msg,
                                                          size_t* size) {
          const upb_MiniTableField field = $field_init$;
          const upb_Array* arr = upb_Message_GetArray(UPB_UPCAST(msg), &field);
          if (arr) {
            if (size) *size = arr->UPB_PRIVATE(size);
            return ($ctype_const$ const*)upb_Array_DataPtr(arr);
          } else {
            if (size) *size = 0;
            return NULL;
          }
        }

        //~ Generate private getter returning array or NULL for immutable and
        //~ upb_Array for mutable.
        //
        //~ Example:
        //~   UPB_INLINE const upb_Array* _name_upbarray(size_t* size)
        //~   UPB_INLINE upb_Array* _name_mutable_upbarray(size_t* size)
        UPB_INLINE const upb_Array* _$msg_name$_$name$_$array_getter$(
            const $msg_name$* msg, size_t* size) {
          const upb_MiniTableField field = $field_init$;
          const upb_Array* arr = upb_Message_GetArray(UPB_UPCAST(msg), &field);
          if (size) {
            *size = arr ? arr->UPB_PRIVATE(size) : 0;
          }
          return arr;
        }

        UPB_INLINE upb_Array* _$msg_name$_$name$_$mutable_array_getter$(
            $msg_name$* msg, size_t* size, upb_Arena* arena) {
          const upb_MiniTableField field = $field_init$;
          upb_Array* arr = upb_Message_GetOrCreateMutableArray(UPB_UPCAST(msg),
                                                               &field, arena);
          if (size) {
            *size = arr ? arr->UPB_PRIVATE(size) : 0;
          }
          return arr;
        }
      )cc");
}

void GenerateScalarGetters(Context& c, upb::FieldDefPtr field,
                           absl::string_view msg_name,
                           const NameMangler& mangler) {
  std::string field_name = mangler.ResolveFieldName(field.name());
  c.Emit(
      {
          {"ctype_const", CTypeConst(field)},
          {"msg_name", msg_name},
          {"name", field_name},
          {"default", FieldDefault(field)},
          {"field_init", FieldInitializerStrong(c.pools(), field, c.options())},
      },
      R"cc(
        UPB_INLINE $ctype_const$ $msg_name$_$name$(const $msg_name$* msg) {
          $ctype_const$ default_val = $default$;
          $ctype_const$ ret;
          const upb_MiniTableField field = $field_init$;
          _upb_Message_GetNonExtensionField(UPB_UPCAST(msg), &field,
                                            &default_val, &ret);
          return ret;
        }
      )cc");
}

void GenerateGetters(Context& c, upb::FieldDefPtr field,
                     absl::string_view msg_name, const NameMangler& mangler) {
  if (field.IsMap()) {
    GenerateMapGetters(c, field, msg_name, mangler);
  } else if (field.IsSequence()) {
    GenerateRepeatedGetters(c, field, msg_name, mangler);
  } else {
    GenerateScalarGetters(c, field, msg_name, mangler);
  }
}

void GenerateMapSetters(Context& c, upb::FieldDefPtr field,
                        absl::string_view msg_name,
                        const NameMangler& mangler) {
  c.Emit(
      {
          {"msg_name", msg_name},
          {"name", mangler.ResolveFieldName(field.name())},
          {"key_type", MapKeyCType(field)},
          {"val_type", MapValueCType(field)},
          {"field_init", FieldInitializerStrong(c.pools(), field, c.options())},
          {"key_size", MapKeySize(field, "key")},
          {"val_size", MapValueSize(field, "val")},
      },
      R"cc(
        UPB_INLINE void $msg_name$_$name$_clear($msg_name$* msg) {
          const upb_MiniTableField field = $field_init$;
          upb_Map* map = (upb_Map*)upb_Message_GetMap(UPB_UPCAST(msg), &field);
          if (!map) return;
          _upb_Map_Clear(map);
        }

        UPB_INLINE bool $msg_name$_$name$_set($msg_name$* msg, $key_type$ key,
                                              $val_type$ val, upb_Arena* a) {
          const upb_MiniTableField field = $field_init$;
          upb_Map* map = _upb_Message_GetOrCreateMutableMap(
              UPB_UPCAST(msg), &field, $key_size$, $val_size$, a);
          return _upb_Map_Insert(map, &key, $key_size$, &val, $val_size$, a) !=
                 kUpb_MapInsertStatus_OutOfMemory;
        }

        UPB_INLINE bool $msg_name$_$name$_delete($msg_name$* msg, $key_type$ key) {
          const upb_MiniTableField field = $field_init$;
          upb_Map* map = (upb_Map*)upb_Message_GetMap(UPB_UPCAST(msg), &field);
          if (!map) return false;
          return _upb_Map_Delete(map, &key, $key_size$, NULL);
        }
      )cc");
}

void GenerateRepeatedSetters(Context& c, upb::FieldDefPtr field,
                             absl::string_view msg_name,
                             const NameMangler& mangler) {
  c.Emit(
      {
          {"ctype", CType(field)},
          {"msg_name", msg_name},
          {"name", mangler.ResolveFieldName(field.name())},
          {"field_init", FieldInitializerStrong(c.pools(), field, c.options())},
      },
      R"cc(
        UPB_INLINE $ctype$* $msg_name$_mutable_$name$($msg_name$* msg,
                                                      size_t* size) {
          upb_MiniTableField field = $field_init$;
          upb_Array* arr = upb_Message_GetMutableArray(UPB_UPCAST(msg), &field);
          if (arr) {
            if (size) *size = arr->UPB_PRIVATE(size);
            return ($ctype$*)upb_Array_MutableDataPtr(arr);
          } else {
            if (size) *size = 0;
            return NULL;
          }
        }

        UPB_INLINE $ctype$* $msg_name$_resize_$name$($msg_name$* msg,
                                                     size_t size,
                                                     upb_Arena* arena) {
          upb_MiniTableField field = $field_init$;
          return ($ctype$*)upb_Message_ResizeArrayUninitialized(
              UPB_UPCAST(msg), &field, size, arena);
        }
      )cc");

  if (field.ctype() == kUpb_CType_Message) {
    c.Emit(
        {
            {"sub_ctype", MessageType(field.message_type())},
            {"msg_name", msg_name},
            {"name", mangler.ResolveFieldName(field.name())},
            {"sub_mini_table",
             MessageMiniTableRef(field.message_type(), c.options())},
            {"field_init",
             FieldInitializerStrong(c.pools(), field, c.options())},
        },
        R"cc(
          UPB_INLINE struct $sub_ctype$* $msg_name$_add_$name$(
              $msg_name$* msg, upb_Arena* arena) {
            upb_MiniTableField field = $field_init$;
            upb_Array* arr = upb_Message_GetOrCreateMutableArray(
                UPB_UPCAST(msg), &field, arena);
            if (!arr || !UPB_PRIVATE(_upb_Array_ResizeUninitialized)(
                            arr, arr->UPB_PRIVATE(size) + 1, arena)) {
              return NULL;
            }
            struct $sub_ctype$* sub =
                (struct $sub_ctype$*)_upb_Message_New($sub_mini_table$, arena);
            if (!arr || !sub) return NULL;
            UPB_PRIVATE(_upb_Array_Set)
            (arr, arr->UPB_PRIVATE(size) - 1, &sub, sizeof(sub));
            return sub;
          }
        )cc");
  } else {
    c.Emit(
        {{"ctype", CType(field)},
         {"msg_name", msg_name},
         {"name", mangler.ResolveFieldName(field.name())},
         {"field_init", FieldInitializerStrong(c.pools(), field, c.options())}},
        R"cc(
          UPB_INLINE bool $msg_name$_add_$name$($msg_name$* msg, $ctype$ val,
                                                upb_Arena* arena) {
            upb_MiniTableField field = $field_init$;
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
        )cc");
  }
}

void GenerateNonRepeatedSetters(Context& c, upb::FieldDefPtr field,
                                absl::string_view msg_name,
                                const NameMangler& mangler) {
  std::string field_name = mangler.ResolveFieldName(field.name());

  c.Emit(
      {{"msg_name", msg_name},
       {"name", field_name},
       {"ctype", CType(field)},
       {"field_init", FieldInitializerStrong(c.pools(), field, c.options())}},
      R"cc(
        UPB_INLINE void $msg_name$_set_$name$($msg_name$* msg, $ctype$ value) {
          const upb_MiniTableField field = $field_init$;
          upb_Message_SetBaseField((upb_Message*)msg, &field, &value);
        }
      )cc");

  // Message fields also have a Msg_mutable_foo() accessor that will create
  // the sub-message if it doesn't already exist.
  if (field.IsSubMessage()) {
    c.Emit({{"sub_ctype", MessageType(field.message_type())},
            {"msg_name", msg_name},
            {"name", field_name},
            {"sub_mini_table",
             MessageMiniTableRef(field.message_type(), c.options())}},
           R"cc(
             UPB_INLINE struct $sub_ctype$* $msg_name$_mutable_$name$(
                 $msg_name$* msg, upb_Arena* arena) {
               struct $sub_ctype$* sub = (struct $sub_ctype$*)$msg_name$_$name$(msg);
               if (sub == NULL) {
                 sub = (struct $sub_ctype$*)_upb_Message_New($sub_mini_table$, arena);
                 if (sub) $msg_name$_set_$name$(msg, sub);
               }
               return sub;
             }
           )cc");
  }
}

void GenerateSetters(Context& c, upb::FieldDefPtr field,
                     absl::string_view msg_name, const NameMangler& mangler) {
  if (field.IsMap()) {
    GenerateMapSetters(c, field, msg_name, mangler);
  } else if (field.IsSequence()) {
    GenerateRepeatedSetters(c, field, msg_name, mangler);
  } else {
    GenerateNonRepeatedSetters(c, field, msg_name, mangler);
  }
}

void GenerateMessageInHeader(Context& c, upb::MessageDefPtr message) {
  c.Emit({{"fullname", message.full_name()}}, R"cc(/* $fullname$ */
  )cc");
  std::string msg_name = MessageType(message);
  GenerateMessageFunctionsInHeader(c, message);

  for (int i = 0; i < message.real_oneof_count(); i++) {
    GenerateOneofInHeader(c, message.oneof(i), msg_name);
  }

  NameMangler mangler(GetUpbFields(message));
  for (auto field : FieldNumberOrder(message)) {
    GenerateClear(c, field, msg_name, mangler);
    GenerateGetters(c, field, msg_name, mangler);
    GenerateHazzer(c, field, msg_name, mangler);
  }

  c.Emit("\n");

  for (auto field : FieldNumberOrder(message)) {
    GenerateSetters(c, field, msg_name, mangler);
  }

  c.Emit("\n");
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

void WriteHeader(Context& c, upb::FileDefPtr file) {
  const DefPoolPair& pools = c.pools();
  const Options& options = c.options();
  const std::vector<upb::MessageDefPtr> sorted_messages = SortedMessages(file);

  // Filter out map entries.
  std::vector<upb::MessageDefPtr> this_file_messages;
  std::copy_if(
      sorted_messages.begin(), sorted_messages.end(),
      std::back_inserter(this_file_messages),
      [](const upb::MessageDefPtr& message) { return !message.mapentry(); });

  const std::vector<upb::FieldDefPtr> this_file_exts = SortedExtensions(file);
  std::vector<upb::EnumDefPtr> this_file_enums = SortedEnums(file, kAllEnums);
  std::vector<upb::MessageDefPtr> forward_messages =
      SortedForwardMessages(this_file_messages, this_file_exts);

  c.Emit(FileWarning(file.name()));
  c.Emit({{"include_guard", IncludeGuard(file.name())}},
         R"cc(#ifndef $include_guard$_UPB_H_
#define $include_guard$_UPB_H_

#include "upb/generated_code_support.h"
         )cc");

  for (int i = 0; i < file.public_dependency_count(); i++) {
    if (i == 0) {
      c.Emit(R"cc(/* Public Imports. */
      )cc");
    }
    c.Emit({{"header", CApiHeaderFilename(file.public_dependency(i).name(),
                                          options.bootstrap_stage >= 0)}},
           R"cc(#include "$header$"
           )cc");
  }
  if (file.public_dependency_count() > 0) {
    c.Emit("\n");
  }

  if (options.bootstrap_stage != 0) {
    c.Emit({{"header", MiniTableHeaderFilename(file.name(),
                                               options.bootstrap_stage >= 0)}},
           R"cc(#include "$header$"
           )cc");
    for (int i = 0; i < file.dependency_count(); i++) {
      if (options.strip_nonfunctional_codegen &&
          google::protobuf::compiler::IsKnownFeatureProto(file.dependency(i).name())) {
        // Strip feature imports for editions codegen tests.
        continue;
      }
      c.Emit(
          {{"header", MiniTableHeaderFilename(file.dependency(i).name(),
                                              options.bootstrap_stage >= 0)}},
          R"cc(#include "$header$"
          )cc");
    }
    c.Emit("\n");
  }

  c.Emit(R"cc(
    // Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
    extern "C" {
#endif
  )cc");

  if (options.bootstrap_stage == 0) {
    for (auto message : this_file_messages) {
      c.Emit(
          {
              {"name", MiniTableMessageVarName(message.full_name())},
          },
          R"cc(
            extern const upb_MiniTable* $name$(void);
          )cc");
    }
    for (auto message : forward_messages) {
      c.Emit(
          {
              {"name", MiniTableMessageVarName(message.full_name())},
          },
          R"cc(
            extern const upb_MiniTable* $name$(void);
          )cc");
    }
    for (auto enumdesc : this_file_enums) {
      c.Emit(
          {
              {"name", MiniTableEnumVarName(enumdesc.full_name())},
          },
          R"cc(
            extern const upb_MiniTableEnum* $name$(void);
          )cc");
    }
    c.Emit("\n");
  }

  // Forward-declare types defined in this file.
  for (auto message : this_file_messages) {
    c.Emit(
        {
            {"msg_type", MessageType(message)},
        },
        R"cc(
          typedef struct $msg_type$ {
            upb_Message UPB_PRIVATE(base);
          } $msg_type$;
        )cc");
    c.Emit("\n");
  }

  // Forward-declare types not in this file, but used as submessages.
  // Order by full name for consistent ordering.
  for (auto msg : forward_messages) {
    c.Emit({{"msg_type", MessageType(msg)}}, R"cc(struct $msg_type$;
    )cc");
  }

  if (!this_file_messages.empty()) {
    c.Emit("\n");
  }

  for (auto enumdesc : this_file_enums) {
    c.Emit({{"enum_type", EnumType(enumdesc)},
            Sub("enum_values", [&] { DumpEnumValues(c, enumdesc); })},
           // Avoiding R"cc( here because clang-format does really weird things
           // with it.
           R"(
             typedef enum {
               $enum_values$;
             } $enum_type$;
           )");
    c.Emit("\n");
  }

  c.Emit("\n");

  c.Emit("\n");
  for (auto message : this_file_messages) {
    GenerateMessageInHeader(c, message);
  }

  for (auto ext : this_file_exts) {
    GenerateExtensionInHeader(c, ext);
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

    c.Emit(
        {
            {"max_name_32", max32_message.full_name()},
            {"max_name_64", max64_message.full_name()},
            {"max_size_32", absl::StrCat(max32)},
            {"max_size_64", absl::StrCat(max64)},
        },
        R"(
          /* Max size 32 is $max_name_32$ */
          /* Max size 64 is $max_name_64$ */
          #define _UPB_MAXOPT_SIZE UPB_SIZE($max_size_32$, $max_size_64$)
        )");
  }

  c.Emit({{"include_guard", IncludeGuard(file.name())}},
         R"cc(#ifdef __cplusplus
              } /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* $include_guard$_UPB_H_ */
         )cc");
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

void WriteResolveCalls(Context& c, upb::MessageDefPtr msg) {
  for (int i = 0; i < msg.field_count(); i++) {
    upb::FieldDefPtr field = msg.field(i);
    if (!field.message_type() && !field.enum_subdef()) continue;
    if (field.message_type()) {
      c.Emit(
          {{"number", absl::StrCat(field.number())},
           {"msg_mini_table",
            MessageMiniTableRef(field.message_type(), c.options())}},
          R"cc(
            upb_MiniTable_SetSubMessage(
                mini_table,
                (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table,
                                                                     $number$),
                $msg_mini_table$);
          )cc");
    } else if (field.enum_subdef() && field.enum_subdef().is_closed()) {
      c.Emit(
          {{"number", absl::StrCat(field.number())},
           {"enum_mini_table",
            EnumMiniTableRef(field.enum_subdef(), c.options())}},
          R"cc(
            upb_MiniTable_SetSubEnum(
                mini_table,
                (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table,
                                                                     $number$),
                $enum_mini_table$);
          )cc");
    }
  }
}

void WriteMessageMiniDescriptorInitializer(Context& c, upb::MessageDefPtr msg) {
  c.Emit({{"name", MiniTableMessageVarName(msg.full_name())},
          {"mini_descriptor", msg.MiniDescriptorEncode()},
          {"resolve_calls", [&] { WriteResolveCalls(c, msg); }}},
         R"cc(
           const upb_MiniTable* $name$() {
             static upb_MiniTable* mini_table = NULL;
             static const char* mini_descriptor = "$mini_descriptor$";
             if (mini_table) return mini_table;
             upb_Status status;
             mini_table =
                 upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                                     upb_BootstrapArena(), &status);
             if (!mini_table) {
               fprintf(stderr, "Failed to build mini_table for $name$: %s\n",
                       upb_Status_ErrorMessage(&status));
               abort();
             }
             $resolve_calls$ return mini_table;
           }
         )cc");
  c.Emit("\n");
}

void WriteEnumMiniDescriptorInitializer(Context& c, upb::EnumDefPtr enum_def) {
  c.Emit({{"name", MiniTableEnumVarName(enum_def.full_name())},
          {"mini_descriptor", enum_def.MiniDescriptorEncode()}},
         R"cc(
           const upb_MiniTableEnum* $name$() {
             static const upb_MiniTableEnum* mini_table = NULL;
             static const char* mini_descriptor = "$mini_descriptor$";
             if (mini_table) return mini_table;
             mini_table = upb_MiniTableEnum_Build(mini_descriptor,
                                                  strlen(mini_descriptor),
                                                  upb_BootstrapArena(), NULL);
             return mini_table;
           }
         )cc");
  c.Emit("\n");
}

void WriteMiniDescriptorSource(Context& c, upb::FileDefPtr file) {
  const Options& options = c.options();
  c.Emit({{"header",
           CApiHeaderFilename(file.name(), options.bootstrap_stage >= 0)}},
         "#include <stddef.h>\n"
         "#include \"upb/generated_code_support.h\"\n"
         "#include \"$header$\"\n\n");

  for (int i = 0; i < file.dependency_count(); i++) {
    if (options.strip_nonfunctional_codegen &&
        google::protobuf::compiler::IsKnownFeatureProto(file.dependency(i).name())) {
      continue;
    }
    c.Emit({{"header", CApiHeaderFilename(file.dependency(i).name(),
                                          options.bootstrap_stage >= 0)}},
           "#include \"$header$\"\n");
  }

  c.Emit(
      R"cc(
        static upb_Arena* upb_BootstrapArena() {
          static upb_Arena* arena = NULL;
          if (!arena) arena = upb_Arena_New();
          return arena;
        }
      )cc");

  c.Emit("\n");

  for (const auto msg : SortedMessages(file)) {
    WriteMessageMiniDescriptorInitializer(c, msg);
  }

  for (const auto msg : SortedEnums(file, kClosedEnums)) {
    WriteEnumMiniDescriptorInitializer(c, msg);
  }
}

void GenerateFile(const DefPoolPair& pools, upb::FileDefPtr file,
                  const Options& options,
                  google::protobuf::compiler::GeneratorContext* context) {
  {
    auto stream =
        absl::WrapUnique(context->Open(CApiHeaderFilename(file.name(), false)));
    google::protobuf::io::Printer printer(stream.get());
    Context c(pools, options, printer);
    WriteHeader(c, file);
  }

  if (options.bootstrap_stage == 0) {
    auto stream = absl::WrapUnique(context->Open(SourceFilename(file)));
    google::protobuf::io::Printer printer(stream.get());
    Context c(pools, options, printer);
    WriteMiniDescriptorSource(c, file);
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
    return google::protobuf::Edition::EDITION_2024;
  }
};

}  // namespace

}  // namespace generator
}  // namespace upb

int main(int argc, char** argv) {
  upb::generator::CGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
