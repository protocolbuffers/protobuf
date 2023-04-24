// Copyright (c) 2009-2022, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Google LLC nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "upbc/upbdev.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else  // _WIN32
#include <unistd.h>
#endif  // !_WIN32

#include "google/protobuf/compiler/plugin.upb.h"
#include "google/protobuf/compiler/plugin.upbdefs.h"
#include "upb/base/status.h"
#include "upb/json/decode.h"
#include "upb/json/encode.h"
#include "upb/mem/arena.h"
#include "upbc/code_generator_request.h"
#include "upbc/code_generator_request.upb.h"
#include "upbc/code_generator_request.upbdefs.h"

static google_protobuf_compiler_CodeGeneratorResponse* upbc_JsonDecode(
    const char* data, size_t size, upb_Arena* arena, upb_Status* status) {
  google_protobuf_compiler_CodeGeneratorResponse* response =
      google_protobuf_compiler_CodeGeneratorResponse_new(arena);

  upb_DefPool* s = upb_DefPool_New();
  const upb_MessageDef* m = google_protobuf_compiler_CodeGeneratorResponse_getmsgdef(s);

  (void)upb_JsonDecode(data, size, response, m, s, 0, arena, status);
  if (!upb_Status_IsOk(status)) return NULL;

  upb_DefPool_Free(s);

  return response;
}

static upb_StringView upbc_JsonEncode(const upbc_CodeGeneratorRequest* request,
                                      upb_Arena* arena, upb_Status* status) {
  upb_StringView out = {.data = NULL, .size = 0};

  upb_DefPool* s = upb_DefPool_New();
  const upb_MessageDef* m = upbc_CodeGeneratorRequest_getmsgdef(s);
  const int options = upb_JsonEncode_FormatEnumsAsIntegers;

  out.size = upb_JsonEncode(request, m, s, options, NULL, 0, status);
  if (!upb_Status_IsOk(status)) goto done;

  char* data = (char*)upb_Arena_Malloc(arena, out.size + 1);

  (void)upb_JsonEncode(request, m, s, options, data, out.size + 1, status);
  if (!upb_Status_IsOk(status)) goto done;

  out.data = (const char*)data;

done:
  upb_DefPool_Free(s);
  return out;
}

upb_StringView upbdev_ProcessInput(const char* buf, size_t size,
                                   upb_Arena* arena, upb_Status* status) {
  upb_StringView out = {.data = NULL, .size = 0};

  google_protobuf_compiler_CodeGeneratorRequest* inner_request =
      google_protobuf_compiler_CodeGeneratorRequest_parse(buf, size, arena);

  const upbc_CodeGeneratorRequest* outer_request =
      upbc_MakeCodeGeneratorRequest(inner_request, arena, status);
  if (upb_Status_IsOk(status))
    out = upbc_JsonEncode(outer_request, arena, status);

  return out;
}

upb_StringView upbdev_ProcessOutput(const char* buf, size_t size,
                                    upb_Arena* arena, upb_Status* status) {
  upb_StringView out = {.data = NULL, .size = 0};

  const google_protobuf_compiler_CodeGeneratorResponse* response =
      upbc_JsonDecode(buf, size, arena, status);
  if (!upb_Status_IsOk(status)) return out;

  out.data = google_protobuf_compiler_CodeGeneratorResponse_serialize(response, arena,
                                                             &out.size);
  return out;
}

void upbdev_ProcessStdout(const char* buf, size_t size, upb_Arena* arena,
                          upb_Status* status) {
  const upb_StringView sv = upbdev_ProcessOutput(buf, size, arena, status);
  if (!upb_Status_IsOk(status)) return;

  const char* ptr = sv.data;
  size_t len = sv.size;
  while (len) {
    int n = write(1, ptr, len);
    if (n > 0) {
      ptr += n;
      len -= n;
    }
  }
}
