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

#include <google/protobuf/compiler/python/pyi_generator.h>

#include <string>

#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/compiler/python/helpers.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace python {

template <typename DescriptorT>
struct SortByName {
  bool operator()(const DescriptorT* l, const DescriptorT* r) const {
    return l->name() < r->name();
  }
};

PyiGenerator::PyiGenerator() : file_(nullptr) {}

PyiGenerator::~PyiGenerator() {}

void PyiGenerator::PrintItemMap(
    const std::map<std::string, std::string>& item_map) const {
  for (const auto& entry : item_map) {
    printer_->Print("$key$: $value$\n", "key", entry.first, "value",
                    entry.second);
  }
}

template <typename DescriptorT>
std::string PyiGenerator::ModuleLevelName(
    const DescriptorT& descriptor,
    const std::map<std::string, std::string>& import_map) const {
  std::string name = NamePrefixedWithNestedTypes(descriptor, ".");
  if (descriptor.file() != file_) {
    std::string module_alias;
    std::string filename = descriptor.file()->name();
    if (import_map.find(filename) == import_map.end()) {
      std::string module_name = ModuleName(descriptor.file()->name());
      std::vector<std::string> tokens = Split(module_name, ".");
      module_alias = "_" + tokens.back();
    } else {
      module_alias = import_map.at(filename);
    }
    name = module_alias + "." + name;
  }
  return name;
}

struct ImportModules {
  bool has_repeated = false;    // _containers
  bool has_iterable = false;    // typing.Iterable
  bool has_messages = false;    // _message
  bool has_enums = false;       // _enum_type_wrapper
  bool has_extendable = false;  // _python_message
  bool has_mapping = false;     // typing.Mapping
  bool has_optional = false;    // typing.Optional
  bool has_union = false;       // typing.Union
  bool has_well_known_type = false;
};

// Checks whether a descriptor name matches a well-known type.
bool IsWellKnownType(const std::string& name) {
  // LINT.IfChange(wktbases)
  return (name == "google.protobuf.Any" ||
          name == "google.protobuf.Duration" ||
          name == "google.protobuf.FieldMask" ||
          name == "google.protobuf.ListValue" ||
          name == "google.protobuf.Struct" ||
          name == "google.protobuf.Timestamp");
  // LINT.ThenChange(//depot/google3/net/proto2/python/internal/well_known_types.py:wktbases)
}

// Checks what modules should be imported for this message
// descriptor.
void CheckImportModules(const Descriptor* descriptor,
                        ImportModules* import_modules) {
  if (descriptor->extension_range_count() > 0) {
    import_modules->has_extendable = true;
  }
  if (descriptor->enum_type_count() > 0) {
    import_modules->has_enums = true;
  }
  if (IsWellKnownType(descriptor->full_name())) {
    import_modules->has_well_known_type = true;
  }
  for (int i = 0; i < descriptor->field_count(); ++i) {
    const FieldDescriptor* field = descriptor->field(i);
    if (IsPythonKeyword(field->name())) {
      continue;
    }
    import_modules->has_optional = true;
    if (field->is_repeated()) {
      import_modules->has_repeated = true;
    }
    if (field->is_map()) {
      import_modules->has_mapping = true;
      const FieldDescriptor* value_des = field->message_type()->field(1);
      if (value_des->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ||
          value_des->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
        import_modules->has_union = true;
      }
    } else {
      if (field->is_repeated()) {
        import_modules->has_iterable = true;
      }
      if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        import_modules->has_union = true;
        import_modules->has_mapping = true;
      }
      if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
        import_modules->has_union = true;
      }
    }
  }
  for (int i = 0; i < descriptor->nested_type_count(); ++i) {
    CheckImportModules(descriptor->nested_type(i), import_modules);
  }
}

