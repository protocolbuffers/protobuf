// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/hpb/gen_extensions.h"

#include <string>
#include <vector>

#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/hpb/context.h"
#include "google/protobuf/compiler/hpb/gen_utils.h"
#include "google/protobuf/compiler/hpb/names.h"
#include "google/protobuf/descriptor.h"
#include "upb_generator/c/names.h"

namespace google::protobuf::hpb_generator {

namespace protobuf = ::proto2;

std::string ExtensionIdentifierBase(const protobuf::FieldDescriptor* ext) {
  assert(ext->is_extension());
  std::string ext_scope;
  if (ext->extension_scope()) {
    return upb::generator::CApiMessageType(ext->extension_scope()->full_name());
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
                                    Context& ctx) {
  std::string mini_table_name =
      absl::StrCat(ExtensionIdentifierBase(ext), "_", ext->name(), "_ext");
  std::string linkage = ext->extension_scope() ? "static" : "extern";
  std::string ext_type = CppTypeParameterName(ext);
  if (ext->is_repeated()) {
    ext_type = absl::StrCat("::hpb::RepeatedField<", ext_type, ">");
  }
  ctx.Emit(
      {{"linkage", linkage},
       {"extendee_type", ContainingTypeName(ext)},
       {"extension_type", ext_type},
       {"extension_name", ext->name()}},
      R"cc(
        $linkage$ const ::hpb::internal::ExtensionIdentifier<$extendee_type$,
                                                             $extension_type$>
            $extension_name$;
      )cc");
}

void WriteExtensionIdentifiersHeader(
    const std::vector<const protobuf::FieldDescriptor*>& extensions,
    Context& ctx) {
  for (const auto* ext : extensions) {
    if (!ext->extension_scope()) {
      WriteExtensionIdentifierHeader(ext, ctx);
    }
  }
}

void WriteExtensionIdentifier(const protobuf::FieldDescriptor* ext,
                              Context& ctx) {
  std::string mini_table_name =
      absl::StrCat(ExtensionIdentifierBase(ext), "_", ext->name(), "_ext");
  std::string class_prefix =
      ext->extension_scope() ? ClassName(ext->extension_scope()) + "::" : "";
  std::string ext_type = CppTypeParameterName(ext);
  if (ext->is_repeated()) {
    ext_type = absl::StrCat("::hpb::RepeatedField<", ext_type, ">");
  }
  ctx.Emit(
      {{"containing_type_name", ContainingTypeName(ext)},
       {"mini_table_name", mini_table_name},
       {"ext_name", ext->name()},
       {"default_value", DefaultValue(ext)},
       {"ext_type", ext_type},
       {"class_prefix", class_prefix}},
      R"cc(
        constexpr ::hpb::internal::ExtensionIdentifier<$containing_type_name$,
                                                       $ext_type$>
            $class_prefix$$ext_name$ =
                ::hpb::internal::PrivateAccess::InvokeConstructor<
                    ::hpb::internal::ExtensionIdentifier<$containing_type_name$,
                                                         $ext_type$>>(
                    &$mini_table_name$, $default_value$);
      )cc");
}

void WriteExtensionIdentifiers(
    const std::vector<const protobuf::FieldDescriptor*>& extensions,
    Context& ctx) {
  for (const auto* ext : extensions) {
    if (!ext->extension_scope()) {
      WriteExtensionIdentifier(ext, ctx);
    }
  }
}

}  // namespace protobuf
}  // namespace google::hpb_generator
