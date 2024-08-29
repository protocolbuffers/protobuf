// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/naming.h"

#include <algorithm>
#include <string>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/rust_field_type.h"
#include "google/protobuf/compiler/rust/rust_keywords.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/port.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

std::string GetCrateName(Context& ctx, const FileDescriptor& dep) {
  return RsSafeName(ctx.ImportPathToCrateName(dep.name()));
}

std::string GetRsFile(Context& ctx, const FileDescriptor& file) {
  auto basename = StripProto(file.name());
  switch (auto k = ctx.opts().kernel) {
    case Kernel::kUpb:
      return absl::StrCat(basename, ".u.pb.rs");
    case Kernel::kCpp:
      return absl::StrCat(basename, ".c.pb.rs");
    default:
      ABSL_LOG(FATAL) << "Unknown kernel type: " << static_cast<int>(k);
      return "";
  }
}

std::string GetThunkCcFile(Context& ctx, const FileDescriptor& file) {
  auto basename = StripProto(file.name());
  return absl::StrCat(basename, ".pb.thunks.cc");
}

std::string GetHeaderFile(Context& ctx, const FileDescriptor& file) {
  auto basename = StripProto(file.name());
  return absl::StrCat(basename, ".proto.h");
}

std::string RawMapThunk(Context& ctx, const Descriptor& msg,
                        absl::string_view key_t, absl::string_view op) {
  return absl::StrCat("proto2_rust_thunk_Map_", key_t, "_",
                      GetUnderscoreDelimitedFullName(ctx, *&msg), "_", op);
}

std::string RawMapThunk(Context& ctx, const EnumDescriptor& desc,
                        absl::string_view key_t, absl::string_view op) {
  // Enums are always 32 bits.
  return absl::StrCat("proto2_rust_thunk_Map_", key_t, "_i32_", op);
}

std::string ThunkName(Context& ctx, const FieldDescriptor& field,
                      absl::string_view op) {
  ABSL_CHECK(ctx.is_cpp());
  return absl::StrCat("proto2_rust_thunk_",
                      UnderscoreDelimitFullName(ctx, field.full_name()), "_",
                      op);
}

std::string ThunkName(Context& ctx, const OneofDescriptor& field,
                      absl::string_view op) {
  ABSL_CHECK(ctx.is_cpp());
  return absl::StrCat("proto2_rust_thunk_",
                      UnderscoreDelimitFullName(ctx, field.full_name()), "_",
                      op);
}

std::string ThunkName(Context& ctx, const Descriptor& msg,
                      absl::string_view op) {
  absl::string_view prefix = ctx.is_cpp() ? "proto2_rust_thunk_Message_" : "";
  return absl::StrCat(prefix, GetUnderscoreDelimitedFullName(ctx, msg), "_",
                      op);
}

template <typename Desc>
std::string GetFullyQualifiedPath(Context& ctx, const Desc& desc) {
  auto rel_path = GetCrateRelativeQualifiedPath(ctx, desc);
  if (IsInCurrentlyGeneratingCrate(ctx, desc)) {
    return absl::StrCat("crate::", rel_path);
  }
  return absl::StrCat(GetCrateName(ctx, *desc.file()), "::", rel_path);
}

template <typename Desc>
std::string GetUnderscoreDelimitedFullName(Context& ctx, const Desc& desc) {
  return UnderscoreDelimitFullName(ctx, desc.full_name());
}

std::string UnderscoreDelimitFullName(Context& ctx,
                                      absl::string_view full_name) {
  std::string result = std::string(full_name);
  absl::StrReplaceAll({{".", "_"}}, &result);
  return result;
}

std::string RsTypePath(Context& ctx, const FieldDescriptor& field) {
  switch (GetRustFieldType(field)) {
    case RustFieldType::BOOL:
      return "bool";
    case RustFieldType::INT32:
      return "i32";
    case RustFieldType::INT64:
      return "i64";
    case RustFieldType::UINT32:
      return "u32";
    case RustFieldType::UINT64:
      return "u64";
    case RustFieldType::FLOAT:
      return "f32";
    case RustFieldType::DOUBLE:
      return "f64";
    case RustFieldType::BYTES:
      return "::__pb::ProtoBytes";
    case RustFieldType::STRING:
      return "::__pb::ProtoString";
    case RustFieldType::MESSAGE:
      return GetFullyQualifiedPath(ctx, *field.message_type());
    case RustFieldType::ENUM:
      return GetFullyQualifiedPath(ctx, *field.enum_type());
  }
  ABSL_LOG(ERROR) << "Unknown field type: " << field.type_name();
  internal::Unreachable();
}