void PyiGenerator::PrintImportForDescriptor(
    const FileDescriptor& desc,
    std::map<std::string, std::string>* import_map,
    std::set<std::string>* seen_aliases) const {
  const std::string& filename = desc.name();
  std::string module_name = StrippedModuleName(filename);
  size_t last_dot_pos = module_name.rfind('.');
  std::string import_statement;
  if (last_dot_pos == std::string::npos) {
    import_statement = "import " + module_name;
  } else {
    import_statement = "from " + module_name.substr(0, last_dot_pos) +
                       " import " + module_name.substr(last_dot_pos + 1);
    module_name = module_name.substr(last_dot_pos + 1);
  }
  std::string alias = "_" + module_name;
  // Generate a unique alias by adding _1 suffixes until we get an unused alias.
  while (seen_aliases->find(alias) != seen_aliases->end()) {
    alias = alias + "_1";
  }
  printer_->Print("$statement$ as $alias$\n", "statement",
                  import_statement, "alias", alias);
  (*import_map)[filename] = alias;
  seen_aliases->insert(alias);
}

void PyiGenerator::PrintImports(
    std::map<std::string, std::string>* item_map,
    std::map<std::string, std::string>* import_map) const {
  // Prints imported dependent _pb2 files.
  std::set<std::string> seen_aliases;
  for (int i = 0; i < file_->dependency_count(); ++i) {
    const FileDescriptor* dep = file_->dependency(i);
    PrintImportForDescriptor(*dep, import_map, &seen_aliases);
    for (int j = 0; j < dep->public_dependency_count(); ++j) {
      PrintImportForDescriptor(
          *dep->public_dependency(j), import_map, &seen_aliases);
    }
  }

  // Checks what modules should be imported.
  ImportModules import_modules;
  if (file_->message_type_count() > 0) {
    import_modules.has_messages = true;
  }
  if (file_->enum_type_count() > 0) {
    import_modules.has_enums = true;
  }
  for (int i = 0; i < file_->message_type_count(); i++) {
    CheckImportModules(file_->message_type(i), &import_modules);
  }

  // Prints modules (e.g. _containers, _messages, typing) that are
  // required in the proto file.
  if (import_modules.has_repeated) {
    printer_->Print(
        "from google.protobuf.internal import containers as "
        "_containers\n");
  }
  if (import_modules.has_enums) {
    printer_->Print(
        "from google.protobuf.internal import enum_type_wrapper"
        " as _enum_type_wrapper\n");
  }
  if (import_modules.has_extendable) {
    printer_->Print(
        "from google.protobuf.internal import python_message"
        " as _python_message\n");
  }
  if (import_modules.has_well_known_type) {
    printer_->Print(
        "from google.protobuf.internal import well_known_types"
        " as _well_known_types\n");
  }
  printer_->Print(
      "from google.protobuf import"
      " descriptor as _descriptor\n");
  if (import_modules.has_messages) {
    printer_->Print(
        "from google.protobuf import message as _message\n");
  }
  if (HasGenericServices(file_)) {
    printer_->Print(
        "from google.protobuf import service as"
        " _service\n");
  }
  printer_->Print("from typing import ");
  printer_->Print("ClassVar as _ClassVar");
  if (import_modules.has_iterable) {
    printer_->Print(", Iterable as _Iterable");
  }
  if (import_modules.has_mapping) {
    printer_->Print(", Mapping as _Mapping");
  }
  if (import_modules.has_optional) {
    printer_->Print(", Optional as _Optional");
  }
  if (import_modules.has_union) {
    printer_->Print(", Union as _Union");
  }
  printer_->Print("\n\n");

  // Public imports
  for (int i = 0; i < file_->public_dependency_count(); ++i) {
    const FileDescriptor* public_dep = file_->public_dependency(i);
    std::string module_name = StrippedModuleName(public_dep->name());
    // Top level messages in public imports
    for (int i = 0; i < public_dep->message_type_count(); ++i) {
      printer_->Print("from $module$ import $message_class$\n", "module",
                      module_name, "message_class",
                      public_dep->message_type(i)->name());
    }
    // Top level enums for public imports
    for (int i = 0; i < public_dep->enum_type_count(); ++i) {
      printer_->Print("from $module$ import $enum_class$\n", "module",
                      module_name, "enum_class",
                      public_dep->enum_type(i)->name());
    }
    // Enum values for public imports
    for (int i = 0; i < public_dep->enum_type_count(); ++i) {
      const EnumDescriptor* enum_descriptor = public_dep->enum_type(i);
      for (int j = 0; j < enum_descriptor->value_count(); ++j) {
        (*item_map)[enum_descriptor->value(j)->name()] =
            ModuleLevelName(*enum_descriptor, *import_map);
      }
    }
    // Top level extensions for public imports
    AddExtensions(*public_dep, item_map);
  }
}

