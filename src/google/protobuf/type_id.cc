// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/type_id.h"

#include "absl/strings/string_view.h"
#include "google/protobuf/class_data.h"

namespace google {
namespace protobuf {

absl::string_view TypeId::name() const {
  if (!data_->is_lite) {
    // For !LITE messages, we use the descriptor method function.
    return data_->descriptor_methods()->get_type_name(data_);
  }

  // For LITE messages, the type name is accessed via ClassDataLite.
  return data_->type_name();
}

}  // namespace protobuf
}  // namespace google
