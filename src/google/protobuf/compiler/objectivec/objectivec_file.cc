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

#include <google/protobuf/compiler/objectivec/objectivec_file.h>
#include <google/protobuf/compiler/objectivec/objectivec_enum.h>
#include <google/protobuf/compiler/objectivec/objectivec_extension.h>
#include <google/protobuf/compiler/objectivec/objectivec_message.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/stubs/stl_util.h>
#include <google/protobuf/stubs/strutil.h>
#include <iostream>
#include <sstream>

// NOTE: src/google/protobuf/compiler/plugin.cc makes use of cerr for some
// error cases, so it seems to be ok to use as a back door for errors.

namespace google {
namespace protobuf {

// This is also found in GPBBootstrap.h, and needs to be kept in sync.  It
// is the version check done to ensure generated code works with the current
// runtime being used.
const int32 GOOGLE_PROTOBUF_OBJC_GEN_VERSION = 30001;

namespace compiler {
namespace objectivec {

namespace {

class ImportWriter {
 public:
  ImportWriter(const Options& options)
      : options_(options),
        need_to_parse_mapping_file_(true) {}

  void AddFile(const FileGenerator* file);
  void Print(io::Printer *printer) const;

 private:
  class ProtoFrameworkCollector : public LineConsumer {
   public:
    ProtoFrameworkCollector(map<string, string>* inout_proto_file_to_framework_name)
        : map_(inout_proto_file_to_framework_name) {}

    virtual bool ConsumeLine(const StringPiece& line, string* out_error);

   private:
    map<string, string>* map_;
  };

  void ParseFrameworkMappings();

  const Options options_;
  map<string, string> proto_file_to_framework_name_;
  bool need_to_parse_mapping_file_;

