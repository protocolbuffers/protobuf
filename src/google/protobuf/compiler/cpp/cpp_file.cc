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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/compiler/cpp/cpp_file.h>
#include <map>
#include <memory>
#ifndef _SHARED_PTR_H
#include <google/protobuf/stubs/shared_ptr.h>
#endif
#include <set>
#include <vector>

#include <google/protobuf/compiler/cpp/cpp_enum.h>
#include <google/protobuf/compiler/cpp/cpp_service.h>
#include <google/protobuf/compiler/cpp/cpp_extension.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/compiler/cpp/cpp_message.h>
#include <google/protobuf/compiler/cpp/cpp_field.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {
// The list of names that are defined as macros on some platforms. We need to
// #undef them for the generated code to compile.
const char* kMacroNames[] = {"major", "minor"};

bool IsMacroName(const string& name) {
  // Just do a linear search as the number of elements is very small.
  for (int i = 0; i < GOOGLE_ARRAYSIZE(kMacroNames); ++i) {
    if (name == kMacroNames[i]) return true;
  }
  return false;
}

void CollectMacroNames(const Descriptor* message, std::vector<string>* names) {
  for (int i = 0; i < message->field_count(); ++i) {
    const FieldDescriptor* field = message->field(i);
    if (IsMacroName(field->name())) {
      names->push_back(field->name());
    }
  }
  for (int i = 0; i < message->nested_type_count(); ++i) {
    CollectMacroNames(message->nested_type(i), names);
  }
}

void CollectMacroNames(const FileDescriptor* file, std::vector<string>* names) {
  // Only do this for protobuf's own types. There are some google3 protos using
  // macros as field names and the generated code compiles after the macro
  // expansion. Undefing these macros actually breaks such code.
  if (file->name() != "google/protobuf/compiler/plugin.proto") {
    return;
  }
  for (int i = 0; i < file->message_type_count(); ++i) {
    CollectMacroNames(file->message_type(i), names);
  }
}


}  // namespace

// ===================================================================

FileGenerator::FileGenerator(const FileDescriptor* file, const Options& options)
    : file_(file),
      options_(options),
      scc_analyzer_(options),
      message_generators_owner_(
          new google::protobuf::scoped_ptr<MessageGenerator>[file->message_type_count()]),
      enum_generators_owner_(
          new google::protobuf::scoped_ptr<EnumGenerator>[file->enum_type_count()]),
      service_generators_owner_(
          new google::protobuf::scoped_ptr<ServiceGenerator>[file->service_count()]),
      extension_generators_owner_(
          new google::protobuf::scoped_ptr<ExtensionGenerator>[file->extension_count()]) {

  for (int i = 0; i < file->message_type_count(); i++) {
    message_generators_owner_[i].reset(
        new MessageGenerator(file->message_type(i), options, &scc_analyzer_));
    message_generators_owner_[i]->Flatten(&message_generators_);
  }

  for (int i = 0; i < message_generators_.size(); i++) {
    message_generators_[i]->AddGenerators(&enum_generators_,
                                          &extension_generators_);
  }

  for (int i = 0; i < file->enum_type_count(); i++) {
    enum_generators_owner_[i].reset(
        new EnumGenerator(file->enum_type(i), options));
    enum_generators_.push_back(enum_generators_owner_[i].get());
  }

  for (int i = 0; i < file->service_count(); i++) {
    service_generators_owner_[i].reset(
        new ServiceGenerator(file->service(i), options));
    service_generators_.push_back(service_generators_owner_[i].get());
  }

  for (int i = 0; i < file->extension_count(); i++) {
    extension_generators_owner_[i].reset(
        new ExtensionGenerator(file->extension(i), options));
    extension_generators_.push_back(extension_generators_owner_[i].get());
  }

  package_parts_ = Split(file_->package(), ".", true);
}

FileGenerator::~FileGenerator() {}

void FileGenerator::GenerateMacroUndefs(io::Printer* printer) {
  std::vector<string> names_to_undef;
  CollectMacroNames(file_, &names_to_undef);
  for (int i = 0; i < names_to_undef.size(); ++i) {
    printer->Print(
        "#ifdef $name$\n"
        "#undef $name$\n"
        "#endif\n",
        "name", names_to_undef[i]);
  }
}

void FileGenerator::GenerateHeader(io::Printer* printer) {
  printer->Print(
    "// @@protoc_insertion_point(includes)\n");

  GenerateMacroUndefs(printer);

  GenerateForwardDeclarations(printer);

  // Open namespace.
  GenerateNamespaceOpeners(printer);

  GenerateGlobalStateFunctionDeclarations(printer);

  printer->Print("\n");

  GenerateEnumDefinitions(printer);

  printer->Print(kThickSeparator);
  printer->Print("\n");

  GenerateMessageDefinitions(printer);

  printer->Print("\n");
  printer->Print(kThickSeparator);
  printer->Print("\n");

  GenerateServiceDefinitions(printer);

  GenerateExtensionIdentifiers(printer);

  printer->Print("\n");
  printer->Print(kThickSeparator);
  printer->Print("\n");

  GenerateInlineFunctionDefinitions(printer);

  printer->Print(
    "\n"
    "// @@protoc_insertion_point(namespace_scope)\n"
    "\n");

  // Close up namespace.
  GenerateNamespaceClosers(printer);

  // We need to specialize some templates in the ::google::protobuf namespace:
  GenerateProto2NamespaceEnumSpecializations(printer);

  printer->Print(
    "\n"
    "// @@protoc_insertion_point(global_scope)\n"
    "\n");
}

