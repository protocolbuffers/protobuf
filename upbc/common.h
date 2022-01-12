/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UPBC_COMMON_H
#define UPBC_COMMON_H

#include <vector>

#include "absl/strings/str_replace.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace upbc {

class Output {
 public:
  Output(google::protobuf::io::ZeroCopyOutputStream* stream)
      : stream_(stream) {}
  ~Output() { stream_->BackUp((int)size_); }

  template <class... Arg>
  void operator()(absl::string_view format, const Arg&... arg) {
    Write(absl::Substitute(format, arg...));
  }

 private:
  void Write(absl::string_view data) {
    std::string stripped;
    if (absl::StartsWith(data, "\n ")) {
      size_t indent = data.substr(1).find_first_not_of(' ');
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
    }
    while (!data.empty()) {
      RefreshOutput();
      size_t to_write = std::min(data.size(), size_);
      memcpy(ptr_, data.data(), to_write);
      data.remove_prefix(to_write);
      ptr_ += to_write;
      size_ -= to_write;
    }
  }

  void RefreshOutput() {
    while (size_ == 0) {
      void* ptr;
      int size;
      if (!stream_->Next(&ptr, &size)) {
        fprintf(stderr, "upbc: Failed to write to to output\n");
        abort();
      }
      ptr_ = static_cast<char*>(ptr);
      size_ = size;
    }
  }

  google::protobuf::io::ZeroCopyOutputStream* stream_;
  char* ptr_ = nullptr;
  size_t size_ = 0;
};

std::string StripExtension(absl::string_view fname);
std::string ToCIdent(absl::string_view str);
std::string ToPreproc(absl::string_view str);
void EmitFileWarning(const google::protobuf::FileDescriptor* file,
                     Output& output);
std::string MessageName(const google::protobuf::Descriptor* descriptor);
std::string FileLayoutName(const google::protobuf::FileDescriptor* file);
std::string HeaderFilename(const google::protobuf::FileDescriptor* file);

}  // namespace upbc

#endif  // UPBC_COMMON_H
