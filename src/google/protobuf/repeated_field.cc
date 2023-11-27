// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/repeated_field.h"

#include <algorithm>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/cord.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {


template <>
PROTOBUF_EXPORT_TEMPLATE_DEFINE size_t
RepeatedField<absl::Cord>::SpaceUsedExcludingSelfLong() const {
  size_t result = current_size_ * sizeof(absl::Cord);
  for (int i = 0; i < current_size_; i++) {
    // Estimate only.
    result += Get(i).size();
  }
  return result;
}


}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
