// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/cpp/helpers.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <queue>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/synchronization/mutex.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/compiler/scc.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/strtod.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/wire_format_lite.h"


// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {
constexpr absl::string_view kAnyMessageName = "Any";
constexpr absl::string_view kAnyProtoFile = "google/protobuf/any.proto";

static const char* const kKeywordList[] = {
    // clang-format off
    "NULL",
    "alignas",
    "alignof",
    "and",
    "and_eq",
    "asm",
    "assert",
    "auto",
    "bitand",
    "bitor",
    "bool",
    "break",
    "case",
    "catch",
    "char",
    "class",
    "compl",
    "const",
    "constexpr",
    "const_cast",
    "continue",
    "decltype",
    "default",
    "delete",
    "do",
    "double",
    "dynamic_cast",
    "else",
    "enum",
    "explicit",
    "export",
    "extern",
    "false",
    "float",
    "for",
    "friend",
    "goto",
    "if",
    "inline",
    "int",
    "long",
    "mutable",
    "namespace",
    "new",
    "noexcept",
    "not",
    "not_eq",
    "nullptr",
    "operator",
    "or",
    "or_eq",
    "private",
    "protected",
    "public",
    "register",
    "reinterpret_cast",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "static_assert",
    "static_cast",
    "struct",
    "switch",
    "template",
    "this",
    "thread_local",
    "throw",
    "true",
    "try",
    "typedef",
    "typeid",
    "typename",
    "union",
    "unsigned",
    "using",
    "virtual",
    "void",
    "volatile",
    "wchar_t",
    "while",
    "xor",
    "xor_eq",
    "char8_t",
    "char16_t",
    "char32_t",
    "concept",
    "consteval",
    "constinit",
    "co_await",
    "co_return",
    "co_yield",
    "requires",
    // clang-format on
};

const absl::flat_hash_set<absl::string_view>& Keywords() {
  static const auto* keywords = [] {
    auto* keywords = new absl::flat_hash_set<absl::string_view>();

    for (const auto keyword : kKeywordList) {
      keywords->emplace(keyword);
    }
    return keywords;
  }();
  return *keywords;
}

std::string IntTypeName(const Options& options, absl::string_view type) {
  return absl::StrCat("::", type, "_t");
}



}  // namespace

bool IsLazy(const FieldDescriptor* field, const Options& options,
            MessageSCCAnalyzer* scc_analyzer) {
  return IsLazilyVerifiedLazy(field, options) ||
         IsEagerlyVerifiedLazy(field, options, scc_analyzer);
}

// Returns true if "field" is a message field that is backed by LazyField per
// profile (go/pdlazy).
inline bool IsLazyByProfile(const FieldDescriptor* field,
                            const Options& options,
                            MessageSCCAnalyzer* scc_analyzer) {
  return false;
}

bool IsEagerlyVerifiedLazy(const FieldDescriptor* field, const Options& options,
                           MessageSCCAnalyzer* scc_analyzer) {
  return false;
}

bool IsLazilyVerifiedLazy(const FieldDescriptor* field,
                          const Options& options) {
  return false;
}

internal::field_layout::TransformValidation GetLazyStyle(
    const FieldDescriptor* field, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) {
  if (IsEagerlyVerifiedLazy(field, options, scc_analyzer)) {
    return internal::field_layout::kTvEager;
  }
  if (IsLazilyVerifiedLazy(field, options)) {
    return internal::field_layout::kTvLazy;
  }
  return {};
}

absl::flat_hash_map<absl::string_view, std::string> MessageVars(
    const Descriptor* desc) {
  absl::string_view prefix = IsMapEntryMessage(desc) ? "" : "_impl_.";
  return {
      {"any_metadata", absl::StrCat(prefix, "_any_metadata_")},
      {"cached_size", absl::StrCat(prefix, "_cached_size_")},
      {"extensions", absl::StrCat(prefix, "_extensions_")},
      {"has_bits", absl::StrCat(prefix, "_has_bits_")},
      {"inlined_string_donated_array",
       absl::StrCat(prefix, "_inlined_string_donated_")},
      {"oneof_case", absl::StrCat(prefix, "_oneof_case_")},
      {"tracker", "Impl_::_tracker_"},
      {"weak_field_map", absl::StrCat(prefix, "_weak_field_map_")},
      {"split", absl::StrCat(prefix, "_split_")},
      {"cached_split_ptr", "cached_split_ptr"},
  };
}

void SetCommonMessageDataVariables(
    const Descriptor* descriptor,
    absl::flat_hash_map<absl::string_view, std::string>* variables) {
  for (auto& pair : MessageVars(descriptor)) {
    variables->emplace(pair);
  }
}

absl::flat_hash_map<absl::string_view, std::string> UnknownFieldsVars(
    const Descriptor* desc, const Options& opts) {
  std::string unknown_fields_type;
  std::string default_instance;
  if (UseUnknownFieldSet(desc->file(), opts)) {
    unknown_fields_type =
        absl::StrCat("::", ProtobufNamespace(opts), "::UnknownFieldSet");
    default_instance = absl::StrCat(unknown_fields_type, "::default_instance");
  } else {
    unknown_fields_type =
        PrimitiveTypeName(opts, FieldDescriptor::CPPTYPE_STRING);
    default_instance = absl::StrCat("::", ProtobufNamespace(opts),
                                    "::internal::GetEmptyString");
  }

  return {
      {"unknown_fields",
       absl::Substitute("_internal_metadata_.unknown_fields<$0>($1)",
                        unknown_fields_type, default_instance)},
      {"unknown_fields_type", unknown_fields_type},
      {"have_unknown_fields", "_internal_metadata_.have_unknown_fields()"},
      {"mutable_unknown_fields",
       absl::Substitute("_internal_metadata_.mutable_unknown_fields<$0>()",
                        unknown_fields_type)},
  };
}

void SetUnknownFieldsVariable(
    const Descriptor* descriptor, const Options& options,
    absl::flat_hash_map<absl::string_view, std::string>* variables) {
  for (auto& pair : UnknownFieldsVars(descriptor, options)) {
    variables->emplace(pair);
  }
}

std::string UnderscoresToCamelCase(absl::string_view input,
                                   bool cap_next_letter) {
  std::string result;
  // Note:  I distrust ctype.h due to locales.
  for (int i = 0; i < input.size(); i++) {
    if ('a' <= input[i] && input[i] <= 'z') {
      if (cap_next_letter) {
        result += input[i] + ('A' - 'a');
      } else {
        result += input[i];
      }
      cap_next_letter = false;
    } else if ('A' <= input[i] && input[i] <= 'Z') {
      // Capital letters are left as-is.
      result += input[i];
      cap_next_letter = false;
    } else if ('0' <= input[i] && input[i] <= '9') {
      result += input[i];
      cap_next_letter = true;
    } else {
      cap_next_letter = true;
    }
  }
  return result;
}

const char kThickSeparator[] =
    "// ===================================================================\n";
const char kThinSeparator[] =
    "// -------------------------------------------------------------------\n";

bool CanInitializeByZeroing(const FieldDescriptor* field,
                            const Options& options,
                            MessageSCCAnalyzer* scc_analyzer) {
  static_assert(
      std::numeric_limits<float>::is_iec559 &&
          std::numeric_limits<double>::is_iec559,
      "proto / abseil requires iec559, which has zero initialized floats.");

  if (field->is_repeated() || field->is_extension()) return false;
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_ENUM:
      return field->default_value_enum()->number() == 0;
    case FieldDescriptor::CPPTYPE_INT32:
      return field->default_value_int32() == 0;
    case FieldDescriptor::CPPTYPE_INT64:
      return field->default_value_int64() == 0;
    case FieldDescriptor::CPPTYPE_UINT32:
      return field->default_value_uint32() == 0;
    case FieldDescriptor::CPPTYPE_UINT64:
      return field->default_value_uint64() == 0;
    case FieldDescriptor::CPPTYPE_FLOAT:
      return field->default_value_float() == 0;
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return field->default_value_double() == 0;
    case FieldDescriptor::CPPTYPE_BOOL:
      return field->default_value_bool() == false;
    case FieldDescriptor::CPPTYPE_MESSAGE:
      // Non-repeated, non-lazy message fields are raw pointers initialized to
      // null.
      return !IsLazy(field, options, scc_analyzer);
    default:
      return false;
  }
}

