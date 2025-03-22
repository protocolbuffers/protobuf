// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/hpb/status.h"

#include <cstdint>

#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "absl/types/source_location.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

namespace hpb {
absl::Status MessageAllocationError(SourceLocation loc) {
  return absl::Status(absl::StatusCode::kUnknown,
                      "Upb message allocation error");
}

absl::Status ExtensionNotFoundError(int ext_number, SourceLocation loc) {
  return absl::Status(absl::StatusCode::kUnknown,
                      absl::StrFormat("Extension %d not found", ext_number));
}

absl::Status MessageEncodeError(upb_EncodeStatus s, SourceLocation loc) {
  return absl::Status(absl::StatusCode::kUnknown, "Encoding error");
}

absl::Status MessageDecodeError(upb_DecodeStatus status, SourceLocation loc

) {
  return absl::Status(absl::StatusCode::kUnknown, "Upb message parse error");
}

}  // namespace hpb
