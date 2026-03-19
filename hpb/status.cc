// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "hpb/status.h"

#include <cstdint>

#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "absl/types/source_location.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

namespace hpb {
absl::Status MessageAllocationError(SourceLocation loc) {
  return absl::Status(absl::StatusCode::kInternal,
                      "Upb message allocation error", loc);
}

absl::Status ExtensionNotFoundError(uint32_t extension_number,
                                    SourceLocation loc) {
  return absl::Status(
      absl::StatusCode::kInternal,
      absl::StrFormat("Extension %d not found", extension_number), loc);
}

absl::Status MessageEncodeError(upb_EncodeStatus status, SourceLocation loc) {
  return absl::Status(absl::StatusCode::kInternal,
                      absl::StrFormat("Upb message encoding error %d", status),
                      loc

  );
}

absl::Status MessageDecodeError(upb_DecodeStatus status, SourceLocation loc

) {
  return absl::Status(absl::StatusCode::kInternal,
                      absl::StrFormat("Upb message parse error %d", status), loc

  );
}

}  // namespace hpb
