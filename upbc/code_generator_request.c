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

#include "upbc/code_generator_request.h"

#include <inttypes.h>
#include <stdio.h>

#include "google/protobuf/compiler/plugin.upb.h"
#include "upb/mini_table.h"
#include "upb/reflection/def.h"
#include "upb/reflection/mini_descriptor_encode.h"

// Must be last.
#include "upb/port_def.inc"

/******************************************************************************/

// Kitchen sink storage for all of our state as we build the mini descriptors.

typedef struct {
  upb_Arena* arena;
  upb_Status* status;
  upb_DefPool* symtab;

  upbc_CodeGeneratorRequest* out;

  jmp_buf jmp;
} upbc_State;

static void upbc_State_Fini(upbc_State* s) {
  if (s->symtab) upb_DefPool_Free(s->symtab);
}

UPB_NORETURN static void upbc_Error(upbc_State* s, const char* fn,
                                    const char* msg) {
  upb_Status_SetErrorFormat(s->status, "%s(): %s", fn, msg);
  upbc_State_Fini(s);
  UPB_LONGJMP(s->jmp, -1);
}

static void upbc_State_Init(upbc_State* s) {
  s->symtab = upb_DefPool_New();
  if (!s->symtab) upbc_Error(s, __func__, "could not allocate def pool");

  s->out = upbc_CodeGeneratorRequest_new(s->arena);
  if (!s->out) upbc_Error(s, __func__, "could not allocate request");
}

static void upbc_State_Emit(upbc_State* s, const char* name, const char* data) {
  const upb_StringView key = upb_StringView_FromString(name);
  const upb_StringView encoding = upb_StringView_FromString(data);
  bool ok = upbc_CodeGeneratorRequest_mini_descriptors_set(s->out, key,
                                                           encoding, s->arena);
  if (!ok) upbc_Error(s, __func__, "could not set mini descriptor in map");
}

/******************************************************************************/

// Forward declaration.
static void upbc_Scrape_Message(upbc_State*, const upb_MessageDef*);

static void upbc_Scrape_Enum(upbc_State* s, const upb_EnumDef* e) {
  const char* desc = upb_MiniDescriptor_EncodeEnum(e, s->arena);
  if (!desc) upbc_Error(s, __func__, "could not encode enum");

  upbc_State_Emit(s, upb_EnumDef_FullName(e), desc);
}

static void upbc_Scrape_Extension(upbc_State* s, const upb_FieldDef* f) {
  const char* desc = upb_MiniDescriptor_EncodeField(f, s->arena);
  if (!desc) upbc_Error(s, __func__, "could not encode extension");

  upbc_State_Emit(s, upb_FieldDef_FullName(f), desc);
}

static void upbc_Scrape_FileEnums(upbc_State* s, const upb_FileDef* f) {
  const size_t len = upb_FileDef_TopLevelEnumCount(f);

  for (size_t i = 0; i < len; i++) {
    upbc_Scrape_Enum(s, upb_FileDef_TopLevelEnum(f, i));
  }
}

static void upbc_Scrape_FileExtensions(upbc_State* s, const upb_FileDef* f) {
  const size_t len = upb_FileDef_TopLevelExtensionCount(f);

  for (size_t i = 0; i < len; i++) {
    upbc_Scrape_Extension(s, upb_FileDef_TopLevelExtension(f, i));
  }
}

static void upbc_Scrape_FileMessages(upbc_State* s, const upb_FileDef* f) {
  const size_t len = upb_FileDef_TopLevelMessageCount(f);

  for (size_t i = 0; i < len; i++) {
    upbc_Scrape_Message(s, upb_FileDef_TopLevelMessage(f, i));
  }
}

static void upbc_Scrape_File(upbc_State* s, const upb_FileDef* f) {
  upbc_Scrape_FileEnums(s, f);
  upbc_Scrape_FileExtensions(s, f);
  upbc_Scrape_FileMessages(s, f);
}

static void upbc_Scrape_Files(upbc_State* s) {
  const google_protobuf_compiler_CodeGeneratorRequest* request =
      upbc_CodeGeneratorRequest_request(s->out);

  size_t len = 0;
  const google_protobuf_FileDescriptorProto* const* files =
      google_protobuf_compiler_CodeGeneratorRequest_proto_file(request, &len);

  for (size_t i = 0; i < len; i++) {
    const upb_FileDef* f = upb_DefPool_AddFile(s->symtab, files[i], s->status);
    if (!f) upbc_Error(s, __func__, "could not add file to def pool");

    upbc_Scrape_File(s, f);
  }
}

static void upbc_Scrape_NestedEnums(upbc_State* s, const upb_MessageDef* m) {
  const size_t len = upb_MessageDef_NestedEnumCount(m);

  for (size_t i = 0; i < len; i++) {
    upbc_Scrape_Enum(s, upb_MessageDef_NestedEnum(m, i));
  }
}

static void upbc_Scrape_NestedExtensions(upbc_State* s,
                                         const upb_MessageDef* m) {
  const size_t len = upb_MessageDef_NestedExtensionCount(m);

  for (size_t i = 0; i < len; i++) {
    upbc_Scrape_Extension(s, upb_MessageDef_NestedExtension(m, i));
  }
}

static void upbc_Scrape_NestedMessages(upbc_State* s, const upb_MessageDef* m) {
  const size_t len = upb_MessageDef_NestedMessageCount(m);

  for (size_t i = 0; i < len; i++) {
    upbc_Scrape_Message(s, upb_MessageDef_NestedMessage(m, i));
  }
}

static void upbc_Scrape_Message(upbc_State* s, const upb_MessageDef* m) {
  const char* desc = upb_MiniDescriptor_EncodeMessage(m, s->arena);
  if (!desc) upbc_Error(s, __func__, "could not encode message");

  upbc_State_Emit(s, upb_MessageDef_FullName(m), desc);

  upbc_Scrape_NestedEnums(s, m);
  upbc_Scrape_NestedExtensions(s, m);
  upbc_Scrape_NestedMessages(s, m);
}

upbc_CodeGeneratorRequest* upbc_MakeCodeGeneratorRequest(
    google_protobuf_compiler_CodeGeneratorRequest* request, upb_Arena* arena,
    upb_Status* status) {
  upbc_State s = {
      .arena = arena,
      .status = status,
      .symtab = NULL,
      .out = NULL,
  };

  if (UPB_SETJMP(s.jmp)) return NULL;
  upbc_State_Init(&s);

  upbc_CodeGeneratorRequest_set_request(s.out, request);
  upbc_Scrape_Files(&s);
  upbc_State_Fini(&s);
  return s.out;
}
