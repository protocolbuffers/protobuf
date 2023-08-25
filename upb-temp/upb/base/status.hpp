// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef UPB_BASE_STATUS_HPP_
#define UPB_BASE_STATUS_HPP_

#include "upb/base/status.h"

namespace upb {

class Status {
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

#endif  // UPB_BASE_STATUS_HPP_
