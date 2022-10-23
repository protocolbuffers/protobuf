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

#ifndef GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_IMPORT_WRITER_H__
#define GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_IMPORT_WRITER_H__

#include <string>
#include <vector>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

// Helper class for parsing framework import mappings and generating
// import statements.
class ImportWriter {
 public:
  ImportWriter(const std::string& generate_for_named_framework,
               const std::string& named_framework_to_proto_path_mappings_path,
               const std::string& runtime_import_prefix,
               bool include_wkt_imports);
  ~ImportWriter();

  void AddFile(const FileDescriptor* file, const std::string& header_extension);
  void Print(io::Printer* printer) const;

  static void PrintRuntimeImports(
      io::Printer* printer, const std::vector<std::string>& header_to_import,
      const std::string& runtime_import_prefix,
      bool default_cpp_symbol = false);

 private:
  void ParseFrameworkMappings();

  const std::string generate_for_named_framework_;
  const std::string named_framework_to_proto_path_mappings_path_;
  const std::string runtime_import_prefix_;
  const bool include_wkt_imports_;
  std::map<std::string, std::string> proto_file_to_framework_name_;
  bool need_to_parse_mapping_file_;

  std::vector<std::string> protobuf_imports_;
  std::vector<std::string> other_framework_imports_;
  std::vector<std::string> other_imports_;
};

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_IMPORT_WRITER_H__
