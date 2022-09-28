// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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
//     * Neither the name of Google Inc. nor the names of its
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

// Author: robinson@google.com (Will Robinson)
//
// This module outputs pure-Python protocol message classes that will
// largely be constructed at runtime via the metaclass in reflection.py.
// In other words, our job is basically to output a Python equivalent
// of the C++ *Descriptor objects, and fix up all circular references
// within these objects.
//
// Note that the runtime performance of protocol message classes created in
// this way is expected to be lousy.  The plan is to create an alternate
// generator that outputs a Python/C extension module that lets
// performance-minded Python code leverage the fast C++ implementation
// directly.

#include "google/protobuf/compiler/python/generator.h"

#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/stubs/logging.h"
#include "google/protobuf/stubs/common.h"
#include "google/protobuf/stubs/strutil.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/strip.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/python/helpers.h"
#include "google/protobuf/compiler/python/pyi_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace python {

namespace {
// Returns the alias we assign to the module of the given .proto filename
// when importing. See testPackageInitializationImport in
// net/proto2/python/internal/reflection_test.py
// to see why we need the alias.
std::string ModuleAlias(const std::string& filename) {
  std::string module_name = ModuleName(filename);
  // We can't have dots in the module name, so we replace each with _dot_.
  // But that could lead to a collision between a.b and a_dot_b, so we also
  // duplicate each underscore.
  absl::StrReplaceAll({{"_", "__"}}, &module_name);
  absl::StrReplaceAll({{".", "_dot_"}}, &module_name);
  return module_name;
}

// Name of the class attribute where we store the Python
// descriptor.Descriptor instance for the generated class.
// Must stay consistent with the _DESCRIPTOR_KEY constant
// in proto2/public/reflection.py.
const char kDescriptorKey[] = "DESCRIPTOR";

const char kThirdPartyPrefix[] = "google3.third_party.py.";

// Returns a Python literal giving the default value for a field.
// If the field specifies no explicit default value, we'll return
// the default default value for the field type (zero for numbers,
// empty string for strings, empty list for repeated fields, and
// None for non-repeated, composite fields).
//
// TODO(robinson): Unify with code from
// //compiler/cpp/internal/primitive_field.cc
// //compiler/cpp/internal/enum_field.cc
// //compiler/cpp/internal/string_field.cc
std::string StringifyDefaultValue(const FieldDescriptor& field) {
  if (field.is_repeated()) {
    return "[]";
  }

  switch (field.cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return absl::StrCat(field.default_value_int32());
    case FieldDescriptor::CPPTYPE_UINT32:
      return absl::StrCat(field.default_value_uint32());
    case FieldDescriptor::CPPTYPE_INT64:
      return absl::StrCat(field.default_value_int64());
    case FieldDescriptor::CPPTYPE_UINT64:
      return absl::StrCat(field.default_value_uint64());
    case FieldDescriptor::CPPTYPE_DOUBLE: {
      double value = field.default_value_double();
      if (value == std::numeric_limits<double>::infinity()) {
        // Python pre-2.6 on Windows does not parse "inf" correctly.  However,
        // a numeric literal that is too big for a double will become infinity.
        return "1e10000";
      } else if (value == -std::numeric_limits<double>::infinity()) {
        // See above.
        return "-1e10000";
      } else if (value != value) {
        // infinity * 0 = nan
        return "(1e10000 * 0)";
      } else {
        return "float(" + SimpleDtoa(value) + ")";
      }
    }
    case FieldDescriptor::CPPTYPE_FLOAT: {
      float value = field.default_value_float();
      if (value == std::numeric_limits<float>::infinity()) {
        // Python pre-2.6 on Windows does not parse "inf" correctly.  However,
        // a numeric literal that is too big for a double will become infinity.
        return "1e10000";
      } else if (value == -std::numeric_limits<float>::infinity()) {
        // See above.
        return "-1e10000";
      } else if (value != value) {
        // infinity - infinity = nan
        return "(1e10000 * 0)";
      } else {
        return "float(" + SimpleFtoa(value) + ")";
      }
    }
    case FieldDescriptor::CPPTYPE_BOOL:
      return field.default_value_bool() ? "True" : "False";
    case FieldDescriptor::CPPTYPE_ENUM:
      return absl::StrCat(field.default_value_enum()->number());
    case FieldDescriptor::CPPTYPE_STRING:
      return "b\"" + absl::CEscape(field.default_value_string()) +
             (field.type() != FieldDescriptor::TYPE_STRING
                  ? "\""
                  : "\".decode('utf-8')");
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return "None";
  }
  // (We could add a default case above but then we wouldn't get the nice
  // compiler warning when a new type is added.)
  GOOGLE_LOG(FATAL) << "Not reached.";
  return "";
}

std::string StringifySyntax(FileDescriptor::Syntax syntax) {
  switch (syntax) {
    case FileDescriptor::SYNTAX_PROTO2:
      return "proto2";
    case FileDescriptor::SYNTAX_PROTO3:
      return "proto3";
    case FileDescriptor::SYNTAX_UNKNOWN:
    default:
      GOOGLE_LOG(FATAL) << "Unsupported syntax; this generator only supports proto2 "
                    "and proto3 syntax.";
      return "";
  }
}

}  // namespace

Generator::Generator() : file_(nullptr) {}

Generator::~Generator() {}

uint64_t Generator::GetSupportedFeatures() const {
  return CodeGenerator::Feature::FEATURE_PROTO3_OPTIONAL;
}

GeneratorOptions Generator::ParseParameter(const std::string& parameter,
                                           std::string* error) const {
  GeneratorOptions options;

  std::vector<std::pair<std::string, std::string> > option_pairs;
  ParseGeneratorParameter(parameter, &option_pairs);

  for (const std::pair<std::string, std::string>& option : option_pairs) {
    if (!opensource_runtime_ &&
        option.first == "no_enforce_api_compatibility") {
      // TODO(b/241584880): remove this legacy option, it has no effect.
    } else if (!opensource_runtime_ && option.first == "bootstrap") {
      options.bootstrap = true;
    } else if (option.first == "pyi_out") {
      options.generate_pyi = true;
    } else if (option.first == "annotate_code") {
      options.annotate_pyi = true;
    } else {
      *error = "Unknown generator option: " + option.first;
    }
  }
  return options;
}