bool CanClearByZeroing(const FieldDescriptor* field) {
  if (field->is_repeated() || field->is_extension()) return false;
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_ENUM:
      return field->default_value_enum()->number() == 0;
    case FieldDescriptor::CPPTYPE_INT32:
      return field->default_value_int32() == 0;
    case FieldDescriptor::CPPTYPE_INT64:
      return field->default_value_int64() == 0;
    case FieldDescriptor::CPPTYPE_UINT32:
      return field->default_value_uint32() == 0;
    case FieldDescriptor::CPPTYPE_UINT64:
      return field->default_value_uint64() == 0;
    case FieldDescriptor::CPPTYPE_FLOAT:
      return field->default_value_float() == 0;
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return field->default_value_double() == 0;
    case FieldDescriptor::CPPTYPE_BOOL:
      return field->default_value_bool() == false;
    default:
      return false;
  }
}

// Determines if swap can be implemented via memcpy.
bool HasTrivialSwap(const FieldDescriptor* field, const Options& options,
                    MessageSCCAnalyzer* scc_analyzer) {
  if (field->is_repeated() || field->is_extension()) return false;
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_ENUM:
    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT32:
    case FieldDescriptor::CPPTYPE_UINT64:
    case FieldDescriptor::CPPTYPE_FLOAT:
    case FieldDescriptor::CPPTYPE_DOUBLE:
    case FieldDescriptor::CPPTYPE_BOOL:
      return true;
    case FieldDescriptor::CPPTYPE_MESSAGE:
      // Non-repeated, non-lazy message fields are simply raw pointers, so we
      // can swap them with memcpy.
      return !IsLazy(field, options, scc_analyzer);
    default:
      return false;
  }
}

std::string ClassName(const Descriptor* descriptor) {
  const Descriptor* parent = descriptor->containing_type();
  std::string res;
  if (parent) absl::StrAppend(&res, ClassName(parent), "_");
  absl::StrAppend(&res, descriptor->name());
  if (IsMapEntryMessage(descriptor)) absl::StrAppend(&res, "_DoNotUse");
  return ResolveKeyword(res);
}

std::string ClassName(const EnumDescriptor* enum_descriptor) {
  if (enum_descriptor->containing_type() == nullptr) {
    return ResolveKeyword(enum_descriptor->name());
  } else {
    return absl::StrCat(ClassName(enum_descriptor->containing_type()), "_",
                        enum_descriptor->name());
  }
}

std::string QualifiedClassName(const Descriptor* d, const Options& options) {
  return QualifiedFileLevelSymbol(d->file(), ClassName(d), options);
}

std::string QualifiedClassName(const EnumDescriptor* d,
                               const Options& options) {
  return QualifiedFileLevelSymbol(d->file(), ClassName(d), options);
}

std::string QualifiedClassName(const Descriptor* d) {
  return QualifiedClassName(d, Options());
}

std::string QualifiedClassName(const EnumDescriptor* d) {
  return QualifiedClassName(d, Options());
}

std::string ExtensionName(const FieldDescriptor* d) {
  if (const Descriptor* scope = d->extension_scope())
    return absl::StrCat(ClassName(scope), "::", ResolveKeyword(d->name()));
  return ResolveKeyword(d->name());
}

std::string QualifiedExtensionName(const FieldDescriptor* d,
                                   const Options& options) {
  ABSL_DCHECK(d->is_extension());
  return QualifiedFileLevelSymbol(d->file(), ExtensionName(d), options);
}

std::string QualifiedExtensionName(const FieldDescriptor* d) {
  return QualifiedExtensionName(d, Options());
}

std::string ResolveKeyword(absl::string_view name) {
  if (Keywords().count(name) > 0) {
    return absl::StrCat(name, "_");
  }
  return std::string(name);
}

std::string DotsToColons(absl::string_view name) {
  std::vector<std::string> scope = absl::StrSplit(name, ".", absl::SkipEmpty());
  for (auto& word : scope) {
    word = ResolveKeyword(word);
  }
  return absl::StrJoin(scope, "::");
}

std::string Namespace(absl::string_view package) {
  if (package.empty()) return "";
  return absl::StrCat("::", DotsToColons(package));
}

std::string Namespace(const FileDescriptor* d) { return Namespace(d, {}); }
std::string Namespace(const FileDescriptor* d, const Options& options) {
  return Namespace(d->package());
}

std::string Namespace(const Descriptor* d) { return Namespace(d, {}); }
std::string Namespace(const Descriptor* d, const Options& options) {
  return Namespace(d->file(), options);
}

std::string Namespace(const FieldDescriptor* d) { return Namespace(d, {}); }
std::string Namespace(const FieldDescriptor* d, const Options& options) {
  return Namespace(d->file(), options);
}

std::string Namespace(const EnumDescriptor* d) { return Namespace(d, {}); }
std::string Namespace(const EnumDescriptor* d, const Options& options) {
  return Namespace(d->file(), options);
}

std::string DefaultInstanceType(const Descriptor* descriptor,
                                const Options& /*options*/, bool split) {
  return ClassName(descriptor) + (split ? "__Impl_Split" : "") +
         "DefaultTypeInternal";
}

std::string DefaultInstanceName(const Descriptor* descriptor,
                                const Options& /*options*/, bool split) {
  return absl::StrCat("_", ClassName(descriptor, false),
                      (split ? "__Impl_Split" : ""), "_default_instance_");
}

std::string DefaultInstancePtr(const Descriptor* descriptor,
                               const Options& options, bool split) {
  return absl::StrCat(DefaultInstanceName(descriptor, options, split), "ptr_");
}

std::string QualifiedDefaultInstanceName(const Descriptor* descriptor,
                                         const Options& options, bool split) {
  return QualifiedFileLevelSymbol(
      descriptor->file(), DefaultInstanceName(descriptor, options, split),
      options);
}

std::string QualifiedDefaultInstancePtr(const Descriptor* descriptor,
                                        const Options& options, bool split) {
  return absl::StrCat(QualifiedDefaultInstanceName(descriptor, options, split),
                      "ptr_");
}

std::string DescriptorTableName(const FileDescriptor* file,
                                const Options& options) {
  return UniqueName("descriptor_table", file, options);
}

std::string FileDllExport(const FileDescriptor* file, const Options& options) {
  return UniqueName("PROTOBUF_INTERNAL_EXPORT", file, options);
}

std::string SuperClassName(const Descriptor* descriptor,
                           const Options& options) {
  if (!HasDescriptorMethods(descriptor->file(), options)) {
    return absl::StrCat("::", ProtobufNamespace(options), "::MessageLite");
  }
  auto simple_base = SimpleBaseClass(descriptor, options);
  if (simple_base.empty()) {
    return absl::StrCat("::", ProtobufNamespace(options), "::Message");
  }
  return absl::StrCat("::", ProtobufNamespace(options),
                      "::internal::", simple_base);
}

std::string FieldName(const FieldDescriptor* field) {
  std::string result = field->name();
  absl::AsciiStrToLower(&result);
  if (Keywords().count(result) > 0) {
    result.append("_");
  }
  return result;
}

std::string FieldMemberName(const FieldDescriptor* field, bool split) {
  absl::string_view prefix =
      IsMapEntryMessage(field->containing_type()) ? "" : "_impl_.";
  absl::string_view split_prefix = split ? "_split_->" : "";
  if (field->real_containing_oneof() == nullptr) {
    return absl::StrCat(prefix, split_prefix, FieldName(field), "_");
  }
  // Oneof fields are never split.
  ABSL_CHECK(!split);
  return absl::StrCat(prefix, field->containing_oneof()->name(), "_.",
                      FieldName(field), "_");
}

std::string OneofCaseConstantName(const FieldDescriptor* field) {
  ABSL_DCHECK(field->containing_oneof());
  std::string field_name = UnderscoresToCamelCase(field->name(), true);
  return absl::StrCat("k", field_name);
}

std::string QualifiedOneofCaseConstantName(const FieldDescriptor* field) {
  ABSL_DCHECK(field->containing_oneof());
  const std::string qualification =
      QualifiedClassName(field->containing_type());
  return absl::StrCat(qualification, "::", OneofCaseConstantName(field));
}