std::string RsViewType(Context& ctx, const FieldDescriptor& field,
                       absl::string_view lifetime) {
  switch (GetRustFieldType(field)) {
    case RustFieldType::BOOL:
    case RustFieldType::INT32:
    case RustFieldType::INT64:
    case RustFieldType::UINT32:
    case RustFieldType::UINT64:
    case RustFieldType::FLOAT:
    case RustFieldType::DOUBLE:
    case RustFieldType::ENUM:
      // The View type of all scalars and enums can be spelled as the type
      // itself.
      return RsTypePath(ctx, field);
    case RustFieldType::BYTES:
      return absl::StrFormat("&%s [u8]", lifetime);
    case RustFieldType::STRING:
      return absl::StrFormat("&%s ::__pb::ProtoStr", lifetime);
    case RustFieldType::MESSAGE:
      if (lifetime.empty()) {
        return absl::StrFormat(
            "%sView", GetFullyQualifiedPath(ctx, *field.message_type()));
      } else {
        return absl::StrFormat(
            "%sView<%s>", GetFullyQualifiedPath(ctx, *field.message_type()),
            lifetime);
      }
  }
  ABSL_LOG(FATAL) << "Unsupported field type: " << field.type_name();
  internal::Unreachable();
}

std::string RustModuleForContainingType(Context& ctx,
                                        const Descriptor* containing_type) {
  std::vector<std::string> modules;

  // Innermost to outermost order.
  const Descriptor* parent = containing_type;
  while (parent != nullptr) {
    modules.push_back(RsSafeName(CamelToSnakeCase(parent->name())));
    parent = parent->containing_type();
  }

  // Reverse the vector to get submodules in outer-to-inner order).
  std::reverse(modules.begin(), modules.end());

  // If there are any modules at all, push an empty string on the end so that
  // we get the trailing ::
  if (!modules.empty()) {
    modules.push_back("");
  }

  return absl::StrJoin(modules, "::");
}

std::string RustModule(Context& ctx, const Descriptor& msg) {
  return RustModuleForContainingType(ctx, msg.containing_type());
}

std::string RustModule(Context& ctx, const EnumDescriptor& enum_) {
  return RustModuleForContainingType(ctx, enum_.containing_type());
}

std::string RustModule(Context& ctx, const OneofDescriptor& oneof) {
  return RustModuleForContainingType(ctx, oneof.containing_type());
}

std::string RustInternalModuleName(Context& ctx, const FileDescriptor& file) {
  return RsSafeName(
      absl::StrReplaceAll(StripProto(file.name()), {{"_", "__"}, {"/", "_s"}}));
}

std::string GetCrateRelativeQualifiedPath(Context& ctx, const Descriptor& msg) {
  return absl::StrCat(RustModule(ctx, msg), RsSafeName(msg.name()));
}

std::string GetCrateRelativeQualifiedPath(Context& ctx,
                                          const EnumDescriptor& enum_) {
  return absl::StrCat(RustModule(ctx, enum_), EnumRsName(enum_));
}

std::string FieldInfoComment(Context& ctx, const FieldDescriptor& field) {
  absl::string_view label = field.is_repeated() ? "repeated" : "optional";
  std::string comment = absl::StrCat(field.name(), ": ", label, " ",
                                     FieldDescriptor::TypeName(field.type()));

  if (auto* m = field.message_type()) {
    absl::StrAppend(&comment, " ", m->full_name());
  }
  if (auto* m = field.enum_type()) {
    absl::StrAppend(&comment, " ", m->full_name());
  }

  return comment;
}

static constexpr absl::string_view kAccessorPrefixes[] = {"clear_", "has_",
                                                          "set_"};

