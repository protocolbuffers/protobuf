// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/naming.h"

#include <algorithm>
#include <cstddef>
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
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/rust_field_type.h"
#include "google/protobuf/compiler/rust/rust_keywords.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/port.h"
#include "google/protobuf/unknown_field_set.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

std::string GetCrateName(Context& ctx, const FileDescriptor& dep) {
  std::string crate_name = RsSafeName(ctx.ImportPathToCrateName(dep.name()));
  if (absl::StartsWith(crate_name, "crate::")) {
    return crate_name;
  }
  return absl::StrCat("::", crate_name);
}

std::string GetEntryPointRsFilePath(Context& ctx, const FileDescriptor& file) {
  size_t last_slash = file.name().find_last_of('/');
  return absl::StrCat(last_slash == std::string::npos
                          ? ""
                          : file.name().substr(0, last_slash + 1),
                      ctx.opts().generated_entry_point_rs_file_name);
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
  constexpr absl::string_view kCcGencodeExt = ".pb.h";

  return absl::StrCat(basename, kCcGencodeExt);
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
      return "::protobuf::ProtoBytes";
    case RustFieldType::STRING:
      return "::protobuf::ProtoString";
    case RustFieldType::MESSAGE:
      return RsTypePath(ctx, *field.message_type());
    case RustFieldType::ENUM:
      return RsTypePath(ctx, *field.enum_type());
  }
  ABSL_LOG(ERROR) << "Unknown field type: " << field.type_name();
  internal::Unreachable();
}

std::string RsTypePath(Context& ctx, const Descriptor& message) {
  return absl::StrCat(RustModule(ctx, message), MessageRsName(message));
}

std::string RsTypePath(Context& ctx, const EnumDescriptor& descriptor) {
  return absl::StrCat(RustModule(ctx, descriptor), EnumRsName(descriptor));
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
      return absl::StrFormat("&%s ::protobuf::ProtoStr", lifetime);
    case RustFieldType::MESSAGE:
      if (lifetime.empty()) {
        return absl::StrFormat("%sView",
                               RsTypePath(ctx, *field.message_type()));
      } else {
        return absl::StrFormat(
            "%sView<%s>", RsTypePath(ctx, *field.message_type()), lifetime);
      }
  }
  ABSL_LOG(FATAL) << "Unsupported field type: " << field.type_name();
  internal::Unreachable();
}

bool GetRustOptionBool(const FileDescriptor* file,
                       absl::string_view option_name) {
  // We use the dynamic pool to discover field numbers without hardcoding.
  const FieldDescriptor* dynamic_ext =
      file->pool()->FindExtensionByName("pb.file.rust");
  if (!dynamic_ext || !dynamic_ext->message_type()) return false;

  const FieldDescriptor* opt_field =
      dynamic_ext->message_type()->FindFieldByName(option_name);
  if (!opt_field || opt_field->type() != FieldDescriptor::TYPE_BOOL)
    return false;

  const Message& options = file->options();
  const Reflection* reflection = options.GetReflection();

  // ONLY use reflection if the extension is known to the static descriptor pool
  // to avoid SIGABRT cross-pool validation failures in reflection->HasField.
  const FieldDescriptor* static_ext =
      options.GetDescriptor()->file()->pool()->FindExtensionByName(
          "pb.file.rust");

  if (static_ext && reflection->HasField(options, static_ext)) {
    const Message& rust_options = reflection->GetMessage(options, static_ext);
    const FieldDescriptor* static_opt_field =
        rust_options.GetDescriptor()->FindFieldByName(option_name);
    if (static_opt_field) {
      return rust_options.GetReflection()->GetBool(rust_options,
                                                   static_opt_field);
    }
  }

  // Fallback to unknown fields.
  const UnknownFieldSet& unknown_fields = reflection->GetUnknownFields(options);
  for (int i = 0; i < unknown_fields.field_count(); ++i) {
    const UnknownField& field = unknown_fields.field(i);
    if (field.number() == dynamic_ext->number() &&
        field.type() == UnknownField::TYPE_LENGTH_DELIMITED) {
      UnknownFieldSet rust_options_unknown;
      if (rust_options_unknown.ParseFromString(field.length_delimited())) {
        for (int j = 0; j < rust_options_unknown.field_count(); ++j) {
          const UnknownField& inner = rust_options_unknown.field(j);
          if (inner.number() == opt_field->number() &&
              inner.type() == UnknownField::TYPE_VARINT) {
            return inner.varint() != 0;
          }
        }
      }
    }
  }
  return false;
}

bool EmitPackageAsMods(const FileDescriptor* file) {
  return GetRustOptionBool(file, "emit_package_as_mods");
}

static std::string RustModuleForContainingType(
    Context& ctx, const Descriptor* containing_type,
    const FileDescriptor& file) {
  std::vector<std::string> modules;

  // Innermost to outermost order.
  const Descriptor* parent = containing_type;
  while (parent != nullptr) {
    modules.push_back(RsSafeName(CamelToSnakeCase(parent->name())));
    parent = parent->containing_type();
  }

  // Reverse the vector to get submodules in outer-to-inner order).
  std::reverse(modules.begin(), modules.end());

  std::string crate_relative = absl::StrJoin(modules, "::");

  if (EmitPackageAsMods(&file)) {
    std::string pkg = std::string(file.package());
    if (!pkg.empty()) {
      std::vector<std::string> pkg_parts;
      for (absl::string_view p : absl::StrSplit(pkg, '.', absl::SkipEmpty())) {
        pkg_parts.push_back(RsSafeName(p));
      }
      std::string pkg_path = absl::StrJoin(pkg_parts, "::");
      if (crate_relative.empty()) {
        crate_relative = pkg_path;
      } else {
        crate_relative = absl::StrCat(pkg_path, "::", crate_relative);
      }
    }
  }

  if (IsInCurrentlyGeneratingCrate(ctx, file)) {
    std::string prefix;
    for (size_t i = 0; i < ctx.GetModuleDepth(); ++i) {
      prefix += "super::";
    }
    return absl::StrCat(prefix, crate_relative,
                        crate_relative.empty() ? "" : "::");
  }
  return absl::StrCat(GetCrateName(ctx, file), "::", crate_relative,
                      crate_relative.empty() ? "" : "::");
}

