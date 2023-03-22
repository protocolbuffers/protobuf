// Copyright (c) 2009-2021, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Google LLC nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef UPB_UPBC_PLUGIN_H_
#define UPB_UPBC_PLUGIN_H_

#include <stdio.h>

#include <string>
#include <vector>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

// begin:google_only
// #ifndef UPB_BOOTSTRAP_STAGE0
// #include "net/proto2/compiler/proto/plugin.upb.h"
// #include "net/proto2/proto/descriptor.upb.h"
// #else
// #include "google/protobuf/compiler/plugin.upb.h"
// #include "google/protobuf/descriptor.upb.h"
// #endif
// end:google_only

// begin:github_only
#include "google/protobuf/compiler/plugin.upb.h"
#include "google/protobuf/descriptor.upb.h"
// end:github_only

#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "upb/reflection/def.hpp"

// Must be last.
#include "upb/port/def.inc"

namespace upbc {

inline std::vector<std::pair<std::string, std::string>> ParseGeneratorParameter(
    const absl::string_view text) {
  std::vector<std::pair<std::string, std::string>> ret;
  for (absl::string_view sp : absl::StrSplit(text, ',', absl::SkipEmpty())) {
    std::string::size_type equals_pos = sp.find_first_of('=');
    std::pair<std::string, std::string> value;
    if (equals_pos == std::string::npos) {
      value.first = std::string(sp);
    } else {
      value.first = std::string(sp.substr(0, equals_pos));
      value.second = std::string(sp.substr(equals_pos + 1));
    }
    ret.push_back(std::move(value));
  }
  return ret;
}

class Plugin {
 public:
  Plugin() { ReadRequest(); }
  ~Plugin() { WriteResponse(); }

  absl::string_view parameter() const {
    return ToStringView(
        UPB_DESC(compiler_CodeGeneratorRequest_parameter)(request_));
  }

  template <class T>
  void GenerateFilesRaw(T&& func) {
    absl::flat_hash_set<absl::string_view> files_to_generate;
    size_t size;
    const upb_StringView* file_to_generate = UPB_DESC(
        compiler_CodeGeneratorRequest_file_to_generate)(request_, &size);
    for (size_t i = 0; i < size; i++) {
      files_to_generate.insert(
          {file_to_generate[i].data, file_to_generate[i].size});
    }

    const UPB_DESC(FileDescriptorProto)* const* files =
        UPB_DESC(compiler_CodeGeneratorRequest_proto_file)(request_, &size);
    for (size_t i = 0; i < size; i++) {
      upb::Status status;
      absl::string_view name =
          ToStringView(UPB_DESC(FileDescriptorProto_name)(files[i]));
      func(files[i], files_to_generate.contains(name));
    }
  }

  template <class T>
  void GenerateFiles(T&& func) {
    GenerateFilesRaw(
        [this, &func](const UPB_DESC(FileDescriptorProto) * file_proto,
                      bool generate) {
          upb::Status status;
          upb::FileDefPtr file = pool_.AddFile(file_proto, &status);
          if (!file) {
            absl::string_view name =
                ToStringView(UPB_DESC(FileDescriptorProto_name)(file_proto));
            ABSL_LOG(FATAL) << "Couldn't add file " << name
                            << " to DefPool: " << status.error_message();
          }
          if (generate) func(file);
        });
  }

  void SetError(absl::string_view error) {
    char* data =
        static_cast<char*>(upb_Arena_Malloc(arena_.ptr(), error.size()));
    memcpy(data, error.data(), error.size());
    UPB_DESC(compiler_CodeGeneratorResponse_set_error)
    (response_, upb_StringView_FromDataAndSize(data, error.size()));
  }

  void AddOutputFile(absl::string_view filename, absl::string_view content) {
    UPB_DESC(compiler_CodeGeneratorResponse_File)* file = UPB_DESC(
        compiler_CodeGeneratorResponse_add_file)(response_, arena_.ptr());
    UPB_DESC(compiler_CodeGeneratorResponse_File_set_name)
    (file, StringDup(filename));
    UPB_DESC(compiler_CodeGeneratorResponse_File_set_content)
    (file, StringDup(content));
  }

 private:
  upb::Arena arena_;
  upb::DefPool pool_;
  UPB_DESC(compiler_CodeGeneratorRequest) * request_;
  UPB_DESC(compiler_CodeGeneratorResponse) * response_;

  static absl::string_view ToStringView(upb_StringView sv) {
    return absl::string_view(sv.data, sv.size);
  }

  upb_StringView StringDup(absl::string_view s) {
    char* data =
        reinterpret_cast<char*>(upb_Arena_Malloc(arena_.ptr(), s.size()));
    memcpy(data, s.data(), s.size());
    return upb_StringView_FromDataAndSize(data, s.size());
  }

  std::string ReadAllStdinBinary() {
    std::string data;
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    char buf[4096];
    while (size_t len = fread(buf, 1, sizeof(buf), stdin)) {
      data.append(buf, len);
    }
    return data;
  }

  void ReadRequest() {
    std::string data = ReadAllStdinBinary();
    request_ = UPB_DESC(compiler_CodeGeneratorRequest_parse)(
        data.data(), data.size(), arena_.ptr());
    if (!request_) {
      ABSL_LOG(FATAL) << "Failed to parse CodeGeneratorRequest";
    }
    response_ = UPB_DESC(compiler_CodeGeneratorResponse_new)(arena_.ptr());
    UPB_DESC(compiler_CodeGeneratorResponse_set_supported_features)
    (response_,
     UPB_DESC(compiler_CodeGeneratorResponse_FEATURE_PROTO3_OPTIONAL));
  }

  void WriteResponse() {
    size_t size;
    char* serialized = UPB_DESC(compiler_CodeGeneratorResponse_serialize)(
        response_, arena_.ptr(), &size);
    if (!serialized) {
      ABSL_LOG(FATAL) << "Failed to serialize CodeGeneratorResponse";
    }

    if (fwrite(serialized, 1, size, stdout) != size) {
      ABSL_LOG(FATAL) << "Failed to write response to stdout";
    }
  }
};

}  // namespace upbc

#include "upb/port/undef.inc"

#endif  // UPB_UPBC_PLUGIN_H_