std::string EnumValueName(const EnumValueDescriptor* enum_value) {
  std::string result = enum_value->name();
  if (Keywords().count(result) > 0) {
    result.append("_");
  }
  return result;
}

int EstimateAlignmentSize(const FieldDescriptor* field) {
  if (field == nullptr) return 0;
  if (field->is_repeated()) return 8;
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_BOOL:
      return 1;

    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_UINT32:
    case FieldDescriptor::CPPTYPE_ENUM:
    case FieldDescriptor::CPPTYPE_FLOAT:
      return 4;

    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT64:
    case FieldDescriptor::CPPTYPE_DOUBLE:
    case FieldDescriptor::CPPTYPE_STRING:
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return 8;
  }
  ABSL_LOG(FATAL) << "Can't get here.";
  return -1;  // Make compiler happy.
}

int EstimateSize(const FieldDescriptor* field) {
  if (field == nullptr) return 0;
  if (field->is_repeated()) {
    if (field->is_map()) {
      return sizeof(google::protobuf::Map<int32_t, int32_t>);
    }
    return field->cpp_type() < FieldDescriptor::CPPTYPE_STRING || IsCord(field)
               ? sizeof(RepeatedField<int32_t>)
               : sizeof(internal::RepeatedPtrFieldBase);
  }
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_BOOL:
      return 1;

    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_UINT32:
    case FieldDescriptor::CPPTYPE_ENUM:
    case FieldDescriptor::CPPTYPE_FLOAT:
      return 4;

    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT64:
    case FieldDescriptor::CPPTYPE_DOUBLE:
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return 8;

    case FieldDescriptor::CPPTYPE_STRING:
      if (IsCord(field)) return sizeof(absl::Cord);
      return sizeof(internal::ArenaStringPtr);
  }
  ABSL_LOG(FATAL) << "Can't get here.";
  return -1;  // Make compiler happy.
}

std::string FieldConstantName(const FieldDescriptor* field) {
  std::string field_name = UnderscoresToCamelCase(field->name(), true);
  std::string result = absl::StrCat("k", field_name, "FieldNumber");

  if (!field->is_extension() &&
      field->containing_type()->FindFieldByCamelcaseName(
          field->camelcase_name()) != field) {
    // This field's camelcase name is not unique.  As a hack, add the field
    // number to the constant name.  This makes the constant rather useless,
    // but what can we do?
    absl::StrAppend(&result, "_", field->number());
  }

  return result;
}

std::string FieldMessageTypeName(const FieldDescriptor* field,
                                 const Options& options) {
  // Note:  The Google-internal version of Protocol Buffers uses this function
  //   as a hook point for hacks to support legacy code.
  return QualifiedClassName(field->message_type(), options);
}

std::string StripProto(absl::string_view filename) {
  /*
   * TODO remove this proxy method
   * once Google's internal codebase will become ready
   */
  return compiler::StripProto(filename);
}

const char* PrimitiveTypeName(FieldDescriptor::CppType type) {
  switch (type) {
    case FieldDescriptor::CPPTYPE_INT32:
      return "::int32_t";
    case FieldDescriptor::CPPTYPE_INT64:
      return "::int64_t";
    case FieldDescriptor::CPPTYPE_UINT32:
      return "::uint32_t";
    case FieldDescriptor::CPPTYPE_UINT64:
      return "::uint64_t";
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return "double";
    case FieldDescriptor::CPPTYPE_FLOAT:
      return "float";
    case FieldDescriptor::CPPTYPE_BOOL:
      return "bool";
    case FieldDescriptor::CPPTYPE_ENUM:
      return "int";
    case FieldDescriptor::CPPTYPE_STRING:
      return "std::string";
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return nullptr;

      // No default because we want the compiler to complain if any new
      // CppTypes are added.
  }

  ABSL_LOG(FATAL) << "Can't get here.";
  return nullptr;
}

std::string PrimitiveTypeName(const Options& options,
                              FieldDescriptor::CppType type) {
  switch (type) {
    case FieldDescriptor::CPPTYPE_INT32:
      return IntTypeName(options, "int32");
    case FieldDescriptor::CPPTYPE_INT64:
      return IntTypeName(options, "int64");
    case FieldDescriptor::CPPTYPE_UINT32:
      return IntTypeName(options, "uint32");
    case FieldDescriptor::CPPTYPE_UINT64:
      return IntTypeName(options, "uint64");
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return "double";
    case FieldDescriptor::CPPTYPE_FLOAT:
      return "float";
    case FieldDescriptor::CPPTYPE_BOOL:
      return "bool";
    case FieldDescriptor::CPPTYPE_ENUM:
      return "int";
    case FieldDescriptor::CPPTYPE_STRING:
      return "std::string";
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return "";

      // No default because we want the compiler to complain if any new
      // CppTypes are added.
  }

  ABSL_LOG(FATAL) << "Can't get here.";
  return "";
}

const char* DeclaredTypeMethodName(FieldDescriptor::Type type) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32:
      return "Int32";
    case FieldDescriptor::TYPE_INT64:
      return "Int64";
    case FieldDescriptor::TYPE_UINT32:
      return "UInt32";
    case FieldDescriptor::TYPE_UINT64:
      return "UInt64";
    case FieldDescriptor::TYPE_SINT32:
      return "SInt32";
    case FieldDescriptor::TYPE_SINT64:
      return "SInt64";
    case FieldDescriptor::TYPE_FIXED32:
      return "Fixed32";
    case FieldDescriptor::TYPE_FIXED64:
      return "Fixed64";
    case FieldDescriptor::TYPE_SFIXED32:
      return "SFixed32";
    case FieldDescriptor::TYPE_SFIXED64:
      return "SFixed64";
    case FieldDescriptor::TYPE_FLOAT:
      return "Float";
    case FieldDescriptor::TYPE_DOUBLE:
      return "Double";

    case FieldDescriptor::TYPE_BOOL:
      return "Bool";
    case FieldDescriptor::TYPE_ENUM:
      return "Enum";

    case FieldDescriptor::TYPE_STRING:
      return "String";
    case FieldDescriptor::TYPE_BYTES:
      return "Bytes";
    case FieldDescriptor::TYPE_GROUP:
      return "Group";
    case FieldDescriptor::TYPE_MESSAGE:
      return "Message";

      // No default because we want the compiler to complain if any new
      // types are added.
  }
  ABSL_LOG(FATAL) << "Can't get here.";
  return "";
}

std::string Int32ToString(int number) {
  if (number == std::numeric_limits<int32_t>::min()) {
    // This needs to be special-cased, see explanation here:
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52661
    return absl::StrCat(number + 1, " - 1");
  } else {
    return absl::StrCat(number);
  }
}

static std::string Int64ToString(int64_t number) {
  if (number == std::numeric_limits<int64_t>::min()) {
    // This needs to be special-cased, see explanation here:
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52661
    return absl::StrCat("::int64_t{", number + 1, "} - 1");
  }
  return absl::StrCat("::int64_t{", number, "}");
}

static std::string UInt64ToString(uint64_t number) {
  return absl::StrCat("::uint64_t{", number, "u}");
}

std::string DefaultValue(const FieldDescriptor* field) {
  return DefaultValue(Options(), field);
}