std::string RustModule(Context& ctx, const Descriptor& msg) {
  return RustModuleForContainingType(ctx, msg.containing_type(), *msg.file());
}

std::string RustModule(Context& ctx, const EnumDescriptor& enum_) {
  return RustModuleForContainingType(ctx, enum_.containing_type(),
                                     *enum_.file());
}

std::string RustModule(Context& ctx, const OneofDescriptor& oneof) {
  return RustModuleForContainingType(ctx, oneof.containing_type(),
                                     *oneof.file());
}

std::string RustInternalModuleName(const FileDescriptor& file) {
  std::string result;
  result.reserve(file.name().size());
  for (char c : StripProto(file.name())) {
    if (c == '_') {
      result += "__";
    } else if (c == '-') {
      result += "__";
    } else if (c == '/') {
      result += "_s";
    } else if (absl::ascii_isalnum(c)) {
      result += c;
    } else {
      // Escape any other characters that aren't valid in Rust identifiers
      // by substituting them with an underscore followed by their hex value
      // and another underscore (e.g. '.' becomes '_2e_').
      absl::StrAppendFormat(&result, "_%02x_", static_cast<unsigned char>(c));
    }
  }
  return RsSafeName(result);
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
    return absl::StrCat(name, "_");
  }
  if (IsRustKeyword(name)) {
    return absl::StrCat("r#", name);
  }
  return std::string(name);
}

namespace {

bool AnyChildMessageNamed(const FileDescriptor* scope, absl::string_view name) {
  for (int i = 0; i < scope->message_type_count(); ++i) {
    if (scope->message_type(i)->name() == name) {
      return true;
    }
  }
  return false;
}

bool AnyChildMessageNamed(const Descriptor* scope, absl::string_view name) {
  for (int i = 0; i < scope->nested_type_count(); ++i) {
    if (scope->nested_type(i)->name() == name) {
      return true;
    }
  }
  return false;
}

template <typename Desc>
bool MustMangleName(const Desc& desc) {
  // If a name ends with 'View', we check if there is a message whose name
  // matches the name without the 'View' suffix. If so, we will append an extra
  // '_' character on the end of the type that ended with 'View'. The reason we
  // special case mangle this is to avoid breakages from the View breaking.
  // https://google.aip.dev/157#view-enumeration
  if (!absl::EndsWith(desc.name(), "View")) {
    return false;
  }
  absl::string_view name_without_view_suffix =
      absl::StripSuffix(desc.name(), "View");
  return desc.containing_type() != nullptr
             ? AnyChildMessageNamed(desc.containing_type(),
                                    name_without_view_suffix)
             : AnyChildMessageNamed(desc.file(), name_without_view_suffix);
}

}  // namespace

std::string MessageRsName(const Descriptor& desc) {
  std::string name = RsSafeName(desc.name());
  if (MustMangleName(desc)) {
    absl::StrAppend(&name, "_");
  }
  return name;
}

std::string EnumRsName(const EnumDescriptor& desc) {
  std::string name = RsSafeName(SnakeToUpperCamelCase(desc.name()));
  if (MustMangleName(desc)) {
    absl::StrAppend(&name, "_");
  }
  return name;
}

std::string EnumValueRsName(const EnumValueDescriptor& value) {
  MultiCasePrefixStripper stripper(value.type()->name());
  return EnumValueRsName(stripper, value.name());
}

std::string EnumValueRsName(const MultiCasePrefixStripper& stripper,
                            absl::string_view value_name) {
  // Enum values may have a prefix of the name of the enum stripped from the
  // value names in the gencode. This prefix is flexible:
  // - It can be the original enum name, the name as UpperCamel, or
  // snake_case.
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
  return SnakeToUpperCamelCase(oneof.name()) + "Oneof";
}

std::string OneofCaseEnumRsName(const OneofDescriptor& oneof) {
  return SnakeToUpperCamelCase(oneof.name()) + "Case";
}

std::string OneofCaseEnumCppName(const OneofDescriptor& oneof) {
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

std::string CrubitCcSymbolName(const Descriptor& msg) {
  // To support forward declares of C++ types, Crubit requires that the symbol
  // literal is spelled identical to the one used in the generated bindings.
  // This requires some string mangling here to make them match.
  std::string cpp_name = cpp::QualifiedClassName(&msg);
  if (absl::StartsWith(cpp_name, "::")) {
    cpp_name = cpp_name.substr(2);
  }
  cpp_name = absl::StrReplaceAll(cpp_name,
                                 {{"::", " :: "}, {"<", " < "}, {">", " > "}});
  absl::StripTrailingAsciiWhitespace(&cpp_name);

  return cpp_name;
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

std::string DescriptorInfoName(const FileDescriptor& file) {
  std::string name =
      absl::StrReplaceAll(StripProto(file.name()), {{"/", "_"}, {"-", "_"}});
  absl::AsciiStrToUpper(&name);
  return absl::StrCat(name, "_DESCRIPTOR_INFO");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