void PyiGenerator::PrintEnum(const EnumDescriptor& enum_descriptor) const {
  std::string enum_name = enum_descriptor.name();
  printer_->Print(
      "class $enum_name$(int, metaclass=_enum_type_wrapper.EnumTypeWrapper):\n"
      "    __slots__ = []\n",
      "enum_name", enum_name);
}

// Adds enum value to item map which will be ordered and printed later.
void PyiGenerator::AddEnumValue(
    const EnumDescriptor& enum_descriptor,
    std::map<std::string, std::string>* item_map,
    const std::map<std::string, std::string>& import_map) const {
  // enum values
  std::string module_enum_name = ModuleLevelName(enum_descriptor, import_map);
  for (int j = 0; j < enum_descriptor.value_count(); ++j) {
    const EnumValueDescriptor* value_descriptor = enum_descriptor.value(j);
    (*item_map)[value_descriptor->name()] = module_enum_name;
  }
}

// Prints top level enums
void PyiGenerator::PrintTopLevelEnums() const {
  for (int i = 0; i < file_->enum_type_count(); ++i) {
    printer_->Print("\n");
    PrintEnum(*file_->enum_type(i));
  }
}

// Add top level extensions to item_map which will be ordered and
// printed later.
template <typename DescriptorT>
void PyiGenerator::AddExtensions(
    const DescriptorT& descriptor,
    std::map<std::string, std::string>* item_map) const {
  for (int i = 0; i < descriptor.extension_count(); ++i) {
    const FieldDescriptor* extension_field = descriptor.extension(i);
    std::string constant_name = extension_field->name() + "_FIELD_NUMBER";
    ToUpper(&constant_name);
    (*item_map)[constant_name] = "_ClassVar[int]";
    (*item_map)[extension_field->name()] = "_descriptor.FieldDescriptor";
  }
}

// Returns the string format of a field's cpp_type
std::string PyiGenerator::GetFieldType(
    const FieldDescriptor& field_des, const Descriptor& containing_des,
    const std::map<std::string, std::string>& import_map) const {
  switch (field_des.cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_UINT32:
    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT64:
      return "int";
    case FieldDescriptor::CPPTYPE_DOUBLE:
    case FieldDescriptor::CPPTYPE_FLOAT:
      return "float";
    case FieldDescriptor::CPPTYPE_BOOL:
      return "bool";
    case FieldDescriptor::CPPTYPE_ENUM:
      return ModuleLevelName(*field_des.enum_type(), import_map);
    case FieldDescriptor::CPPTYPE_STRING:
      if (field_des.type() == FieldDescriptor::TYPE_STRING) {
        return "str";
      } else {
        return "bytes";
      }
    case FieldDescriptor::CPPTYPE_MESSAGE: {
      // If the field is inside a nested message and the nested message has the
      // same name as a top-level message, then we need to prefix the field type
      // with the module name for disambiguation.
      std::string name = ModuleLevelName(*field_des.message_type(), import_map);
      if ((containing_des.containing_type() != nullptr &&
           name == containing_des.name())) {
        std::string module = ModuleName(field_des.file()->name());
        name = module + "." + name;
      }
      return name;
    }
    default:
      GOOGLE_LOG(FATAL) << "Unsupported field type.";
  }
  return "";
}