std::string DefaultValue(const Options& options, const FieldDescriptor* field) {
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return Int32ToString(field->default_value_int32());
    case FieldDescriptor::CPPTYPE_UINT32:
      return absl::StrCat(field->default_value_uint32(), "u");
    case FieldDescriptor::CPPTYPE_INT64:
      return Int64ToString(field->default_value_int64());
    case FieldDescriptor::CPPTYPE_UINT64:
      return UInt64ToString(field->default_value_uint64());
    case FieldDescriptor::CPPTYPE_DOUBLE: {
      double value = field->default_value_double();
      if (value == std::numeric_limits<double>::infinity()) {
        return "std::numeric_limits<double>::infinity()";
      } else if (value == -std::numeric_limits<double>::infinity()) {
        return "-std::numeric_limits<double>::infinity()";
      } else if (value != value) {
        return "std::numeric_limits<double>::quiet_NaN()";
      } else {
        return io::SimpleDtoa(value);
      }
    }
    case FieldDescriptor::CPPTYPE_FLOAT: {
      float value = field->default_value_float();
      if (value == std::numeric_limits<float>::infinity()) {
        return "std::numeric_limits<float>::infinity()";
      } else if (value == -std::numeric_limits<float>::infinity()) {
        return "-std::numeric_limits<float>::infinity()";
      } else if (value != value) {
        return "std::numeric_limits<float>::quiet_NaN()";
      } else {
        std::string float_value = io::SimpleFtoa(value);
        // If floating point value contains a period (.) or an exponent
        // (either E or e), then append suffix 'f' to make it a float
        // literal.
        if (float_value.find_first_of(".eE") != std::string::npos) {
          float_value.push_back('f');
        }
        return float_value;
      }
    }
    case FieldDescriptor::CPPTYPE_BOOL:
      return field->default_value_bool() ? "true" : "false";
    case FieldDescriptor::CPPTYPE_ENUM:
      // Lazy:  Generate a static_cast because we don't have a helper function
      //   that constructs the full name of an enum value.
      return absl::Substitute(
          "static_cast< $0 >($1)", ClassName(field->enum_type(), true),
          Int32ToString(field->default_value_enum()->number()));
    case FieldDescriptor::CPPTYPE_STRING:
      return absl::StrCat(
          "\"", EscapeTrigraphs(absl::CEscape(field->default_value_string())),
          "\"");
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return absl::StrCat("*", FieldMessageTypeName(field, options),
                          "::internal_default_instance()");
  }
  // Can't actually get here; make compiler happy.  (We could add a default
  // case above but then we wouldn't get the nice compiler warning when a
  // new type is added.)
  ABSL_LOG(FATAL) << "Can't get here.";
  return "";
}

// Convert a file name into a valid identifier.
std::string FilenameIdentifier(absl::string_view filename) {
  std::string result;
  for (int i = 0; i < filename.size(); i++) {
    if (absl::ascii_isalnum(filename[i])) {
      result.push_back(filename[i]);
    } else {
      // Not alphanumeric.  To avoid any possibility of name conflicts we
      // use the hex code for the character.
      absl::StrAppend(&result, "_",
                      absl::Hex(static_cast<uint8_t>(filename[i])));
    }
  }
  return result;
}

std::string UniqueName(absl::string_view name, absl::string_view filename,
                       const Options& options) {
  return absl::StrCat(name, "_", FilenameIdentifier(filename));
}

// Return the qualified C++ name for a file level symbol.
std::string QualifiedFileLevelSymbol(const FileDescriptor* file,
                                     absl::string_view name,
                                     const Options& options) {
  if (file->package().empty()) {
    return absl::StrCat("::", name);
  }
  return absl::StrCat(Namespace(file, options), "::", name);
}

// Escape C++ trigraphs by escaping question marks to \?
std::string EscapeTrigraphs(absl::string_view to_escape) {
  return absl::StrReplaceAll(to_escape, {{"?", "\\?"}});
}

// Escaped function name to eliminate naming conflict.
std::string SafeFunctionName(const Descriptor* descriptor,
                             const FieldDescriptor* field,
                             absl::string_view prefix) {
  // Do not use FieldName() since it will escape keywords.
  std::string name = field->name();
  absl::AsciiStrToLower(&name);
  std::string function_name = absl::StrCat(prefix, name);
  if (descriptor->FindFieldByName(function_name)) {
    // Single underscore will also make it conflicting with the private data
    // member. We use double underscore to escape function names.
    function_name.append("__");
  } else if (Keywords().count(name) > 0) {
    // If the field name is a keyword, we append the underscore back to keep it
    // consistent with other function names.
    function_name.append("_");
  }
  return function_name;
}

bool IsProfileDriven(const Options& options) {
  return !options.bootstrap && !options.opensource_runtime &&
         options.access_info_map != nullptr;
}

bool IsRarelyPresent(const FieldDescriptor* field, const Options& options) {
  return false;
}

bool IsLikelyPresent(const FieldDescriptor* field, const Options& options) {
  return false;
}

float GetPresenceProbability(const FieldDescriptor* field,
                             const Options& options) {
  return 1.f;
}

bool IsStringInliningEnabled(const Options& options) {
  return options.force_inline_string || IsProfileDriven(options);
}

bool CanStringBeInlined(const FieldDescriptor* field) {
  // TODO: Handle inlining for any.proto.
  if (IsAnyMessage(field->containing_type())) return false;
  if (field->containing_type()->options().map_entry()) return false;
  if (field->is_repeated()) return false;

  // We rely on has bits to distinguish field presence for release_$name$.  When
  // there is no hasbit, we cannot use the address of the string instance when
  // the field has been inlined.
  if (!internal::cpp::HasHasbit(field)) return false;

  if (!IsString(field)) return false;
  if (!field->default_value_string().empty()) return false;

  return true;
}

bool IsStringInlined(const FieldDescriptor* field, const Options& options) {
  (void)field;
  (void)options;
  return false;
}

static bool HasLazyFields(const Descriptor* descriptor, const Options& options,
                          MessageSCCAnalyzer* scc_analyzer) {
  for (int field_idx = 0; field_idx < descriptor->field_count(); field_idx++) {
    if (IsLazy(descriptor->field(field_idx), options, scc_analyzer)) {
      return true;
    }
  }
  for (int idx = 0; idx < descriptor->extension_count(); idx++) {
    if (IsLazy(descriptor->extension(idx), options, scc_analyzer)) {
      return true;
    }
  }
  for (int idx = 0; idx < descriptor->nested_type_count(); idx++) {
    if (HasLazyFields(descriptor->nested_type(idx), options, scc_analyzer)) {
      return true;
    }
  }
  return false;
}

// Does the given FileDescriptor use lazy fields?
bool HasLazyFields(const FileDescriptor* file, const Options& options,
                   MessageSCCAnalyzer* scc_analyzer) {
  for (int i = 0; i < file->message_type_count(); i++) {
    const Descriptor* descriptor(file->message_type(i));
    if (HasLazyFields(descriptor, options, scc_analyzer)) {
      return true;
    }
  }
  for (int field_idx = 0; field_idx < file->extension_count(); field_idx++) {
    if (IsLazy(file->extension(field_idx), options, scc_analyzer)) {
      return true;
    }
  }
  return false;
}

bool ShouldVerify(const Descriptor* descriptor, const Options& options,
                  MessageSCCAnalyzer* scc_analyzer) {
  (void)descriptor;
  (void)options;
  (void)scc_analyzer;
  return false;
}

bool ShouldVerify(const FileDescriptor* file, const Options& options,
                  MessageSCCAnalyzer* scc_analyzer) {
  (void)file;
  (void)options;
  (void)scc_analyzer;
  return false;
}

bool ShouldVerifyRecursively(const FieldDescriptor* field) {
  (void)field;
  return false;
}

VerifySimpleType ShouldVerifySimple(const Descriptor* descriptor) {
  (void)descriptor;
  return VerifySimpleType::kCustom;
}

bool ShouldSplit(const Descriptor*, const Options&) { return false; }
bool ShouldSplit(const FieldDescriptor*, const Options&) { return false; }

bool ShouldForceAllocationOnConstruction(const Descriptor* desc,
                                         const Options& options) {
  (void)desc;
  (void)options;
  return false;
}

bool IsPresentMessage(const Descriptor* descriptor, const Options& options) {
  (void)descriptor;
  (void)options;
  // Assume that the message is present if there is no profile.
  return true;
}

const FieldDescriptor* FindHottestField(
    const std::vector<const FieldDescriptor*>& fields, const Options& options) {
  (void)fields;
  (void)options;
  return nullptr;
}

static bool HasRepeatedFields(const Descriptor* descriptor) {
  for (int i = 0; i < descriptor->field_count(); ++i) {
    if (descriptor->field(i)->label() == FieldDescriptor::LABEL_REPEATED) {
      return true;
    }
  }
  for (int i = 0; i < descriptor->nested_type_count(); ++i) {
    if (HasRepeatedFields(descriptor->nested_type(i))) return true;
  }
  return false;
}

bool HasRepeatedFields(const FileDescriptor* file) {
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (HasRepeatedFields(file->message_type(i))) return true;
  }
  return false;
}

static bool IsStringPieceField(const FieldDescriptor* field,
                               const Options& options) {
  return field->cpp_type() == FieldDescriptor::CPPTYPE_STRING &&
         internal::cpp::EffectiveStringCType(field) ==
             FieldOptions::STRING_PIECE;
}