bool Generator::Generate(const FileDescriptor* file,
                         const std::string& parameter,
                         GeneratorContext* context, std::string* error) const {
  // -----------------------------------------------------------------
  GeneratorOptions options = ParseParameter(parameter, error);
  if (!error->empty()) return false;

  // Generate pyi typing information
  if (options.generate_pyi) {
    python::PyiGenerator pyi_generator;
    std::string pyi_options = options.annotate_pyi ? "annotate_code" : "";
    if (!pyi_generator.Generate(file, pyi_options, context, error)) {
      return false;
    }
  }

  // Completely serialize all Generate() calls on this instance.  The
  // thread-safety constraints of the CodeGenerator interface aren't clear so
  // just be as conservative as possible.  It's easier to relax this later if
  // we need to, but I doubt it will be an issue.
  // TODO(kenton):  The proper thing to do would be to allocate any state on
  //   the stack and use that, so that the Generator class itself does not need
  //   to have any mutable members.  Then it is implicitly thread-safe.
  absl::MutexLock lock(&mutex_);
  file_ = file;

  std::string filename = GetFileName(file, ".py");

  FileDescriptorProto fdp;
  file_->CopyTo(&fdp);
  fdp.SerializeToString(&file_descriptor_serialized_);

  if (!opensource_runtime_ && GeneratingDescriptorProto()) {
    std::string bootstrap_filename =
        "net/proto2/python/internal/descriptor_pb2.py";
    if (options.bootstrap) {
      filename = bootstrap_filename;
    } else {
      std::unique_ptr<io::ZeroCopyOutputStream> output(context->Open(filename));
      io::Printer printer(output.get(), '$');
      printer.Print(
          "from $internal_package$ import descriptor_pb2\n"
          "\n",
          "internal_package", InternalPackage());

      // For static checkers, we need to explicitly assign to the symbols we
      // publicly export.
      for (int i = 0; i < file_->message_type_count(); i++) {
        const Descriptor* message = file_->message_type(i);
        printer.Print("$name$ = descriptor_pb2.$name$\n", "name",
                      message->name());
      }

      // Sadly some clients access our internal variables (starting with "_").
      // To support them, we iterate over *all* symbols to expose even the
      // private ones.  Statically type-checked code should (especially) never
      // use these, so we don't worry about making them available to pytype
      // checks.
      printer.Print(
          "\n"
          "globals().update(descriptor_pb2.__dict__)\n"
          "\n");

      printer.Print(
          "# @@protoc_insertion_point(module_scope)\n"
          "\n");
      return true;
    }
  }

  std::unique_ptr<io::ZeroCopyOutputStream> output(context->Open(filename));
  GOOGLE_CHECK(output.get());
  io::Printer printer(output.get(), '$');
  printer_ = &printer;

  PrintTopBoilerplate();
  PrintImports();
  PrintFileDescriptor();
  if (GeneratingDescriptorProto()) {
    printer_->Print("if _descriptor._USE_C_DESCRIPTORS == False:\n");
    printer_->Indent();
    // Create enums before message descriptors
    PrintAllNestedEnumsInFile();
    PrintMessageDescriptors();
    FixForeignFieldsInDescriptors();
    printer_->Outdent();
    printer_->Print("else:\n");
    printer_->Indent();
  }
  // Find the message descriptors first and then use the message
  // descriptor to find enums.
  printer_->Print(
      "_builder.BuildMessageAndEnumDescriptors(DESCRIPTOR, globals())\n");
  if (GeneratingDescriptorProto()) {
    printer_->Outdent();
  }
  std::string module_name = ModuleName(file->name());
  if (!opensource_runtime_) {
    module_name =
        std::string(absl::StripPrefix(module_name, kThirdPartyPrefix));
  }
  printer_->Print(
      "_builder.BuildTopDescriptorsAndMessages(DESCRIPTOR, '$module_name$', "
      "globals())\n",
      "module_name", module_name);
  printer.Print("if _descriptor._USE_C_DESCRIPTORS == False:\n");
  printer_->Indent();

  // We have to fix up the extensions after the message classes themselves,
  // since they need to call static RegisterExtension() methods on these
  // classes.
  FixForeignFieldsInExtensions();
  // Descriptor options may have custom extensions. These custom options
  // can only be successfully parsed after we register corresponding
  // extensions. Therefore we parse all options again here to recognize
  // custom options that may be unknown when we define the descriptors.
  // This does not apply to services because they are not used by extensions.
  FixAllDescriptorOptions();

  // Set serialized_start and serialized_end.
  SetSerializedPbInterval();

  printer_->Outdent();
  if (HasGenericServices(file)) {
    printer_->Print(
        "_builder.BuildServices(DESCRIPTOR, '$module_name$', globals())\n",
        "module_name", module_name);
  }

  printer.Print("# @@protoc_insertion_point(module_scope)\n");

  return !printer.failed();
}

// file output by this generator.
void Generator::PrintTopBoilerplate() const {
  // TODO(robinson): Allow parameterization of Python version?
  printer_->Print(
      "# -*- coding: utf-8 -*-\n"
      "# Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
      "# source: $filename$\n"
      "\"\"\"Generated protocol buffer code.\"\"\"\n",
      "filename", file_->name());
  if (!opensource_runtime_) {
    // This import is needed so that compatibility proto1 compiler output
    // inserted at protoc_insertion_point can refer to other protos like
    // google3.a.b.c. Code generated by proto2 compiler doesn't do it, and
    // instead uses aliases assigned when importing modules.
    printer_->Print("import google3\n");
  }
  printer_->Print(
      "from $internal_package$ import builder as _builder\n"
      "from $public_package$ import descriptor as _descriptor\n"
      "from $public_package$ import descriptor_pool as _descriptor_pool\n"
      "from $public_package$ import symbol_database as _symbol_database\n",
      "internal_package", InternalPackage(), "public_package", PublicPackage());

  printer_->Print("# @@protoc_insertion_point(imports)\n\n");
  printer_->Print("_sym_db = _symbol_database.Default()\n");
  printer_->Print("\n\n");
}

// Prints Python imports for all modules imported by |file|.
void Generator::PrintImports() const {
  for (int i = 0; i < file_->dependency_count(); ++i) {
    const std::string& filename = file_->dependency(i)->name();

    std::string module_name = ModuleName(filename);
    std::string module_alias = ModuleAlias(filename);
    if (!opensource_runtime_) {
      module_name =
          std::string(absl::StripPrefix(module_name, kThirdPartyPrefix));
    }
    if (ContainsPythonKeyword(module_name)) {
      // If the module path contains a Python keyword, we have to quote the
      // module name and import it using importlib. Otherwise the usual kind of
      // import statement would result in a syntax error from the presence of
      // the keyword.
      printer_->Print("import importlib\n");
      printer_->Print("$alias$ = importlib.import_module('$name$')\n", "alias",
                      module_alias, "name", module_name);
    } else {
      int last_dot_pos = module_name.rfind('.');
      std::string import_statement;
      if (last_dot_pos == std::string::npos) {
        // NOTE(petya): this is not tested as it would require a protocol buffer
        // outside of any package, and I don't think that is easily achievable.
        import_statement = "import " + module_name;
      } else {
        import_statement = "from " + module_name.substr(0, last_dot_pos) +
                           " import " + module_name.substr(last_dot_pos + 1);
      }
      printer_->Print("$statement$ as $alias$\n", "statement", import_statement,
                      "alias", module_alias);
    }

    CopyPublicDependenciesAliases(module_alias, file_->dependency(i));
  }
  printer_->Print("\n");

  // Print public imports.
  for (int i = 0; i < file_->public_dependency_count(); ++i) {
    std::string module_name = ModuleName(file_->public_dependency(i)->name());
    if (!opensource_runtime_) {
      module_name =
          std::string(absl::StripPrefix(module_name, kThirdPartyPrefix));
    }
    printer_->Print("from $module$ import *\n", "module", module_name);
  }
  printer_->Print("\n");
}

