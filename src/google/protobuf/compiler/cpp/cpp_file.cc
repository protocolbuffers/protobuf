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

FileGenerator::FileGenerator(const FileDescriptor* file, const Options& options)
    : file_(file),
      options_(options),
      scc_analyzer_(options),
      enum_generators_owner_(
          new google::protobuf::scoped_ptr<EnumGenerator>[file->enum_type_count()]),
      service_generators_owner_(
          new google::protobuf::scoped_ptr<ServiceGenerator>[file->service_count()]),
      extension_generators_owner_(
          new google::protobuf::scoped_ptr<ExtensionGenerator>[file->extension_count()]) {
  std::vector<const Descriptor*> msgs = FlattenMessagesInFile(file);
  for (int i = 0; i < msgs.size(); i++) {
    // Deleted in destructor
    MessageGenerator* msg_gen =
        new MessageGenerator(msgs[i], i, options, &scc_analyzer_);
    message_generators_.push_back(msg_gen);
    msg_gen->AddGenerators(&enum_generators_, &extension_generators_);
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
  if (HasGenericServices(file_, options_)) {
    for (int i = 0; i < service_generators_.size(); i++) {
      service_generators_[i]->index_in_metadata_ = i;
    }
  }

  for (int i = 0; i < file->extension_count(); i++) {
    extension_generators_owner_[i].reset(
        new ExtensionGenerator(file->extension(i), options));
    extension_generators_.push_back(extension_generators_owner_[i].get());
  }


  package_parts_ = Split(file_->package(), ".", true);
}

FileGenerator::~FileGenerator() {
  for (int i = 0; i < message_generators_.size(); i++) {
    delete message_generators_[i];
  }
}

void FileGenerator::GenerateMacroUndefs(io::Printer* printer) {
  // Only do this for protobuf's own types. There are some google3 protos using
  // macros as field names and the generated code compiles after the macro
  // expansion. Undefing these macros actually breaks such code.
  if (file_->name() != "google/protobuf/compiler/plugin.proto") {
    return;
  }
  std::vector<string> names_to_undef;
  std::vector<const FieldDescriptor*> fields;
  ListAllFields(file_, &fields);
  for (int i = 0; i < fields.size(); i++) {
    const string& name = fields[i]->name();
    static const char* kMacroNames[] = {"major", "minor"};
    for (int i = 0; i < GOOGLE_ARRAYSIZE(kMacroNames); ++i) {
      if (name == kMacroNames[i]) {
        names_to_undef.push_back(name);
        break;
      }
    }
  }
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

  GenerateGlobalStateFunctionDeclarations(printer);

  GenerateForwardDeclarations(printer);

  {
    NamespaceOpener ns(Namespace(file_), printer);

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
  }

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
    {
      NamespaceOpener ns(Namespace(file_), printer);
      printer->Print(
        "\n"
        "// @@protoc_insertion_point(namespace_scope)\n");
    }
    printer->Print(
      "\n"
      "// @@protoc_insertion_point(global_scope)\n"
      "\n");
  }

  GenerateBottomHeaderGuard(printer, filename_identifier);
}