void FileGenerator::GenerateProtoHeader(io::Printer* printer,
                                        const string& info_path) {
  if (!options_.proto_h) {
    return;
  }

  string filename_identifier = FilenameIdentifier(file_->name());
  GenerateTopHeaderGuard(printer, filename_identifier);


  GenerateLibraryIncludes(printer);

  for (int i = 0; i < file_->public_dependency_count(); i++) {
    const FileDescriptor* dep = file_->public_dependency(i);
    const char* extension = ".proto.h";
    string dependency = StripProto(dep->name()) + extension;
    printer->Print(
      "#include \"$dependency$\"  // IWYU pragma: export\n",
      "dependency", dependency);
  }

  GenerateMetadataPragma(printer, info_path);

  GenerateHeader(printer);

  GenerateBottomHeaderGuard(printer, filename_identifier);
}

void FileGenerator::GeneratePBHeader(io::Printer* printer,
                                     const string& info_path) {
  string filename_identifier =
      FilenameIdentifier(file_->name() + (options_.proto_h ? ".pb.h" : ""));
  GenerateTopHeaderGuard(printer, filename_identifier);

  if (options_.proto_h) {
    printer->Print("#include \"$basename$.proto.h\"  // IWYU pragma: export\n",
                   "basename", StripProto(file_->name()));
  } else {
    GenerateLibraryIncludes(printer);
  }

  GenerateDependencyIncludes(printer);
  GenerateMetadataPragma(printer, info_path);

  if (!options_.proto_h) {
    GenerateHeader(printer);
  } else {
    // This is unfortunately necessary for some plugins. I don't see why we
    // need two of the same insertion points.
    // TODO(gerbens) remove this.
    printer->Print(
      "// @@protoc_insertion_point(includes)\n");

    // Open namespace.
    GenerateNamespaceOpeners(printer);
    printer->Print(
      "\n"
      "// @@protoc_insertion_point(namespace_scope)\n");
    // Close up namespace.
    GenerateNamespaceClosers(printer);

    printer->Print(
      "\n"
      "// @@protoc_insertion_point(global_scope)\n"
      "\n");
  }

  GenerateBottomHeaderGuard(printer, filename_identifier);
}

void FileGenerator::GenerateSource(io::Printer* printer) {
  const bool use_system_include = IsWellKnownMessage(file_);
  string header =
      StripProto(file_->name()) + (options_.proto_h ? ".proto.h" : ".pb.h");
  printer->Print(
    "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
    "// source: $filename$\n"
    "\n"
    // The generated code calls accessors that might be deprecated. We don't
    // want the compiler to warn in generated code.
    "#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION\n"
    "#include $left$$header$$right$\n"
    "\n"
    "#include <algorithm>\n"    // for swap()
    "\n"
    "#include <google/protobuf/stubs/common.h>\n"
    "#include <google/protobuf/stubs/port.h>\n"
    "#include <google/protobuf/stubs/once.h>\n"
    "#include <google/protobuf/io/coded_stream.h>\n"
    "#include <google/protobuf/wire_format_lite_inl.h>\n",
    "filename", file_->name(),
    "header", header,
    "left", use_system_include ? "<" : "\"",
    "right", use_system_include ? ">" : "\"");

  // Unknown fields implementation in lite mode uses StringOutputStream
  if (!UseUnknownFieldSet(file_, options_) && !message_generators_.empty()) {
    printer->Print(
      "#include <google/protobuf/io/zero_copy_stream_impl_lite.h>\n");
  }

  if (HasDescriptorMethods(file_, options_)) {
    printer->Print(
      "#include <google/protobuf/descriptor.h>\n"
      "#include <google/protobuf/generated_message_reflection.h>\n"
      "#include <google/protobuf/reflection_ops.h>\n"
      "#include <google/protobuf/wire_format.h>\n");
  }

  if (options_.proto_h) {
    // Use the smaller .proto.h files.
    for (int i = 0; i < file_->dependency_count(); i++) {
      const FileDescriptor* dep = file_->dependency(i);
      const char* extension = ".proto.h";
      string dependency = StripProto(dep->name()) + extension;
      printer->Print(
          "#include \"$dependency$\"\n",
          "dependency", dependency);
    }
  }

  printer->Print(
    "// @@protoc_insertion_point(includes)\n");

  GenerateNamespaceOpeners(printer);

  for (int i = 0; i < message_generators_.size(); i++) {
    string parent;
    if (IsMapEntryMessage(message_generators_[i]->descriptor_)) {
      parent = ClassName(message_generators_[i]->descriptor_->containing_type(),
                         false) +
               "::";
    }
    printer->Print(
        "class $classname$DefaultTypeInternal : "
        "public ::google::protobuf::internal::ExplicitlyConstructed<$parent$$classname$> "
        "{\n",
        "parent", parent, "classname", message_generators_[i]->classname_);
    printer->Indent();
    message_generators_[i]->GenerateExtraDefaultFields(printer);
    printer->Outdent();
    printer->Print(
        "} _$classname$_default_instance_;\n",
        "classname", message_generators_[i]->classname_);
  }

  for (int i = 0; i < enum_generators_.size(); i++) {
    enum_generators_[i]->index_in_metadata_ = i;
  }
  if (HasGenericServices(file_, options_)) {
    for (int i = 0; i < service_generators_.size(); i++) {
      service_generators_[i]->index_in_metadata_ = i;
    }
  }

  printer->Print(
      "\n"
      "namespace $file_namespace$ {\n"
      "\n",
      "file_namespace", FileLevelNamespace(file_->name()));

  if (HasDescriptorMethods(file_, options_)) {
    printer->Print(
      "\n"
      "namespace {\n"
      "\n");

    if (!message_generators_.empty()) {
      printer->Print("::google::protobuf::Metadata file_level_metadata[$size$];\n",
                     "size", SimpleItoa(message_generators_.size()));
    }
    if (!enum_generators_.empty()) {
      printer->Print(
          "const ::google::protobuf::EnumDescriptor* "
          "file_level_enum_descriptors[$size$];\n",
          "size", SimpleItoa(enum_generators_.size()));
    }
    if (HasGenericServices(file_, options_) && file_->service_count() > 0) {
      printer->Print(
          "const ::google::protobuf::ServiceDescriptor* "
          "file_level_service_descriptors[$size$];\n",
          "size", SimpleItoa(file_->service_count()));
    }

    printer->Print(
      "\n"
      "}  // namespace\n"
      "\n");
  }

  // Define our externally-visible BuildDescriptors() function.  (For the lite
  // library, all this does is initialize default instances.)
  GenerateBuildDescriptors(printer);

  printer->Print(
      "\n"
      "}  // namespace $file_namespace$\n"
      "\n",
      "file_namespace", FileLevelNamespace(file_->name()));

  // Generate enums.
  for (int i = 0; i < enum_generators_.size(); i++) {
    enum_generators_[i]->GenerateMethods(printer);
  }

  // Generate classes.
  for (int i = 0; i < message_generators_.size(); i++) {
    printer->Print("\n");
    printer->Print(kThickSeparator);
    printer->Print("\n");
    message_generators_[i]->GenerateClassMethods(printer);

    printer->Print("#if PROTOBUF_INLINE_NOT_IN_HEADERS\n");
    // Generate class inline methods.
    message_generators_[i]->GenerateInlineMethods(printer,
                                                  /* is_inline = */ false);
    printer->Print("#endif  // PROTOBUF_INLINE_NOT_IN_HEADERS\n");
  }

  if (HasGenericServices(file_, options_)) {
    // Generate services.
    for (int i = 0; i < service_generators_.size(); i++) {
      if (i == 0) printer->Print("\n");
      printer->Print(kThickSeparator);
      printer->Print("\n");
      service_generators_[i]->GenerateImplementation(printer);
    }
  }

  // Define extensions.
  for (int i = 0; i < extension_generators_.size(); i++) {
    extension_generators_[i]->GenerateDefinition(printer);
  }

  printer->Print(
    "\n"
    "// @@protoc_insertion_point(namespace_scope)\n");

  GenerateNamespaceClosers(printer);

  printer->Print(
    "\n"
    "// @@protoc_insertion_point(global_scope)\n");
}

