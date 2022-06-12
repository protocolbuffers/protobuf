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
#include <setjmp.h>
#include <stdio.h>

#include "google/protobuf/compiler/plugin.upb.h"
#include "upb/mini_descriptor.h"
#include "upb/mini_table.h"

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

static void upbc_Error(upbc_State* s, const char* fn, const char* msg) {
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

static void upbc_State_Emit(upbc_State* s, const char* name,
                            upb_StringView encoding) {
  const upb_StringView key = upb_StringView_FromString(name);
  bool ok = upbc_CodeGeneratorRequest_mini_descriptors_set(s->out, key,
                                                           encoding, s->arena);
  if (!ok) upbc_Error(s, __func__, "could not set mini descriptor in map");
}

/******************************************************************************/

// Forward declaration.
static void upbc_Scrape_Message(upbc_State*, const upb_MessageDef*);

static void upbc_Scrape_Enum(upbc_State* s, const upb_EnumDef* enum_def) {
  const char* name = upb_EnumDef_FullName(enum_def);
  const upb_StringView encoding =
      upb_MiniDescriptor_EncodeEnum(enum_def, s->arena);
  upbc_State_Emit(s, name, encoding);
}

static void upbc_Scrape_Extension(upbc_State* s,
                                  const upb_FieldDef* field_def) {
  const char* name = upb_FieldDef_FullName(field_def);
  const upb_StringView encoding =
      upb_MiniDescriptor_EncodeExtension(field_def, s->arena);
  upbc_State_Emit(s, name, encoding);
}

static void upbc_Scrape_FileEnums(upbc_State* s, const upb_FileDef* file_def) {
  const size_t len = upb_FileDef_TopLevelEnumCount(file_def);
  for (size_t i = 0; i < len; i++) {
    const upb_EnumDef* enum_def = upb_FileDef_TopLevelEnum(file_def, i);
    upbc_Scrape_Enum(s, enum_def);
  }
}

static void upbc_Scrape_FileExtensions(upbc_State* s,
                                       const upb_FileDef* file_def) {
  const size_t len = upb_FileDef_TopLevelExtensionCount(file_def);
  for (size_t i = 0; i < len; i++) {
    const upb_FieldDef* field_def = upb_FileDef_TopLevelExtension(file_def, i);
    upbc_Scrape_Extension(s, field_def);
  }
}

static void upbc_Scrape_FileMessages(upbc_State* s,
                                     const upb_FileDef* file_def) {
  const size_t len = upb_FileDef_TopLevelMessageCount(file_def);
  for (size_t i = 0; i < len; i++) {
    const upb_MessageDef* message_def =
        upb_FileDef_TopLevelMessage(file_def, i);
    upbc_Scrape_Message(s, message_def);
  }
}

static void upbc_Scrape_File(upbc_State* s, const upb_FileDef* file_def) {
  upbc_Scrape_FileEnums(s, file_def);
  upbc_Scrape_FileExtensions(s, file_def);
  upbc_Scrape_FileMessages(s, file_def);
}

static void upbc_Scrape_Files(upbc_State* s) {
  const google_protobuf_compiler_CodeGeneratorRequest* request =
      upbc_CodeGeneratorRequest_request(s->out);

  size_t len = 0;
  const google_protobuf_FileDescriptorProto* const* file_types =
      google_protobuf_compiler_CodeGeneratorRequest_proto_file(request, &len);

  for (size_t i = 0; i < len; i++) {
    const upb_FileDef* file_def =
        upb_DefPool_AddFile(s->symtab, file_types[i], s->status);
    if (!file_def) upbc_Error(s, __func__, "could not add file to def pool");

    upbc_Scrape_File(s, file_def);
  }
}

static void upbc_Scrape_NestedEnums(upbc_State* s,
                                    const upb_MessageDef* message_def) {
  const size_t len = upb_MessageDef_NestedEnumCount(message_def);
  for (size_t i = 0; i < len; i++) {
    const upb_EnumDef* enum_def = upb_MessageDef_NestedEnum(message_def, i);
    upbc_Scrape_Enum(s, enum_def);
  }
}

static void upbc_Scrape_NestedExtensions(upbc_State* s,
                                         const upb_MessageDef* message_def) {
  const size_t len = upb_MessageDef_NestedExtensionCount(message_def);
  for (size_t i = 0; i < len; i++) {
    const upb_FieldDef* field_def =
        upb_MessageDef_NestedExtension(message_def, i);
    upbc_Scrape_Extension(s, field_def);
  }
}

static void upbc_Scrape_NestedMessages(upbc_State* s,
                                       const upb_MessageDef* message_def) {
  const size_t len = upb_MessageDef_NestedMessageCount(message_def);
  for (size_t i = 0; i < len; i++) {
    const upb_MessageDef* nested_def =
        upb_MessageDef_NestedMessage(message_def, i);
    upbc_Scrape_Message(s, nested_def);
  }
}

static void upbc_Scrape_Message(upbc_State* s,
                                const upb_MessageDef* message_def) {
  const char* name = upb_MessageDef_FullName(message_def);
  const upb_StringView encoding =
      upb_MiniDescriptor_EncodeMessage(message_def, s->arena);
  upbc_State_Emit(s, name, encoding);

  upbc_Scrape_NestedEnums(s, message_def);
  upbc_Scrape_NestedExtensions(s, message_def);
  upbc_Scrape_NestedMessages(s, message_def);
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