// Prints the single file descriptor for this file.
void Generator::PrintFileDescriptor() const {
  std::map<std::string, std::string> m;
  m["descriptor_name"] = kDescriptorKey;
  m["name"] = file_->name();
  m["package"] = file_->package();
  m["syntax"] = StringifySyntax(file_->syntax());
  m["options"] = OptionsValue(file_->options().SerializeAsString());
  m["serialized_descriptor"] = absl::CHexEscape(file_descriptor_serialized_);
  if (GeneratingDescriptorProto()) {
    printer_->Print("if _descriptor._USE_C_DESCRIPTORS == False:\n");
    printer_->Indent();
    // Pure python's AddSerializedFile() depend on the generated
    // descriptor_pb2.py thus we can not use AddSerializedFile() when
    // generated descriptor.proto for pure python.
    const char file_descriptor_template[] =
        "$descriptor_name$ = _descriptor.FileDescriptor(\n"
        "  name='$name$',\n"
        "  package='$package$',\n"
        "  syntax='$syntax$',\n"
        "  serialized_options=$options$,\n"
        "  create_key=_descriptor._internal_create_key,\n";
    printer_->Print(m, file_descriptor_template);
    printer_->Indent();
    printer_->Print("serialized_pb=b'$value$'\n", "value",
                    absl::CHexEscape(file_descriptor_serialized_));
    if (file_->dependency_count() != 0) {
      printer_->Print(",\ndependencies=[");
      for (int i = 0; i < file_->dependency_count(); ++i) {
        std::string module_alias = ModuleAlias(file_->dependency(i)->name());
        printer_->Print("$module_alias$.DESCRIPTOR,", "module_alias",
                        module_alias);
      }
      printer_->Print("]");
    }
    if (file_->public_dependency_count() > 0) {
      printer_->Print(",\npublic_dependencies=[");
      for (int i = 0; i < file_->public_dependency_count(); ++i) {
        std::string module_alias =
            ModuleAlias(file_->public_dependency(i)->name());
        printer_->Print("$module_alias$.DESCRIPTOR,", "module_alias",
                        module_alias);
      }
      printer_->Print("]");
    }

    // TODO(falk): Also print options and fix the message_type, enum_type,
    //             service and extension later in the generation.

    printer_->Outdent();
    printer_->Print(")\n");

    printer_->Outdent();
    printer_->Print("else:\n");
    printer_->Indent();
  }
  printer_->Print(m,
                  "$descriptor_name$ = "
                  "_descriptor_pool.Default().AddSerializedFile(b'$serialized_"
                  "descriptor$')\n");
  if (GeneratingDescriptorProto()) {
    printer_->Outdent();
  }
  printer_->Print("\n");
}

// Prints all enums contained in all message types in |file|.
void Generator::PrintAllNestedEnumsInFile() const {
  for (int i = 0; i < file_->message_type_count(); ++i) {
    PrintNestedEnums(*file_->message_type(i));
  }
}

// Prints a Python statement assigning the appropriate module-level
// enum name to a Python EnumDescriptor object equivalent to
// enum_descriptor.
void Generator::PrintEnum(const EnumDescriptor& enum_descriptor) const {
  std::map<std::string, std::string> m;
  std::string module_level_descriptor_name =
      ModuleLevelDescriptorName(enum_descriptor);
  m["descriptor_name"] = module_level_descriptor_name;
  m["name"] = enum_descriptor.name();
  m["full_name"] = enum_descriptor.full_name();
  m["file"] = kDescriptorKey;
  const char enum_descriptor_template[] =
      "$descriptor_name$ = _descriptor.EnumDescriptor(\n"
      "  name='$name$',\n"
      "  full_name='$full_name$',\n"
      "  filename=None,\n"
      "  file=$file$,\n"
      "  create_key=_descriptor._internal_create_key,\n"
      "  values=[\n";
  std::string options_string;
  enum_descriptor.options().SerializeToString(&options_string);
  printer_->Print(m, enum_descriptor_template);
  printer_->Indent();
  printer_->Indent();

  for (int i = 0; i < enum_descriptor.value_count(); ++i) {
    PrintEnumValueDescriptor(*enum_descriptor.value(i));
    printer_->Print(",\n");
  }

  printer_->Outdent();
  printer_->Print("],\n");
  printer_->Print("containing_type=None,\n");
  printer_->Print("serialized_options=$options_value$,\n", "options_value",
                  OptionsValue(options_string));
  EnumDescriptorProto edp;
  printer_->Outdent();
  printer_->Print(")\n");
  printer_->Print("_sym_db.RegisterEnumDescriptor($name$)\n", "name",
                  module_level_descriptor_name);
  printer_->Print("\n");
}

// Recursively prints enums in nested types within descriptor, then
// prints enums contained at the top level in descriptor.
void Generator::PrintNestedEnums(const Descriptor& descriptor) const {
  for (int i = 0; i < descriptor.nested_type_count(); ++i) {
    PrintNestedEnums(*descriptor.nested_type(i));
  }

  for (int i = 0; i < descriptor.enum_type_count(); ++i) {
    PrintEnum(*descriptor.enum_type(i));
  }
}

// Prints Python equivalents of all Descriptors in |file|.
void Generator::PrintMessageDescriptors() const {
  for (int i = 0; i < file_->message_type_count(); ++i) {
    PrintDescriptor(*file_->message_type(i));
    printer_->Print("\n");
  }
}

void Generator::PrintServiceDescriptors() const {
  for (int i = 0; i < file_->service_count(); ++i) {
    PrintServiceDescriptor(*file_->service(i));
  }
}

void Generator::PrintServices() const {
  for (int i = 0; i < file_->service_count(); ++i) {
    PrintServiceClass(*file_->service(i));
    PrintServiceStub(*file_->service(i));
    printer_->Print("\n");
  }
}

void Generator::PrintServiceDescriptor(
    const ServiceDescriptor& descriptor) const {
  std::map<std::string, std::string> m;
  m["service_name"] = ModuleLevelServiceDescriptorName(descriptor);
  m["name"] = descriptor.name();
  m["file"] = kDescriptorKey;
  printer_->Print(m, "$service_name$ = $file$.services_by_name['$name$']\n");
}

void Generator::PrintDescriptorKeyAndModuleName(
    const ServiceDescriptor& descriptor) const {
  std::string name = ModuleLevelServiceDescriptorName(descriptor);
  printer_->Print("$descriptor_key$ = $descriptor_name$,\n", "descriptor_key",
                  kDescriptorKey, "descriptor_name", name);
  std::string module_name = ModuleName(file_->name());
  if (!opensource_runtime_) {
    module_name =
        std::string(absl::StripPrefix(module_name, kThirdPartyPrefix));
  }
  printer_->Print("__module__ = '$module_name$'\n", "module_name", module_name);
}

