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

#include <map>
#include <set>
#include <string>
#include <vector>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

class EnumGenerator;
class ExtensionGenerator;
class MessageGenerator;

class FileGenerator {
 public:
  struct GenerationOptions {
    GenerationOptions()
      // TODO(thomasvl): Eventually flip this default to false for better
      // interop with Swift if proto usages span modules made from ObjC sources.
      : headers_use_forward_declarations(true) {}
    std::string generate_for_named_framework;
    std::string named_framework_to_proto_path_mappings_path;
    std::string runtime_import_prefix;
    bool headers_use_forward_declarations;
  };

  // Wrapper for some common state that is shared between file generations to
  // improve performance when more than one file is generated at a time.
  struct CommonState {
    CommonState();

    const std::vector<const FileDescriptor*>
    CollectMinimalFileDepsContainingExtensions(const FileDescriptor* file);

   private:
    struct MinDepsEntry {
      bool has_extensions;
      std::set<const FileDescriptor*> min_deps;
      // `covered_deps` are the transtive deps of `min_deps_w_exts` that also
      // have extensions.
      std::set<const FileDescriptor*> covered_deps;
    };
    const MinDepsEntry& CollectMinimalFileDepsContainingExtensionsInternal(const FileDescriptor* file);
    std::map<const FileDescriptor*, MinDepsEntry> deps_info_cache_;
  };

  FileGenerator(const FileDescriptor* file,
                const GenerationOptions& generation_options,
                CommonState& common_state);
  ~FileGenerator();

  FileGenerator(const FileGenerator&) = delete;
  FileGenerator& operator=(const FileGenerator&) = delete;

  void GenerateSource(io::Printer* printer);
  void GenerateHeader(io::Printer* printer);

 private:
  const FileDescriptor* file_;
  const GenerationOptions& generation_options_;
  CommonState& common_state_;
  std::string root_class_name_;
  bool is_bundled_proto_;

  std::vector<std::unique_ptr<EnumGenerator>> enum_generators_;
  std::vector<std::unique_ptr<MessageGenerator>> message_generators_;
  std::vector<std::unique_ptr<ExtensionGenerator>> extension_generators_;

  void PrintFileRuntimePreamble(
      io::Printer* printer,
      const std::vector<std::string>& headers_to_import) const;
};

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_FILE_H__
