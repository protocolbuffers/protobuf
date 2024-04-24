// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_BASE_STATUS_HPP_
#define UPB_BASE_STATUS_HPP_

#ifdef __cplusplus

#include "upb/base/status.h"

namespace upb {

class Status final {
 public:
  Status() { upb_Status_Clear(&status_); }

  upb_Status* ptr() { return &status_; }

  // Returns true if there is no error.
  bool ok() const { return upb_Status_IsOk(&status_); }

  // Guaranteed to be NULL-terminated.
  const char* error_message() const {
    return upb_Status_ErrorMessage(&status_);
  }

  // The error message will be truncated if it is longer than
  // _kUpb_Status_MaxMessage-4.
  void SetErrorMessage(const char* msg) {
    upb_Status_SetErrorMessage(&status_, msg);
  }
  void SetFormattedErrorMessage(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    upb_Status_VSetErrorFormat(&status_, fmt, args);
    va_end(args);
  }

  // Resets the status to a successful state with no message.
  void Clear() { upb_Status_Clear(&status_); }

 private:
  upb_Status status_;
};

}  // namespace upb

#endif  // __cplusplus

#endif  // UPB_BASE_STATUS_HPP_
