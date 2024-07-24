// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/hpb/gen_extensions.h"

#include <cassert>
#include <string>
#include <vector>

#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/hpb/names.h"
#include "google/protobuf/compiler/hpb/output.h"
#include "google/protobuf/descriptor.h"

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
                                    Output& output) {
  std::string mini_table_name =
      absl::StrCat(ExtensionIdentifierBase(ext), "_", ext->name(), "_ext");
  if (ext->extension_scope()) {
    output(
        R"cc(
          static constexpr ::hpb::internal::ExtensionIdentifier<$0, $3> $2{$4,
                                                                           &$1};
        )cc",
        ContainingTypeName(ext), mini_table_name, ext->name(),
        CppTypeParameterName(ext), ext->number());
  } else {
    output(
        R"cc(
          inline constexpr ::hpb::internal::ExtensionIdentifier<$0, $3> $2{$4,
                                                                           &$1};
        )cc",
        ContainingTypeName(ext), mini_table_name, ext->name(),
        CppTypeParameterName(ext), ext->number());
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

}  // namespace protobuf
}  // namespace google::hpb_generator
