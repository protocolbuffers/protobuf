// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_INTERNAL_ERROR_ACCESS_H__
#define GOOGLE_PROTOBUF_HPB_INTERNAL_ERROR_ACCESS_H__

#include <cstdint>

#include "hpb/error.h"

namespace hpb::internal {

struct ErrorAccess {
  static constexpr Error OutOfMemory() {
    return Error(ErrorCode::kOutOfMemory, 0);
  }
  static constexpr Error ExtensionNotFound(uint32_t extension_number) {
    return Error(ErrorCode::kExtensionNotFound, extension_number);
  }
  static constexpr Error DecodeError(
      DecodeStatus status = DecodeStatus::kMalformed) {
    return Error(ErrorCode::kDecodeError, static_cast<uint32_t>(status));
  }
  static constexpr Error EncodeError(
      EncodeStatus status = EncodeStatus::kOutOfMemory) {
    return Error(ErrorCode::kEncodeError, static_cast<uint32_t>(status));
  }
};

}  // namespace hpb::internal

#endif  // GOOGLE_PROTOBUF_HPB_INTERNAL_ERROR_ACCESS_H__
