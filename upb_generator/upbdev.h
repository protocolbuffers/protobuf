// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_GENERATOR_UPBDEV_H_
#define UPB_GENERATOR_UPBDEV_H_

#include "upb/base/status.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Consume |buf|, deserialize it to a Code_Generator_Request proto, construct a
// upb_Code_Generator_Request, and return it as a JSON-encoded string.
UPB_API upb_StringView upbdev_ProcessInput(const char* buf, size_t size,
                                           upb_Arena* arena,
                                           upb_Status* status);

// Decode |buf| from JSON, serialize to wire format, and return it.
UPB_API upb_StringView upbdev_ProcessOutput(const char* buf, size_t size,
                                            upb_Arena* arena,
                                            upb_Status* status);

// Decode |buf| from JSON, serialize to wire format, and write it to stdout.
UPB_API void upbdev_ProcessStdout(const char* buf, size_t size,
                                  upb_Arena* arena, upb_Status* status);

// The following wrappers allow the protoc plugins to call the above functions
// without pulling in the entire pb_runtime library.
UPB_API upb_Arena* upbdev_Arena_New(void);
UPB_API void upbdev_Status_Clear(upb_Status* status);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_GENERATOR_UPBDEV_H_
