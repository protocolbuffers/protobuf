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

#include "google/protobuf/compiler/objectivec/import_writer.h"

#include "absl/strings/ascii.h"
#include "google/protobuf/compiler/objectivec/line_consumer.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/io/printer.h"

// NOTE: src/google/protobuf/compiler/plugin.cc makes use of cerr for some
// error cases, so it seems to be ok to use as a back door for errors.

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {

class ProtoFrameworkCollector : public LineConsumer {
 public:
  ProtoFrameworkCollector(
      std::map<std::string, std::string>* inout_proto_file_to_framework_name)
      : map_(inout_proto_file_to_framework_name) {}

  virtual bool ConsumeLine(const absl::string_view& line,
                           std::string* out_error) override;

 private:
  std::map<std::string, std::string>* map_;
};

bool ProtoFrameworkCollector::ConsumeLine(const absl::string_view& line,
                                          std::string* out_error) {
  int offset = line.find(':');
  if (offset == absl::string_view::npos) {
    *out_error =
        std::string("Framework/proto file mapping line without colon sign: '") +
        std::string(line) + "'.";
    return false;
  }
  absl::string_view framework_name =
      absl::StripAsciiWhitespace(line.substr(0, offset));
  absl::string_view proto_file_list =
      absl::StripAsciiWhitespace(line.substr(offset + 1));

  int start = 0;
  while (start < proto_file_list.length()) {
    offset = proto_file_list.find(',', start);
    if (offset == absl::string_view::npos) {
      offset = proto_file_list.length();
    }

    absl::string_view proto_file = absl::StripAsciiWhitespace(
        proto_file_list.substr(start, offset - start));
    if (!proto_file.empty()) {
      std::map<std::string, std::string>::iterator existing_entry =
          map_->find(std::string(proto_file));
      if (existing_entry != map_->end()) {
        std::cerr << "warning: duplicate proto file reference, replacing "
                     "framework entry for '"
                  << std::string(proto_file) << "' with '"
                  << std::string(framework_name) << "' (was '"
                  << existing_entry->second << "')." << std::endl;
        std::cerr.flush();
      }

      if (proto_file.find(' ') != absl::string_view::npos) {
        std::cerr << "note: framework mapping file had a proto file with a "
                     "space in, hopefully that isn't a missing comma: '"
                  << std::string(proto_file) << "'" << std::endl;
        std::cerr.flush();
      }

      (*map_)[std::string(proto_file)] = std::string(framework_name);
    }

    start = offset + 1;
  }

  return true;
}

}  // namespace

ImportWriter::ImportWriter(
    const std::string& generate_for_named_framework,
    const std::string& named_framework_to_proto_path_mappings_path,
    const std::string& runtime_import_prefix, bool include_wkt_imports)
    : generate_for_named_framework_(generate_for_named_framework),
      named_framework_to_proto_path_mappings_path_(
          named_framework_to_proto_path_mappings_path),
      runtime_import_prefix_(runtime_import_prefix),
      include_wkt_imports_(include_wkt_imports),
      need_to_parse_mapping_file_(true) {}

ImportWriter::~ImportWriter() {}

void ImportWriter::AddFile(const FileDescriptor* file,
                           const std::string& header_extension) {
  if (IsProtobufLibraryBundledProtoFile(file)) {
    // The imports of the WKTs are only needed within the library itself,
    // in other cases, they get skipped because the generated code already
    // import GPBProtocolBuffers.h and hence proves them.
    if (include_wkt_imports_) {
      const std::string header_name =
          "GPB" + FilePathBasename(file) + header_extension;
      protobuf_imports_.push_back(header_name);
    }
    return;
  }

  // Lazy parse any mappings.
  if (need_to_parse_mapping_file_) {
    ParseFrameworkMappings();
  }

  std::map<std::string, std::string>::iterator proto_lookup =
      proto_file_to_framework_name_.find(file->name());
  if (proto_lookup != proto_file_to_framework_name_.end()) {
    other_framework_imports_.push_back(
        proto_lookup->second + "/" + FilePathBasename(file) + header_extension);
    return;
  }

  if (!generate_for_named_framework_.empty()) {
    other_framework_imports_.push_back(generate_for_named_framework_ + "/" +
                                       FilePathBasename(file) +
                                       header_extension);
    return;
  }

  other_imports_.push_back(FilePath(file) + header_extension);
}

void ImportWriter::Print(io::Printer* printer) const {
  bool add_blank_line = false;

  if (!protobuf_imports_.empty()) {
    PrintRuntimeImports(printer, protobuf_imports_, runtime_import_prefix_);
    add_blank_line = true;
  }

  if (!other_framework_imports_.empty()) {
    if (add_blank_line) {
      printer->Print("\n");
    }

    for (std::vector<std::string>::const_iterator iter =
             other_framework_imports_.begin();
         iter != other_framework_imports_.end(); ++iter) {
      printer->Print("#import <$header$>\n", "header", *iter);
    }

    add_blank_line = true;
  }

  if (!other_imports_.empty()) {
    if (add_blank_line) {
      printer->Print("\n");
    }

    for (std::vector<std::string>::const_iterator iter = other_imports_.begin();
         iter != other_imports_.end(); ++iter) {
      printer->Print("#import \"$header$\"\n", "header", *iter);
    }
  }
}

void ImportWriter::PrintRuntimeImports(
    io::Printer* printer, const std::vector<std::string>& header_to_import,
    const std::string& runtime_import_prefix, bool default_cpp_symbol) {
  // Given an override, use that.
  if (!runtime_import_prefix.empty()) {
    for (const auto& header : header_to_import) {
      printer->Print(" #import \"$import_prefix$/$header$\"\n", "import_prefix",
                     runtime_import_prefix, "header", header);
    }
    return;
  }

  const std::string framework_name(ProtobufLibraryFrameworkName);
  const std::string cpp_symbol(ProtobufFrameworkImportSymbol(framework_name));

  if (default_cpp_symbol) {
    printer->Print(
        // clang-format off
        "// This CPP symbol can be defined to use imports that match up to the framework\n"
        "// imports needed when using CocoaPods.\n"
        "#if !defined($cpp_symbol$)\n"
        " #define $cpp_symbol$ 0\n"
        "#endif\n"
        "\n",
        // clang-format on
        "cpp_symbol", cpp_symbol);
  }

  printer->Print("#if $cpp_symbol$\n", "cpp_symbol", cpp_symbol);
  for (const auto& header : header_to_import) {
    printer->Print(" #import <$framework_name$/$header$>\n", "framework_name",
                   framework_name, "header", header);
  }
  printer->Print("#else\n");
  for (const auto& header : header_to_import) {
    printer->Print(" #import \"$header$\"\n", "header", header);
  }
  printer->Print("#endif\n");
}

void ImportWriter::ParseFrameworkMappings() {
  need_to_parse_mapping_file_ = false;
  if (named_framework_to_proto_path_mappings_path_.empty()) {
    return;  // Nothing to do.
  }

  ProtoFrameworkCollector collector(&proto_file_to_framework_name_);
  std::string parse_error;
  if (!ParseSimpleFile(named_framework_to_proto_path_mappings_path_, &collector,
                       &parse_error)) {
    std::cerr << "error parsing "
              << named_framework_to_proto_path_mappings_path_ << " : "
              << parse_error << std::endl;
    std::cerr.flush();
  }
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
