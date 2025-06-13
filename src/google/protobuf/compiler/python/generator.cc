// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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

#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/memory/memory.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/python/helpers.h"
#include "google/protobuf/compiler/python/pyi_generator.h"
#include "google/protobuf/compiler/retention.h"
#include "google/protobuf/compiler/versions.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor_visitor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/strtod.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/message.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace python {

namespace {
// Returns the alias we assign to the module of the given .proto filename
// when importing. See testPackageInitializationImport in
// third_party/py/google/protobuf/internal/reflection_test.py
// to see why we need the alias.
std::string ModuleAlias(absl::string_view filename) {
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
// TODO: Unify with code from
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
        return absl::StrCat("float(", io::SimpleDtoa(value), ")");
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
        return absl::StrCat("float(", io::SimpleFtoa(value), ")");
      }
    }
    case FieldDescriptor::CPPTYPE_BOOL:
      return field.default_value_bool() ? "True" : "False";
    case FieldDescriptor::CPPTYPE_ENUM:
      return absl::StrCat(field.default_value_enum()->number());
    case FieldDescriptor::CPPTYPE_STRING:
      return absl::StrCat("b\"", absl::CEscape(field.default_value_string()),
                          (field.type() != FieldDescriptor::TYPE_STRING
                               ? "\""
                               : "\".decode('utf-8')"));
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return "None";
  }
  // (We could add a default case above but then we wouldn't get the nice
  // compiler warning when a new type is added.)
  ABSL_LOG(FATAL) << "Not reached.";
  return "";
}

// Returns a CEscaped string of serialized_options.
std::string OptionsValue(absl::string_view serialized_options) {
  if (serialized_options.empty()) {
    return "None";
  } else {
    return absl::StrCat("b'", absl::CEscape(serialized_options), "'");
  }
}

std::string GetLegacySyntaxName(Edition edition) {
  switch (edition) {
    case Edition::EDITION_PROTO2:
      return "proto2";
    case Edition::EDITION_PROTO3:
      return "proto3";
    default:
      return "editions";
  }
}

}  // namespace

Generator::Generator() : file_(nullptr) {}

Generator::~Generator() {}

