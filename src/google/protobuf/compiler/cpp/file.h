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

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_FILE_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_FILE_H__

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/stubs/common.h"
#include "google/protobuf/compiler/scc.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/compiler/cpp/enum.h"
#include "google/protobuf/compiler/cpp/extension.h"
#include "google/protobuf/compiler/cpp/field.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/message.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/compiler/cpp/service.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/port.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
class FileGenerator {
 public:
  FileGenerator(const FileDescriptor* file, const Options& options);

  FileGenerator(const FileGenerator&) = delete;
  FileGenerator& operator=(const FileGenerator&) = delete;

  ~FileGenerator() = default;

  // info_path, if non-empty, should be the path (relative to printer's
  // output) to the metadata file describing this proto header.
  void GenerateProtoHeader(io::Printer* p, absl::string_view info_path);
  // info_path, if non-empty, should be the path (relative to printer's
  // output) to the metadata file describing this PB header.
  void GeneratePBHeader(io::Printer* p, absl::string_view info_path);
  void GenerateSource(io::Printer* p);

  // The following member functions are used when the lite_implicit_weak_fields
  // option is set. In this mode the code is organized a bit differently to
  // promote better linker stripping of unused code. In particular, we generate
  // one .cc file per message, one .cc file per extension, and a main pb.cc file
  // containing everything else.

  int NumMessages() const { return message_generators_.size(); }
  int NumExtensions() const { return extension_generators_.size(); }
  // Generates the source file for one message.
  void GenerateSourceForMessage(int idx, io::Printer* p);
  // Generates the source file for one extension.
  void GenerateSourceForExtension(int idx, io::Printer* p);
  // Generates a source file containing everything except messages and
  // extensions.
  void GenerateGlobalSource(io::Printer* p);

 private:
  // Generates a file, setting up the necessary accoutrements that start and
  // end the file, calling `cb` in between.
  //
  // This includes header guards and file-global variables.
  void GenerateFile(io::Printer* p, GeneratedFileType file_type,
                    std::function<void()> cb);

  // Shared code between the two header generators.
  void GenerateSharedHeaderCode(io::Printer* p);

  // Internal type used by GenerateForwardDeclarations (defined in file.cc).
  class ForwardDeclarations;
  struct CrossFileReferences;

  void IncludeFile(absl::string_view google3_name, io::Printer* p) {
    DoIncludeFile(google3_name, false, p);
  }
  void IncludeFileAndExport(absl::string_view google3_name, io::Printer* p) {
    DoIncludeFile(google3_name, true, p);
  }
  void DoIncludeFile(absl::string_view google3_name, bool do_export,
                     io::Printer* p);

  std::string CreateHeaderInclude(absl::string_view basename,
                                  const FileDescriptor* file);
  void GetCrossFileReferencesForField(const FieldDescriptor* field,
                                      CrossFileReferences* refs);
  void GetCrossFileReferencesForFile(const FileDescriptor* file,
                                     CrossFileReferences* refs);
  void GenerateInternalForwardDeclarations(const CrossFileReferences& refs,
                                           io::Printer* p);
  void GenerateSourceIncludes(io::Printer* p);
  void GenerateSourcePrelude(io::Printer* p);
  void GenerateSourceDefaultInstance(int idx, io::Printer* p);

  void GenerateInitForSCC(const SCC* scc, const CrossFileReferences& refs,
                          io::Printer* p);
  void GenerateReflectionInitializationCode(io::Printer* p);

  // For other imports, generates their forward-declarations.
  void GenerateForwardDeclarations(io::Printer* p);

  // Generates top or bottom of a header file.
  void GenerateTopHeaderGuard(io::Printer* p, GeneratedFileType file_type);
  void GenerateBottomHeaderGuard(io::Printer* p, GeneratedFileType file_type);

  // Generates #include directives.
  void GenerateLibraryIncludes(io::Printer* p);
  void GenerateDependencyIncludes(io::Printer* p);

  // Generate a pragma to pull in metadata using the given info_path (if
  // non-empty). info_path should be relative to printer's output.
  void GenerateMetadataPragma(io::Printer* p, absl::string_view info_path);

  // Generates a couple of different pieces before definitions:
  void GenerateGlobalStateFunctionDeclarations(io::Printer* p);

  // Generates types for classes.
  void GenerateMessageDefinitions(io::Printer* p);

  void GenerateEnumDefinitions(io::Printer* p);

  // Generates generic service definitions.
  void GenerateServiceDefinitions(io::Printer* p);

  // Generates extension identifiers.
  void GenerateExtensionIdentifiers(io::Printer* p);

  // Generates inline function definitions.
  void GenerateInlineFunctionDefinitions(io::Printer* p);

  void GenerateProto2NamespaceEnumSpecializations(io::Printer* p);

  // Sometimes the names we use in a .proto file happen to be defined as
  // macros on some platforms (e.g., macro/minor used in plugin.proto are
  // defined as macros in sys/types.h on FreeBSD and a few other platforms).
  // To make the generated code compile on these platforms, we either have to
  // undef the macro for these few platforms, or rename the field name for all
  // platforms. Since these names are part of protobuf public API, renaming is
  // generally a breaking change so we prefer the #undef approach.
  void GenerateMacroUndefs(io::Printer* p);

  bool IsDepWeak(const FileDescriptor* dep) const {
    if (weak_deps_.count(dep) != 0) {
      ABSL_CHECK(!options_.opensource_runtime);
      return true;
    }
    return false;
  }

  absl::flat_hash_set<const FileDescriptor*> weak_deps_;

  const FileDescriptor* file_;
  Options options_;

  MessageSCCAnalyzer scc_analyzer_;

  // This member is unused and should be deleted once all old-style variable
  // maps are gone.
  // TODO(b/245791219)
  absl::flat_hash_map<absl::string_view, std::string> variables_;

  // Contains the post-order walk of all the messages (and child messages) in
  // this file. If you need a pre-order walk just reverse iterate.
  std::vector<std::unique_ptr<MessageGenerator>> message_generators_;
  std::vector<std::unique_ptr<EnumGenerator>> enum_generators_;
  std::vector<std::unique_ptr<ServiceGenerator>> service_generators_;
  std::vector<std::unique_ptr<ExtensionGenerator>> extension_generators_;
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_FILE_H__