void Generator::PrintServiceClass(const ServiceDescriptor& descriptor) const {
  // Print the service.
  printer_->Print(
      "$class_name$ = service_reflection.GeneratedServiceType("
      "'$class_name$', (_service.Service,), dict(\n",
      "class_name", descriptor.name());
  printer_->Indent();
  Generator::PrintDescriptorKeyAndModuleName(descriptor);
  printer_->Print("))\n\n");
  printer_->Outdent();
}

void Generator::PrintServiceStub(const ServiceDescriptor& descriptor) const {
  // Print the service stub.
  printer_->Print(
      "$class_name$_Stub = "
      "service_reflection.GeneratedServiceStubType("
      "'$class_name$_Stub', ($class_name$,), dict(\n",
      "class_name", descriptor.name());
  printer_->Indent();
  Generator::PrintDescriptorKeyAndModuleName(descriptor);
  printer_->Print("))\n\n");
  printer_->Outdent();
}

// Prints statement assigning ModuleLevelDescriptorName(message_descriptor)
// to a Python Descriptor object for message_descriptor.
//
// Mutually recursive with PrintNestedDescriptors().
void Generator::PrintDescriptor(const Descriptor& message_descriptor) const {
  std::map<std::string, std::string> m;
  m["name"] = message_descriptor.name();
  m["full_name"] = message_descriptor.full_name();
  m["file"] = kDescriptorKey;

  PrintNestedDescriptors(message_descriptor);

  printer_->Print("\n");
  printer_->Print("$descriptor_name$ = _descriptor.Descriptor(\n",
                  "descriptor_name",
                  ModuleLevelDescriptorName(message_descriptor));
  printer_->Indent();
  const char required_function_arguments[] =
      "name='$name$',\n"
      "full_name='$full_name$',\n"
      "filename=None,\n"
      "file=$file$,\n"
      "containing_type=None,\n"
      "create_key=_descriptor._internal_create_key,\n";
  printer_->Print(m, required_function_arguments);
  PrintFieldsInDescriptor(message_descriptor);
  PrintExtensionsInDescriptor(message_descriptor);

  // Nested types
  printer_->Print("nested_types=[");
  for (int i = 0; i < message_descriptor.nested_type_count(); ++i) {
    const std::string nested_name =
        ModuleLevelDescriptorName(*message_descriptor.nested_type(i));
    printer_->Print("$name$, ", "name", nested_name);
  }
  printer_->Print("],\n");

  // Enum types
  printer_->Print("enum_types=[\n");
  printer_->Indent();
  for (int i = 0; i < message_descriptor.enum_type_count(); ++i) {
    const std::string descriptor_name =
        ModuleLevelDescriptorName(*message_descriptor.enum_type(i));
    printer_->Print(descriptor_name.c_str());
    printer_->Print(",\n");
  }
  printer_->Outdent();
  printer_->Print("],\n");
  std::string options_string;
  message_descriptor.options().SerializeToString(&options_string);
  printer_->Print(
      "serialized_options=$options_value$,\n"
      "is_extendable=$extendable$,\n"
      "syntax='$syntax$'",
      "options_value", OptionsValue(options_string), "extendable",
      message_descriptor.extension_range_count() > 0 ? "True" : "False",
      "syntax", StringifySyntax(message_descriptor.file()->syntax()));
  printer_->Print(",\n");

  // Extension ranges
  printer_->Print("extension_ranges=[");
  for (int i = 0; i < message_descriptor.extension_range_count(); ++i) {
    const Descriptor::ExtensionRange* range =
        message_descriptor.extension_range(i);
    printer_->Print("($start$, $end$), ", "start", absl::StrCat(range->start),
                    "end", absl::StrCat(range->end));
  }
  printer_->Print("],\n");
  printer_->Print("oneofs=[\n");
  printer_->Indent();
  for (int i = 0; i < message_descriptor.oneof_decl_count(); ++i) {
    const OneofDescriptor* desc = message_descriptor.oneof_decl(i);
    m.clear();
    m["name"] = desc->name();
    m["full_name"] = desc->full_name();
    m["index"] = absl::StrCat(desc->index());
    options_string = OptionsValue(desc->options().SerializeAsString());
    if (options_string == "None") {
      m["serialized_options"] = "";
    } else {
      m["serialized_options"] = ", serialized_options=" + options_string;
    }
    printer_->Print(m,
                    "_descriptor.OneofDescriptor(\n"
                    "  name='$name$', full_name='$full_name$',\n"
                    "  index=$index$, containing_type=None,\n"
                    "  create_key=_descriptor._internal_create_key,\n"
                    "fields=[]$serialized_options$),\n");
  }
  printer_->Outdent();
  printer_->Print("],\n");

  printer_->Outdent();
  printer_->Print(")\n");
}

// Prints Python Descriptor objects for all nested types contained in
// message_descriptor.
//
// Mutually recursive with PrintDescriptor().
void Generator::PrintNestedDescriptors(
    const Descriptor& containing_descriptor) const {
  for (int i = 0; i < containing_descriptor.nested_type_count(); ++i) {
    PrintDescriptor(*containing_descriptor.nested_type(i));
  }
}

// Prints all messages in |file|.
void Generator::PrintMessages() const {
  for (int i = 0; i < file_->message_type_count(); ++i) {
    std::vector<std::string> to_register;
    PrintMessage(*file_->message_type(i), "", &to_register, false);
    for (int j = 0; j < to_register.size(); ++j) {
      printer_->Print("_sym_db.RegisterMessage($name$)\n", "name",
                      ResolveKeyword(to_register[j]));
    }
    printer_->Print("\n");
  }
}

// Prints a Python class for the given message descriptor.  We defer to the
// metaclass to do almost all of the work of actually creating a useful class.
// The purpose of this function and its many helper functions above is merely
// to output a Python version of the descriptors, which the metaclass in
// reflection.py will use to construct the meat of the class itself.
//
// Mutually recursive with PrintNestedMessages().
// Collect nested message names to_register for the symbol_database.
void Generator::PrintMessage(const Descriptor& message_descriptor,
                             const std::string& prefix,
                             std::vector<std::string>* to_register,
                             bool is_nested) const {
  std::string qualified_name;
  if (is_nested) {
    if (IsPythonKeyword(message_descriptor.name())) {
      qualified_name =
          "getattr(" + prefix + ", '" + message_descriptor.name() + "')";
    } else {
      qualified_name = prefix + "." + message_descriptor.name();
    }
    printer_->Print(
        "'$name$' : _reflection.GeneratedProtocolMessageType('$name$', "
        "(_message.Message,), {\n",
        "name", message_descriptor.name());
  } else {
    qualified_name = ResolveKeyword(message_descriptor.name());
    printer_->Print(
        "$qualified_name$ = _reflection.GeneratedProtocolMessageType('$name$', "
        "(_message.Message,), {\n",
        "qualified_name", qualified_name, "name", message_descriptor.name());
  }
  printer_->Indent();

  to_register->push_back(qualified_name);

  PrintNestedMessages(message_descriptor, qualified_name, to_register);
  std::map<std::string, std::string> m;
  m["descriptor_key"] = kDescriptorKey;
  m["descriptor_name"] = ModuleLevelDescriptorName(message_descriptor);
  printer_->Print(m, "'$descriptor_key$' : $descriptor_name$,\n");
  std::string module_name = ModuleName(file_->name());
  if (!opensource_runtime_) {
    module_name =
        std::string(absl::StripPrefix(module_name, kThirdPartyPrefix));
  }
  printer_->Print("'__module__' : '$module_name$'\n", "module_name",
                  module_name);
  printer_->Print("# @@protoc_insertion_point(class_scope:$full_name$)\n",
                  "full_name", message_descriptor.full_name());
  printer_->Print("})\n");
  printer_->Outdent();
}