static constexpr absl::string_view kAccessorSuffixes[] = {"_mut", "_opt"};

std::string FieldNameWithCollisionAvoidance(const FieldDescriptor& field) {
  absl::string_view name = field.name();
  const Descriptor& msg = *field.containing_type();

  for (absl::string_view prefix : kAccessorPrefixes) {
    if (absl::StartsWith(name, prefix)) {
      absl::string_view without_prefix = name;
      without_prefix.remove_prefix(prefix.size());

      if (msg.FindFieldByName(without_prefix) != nullptr) {
        return absl::StrCat(name, "_", field.number());
      }
    }
  }

  for (absl::string_view suffix : kAccessorSuffixes) {
    if (absl::EndsWith(name, suffix)) {
      absl::string_view without_suffix = name;
      without_suffix.remove_suffix(suffix.size());

      if (msg.FindFieldByName(without_suffix) != nullptr) {
        return absl::StrCat(name, "_", field.number());
      }
    }
  }

  return std::string(name);
}

std::string RsSafeName(absl::string_view name) {
  if (!IsLegalRawIdentifierName(name)) {
    return absl::StrCat(name,
                        "__mangled_because_ident_isnt_a_legal_raw_identifier");
  }
  if (IsRustKeyword(name)) {
    return absl::StrCat("r#", name);
  }
  return std::string(name);
}

std::string EnumRsName(const EnumDescriptor& desc) {
  return RsSafeName(SnakeToUpperCamelCase(desc.name()));
}

std::string EnumValueRsName(const EnumValueDescriptor& value) {
  MultiCasePrefixStripper stripper(value.type()->name());
  return EnumValueRsName(stripper, value.name());
}

std::string EnumValueRsName(const MultiCasePrefixStripper& stripper,
                            absl::string_view value_name) {
  // Enum values may have a prefix of the name of the enum stripped from the
  // value names in the gencode. This prefix is flexible:
  // - It can be the original enum name, the name as UpperCamel, or snake_case.
  // - The stripped prefix may also end in an underscore.
  auto stripped = stripper.StripPrefix(value_name);

  auto name = ScreamingSnakeToUpperCamelCase(stripped);
  ABSL_CHECK(!name.empty());

  // Invalid identifiers are prefixed with `_`.
  if (absl::ascii_isdigit(name[0])) {
    name = absl::StrCat("_", name);
  }
  return RsSafeName(name);
}

std::string OneofViewEnumRsName(const OneofDescriptor& oneof) {
  return RsSafeName(SnakeToUpperCamelCase(oneof.name()));
}

std::string OneofCaseEnumRsName(const OneofDescriptor& oneof) {
  // Note: This is the name used for the cpp Case enum, we use it for both
  // the Rust Case enum as well as for the cpp case enum in the cpp thunk.
  return SnakeToUpperCamelCase(oneof.name()) + "Case";
}

std::string OneofCaseRsName(const FieldDescriptor& oneof_field) {
  return RsSafeName(SnakeToUpperCamelCase(oneof_field.name()));
}

std::string CamelToSnakeCase(absl::string_view input) {
  std::string result;
  result.reserve(input.size() + 4);  // No reallocation for 4 _
  bool is_first_character = true;
  bool last_char_was_underscore = false;
  for (const char c : input) {
    if (!is_first_character && absl::ascii_isupper(c) &&
        !last_char_was_underscore) {
      result += '_';
    }
    last_char_was_underscore = c == '_';
    result += absl::ascii_tolower(c);
    is_first_character = false;
  }
  return result;
}

std::string SnakeToUpperCamelCase(absl::string_view input) {
  return cpp::UnderscoresToCamelCase(input, /*cap first letter=*/true);
}

std::string ScreamingSnakeToUpperCamelCase(absl::string_view input) {
  std::string result;
  result.reserve(input.size());
  bool cap_next_letter = true;
  for (const char c : input) {
    if (absl::ascii_isalpha(c)) {
      if (cap_next_letter) {
        result += absl::ascii_toupper(c);
      } else {
        result += absl::ascii_tolower(c);
      }
      cap_next_letter = false;
    } else if (absl::ascii_isdigit(c)) {
      result += c;
      cap_next_letter = true;
    } else {
      cap_next_letter = true;
    }
  }
  return result;
}

