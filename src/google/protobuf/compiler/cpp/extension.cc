// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/cpp/extension.h"

#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

using ::google::protobuf::internal::cpp::IsLazilyInitializedFile;

ExtensionGenerator::ExtensionGenerator(const FieldDescriptor* descriptor,
                                       const Options& options,
                                       MessageSCCAnalyzer* scc_analyzer)
    : descriptor_(descriptor), options_(options), scc_analyzer_(scc_analyzer) {
  // Construct type_traits_.
  if (descriptor_->is_repeated()) {
    type_traits_ = "Repeated";
  }

  switch (descriptor_->cpp_type()) {
    case FieldDescriptor::CPPTYPE_ENUM:
      type_traits_.append("EnumTypeTraits< ");
      type_traits_.append(ClassName(descriptor_->enum_type(), true));
      type_traits_.append(", ");
      type_traits_.append(ClassName(descriptor_->enum_type(), true));
      type_traits_.append("_IsValid>");
      break;
    case FieldDescriptor::CPPTYPE_STRING:
      type_traits_.append("StringTypeTraits");
      break;
    case FieldDescriptor::CPPTYPE_MESSAGE:
      type_traits_.append("MessageTypeTraits< ");
      type_traits_.append(ClassName(descriptor_->message_type(), true));
      type_traits_.append(" >");
      break;
    default:
      type_traits_.append("PrimitiveTypeTraits< ");
      type_traits_.append(PrimitiveTypeName(options_, descriptor_->cpp_type()));
      type_traits_.append(" >");
      break;
  }
  SetCommonMessageDataVariables(descriptor_->containing_type(), &variables_);
  variables_["extendee"] =
      QualifiedClassName(descriptor_->containing_type(), options_);
  variables_["type_traits"] = type_traits_;
  std::string name = descriptor_->name();
  variables_["name"] = ResolveKeyword(name);
  variables_["constant_name"] = FieldConstantName(descriptor_);
  variables_["field_type"] =
      absl::StrCat(static_cast<int>(descriptor_->type()));
  variables_["packed"] = descriptor_->is_packed() ? "true" : "false";
  variables_["dllexport_decl"] = options.dllexport_decl;

  std::string scope;
  if (IsScoped()) {
    scope =
        absl::StrCat(ClassName(descriptor_->extension_scope(), false), "::");
  }

  variables_["scope"] = scope;
  variables_["scoped_name"] = ExtensionName(descriptor_);
  variables_["number"] = absl::StrCat(descriptor_->number());
}

ExtensionGenerator::~ExtensionGenerator() = default;

bool ExtensionGenerator::IsScoped() const {
  return descriptor_->extension_scope() != nullptr;
}

void ExtensionGenerator::GenerateDeclaration(io::Printer* p) const {
  auto var = p->WithVars(variables_);
  auto annotate = p->WithAnnotations({{"name", descriptor_}});

  p->Emit({{"qualifier",
            // If this is a class member, it needs to be declared "static".
            // Otherwise, it needs to be "extern".  In the latter case, it
            // also needs the DLL export/import specifier.
            IsScoped() ? "static"
            : options_.dllexport_decl.empty()
                ? "extern"
                : absl::StrCat(options_.dllexport_decl, " extern")}},
          R"cc(
            static const int $constant_name$ = $number$;
            $qualifier$ ::$proto_ns$::internal::ExtensionIdentifier<
                $extendee$, ::$proto_ns$::internal::$type_traits$, $field_type$,
                $packed$>
                $name$;
          )cc");
}

void ExtensionGenerator::GenerateDefinition(io::Printer* p) {
  auto vars = p->WithVars(variables_);
  auto generate_default_string = [&] {
    if (descriptor_->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
      // We need to declare a global string which will contain the default
      // value. We cannot declare it at class scope because that would require
      // exposing it in the header which would be annoying for other reasons. So
      // we replace :: with _ in the name and declare it as a global.
      return absl::StrReplaceAll(variables_["scoped_name"], {{"::", "_"}}) +
             "_default";
    } else if (descriptor_->message_type()) {
      // We have to initialize the default instance for extensions at
      // registration time.
      return absl::StrCat(FieldMessageTypeName(descriptor_, options_),
                          "::default_instance()");
    } else {
      return DefaultValue(options_, descriptor_);
    }
  };

  auto local_var = p->WithVars({
      {"default_str", generate_default_string()},
      {"default_val", DefaultValue(options_, descriptor_)},
      {"message_type", descriptor_->message_type() != nullptr
                           ? FieldMessageTypeName(descriptor_, options_)
                           : ""},
  });
  p->Emit(
      {
          {"declare_default_str",
           [&] {
             if (descriptor_->cpp_type() != FieldDescriptor::CPPTYPE_STRING)
               return;

             // If this is a class member, it needs to be declared in its class
             // scope.
             p->Emit(R"cc(
               const std::string $default_str$($default_val$);
             )cc");
           }},
          {"declare_const_var",
           [&] {
             if (!IsScoped()) return;
             // Likewise, class members need to declare the field constant
             // variable.
             p->Emit(R"cc(
#if !defined(_MSC_VER) || (_MSC_VER >= 1900 && _MSC_VER < 1912)
               const int $scope$$constant_name$;
#endif
             )cc");
           }},
          {"define_extension_id",
           [&] {
             if (IsLazilyInitializedFile(descriptor_->file()->name())) {
               p->Emit(R"cc(
                 PROTOBUF_CONSTINIT$ dllexport_decl$
                     PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 ::_pbi::
                         ExtensionIdentifier<$extendee$, ::_pbi::$type_traits$,
                                             $field_type$, $packed$>
                             $scoped_name$($constant_name$);
               )cc");
               return;
             }

             bool should_verify =
                 // Only verify msgs.
                 descriptor_->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
                 // Options say to verify.
                 ShouldVerify(descriptor_->message_type(), options_,
                              scc_analyzer_) &&
                 ShouldVerify(descriptor_->containing_type(), options_,
                              scc_analyzer_);

             if (!should_verify) {
               p->Emit(R"cc(
                 $dllexport_decl $PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 ::_pbi::
                     ExtensionIdentifier<$extendee$, ::_pbi::$type_traits$,
                                         $field_type$, $packed$>
                         $scoped_name$($constant_name$, $default_str$);
               )cc");
               return;
             }

             const auto& options = descriptor_->options();
             if (options.has_lazy()) {
               p->Emit(
                   {{"is_lazy", options.lazy() ? "kLazy" : "kEager"}},
                   R"cc(
                     $dllexport_decl $PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 ::_pbi::
                         ExtensionIdentifier<$extendee$, ::_pbi::$type_traits$,
                                             $field_type$, $packed$>
                             $scoped_name$($constant_name$, $default_str$,
                                           &$message_type$::InternalVerify,
                                           ::_pbi::LazyAnnotation::$is_lazy$);
                   )cc");
             } else {
               p->Emit(
                   R"cc(
                     $dllexport_decl $PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 ::_pbi::
                         ExtensionIdentifier<$extendee$, ::_pbi::$type_traits$,
                                             $field_type$, $packed$>
                             $scoped_name$($constant_name$, $default_str$,
                                           &$message_type$::InternalVerify);
                   )cc");
             }
           }},
      },
      R"cc(
        $declare_default_str$;
        $declare_const_var$;
        $define_extension_id$;
      )cc");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