class FileGenerator::ForwardDeclarations {
 public:
  ~ForwardDeclarations() {
    for (std::map<string, ForwardDeclarations*>::iterator
             it = namespaces_.begin(),
             end = namespaces_.end();
         it != end; ++it) {
      delete it->second;
    }
    namespaces_.clear();
  }

  ForwardDeclarations* AddOrGetNamespace(const string& ns_name) {
    ForwardDeclarations*& ns = namespaces_[ns_name];
    if (ns == NULL) {
      ns = new ForwardDeclarations;
    }
    return ns;
  }

  std::map<string, const Descriptor*>& classes() { return classes_; }
  std::map<string, const EnumDescriptor*>& enums() { return enums_; }

  void Print(io::Printer* printer, const Options& options) const {
    for (std::map<string, const EnumDescriptor *>::const_iterator
             it = enums_.begin(),
             end = enums_.end();
         it != end; ++it) {
      printer->Print("enum $enumname$ : int;\n", "enumname", it->first);
      printer->Annotate("enumname", it->second);
      printer->Print("bool $enumname$_IsValid(int value);\n", "enumname",
                     it->first);
    }
    for (std::map<string, const Descriptor*>::const_iterator
             it = classes_.begin(),
             end = classes_.end();
         it != end; ++it) {
      printer->Print("class $classname$;\n", "classname", it->first);
      printer->Annotate("classname", it->second);

      printer->Print(
          "class $classname$DefaultTypeInternal;\n"
          "$dllexport_decl$"
          "extern $classname$DefaultTypeInternal "
          "_$classname$_default_instance_;\n",  // NOLINT
          "dllexport_decl",
          options.dllexport_decl.empty() ? "" : options.dllexport_decl + " ",
          "classname",
          it->first);
    }
    for (std::map<string, ForwardDeclarations *>::const_iterator
             it = namespaces_.begin(),
             end = namespaces_.end();
         it != end; ++it) {
      printer->Print("namespace $nsname$ {\n",
                     "nsname", it->first);
      it->second->Print(printer, options);
      printer->Print("}  // namespace $nsname$\n",
                     "nsname", it->first);
    }
  }


 private:
  std::map<string, ForwardDeclarations*> namespaces_;
  std::map<string, const Descriptor*> classes_;
  std::map<string, const EnumDescriptor*> enums_;
};