MultiCasePrefixStripper::MultiCasePrefixStripper(absl::string_view prefix)
    : prefixes_{
          std::string(prefix),
          ScreamingSnakeToUpperCamelCase(prefix),
          CamelToSnakeCase(prefix),
      } {}

absl::string_view MultiCasePrefixStripper::StripPrefix(
    absl::string_view name) const {
  absl::string_view start_name = name;
  for (absl::string_view prefix : prefixes_) {
    if (absl::StartsWithIgnoreCase(name, prefix)) {
      name.remove_prefix(prefix.size());

      // Also strip a joining underscore, if present.
      absl::ConsumePrefix(&name, "_");

      // Only strip one prefix.
      break;
    }
  }

  if (name.empty()) {
    return start_name;
  }
  return name;
}

PROTOBUF_CONSTINIT const MapKeyType kMapKeyTypes[] = {
    {/*thunk_ident=*/"i32", /*rs_key_t=*/"i32", /*rs_ffi_key_t=*/"i32",
     /*rs_to_ffi_key_expr=*/"key", /*rs_from_ffi_key_expr=*/"ffi_key",
     /*cc_key_t=*/"int32_t", /*cc_ffi_key_t=*/"int32_t",
     /*cc_from_ffi_key_expr=*/"key",
     /*cc_to_ffi_key_expr=*/"cpp_key"},
    {/*thunk_ident=*/"u32", /*rs_key_t=*/"u32", /*rs_ffi_key_t=*/"u32",
     /*rs_to_ffi_key_expr=*/"key", /*rs_from_ffi_key_expr=*/"ffi_key",
     /*cc_key_t=*/"uint32_t", /*cc_ffi_key_t=*/"uint32_t",
     /*cc_from_ffi_key_expr=*/"key",
     /*cc_to_ffi_key_expr=*/"cpp_key"},
    {/*thunk_ident=*/"i64", /*rs_key_t=*/"i64", /*rs_ffi_key_t=*/"i64",
     /*rs_to_ffi_key_expr=*/"key", /*rs_from_ffi_key_expr=*/"ffi_key",
     /*cc_key_t=*/"int64_t", /*cc_ffi_key_t=*/"int64_t",
     /*cc_from_ffi_key_expr=*/"key",
     /*cc_to_ffi_key_expr=*/"cpp_key"},
    {/*thunk_ident=*/"u64", /*rs_key_t=*/"u64", /*rs_ffi_key_t=*/"u64",
     /*rs_to_ffi_key_expr=*/"key", /*rs_from_ffi_key_expr=*/"ffi_key",
     /*cc_key_t=*/"uint64_t", /*cc_ffi_key_t=*/"uint64_t",
     /*cc_from_ffi_key_expr=*/"key",
     /*cc_to_ffi_key_expr=*/"cpp_key"},
    {/*thunk_ident=*/"bool", /*rs_key_t=*/"bool", /*rs_ffi_key_t=*/"bool",
     /*rs_to_ffi_key_expr=*/"key", /*rs_from_ffi_key_expr=*/"ffi_key",
     /*cc_key_t=*/"bool", /*cc_ffi_key_t=*/"bool",
     /*cc_from_ffi_key_expr=*/"key",
     /*cc_to_ffi_key_expr=*/"cpp_key"},
    {/*thunk_ident=*/"ProtoString",
     /*rs_key_t=*/"$pb$::ProtoString",
     /*rs_ffi_key_t=*/"$pbr$::PtrAndLen",
     /*rs_to_ffi_key_expr=*/"key.as_bytes().into()",
     /*rs_from_ffi_key_expr=*/
     "$pb$::ProtoStr::from_utf8_unchecked(ffi_key.as_ref())",
     /*cc_key_t=*/"std::string",
     /*cc_ffi_key_t=*/"google::protobuf::rust::PtrAndLen",
     /*cc_from_ffi_key_expr=*/
     "std::string(key.ptr, key.len)", /*cc_to_ffi_key_expr=*/
     "google::protobuf::rust::PtrAndLen(cpp_key.data(), cpp_key.size())"}};

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
