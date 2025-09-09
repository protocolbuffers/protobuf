// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_BACKEND_UPB_ERROR_H__
#define GOOGLE_PROTOBUF_HPB_BACKEND_UPB_ERROR_H__

#include <cstdint>
#include <string>

#include "absl/log/absl_log.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

namespace hpb {
namespace internal {
namespace backend {
namespace upb {
class Error {
 public:
  explicit Error(upb_DecodeStatus error)
      : enum_kind_(kDecodeStatus), error_code_(error) {}
  explicit Error(upb_EncodeStatus error)
      : enum_kind_(kEncodeStatus), error_code_(error) {}

  std::string ToString() const {
    switch (enum_kind_) {
      case kDecodeStatus: {
        upb_DecodeStatus decode_status =
            static_cast<upb_DecodeStatus>(error_code_);
        return std::string(upb_DecodeStatus_String(decode_status));
      }
      case kEncodeStatus: {
        upb_EncodeStatus encode_status =
            static_cast<upb_EncodeStatus>(error_code_);
        return std::string(upb_EncodeStatus_String(encode_status));
      }
      default: {
        ABSL_LOG(FATAL) << "hpb::Error unknown enum kind: " << enum_kind_;
      }
    }
  }

 private:
  enum EnumCode { kDecodeStatus, kEncodeStatus };

  uint16_t enum_kind_ = 0;
  uint16_t error_code_ = 0;
};
}  // namespace upb
}  // namespace backend
}  // namespace internal
}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_BACKEND_UPB_ERROR_H__
