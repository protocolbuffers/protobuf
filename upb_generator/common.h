// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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
//     * Neither the name of Google LLC nor the names of its
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

#ifndef UPB_GENERATOR_COMMON_H
#define UPB_GENERATOR_COMMON_H

#include <vector>

#include "absl/strings/str_replace.h"
#include "absl/strings/substitute.h"
#include "upb/reflection/def.hpp"

namespace upb {
namespace generator {

class Output {
 public:
  template <class... Arg>
  void operator()(absl::string_view format, const Arg&... arg) {
    Write(absl::Substitute(format, arg...));
  }

  absl::string_view output() const { return output_; }

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
    absl::StrAppend(&output_, data);
  }

  std::string output_;
};

std::string StripExtension(absl::string_view fname);
std::string ToCIdent(absl::string_view str);
std::string ToPreproc(absl::string_view str);
void EmitFileWarning(absl::string_view name, Output& output);
std::string MessageInit(absl::string_view full_name);
std::string MessageInitName(upb::MessageDefPtr descriptor);
std::string MessageName(upb::MessageDefPtr descriptor);
std::string FileLayoutName(upb::FileDefPtr file);
std::string MiniTableHeaderFilename(upb::FileDefPtr file);
std::string CApiHeaderFilename(upb::FileDefPtr file);

std::string EnumInit(upb::EnumDefPtr descriptor);

std::string FieldInitializer(upb::FieldDefPtr field,
                             const upb_MiniTableField* field64,
                             const upb_MiniTableField* field32);
std::string ArchDependentSize(int64_t size32, int64_t size64);
std::string GetModeInit(const upb_MiniTableField* field32,
                        const upb_MiniTableField* field64);
std::string GetFieldRep(const upb_MiniTableField* field32,
                        const upb_MiniTableField* field64);

}  // namespace generator
}  // namespace upb

#endif  // UPB_GENERATOR_COMMON_H
