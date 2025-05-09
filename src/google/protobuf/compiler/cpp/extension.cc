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
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/feature_resolver.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

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
      type_traits_.append(">");
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
  variables_["name"] = ResolveKnownNameCollisions(
      descriptor_->name(),
      descriptor_->extension_scope() != nullptr ? NameContext::kMessage
                                                : NameContext::kFile,
      NameKind::kValue);
  variables_["constant_name"] = FieldConstantName(descriptor_);
  variables_["field_type"] =
      absl::StrCat(static_cast<int>(descriptor_->type()));
  // Downgrade string to bytes if it is not UTF8 validated.
  if (descriptor_->type() == FieldDescriptor::TYPE_STRING &&
      !descriptor_->requires_utf8_validation()) {
    variables_["field_type"] =
        absl::StrCat(static_cast<int>(FieldDescriptor::TYPE_BYTES));
  }
  variables_["repeated"] = descriptor_->is_repeated() ? "true" : "false";
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

namespace {
bool ShouldGenerateFeatureSetDefaultData(absl::string_view extension) {
  return extension == "pb.java" || extension == "pb.java_mutable" ||
         extension == "pb.test" || extension == "pb.proto1";
}
}  // namespace

void ExtensionGenerator::GenerateDeclaration(io::Printer* p) const {
  auto var = p->WithVars(variables_);
  auto annotate = p->WithAnnotations({{"name", descriptor_}});
  p->Emit(
      {{"constant_qualifier",
        // If this is a class member, it needs to be declared
        //   `static constexpr`.
        // Otherwise, it will be
        //   `inline constexpr`.
        IsScoped() ? "static" : ""},
       {"id_qualifier",
        // If this is a class member, it needs to be declared "static".
        // Otherwise, it needs to be "extern".  In the latter case, it
        // also needs the DLL export/import specifier.
        IsScoped() ? "static"
        : options_.dllexport_decl.empty()
            ? "extern"
            : absl::StrCat(options_.dllexport_decl, " extern")},
       {"feature_set_defaults",
        [&] {
          if (!ShouldGenerateFeatureSetDefaultData(descriptor_->full_name())) {
            return;
          }
          if (descriptor_->message_type() == nullptr) return;
          absl::string_view extendee =
              descriptor_->containing_type()->full_name();
          if (extendee != "google.protobuf.FeatureSet") return;

          std::vector<const FieldDescriptor*> extensions = {descriptor_};
          absl::StatusOr<FeatureSetDefaults> defaults =
              FeatureResolver::CompileDefaults(
                  descriptor_->containing_type(), extensions,
                  ProtocMinimumEdition(), ProtocMaximumEdition());
          ABSL_CHECK_OK(defaults);
          p->Emit(
              {{"defaults", absl::Base64Escape(defaults->SerializeAsString())},
               {"extension_type", ClassName(descriptor_->message_type(), true)},
               {"function_name", "GetFeatureSetDefaultsData"}},
              R"cc(
                namespace internal {
                template <>
                inline ::absl::string_view $function_name$<$extension_type$>() {
                  static constexpr char kDefaults[] = "$defaults$";
                  return kDefaults;
                }
                }  // namespace internal
              )cc");
        }}},
      R"cc(
        inline $constant_qualifier $constexpr int $constant_name$ = $number$;
        $id_qualifier$ $pbi$::ExtensionIdentifier<
            $extendee$, $pbi$::$type_traits$, $field_type$, $packed$>
            $name$;
        $feature_set_defaults$;
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
      return absl::StrCat("&", QualifiedDefaultInstanceName(
                                   descriptor_->message_type(), options_));
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
      },
      R"cc(
        $declare_default_str$;
        PROTOBUF_CONSTINIT$ dllexport_decl$
            PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 ::_pbi::ExtensionIdentifier<
                $extendee$, ::_pbi::$type_traits$, $field_type$, $packed$>
                $scoped_name$($constant_name$, $default_str$);
      )cc");
}

bool ExtensionGenerator::WillGenerateRegistration(InitPriority priority) {
  // When not using weak descriptors we initialize everything on priority 102.
  if (!UsingImplicitWeakDescriptor(descriptor_->file(), options_)) {
    return priority == kInitPriority102;
  }
  return true;
}

