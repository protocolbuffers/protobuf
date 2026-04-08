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
#include <cstddef>
#include <cstdint>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/cord.h"
#include "google/protobuf/repeated_ptr_field.h"

// Must be included last.
#include "google/protobuf/port_def.inc"


namespace google {
namespace protobuf {

namespace internal {

void LogIndexOutOfBounds(int index, int size) {
  ABSL_DLOG(FATAL) << "Index " << index << " out of bounds " << size;
}

void LogIndexOutOfBoundsAndAbort(int64_t index, int64_t size,
                                 BoundsCheckMessageType type) {
  switch (type) {
    case BoundsCheckMessageType::kIndex:
      ABSL_LOG(FATAL) << "Index (" << index
                      << ") out of bounds of container with size (" << size
                      << ")";
      break;
    case BoundsCheckMessageType::kGe:
      ABSL_LOG(FATAL) << "Value (" << index
                      << ") must be greater than or equal to limit (" << size
                      << ")";
      break;
    case BoundsCheckMessageType::kLe:
      ABSL_LOG(FATAL) << "Value (" << index
                      << ") must be less than or equal to limit (" << size
                      << ")";
      break;
  }
}
}  // namespace internal

template <>
PROTOBUF_EXPORT_TEMPLATE_DEFINE size_t
RepeatedField<absl::Cord>::SpaceUsedExcludingSelfLong() const {
  size_t result = size() * sizeof(absl::Cord);
  for (int i = 0; i < size(); i++) {
    // Estimate only.
    result += Get(i).size();
  }
  return result;
}


}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
