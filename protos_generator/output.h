// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_PROTOS_GENERATOR_OUTPUT_H
#define UPB_PROTOS_GENERATOR_OUTPUT_H

#include <vector>

#include "absl/log/absl_log.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace protos_generator {

class Output {
 public:
  Output(google::protobuf::io::ZeroCopyOutputStream* stream) : stream_(stream) {}
  ~Output() { stream_->BackUp((int)buffer_size_); }

  template <class... Arg>
  void operator()(absl::string_view format, const Arg&... arg) {
    Write(absl::Substitute(format, arg...));
  }

  // Indentation size in characters.
  static constexpr size_t kIndentationSize = 2;

  void Indent() { Indent(kIndentationSize); }
  void Indent(size_t size) { indent_ += size; }

  void Outdent() { Outdent(kIndentationSize); }
  void Outdent(size_t size) {
    if (indent_ < size) {
      ABSL_LOG(FATAL) << "mismatched Output indent/unindent calls";
    }
    indent_ -= size;
  }

 private:
  void Write(absl::string_view data) {
    std::string stripped;
    if (absl::StartsWith(data, "\n ")) {
      size_t indent = data.substr(1).find_first_not_of(' ');
      if (indent > indent_) {
        indent -= indent_;
      }
      if (indent != absl::string_view::npos) {
        // Remove indentation from all lines.
        auto line_prefix = data.substr(0, indent + 1);
        // The final line has an extra newline and is indented two less, eg.
        //    R"cc(
        //      UPB_INLINE $0 $1_$2(const $1 *msg) {
        //        return $1_has_$2(msg) ? *UPB_PTR_AT(msg, $3, $0) : $4;
        //      }
        //    )cc",
        std::string last_line_prefix = std::string(line_prefix);
        last_line_prefix.resize(last_line_prefix.size() - 2);
        data.remove_prefix(line_prefix.size());
        stripped = absl::StrReplaceAll(
            data, {{line_prefix, "\n"}, {last_line_prefix, "\n"}});
        data = stripped;
      }
    } else {
      WriteIndent();
    }
    WriteRaw(data);
  }

  void WriteRaw(absl::string_view data) {
    while (!data.empty()) {
      RefreshOutput();
      size_t to_write = std::min(data.size(), buffer_size_);
      memcpy(output_buffer_, data.data(), to_write);
      data.remove_prefix(to_write);
      output_buffer_ += to_write;
      buffer_size_ -= to_write;
    }
  }

  void WriteIndent() {
    if (indent_ == 0) {
      return;
    }
    size_t size = indent_;
    while (size > buffer_size_) {
      if (buffer_size_ > 0) {
        memset(output_buffer_, ' ', buffer_size_);
      }
      size -= buffer_size_;
      buffer_size_ = 0;
      RefreshOutput();
    }
    memset(output_buffer_, ' ', size);
    output_buffer_ += size;
    buffer_size_ -= size;
  }

  void RefreshOutput() {
    while (buffer_size_ == 0) {
      void* void_buffer;
      int size;
      if (!stream_->Next(&void_buffer, &size)) {
        fprintf(stderr, "upb_generator: Failed to write to to output\n");
        abort();
      }
      output_buffer_ = static_cast<char*>(void_buffer);
      buffer_size_ = size;
    }
  }

  google::protobuf::io::ZeroCopyOutputStream* stream_;
  char* output_buffer_ = nullptr;
  size_t buffer_size_ = 0;
  // Current indentation size in characters.
  size_t indent_ = 0;
  friend class OutputIndenter;
};

class OutputIndenter {
 public:
  OutputIndenter(Output& output)
      : OutputIndenter(output, Output::kIndentationSize) {}
  OutputIndenter(Output& output, size_t indent_size)
      : indent_size_(indent_size), output_(output) {
    output.Indent(indent_size);
  }
  ~OutputIndenter() { output_.Outdent(indent_size_); }

 private:
  size_t indent_size_;
  Output& output_;
};

std::string StripExtension(absl::string_view fname);
std::string ToCIdent(absl::string_view str);
std::string ToPreproc(absl::string_view str);
void EmitFileWarning(const google::protobuf::FileDescriptor* file, Output& output);
std::string MessageName(const google::protobuf::Descriptor* descriptor);
std::string FileLayoutName(const google::protobuf::FileDescriptor* file);
std::string CHeaderFilename(const google::protobuf::FileDescriptor* file);
std::string CSourceFilename(const google::protobuf::FileDescriptor* file);

}  // namespace protos_generator

#endif  // UPB_PROTOS_GENERATOR_OUTPUT_H
