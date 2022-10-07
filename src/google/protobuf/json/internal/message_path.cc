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

#include "google/protobuf/json/internal/message_path.h"

#include <string>

#include "absl/strings/str_cat.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace json_internal {
void MessagePath::Describe(std::string& out) const {
  absl::StrAppend(&out, components_.front().type_name);
  if (components_.size() == 1) {
    return;
  }

  absl::StrAppend(&out, " @ ");
  for (size_t i = 1; i < components_.size(); ++i) {
    absl::StrAppend(&out, i == 1 ? "" : ".", components_[i].field_name);
    if (components_[i].repeated_index >= 0) {
      absl::StrAppend(&out, "[", components_[i].repeated_index, "]");
    }
  }
  absl::string_view kind_name =
      FieldDescriptor::TypeName(components_.back().type);
  absl::StrAppend(&out, ": ", kind_name);

  absl::string_view type_name = components_.back().type_name;
  if (!type_name.empty()) {
    absl::StrAppend(&out, " ", type_name);
  }
}
}  // namespace json_internal
}  // namespace protobuf
}  // namespace google
