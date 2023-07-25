/*
 * Copyright (c) 2009-2022, Google LLC
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

#ifndef UPBC_UPBDEV_H_
#define UPBC_UPBDEV_H_

#include "upb/base/status.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Consume |buf|, deserialize it to a Code_Generator_Request proto, construct a
// upbc_Code_Generator_Request, and return it as a JSON-encoded string.
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
UPB_API upb_Arena* upbdev_Arena_New();
UPB_API void upbdev_Status_Clear(upb_Status* status);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPBC_UPBDEV_H_