void PyiGenerator::PrintMessage(
    const Descriptor& message_descriptor, bool is_nested,
    const std::map<std::string, std::string>& import_map) const {
  if (!is_nested) {
    printer_->Print("\n");
  }
  std::string class_name = message_descriptor.name();
  std::string extra_base;
  // A well-known type needs to inherit from its corresponding base class in
  // net/proto2/python/internal/well_known_types.
  if (IsWellKnownType(message_descriptor.full_name())) {
    extra_base = ", _well_known_types." + message_descriptor.name();
  } else {
    extra_base = "";
  }
  printer_->Print("class $class_name$(_message.Message$extra_base$):\n",
                  "class_name", class_name, "extra_base", extra_base);
  printer_->Indent();
  printer_->Indent();

  std::vector<const FieldDescriptor*> fields;
  fields.reserve(message_descriptor.field_count());
  for (int i = 0; i < message_descriptor.field_count(); ++i) {
    fields.push_back(message_descriptor.field(i));
  }
  std::sort(fields.begin(), fields.end(), SortByName<FieldDescriptor>());

  // Prints slots
  printer_->Print("__slots__ = [", "class_name", class_name);
  bool first_item = true;
  for (const auto& field_des : fields) {
    if (IsPythonKeyword(field_des->name())) {
      continue;
    }
    if (first_item) {
      first_item = false;
    } else {
      printer_->Print(", ");
    }
    printer_->Print("\"$field_name$\"", "field_name", field_des->name());
  }
  printer_->Print("]\n");

  std::map<std::string, std::string> item_map;
  // Prints Extensions for extendable messages
  if (message_descriptor.extension_range_count() > 0) {
    item_map["Extensions"] = "_python_message._ExtensionDict";
  }

  // Prints nested enums
  std::vector<const EnumDescriptor*> nested_enums;
  nested_enums.reserve(message_descriptor.enum_type_count());
  for (int i = 0; i < message_descriptor.enum_type_count(); ++i) {
    nested_enums.push_back(message_descriptor.enum_type(i));
  }
  std::sort(nested_enums.begin(), nested_enums.end(),
            SortByName<EnumDescriptor>());

  for (const auto& entry : nested_enums) {
    PrintEnum(*entry);
    // Adds enum value to item_map which will be ordered and printed later
    AddEnumValue(*entry, &item_map, import_map);
  }

  // Prints nested messages
  std::vector<const Descriptor*> nested_messages;
  nested_messages.reserve(message_descriptor.nested_type_count());
  for (int i = 0; i < message_descriptor.nested_type_count(); ++i) {
    nested_messages.push_back(message_descriptor.nested_type(i));
  }
  std::sort(nested_messages.begin(), nested_messages.end(),
            SortByName<Descriptor>());

  for (const auto& entry : nested_messages) {
    PrintMessage(*entry, true, import_map);
  }

  // Adds extensions to item_map which will be ordered and printed later
  AddExtensions(message_descriptor, &item_map);

  // Adds field number and field descriptor to item_map
  for (int i = 0; i < message_descriptor.field_count(); ++i) {
    const FieldDescriptor& field_des = *message_descriptor.field(i);
    item_map[ToUpper(field_des.name()) + "_FIELD_NUMBER"] =
        "_ClassVar[int]";
    if (IsPythonKeyword(field_des.name())) {
      continue;
    }
    std::string field_type = "";
    if (field_des.is_map()) {
      const FieldDescriptor* key_des = field_des.message_type()->field(0);
      const FieldDescriptor* value_des = field_des.message_type()->field(1);
      field_type = (value_des->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE
                        ? "_containers.MessageMap["
                        : "_containers.ScalarMap[");
      field_type += GetFieldType(*key_des, message_descriptor, import_map);
      field_type += ", ";
      field_type += GetFieldType(*value_des, message_descriptor, import_map);
    } else {
      if (field_des.is_repeated()) {
        field_type = (field_des.cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE
                          ? "_containers.RepeatedCompositeFieldContainer["
                          : "_containers.RepeatedScalarFieldContainer[");
      }
      field_type += GetFieldType(field_des, message_descriptor, import_map);
    }

    if (field_des.is_repeated()) {
      field_type += "]";
    }
    item_map[field_des.name()] = field_type;
  }

  // Prints all items in item_map
  PrintItemMap(item_map);

  // Prints __init__
  printer_->Print("def __init__(self");
  bool has_key_words = false;
  bool is_first = true;
  for (int i = 0; i < message_descriptor.field_count(); ++i) {
    const FieldDescriptor* field_des = message_descriptor.field(i);
    if (IsPythonKeyword(field_des->name())) {
      has_key_words = true;
      continue;
    }
    std::string field_name = field_des->name();
    if (is_first && field_name == "self") {
      // See b/144146793 for an example of real code that generates a (self,
      // self) method signature. Since repeating a parameter name is illegal in
      // Python, we rename the duplicate self.
      field_name = "self_";
    }
    is_first = false;
    printer_->Print(", $field_name$: ", "field_name", field_name);
    if (field_des->is_repeated() ||
        field_des->cpp_type() != FieldDescriptor::CPPTYPE_BOOL) {
      printer_->Print("_Optional[");
    }
    if (field_des->is_map()) {
      const Descriptor* map_entry = field_des->message_type();
      printer_->Print(
          "_Mapping[$key_type$, $value_type$]", "key_type",
          GetFieldType(*map_entry->field(0), message_descriptor, import_map),
          "value_type",
          GetFieldType(*map_entry->field(1), message_descriptor, import_map));
    } else {
      if (field_des->is_repeated()) {
        printer_->Print("_Iterable[");
      }
      if (field_des->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        printer_->Print(
            "_Union[$type_name$, _Mapping]", "type_name",
            GetFieldType(*field_des, message_descriptor, import_map));
      } else {
        if (field_des->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
          printer_->Print("_Union[$type_name$, str]", "type_name",
                          ModuleLevelName(*field_des->enum_type(), import_map));
        } else {
          printer_->Print(
              "$type_name$", "type_name",
              GetFieldType(*field_des, message_descriptor, import_map));
        }
      }
      if (field_des->is_repeated()) {
        printer_->Print("]");
      }
    }
    if (field_des->is_repeated() ||
        field_des->cpp_type() != FieldDescriptor::CPPTYPE_BOOL) {
      printer_->Print("]");
    }
    printer_->Print(" = ...");
  }
  if (has_key_words) {
    printer_->Print(", **kwargs");
  }
  printer_->Print(") -> None: ...\n");

  printer_->Outdent();
  printer_->Outdent();
}

