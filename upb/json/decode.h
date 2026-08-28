// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_JSON_DECODE_H_
#define UPB_JSON_DECODE_H_

#include <stddef.h>

#include "upb/base/status.h"
#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/reflection/def.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  upb_JsonDecode_IgnoreUnknown = 1 << 0,

  // Controls whether the decoder performs UTF-8 validation on the JSON input.
  // Under Enforce, it will fail with a decode error if the provided input is
  // not well-formed UTF-8.
  // Callers must not set both upb_JsonDecode_ValidateUtf8_Disable and
  // upb_JsonDecode_ValidateUtf8_Enforce.
  // The default behavior (when neither is set) is to warn on invalid UTF-8,
  // but this will become Enforce in an upcoming breaking change.
  upb_JsonDecode_ValidateUtf8_Disable = 1 << 1,
  upb_JsonDecode_ValidateUtf8_Enforce = 2 << 1,
};

enum {
  kUpb_JsonDecodeResult_Ok = 0,
  kUpb_JsonDecodeResult_Error = 2,
};

UPB_NODISCARD UPB_API int upb_JsonDecodeDetectingNonconformance(
    const char* buf, size_t size, upb_Message* msg, const upb_MessageDef* m,
    const upb_DefPool* symtab, int options, upb_Arena* arena,
    upb_Status* status);

UPB_NODISCARD UPB_API_INLINE bool upb_JsonDecode(const char* buf, size_t size,
                                                 upb_Message* msg,
                                                 const upb_MessageDef* m,
                                                 const upb_DefPool* symtab,
                                                 int options, upb_Arena* arena,
                                                 upb_Status* status) {
  return upb_JsonDecodeDetectingNonconformance(buf, size, msg, m, symtab,
                                               options, arena, status) ==
         kUpb_JsonDecodeResult_Ok;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_JSONDECODE_H_ */