void FileGenerator::GenerateBuildDescriptors(io::Printer* printer) {
  // AddDescriptors() is a file-level procedure which adds the encoded
  // FileDescriptorProto for this .proto file to the global DescriptorPool for
  // generated files (DescriptorPool::generated_pool()). It either runs at
  // static initialization time (by default) or when default_instance() is
  // called for the first time (in LITE_RUNTIME mode with
  // GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER flag enabled). This procedure also
  // constructs default instances and registers extensions.
  //
  // Its sibling, AssignDescriptors(), actually pulls the compiled
  // FileDescriptor from the DescriptorPool and uses it to populate all of
  // the global variables which store pointers to the descriptor objects.
  // It also constructs the reflection objects.  It is called the first time
  // anyone calls descriptor() or GetReflection() on one of the types defined
  // in the file.

  // In optimize_for = LITE_RUNTIME mode, we don't generate AssignDescriptors()
  // and we only use AddDescriptors() to allocate default instances.

  // TODO(ckennelly): Gate this with the same options flag to enable
  // table-driven parsing.

  printer->Print("PROTOBUF_CONSTEXPR_VAR ::google::protobuf::internal::ParseTableField\n"
                 "    const TableStruct::entries[] = {\n");
  printer->Indent();

  std::vector<size_t> entries;
  size_t count = 0;
  for (int i = 0; i < message_generators_.size(); i++) {
    size_t value = message_generators_[i]->GenerateParseOffsets(printer);
    entries.push_back(value);
    count += value;
  }

  // We need these arrays to exist, and MSVC does not like empty arrays.
  if (count == 0) {
    printer->Print("{0, 0, 0, ::google::protobuf::internal::kInvalidMask, 0, 0},\n");
  }

  printer->Outdent();
  printer->Print(
      "};\n"
      "\n"
      "PROTOBUF_CONSTEXPR_VAR ::google::protobuf::internal::AuxillaryParseTableField\n"
      "    const TableStruct::aux[] = {\n");
  printer->Indent();

  std::vector<size_t> aux_entries;
  count = 0;
  for (int i = 0; i < message_generators_.size(); i++) {
    size_t value = message_generators_[i]->GenerateParseAuxTable(printer);
    aux_entries.push_back(value);
    count += value;
  }

  if (count == 0) {
    printer->Print("::google::protobuf::internal::AuxillaryParseTableField(),\n");
  }

  printer->Outdent();
  printer->Print(
      "};\n"
      "PROTOBUF_CONSTEXPR_VAR ::google::protobuf::internal::ParseTable const\n"
      "    TableStruct::schema[] = {\n");
  printer->Indent();

  size_t offset = 0;
  size_t aux_offset = 0;
  for (int i = 0; i < message_generators_.size(); i++) {
    message_generators_[i]->GenerateParseTable(printer, offset, aux_offset);
    offset += entries[i];
    aux_offset += aux_entries[i];
  }

  if (message_generators_.empty()) {
    printer->Print("{ NULL, NULL, 0, -1, -1, false },\n");
  }

  printer->Outdent();
  printer->Print(
      "};\n"
      "\n");

  if (HasDescriptorMethods(file_, options_)) {
    if (!message_generators_.empty()) {
      printer->Print("const ::google::protobuf::uint32 TableStruct::offsets[] = {\n");
      printer->Indent();
      std::vector<std::pair<size_t, size_t> > pairs;
      for (int i = 0; i < message_generators_.size(); i++) {
        pairs.push_back(message_generators_[i]->GenerateOffsets(printer));
      }
      printer->Outdent();
      printer->Print(
          "};\n"
          "\n"
          "static const ::google::protobuf::internal::MigrationSchema schemas[] = {\n");
      printer->Indent();
      {
        int offset = 0;
        for (int i = 0; i < message_generators_.size(); i++) {
          message_generators_[i]->GenerateSchema(printer, offset,
                                                 pairs[i].second);
          offset += pairs[i].first;
        }
      }
      printer->Outdent();
      printer->Print(
          "};\n"
          "\nstatic "
          "::google::protobuf::Message const * const file_default_instances[] = {\n");
      printer->Indent();
      for (int i = 0; i < message_generators_.size(); i++) {
        const Descriptor* descriptor = message_generators_[i]->descriptor_;
        printer->Print(
            "reinterpret_cast<const "
            "::google::protobuf::Message*>(&_$classname$_default_instance_),\n",
            "classname", ClassName(descriptor, false));
      }
      printer->Outdent();
      printer->Print(
          "};\n"
          "\n");
    } else {
      // we still need these symbols to exist
      printer->Print(
          // MSVC doesn't like empty arrays, so we add a dummy.
          "const ::google::protobuf::uint32 TableStruct::offsets[] = { ~0u };\n"
          "static const ::google::protobuf::internal::MigrationSchema* schemas = NULL;\n"
          "static const ::google::protobuf::Message* const* "
          "file_default_instances = NULL;\n");
    }

    // ---------------------------------------------------------------

    // protobuf_AssignDescriptorsOnce():  The first time it is called, calls
    // AssignDescriptors().  All later times, waits for the first call to
    // complete and then returns.
    string message_factory = "NULL";
    printer->Print(
        "namespace {\n"
        "\n"
        "void protobuf_AssignDescriptors() {\n"
        // Make sure the file has found its way into the pool.  If a descriptor
        // is requested *during* static init then AddDescriptors() may not have
        // been called yet, so we call it manually.  Note that it's fine if
        // AddDescriptors() is called multiple times.
        "  AddDescriptors();\n"
        "  ::google::protobuf::MessageFactory* factory = $factory$;\n"
        "  AssignDescriptors(\n"
        "      \"$filename$\", schemas, file_default_instances, "
        "TableStruct::offsets, factory,\n"
        "      $metadata$, $enum_descriptors$, $service_descriptors$);\n",
        "filename", file_->name(), "metadata",
        !message_generators_.empty() ? "file_level_metadata" : "NULL",
        "enum_descriptors",
        !enum_generators_.empty() ? "file_level_enum_descriptors" : "NULL",
        "service_descriptors",
        HasGenericServices(file_, options_) && file_->service_count() > 0
            ? "file_level_service_descriptors"
            : "NULL",
        "factory", message_factory);
    // TODO(gerbens) have the compiler include the schemas for map types
    // so that this can go away, and we can potentially use table driven
    // serialization for map types as well.
    for (int i = 0; i < message_generators_.size(); i++) {
      if (!IsMapEntryMessage(message_generators_[i]->descriptor_)) continue;
      printer->Print(
          "file_level_metadata[$index$].reflection = "
          "$parent$::$classname$::CreateReflection(file_level_metadata[$index$]"
          ".descriptor, _$classname$_default_instance_.get_mutable());\n",
          "index", SimpleItoa(i), "parent",
          ClassName(message_generators_[i]->descriptor_->containing_type(),
                    false),
          "classname", ClassName(message_generators_[i]->descriptor_, false));
    }
    printer->Print(
        "}\n"
        "\n"
        "void protobuf_AssignDescriptorsOnce() {\n"
        "  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);\n"
        "  ::google::protobuf::GoogleOnceInit(&once, &protobuf_AssignDescriptors);\n"
        "}\n"
        "\n",
        "filename", file_->name(), "metadata",
        !message_generators_.empty() ? "file_level_metadata" : "NULL",
        "enum_descriptors",
        !enum_generators_.empty() ? "file_level_enum_descriptors" : "NULL",
        "service_descriptors",
        HasGenericServices(file_, options_) && file_->service_count() > 0
            ? "file_level_service_descriptors"
            : "NULL",
        "factory", message_factory);

    // Only here because of useless string reference that we don't want in
    // protobuf_AssignDescriptorsOnce, because that is called from all the
    // GetMetadata member methods.
    printer->Print(
        "void protobuf_RegisterTypes(const ::std::string&) GOOGLE_ATTRIBUTE_COLD;\n"
        "void protobuf_RegisterTypes(const ::std::string&) {\n"
        "  protobuf_AssignDescriptorsOnce();\n");
    printer->Indent();

    // All normal messages can be done generically
    if (!message_generators_.empty()) {
      printer->Print(
        "::google::protobuf::internal::RegisterAllTypes(file_level_metadata, $size$);\n",
        "size", SimpleItoa(message_generators_.size()));
    }

    printer->Outdent();
    printer->Print(
      "}\n"
      "\n"
      "}  // namespace\n");
  }

  // -----------------------------------------------------------------

  // ShutdownFile():  Deletes descriptors, default instances, etc. on shutdown.
  printer->Print(
    "\n"
    "void TableStruct::Shutdown() {\n");
  printer->Indent();

  for (int i = 0; i < message_generators_.size(); i++) {
    message_generators_[i]->GenerateShutdownCode(printer);
  }

  if (HasDescriptorMethods(file_, options_)) {
    for (int i = 0; i < message_generators_.size(); i++) {
      if (!IsMapEntryMessage(message_generators_[i]->descriptor_)) continue;
      printer->Print(
          "delete file_level_metadata[$index$].reflection;\n",
          "index", SimpleItoa(i));
    }
  }

  printer->Outdent();
  printer->Print(
    "}\n\n");

  // -----------------------------------------------------------------

  // Now generate the InitDefaultsImpl() function.
  printer->Print(
      "void TableStruct::InitDefaultsImpl() {\n"
      "  GOOGLE_PROTOBUF_VERIFY_VERSION;\n\n"
      // Force initialization of primitive values we depend on.
      "  ::google::protobuf::internal::InitProtobufDefaults();\n");

  printer->Indent();

  // Call the InitDefaults() methods for all of our dependencies, to make
  // sure they get added first.
  for (int i = 0; i < file_->dependency_count(); i++) {
    const FileDescriptor* dependency = file_->dependency(i);
    // Print the namespace prefix for the dependency.
    string file_namespace = QualifiedFileLevelSymbol(
        dependency->package(), FileLevelNamespace(dependency->name()));
    // Call its AddDescriptors function.
    printer->Print("$file_namespace$::InitDefaults();\n", "file_namespace",
                   file_namespace);
  }

  // Allocate and initialize default instances.  This can't be done lazily
  // since default instances are returned by simple accessors and are used with
  // extensions.  Speaking of which, we also register extensions at this time.
  for (int i = 0; i < message_generators_.size(); i++) {
    message_generators_[i]->GenerateDefaultInstanceAllocator(printer);
  }
  for (int i = 0; i < extension_generators_.size(); i++) {
    extension_generators_[i]->GenerateRegistration(printer);
  }
  for (int i = 0; i < message_generators_.size(); i++) {
    message_generators_[i]->GenerateDefaultInstanceInitializer(printer);
  }
  printer->Outdent();
  printer->Print(
      "}\n"
      "\n"
      "void InitDefaults() {\n"
      "  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);\n"
      "  ::google::protobuf::GoogleOnceInit(&once, &TableStruct::InitDefaultsImpl);\n"
      "}\n");

  // -----------------------------------------------------------------

  // Now generate the AddDescriptors() function.
  printer->Print(
      "void AddDescriptorsImpl() {\n"
      "  InitDefaults();\n");

  printer->Indent();
  if (HasDescriptorMethods(file_, options_)) {
    // Embed the descriptor.  We simply serialize the entire
    // FileDescriptorProto
    // and embed it as a string literal, which is parsed and built into real
    // descriptors at initialization time.
    FileDescriptorProto file_proto;
    file_->CopyTo(&file_proto);
    string file_data;
    file_proto.SerializeToString(&file_data);

    printer->Print("static const char descriptor[] = {\n");
    printer->Indent();

    if (file_data.size() > 66535) {
      // Workaround for MSVC: "Error C1091: compiler limit: string exceeds 65535
      // bytes in length". Declare a static array of characters rather than use
      // a string literal. Only write 25 bytes per line.
      static const int kBytesPerLine = 25;
      for (int i = 0; i < file_data.size();) {
        for (int j = 0; j < kBytesPerLine && i < file_data.size(); ++i, ++j) {
          printer->Print("'$char$', ", "char",
                         CEscape(file_data.substr(i, 1)));
        }
        printer->Print("\n");
      }
    } else {
      // Only write 40 bytes per line.
      static const int kBytesPerLine = 40;
      for (int i = 0; i < file_data.size(); i += kBytesPerLine) {
        printer->Print("  \"$data$\"\n", "data",
                       EscapeTrigraphs(CEscape(
                           file_data.substr(i, kBytesPerLine))));
      }
    }

    printer->Outdent();
    printer->Print("};\n");
    printer->Print(
        "::google::protobuf::DescriptorPool::InternalAddGeneratedFile(\n"
        "    descriptor, $size$);\n",
        "size", SimpleItoa(file_data.size()));

    // Call MessageFactory::InternalRegisterGeneratedFile().
    printer->Print(
      "::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(\n"
      "  \"$filename$\", &protobuf_RegisterTypes);\n",
      "filename", file_->name());
  }

  // Call the AddDescriptors() methods for all of our dependencies, to make
  // sure they get added first.
  for (int i = 0; i < file_->dependency_count(); i++) {
    const FileDescriptor* dependency = file_->dependency(i);
    // Print the namespace prefix for the dependency.
    string file_namespace = QualifiedFileLevelSymbol(
        dependency->package(), FileLevelNamespace(dependency->name()));
    // Call its AddDescriptors function.
    printer->Print("$file_namespace$::AddDescriptors();\n", "file_namespace",
                   file_namespace);
  }

  printer->Print(
      "::google::protobuf::internal::OnShutdown(&TableStruct::Shutdown);\n");

  printer->Outdent();
  printer->Print(
      "}\n"
      "\n"
      "void AddDescriptors() {\n"
      "  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);\n"
      "  ::google::protobuf::GoogleOnceInit(&once, &AddDescriptorsImpl);\n"
      "}\n");

  if (!StaticInitializersForced(file_, options_)) {
    printer->Print("#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER\n");
  }
  printer->Print(
      // With static initializers.
      "// Force AddDescriptors() to be called at static initialization time.\n"
      "struct StaticDescriptorInitializer {\n"
      "  StaticDescriptorInitializer() {\n"
      "    AddDescriptors();\n"
      "  }\n"
      "} static_descriptor_initializer;\n");
  if (!StaticInitializersForced(file_, options_)) {
    printer->Print("#endif  // GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER\n");
  }
}