void PyiGenerator::PrintMessages(
    const std::map<std::string, std::string>& import_map) const {
  // Deterministically order the descriptors.
  std::vector<const Descriptor*> messages;
  messages.reserve(file_->message_type_count());
  for (int i = 0; i < file_->message_type_count(); ++i) {
    messages.push_back(file_->message_type(i));
  }
  std::sort(messages.begin(), messages.end(), SortByName<Descriptor>());

  for (const auto& entry : messages) {
    PrintMessage(*entry, false, import_map);
  }
}

void PyiGenerator::PrintServices() const {
  std::vector<const ServiceDescriptor*> services;
  services.reserve(file_->service_count());
  for (int i = 0; i < file_->service_count(); ++i) {
    services.push_back(file_->service(i));
  }
  std::sort(services.begin(), services.end(), SortByName<ServiceDescriptor>());

  // Prints $Service$ and $Service$_Stub classes
  for (const auto& entry : services) {
    printer_->Print("\n");
    printer_->Print(
        "class $service_name$(_service.service): ...\n\n"
        "class $service_name$_Stub($service_name$): ...\n",
        "service_name", entry->name());
  }
}

bool PyiGenerator::Generate(const FileDescriptor* file,
                            const std::string& parameter,
                            GeneratorContext* context,
                            std::string* error) const {
  MutexLock lock(&mutex_);
  // Calculate file name.
  file_ = file;
  std::string filename =
      parameter.empty() ? GetFileName(file, ".pyi") : parameter;

  std::unique_ptr<io::ZeroCopyOutputStream> output(context->Open(filename));
  GOOGLE_CHECK(output.get());
  io::Printer printer(output.get(), '$');
  printer_ = &printer;

  // item map will store "DESCRIPTOR", top level extensions, top level enum
  // values. The items will be sorted and printed later.
  std::map<std::string, std::string> item_map;

  // Adds "DESCRIPTOR" into item_map.
  item_map["DESCRIPTOR"] = "_descriptor.FileDescriptor";

  // import_map will be a mapping from filename to module alias, e.g.
  // "google3/foo/bar.py" -> "_bar"
  std::map<std::string, std::string> import_map;

  PrintImports(&item_map, &import_map);
  // Adds top level enum values to item_map.
  for (int i = 0; i < file_->enum_type_count(); ++i) {
    AddEnumValue(*file_->enum_type(i), &item_map, import_map);
  }
  // Adds top level extensions to item_map.
  AddExtensions(*file_, &item_map);
  // Prints item map
  PrintItemMap(item_map);

  PrintMessages(import_map);
  PrintTopLevelEnums();
  if (HasGenericServices(file)) {
    PrintServices();
  }
  return true;
}

}  // namespace python
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
