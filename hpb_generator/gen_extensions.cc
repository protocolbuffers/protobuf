// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/hpb/gen_extensions.h"

#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/hpb/gen_utils.h"
#include "google/protobuf/compiler/hpb/names.h"
#include "google/protobuf/io/printer.h"

namespace google::protobuf::hpb_generator {

namespace protobuf = ::proto2;

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
                                    io::Printer& printer) {
  std::string mini_table_name =
      absl::StrCat(ExtensionIdentifierBase(ext), "_", ext->name(), "_ext");
  if (ext->extension_scope()) {
    printer.Emit(
        {{"containing_type", ContainingTypeName(ext)},
         {"cpp_type", CppTypeParameterName(ext)},
         {"ext_name", ext->name()}},
        R"cc(
          ystatic const ::hpb::internal::ExtensionIdentifier<$containing_type$,
                                                             $cpp_type$>
              $ext_name$;
        )cc");
  } else {
    printer.Emit(
        {{"containing_type", ContainingTypeName(ext)},
         {"cpp_type", CppTypeParameterName(ext)},
         {"ext_name", ext->name()}},
        R"cc(
          yextern const ::hpb::internal::ExtensionIdentifier<$containing_type$,
                                                             $cpp_type$>
              $ext_name$;
        )cc");
  }
}

void WriteExtensionIdentifiersHeader(
    const std::vector<const protobuf::FieldDescriptor*>& extensions,
    io::Printer& printer) {
  for (const auto* ext : extensions) {
    if (!ext->extension_scope()) {
      WriteExtensionIdentifierHeader(ext, printer);
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
          const hpb::internal::ExtensionIdentifier<$0, $3> $4::$2(&$1);
        )cc",
        ContainingTypeName(ext), mini_table_name, ext->name(),
        CppTypeParameterName(ext), ClassName(ext->extension_scope()));
  } else {
    output(
        R"cc(
          const hpb::internal::ExtensionIdentifier<$0, $3> $2(&$1);
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

}  // namespace protobuf
}  // namespace google::hpb_generator