void FileGenerator::GenerateSourceIncludes(io::Printer* printer) {
  const bool use_system_include = IsWellKnownMessage(file_);
  string header =
      StripProto(file_->name()) + (options_.proto_h ? ".proto.h" : ".pb.h");
  printer->Print(
    "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
    "// source: $filename$\n"
    "\n"
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

  // TODO(gerbens) Remove this when all code in google is using the same
  // proto library. This is a temporary hack to force build errors if
  // the proto library is compiled with GOOGLE_PROTOBUF_ENFORCE_UNIQUENESS
  // and is also linking internal proto2. This is to prevent regressions while
  // we work cleaning up the code base. After this is completed and we have
  // one proto lib all code uses this should be removed.
  printer->Print(
    "// This is a temporary google only hack\n"
    "#ifdef GOOGLE_PROTOBUF_ENFORCE_UNIQUENESS\n"
    "#include \"third_party/protobuf/version.h\"\n"
    "#endif\n");

  printer->Print(
    "// @@protoc_insertion_point(includes)\n");
}

void FileGenerator::GenerateSourceDefaultInstance(int idx,
                                                  io::Printer* printer) {
  printer->Print(
      "class $classname$DefaultTypeInternal {\n"
      " public:\n"
      "  ::google::protobuf::internal::ExplicitlyConstructed<$classname$>\n"
      "      _instance;\n",
      "classname", message_generators_[idx]->classname_);
  printer->Indent();
  message_generators_[idx]->GenerateExtraDefaultFields(printer);
  printer->Outdent();
  printer->Print("} _$classname$_default_instance_;\n", "classname",
                 message_generators_[idx]->classname_);
}

namespace {

// Generates weak symbol declarations for types that are to be considered weakly
// referenced.
void GenerateWeakDeclarations(
    const FileDescriptor* file, const Options& options,
    SCCAnalyzer* scc_analyzer,
    io::Printer* printer) {
  std::vector<const FieldDescriptor*> fields;
  ListAllFields(file, &fields);

  // To ensure determinism and minimize the number of namespace statements,
  // we output the forward declarations sorted on namespace and type / function
  // name.
  std::set<std::pair<string, string> > messages;
  std::set<std::pair<string, string> > inits;
  for (int i = 0; i < fields.size(); ++i) {
    const FieldDescriptor* field = fields[i];
    bool is_weak = IsImplicitWeakField(field, options);
    if (is_weak) {
      const Descriptor* msg = field->message_type();
      string flns = FileLevelNamespace(msg);
      string repr = ClassName(scc_analyzer->GetSCC(msg)->GetRepresentative());
      inits.insert(std::make_pair(flns, "InitDefaults" + repr));
      inits.insert(std::make_pair(flns, "AddDescriptors"));
      messages.insert(std::make_pair(Namespace(msg), ClassName(msg)));
    }
  }

  if (messages.empty()) {
    return;
  }

  printer->Print("\n");
  NamespaceOpener ns(printer);
  for (std::set<std::pair<string, string> >::const_iterator it =
           messages.begin();
       it != messages.end(); ++it) {
    ns.ChangeTo(it->first);
    printer->Print(
        "extern __attribute__((weak)) $classname$DefaultTypeInternal "
        "_$classname$_default_instance_;\n",
        "classname", it->second);
  }
  for (std::set<std::pair<string, string> >::const_iterator it = inits.begin();
       it != inits.end(); ++it) {
    ns.ChangeTo(it->first);
    printer->Print("void $name$() __attribute__((weak));\n",
                   "name", it->second);
  }
}

}  // namespace

void FileGenerator::GenerateSourceForMessage(int idx, io::Printer* printer) {
  GenerateSourceIncludes(printer);
  GenerateWeakDeclarations(file_, options_, &scc_analyzer_, printer);

  {  // package namespace
    NamespaceOpener ns(Namespace(file_), printer);

    // Define default instances
    GenerateSourceDefaultInstance(idx, printer);
    if (UsingImplicitWeakFields(file_, options_)) {
      printer->Print("void $classname$_ReferenceStrong() {}\n", "classname",
                     message_generators_[idx]->classname_);
    }

    // Generate classes.
    printer->Print("\n");
    message_generators_[idx]->GenerateClassMethods(printer);

    printer->Print(
        "\n"
        "// @@protoc_insertion_point(namespace_scope)\n");
  }  // end package namespace

  if (IsSCCRepresentative(message_generators_[idx]->descriptor_)) {
    NamespaceOpener ns(FileLevelNamespace(file_), printer);
    GenerateInitForSCC(GetSCC(message_generators_[idx]->descriptor_), printer);
  }

  printer->Print(
      "\n"
      "// @@protoc_insertion_point(global_scope)\n");
}

void FileGenerator::GenerateGlobalSource(io::Printer* printer) {
  GenerateSourceIncludes(printer);
  GenerateWeakDeclarations(file_, options_, &scc_analyzer_, printer);

  // TODO(gerbens) Generate tables here

  // Define the code to initialize reflection. This code uses a global
  // constructor to register reflection data with the runtime pre-main.
  if (HasDescriptorMethods(file_, options_)) {
    NamespaceOpener ns(FileLevelNamespace(file_), printer);
    GenerateReflectionInitializationCode(printer);
  }

  NamespaceOpener ns(Namespace(file_), printer);

  // Generate enums.
  for (int i = 0; i < enum_generators_.size(); i++) {
    enum_generators_[i]->GenerateMethods(i, printer);
  }

  // Define extensions.
  for (int i = 0; i < extension_generators_.size(); i++) {
    extension_generators_[i]->GenerateDefinition(printer);
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
}

void FileGenerator::GenerateSource(io::Printer* printer) {
  GenerateSourceIncludes(printer);
  GenerateWeakDeclarations(file_, options_, &scc_analyzer_, printer);

  {
    NamespaceOpener ns(Namespace(file_), printer);

    // Define default instances
    for (int i = 0; i < message_generators_.size(); i++) {
      GenerateSourceDefaultInstance(i, printer);
      if (UsingImplicitWeakFields(file_, options_)) {
        printer->Print("void $classname$_ReferenceStrong() {}\n", "classname",
                       message_generators_[i]->classname_);
      }
    }
  }

  {
    NamespaceOpener ns(FileLevelNamespace(file_), printer);
    // Define the initialization code to initialize the default instances.
    // This code doesn't use a global constructor.
    GenerateInitializationCode(printer);

    // Define the code to initialize reflection. This code uses a global
    // constructor to register reflection data with the runtime pre-main.
    if (HasDescriptorMethods(file_, options_)) {
      GenerateReflectionInitializationCode(printer);
    }
  }


  {
    NamespaceOpener ns(Namespace(file_), printer);

    // Actually implement the protos

    // Generate enums.
    for (int i = 0; i < enum_generators_.size(); i++) {
      enum_generators_[i]->GenerateMethods(i, printer);
    }

    // Generate classes.
    for (int i = 0; i < message_generators_.size(); i++) {
      printer->Print("\n");
      printer->Print(kThickSeparator);
      printer->Print("\n");
      message_generators_[i]->GenerateClassMethods(printer);
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
  }
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
      if (options.lite_implicit_weak_fields) {
        printer->Print("void $classname$_ReferenceStrong();\n",
                       "classname", it->first);
      }
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

void FileGenerator::GenerateReflectionInitializationCode(io::Printer* printer) {
  // AddDescriptors() is a file-level procedure which adds the encoded
  // FileDescriptorProto for this .proto file to the global DescriptorPool for
  // generated files (DescriptorPool::generated_pool()). It ordinarily runs at
  // static initialization time, but is not used at all in LITE_RUNTIME mode.
  //
  // Its sibling, AssignDescriptors(), actually pulls the compiled
  // FileDescriptor from the DescriptorPool and uses it to populate all of
  // the global variables which store pointers to the descriptor objects.
  // It also constructs the reflection objects.  It is called the first time
  // anyone calls descriptor() or GetReflection() on one of the types defined
  // in the file.

  if (!message_generators_.empty()) {
    printer->Print("::google::protobuf::Metadata file_level_metadata[$size$];\n", "size",
                   SimpleItoa(message_generators_.size()));
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

  if (!message_generators_.empty()) {
    printer->Print(
        "\n"
        "const ::google::protobuf::uint32 TableStruct::offsets[] "
        "GOOGLE_PROTOBUF_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {\n");
    printer->Indent();
    std::vector<std::pair<size_t, size_t> > pairs;
    pairs.reserve(message_generators_.size());
    for (int i = 0; i < message_generators_.size(); i++) {
      pairs.push_back(message_generators_[i]->GenerateOffsets(printer));
    }
    printer->Outdent();
    printer->Print(
        "};\n"
        "static const ::google::protobuf::internal::MigrationSchema schemas[] "
        "GOOGLE_PROTOBUF_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {\n");
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
          "::google::protobuf::Message*>(&$ns$::_$classname$_default_instance_),\n",
          "classname", ClassName(descriptor), "ns", Namespace(descriptor));
    }
    printer->Outdent();
    printer->Print(
        "};\n"
        "\n");
  } else {
    // we still need these symbols to exist
    printer->Print(
        // MSVC doesn't like empty arrays, so we add a dummy.
        "const ::google::protobuf::uint32 TableStruct::offsets[1] = {};\n"
        "static const ::google::protobuf::internal::MigrationSchema* schemas = NULL;\n"
        "static const ::google::protobuf::Message* const* "
        "file_default_instances = NULL;\n"
        "\n");
  }

  // ---------------------------------------------------------------

  // protobuf_AssignDescriptorsOnce():  The first time it is called, calls
  // AssignDescriptors().  All later times, waits for the first call to
  // complete and then returns.
  string message_factory = "NULL";
    printer->Print(
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
        "void protobuf_RegisterTypes(const ::std::string&) "
        "GOOGLE_PROTOBUF_ATTRIBUTE_COLD;\n"
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
        "\n");

    // Now generate the AddDescriptors() function.
    printer->Print(
        "void AddDescriptorsImpl() {\n"
        "  InitDefaults();\n");
    printer->Indent();

    // Embed the descriptor.  We simply serialize the entire
    // FileDescriptorProto
    // and embed it as a string literal, which is parsed and built into real
    // descriptors at initialization time.
    FileDescriptorProto file_proto;
    file_->CopyTo(&file_proto);
    string file_data;
    file_proto.SerializeToString(&file_data);

    printer->Print("static const char descriptor[] "
                   "GOOGLE_PROTOBUF_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) "
                   "= {\n");
    printer->Indent();

    if (file_data.size() > 65535) {
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

  // Call the AddDescriptors() methods for all of our dependencies, to make
  // sure they get added first.
  for (int i = 0; i < file_->dependency_count(); i++) {
    const FileDescriptor* dependency = file_->dependency(i);
    // Print the namespace prefix for the dependency.
    string file_namespace = FileLevelNamespace(dependency);
    // Call its AddDescriptors function.
    printer->Print("::$file_namespace$::AddDescriptors();\n", "file_namespace",
                   file_namespace);
  }

  printer->Outdent();
  printer->Print(
      "}\n"
      "\n"
      "void AddDescriptors() {\n"
      "  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);\n"
      "  ::google::protobuf::GoogleOnceInit(&once, &AddDescriptorsImpl);\n"
      "}\n");

    printer->Print(
        "// Force AddDescriptors() to be called at dynamic initialization "
        "time.\n"
        "struct StaticDescriptorInitializer {\n"
        "  StaticDescriptorInitializer() {\n"
        "    AddDescriptors();\n"
        "  }\n"
        "} static_descriptor_initializer;\n");
}

void FileGenerator::GenerateInitForSCC(const SCC* scc, io::Printer* printer) {
  const string scc_name = ClassName(scc->GetRepresentative());
  printer->Print(
      "void InitDefaults$scc_name$Impl() {\n"
      "  GOOGLE_PROTOBUF_VERIFY_VERSION;\n\n"
      "#ifdef GOOGLE_PROTOBUF_ENFORCE_UNIQUENESS\n"
      "  ::google::protobuf::internal::InitProtobufDefaultsForceUnique();\n"
      "#else\n"
      "  ::google::protobuf::internal::InitProtobufDefaults();\n"
      "#endif  // GOOGLE_PROTOBUF_ENFORCE_UNIQUENESS\n",
        // Force initialization of primitive values we depend on.
      "scc_name", scc_name);

  printer->Indent();

  // Call the InitDefaults() methods for all of our dependencies, to make
  // sure they get added first.
  for (int i = 0; i < scc->children.size(); i++) {
    const SCC* child_scc = scc->children[i];
    const FileDescriptor* dependency = child_scc->GetRepresentative()->file();
    // Print the namespace prefix for the dependency.
    string file_namespace = FileLevelNamespace(dependency);
    std::map<string, string> variables;
    variables["file_namespace"] = file_namespace;
    variables["scc_name"] = ClassName(child_scc->GetRepresentative(), false);
    bool using_weak_fields = UsingImplicitWeakFields(file_, options_);
    if (using_weak_fields) {
      // We're building for lite with implicit weak fields, so we need to handle
      // the possibility that this InitDefaults function is not linked into the
      // binary. Some of these might actually be guaranteed to be non-null since
      // we might have a strong reference to the dependency (via a required
      // field, for example), but it's simplest to just assume that any of them
      // could be null.
      printer->Print(
          variables,
          "if (&$file_namespace$::InitDefaults$scc_name$ != NULL) {\n"
          "  $file_namespace$::InitDefaults$scc_name$();\n"
          "}\n");
    } else {
      printer->Print(variables,
                     "$file_namespace$::InitDefaults$scc_name$();\n");
    }
  }

  // First construct all the necessary default instances.
  for (int i = 0; i < message_generators_.size(); i++) {
    if (scc_analyzer_.GetSCC(message_generators_[i]->descriptor_) != scc) {
      continue;
    }
    // TODO(gerbens) This requires this function to be friend. Remove
    // the need for this.
    message_generators_[i]->GenerateFieldDefaultInstances(printer);
    printer->Print(
        "{\n"
        "  void* ptr = &$ns$::_$classname$_default_instance_;\n"
        "  new (ptr) $ns$::$classname$();\n",
        "ns", Namespace(message_generators_[i]->descriptor_),
        "classname", ClassName(message_generators_[i]->descriptor_));
    if (!IsMapEntryMessage(message_generators_[i]->descriptor_)) {
      printer->Print(
          "  ::google::protobuf::internal::OnShutdownDestroyMessage(ptr);\n");
    }
    printer->Print("}\n");
  }

  // TODO(gerbens) make default instances be the same as normal instances.
  // Default instances differ from normal instances because they have cross
  // linked message fields.
  for (int i = 0; i < message_generators_.size(); i++) {
    if (scc_analyzer_.GetSCC(message_generators_[i]->descriptor_) != scc) {
      continue;
    }
    printer->Print("$classname$::InitAsDefaultInstance();\n", "classname",
                   QualifiedClassName(message_generators_[i]->descriptor_));
  }
  printer->Outdent();
  printer->Print("}\n\n");
  printer->Print(
      "void InitDefaults$scc_name$() {\n"
      "  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);\n"
      "  ::google::protobuf::GoogleOnceInit(&once, "
      "&InitDefaults$scc_name$Impl);\n"
      "}\n\n",
      "scc_name", scc_name);
}

void FileGenerator::GenerateInitializationCode(io::Printer* printer) {
  // Messages depend on the existence of a default instance, which has to
  // initialized properly. The default instances are allocated in the data
  // segment, but we can't quite allocate the type directly. The destructors
  // cannot run at program exit as this could lead to segfaults in a threaded
  // environment. Hence these instances must be inplace constructed at first
  // use.

  if (options_.table_driven_parsing) {
    // TODO(ckennelly): Gate this with the same options flag to enable
    // table-driven parsing.
    printer->Print(
        "PROTOBUF_CONSTEXPR_VAR ::google::protobuf::internal::ParseTableField\n"
        "    const TableStruct::entries[] "
        "GOOGLE_PROTOBUF_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {\n");
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
        "    const TableStruct::aux[] "
        "GOOGLE_PROTOBUF_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {\n");
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
        "    TableStruct::schema[] "
        "GOOGLE_PROTOBUF_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {\n");
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
  }

  if (!message_generators_.empty() && options_.table_driven_serialization) {
    printer->Print(
        "const ::google::protobuf::internal::FieldMetadata TableStruct::field_metadata[] "
        "= {\n");
    printer->Indent();
    std::vector<int> field_metadata_offsets;
    int idx = 0;
    for (int i = 0; i < message_generators_.size(); i++) {
      field_metadata_offsets.push_back(idx);
      idx += message_generators_[i]->GenerateFieldMetadata(printer);
    }
    field_metadata_offsets.push_back(idx);
    printer->Outdent();
    printer->Print(
        "};\n"
        "const ::google::protobuf::internal::SerializationTable "
        "TableStruct::serialization_table[] = {\n");
    printer->Indent();
    // We rely on the order we layout the tables to match the order we
    // calculate them with FlattenMessagesInFile, so we check here that
    // these match exactly.
    std::vector<const Descriptor*> calculated_order =
        FlattenMessagesInFile(file_);
    GOOGLE_CHECK_EQ(calculated_order.size(), message_generators_.size());
    for (int i = 0; i < message_generators_.size(); i++) {
      GOOGLE_CHECK_EQ(calculated_order[i], message_generators_[i]->descriptor_);
      printer->Print(
          "{$num_fields$, TableStruct::field_metadata + $index$},\n",
          "classname", message_generators_[i]->classname_, "num_fields",
          SimpleItoa(field_metadata_offsets[i + 1] - field_metadata_offsets[i]),
          "index", SimpleItoa(field_metadata_offsets[i]));
    }
    printer->Outdent();
    printer->Print(
        "};\n"
        "\n");
  }

  // -----------------------------------------------------------------
  // All functionality that need private access.

  // Now generate the InitDefaults for each SCC.
  for (int i = 0; i < message_generators_.size(); i++) {
    if (IsSCCRepresentative(message_generators_[i]->descriptor_)) {
      GenerateInitForSCC(GetSCC(message_generators_[i]->descriptor_), printer);
    }
  }
}

void FileGenerator::GenerateForwardDeclarations(io::Printer* printer) {
  ForwardDeclarations decls;
  FillForwardDeclarations(&decls);
  decls.Print(printer, options_);
}

void FileGenerator::FillForwardDeclarations(ForwardDeclarations* decls) {
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
  if (UsingImplicitWeakFields(file_, options_)) {
    printer->Print("#include <google/protobuf/implicit_weak_message.h>\n");
  }

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
      printer->Print("#include <google/protobuf/map_entry.h>\n");
      printer->Print("#include <google/protobuf/map_field_inl.h>\n");
    } else {
      printer->Print("#include <google/protobuf/map_entry_lite.h>\n");
      printer->Print("#include <google/protobuf/map_field_lite.h>\n");
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
// Forward-declare the AddDescriptors, InitDefaults because these are called
// by .pb.cc files depending on this file.
  printer->Print(
      "\n"
      "namespace $file_namespace$ {\n"
      "// Internal implementation detail -- do not use these members.\n"
      "struct $dllexport_decl$TableStruct {\n"
      // These tables describe how to serialize and parse messages. Used
      // for table driven code.
      "  static const ::google::protobuf::internal::ParseTableField entries[];\n"
      "  static const ::google::protobuf::internal::AuxillaryParseTableField aux[];\n"
      "  static const ::google::protobuf::internal::ParseTable schema[$num$];\n"
      "  static const ::google::protobuf::internal::FieldMetadata field_metadata[];\n"
      "  static const ::google::protobuf::internal::SerializationTable "
      "serialization_table[];\n"
      "  static const ::google::protobuf::uint32 offsets[];\n"
      "};\n",
      "file_namespace", FileLevelNamespace(file_), "dllexport_decl",
      options_.dllexport_decl.empty() ? "" : options_.dllexport_decl + " ",
      "num", SimpleItoa(std::max(size_t(1), message_generators_.size())));
  if (HasDescriptorMethods(file_, options_)) {
    printer->Print(
        "void $dllexport_decl$AddDescriptors();\n", "dllexport_decl",
        options_.dllexport_decl.empty() ? "" : options_.dllexport_decl + " ");
  }
  for (int i = 0; i < message_generators_.size(); i++) {
    if (!IsSCCRepresentative(message_generators_[i]->descriptor_)) continue;
    string scc_name = ClassName(message_generators_[i]->descriptor_);
    // TODO(gerbens) Remove the Impl from header. This is solely because
    // it currently still needs to be a friend of the protos.
    printer->Print(
        "void $dllexport_decl$InitDefaults$scc_name$Impl();\n"
        "void $dllexport_decl$InitDefaults$scc_name$();\n",
        "scc_name", scc_name, "dllexport_decl",
        options_.dllexport_decl.empty() ? "" : options_.dllexport_decl + " ");
  }
  // TODO(gerbens) This is for proto1 interoperability. Remove when proto1
  // is gone.
  printer->Print(
      "inline void $dllexport_decl$InitDefaults() {\n", "dllexport_decl",
      options_.dllexport_decl.empty() ? "" : options_.dllexport_decl + " ");
  for (int i = 0; i < message_generators_.size(); i++) {
    if (!IsSCCRepresentative(message_generators_[i]->descriptor_)) continue;
    string scc_name = ClassName(message_generators_[i]->descriptor_);
    printer->Print("  InitDefaults$scc_name$();\n", "scc_name", scc_name);
  }
  printer->Print("}\n");
  printer->Print(
      "}  // namespace $file_namespace$\n",
      "file_namespace", FileLevelNamespace(file_));
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
  // TODO(gerbens) remove pragmas when gcc is no longer used. Current version
  // of gcc fires a bogus error when compiled with strict-aliasing.
  printer->Print(
    "#ifdef __GNUC__\n"
    "  #pragma GCC diagnostic push\n"
    "  #pragma GCC diagnostic ignored \"-Wstrict-aliasing\"\n"
    "#endif  // __GNUC__\n");
  // Generate class inline methods.
  for (int i = 0; i < message_generators_.size(); i++) {
    if (i > 0) {
      printer->Print(kThinSeparator);
      printer->Print("\n");
    }
    message_generators_[i]->GenerateInlineMethods(printer);
  }
  printer->Print(
    "#ifdef __GNUC__\n"
    "  #pragma GCC diagnostic pop\n"
    "#endif  // __GNUC__\n");

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
    printer->Print(
        "\n"
        "namespace google {\nnamespace protobuf {\n"
        "\n");
    for (int i = 0; i < enum_generators_.size(); i++) {
      enum_generators_[i]->GenerateGetEnumDescriptorSpecializations(printer);
    }
    printer->Print(
        "\n"
        "}  // namespace protobuf\n}  // namespace google\n");
  }
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