static bool HasStringPieceFields(const Descriptor* descriptor,
                                 const Options& options) {
  for (int i = 0; i < descriptor->field_count(); ++i) {
    if (IsStringPieceField(descriptor->field(i), options)) return true;
  }
  for (int i = 0; i < descriptor->nested_type_count(); ++i) {
    if (HasStringPieceFields(descriptor->nested_type(i), options)) return true;
  }
  return false;
}

bool HasStringPieceFields(const FileDescriptor* file, const Options& options) {
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (HasStringPieceFields(file->message_type(i), options)) return true;
  }
  return false;
}

static bool IsCordField(const FieldDescriptor* field, const Options& options) {
  return field->cpp_type() == FieldDescriptor::CPPTYPE_STRING &&
         internal::cpp::EffectiveStringCType(field) == FieldOptions::CORD;
}

static bool HasCordFields(const Descriptor* descriptor,
                          const Options& options) {
  for (int i = 0; i < descriptor->field_count(); ++i) {
    if (IsCordField(descriptor->field(i), options)) return true;
  }
  for (int i = 0; i < descriptor->nested_type_count(); ++i) {
    if (HasCordFields(descriptor->nested_type(i), options)) return true;
  }
  return false;
}

bool HasCordFields(const FileDescriptor* file, const Options& options) {
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (HasCordFields(file->message_type(i), options)) return true;
  }
  return false;
}

static bool HasExtensionsOrExtendableMessage(const Descriptor* descriptor) {
  if (descriptor->extension_range_count() > 0) return true;
  if (descriptor->extension_count() > 0) return true;
  for (int i = 0; i < descriptor->nested_type_count(); ++i) {
    if (HasExtensionsOrExtendableMessage(descriptor->nested_type(i))) {
      return true;
    }
  }
  return false;
}

bool HasExtensionsOrExtendableMessage(const FileDescriptor* file) {
  if (file->extension_count() > 0) return true;
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (HasExtensionsOrExtendableMessage(file->message_type(i))) return true;
  }
  return false;
}

static bool HasMapFields(const Descriptor* descriptor) {
  for (int i = 0; i < descriptor->field_count(); ++i) {
    if (descriptor->field(i)->is_map()) {
      return true;
    }
  }
  for (int i = 0; i < descriptor->nested_type_count(); ++i) {
    if (HasMapFields(descriptor->nested_type(i))) return true;
  }
  return false;
}

bool HasMapFields(const FileDescriptor* file) {
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (HasMapFields(file->message_type(i))) return true;
  }
  return false;
}

static bool HasEnumDefinitions(const Descriptor* message_type) {
  if (message_type->enum_type_count() > 0) return true;
  for (int i = 0; i < message_type->nested_type_count(); ++i) {
    if (HasEnumDefinitions(message_type->nested_type(i))) return true;
  }
  return false;
}

bool HasEnumDefinitions(const FileDescriptor* file) {
  if (file->enum_type_count() > 0) return true;
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (HasEnumDefinitions(file->message_type(i))) return true;
  }
  return false;
}

bool IsStringOrMessage(const FieldDescriptor* field) {
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT32:
    case FieldDescriptor::CPPTYPE_UINT64:
    case FieldDescriptor::CPPTYPE_DOUBLE:
    case FieldDescriptor::CPPTYPE_FLOAT:
    case FieldDescriptor::CPPTYPE_BOOL:
    case FieldDescriptor::CPPTYPE_ENUM:
      return false;
    case FieldDescriptor::CPPTYPE_STRING:
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return true;
  }

  ABSL_LOG(FATAL) << "Can't get here.";
  return false;
}

bool IsAnyMessage(const FileDescriptor* descriptor) {
  return descriptor->name() == kAnyProtoFile;
}

bool IsAnyMessage(const Descriptor* descriptor) {
  return descriptor->name() == kAnyMessageName &&
         IsAnyMessage(descriptor->file());
}

bool IsWellKnownMessage(const FileDescriptor* file) {
  static const auto* well_known_files = new absl::flat_hash_set<std::string>{
      "google/protobuf/any.proto",
      "google/protobuf/api.proto",
      "google/protobuf/compiler/plugin.proto",
      "google/protobuf/descriptor.proto",
      "google/protobuf/duration.proto",
      "google/protobuf/empty.proto",
      "google/protobuf/field_mask.proto",
      "google/protobuf/source_context.proto",
      "google/protobuf/struct.proto",
      "google/protobuf/timestamp.proto",
      "google/protobuf/type.proto",
      "google/protobuf/wrappers.proto",
  };
  return well_known_files->find(file->name()) != well_known_files->end();
}

void NamespaceOpener::ChangeTo(absl::string_view name,
                               io::Printer::SourceLocation loc) {
  std::vector<std::string> new_stack =
      absl::StrSplit(name, "::", absl::SkipEmpty());
  size_t len = std::min(name_stack_.size(), new_stack.size());
  size_t common_idx = 0;
  while (common_idx < len) {
    if (name_stack_[common_idx] != new_stack[common_idx]) {
      break;
    }
    ++common_idx;
  }

  for (size_t i = name_stack_.size(); i > common_idx; i--) {
    p_->Emit({{"ns", name_stack_[i - 1]}}, R"(
      }  // namespace $ns$
    )",
             loc);
  }
  for (size_t i = common_idx; i < new_stack.size(); ++i) {
    p_->Emit({{"ns", new_stack[i]}}, R"(
      namespace $ns$ {
    )",
             loc);
  }

  name_stack_ = std::move(new_stack);
}

static void GenerateUtf8CheckCode(io::Printer* p, const FieldDescriptor* field,
                                  const Options& options, bool for_parse,
                                  absl::string_view params,
                                  absl::string_view strict_function,
                                  absl::string_view verify_function) {
  if (field->type() != FieldDescriptor::TYPE_STRING) return;

  auto v = p->WithVars({
      {"params", params},
      {"Strict", strict_function},
      {"Verify", verify_function},
  });

  bool is_lite =
      GetOptimizeFor(field->file(), options) == FileOptions::LITE_RUNTIME;
  switch (internal::cpp::GetUtf8CheckMode(field, is_lite)) {
    case internal::cpp::Utf8CheckMode::kStrict:
      if (for_parse) {
        p->Emit(R"cc(
          DO_($pbi$::WireFormatLite::$Strict$(
              $params$ $pbi$::WireFormatLite::PARSE, "$pkg.Msg.field$"));
        )cc");
      } else {
        p->Emit(R"cc(
          $pbi$::WireFormatLite::$Strict$(
              $params$ $pbi$::WireFormatLite::SERIALIZE, "$pkg.Msg.field$");
        )cc");
      }
      break;

    case internal::cpp::Utf8CheckMode::kVerify:
      if (for_parse) {
        p->Emit(R"cc(
          $pbi$::WireFormat::$Verify$($params$ $pbi$::WireFormat::PARSE,
                                      "$pkg.Msg.field$");
        )cc");
      } else {
        p->Emit(R"cc(
          $pbi$::WireFormat::$Verify$($params$ $pbi$::WireFormat::SERIALIZE,
                                      "$pkg.Msg.field$");
        )cc");
      }
      break;

    case internal::cpp::Utf8CheckMode::kNone:
      break;
  }
}

void GenerateUtf8CheckCodeForString(const FieldDescriptor* field,
                                    const Options& options, bool for_parse,
                                    absl::string_view parameters,
                                    const Formatter& format) {
  GenerateUtf8CheckCode(format.printer(), field, options, for_parse, parameters,
                        "VerifyUtf8String", "VerifyUTF8StringNamedField");
}

void GenerateUtf8CheckCodeForCord(const FieldDescriptor* field,
                                  const Options& options, bool for_parse,
                                  absl::string_view parameters,
                                  const Formatter& format) {
  GenerateUtf8CheckCode(format.printer(), field, options, for_parse, parameters,
                        "VerifyUtf8Cord", "VerifyUTF8CordNamedField");
}

void GenerateUtf8CheckCodeForString(io::Printer* p,
                                    const FieldDescriptor* field,
                                    const Options& options, bool for_parse,
                                    absl::string_view parameters) {
  GenerateUtf8CheckCode(p, field, options, for_parse, parameters,
                        "VerifyUtf8String", "VerifyUTF8StringNamedField");
}

