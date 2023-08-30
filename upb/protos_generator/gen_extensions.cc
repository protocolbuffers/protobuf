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