// Prints all nested messages within |containing_descriptor|.
// Mutually recursive with PrintMessage().
void Generator::PrintNestedMessages(
    const Descriptor& containing_descriptor, const std::string& prefix,
    std::vector<std::string>* to_register) const {
  for (int i = 0; i < containing_descriptor.nested_type_count(); ++i) {
    printer_->Print("\n");
    PrintMessage(*containing_descriptor.nested_type(i), prefix, to_register,
                 true);
    printer_->Print(",\n");
  }
}

// Recursively fixes foreign fields in all nested types in |descriptor|, then
// sets the message_type and enum_type of all message and enum fields to point
// to their respective descriptors.
// Args:
//   descriptor: descriptor to print fields for.
//   containing_descriptor: if descriptor is a nested type, this is its
//       containing type, or NULL if this is a root/top-level type.
void Generator::FixForeignFieldsInDescriptor(
    const Descriptor& descriptor,
    const Descriptor* containing_descriptor) const {
  for (int i = 0; i < descriptor.nested_type_count(); ++i) {
    FixForeignFieldsInDescriptor(*descriptor.nested_type(i), &descriptor);
  }

  for (int i = 0; i < descriptor.field_count(); ++i) {
    const FieldDescriptor& field_descriptor = *descriptor.field(i);
    FixForeignFieldsInField(&descriptor, field_descriptor, "fields_by_name");
  }

  FixContainingTypeInDescriptor(descriptor, containing_descriptor);
  for (int i = 0; i < descriptor.enum_type_count(); ++i) {
    const EnumDescriptor& enum_descriptor = *descriptor.enum_type(i);
    FixContainingTypeInDescriptor(enum_descriptor, &descriptor);
  }
  for (int i = 0; i < descriptor.oneof_decl_count(); ++i) {
    std::map<std::string, std::string> m;
    const OneofDescriptor* oneof = descriptor.oneof_decl(i);
    m["descriptor_name"] = ModuleLevelDescriptorName(descriptor);
    m["oneof_name"] = oneof->name();
    for (int j = 0; j < oneof->field_count(); ++j) {
      m["field_name"] = oneof->field(j)->name();
      printer_->Print(
          m,
          "$descriptor_name$.oneofs_by_name['$oneof_name$'].fields.append(\n"
          "  $descriptor_name$.fields_by_name['$field_name$'])\n");
      printer_->Print(
          m,
          "$descriptor_name$.fields_by_name['$field_name$'].containing_oneof = "
          "$descriptor_name$.oneofs_by_name['$oneof_name$']\n");
    }
  }
}

void Generator::AddMessageToFileDescriptor(const Descriptor& descriptor) const {
  std::map<std::string, std::string> m;
  m["descriptor_name"] = kDescriptorKey;
  m["message_name"] = descriptor.name();
  m["message_descriptor_name"] = ModuleLevelDescriptorName(descriptor);
  const char file_descriptor_template[] =
      "$descriptor_name$.message_types_by_name['$message_name$'] = "
      "$message_descriptor_name$\n";
  printer_->Print(m, file_descriptor_template);
}

void Generator::AddServiceToFileDescriptor(
    const ServiceDescriptor& descriptor) const {
  std::map<std::string, std::string> m;
  m["descriptor_name"] = kDescriptorKey;
  m["service_name"] = descriptor.name();
  m["service_descriptor_name"] = ModuleLevelServiceDescriptorName(descriptor);
  const char file_descriptor_template[] =
      "$descriptor_name$.services_by_name['$service_name$'] = "
      "$service_descriptor_name$\n";
  printer_->Print(m, file_descriptor_template);
}

void Generator::AddEnumToFileDescriptor(
    const EnumDescriptor& descriptor) const {
  std::map<std::string, std::string> m;
  m["descriptor_name"] = kDescriptorKey;
  m["enum_name"] = descriptor.name();
  m["enum_descriptor_name"] = ModuleLevelDescriptorName(descriptor);
  const char file_descriptor_template[] =
      "$descriptor_name$.enum_types_by_name['$enum_name$'] = "
      "$enum_descriptor_name$\n";
  printer_->Print(m, file_descriptor_template);
}

void Generator::AddExtensionToFileDescriptor(
    const FieldDescriptor& descriptor) const {
  std::map<std::string, std::string> m;
  m["descriptor_name"] = kDescriptorKey;
  m["field_name"] = descriptor.name();
  m["resolved_name"] = ResolveKeyword(descriptor.name());
  const char file_descriptor_template[] =
      "$descriptor_name$.extensions_by_name['$field_name$'] = "
      "$resolved_name$\n";
  printer_->Print(m, file_descriptor_template);
}

// Sets any necessary message_type and enum_type attributes
// for the Python version of |field|.
//
// containing_type may be NULL, in which case this is a module-level field.
//
// python_dict_name is the name of the Python dict where we should
// look the field up in the containing type.  (e.g., fields_by_name
// or extensions_by_name).  We ignore python_dict_name if containing_type
// is NULL.
void Generator::FixForeignFieldsInField(
    const Descriptor* containing_type, const FieldDescriptor& field,
    const std::string& python_dict_name) const {
  const std::string field_referencing_expression =
      FieldReferencingExpression(containing_type, field, python_dict_name);
  std::map<std::string, std::string> m;
  m["field_ref"] = field_referencing_expression;
  const Descriptor* foreign_message_type = field.message_type();
  if (foreign_message_type) {
    m["foreign_type"] = ModuleLevelDescriptorName(*foreign_message_type);
    printer_->Print(m, "$field_ref$.message_type = $foreign_type$\n");
  }
  const EnumDescriptor* enum_type = field.enum_type();
  if (enum_type) {
    m["enum_type"] = ModuleLevelDescriptorName(*enum_type);
    printer_->Print(m, "$field_ref$.enum_type = $enum_type$\n");
  }
}