void GenerateUtf8CheckCodeForCord(io::Printer* p, const FieldDescriptor* field,
                                  const Options& options, bool for_parse,
                                  absl::string_view parameters) {
  GenerateUtf8CheckCode(p, field, options, for_parse, parameters,
                        "VerifyUtf8Cord", "VerifyUTF8CordNamedField");
}

void FlattenMessagesInFile(const FileDescriptor* file,
                           std::vector<const Descriptor*>* result) {
  internal::cpp::VisitDescriptorsInFileOrder(file,
                                             [&](const Descriptor* descriptor) {
                                               result->push_back(descriptor);
                                               return std::false_type{};
                                             });
}

// TopologicalSortMessagesInFile topologically sorts and returns a vector of
// proto descriptors defined in the file provided as input.  The underlying
// graph is defined using dependency relationship between protos.  For example,
// if proto A contains proto B as a member, then proto B would be ordered before
// proto A in a topological ordering, assuming there is no mutual dependence
// between the two protos.  The topological order is used to emit proto
// declarations so that a proto is declared after all the protos it is dependent
// on have been declared (again assuming no mutual dependence).  This is needed
// in cases where we may declare proto B as a member of proto A using an object,
// instead of a pointer.
//
// The proto dependency graph can have cycles.  So instead of directly working
// with protos, we compute strong connected components (SCCs) composed of protos
// with mutual dependence.  The dependency graph on SCCs is a directed acyclic
// graph (DAG) and therefore a topological order can be computed for it i.e. an
// order where an SCC is ordered after all other SCCs it is dependent on have
// been ordered.
//
// The function below first constructs the SCC graph and then computes a
// deterministic topological order for the graph.
//
// For computing the SCC graph, we follow the following steps:
// 1. Collect the descriptors for the messages in the file.
// 2. Construct a map for descriptor to SCC mapping.
// 3. Construct a map for dependence between SCCs, referred to as
// child_to_parent_scc_map below.  This map constructed by running a BFS on the
// SCCs.
//
// For computing a deterministic topological order on the graph computed in step
// 3 above, we do the following:
// 1. Since the graph on SCCs is a DAG, therefore there will be at least one SCC
// that does not depend on other SCCs.  We first construct a list of all such
// SCCs.
// 2. Next we run a BFS starting with the list of SCCs computed in step 1.  For
// each SCC, we track the number of the SCC it is dependent on and the number of
// those SCC that have been ordered.  Once all the SCCs an SCC is dependent on
// have been ordered, this SCC is added to list of SCCs that are to be ordered
// next.
// 3. Within an SCC, the descriptors are ordered on the basis of the full_name()
// of the descriptors.
std::vector<const Descriptor*> TopologicalSortMessagesInFile(
    const FileDescriptor* file, MessageSCCAnalyzer& scc_analyzer) {
  // Collect the messages defined in this file.
  std::vector<const Descriptor*> messages_in_file = FlattenMessagesInFile(file);
  if (messages_in_file.empty()) return {};
  // Populate the map from the descriptor to the SCC to which the descriptor
  // belongs.
  absl::flat_hash_map<const Descriptor*, const SCC*> descriptor_to_scc_map;
  descriptor_to_scc_map.reserve(messages_in_file.size());
  for (const Descriptor* d : messages_in_file) {
    descriptor_to_scc_map.emplace(d, scc_analyzer.GetSCC(d));
  }
  ABSL_DCHECK(messages_in_file.size() == descriptor_to_scc_map.size())
      << "messages_in_file has duplicate messages!";
  // Each parent SCC has information about the child SCCs i.e. SCCs for fields
  // that are contained in the protos that belong to the parent SCC.  Use this
  // information to construct the inverse map from child SCC to parent SCC.
  absl::flat_hash_map<const SCC*, absl::flat_hash_set<const SCC*>>
      child_to_parent_scc_map;
  // For recording the number of edges from each SCC to other SCCs in the
  // forward map.
  absl::flat_hash_map<const SCC*, int> scc_to_outgoing_edges_map;
  std::queue<const SCC*> sccs_to_process;
  for (const auto& p : descriptor_to_scc_map) {
    sccs_to_process.push(p.second);
  }
  // Run a BFS to fill the two data structures: child_to_parent_scc_map and
  // scc_to_outgoing_edges_map.
  while (!sccs_to_process.empty()) {
    const SCC* scc = sccs_to_process.front();
    sccs_to_process.pop();
    auto& count = scc_to_outgoing_edges_map[scc];
    for (const auto& child : scc->children) {
      // Test whether this child has been seen thus far.  We do not know if the
      // children SCC vector contains unique children SCC.
      auto& parent_set = child_to_parent_scc_map[child];
      if (parent_set.empty()) {
        // Just added.
        sccs_to_process.push(child);
      }
      auto ret = parent_set.insert(scc);
      if (ret.second) {
        ++count;
      }
    }
  }
  std::vector<const SCC*> next_scc_q;
  // Find out the SCCs that do not have an outgoing edge i.e. the protos in this
  // SCC do not depend on protos other than the ones in this SCC.
  for (const auto& p : scc_to_outgoing_edges_map) {
    if (p.second == 0) {
      next_scc_q.push_back(p.first);
    }
  }
  ABSL_DCHECK(!next_scc_q.empty()) << "No independent components!";
  // Topologically sort the SCCs.
  // If an SCC no longer has an outgoing edge i.e. all the SCCs it depends on
  // have been ordered, then this SCC is now a candidate for ordering.
  std::vector<const Descriptor*> sorted_messages;
  while (!next_scc_q.empty()) {
    std::vector<const SCC*> current_scc_q;
    current_scc_q.swap(next_scc_q);
    // SCCs present in the current_scc_q are topologically equivalent to each
    // other.  Therefore they can be added to the output in any order.  We sort
    // these SCCs by the full_name() of the first descriptor that belongs to the
    // SCC.  This works well since the descriptors in each SCC are sorted by
    // full_name() and also that a descriptor can be part of only one SCC.
    std::sort(current_scc_q.begin(), current_scc_q.end(),
              [](const SCC* a, const SCC* b) {
                ABSL_DCHECK(!a->descriptors.empty()) << "No descriptors!";
                ABSL_DCHECK(!b->descriptors.empty()) << "No descriptors!";
                const Descriptor* ad = a->descriptors[0];
                const Descriptor* bd = b->descriptors[0];
                return ad->full_name() < bd->full_name();
              });
    while (!current_scc_q.empty()) {
      const SCC* scc = current_scc_q.back();
      current_scc_q.pop_back();
      // Messages in an SCC are already sorted on full_name().  So we can emit
      // them right away.
      for (const Descriptor* d : scc->descriptors) {
        // Only push messages that are defined in the file.
        if (descriptor_to_scc_map.contains(d)) {
          sorted_messages.push_back(d);
        }
      }
      // Find all the SCCs that are dependent on the current SCC.
      const auto& parents = child_to_parent_scc_map.find(scc);
      if (parents == child_to_parent_scc_map.end()) continue;
      for (const SCC* parent : parents->second) {
        auto it = scc_to_outgoing_edges_map.find(parent);
        ABSL_CHECK(it != scc_to_outgoing_edges_map.end());
        ABSL_CHECK(it->second > 0);
        // Reduce the dependency count for the SCC.  In case the dependency
        // count reaches 0, add the SCC to the list of SCCs to be ordered next.
        it->second--;
        if (it->second == 0) {
          next_scc_q.push_back(parent);
        }
      }
    }
  }
  for (const auto& p : scc_to_outgoing_edges_map) {
    ABSL_DCHECK(p.second == 0) << "SCC left behind!";
  }
  return sorted_messages;
}

bool HasWeakFields(const Descriptor* descriptor, const Options& options) {
  for (int i = 0; i < descriptor->field_count(); i++) {
    if (IsWeak(descriptor->field(i), options)) return true;
  }
  return false;
}

bool HasWeakFields(const FileDescriptor* file, const Options& options) {
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (HasWeakFields(file->message_type(i), options)) return true;
  }
  return false;
}

bool UsingImplicitWeakDescriptor(const FileDescriptor* file,
                                 const Options& options) {
  return HasDescriptorMethods(file, options) &&
         !IsBootstrapProto(options, file) &&
         options.descriptor_implicit_weak_messages &&
         !options.opensource_runtime;
}

