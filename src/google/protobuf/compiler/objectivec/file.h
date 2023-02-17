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

#ifndef GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_FILE_H__
#define GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_FILE_H__

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "google/protobuf/compiler/objectivec/enum.h"
#include "google/protobuf/compiler/objectivec/extension.h"
#include "google/protobuf/compiler/objectivec/message.h"
#include "google/protobuf/compiler/objectivec/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

class FileGenerator {
 public:
  // Wrapper for some common state that is shared between file generations to
  // improve performance when more than one file is generated at a time.
  struct CommonState {
    CommonState() = default;

    std::vector<const FileDescriptor*>
    CollectMinimalFileDepsContainingExtensions(const FileDescriptor* file);

   private:
    struct MinDepsEntry {
      bool has_extensions;
      // The minimal dependencies that cover all the dependencies with
      // extensions.
      absl::flat_hash_set<const FileDescriptor*> min_deps;
      absl::flat_hash_set<const FileDescriptor*> transitive_deps;
    };
    const MinDepsEntry& CollectMinimalFileDepsContainingExtensionsInternal(
        const FileDescriptor* file);
    absl::flat_hash_map<const FileDescriptor*, MinDepsEntry> deps_info_cache;
  };

  FileGenerator(const FileDescriptor* file,
                const GenerationOptions& generation_options,
                CommonState& common_state);
  ~FileGenerator() = default;

  FileGenerator(const FileGenerator&) = delete;
  FileGenerator& operator=(const FileGenerator&) = delete;

  void GenerateHeader(io::Printer* p) const;
  void GenerateSource(io::Printer* p) const;

  int NumEnums() const { return enum_generators_.size(); }
  int NumMessages() const { return message_generators_.size(); }

  void GenerateGlobalSource(io::Printer* p) const;
  void GenerateSourceForMessage(int idx, io::Printer* p) const;
  void GenerateSourceForEnums(io::Printer* p) const;

 private:
  enum class GeneratedFileType : int { kHeader, kSource };
  struct GeneratedFileOptions {
    std::vector<std::string> ignored_warnings;
    std::vector<const FileDescriptor*> extra_files_to_import;
    std::vector<std::string> extra_system_headers;
  };

  void GenerateFile(io::Printer* p, GeneratedFileType file_type,
                    const GeneratedFileOptions& file_options,
                    std::function<void()> body) const;
  void GenerateFile(io::Printer* p, GeneratedFileType file_type,
                    std::function<void()> body) const {
    GeneratedFileOptions file_options;
    GenerateFile(p, file_type, file_options, body);
  }

  void PrintRootImplementation(
      io::Printer* p,
      const std::vector<const FileDescriptor*>& deps_with_extensions) const;
  void PrintRootExtensionRegistryImplementation(
      io::Printer* p,
      const std::vector<const FileDescriptor*>& deps_with_extensions) const;
  void PrintFileDescription(io::Printer* p) const;

  bool HeadersUseForwardDeclarations() const {
    // The bundled protos (WKTs) don't make use of forward declarations.
    return !is_bundled_proto_ &&
           generation_options_.headers_use_forward_declarations;
  }

  const FileDescriptor* file_;
  const GenerationOptions& generation_options_;
  mutable CommonState* common_state_;
  const std::string root_class_name_;
  const std::string file_description_name_;
  const bool is_bundled_proto_;

  std::vector<std::unique_ptr<EnumGenerator>> enum_generators_;
  std::vector<std::unique_ptr<MessageGenerator>> message_generators_;
  // The first file_->extension_count() are the extensions at file level scope.
  std::vector<std::unique_ptr<ExtensionGenerator>> extension_generators_;
};

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_FILE_H__
