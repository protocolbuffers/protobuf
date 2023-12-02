// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "protos_generator/gen_extensions.h"

#include "absl/strings/str_cat.h"
#include "protos_generator/gen_utils.h"
#include "protos_generator/names.h"

namespace protos_generator {

namespace protobuf = ::google::protobuf;

std::string ExtensionIdentifierBase(const protobuf::FieldDescriptor* ext) {
  assert(ext->is_extension());
  std::string ext_scope;
  if (ext->extension_scope()) {
    return MessageName(ext->extension_scope());
  } else {
    return ToCIdent(ext->file()->package());
  }
}

std::string ContainingTypeName(const protobuf::FieldDescriptor* ext) {
  return ext->containing_type()->file() != ext->file()
             ? QualifiedClassName(ext->containing_type())
             : ClassName(ext->containing_type());
}

void WriteExtensionIdentifierHeader(const protobuf::FieldDescriptor* ext,
                                    Output& output) {
  std::string mini_table_name =
      absl::StrCat(ExtensionIdentifierBase(ext), "_", ext->name(), "_ext");
  if (ext->extension_scope()) {
    output(
        R"cc(
          static const ::protos::internal::ExtensionIdentifier<$0, $1> $2;
        )cc",
        ContainingTypeName(ext), CppTypeParameterName(ext), ext->name());
  } else {
    output(
        R"cc(
          extern const ::protos::internal::ExtensionIdentifier<$0, $1> $2;
        )cc",
        ContainingTypeName(ext), CppTypeParameterName(ext), ext->name());
  }
}

void WriteExtensionIdentifiersHeader(
    const std::vector<const protobuf::FieldDescriptor*>& extensions,
    Output& output) {
  for (const auto* ext : extensions) {
    if (!ext->extension_scope()) {
      WriteExtensionIdentifierHeader(ext, output);
    }
  }
}

void WriteExtensionIdentifier(const protobuf::FieldDescriptor* ext,
                              Output& output) {
  std::string mini_table_name =
      absl::StrCat(ExtensionIdentifierBase(ext), "_", ext->name(), "_ext");
  if (ext->extension_scope()) {
    output(
        R"cc(
          const ::protos::internal::ExtensionIdentifier<$0, $3> $4::$2(&$1);
        )cc",
        ContainingTypeName(ext), mini_table_name, ext->name(),
        CppTypeParameterName(ext), ClassName(ext->extension_scope()));
  } else {
    output(
        R"cc(
          const ::protos::internal::ExtensionIdentifier<$0, $3> $2(&$1);
        )cc",
        ContainingTypeName(ext), mini_table_name, ext->name(),
        CppTypeParameterName(ext));
  }
}

void WriteExtensionIdentifiers(
    const std::vector<const protobuf::FieldDescriptor*>& extensions,
    Output& output) {
  for (const auto* ext : extensions) {
    if (!ext->extension_scope()) {
      WriteExtensionIdentifier(ext, output);
    }
  }
}

}  // namespace protos_generator