std::string StrongReferenceToType(const Descriptor* desc,
                                  const Options& options) {
  const auto name = QualifiedDefaultInstanceName(desc, options);
  return absl::StrFormat("::%s::internal::StrongPointer<decltype(%s)*, &%s>()",
                         ProtobufNamespace(options), name, name);
}

std::string WeakDescriptorDataSection(absl::string_view prefix,
                                      const Descriptor* descriptor,
                                      int index_in_file_messages,
                                      const Options& options) {
  const auto* file = descriptor->file();

  // To make a compact name we use the index of the object in its file
  // of its name.
  // So the name could be `pb_def_3_HASH` instead of
  // `pd_def_VeryLongClassName_WithNesting_AndMoreNames_HASH`
  // We need a know common prefix to merge the sections later on.
  return UniqueName(absl::StrCat("pb_", prefix, "_", index_in_file_messages),
                    file, options);
}

bool UsingImplicitWeakFields(const FileDescriptor* file,
                             const Options& options) {
  return options.lite_implicit_weak_fields &&
         GetOptimizeFor(file, options) == FileOptions::LITE_RUNTIME;
}

bool IsImplicitWeakField(const FieldDescriptor* field, const Options& options,
                         MessageSCCAnalyzer* scc_analyzer) {
  return UsingImplicitWeakFields(field->file(), options) &&
         field->type() == FieldDescriptor::TYPE_MESSAGE &&
         !field->is_required() && !field->is_map() && !field->is_extension() &&
         !IsWellKnownMessage(field->message_type()->file()) &&
         field->message_type()->file()->name() !=
             "net/proto2/proto/descriptor.proto" &&
         // We do not support implicit weak fields between messages in the same
         // strongly-connected component.
         scc_analyzer->GetSCC(field->containing_type()) !=
             scc_analyzer->GetSCC(field->message_type());
}

MessageAnalysis MessageSCCAnalyzer::GetSCCAnalysis(const SCC* scc) {
  auto it = analysis_cache_.find(scc);
  if (it != analysis_cache_.end()) return it->second;

  MessageAnalysis result;
  if (UsingImplicitWeakFields(scc->GetFile(), options_)) {
    result.contains_weak = true;
  }
  for (int i = 0; i < scc->descriptors.size(); i++) {
    const Descriptor* descriptor = scc->descriptors[i];
    if (descriptor->extension_range_count() > 0) {
      result.contains_extension = true;
    }
    for (int j = 0; j < descriptor->field_count(); j++) {
      const FieldDescriptor* field = descriptor->field(j);
      if (field->is_required()) {
        result.contains_required = true;
      }
      if (field->options().weak()) {
        result.contains_weak = true;
      }
      switch (field->type()) {
        case FieldDescriptor::TYPE_STRING:
        case FieldDescriptor::TYPE_BYTES: {
          if (field->options().ctype() == FieldOptions::CORD) {
            result.contains_cord = true;
          }
          break;
        }
        case FieldDescriptor::TYPE_GROUP:
        case FieldDescriptor::TYPE_MESSAGE: {
          const SCC* child = analyzer_.GetSCC(field->message_type());
          if (child != scc) {
            MessageAnalysis analysis = GetSCCAnalysis(child);
            result.contains_cord |= analysis.contains_cord;
            result.contains_extension |= analysis.contains_extension;
            if (!ShouldIgnoreRequiredFieldCheck(field, options_)) {
              result.contains_required |= analysis.contains_required;
            }
            result.contains_weak |= analysis.contains_weak;
          } else {
            // This field points back into the same SCC hence the messages
            // in the SCC are recursive. Note if SCC contains more than two
            // nodes it has to be recursive, however this test also works for
            // a single node that is recursive.
            result.is_recursive = true;
          }
          break;
        }
        default:
          break;
      }
    }
  }
  // We deliberately only insert the result here. After we contracted the SCC
  // in the graph, the graph should be a DAG. Hence we shouldn't need to mark
  // nodes visited as we can never return to them. By inserting them here
  // we will go in an infinite loop if the SCC is not correct.
  return analysis_cache_[scc] = std::move(result);
}

void ListAllFields(const Descriptor* d,
                   std::vector<const FieldDescriptor*>* fields) {
  // Collect sub messages
  for (int i = 0; i < d->nested_type_count(); i++) {
    ListAllFields(d->nested_type(i), fields);
  }
  // Collect message level extensions.
  for (int i = 0; i < d->extension_count(); i++) {
    fields->push_back(d->extension(i));
  }
  // Add types of fields necessary
  for (int i = 0; i < d->field_count(); i++) {
    fields->push_back(d->field(i));
  }
}

void ListAllFields(const FileDescriptor* d,
                   std::vector<const FieldDescriptor*>* fields) {
  // Collect file level message.
  for (int i = 0; i < d->message_type_count(); i++) {
    ListAllFields(d->message_type(i), fields);
  }
  // Collect message level extensions.
  for (int i = 0; i < d->extension_count(); i++) {
    fields->push_back(d->extension(i));
  }
}

void ListAllTypesForServices(const FileDescriptor* fd,
                             std::vector<const Descriptor*>* types) {
  for (int i = 0; i < fd->service_count(); i++) {
    const ServiceDescriptor* sd = fd->service(i);
    for (int j = 0; j < sd->method_count(); j++) {
      const MethodDescriptor* method = sd->method(j);
      types->push_back(method->input_type());
      types->push_back(method->output_type());
    }
  }
}

bool GetBootstrapBasename(const Options& options, absl::string_view basename,
                          std::string* bootstrap_basename) {
  if (options.opensource_runtime) {
    return false;
  }

  static const auto* bootstrap_mapping =
      // TODO Replace these with string_view once we remove
      // StringPiece.
      new absl::flat_hash_map<absl::string_view, std::string>{
          {"net/proto2/proto/descriptor",
           "third_party/protobuf/descriptor"},
          {"third_party/protobuf/cpp_features",
           "third_party/protobuf/cpp_features"},
          {"third_party/protobuf/compiler/plugin",
           "third_party/protobuf/compiler/plugin"},
          {"net/proto2/compiler/proto/profile",
           "net/proto2/compiler/proto/profile_bootstrap"},
      };
  auto iter = bootstrap_mapping->find(basename);
  if (iter == bootstrap_mapping->end()) {
    *bootstrap_basename = std::string(basename);
    return false;
  } else {
    *bootstrap_basename = iter->second;
    return true;
  }
}

bool IsBootstrapProto(const Options& options, const FileDescriptor* file) {
  std::string my_name = StripProto(file->name());
  return GetBootstrapBasename(options, my_name, &my_name);
}

bool MaybeBootstrap(const Options& options, GeneratorContext* generator_context,
                    bool bootstrap_flag, std::string* basename) {
  std::string bootstrap_basename;
  if (!GetBootstrapBasename(options, *basename, &bootstrap_basename)) {
    return false;
  }

  if (bootstrap_flag) {
    // Adjust basename, but don't abort code generation.
    *basename = bootstrap_basename;
    return false;
  }

  auto pb_h = absl::WrapUnique(
      generator_context->Open(absl::StrCat(*basename, ".pb.h")));

  io::Printer p(pb_h.get());
  p.Emit(
      {
          {"fwd_to", bootstrap_basename},
          {"file", FilenameIdentifier(*basename)},
          {"fwd_to_suffix", options.opensource_runtime ? "pb" : "proto"},
          {"swig_evil",
           [&] {
             if (options.opensource_runtime) {
               return;
             }
             p.Emit(R"(
               #ifdef SWIG
               %include "$fwd_to$.pb.h"
               #endif  // SWIG
             )");
           }},
      },
      R"(
          #ifndef PROTOBUF_INCLUDED_$file$_FORWARD_PB_H
          #define PROTOBUF_INCLUDED_$file$_FORWARD_PB_H
          #include "$fwd_to$.$fwd_to_suffix$.h"  // IWYU pragma: export
          #endif  // PROTOBUF_INCLUDED_$file$_FORWARD_PB_H
          $swig_evil$;
      )");

  auto proto_h = absl::WrapUnique(
      generator_context->Open(absl::StrCat(*basename, ".proto.h")));
  io::Printer(proto_h.get())
      .Emit(
          {
              {"fwd_to", bootstrap_basename},
              {"file", FilenameIdentifier(*basename)},
          },
          R"(
            #ifndef PROTOBUF_INCLUDED_$file$_FORWARD_PROTO_H
            #define PROTOBUF_INCLUDED_$file$_FORWARD_PROTO_H
            #include "$fwd_to$.proto.h"  // IWYU pragma: export
            #endif // PROTOBUF_INCLUDED_$file$_FORWARD_PROTO_H
          )");

  auto pb_cc = absl::WrapUnique(
      generator_context->Open(absl::StrCat(*basename, ".pb.cc")));
  io::Printer(pb_cc.get()).PrintRaw("\n");

  (void)absl::WrapUnique(
      generator_context->Open(absl::StrCat(*basename, ".pb.h.meta")));

  (void)absl::WrapUnique(
      generator_context->Open(absl::StrCat(*basename, ".proto.h.meta")));

  // Abort code generation.
  return true;
}

