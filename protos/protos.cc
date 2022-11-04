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

#include "protos/protos.h"

#include "absl/strings/str_format.h"

namespace protos {

// begin:google_only
// absl::Status MessageAllocationError(SourceLocation loc) {
//   return absl::Status(absl::StatusCode::kInternal,
//                       "Upb message allocation error", loc);
// }
//
// absl::Status ExtensionNotFoundError(int extension_number, SourceLocation loc) {
//   return absl::Status(
//       absl::StatusCode::kInternal,
//       absl::StrFormat("Extension %d not found", extension_number), loc);
// }
//
// absl::Status MessageEncodeError(upb_EncodeStatus status, SourceLocation loc) {
//   return absl::Status(absl::StatusCode::kInternal,
//                       absl::StrFormat("Upb message encoding error %d", status),
//                       loc
//
//   );
// }
//
// absl::Status MessageDecodeError(upb_DecodeStatus status, SourceLocation loc
//
// ) {
//   return absl::Status(absl::StatusCode::kInternal,
//                       absl::StrFormat("Upb message parse error %d", status), loc
//
//   );
// }
// end:google_only

// begin:github_only
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
// end:github_only

namespace internal {

upb_ExtensionRegistry* GetUpbExtensions(
    const ExtensionRegistry& extension_registry) {
  return extension_registry.registry_;
}

absl::StatusOr<absl::string_view> Serialize(const upb_Message* message,
                                            const upb_MiniTable* mini_table,
                                            upb_Arena* arena, int options) {
  size_t len;
  char* ptr;
  upb_EncodeStatus status =
      upb_Encode(message, mini_table, options, arena, &ptr, &len);
  if (status == kUpb_EncodeStatus_Ok) {
    return absl::string_view(ptr, len);
  }
  return MessageEncodeError(status);
}

}  // namespace internal

}  // namespace protos