void FileGenerator::GenerateNamespaceOpeners(io::Printer* printer) {
  if (package_parts_.size() > 0) printer->Print("\n");

  for (int i = 0; i < package_parts_.size(); i++) {
    printer->Print("namespace $part$ {\n",
                   "part", package_parts_[i]);
  }
}

void FileGenerator::GenerateNamespaceClosers(io::Printer* printer) {
  if (package_parts_.size() > 0) printer->Print("\n");

  for (int i = package_parts_.size() - 1; i >= 0; i--) {
    printer->Print("}  // namespace $part$\n",
                   "part", package_parts_[i]);
  }
}

void FileGenerator::GenerateForwardDeclarations(io::Printer* printer) {
  ForwardDeclarations decls;
  for (int i = 0; i < file_->dependency_count(); i++) {
    FileGenerator dependency(file_->dependency(i), options_);
    dependency.FillForwardDeclarations(&decls);
  }
  FillForwardDeclarations(&decls);
  decls.Print(printer, options_);
}

void FileGenerator::FillForwardDeclarations(ForwardDeclarations* decls) {
  for (int i = 0; i < file_->public_dependency_count(); i++) {
    FileGenerator dependency(file_->public_dependency(i), options_);
    dependency.FillForwardDeclarations(decls);
  }
  for (int i = 0; i < package_parts_.size(); i++) {
    decls = decls->AddOrGetNamespace(package_parts_[i]);
  }
  // Generate enum definitions.
  for (int i = 0; i < enum_generators_.size(); i++) {
    enum_generators_[i]->FillForwardDeclaration(&decls->enums());
  }
  // Generate forward declarations of classes.
  for (int i = 0; i < message_generators_.size(); i++) {
    message_generators_[i]->FillMessageForwardDeclarations(
        &decls->classes());
  }
}