  vector<string> protobuf_framework_imports_;
  vector<string> protobuf_non_framework_imports_;
  vector<string> other_framework_imports_;
  vector<string> other_imports_;
};

void ImportWriter::AddFile(const FileGenerator* file) {
  const FileDescriptor* file_descriptor = file->Descriptor();
  const string extension(".pbobjc.h");

  if (IsProtobufLibraryBundledProtoFile(file_descriptor)) {
    protobuf_framework_imports_.push_back(
        FilePathBasename(file_descriptor) + extension);
    protobuf_non_framework_imports_.push_back(file->Path() + extension);
    return;
  }

  // Lazy parse any mappings.
  if (need_to_parse_mapping_file_) {
    ParseFrameworkMappings();
  }

  map<string, string>::iterator proto_lookup =
      proto_file_to_framework_name_.find(file_descriptor->name());
  if (proto_lookup != proto_file_to_framework_name_.end()) {
    other_framework_imports_.push_back(
        proto_lookup->second + "/" +
        FilePathBasename(file_descriptor) + extension);
    return;
  }

  if (!options_.generate_for_named_framework.empty()) {
    other_framework_imports_.push_back(
        options_.generate_for_named_framework + "/" +
        FilePathBasename(file_descriptor) + extension);
    return;
  }

  other_imports_.push_back(file->Path() + extension);
}

void ImportWriter::Print(io::Printer* printer) const {
  assert(protobuf_non_framework_imports_.size() ==
         protobuf_framework_imports_.size());

  bool add_blank_line = false;

  if (protobuf_framework_imports_.size() > 0) {
    const string framework_name(ProtobufLibraryFrameworkName);
    const string cpp_symbol(ProtobufFrameworkImportSymbol(framework_name));

    printer->Print(
        "#if $cpp_symbol$\n",
        "cpp_symbol", cpp_symbol);
    for (vector<string>::const_iterator iter = protobuf_framework_imports_.begin();
         iter != protobuf_framework_imports_.end(); ++iter) {
      printer->Print(
          " #import <$framework_name$/$header$>\n",
          "framework_name", framework_name,
          "header", *iter);
    }
    printer->Print(
        "#else\n");
    for (vector<string>::const_iterator iter = protobuf_non_framework_imports_.begin();
         iter != protobuf_non_framework_imports_.end(); ++iter) {
      printer->Print(
          " #import \"$header$\"\n",
          "header", *iter);
    }
    printer->Print(
        "#endif\n");

    add_blank_line = true;
  }

  if (other_framework_imports_.size() > 0) {
    if (add_blank_line) {
      printer->Print("\n");
    }

    for (vector<string>::const_iterator iter = other_framework_imports_.begin();
         iter != other_framework_imports_.end(); ++iter) {
      printer->Print(
          " #import <$header$>\n",
          "header", *iter);
    }

    add_blank_line = true;
  }

  if (other_imports_.size() > 0) {
    if (add_blank_line) {
      printer->Print("\n");
    }

    for (vector<string>::const_iterator iter = other_imports_.begin();
         iter != other_imports_.end(); ++iter) {
      printer->Print(
          " #import \"$header$\"\n",
          "header", *iter);
    }
  }
}

void ImportWriter::ParseFrameworkMappings() {
  need_to_parse_mapping_file_ = false;
  if (options_.named_framework_to_proto_path_mappings_path.empty()) {
    return;  // Nothing to do.
  }

  ProtoFrameworkCollector collector(&proto_file_to_framework_name_);
  string parse_error;
  if (!ParseSimpleFile(options_.named_framework_to_proto_path_mappings_path,
                       &collector, &parse_error)) {
    cerr << "error parsing " << options_.named_framework_to_proto_path_mappings_path
         << " : " << parse_error << endl;
    cerr.flush();
  }
}

bool ImportWriter::ProtoFrameworkCollector::ConsumeLine(
    const StringPiece& line, string* out_error) {
  int offset = line.find(':');
  if (offset == StringPiece::npos) {
    *out_error =
        string("Framework/proto file mapping line without colon sign: '") +
        line.ToString() + "'.";
    return false;
  }
  StringPiece framework_name(line, 0, offset);
  StringPiece proto_file_list(line, offset + 1, line.length() - offset - 1);
  StringPieceTrimWhitespace(&framework_name);

  int start = 0;
  while (start < proto_file_list.length()) {
    offset = proto_file_list.find(',', start);
    if (offset == StringPiece::npos) {
      offset = proto_file_list.length();
    }

    StringPiece proto_file(proto_file_list, start, offset - start);
    StringPieceTrimWhitespace(&proto_file);
    if (proto_file.size() != 0) {
      map<string, string>::iterator existing_entry =
          map_->find(proto_file.ToString());
      if (existing_entry != map_->end()) {
        cerr << "warning: duplicate proto file reference, replacing framework entry for '"
             << proto_file.ToString() << "' with '" << framework_name.ToString()
             << "' (was '" << existing_entry->second << "')." << endl;
        cerr.flush();
      }

      if (proto_file.find(' ') != StringPiece::npos) {
        cerr << "note: framework mapping file had a proto file with a space in, hopefully that isn't a missing comma: '"
             << proto_file.ToString() << "'" << endl;
        cerr.flush();
      }

      (*map_)[proto_file.ToString()] = framework_name.ToString();
    }

    start = offset + 1;
  }

  return true;
}

}  // namespace


FileGenerator::FileGenerator(const FileDescriptor *file, const Options& options)
    : file_(file),
      root_class_name_(FileClassName(file)),
      is_public_dep_(false),
      options_(options) {
  for (int i = 0; i < file_->enum_type_count(); i++) {
    EnumGenerator *generator = new EnumGenerator(file_->enum_type(i));
    enum_generators_.push_back(generator);
  }
  for (int i = 0; i < file_->message_type_count(); i++) {
    MessageGenerator *generator =
        new MessageGenerator(root_class_name_, file_->message_type(i), options_);
    message_generators_.push_back(generator);
  }
  for (int i = 0; i < file_->extension_count(); i++) {
    ExtensionGenerator *generator =
        new ExtensionGenerator(root_class_name_, file_->extension(i));
    extension_generators_.push_back(generator);
  }
}

FileGenerator::~FileGenerator() {
  STLDeleteContainerPointers(dependency_generators_.begin(),
                             dependency_generators_.end());
  STLDeleteContainerPointers(enum_generators_.begin(), enum_generators_.end());
  STLDeleteContainerPointers(message_generators_.begin(),
                             message_generators_.end());
  STLDeleteContainerPointers(extension_generators_.begin(),
                             extension_generators_.end());
}

void FileGenerator::GenerateHeader(io::Printer *printer) {
  PrintFileRuntimePreamble(printer, "GPBProtocolBuffers.h");

  // Add some verification that the generated code matches the source the
  // code is being compiled with.
  printer->Print(
      "#if GOOGLE_PROTOBUF_OBJC_GEN_VERSION != $protoc_gen_objc_version$\n"
      "#error This file was generated by a different version of protoc which is incompatible with your Protocol Buffer library sources.\n"
      "#endif\n"
      "\n",
      "protoc_gen_objc_version",
      SimpleItoa(GOOGLE_PROTOBUF_OBJC_GEN_VERSION));

  // #import any headers for "public imports" in the proto file.
  {
    ImportWriter import_writer(options_);
    const vector<FileGenerator *> &dependency_generators = DependencyGenerators();
    for (vector<FileGenerator *>::const_iterator iter =
             dependency_generators.begin();
         iter != dependency_generators.end(); ++iter) {
      if ((*iter)->IsPublicDependency()) {
        import_writer.AddFile(*iter);
      }
    }
    import_writer.Print(printer);
  }

  // Note:
  //  deprecated-declarations suppression is only needed if some place in this
  //    proto file is something deprecated or if it references something from
  //    another file that is deprecated.
  printer->Print(
      "// @@protoc_insertion_point(imports)\n"
      "\n"
      "#pragma clang diagnostic push\n"
      "#pragma clang diagnostic ignored \"-Wdeprecated-declarations\"\n"
      "\n"
      "CF_EXTERN_C_BEGIN\n"
      "\n");

  set<string> fwd_decls;
  for (vector<MessageGenerator *>::iterator iter = message_generators_.begin();
       iter != message_generators_.end(); ++iter) {
    (*iter)->DetermineForwardDeclarations(&fwd_decls);
  }
  for (set<string>::const_iterator i(fwd_decls.begin());
       i != fwd_decls.end(); ++i) {
    printer->Print("$value$;\n", "value", *i);
  }
  if (fwd_decls.begin() != fwd_decls.end()) {
    printer->Print("\n");
  }

  printer->Print(
      "NS_ASSUME_NONNULL_BEGIN\n"
      "\n");

  // need to write out all enums first
  for (vector<EnumGenerator *>::iterator iter = enum_generators_.begin();
       iter != enum_generators_.end(); ++iter) {
    (*iter)->GenerateHeader(printer);
  }

  for (vector<MessageGenerator *>::iterator iter = message_generators_.begin();
       iter != message_generators_.end(); ++iter) {
    (*iter)->GenerateEnumHeader(printer);
  }

  // For extensions to chain together, the Root gets created even if there
  // are no extensions.
  printer->Print(
      "#pragma mark - $root_class_name$\n"
      "\n"
      "/**\n"
      " * Exposes the extension registry for this file.\n"
      " *\n"
      " * The base class provides:\n"
      " * @code\n"
      " *   + (GPBExtensionRegistry *)extensionRegistry;\n"
      " * @endcode\n"
      " * which is a @c GPBExtensionRegistry that includes all the extensions defined by\n"
      " * this file and all files that it depends on.\n"
      " **/\n"
      "@interface $root_class_name$ : GPBRootObject\n"
      "@end\n"
      "\n",
      "root_class_name", root_class_name_);

  if (extension_generators_.size() > 0) {
    // The dynamic methods block is only needed if there are extensions.
    printer->Print(
        "@interface $root_class_name$ (DynamicMethods)\n",
        "root_class_name", root_class_name_);

    for (vector<ExtensionGenerator *>::iterator iter =
             extension_generators_.begin();
         iter != extension_generators_.end(); ++iter) {
      (*iter)->GenerateMembersHeader(printer);
    }

    printer->Print("@end\n\n");
  }  // extension_generators_.size() > 0

  for (vector<MessageGenerator *>::iterator iter = message_generators_.begin();
       iter != message_generators_.end(); ++iter) {
    (*iter)->GenerateMessageHeader(printer);
  }

  printer->Print(
      "NS_ASSUME_NONNULL_END\n"
      "\n"
      "CF_EXTERN_C_END\n"
      "\n"
      "#pragma clang diagnostic pop\n"
      "\n"
      "// @@protoc_insertion_point(global_scope)\n");
}

void FileGenerator::GenerateSource(io::Printer *printer) {
  // #import the runtime support.
  PrintFileRuntimePreamble(printer, "GPBProtocolBuffers_RuntimeSupport.h");

  {
    ImportWriter import_writer(options_);

    // #import the header for this proto file.
    import_writer.AddFile(this);

    // #import the headers for anything that a plain dependency of this proto
    // file (that means they were just an include, not a "public" include).
    const vector<FileGenerator *> &dependency_generators =
        DependencyGenerators();
    for (vector<FileGenerator *>::const_iterator iter =
             dependency_generators.begin();
         iter != dependency_generators.end(); ++iter) {
      if (!(*iter)->IsPublicDependency()) {
        import_writer.AddFile(*iter);
      }
    }

    import_writer.Print(printer);
  }

  bool includes_oneof = false;
  for (vector<MessageGenerator *>::iterator iter = message_generators_.begin();
       iter != message_generators_.end(); ++iter) {
    if ((*iter)->IncludesOneOfDefinition()) {
      includes_oneof = true;
      break;
    }
  }

  // Note:
  //  deprecated-declarations suppression is only needed if some place in this
  //    proto file is something deprecated or if it references something from
  //    another file that is deprecated.
  printer->Print(
      "// @@protoc_insertion_point(imports)\n"
      "\n"
      "#pragma clang diagnostic push\n"
      "#pragma clang diagnostic ignored \"-Wdeprecated-declarations\"\n");
  if (includes_oneof) {
    // The generated code for oneof's uses direct ivar access, suppress the
    // warning incase developer turn that on in the context they compile the
    // generated code.
    printer->Print(
        "#pragma clang diagnostic ignored \"-Wdirect-ivar-access\"\n");
  }

  printer->Print(
      "\n"
      "#pragma mark - $root_class_name$\n"
      "\n"
      "@implementation $root_class_name$\n\n",
      "root_class_name", root_class_name_);

  // Generate the extension initialization structures for the top level and
  // any nested messages.
  ostringstream extensions_stringstream;
  if (file_->extension_count() + file_->message_type_count() > 0) {
    io::OstreamOutputStream extensions_outputstream(&extensions_stringstream);
    io::Printer extensions_printer(&extensions_outputstream, '$');
    for (vector<ExtensionGenerator *>::iterator iter =
             extension_generators_.begin();
         iter != extension_generators_.end(); ++iter) {
      (*iter)->GenerateStaticVariablesInitialization(&extensions_printer);
    }
    for (vector<MessageGenerator *>::iterator iter =
             message_generators_.begin();
         iter != message_generators_.end(); ++iter) {
      (*iter)->GenerateStaticVariablesInitialization(&extensions_printer);
    }
    extensions_stringstream.flush();
  }

  // If there were any extensions or this file has any dependencies, output
  // a registry to override to create the file specific registry.
  const string& extensions_str = extensions_stringstream.str();
  if (extensions_str.length() > 0 || file_->dependency_count() > 0) {
    printer->Print(
        "+ (GPBExtensionRegistry*)extensionRegistry {\n"
        "  // This is called by +initialize so there is no need to worry\n"
        "  // about thread safety and initialization of registry.\n"
        "  static GPBExtensionRegistry* registry = nil;\n"
        "  if (!registry) {\n"
        "    GPBDebugCheckRuntimeVersion();\n"
        "    registry = [[GPBExtensionRegistry alloc] init];\n");

    printer->Indent();
    printer->Indent();

    if (extensions_str.length() > 0) {
      printer->Print(
          "static GPBExtensionDescription descriptions[] = {\n");
      printer->Indent();
      printer->Print(extensions_str.c_str());
      printer->Outdent();
      printer->Print(
          "};\n"
          "for (size_t i = 0; i < sizeof(descriptions) / sizeof(descriptions[0]); ++i) {\n"
          "  GPBExtensionDescriptor *extension =\n"
          "      [[GPBExtensionDescriptor alloc] initWithExtensionDescription:&descriptions[i]];\n"
          "  [registry addExtension:extension];\n"
          "  [self globallyRegisterExtension:extension];\n"
          "  [extension release];\n"
          "}\n");
    }

    const vector<FileGenerator *> &dependency_generators =
        DependencyGenerators();
    for (vector<FileGenerator *>::const_iterator iter =
             dependency_generators.begin();
         iter != dependency_generators.end(); ++iter) {
      printer->Print(
          "[registry addExtensions:[$dependency$ extensionRegistry]];\n",
          "dependency", (*iter)->RootClassName());
    }

    printer->Outdent();
    printer->Outdent();

    printer->Print(
        "  }\n"
        "  return registry;\n"
        "}\n"
        "\n");
  }

  printer->Print("@end\n\n");

  // File descriptor only needed if there are messages to use it.
  if (message_generators_.size() > 0) {
    string syntax;
    switch (file_->syntax()) {
      case FileDescriptor::SYNTAX_UNKNOWN:
        syntax = "GPBFileSyntaxUnknown";
        break;
      case FileDescriptor::SYNTAX_PROTO2:
        syntax = "GPBFileSyntaxProto2";
        break;
      case FileDescriptor::SYNTAX_PROTO3:
        syntax = "GPBFileSyntaxProto3";
        break;
    }
    printer->Print(
        "#pragma mark - $root_class_name$_FileDescriptor\n"
        "\n"
        "static GPBFileDescriptor *$root_class_name$_FileDescriptor(void) {\n"
        "  // This is called by +initialize so there is no need to worry\n"
        "  // about thread safety of the singleton.\n"
        "  static GPBFileDescriptor *descriptor = NULL;\n"
        "  if (!descriptor) {\n"
        "    GPBDebugCheckRuntimeVersion();\n"
        "    descriptor = [[GPBFileDescriptor alloc] initWithPackage:@\"$package$\"\n"
        "                                                     syntax:$syntax$];\n"
        "  }\n"
        "  return descriptor;\n"
        "}\n"
        "\n",
        "root_class_name", root_class_name_,
        "package", file_->package(),
        "syntax", syntax);
  }

  for (vector<EnumGenerator *>::iterator iter = enum_generators_.begin();
       iter != enum_generators_.end(); ++iter) {
    (*iter)->GenerateSource(printer);
  }
  for (vector<MessageGenerator *>::iterator iter = message_generators_.begin();
       iter != message_generators_.end(); ++iter) {
    (*iter)->GenerateSource(printer);
  }

  printer->Print(
    "\n"
    "#pragma clang diagnostic pop\n"
    "\n"
    "// @@protoc_insertion_point(global_scope)\n");
}

const vector<FileGenerator *> &FileGenerator::DependencyGenerators() {
  if (file_->dependency_count() != dependency_generators_.size()) {
    set<string> public_import_names;
    for (int i = 0; i < file_->public_dependency_count(); i++) {
      public_import_names.insert(file_->public_dependency(i)->name());
    }
    for (int i = 0; i < file_->dependency_count(); i++) {
      FileGenerator *generator =
          new FileGenerator(file_->dependency(i), options_);
      const string& name = file_->dependency(i)->name();
      bool public_import = (public_import_names.count(name) != 0);
      generator->SetIsPublicDependency(public_import);
      dependency_generators_.push_back(generator);
    }
  }
  return dependency_generators_;
}

// Helper to print the import of the runtime support at the top of generated
// files. This currently only supports the runtime coming from a framework
// as defined by the official CocoaPod.
void FileGenerator::PrintFileRuntimePreamble(
    io::Printer* printer, const string& header_to_import) const {
  printer->Print(
      "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
      "// source: $filename$\n"
      "\n",
      "filename", file_->name());

  const string framework_name(ProtobufLibraryFrameworkName);
  const string cpp_symbol(ProtobufFrameworkImportSymbol(framework_name));
  printer->Print(
      "// This CPP symbol can be defined to use imports that match up to the framework\n"
      "// imports needed when using CocoaPods.\n"
      "#if !defined($cpp_symbol$)\n"
      " #define $cpp_symbol$ 0\n"
      "#endif\n"
      "\n"
      "#if $cpp_symbol$\n"
      " #import <$framework_name$/$header$>\n"
      "#else\n"
      " #import \"$header$\"\n"
      "#endif\n"
      "\n",
      "cpp_symbol", cpp_symbol,
      "header", header_to_import,
      "framework_name", framework_name);
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