// Returns the module-level expression for the given FieldDescriptor.
// Only works for fields in the .proto file this Generator is generating for.
//
// containing_type may be NULL, in which case this is a module-level field.
//
// python_dict_name is the name of the Python dict where we should
// look the field up in the containing type.  (e.g., fields_by_name
// or extensions_by_name).  We ignore python_dict_name if containing_type
// is NULL.
std::string Generator::FieldReferencingExpression(
    const Descriptor* containing_type, const FieldDescriptor& field,
    const std::string& python_dict_name) const {
  // We should only ever be looking up fields in the current file.
  // The only things we refer to from other files are message descriptors.
  GOOGLE_CHECK_EQ(field.file(), file_)
      << field.file()->name() << " vs. " << file_->name();
  if (!containing_type) {
    return ResolveKeyword(field.name());
  }
  return absl::Substitute("$0.$1['$2']",
                          ModuleLevelDescriptorName(*containing_type),
                          python_dict_name, field.name());
}

// Prints containing_type for nested descriptors or enum descriptors.
template <typename DescriptorT>
void Generator::FixContainingTypeInDescriptor(
    const DescriptorT& descriptor,
    const Descriptor* containing_descriptor) const {
  if (containing_descriptor != nullptr) {
    const std::string nested_name = ModuleLevelDescriptorName(descriptor);
    const std::string parent_name =
        ModuleLevelDescriptorName(*containing_descriptor);
    printer_->Print("$nested_name$.containing_type = $parent_name$\n",
                    "nested_name", nested_name, "parent_name", parent_name);
  }
}

// Prints statements setting the message_type and enum_type fields in the
// Python descriptor objects we've already output in the file.  We must
// do this in a separate step due to circular references (otherwise, we'd
// just set everything in the initial assignment statements).
void Generator::FixForeignFieldsInDescriptors() const {
  for (int i = 0; i < file_->message_type_count(); ++i) {
    FixForeignFieldsInDescriptor(*file_->message_type(i), nullptr);
  }
  for (int i = 0; i < file_->message_type_count(); ++i) {
    AddMessageToFileDescriptor(*file_->message_type(i));
  }
  for (int i = 0; i < file_->enum_type_count(); ++i) {
    AddEnumToFileDescriptor(*file_->enum_type(i));
  }
  for (int i = 0; i < file_->extension_count(); ++i) {
    AddExtensionToFileDescriptor(*file_->extension(i));
  }

  // TODO(jieluo): Move this register to PrintFileDescriptor() when
  // FieldDescriptor.file is added in generated file.
  printer_->Print("_sym_db.RegisterFileDescriptor($name$)\n", "name",
                  kDescriptorKey);
  printer_->Print("\n");
}

// We need to not only set any necessary message_type fields, but
// also need to call RegisterExtension() on each message we're
// extending.
void Generator::FixForeignFieldsInExtensions() const {
  // Top-level extensions.
  for (int i = 0; i < file_->extension_count(); ++i) {
    FixForeignFieldsInExtension(*file_->extension(i));
  }
  // Nested extensions.
  for (int i = 0; i < file_->message_type_count(); ++i) {
    FixForeignFieldsInNestedExtensions(*file_->message_type(i));
  }
  printer_->Print("\n");
}

void Generator::FixForeignFieldsInExtension(
    const FieldDescriptor& extension_field) const {
  GOOGLE_CHECK(extension_field.is_extension());

  std::map<std::string, std::string> m;
  // Confusingly, for FieldDescriptors that happen to be extensions,
  // containing_type() means "extended type."
  // On the other hand, extension_scope() will give us what we normally
  // mean by containing_type().
  m["extended_message_class"] =
      ModuleLevelMessageName(*extension_field.containing_type());
  m["field"] = FieldReferencingExpression(
      extension_field.extension_scope(), extension_field, "extensions_by_name");
  printer_->Print(m, "$extended_message_class$.RegisterExtension($field$)\n");
}

void Generator::FixForeignFieldsInNestedExtensions(
    const Descriptor& descriptor) const {
  // Recursively fix up extensions in all nested types.
  for (int i = 0; i < descriptor.nested_type_count(); ++i) {
    FixForeignFieldsInNestedExtensions(*descriptor.nested_type(i));
  }
  // Fix up extensions directly contained within this type.
  for (int i = 0; i < descriptor.extension_count(); ++i) {
    FixForeignFieldsInExtension(*descriptor.extension(i));
  }
}

// Returns a Python expression that instantiates a Python EnumValueDescriptor
// object for the given C++ descriptor.
void Generator::PrintEnumValueDescriptor(
    const EnumValueDescriptor& descriptor) const {
  // TODO(robinson): Fix up EnumValueDescriptor "type" fields.
  // More circular references.  ::sigh::
  std::string options_string;
  descriptor.options().SerializeToString(&options_string);
  std::map<std::string, std::string> m;
  m["name"] = descriptor.name();
  m["index"] = absl::StrCat(descriptor.index());
  m["number"] = absl::StrCat(descriptor.number());
  m["options"] = OptionsValue(options_string);
  printer_->Print(m,
                  "_descriptor.EnumValueDescriptor(\n"
                  "  name='$name$', index=$index$, number=$number$,\n"
                  "  serialized_options=$options$,\n"
                  "  type=None,\n"
                  "  create_key=_descriptor._internal_create_key)");
}

// Returns a CEscaped string of serialized_options.
std::string Generator::OptionsValue(
    const std::string& serialized_options) const {
  if (serialized_options.length() == 0 || GeneratingDescriptorProto()) {
    return "None";
  } else {
    return "b'" + absl::CEscape(serialized_options) + "'";
  }
}

// Prints an expression for a Python FieldDescriptor for |field|.
void Generator::PrintFieldDescriptor(const FieldDescriptor& field,
                                     bool is_extension) const {
  std::string options_string;
  field.options().SerializeToString(&options_string);
  std::map<std::string, std::string> m;
  m["name"] = field.name();
  m["full_name"] = field.full_name();
  m["index"] = absl::StrCat(field.index());
  m["number"] = absl::StrCat(field.number());
  m["type"] = absl::StrCat(field.type());
  m["cpp_type"] = absl::StrCat(field.cpp_type());
  m["label"] = absl::StrCat(field.label());
  m["has_default_value"] = field.has_default_value() ? "True" : "False";
  m["default_value"] = StringifyDefaultValue(field);
  m["is_extension"] = is_extension ? "True" : "False";
  m["serialized_options"] = OptionsValue(options_string);
  m["json_name"] =
      field.has_json_name() ? ", json_name='" + field.json_name() + "'" : "";
  // We always set message_type and enum_type to None at this point, and then
  // these fields in correctly after all referenced descriptors have been
  // defined and/or imported (see FixForeignFieldsInDescriptors()).
  const char field_descriptor_decl[] =
      "_descriptor.FieldDescriptor(\n"
      "  name='$name$', full_name='$full_name$', index=$index$,\n"
      "  number=$number$, type=$type$, cpp_type=$cpp_type$, label=$label$,\n"
      "  has_default_value=$has_default_value$, "
      "default_value=$default_value$,\n"
      "  message_type=None, enum_type=None, containing_type=None,\n"
      "  is_extension=$is_extension$, extension_scope=None,\n"
      "  serialized_options=$serialized_options$$json_name$, file=DESCRIPTOR,"
      "  create_key=_descriptor._internal_create_key)";
  printer_->Print(m, field_descriptor_decl);
}