void FileGenerator::GenerateTopHeaderGuard(io::Printer* printer,
                                           const string& filename_identifier) {
  // Generate top of header.
  printer->Print(
      "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
      "// source: $filename$\n"
      "\n"
      "#ifndef PROTOBUF_$filename_identifier$__INCLUDED\n"
      "#define PROTOBUF_$filename_identifier$__INCLUDED\n"
      "\n"
      "#include <string>\n",
      "filename", file_->name(), "filename_identifier", filename_identifier);
  printer->Print("\n");
}

void FileGenerator::GenerateBottomHeaderGuard(
    io::Printer* printer, const string& filename_identifier) {
  printer->Print(
    "#endif  // PROTOBUF_$filename_identifier$__INCLUDED\n",
    "filename_identifier", filename_identifier);
}

void FileGenerator::GenerateLibraryIncludes(io::Printer* printer) {

  printer->Print(
    "#include <google/protobuf/stubs/common.h>\n"
    "\n");

  // Verify the protobuf library header version is compatible with the protoc
  // version before going any further.
  printer->Print(
    "#if GOOGLE_PROTOBUF_VERSION < $min_header_version$\n"
    "#error This file was generated by a newer version of protoc which is\n"
    "#error incompatible with your Protocol Buffer headers.  Please update\n"
    "#error your headers.\n"
    "#endif\n"
    "#if $protoc_version$ < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION\n"
    "#error This file was generated by an older version of protoc which is\n"
    "#error incompatible with your Protocol Buffer headers.  Please\n"
    "#error regenerate this file with a newer version of protoc.\n"
    "#endif\n"
    "\n",
    "min_header_version",
      SimpleItoa(protobuf::internal::kMinHeaderVersionForProtoc),
    "protoc_version", SimpleItoa(GOOGLE_PROTOBUF_VERSION));

  // OK, it's now safe to #include other files.
  printer->Print(
      "#include <google/protobuf/io/coded_stream.h>\n"
      "#include <google/protobuf/arena.h>\n"
      "#include <google/protobuf/arenastring.h>\n"
      "#include <google/protobuf/generated_message_table_driven.h>\n"
      "#include <google/protobuf/generated_message_util.h>\n");

  if (HasDescriptorMethods(file_, options_)) {
    printer->Print(
      "#include <google/protobuf/metadata.h>\n");
  } else {
    printer->Print(
      "#include <google/protobuf/metadata_lite.h>\n");
  }

  if (!message_generators_.empty()) {
    if (HasDescriptorMethods(file_, options_)) {
      printer->Print(
        "#include <google/protobuf/message.h>\n");
    } else {
      printer->Print(
        "#include <google/protobuf/message_lite.h>\n");
    }
  }
  printer->Print(
    "#include <google/protobuf/repeated_field.h>"
    "  // IWYU pragma: export\n"
    "#include <google/protobuf/extension_set.h>"
    "  // IWYU pragma: export\n");
  if (HasMapFields(file_)) {
    printer->Print(
        "#include <google/protobuf/map.h>"
        "  // IWYU pragma: export\n");
    if (HasDescriptorMethods(file_, options_)) {
      printer->Print(
          "#include <google/protobuf/map_field_inl.h>\n");
    } else {
      printer->Print(
          "#include <google/protobuf/map_field_lite.h>\n");
    }
  }

  if (HasEnumDefinitions(file_)) {
    if (HasDescriptorMethods(file_, options_)) {
      printer->Print(
          "#include <google/protobuf/generated_enum_reflection.h>\n");
    } else {
      printer->Print(
          "#include <google/protobuf/generated_enum_util.h>\n");
    }
  }

  if (HasGenericServices(file_, options_)) {
    printer->Print(
      "#include <google/protobuf/service.h>\n");
  }

  if (UseUnknownFieldSet(file_, options_) && !message_generators_.empty()) {
    printer->Print(
      "#include <google/protobuf/unknown_field_set.h>\n");
  }


  if (IsAnyMessage(file_)) {
    printer->Print(
      "#include <google/protobuf/any.h>\n");
  }
}

