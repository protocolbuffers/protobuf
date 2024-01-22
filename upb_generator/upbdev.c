// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb_generator/upbdev.h"

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
#include "upb/base/upcast.h"
#include "upb/json/decode.h"
#include "upb/json/encode.h"
#include "upb/mem/arena.h"
#include "upb_generator/code_generator_request.h"
#include "upb_generator/code_generator_request.upb.h"
#include "upb_generator/code_generator_request.upbdefs.h"

static google_protobuf_compiler_CodeGeneratorResponse* upbc_JsonDecode(
    const char* data, size_t size, upb_Arena* arena, upb_Status* status) {
  google_protobuf_compiler_CodeGeneratorResponse* response =
      google_protobuf_compiler_CodeGeneratorResponse_new(arena);

  upb_DefPool* s = upb_DefPool_New();
  const upb_MessageDef* m = google_protobuf_compiler_CodeGeneratorResponse_getmsgdef(s);

  (void)upb_JsonDecode(data, size, UPB_UPCAST(response), m, s, 0, arena,
                       status);
  if (!upb_Status_IsOk(status)) return NULL;

  upb_DefPool_Free(s);

  return response;
}

static upb_StringView upbc_JsonEncode(const upb_CodeGeneratorRequest* request,
                                      upb_Arena* arena, upb_Status* status) {
  upb_StringView out = {.data = NULL, .size = 0};

  upb_DefPool* s = upb_DefPool_New();
  const upb_MessageDef* m = upb_CodeGeneratorRequest_getmsgdef(s);
  const int options = upb_JsonEncode_FormatEnumsAsIntegers;

  out.size =
      upb_JsonEncode(UPB_UPCAST(request), m, s, options, NULL, 0, status);
  if (!upb_Status_IsOk(status)) goto done;

  char* data = (char*)upb_Arena_Malloc(arena, out.size + 1);

  (void)upb_JsonEncode(UPB_UPCAST(request), m, s, options, data, out.size + 1,
                       status);
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

  const upb_CodeGeneratorRequest* outer_request =
      upbc_MakeCodeGeneratorRequest(inner_request, arena, status);
  if (!upb_Status_IsOk(status)) return out;

  return upbc_JsonEncode(outer_request, arena, status);
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

upb_Arena* upbdev_Arena_New(void) { return upb_Arena_New(); }

void upbdev_Status_Clear(upb_Status* status) { upb_Status_Clear(status); }