// Helper for Print{Fields,Extensions}InDescriptor().
void Generator::PrintFieldDescriptorsInDescriptor(
    const Descriptor& message_descriptor, bool is_extension,
    const std::string& list_variable_name, int (Descriptor::*CountFn)() const,
    const FieldDescriptor* (Descriptor::*GetterFn)(int)const) const {
  printer_->Print("$list$=[\n", "list", list_variable_name);
  printer_->Indent();
  for (int i = 0; i < (message_descriptor.*CountFn)(); ++i) {
    PrintFieldDescriptor(*(message_descriptor.*GetterFn)(i), is_extension);
    printer_->Print(",\n");
  }
  printer_->Outdent();
  printer_->Print("],\n");
}

// Prints a statement assigning "fields" to a list of Python FieldDescriptors,
// one for each field present in message_descriptor.
void Generator::PrintFieldsInDescriptor(
    const Descriptor& message_descriptor) const {
  const bool is_extension = false;
  PrintFieldDescriptorsInDescriptor(message_descriptor, is_extension, "fields",
                                    &Descriptor::field_count,
                                    &Descriptor::field);
}

// Prints a statement assigning "extensions" to a list of Python
// FieldDescriptors, one for each extension present in message_descriptor.
void Generator::PrintExtensionsInDescriptor(
    const Descriptor& message_descriptor) const {
  const bool is_extension = true;
  PrintFieldDescriptorsInDescriptor(message_descriptor, is_extension,
                                    "extensions", &Descriptor::extension_count,
                                    &Descriptor::extension);
}

bool Generator::GeneratingDescriptorProto() const {
  return file_->name() == "net/proto2/proto/descriptor.proto" ||
         file_->name() == "google/protobuf/descriptor.proto";
}

// Returns the unique Python module-level identifier given to a descriptor.
// This name is module-qualified iff the given descriptor describes an
// entity that doesn't come from the current file.
template <typename DescriptorT>
std::string Generator::ModuleLevelDescriptorName(
    const DescriptorT& descriptor) const {
  // FIXME(robinson):
  // We currently don't worry about collisions with underscores in the type
  // names, so these would collide in nasty ways if found in the same file:
  //   OuterProto.ProtoA.ProtoB
  //   OuterProto_ProtoA.ProtoB  # Underscore instead of period.
  // As would these:
  //   OuterProto.ProtoA_.ProtoB
  //   OuterProto.ProtoA._ProtoB  # Leading vs. trailing underscore.
  // (Contrived, but certainly possible).
  //
  // The C++ implementation doesn't guard against this either.  Leaving
  // it for now...
  std::string name = NamePrefixedWithNestedTypes(descriptor, "_");
  absl::AsciiStrToUpper(&name);
  // Module-private for now.  Easy to make public later; almost impossible
  // to make private later.
  name = "_" + name;
  // We now have the name relative to its own module.  Also qualify with
  // the module name iff this descriptor is from a different .proto file.
  if (descriptor.file() != file_) {
    name = ModuleAlias(descriptor.file()->name()) + "." + name;
  }
  return name;
}

// Returns the name of the message class itself, not the descriptor.
// Like ModuleLevelDescriptorName(), module-qualifies the name iff
// the given descriptor describes an entity that doesn't come from
// the current file.
std::string Generator::ModuleLevelMessageName(
    const Descriptor& descriptor) const {
  std::string name = NamePrefixedWithNestedTypes(descriptor, ".");
  if (descriptor.file() != file_) {
    name = ModuleAlias(descriptor.file()->name()) + "." + name;
  }
  return name;
}

// Returns the unique Python module-level identifier given to a service
// descriptor.
std::string Generator::ModuleLevelServiceDescriptorName(
    const ServiceDescriptor& descriptor) const {
  std::string name = descriptor.name();
  absl::AsciiStrToUpper(&name);
  name = "_" + name;
  if (descriptor.file() != file_) {
    name = ModuleAlias(descriptor.file()->name()) + "." + name;
  }
  return name;
}

std::string Generator::PublicPackage() const {
  return opensource_runtime_ ? "google.protobuf"
                             : "google3.net.google.protobuf.python.public";
}

std::string Generator::InternalPackage() const {
  return opensource_runtime_ ? "google.protobuf.internal"
                             : "google3.net.google.protobuf.python.internal";
}

// Prints standard constructor arguments serialized_start and serialized_end.
// Args:
//   descriptor: The cpp descriptor to have a serialized reference.
//   proto: A proto
// Example printer output:
// serialized_start=41,
// serialized_end=43,
//
template <typename DescriptorT, typename DescriptorProtoT>
void Generator::PrintSerializedPbInterval(const DescriptorT& descriptor,
                                          DescriptorProtoT& proto,
                                          const std::string& name) const {
  descriptor.CopyTo(&proto);
  std::string sp;
  proto.SerializeToString(&sp);
  int offset = file_descriptor_serialized_.find(sp);
  GOOGLE_CHECK_GE(offset, 0);

  printer_->Print(
      "$name$._serialized_start=$serialized_start$\n"
      "$name$._serialized_end=$serialized_end$\n",
      "name", name, "serialized_start", absl::StrCat(offset), "serialized_end",
      absl::StrCat(offset + sp.size()));
}

namespace {
void PrintDescriptorOptionsFixingCode(const std::string& descriptor,
                                      const std::string& options,
                                      io::Printer* printer) {
  // Reset the _options to None thus DescriptorBase.GetOptions() can
  // parse _options again after extensions are registered.
  printer->Print(
      "$descriptor$._options = None\n"
      "$descriptor$._serialized_options = $serialized_value$\n",
      "descriptor", descriptor, "serialized_value", options);
}
}  // namespace

void Generator::SetSerializedPbInterval() const {
  // Top level enums.
  for (int i = 0; i < file_->enum_type_count(); ++i) {
    EnumDescriptorProto proto;
    const EnumDescriptor& descriptor = *file_->enum_type(i);
    PrintSerializedPbInterval(descriptor, proto,
                              ModuleLevelDescriptorName(descriptor));
  }

  // Messages.
  for (int i = 0; i < file_->message_type_count(); ++i) {
    SetMessagePbInterval(*file_->message_type(i));
  }

  // Services.
  for (int i = 0; i < file_->service_count(); ++i) {
    ServiceDescriptorProto proto;
    const ServiceDescriptor& service = *file_->service(i);
    PrintSerializedPbInterval(service, proto,
                              ModuleLevelServiceDescriptorName(service));
  }
}

void Generator::SetMessagePbInterval(const Descriptor& descriptor) const {
  DescriptorProto message_proto;
  PrintSerializedPbInterval(descriptor, message_proto,
                            ModuleLevelDescriptorName(descriptor));

  // Nested messages.
  for (int i = 0; i < descriptor.nested_type_count(); ++i) {
    SetMessagePbInterval(*descriptor.nested_type(i));
  }

  for (int i = 0; i < descriptor.enum_type_count(); ++i) {
    EnumDescriptorProto proto;
    const EnumDescriptor& enum_des = *descriptor.enum_type(i);
    PrintSerializedPbInterval(enum_des, proto,
                              ModuleLevelDescriptorName(enum_des));
  }
}

