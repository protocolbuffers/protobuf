// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