GeneratorOptions Generator::ParseParameter(absl::string_view parameter,
                                           std::string* error) const {
  GeneratorOptions options;

  std::vector<std::pair<std::string, std::string> > option_pairs;
  ParseGeneratorParameter(parameter, &option_pairs);

  for (const std::pair<std::string, std::string>& option : option_pairs) {
    if (!opensource_runtime_ && option.first == "bootstrap") {
      options.bootstrap = true;
    } else if (option.first == "pyi_out") {
      options.generate_pyi = true;
    } else if (option.first == "annotate_code") {
      options.annotate_pyi = true;
    } else if (option.first == "experimental_strip_nonfunctional_codegen") {
      options.strip_nonfunctional_codegen = true;
    } else {
      *error = absl::StrCat("Unknown generator option: ", option.first);
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
    std::vector<std::string> pyi_options;
    if (options.annotate_pyi) {
      pyi_options.push_back("annotate_code");
    }
    if (options.strip_nonfunctional_codegen) {
      pyi_options.push_back("experimental_strip_nonfunctional_codegen");
    }
    if (!pyi_generator.Generate(file, absl::StrJoin(pyi_options, ","), context,
                                error)) {
      return false;
    }
  }

  // Completely serialize all Generate() calls on this instance.  The
  // thread-safety constraints of the CodeGenerator interface aren't clear so
  // just be as conservative as possible.  It's easier to relax this later if
  // we need to, but I doubt it will be an issue.
  // TODO:  The proper thing to do would be to allocate any state on
  //   the stack and use that, so that the Generator class itself does not need
  //   to have any mutable members.  Then it is implicitly thread-safe.
  absl::MutexLock lock(&mutex_);
  file_ = file;

  std::string filename = GetFileName(file, ".py");

  proto_ = StripSourceRetentionOptions(*file_);
  proto_.SerializeToString(&file_descriptor_serialized_);

  if (!opensource_runtime_ && GeneratingDescriptorProto()) {
    std::string bootstrap_filename =
        "net/proto2/python/internal/descriptor_pb2.py";
    if (options.bootstrap) {
      filename = bootstrap_filename;
    } else {
      std::unique_ptr<io::ZeroCopyOutputStream> output(context->Open(filename));
      io::Printer printer(output.get(), '$');
      printer.Print(
          "from google3.net.google.protobuf.python.internal import "
          "descriptor_pb2\n"
          "\n");

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
  ABSL_CHECK(output.get());
  io::Printer printer(output.get(), '$');
  printer_ = &printer;

  PrintTopBoilerplate();
  PrintImports();
  PrintFileDescriptor();
  printer_->Print("_globals = globals()\n");
  if (GeneratingDescriptorProto()) {
    printer_->Print("if not _descriptor._USE_C_DESCRIPTORS:\n");
    printer_->Indent();
    // Create enums before message descriptors
    PrintAllEnumsInFile();
    PrintMessageDescriptors();
    FixForeignFieldsInDescriptors();
    PrintResolvedFeatures();
    printer_->Outdent();
    printer_->Print("else:\n");
    printer_->Indent();
  }
  // Find the message descriptors first and then use the message
  // descriptor to find enums.
  printer_->Print(
      "_builder.BuildMessageAndEnumDescriptors(DESCRIPTOR, _globals)\n");
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
      "_globals)\n",
      "module_name", module_name);
  printer.Print("if not _descriptor._USE_C_DESCRIPTORS:\n");
  printer_->Indent();

  // Descriptor options may have custom extensions. These custom options
  // can only be successfully parsed after we register corresponding
  // extensions. Therefore we parse all options again here to recognize
  // custom options that may be unknown when we define the descriptors.
  // This does not apply to services because they are not used by extensions.
  FixAllDescriptorOptions();

  // Set serialized_start and serialized_end.
  SetSerializedPbInterval(proto_);

  printer_->Outdent();
  if (HasGenericServices(file)) {
    printer_->Print(
        "_builder.BuildServices(DESCRIPTOR, '$module_name$', _globals)\n",
        "module_name", module_name);
  }

  printer.Print("# @@protoc_insertion_point(module_scope)\n");

  return !printer.failed();
}

// file output by this generator.
void Generator::PrintTopBoilerplate() const {
  // TODO: Allow parameterization of Python version?
  printer_->Print(
      "# -*- coding: utf-8 -*-\n"
      "# Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
      "# NO CHECKED-IN PROTOBUF "
      // Intentional line breaker
      "GENCODE\n"
      "# source: $filename$\n",
      "filename", file_->name());
  if (opensource_runtime_) {
    printer_->Print("# Protobuf Python Version: $protobuf_python_version$\n",
                    "protobuf_python_version", PROTOBUF_PYTHON_VERSION_STRING);
  }
  printer_->Print("\"\"\"Generated protocol buffer code.\"\"\"\n");
  bool runtime_version_disabled = false;
  printer_->Print(
      "from google.protobuf import descriptor as _descriptor\n"
      "from google.protobuf import descriptor_pool as _descriptor_pool\n"
      "$runtime_version_import$"
      "from google.protobuf import symbol_database as _symbol_database\n"
      "from google.protobuf.internal import builder as _builder\n",
      "runtime_version_import",
      runtime_version_disabled ? ""
                               : "from google.protobuf import runtime_version "
                                 "as _runtime_version\n");
  if (!runtime_version_disabled) {
    const auto& version = GetProtobufPythonVersion(opensource_runtime_);
    printer_->Print(
        "_runtime_version.ValidateProtobufRuntimeVersion(\n"
        "    $domain$,\n"
        "    $major$,\n"
        "    $minor$,\n"
        "    $patch$,\n"
        "    '$suffix$',\n"
        "    '$location$'\n"
        ")\n",
        "domain",
        opensource_runtime_ ? "_runtime_version.Domain.PUBLIC"
                            : "_runtime_version.Domain.GOOGLE_INTERNAL",
        "major", absl::StrCat(version.major()), "minor",
        absl::StrCat(version.minor()), "patch", absl::StrCat(version.patch()),
        "suffix", version.suffix(), "location", file_->name());
  }
  printer_->Print("# @@protoc_insertion_point(imports)\n\n");
  printer_->Print("_sym_db = _symbol_database.Default()\n");
  printer_->Print("\n\n");
}

// Prints Python imports for all modules imported by |file|.
void Generator::PrintImports() const {
  bool has_importlib = false;
  for (int i = 0; i < file_->dependency_count(); ++i) {
    absl::string_view filename = file_->dependency(i)->name();

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
      if (has_importlib == false) {
        printer_->Print("import importlib\n");
        has_importlib = true;
      }
      printer_->Print("$alias$ = importlib.import_module('$name$')\n", "alias",
                      module_alias, "name", module_name);
    } else {
      size_t last_dot_pos = module_name.rfind('.');
      std::string import_statement;
      if (last_dot_pos == std::string::npos) {
        // NOTE: this is not tested as it would require a protocol buffer
        // outside of any package, and I don't think that is easily achievable.
        import_statement = absl::StrCat("import ", module_name);
      } else {
        import_statement =
            absl::StrCat("from ", module_name.substr(0, last_dot_pos),
                         " import ", module_name.substr(last_dot_pos + 1));
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

template <typename DescriptorT>
std::string Generator::GetResolvedFeatures(
    const DescriptorT& descriptor) const {
  if (!GeneratingDescriptorProto()) {
    // Everything but descriptor.proto can handle proper feature resolution.
    return "None";
  }

  // Load the resolved features from our pool.
  const Descriptor* feature_set =
      file_->FindMessageTypeByName(FeatureSet::GetDescriptor()->name());
  ABSL_CHECK(feature_set != nullptr)
      << "Malformed descriptor.proto doesn't contain "
      << FeatureSet::GetDescriptor()->full_name();
  auto message_factory = absl::make_unique<DynamicMessageFactory>();
  auto features =
      absl::WrapUnique(message_factory->GetPrototype(feature_set)->New());
  features->ParseFromString(
      GetResolvedSourceFeatures(descriptor).SerializeAsString());

  // Collect all of the resolved features.
  std::vector<std::string> feature_args;
  const Reflection* reflection = features->GetReflection();
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(*features, &fields);
  for (const auto* field : fields) {
    // Assume these are all enums.  If we add non-enum global features or any
    // python-specific features, we will need to come back and improve this
    // logic.
    if (field->type() != FieldDescriptor::TYPE_ENUM) {
      ABSL_CHECK(field->is_extension())
          << "Unsupported non-enum global feature found: "
          << field->full_name();
      // Placeholder for python-specific features.
      ABSL_CHECK(field->number() != 1003)
          << "Unsupported python-specific feature found: "
          << field->full_name();
      // Skip any non-python language-specific features.
      continue;
    }
    if (field->options().retention() == FieldOptions::RETENTION_SOURCE) {
      // Skip any source-retention features.
      continue;
    }
    const EnumDescriptor* enm = field->enum_type();
    const EnumValueDescriptor* value =
        enm->FindValueByNumber(reflection->GetEnumValue(*features, field));

    feature_args.emplace_back(absl::StrCat(
        field->name(), "=",
        absl::StrFormat("%s.values_by_name[\"%s\"].number",
                        ModuleLevelDescriptorName(*enm), value->name())));
  }
  return absl::StrCat("_ResolvedFeatures(", absl::StrJoin(feature_args, ","),
                      ")");
}

void Generator::PrintResolvedFeatures() const {
  // Since features are used during the descriptor build, it's impossible to do
  // feature resolution at the normal point for descriptor.proto. Instead, we do
  // feature resolution here in the generator, and embed a custom object on all
  // of the generated descriptors.  This object should act like any other
  // FeatureSet message on normal descriptors, but will never have to be
  // resolved by the python runtime.
  ABSL_CHECK(GeneratingDescriptorProto());
  printer_->Emit({{"resolved_features", GetResolvedFeatures(*file_)},
                  {"descriptor_name", kDescriptorKey}},
                 R"py(
                  class _ResolvedFeatures:
                    def __init__(self, features = None, **kwargs):
                      if features:
                        for k, v in features.FIELDS.items():
                          setattr(self, k, getattr(features, k))
                      else:
                        for k, v in kwargs.items():
                          setattr(self, k, v)
                  $descriptor_name$._features = $resolved_features$
                )py");

#define MAKE_NESTED(desc, CPP_FIELD, PY_FIELD)                                \
  [&] {                                                                       \
    for (int i = 0; i < desc.CPP_FIELD##_count(); ++i) {                      \
      printer_->Emit(                                                         \
          {{"resolved_subfeatures", GetResolvedFeatures(*desc.CPP_FIELD(i))}, \
           {"index", absl::StrCat(i)},                                        \
           {"field", PY_FIELD}},                                              \
          "$descriptor_name$.$field$[$index$]._features = "                   \
          "$resolved_subfeatures$\n");                                        \
    }                                                                         \
  }

  google::protobuf::internal::VisitDescriptors(*file_, [&](const Descriptor& msg) {
    printer_->Emit(
        {{"resolved_features", GetResolvedFeatures(msg)},
         {"descriptor_name", ModuleLevelDescriptorName(msg)},
         {"field_features", MAKE_NESTED(msg, field, "fields")},
         {"oneof_features", MAKE_NESTED(msg, oneof_decl, "oneofs")},
         {"ext_features", MAKE_NESTED(msg, extension, "extensions")}},
        R"py(
          $descriptor_name$._features = $resolved_features$
          $field_features$
          $oneof_features$
          $ext_features$
        )py");
  });
  google::protobuf::internal::VisitDescriptors(*file_, [&](const EnumDescriptor& enm) {
    printer_->Emit({{"resolved_features", GetResolvedFeatures(enm)},
                    {"descriptor_name", ModuleLevelDescriptorName(enm)},
                    {"value_features", MAKE_NESTED(enm, value, "values")}},
                   R"py(
                    $descriptor_name$._features = $resolved_features$
                    $value_features$
                  )py");
  });
#undef MAKE_NESTED
}

// Prints the single file descriptor for this file.
void Generator::PrintFileDescriptor() const {
  absl::flat_hash_map<absl::string_view, std::string> m;
  m["descriptor_name"] = kDescriptorKey;
  m["name"] = std::string(file_->name());
  m["package"] = std::string(file_->package());
  m["syntax"] = GetLegacySyntaxName(GetEdition(*file_));
  m["edition"] = Edition_Name(GetEdition(*file_));
  m["options"] = OptionsValue(proto_.options().SerializeAsString());
  m["serialized_descriptor"] = absl::CHexEscape(file_descriptor_serialized_);
  if (GeneratingDescriptorProto()) {
    printer_->Print("if not _descriptor._USE_C_DESCRIPTORS:\n");
    printer_->Indent();
    // Pure python's AddSerializedFile() depend on the generated
    // descriptor_pb2.py thus we can not use AddSerializedFile() when
    // generated descriptor.proto for pure python.
    const char file_descriptor_template[] =
        "$descriptor_name$ = _descriptor.FileDescriptor(\n"
        "  name='$name$',\n"
        "  package='$package$',\n"
        "  syntax='$syntax$',\n"
        "  edition='$edition$',\n"
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

    // TODO: Also print options and fix the message_type, enum_type,
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
void Generator::PrintAllEnumsInFile() const {
  for (int i = 0; i < file_->enum_type_count(); ++i) {
    PrintEnum(*file_->enum_type(i), proto_.enum_type(i));
  }
  for (int i = 0; i < file_->message_type_count(); ++i) {
    PrintNestedEnums(*file_->message_type(i), proto_.message_type(i));
  }
}

// Prints a Python statement assigning the appropriate module-level
// enum name to a Python EnumDescriptor object equivalent to
// enum_descriptor.
void Generator::PrintEnum(const EnumDescriptor& enum_descriptor,
                          const EnumDescriptorProto& proto) const {
  absl::flat_hash_map<absl::string_view, std::string> m;
  std::string module_level_descriptor_name =
      ModuleLevelDescriptorName(enum_descriptor);
  m["descriptor_name"] = module_level_descriptor_name;
  m["name"] = std::string(enum_descriptor.name());
  m["full_name"] = std::string(enum_descriptor.full_name());
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
  proto.options().SerializeToString(&options_string);
  printer_->Print(m, enum_descriptor_template);
  printer_->Indent();
  printer_->Indent();

  for (int i = 0; i < enum_descriptor.value_count(); ++i) {
    PrintEnumValueDescriptor(*enum_descriptor.value(i), proto.value(i));
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
void Generator::PrintNestedEnums(const Descriptor& descriptor,
                                 const DescriptorProto& proto) const {
  for (int i = 0; i < descriptor.nested_type_count(); ++i) {
    PrintNestedEnums(*descriptor.nested_type(i), proto.nested_type(i));
  }

  for (int i = 0; i < descriptor.enum_type_count(); ++i) {
    PrintEnum(*descriptor.enum_type(i), proto.enum_type(i));
  }
}

// Prints Python equivalents of all Descriptors in |file|.
void Generator::PrintMessageDescriptors() const {
  for (int i = 0; i < file_->message_type_count(); ++i) {
    PrintDescriptor(*file_->message_type(i), proto_.message_type(i));
    printer_->Print("\n");
  }
}

// TODO: Remove python service code from opensource.
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
  absl::flat_hash_map<absl::string_view, std::string> m;
  m["service_name"] = ModuleLevelServiceDescriptorName(descriptor);
  m["name"] = std::string(descriptor.name());
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
void Generator::PrintDescriptor(const Descriptor& message_descriptor,
                                const DescriptorProto& proto) const {
  absl::flat_hash_map<absl::string_view, std::string> m;
  m["name"] = std::string(message_descriptor.name());
  m["full_name"] = std::string(message_descriptor.full_name());
  m["file"] = kDescriptorKey;

  PrintNestedDescriptors(message_descriptor, proto);

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
  PrintFieldsInDescriptor(message_descriptor, proto);
  PrintExtensionsInDescriptor(message_descriptor, proto);

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
    printer_->Print(descriptor_name);
    printer_->Print(",\n");
  }
  printer_->Outdent();
  printer_->Print("],\n");
  std::string options_string;
  proto.options().SerializeToString(&options_string);
  printer_->Print(
      "serialized_options=$options_value$,\n"
      "is_extendable=$extendable$",
      "options_value", OptionsValue(options_string), "extendable",
      message_descriptor.extension_range_count() > 0 ? "True" : "False");
  printer_->Print(",\n");

  // Extension ranges
  printer_->Print("extension_ranges=[");
  for (int i = 0; i < message_descriptor.extension_range_count(); ++i) {
    const Descriptor::ExtensionRange* range =
        message_descriptor.extension_range(i);
    printer_->Print("($start$, $end$), ", "start",
                    absl::StrCat(range->start_number()), "end",
                    absl::StrCat(range->end_number()));
  }
  printer_->Print("],\n");
  printer_->Print("oneofs=[\n");
  printer_->Indent();
  for (int i = 0; i < message_descriptor.oneof_decl_count(); ++i) {
    const OneofDescriptor* desc = message_descriptor.oneof_decl(i);
    m.clear();
    m["name"] = std::string(desc->name());
    m["full_name"] = std::string(desc->full_name());
    m["index"] = absl::StrCat(desc->index());
    options_string =
        OptionsValue(proto.oneof_decl(i).options().SerializeAsString());
    if (options_string == "None") {
      m["serialized_options"] = "";
    } else {
      m["serialized_options"] =
          absl::StrCat(", serialized_options=", options_string);
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
void Generator::PrintNestedDescriptors(const Descriptor& containing_descriptor,
                                       const DescriptorProto& proto) const {
  for (int i = 0; i < containing_descriptor.nested_type_count(); ++i) {
    PrintDescriptor(*containing_descriptor.nested_type(i),
                    proto.nested_type(i));
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
                             absl::string_view prefix,
                             std::vector<std::string>* to_register,
                             bool is_nested) const {
  std::string qualified_name;
  if (is_nested) {
    if (IsPythonKeyword(message_descriptor.name())) {
      qualified_name = absl::StrCat("getattr(", prefix, ", '",
                                    message_descriptor.name(), "')");
    } else {
      qualified_name = absl::StrCat(prefix, ".", message_descriptor.name());
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
  absl::flat_hash_map<absl::string_view, std::string> m;
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
    const Descriptor& containing_descriptor, absl::string_view prefix,
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
    absl::flat_hash_map<absl::string_view, std::string> m;
    const OneofDescriptor* oneof = descriptor.oneof_decl(i);
    m["descriptor_name"] = ModuleLevelDescriptorName(descriptor);
    m["oneof_name"] = std::string(oneof->name());
    for (int j = 0; j < oneof->field_count(); ++j) {
      m["field_name"] = std::string(oneof->field(j)->name());
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
  absl::flat_hash_map<absl::string_view, std::string> m;
  m["descriptor_name"] = kDescriptorKey;
  m["message_name"] = std::string(descriptor.name());
  m["message_descriptor_name"] = ModuleLevelDescriptorName(descriptor);
  const char file_descriptor_template[] =
      "$descriptor_name$.message_types_by_name['$message_name$'] = "
      "$message_descriptor_name$\n";
  printer_->Print(m, file_descriptor_template);
}

void Generator::AddServiceToFileDescriptor(
    const ServiceDescriptor& descriptor) const {
  absl::flat_hash_map<absl::string_view, std::string> m;
  m["descriptor_name"] = kDescriptorKey;
  m["service_name"] = std::string(descriptor.name());
  m["service_descriptor_name"] = ModuleLevelServiceDescriptorName(descriptor);
  const char file_descriptor_template[] =
      "$descriptor_name$.services_by_name['$service_name$'] = "
      "$service_descriptor_name$\n";
  printer_->Print(m, file_descriptor_template);
}

void Generator::AddEnumToFileDescriptor(
    const EnumDescriptor& descriptor) const {
  absl::flat_hash_map<absl::string_view, std::string> m;
  m["descriptor_name"] = kDescriptorKey;
  m["enum_name"] = std::string(descriptor.name());
  m["enum_descriptor_name"] = ModuleLevelDescriptorName(descriptor);
  const char file_descriptor_template[] =
      "$descriptor_name$.enum_types_by_name['$enum_name$'] = "
      "$enum_descriptor_name$\n";
  printer_->Print(m, file_descriptor_template);
}

void Generator::AddExtensionToFileDescriptor(
    const FieldDescriptor& descriptor) const {
  absl::flat_hash_map<absl::string_view, std::string> m;
  m["descriptor_name"] = kDescriptorKey;
  m["field_name"] = std::string(descriptor.name());
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
    absl::string_view python_dict_name) const {
  const std::string field_referencing_expression =
      FieldReferencingExpression(containing_type, field, python_dict_name);
  absl::flat_hash_map<absl::string_view, std::string> m;
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
    absl::string_view python_dict_name) const {
  // We should only ever be looking up fields in the current file.
  // The only things we refer to from other files are message descriptors.
  ABSL_CHECK_EQ(field.file(), file_)
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

  // TODO: Move this register to PrintFileDescriptor() when
  // FieldDescriptor.file is added in generated file.
  printer_->Print("_sym_db.RegisterFileDescriptor($name$)\n", "name",
                  kDescriptorKey);
  printer_->Print("\n");
}

// Returns a Python expression that instantiates a Python EnumValueDescriptor
// object for the given C++ descriptor.
void Generator::PrintEnumValueDescriptor(
    const EnumValueDescriptor& descriptor,
    const EnumValueDescriptorProto& proto) const {
  // TODO: Fix up EnumValueDescriptor "type" fields.
  // More circular references.  ::sigh::
  std::string options_string;
  proto.options().SerializeToString(&options_string);
  absl::flat_hash_map<absl::string_view, std::string> m;
  m["name"] = std::string(descriptor.name());
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

// Prints an expression for a Python FieldDescriptor for |field|.
void Generator::PrintFieldDescriptor(const FieldDescriptor& field,
                                     const FieldDescriptorProto& proto) const {
  std::string options_string;
  proto.options().SerializeToString(&options_string);
  absl::flat_hash_map<absl::string_view, std::string> m;
  m["name"] = std::string(field.name());
  m["full_name"] = std::string(field.full_name());
  m["index"] = absl::StrCat(field.index());
  m["number"] = absl::StrCat(field.number());
  m["type"] = absl::StrCat(field.type());
  m["cpp_type"] = absl::StrCat(field.cpp_type());
  m["has_default_value"] = field.has_default_value() ? "True" : "False";
  m["default_value"] = StringifyDefaultValue(field);
  m["is_extension"] = field.is_extension() ? "True" : "False";
  m["serialized_options"] = OptionsValue(options_string);
  m["json_name"] = field.has_json_name()
                       ? absl::StrCat(", json_name='", field.json_name(), "'")
                       : "";
  if (field.is_required()) {
    m["label"] = absl::StrCat(FieldDescriptor::Label::LABEL_REQUIRED);
  } else if (field.is_repeated()) {
    m["label"] = absl::StrCat(FieldDescriptor::Label::LABEL_REPEATED);
  } else {
    m["label"] = absl::StrCat(FieldDescriptor::Label::LABEL_OPTIONAL);
  }
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
    const Descriptor& message_descriptor, const DescriptorProto& proto,
    bool is_extension, absl::string_view list_variable_name) const {
  printer_->Print("$list$=[\n", "list", list_variable_name);
  printer_->Indent();
  int count = is_extension ? message_descriptor.extension_count()
                           : message_descriptor.field_count();
  for (int i = 0; i < count; ++i) {
    PrintFieldDescriptor(is_extension ? *message_descriptor.extension(i)
                                      : *message_descriptor.field(i),
                         is_extension ? proto.extension(i) : proto.field(i));
    printer_->Print(",\n");
  }
  printer_->Outdent();
  printer_->Print("],\n");
}

// Prints a statement assigning "fields" to a list of Python FieldDescriptors,
// one for each field present in message_descriptor.
void Generator::PrintFieldsInDescriptor(const Descriptor& message_descriptor,
                                        const DescriptorProto& proto) const {
  const bool is_extension = false;
  PrintFieldDescriptorsInDescriptor(message_descriptor, proto, is_extension,
                                    "fields");
}

// Prints a statement assigning "extensions" to a list of Python
// FieldDescriptors, one for each extension present in message_descriptor.
void Generator::PrintExtensionsInDescriptor(
    const Descriptor& message_descriptor, const DescriptorProto& proto) const {
  const bool is_extension = true;
  PrintFieldDescriptorsInDescriptor(message_descriptor, proto, is_extension,
                                    "extensions");
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
  name = absl::StrCat("_", name);
  // We now have the name relative to its own module.  Also qualify with
  // the module name iff this descriptor is from a different .proto file.
  if (descriptor.file() != file_) {
    name = absl::StrCat(ModuleAlias(descriptor.file()->name()), ".", name);
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
    name = absl::StrCat(ModuleAlias(descriptor.file()->name()), ".", name);
  }
  return name;
}

// Returns the unique Python module-level identifier given to a service
// descriptor.
std::string Generator::ModuleLevelServiceDescriptorName(
    const ServiceDescriptor& descriptor) const {
  std::string name = absl::StrCat("_", descriptor.name());
  absl::AsciiStrToUpper(&name);
  if (descriptor.file() != file_) {
    name = absl::StrCat(ModuleAlias(descriptor.file()->name()), ".", name);
  }
  return name;
}

// Prints descriptor offsets _serialized_start and _serialized_end.
// Args:
//   descriptor_proto: The descriptor proto to have a serialized reference.
// Example printer output:
// _globals['_MYMESSAGE']._serialized_start=47
// _globals['_MYMESSAGE']._serialized_end=76
template <typename DescriptorProtoT>
void Generator::PrintSerializedPbInterval(
    const DescriptorProtoT& descriptor_proto, absl::string_view name) const {
  std::string sp;
  descriptor_proto.SerializeToString(&sp);
  size_t offset = file_descriptor_serialized_.find(sp);
  ABSL_CHECK_GE(offset, 0);

  printer_->Print(
      "_globals['$name$']._serialized_start=$serialized_start$\n"
      "_globals['$name$']._serialized_end=$serialized_end$\n",
      "name", name, "serialized_start", absl::StrCat(offset), "serialized_end",
      absl::StrCat(offset + sp.size()));
}

template <typename DescriptorT>
bool Generator::PrintDescriptorOptionsFixingCode(
    const DescriptorT& descriptor, const typename DescriptorT::Proto& proto,
    absl::string_view descriptor_str) const {
  std::string options = OptionsValue(proto.options().SerializeAsString());

  // Reset the _options to None thus DescriptorBase.GetOptions() can
  // parse _options again after extensions are registered.
  size_t dot_pos = descriptor_str.find('.');
  std::string descriptor_name;
  if (dot_pos == std::string::npos) {
    descriptor_name = absl::StrCat("_globals['", descriptor_str, "']");
  } else {
    descriptor_name =
        absl::StrCat("_globals['", descriptor_str.substr(0, dot_pos), "']",
                     descriptor_str.substr(dot_pos));
  }

  if (options == "None") {
    return false;
  }

  printer_->Print(
      "$descriptor_name$._loaded_options = None\n"
      "$descriptor_name$._serialized_options = $serialized_value$\n",
      "descriptor_name", descriptor_name, "serialized_value", options);
  return true;
}

// Generates the start and end offsets for each entity in the serialized file
// descriptor. The file argument must exactly match what was serialized into
// file_descriptor_serialized_, and should already have had any
// source-retention options stripped out. This is important because we need an
// exact byte-for-byte match so that we can successfully find the correct
// offsets in the serialized descriptors.
void Generator::SetSerializedPbInterval(const FileDescriptorProto& file) const {
  // Top level enums.
  for (int i = 0; i < file_->enum_type_count(); ++i) {
    const EnumDescriptor& descriptor = *file_->enum_type(i);
    PrintSerializedPbInterval(file.enum_type(i),
                              ModuleLevelDescriptorName(descriptor));
  }

  // Messages.
  for (int i = 0; i < file_->message_type_count(); ++i) {
    SetMessagePbInterval(file.message_type(i), *file_->message_type(i));
  }

  // Services.
  for (int i = 0; i < file_->service_count(); ++i) {
    const ServiceDescriptor& service = *file_->service(i);
    PrintSerializedPbInterval(file.service(i),
                              ModuleLevelServiceDescriptorName(service));
  }
}

void Generator::SetMessagePbInterval(const DescriptorProto& message_proto,
                                     const Descriptor& descriptor) const {
  PrintSerializedPbInterval(message_proto,
                            ModuleLevelDescriptorName(descriptor));

  // Nested messages.
  for (int i = 0; i < descriptor.nested_type_count(); ++i) {
    SetMessagePbInterval(message_proto.nested_type(i),
                         *descriptor.nested_type(i));
  }

  for (int i = 0; i < descriptor.enum_type_count(); ++i) {
    const EnumDescriptor& enum_des = *descriptor.enum_type(i);
    PrintSerializedPbInterval(message_proto.enum_type(i),
                              ModuleLevelDescriptorName(enum_des));
  }
}

// Prints expressions that set the options field of all descriptors.
void Generator::FixAllDescriptorOptions() const {
  // Prints an expression that sets the file descriptor's options.
  if (!PrintDescriptorOptionsFixingCode(*file_, proto_, kDescriptorKey)) {
    printer_->Print("DESCRIPTOR._loaded_options = None\n");
  }
  // Prints expressions that set the options for all top level enums.
  for (int i = 0; i < file_->enum_type_count(); ++i) {
    FixOptionsForEnum(*file_->enum_type(i), proto_.enum_type(i));
  }
  // Prints expressions that set the options for all top level extensions.
  for (int i = 0; i < file_->extension_count(); ++i) {
    FixOptionsForField(*file_->extension(i), proto_.extension(i));
  }
  // Prints expressions that set the options for all messages, nested enums,
  // nested extensions and message fields.
  for (int i = 0; i < file_->message_type_count(); ++i) {
    FixOptionsForMessage(*file_->message_type(i), proto_.message_type(i));
  }

  for (int i = 0; i < file_->service_count(); ++i) {
    FixOptionsForService(*file_->service(i), proto_.service(i));
  }
}

void Generator::FixOptionsForOneof(const OneofDescriptor& oneof,
                                   const OneofDescriptorProto& proto) const {
  std::string oneof_name = absl::Substitute(
      "$0.$1['$2']", ModuleLevelDescriptorName(*oneof.containing_type()),
      "oneofs_by_name", oneof.name());
  PrintDescriptorOptionsFixingCode(oneof, proto, oneof_name);
}

// Prints expressions that set the options for an enum descriptor and its
// value descriptors.
void Generator::FixOptionsForEnum(const EnumDescriptor& enum_descriptor,
                                  const EnumDescriptorProto& proto) const {
  std::string descriptor_name = ModuleLevelDescriptorName(enum_descriptor);
  PrintDescriptorOptionsFixingCode(enum_descriptor, proto, descriptor_name);
  for (int i = 0; i < enum_descriptor.value_count(); ++i) {
    const EnumValueDescriptor& value_descriptor = *enum_descriptor.value(i);
    PrintDescriptorOptionsFixingCode(
        value_descriptor, proto.value(i),
        absl::StrFormat("%s.values_by_name[\"%s\"]", descriptor_name.c_str(),
                        value_descriptor.name()));
  }
}

// Prints expressions that set the options for an service descriptor and its
// value descriptors.
void Generator::FixOptionsForService(
    const ServiceDescriptor& service_descriptor,
    const ServiceDescriptorProto& proto) const {
  std::string descriptor_name =
      ModuleLevelServiceDescriptorName(service_descriptor);
  PrintDescriptorOptionsFixingCode(service_descriptor, proto, descriptor_name);

  for (int i = 0; i < service_descriptor.method_count(); ++i) {
    const MethodDescriptor* method = service_descriptor.method(i);
    std::string method_name = absl::StrCat(
        descriptor_name, ".methods_by_name['", method->name(), "']");
    PrintDescriptorOptionsFixingCode(*method, proto.method(i), method_name);
  }
}

// Prints expressions that set the options for field descriptors (including
// extensions).
void Generator::FixOptionsForField(const FieldDescriptor& field,
                                   const FieldDescriptorProto& proto) const {
  std::string field_name;
  if (field.is_extension()) {
    if (field.extension_scope() == nullptr) {
      // Top level extensions.
      field_name = std::string(field.name());
    } else {
      field_name = FieldReferencingExpression(field.extension_scope(), field,
                                              "extensions_by_name");
    }
  } else {
    field_name = FieldReferencingExpression(field.containing_type(), field,
                                            "fields_by_name");
  }
  PrintDescriptorOptionsFixingCode(field, proto, field_name);
}

// Prints expressions that set the options for a message and all its inner
// types (nested messages, nested enums, extensions, fields).
void Generator::FixOptionsForMessage(const Descriptor& descriptor,
                                     const DescriptorProto& proto) const {
  // Nested messages.
  for (int i = 0; i < descriptor.nested_type_count(); ++i) {
    FixOptionsForMessage(*descriptor.nested_type(i), proto.nested_type(i));
  }
  // Oneofs.
  for (int i = 0; i < descriptor.oneof_decl_count(); ++i) {
    FixOptionsForOneof(*descriptor.oneof_decl(i), proto.oneof_decl(i));
  }
  // Enums.
  for (int i = 0; i < descriptor.enum_type_count(); ++i) {
    FixOptionsForEnum(*descriptor.enum_type(i), proto.enum_type(i));
  }
  // Fields.
  for (int i = 0; i < descriptor.field_count(); ++i) {
    const FieldDescriptor& field = *descriptor.field(i);
    FixOptionsForField(field, proto.field(i));
  }
  // Extensions.
  for (int i = 0; i < descriptor.extension_count(); ++i) {
    const FieldDescriptor& field = *descriptor.extension(i);
    FixOptionsForField(field, proto.extension(i));
  }
  // Message option for this message.
  PrintDescriptorOptionsFixingCode(descriptor, proto,
                                   ModuleLevelDescriptorName(descriptor));
}

// If a dependency forwards other files through public dependencies, let's
// copy over the corresponding module aliases.
void Generator::CopyPublicDependenciesAliases(
    absl::string_view copy_from, const FileDescriptor* file) const {
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