// Prints expressions that set the options field of all descriptors.
void Generator::FixAllDescriptorOptions() const {
  // Prints an expression that sets the file descriptor's options.
  std::string file_options = OptionsValue(file_->options().SerializeAsString());
  if (file_options != "None") {
    PrintDescriptorOptionsFixingCode(kDescriptorKey, file_options, printer_);
  } else {
    printer_->Print("DESCRIPTOR._options = None\n");
  }
  // Prints expressions that set the options for all top level enums.
  for (int i = 0; i < file_->enum_type_count(); ++i) {
    const EnumDescriptor& enum_descriptor = *file_->enum_type(i);
    FixOptionsForEnum(enum_descriptor);
  }
  // Prints expressions that set the options for all top level extensions.
  for (int i = 0; i < file_->extension_count(); ++i) {
    const FieldDescriptor& field = *file_->extension(i);
    FixOptionsForField(field);
  }
  // Prints expressions that set the options for all messages, nested enums,
  // nested extensions and message fields.
  for (int i = 0; i < file_->message_type_count(); ++i) {
    FixOptionsForMessage(*file_->message_type(i));
  }

  for (int i = 0; i < file_->service_count(); ++i) {
    FixOptionsForService(*file_->service(i));
  }
}

void Generator::FixOptionsForOneof(const OneofDescriptor& oneof) const {
  std::string oneof_options = OptionsValue(oneof.options().SerializeAsString());
  if (oneof_options != "None") {
    std::string oneof_name = absl::Substitute(
        "$0.$1['$2']", ModuleLevelDescriptorName(*oneof.containing_type()),
        "oneofs_by_name", oneof.name());
    PrintDescriptorOptionsFixingCode(oneof_name, oneof_options, printer_);
  }
}

// Prints expressions that set the options for an enum descriptor and its
// value descriptors.
void Generator::FixOptionsForEnum(const EnumDescriptor& enum_descriptor) const {
  std::string descriptor_name = ModuleLevelDescriptorName(enum_descriptor);
  std::string enum_options =
      OptionsValue(enum_descriptor.options().SerializeAsString());
  if (enum_options != "None") {
    PrintDescriptorOptionsFixingCode(descriptor_name, enum_options, printer_);
  }
  for (int i = 0; i < enum_descriptor.value_count(); ++i) {
    const EnumValueDescriptor& value_descriptor = *enum_descriptor.value(i);
    std::string value_options =
        OptionsValue(value_descriptor.options().SerializeAsString());
    if (value_options != "None") {
      PrintDescriptorOptionsFixingCode(
          absl::StrFormat("%s.values_by_name[\"%s\"]", descriptor_name.c_str(),
                          value_descriptor.name().c_str()),
          value_options, printer_);
    }
  }
}

// Prints expressions that set the options for an service descriptor and its
// value descriptors.
void Generator::FixOptionsForService(
    const ServiceDescriptor& service_descriptor) const {
  std::string descriptor_name =
      ModuleLevelServiceDescriptorName(service_descriptor);
  std::string service_options =
      OptionsValue(service_descriptor.options().SerializeAsString());
  if (service_options != "None") {
    PrintDescriptorOptionsFixingCode(descriptor_name, service_options,
                                     printer_);
  }

  for (int i = 0; i < service_descriptor.method_count(); ++i) {
    const MethodDescriptor* method = service_descriptor.method(i);
    std::string method_options =
        OptionsValue(method->options().SerializeAsString());
    if (method_options != "None") {
      std::string method_name =
          descriptor_name + ".methods_by_name['" + method->name() + "']";
      PrintDescriptorOptionsFixingCode(method_name, method_options, printer_);
    }
  }
}

// Prints expressions that set the options for field descriptors (including
// extensions).
void Generator::FixOptionsForField(const FieldDescriptor& field) const {
  std::string field_options = OptionsValue(field.options().SerializeAsString());
  if (field_options != "None") {
    std::string field_name;
    if (field.is_extension()) {
      if (field.extension_scope() == nullptr) {
        // Top level extensions.
        field_name = field.name();
      } else {
        field_name = FieldReferencingExpression(field.extension_scope(), field,
                                                "extensions_by_name");
      }
    } else {
      field_name = FieldReferencingExpression(field.containing_type(), field,
                                              "fields_by_name");
    }
    PrintDescriptorOptionsFixingCode(field_name, field_options, printer_);
  }
}

// Prints expressions that set the options for a message and all its inner
// types (nested messages, nested enums, extensions, fields).
void Generator::FixOptionsForMessage(const Descriptor& descriptor) const {
  // Nested messages.
  for (int i = 0; i < descriptor.nested_type_count(); ++i) {
    FixOptionsForMessage(*descriptor.nested_type(i));
  }
  // Oneofs.
  for (int i = 0; i < descriptor.oneof_decl_count(); ++i) {
    FixOptionsForOneof(*descriptor.oneof_decl(i));
  }
  // Enums.
  for (int i = 0; i < descriptor.enum_type_count(); ++i) {
    FixOptionsForEnum(*descriptor.enum_type(i));
  }
  // Fields.
  for (int i = 0; i < descriptor.field_count(); ++i) {
    const FieldDescriptor& field = *descriptor.field(i);
    FixOptionsForField(field);
  }
  // Extensions.
  for (int i = 0; i < descriptor.extension_count(); ++i) {
    const FieldDescriptor& field = *descriptor.extension(i);
    FixOptionsForField(field);
  }
  // Message option for this message.
  std::string message_options =
      OptionsValue(descriptor.options().SerializeAsString());
  if (message_options != "None") {
    std::string descriptor_name = ModuleLevelDescriptorName(descriptor);
    PrintDescriptorOptionsFixingCode(descriptor_name, message_options,
                                     printer_);
  }
}

// If a dependency forwards other files through public dependencies, let's
// copy over the corresponding module aliases.
void Generator::CopyPublicDependenciesAliases(
    const std::string& copy_from, const FileDescriptor* file) const {
  for (int i = 0; i < file->public_dependency_count(); ++i) {
    std::string module_name = ModuleName(file->public_dependency(i)->name());
    std::string module_alias = ModuleAlias(file->public_dependency(i)->name());
    // There's no module alias in the dependent file if it was generated by
    // an old protoc (less than 3.0.0-alpha-1). Use module name in this
    // situation.
    printer_->Print(
        "try:\n"
        "  $alias$ = $copy_from$.$alias$\n"
        "except AttributeError:\n"
        "  $alias$ = $copy_from$.$module$\n",
        "alias", module_alias, "module", module_name, "copy_from", copy_from);
    CopyPublicDependenciesAliases(copy_from, file->public_dependency(i));
  }
}

}  // namespace python
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