void FileGenerator::GenerateMetadataPragma(io::Printer* printer,
                                           const string& info_path) {
  if (!info_path.empty() && !options_.annotation_pragma_name.empty() &&
      !options_.annotation_guard_name.empty()) {
    printer->Print(
        "#ifdef $guard$\n"
        "#pragma $pragma$ \"$info_path$\"\n"
        "#endif  // $guard$\n",
        "guard", options_.annotation_guard_name, "pragma",
        options_.annotation_pragma_name, "info_path", info_path);
  }
}

void FileGenerator::GenerateDependencyIncludes(io::Printer* printer) {
  std::set<string> public_import_names;
  for (int i = 0; i < file_->public_dependency_count(); i++) {
    public_import_names.insert(file_->public_dependency(i)->name());
  }

  for (int i = 0; i < file_->dependency_count(); i++) {
    const bool use_system_include = IsWellKnownMessage(file_->dependency(i));
    const string& name = file_->dependency(i)->name();
    bool public_import = (public_import_names.count(name) != 0);


    printer->Print(
      "#include $left$$dependency$.pb.h$right$$iwyu$\n",
      "dependency", StripProto(name),
      "iwyu", (public_import) ? "  // IWYU pragma: export" : "",
      "left", use_system_include ? "<" : "\"",
      "right", use_system_include ? ">" : "\"");
  }
}