void ExtensionGenerator::GenerateRegistration(io::Printer* p,
                                              InitPriority priority) {
  ABSL_CHECK(WillGenerateRegistration(priority));
  const bool using_implicit_weak_descriptors =
      UsingImplicitWeakDescriptor(descriptor_->file(), options_);
  const auto find_index = [](auto* desc) {
    const std::vector<const Descriptor*> msgs =
        FlattenMessagesInFile(desc->file());
    return absl::c_find(msgs, desc) - msgs.begin();
  };
  auto vars = p->WithVars(variables_);
  auto vars2 = p->WithVars({{
      {"extendee_table",
       DescriptorTableName(descriptor_->containing_type()->file(), options_)},
      {"extendee_index", find_index(descriptor_->containing_type())},
      {"preregister", priority == kInitPriority101},
  }});
  switch (descriptor_->cpp_type()) {
    case FieldDescriptor::CPPTYPE_ENUM:
      if (using_implicit_weak_descriptors) {
        p->Emit({{"enum_name", ClassName(descriptor_->enum_type(), true)}},
                R"cc(
                  (::_pbi::ExtensionSet::ShouldRegisterAtThisTime(
                       {{&$extendee_table$, $extendee_index$}}, $preregister$)
                       ? ::_pbi::ExtensionSet::RegisterEnumExtension(
                             ::_pbi::GetPrototypeForWeakDescriptor(
                                 &$extendee_table$, $extendee_index$, true),
                             $number$, $field_type$, $repeated$, $packed$,
                             $enum_name$_internal_data_)
                       : (void)0),
                )cc");
      } else if (priority == kInitPriority102) {
        p->Emit({{"enum_name", ClassName(descriptor_->enum_type(), true)}},
                R"cc(
                  ::_pbi::ExtensionSet::RegisterEnumExtension(
                      &$extendee$::default_instance(), $number$, $field_type$,
                      $repeated$, $packed$, $enum_name$_internal_data_),
                )cc");
      }

      break;
    case FieldDescriptor::CPPTYPE_MESSAGE: {
      const bool should_verify =
          // Only verify msgs.
          descriptor_->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
          // Options say to verify.
          ShouldVerify(descriptor_->message_type(), options_, scc_analyzer_) &&
          ShouldVerify(descriptor_->containing_type(), options_, scc_analyzer_);
      const auto message_type = FieldMessageTypeName(descriptor_, options_);
      auto v = p->WithVars(
          {{"verify", should_verify
                          ? absl::StrCat("&", message_type, "::InternalVerify")
                          : "nullptr"},
           {"message_type", message_type},
           {"lazy", "kUndefined"}});
      if (using_implicit_weak_descriptors) {
        p->Emit(
            {
                {"extension_table",
                 DescriptorTableName(descriptor_->message_type()->file(),
                                     options_)},
                {"extension_index", find_index(descriptor_->message_type())},
            },
            R"cc(
              (::_pbi::ExtensionSet::ShouldRegisterAtThisTime(
                   {{&$extendee_table$, $extendee_index$},
                    {&$extension_table$, $extension_index$}},
                   $preregister$)
                   ? ::_pbi::ExtensionSet::RegisterMessageExtension(
                         ::_pbi::GetPrototypeForWeakDescriptor(
                             &$extendee_table$, $extendee_index$, true),
                         $number$, $field_type$, $repeated$, $packed$,
                         ::_pbi::GetPrototypeForWeakDescriptor(
                             &$extension_table$, $extension_index$, true),
                         $verify$, ::_pbi::LazyAnnotation::$lazy$)
                   : (void)0),
            )cc");
      } else if (priority == kInitPriority102) {
        p->Emit(R"cc(
          ::_pbi::ExtensionSet::RegisterMessageExtension(
              &$extendee$::default_instance(), $number$, $field_type$,
              $repeated$, $packed$, &$message_type$::default_instance(),
              $verify$, ::_pbi::LazyAnnotation::$lazy$),
        )cc");
      }
      break;
    }

    default:
      if (using_implicit_weak_descriptors) {
        p->Emit(R"cc(
          (::_pbi::ExtensionSet::ShouldRegisterAtThisTime(
               {{&$extendee_table$, $extendee_index$}}, $preregister$)
               ? ::_pbi::ExtensionSet::RegisterExtension(
                     ::_pbi::GetPrototypeForWeakDescriptor(&$extendee_table$,
                                                           $extendee_index$,
                                                           true),
                     $number$, $field_type$, $repeated$, $packed$)
               : (void)0),
        )cc");
      } else if (priority == kInitPriority102) {
        p->Emit(
            R"cc(
              ::_pbi::ExtensionSet::RegisterExtension(
                  &$extendee$::default_instance(), $number$, $field_type$,
                  $repeated$, $packed$),
            )cc");
      }

      break;
  }
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