static bool HasExtensionFromFile(const Message& msg, const FileDescriptor* file,
                                 const Options& options,
                                 bool* has_opt_codesize_extension) {
  std::vector<const FieldDescriptor*> fields;
  auto reflection = msg.GetReflection();
  reflection->ListFields(msg, &fields);
  for (auto field : fields) {
    const auto* field_msg = field->message_type();
    if (field_msg == nullptr) {
      // It so happens that enums Is_Valid are still generated so enums work.
      // Only messages have potential problems.
      continue;
    }
    // If this option has an extension set AND that extension is defined in the
    // same file we have bootstrap problem.
    if (field->is_extension()) {
      const auto* msg_extension_file = field->message_type()->file();
      if (msg_extension_file == file) return true;
      if (has_opt_codesize_extension &&
          GetOptimizeFor(msg_extension_file, options) ==
              FileOptions::CODE_SIZE) {
        *has_opt_codesize_extension = true;
      }
    }
    // Recurse in this field to see if there is a problem in there
    if (field->is_repeated()) {
      for (int i = 0; i < reflection->FieldSize(msg, field); i++) {
        if (HasExtensionFromFile(reflection->GetRepeatedMessage(msg, field, i),
                                 file, options, has_opt_codesize_extension)) {
          return true;
        }
      }
    } else {
      if (HasExtensionFromFile(reflection->GetMessage(msg, field), file,
                               options, has_opt_codesize_extension)) {
        return true;
      }
    }
  }
  return false;
}

static bool HasBootstrapProblem(const FileDescriptor* file,
                                const Options& options,
                                bool* has_opt_codesize_extension) {
  struct BootstrapGlobals {
    absl::Mutex mutex;
    absl::flat_hash_set<const FileDescriptor*> cached ABSL_GUARDED_BY(mutex);
    absl::flat_hash_set<const FileDescriptor*> non_cached
        ABSL_GUARDED_BY(mutex);
  };
  static auto& bootstrap_cache = *new BootstrapGlobals();

  absl::MutexLock lock(&bootstrap_cache.mutex);
  if (bootstrap_cache.cached.contains(file)) return true;
  if (bootstrap_cache.non_cached.contains(file)) return false;

  // In order to build the data structures for the reflective parse, it needs
  // to parse the serialized descriptor describing all the messages defined in
  // this file. Obviously this presents a bootstrap problem for descriptor
  // messages.
  if (file->name() == "net/proto2/proto/descriptor.proto" ||
      file->name() == "google/protobuf/descriptor.proto") {
    return true;
  }
  // Unfortunately we're not done yet. The descriptor option messages allow
  // for extensions. So we need to be able to parse these extensions in order
  // to parse the file descriptor for a file that has custom options. This is a
  // problem when these custom options extensions are defined in the same file.
  FileDescriptorProto linkedin_fd_proto;
  const DescriptorPool* pool = file->pool();
  const Descriptor* fd_proto_descriptor =
      pool->FindMessageTypeByName(linkedin_fd_proto.GetTypeName());
  // Not all pools have descriptor.proto in them. In these cases there for sure
  // are no custom options.
  if (fd_proto_descriptor == nullptr) return false;

  // It's easier to inspect file as a proto, because we can use reflection on
  // the proto to iterate over all content.
  file->CopyTo(&linkedin_fd_proto);

  // linkedin_fd_proto is a generated proto linked in the proto compiler. As
  // such it doesn't know the extensions that are potentially present in the
  // descriptor pool constructed from the protos that are being compiled. These
  // custom options are therefore in the unknown fields.
  // By building the corresponding FileDescriptorProto in the pool constructed
  // by the protos that are being compiled, ie. file's pool, the unknown fields
  // are converted to extensions.
  DynamicMessageFactory factory(pool);
  Message* fd_proto = factory.GetPrototype(fd_proto_descriptor)->New();
  fd_proto->ParseFromString(linkedin_fd_proto.SerializeAsString());

  bool res = HasExtensionFromFile(*fd_proto, file, options,
                                  has_opt_codesize_extension);
  if (res) {
    bootstrap_cache.cached.insert(file);
  } else {
    bootstrap_cache.non_cached.insert(file);
  }
  delete fd_proto;
  return res;
}

FileOptions_OptimizeMode GetOptimizeFor(const FileDescriptor* file,
                                        const Options& options,
                                        bool* has_opt_codesize_extension) {
  if (has_opt_codesize_extension) *has_opt_codesize_extension = false;
  switch (options.enforce_mode) {
    case EnforceOptimizeMode::kSpeed:
      return FileOptions::SPEED;
    case EnforceOptimizeMode::kLiteRuntime:
      return FileOptions::LITE_RUNTIME;
    case EnforceOptimizeMode::kCodeSize:
      if (file->options().optimize_for() == FileOptions::LITE_RUNTIME) {
        return FileOptions::LITE_RUNTIME;
      }
      if (HasBootstrapProblem(file, options, has_opt_codesize_extension)) {
        return FileOptions::SPEED;
      }
      return FileOptions::CODE_SIZE;
    case EnforceOptimizeMode::kNoEnforcement:
      if (file->options().optimize_for() == FileOptions::CODE_SIZE) {
        if (HasBootstrapProblem(file, options, has_opt_codesize_extension)) {
          ABSL_LOG(WARNING)
              << "Proto states optimize_for = CODE_SIZE, but we "
                 "cannot honor that because it contains custom option "
                 "extensions defined in the same proto.";
          return FileOptions::SPEED;
        }
      }
      return file->options().optimize_for();
  }

  ABSL_LOG(FATAL) << "Unknown optimization enforcement requested.";
  // The phony return below serves to silence a warning from GCC 8.
  return FileOptions::SPEED;
}

bool HasMessageFieldOrExtension(const Descriptor* desc) {
  if (desc->extension_range_count() > 0) return true;
  for (const auto* f : FieldRange(desc)) {
    if (f->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) return true;
  }
  return false;
}

std::vector<io::Printer::Sub> AnnotatedAccessors(
    const FieldDescriptor* field, absl::Span<const absl::string_view> prefixes,
    absl::optional<google::protobuf::io::AnnotationCollector::Semantic> semantic) {
  auto field_name = FieldName(field);

  std::vector<io::Printer::Sub> vars;
  for (auto prefix : prefixes) {
    vars.push_back(io::Printer::Sub(absl::StrCat(prefix, "name"),
                                    absl::StrCat(prefix, field_name))
                       .AnnotatedAs({field, semantic}));
  }

  return vars;
}

bool IsFileDescriptorProto(const FileDescriptor* file, const Options& options) {
  if (Namespace(file, options) !=
      absl::StrCat("::", ProtobufNamespace(options))) {
    return false;
  }
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (file->message_type(i)->name() == "FileDescriptorProto") return true;
  }
  return false;
}

bool ShouldGenerateClass(const Descriptor* descriptor, const Options& options) {
  return !IsMapEntryMessage(descriptor) ||
         HasDescriptorMethods(descriptor->file(), options);
}

bool HasOnDeserializeTracker(const Descriptor* descriptor,
                             const Options& options) {
  return HasTracker(descriptor, options) &&
         !options.field_listener_options.forbidden_field_listener_events
              .contains("deserialize");
}


bool NeedsPostLoopHandler(const Descriptor* descriptor,
                          const Options& options) {
  if (HasOnDeserializeTracker(descriptor, options)) {
    return true;
  }
  return false;
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