void FileGenerator::GenerateGlobalStateFunctionDeclarations(
    io::Printer* printer) {
  // Forward-declare the AddDescriptors, AssignDescriptors, and ShutdownFile
  // functions, so that we can declare them to be friends of each class.
  printer->Print(
      "\n"
      "namespace $file_namespace$ {\n"
      "// Internal implementation detail -- do not call these.\n"
      "struct $dllexport_decl$TableStruct {\n"
      "  static const ::google::protobuf::internal::ParseTableField entries[];\n"
      "  static const ::google::protobuf::internal::AuxillaryParseTableField aux[];\n"
      "  static const ::google::protobuf::internal::ParseTable schema[];\n"
      "  static const ::google::protobuf::uint32 offsets[];\n"
      // The following function(s) need to be able to access private members of
      // the messages defined in the file. So we make them static members.
      // This is the internal implementation of InitDefaults. It should only
      // be called by InitDefaults which makes sure it will be called only once.
      "  static void InitDefaultsImpl();\n"
      "  static void Shutdown();\n"
      "};\n"
      "void $dllexport_decl$AddDescriptors();\n"
      "void $dllexport_decl$InitDefaults();\n"
      "}  // namespace $file_namespace$\n",
      "file_namespace", FileLevelNamespace(file_->name()), "dllexport_decl",
      options_.dllexport_decl.empty() ? "" : options_.dllexport_decl + " ");
}

void FileGenerator::GenerateMessageDefinitions(io::Printer* printer) {
  // Generate class definitions.
  for (int i = 0; i < message_generators_.size(); i++) {
    if (i > 0) {
      printer->Print("\n");
      printer->Print(kThinSeparator);
      printer->Print("\n");
    }
    message_generators_[i]->GenerateClassDefinition(printer);
  }
}

void FileGenerator::GenerateEnumDefinitions(io::Printer* printer) {
  // Generate enum definitions.
  for (int i = 0; i < enum_generators_.size(); i++) {
    enum_generators_[i]->GenerateDefinition(printer);
  }
}

void FileGenerator::GenerateServiceDefinitions(io::Printer* printer) {
  if (HasGenericServices(file_, options_)) {
    // Generate service definitions.
    for (int i = 0; i < service_generators_.size(); i++) {
      if (i > 0) {
        printer->Print("\n");
        printer->Print(kThinSeparator);
        printer->Print("\n");
      }
      service_generators_[i]->GenerateDeclarations(printer);
    }

    printer->Print("\n");
    printer->Print(kThickSeparator);
    printer->Print("\n");
  }
}

void FileGenerator::GenerateExtensionIdentifiers(io::Printer* printer) {
  // Declare extension identifiers. These are in global scope and so only
  // the global scope extensions.
  for (int i = 0; i < file_->extension_count(); i++) {
    extension_generators_owner_[i]->GenerateDeclaration(printer);
  }
}

void FileGenerator::GenerateInlineFunctionDefinitions(io::Printer* printer) {
  // An aside about inline functions in .proto.h mode:
  //
  // The PROTOBUF_INLINE_NOT_IN_HEADERS symbol controls conditionally
  // moving much of the inline functions to the .pb.cc file, which can be a
  // significant performance benefit for compilation time, at the expense
  // of non-inline function calls.
  //
  // However, in .proto.h mode, the definition of the internal dependent
  // base class must remain in the header, and can never be out-lined. The
  // dependent base class also needs access to has-bit manipuation
  // functions, so the has-bit functions must be unconditionally inlined in
  // proto_h mode.
  //
  // This gives us three flavors of functions:
  //
  //  1. Functions on the message not used by the internal dependent base
  //     class: in .proto.h mode, only some functions are defined on the
  //     message class; others are defined on the dependent base class.
  //     These are guarded and can be out-lined. These are generated by
  //     GenerateInlineMethods, and include has_* bit functions in
  //     non-proto_h mode.
  //
  //  2. Functions on the internal dependent base class: these functions
  //     are dependent on a template parameter, so they always need to
  //     remain in the header.
  //
  //  3. Functions on the message that are used by the dependent base: the
  //     dependent base class down casts itself to the message
  //     implementation class to access these functions (the has_* bit
  //     manipulation functions). Unlike #1, these functions must
  //     unconditionally remain in the header. These are emitted by
  //     GenerateDependentInlineMethods, even though they are not actually
  //     dependent.

  printer->Print("#if !PROTOBUF_INLINE_NOT_IN_HEADERS\n");
  // Generate class inline methods.
  for (int i = 0; i < message_generators_.size(); i++) {
    if (i > 0) {
      printer->Print(kThinSeparator);
      printer->Print("\n");
    }
    message_generators_[i]->GenerateInlineMethods(printer,
                                                  /* is_inline = */ true);
  }
  printer->Print("#endif  // !PROTOBUF_INLINE_NOT_IN_HEADERS\n");

  for (int i = 0; i < message_generators_.size(); i++) {
    if (i > 0) {
      printer->Print(kThinSeparator);
      printer->Print("\n");
    }
    // Methods of the dependent base class must always be inline in the header.
    message_generators_[i]->GenerateDependentInlineMethods(printer);
  }
}

void FileGenerator::GenerateProto2NamespaceEnumSpecializations(
    io::Printer* printer) {
  // Emit GetEnumDescriptor specializations into google::protobuf namespace:
  if (HasEnumDefinitions(file_)) {
    // The SWIG conditional is to avoid a null-pointer dereference
    // (bug 1984964) in swig-1.3.21 resulting from the following syntax:
    //   namespace X { void Y<Z::W>(); }
    // which appears in GetEnumDescriptor() specializations.
    printer->Print(
        "\n"
        "#ifndef SWIG\n"
        "namespace google {\nnamespace protobuf {\n"
        "\n");
    for (int i = 0; i < enum_generators_.size(); i++) {
      enum_generators_[i]->GenerateGetEnumDescriptorSpecializations(printer);
    }
    printer->Print(
        "\n"
        "}  // namespace protobuf\n}  // namespace google\n"
        "#endif  // SWIG\n");
  }
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
