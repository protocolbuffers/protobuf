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

#include <assert.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "google/protobuf/compiler/plugin.upb.h"
#include "upb/def.h"
#include "upb/mini_descriptor.h"
#include "upb/mini_table.h"

// Must be last.
#include "upb/port_def.inc"

enum {
  kErrArenaMalloc = 1,
  kErrEnumName,
  kErrExtensionName,
  kErrFieldName,
  kErrFilePackage,
  kErrMapCollision,
  kErrMiniDescriptorsSet,
  kErrStateGrow,
};

/* upbc_PathState *************************************************************/

// Manages the current fully qualified path name as we dig down into a proto.
// Basically just a string that grows and shrinks like a stack.

typedef struct {
  size_t len;
  char path[4000];  // TODO(salo): make this dynamic
} upbc_PathState;

static void upbc_PathState_Init(upbc_PathState* p) { p->len = 0; }

static void upbc_PathState_Push(upbc_PathState* p, upb_StringView name) {
  if (p->len) {
    p->path[p->len++] = '.';
  }
  memcpy(&p->path[p->len], name.data, name.size);
  p->len += name.size;
}

static void upbc_PathState_Pop(upbc_PathState* p, upb_StringView name) {
  p->len -= name.size;
  if (p->len) {
    p->len--;
  }
}

static upb_StringView upbc_PathState_String(const upbc_PathState* p) {
  return upb_StringView_FromDataAndSize(p->path, p->len);
}

/******************************************************************************/

// Kitchen sink storage for the mini descriptor state.

typedef struct {
  upb_Arena* a;
  upb_Syntax syntax;

  upbc_CodeGeneratorRequest* out;

  jmp_buf err;

  upbc_PathState path;
} upbc_ScrapeState;

static void upbc_ScrapeState_Init(upbc_ScrapeState* s, upb_Arena* a) {
  s->a = a;

  upbc_PathState_Init(&s->path);

  s->out = upbc_CodeGeneratorRequest_new(a);
  if (!s->out) UPB_LONGJMP(s->err, kErrArenaMalloc);
}

static void upbc_ScrapeState_Push(upbc_ScrapeState* s, upb_StringView name) {
  upbc_PathState_Push(&s->path, name);

  const upb_StringView key = upbc_PathState_String(&s->path);
  if (upbc_CodeGeneratorRequest_mini_descriptors_get(s->out, key, NULL)) {
    UPB_LONGJMP(s->err, kErrMapCollision);
  }
}

static void upbc_ScrapeState_Pop(upbc_ScrapeState* s, upb_StringView name) {
  upbc_PathState_Pop(&s->path, name);
}

static void upbc_ScrapeState_String(upbc_ScrapeState* s,
                                    upb_StringView encoding) {
  const upb_StringView path = upbc_PathState_String(&s->path);
  bool ok = upbc_CodeGeneratorRequest_mini_descriptors_set(s->out, path,
                                                           encoding, s->a);
  if (!ok) UPB_LONGJMP(s->err, kErrMiniDescriptorsSet);
}

/******************************************************************************/

// File accessors.

static upb_Syntax upbc_File_Syntax(const google_protobuf_FileDescriptorProto* file) {
  if (google_protobuf_FileDescriptorProto_has_syntax(file)) {
    const upb_StringView syntax = google_protobuf_FileDescriptorProto_syntax(file);
    const upb_StringView proto3 = upb_StringView_FromString("proto3");
    if (upb_StringView_IsEqual(syntax, proto3)) return kUpb_Syntax_Proto3;
  }
  return kUpb_Syntax_Proto2;
}

/******************************************************************************/

// Forward declaration.
static void upbc_Scrape_Messages(upbc_ScrapeState*,
                                 const google_protobuf_DescriptorProto* const*, size_t);

static void upbc_Scrape_Enum(upbc_ScrapeState* s,
                             const google_protobuf_EnumDescriptorProto* enum_type) {
  if (!google_protobuf_EnumDescriptorProto_has_name(enum_type)) {
    UPB_LONGJMP(s->err, kErrEnumName);
  }
  const upb_StringView name = google_protobuf_EnumDescriptorProto_name(enum_type);

  upbc_ScrapeState_Push(s, name);

  const upb_StringView encoding =
      upb_MiniDescriptor_EncodeEnum(enum_type, s->a);

  upbc_ScrapeState_String(s, encoding);
  upbc_ScrapeState_Pop(s, name);
}

static void upbc_Scrape_Enums(
    upbc_ScrapeState* s, const google_protobuf_EnumDescriptorProto* const* enum_types,
    size_t len) {
  for (size_t i = 0; i < len; i++) {
    upbc_Scrape_Enum(s, enum_types[i]);
  }
}

static void upbc_Scrape_Extension(
    upbc_ScrapeState* s, const google_protobuf_FieldDescriptorProto* extension_type) {
  if (!google_protobuf_FieldDescriptorProto_has_name(extension_type)) {
    UPB_LONGJMP(s->err, kErrExtensionName);
  }
  const upb_StringView name = google_protobuf_FieldDescriptorProto_name(extension_type);

  upbc_ScrapeState_Push(s, name);

  const upb_StringView encoding =
      upb_MiniDescriptor_EncodeExtension(extension_type, s->syntax, s->a);

  upbc_ScrapeState_String(s, encoding);
  upbc_ScrapeState_Pop(s, name);
}

static void upbc_Scrape_Extensions(
    const google_protobuf_FieldDescriptorProto* const* extension_types, size_t len,
    upbc_ScrapeState* s) {
  for (size_t i = 0; i < len; i++) {
    upbc_Scrape_Extension(s, extension_types[i]);
  }
}

static void upbc_Scrape_File(upbc_ScrapeState* s,
                             const google_protobuf_FileDescriptorProto* file_type) {
  if (!google_protobuf_FileDescriptorProto_has_package(file_type)) {
    UPB_LONGJMP(s->err, kErrFilePackage);
  }
  const upb_StringView package = google_protobuf_FileDescriptorProto_package(file_type);
  upbc_ScrapeState_Push(s, package);

  s->syntax = upbc_File_Syntax(file_type);

  size_t len = 0;
  const google_protobuf_EnumDescriptorProto* const* enum_types =
      google_protobuf_FileDescriptorProto_enum_type(file_type, &len);
  upbc_Scrape_Enums(s, enum_types, len);

  const google_protobuf_FieldDescriptorProto* const* extension_types =
      google_protobuf_FileDescriptorProto_extension(file_type, &len);
  upbc_Scrape_Extensions(extension_types, len, s);

  const google_protobuf_DescriptorProto* const* message_types =
      google_protobuf_FileDescriptorProto_message_type(file_type, &len);
  upbc_Scrape_Messages(s, message_types, len);

  upbc_ScrapeState_Pop(s, package);
}

static void upbc_Scrape_Files(
    upbc_ScrapeState* s, const google_protobuf_FileDescriptorProto* const* file_types,
    size_t len) {
  for (size_t i = 0; i < len; i++) {
    upbc_Scrape_File(s, file_types[i]);
  }
}

static void upbc_Scrape_Message(upbc_ScrapeState* s,
                                const google_protobuf_DescriptorProto* message_type) {
  if (!google_protobuf_DescriptorProto_has_name(message_type)) return;

  const upb_StringView name = google_protobuf_DescriptorProto_name(message_type);
  upbc_ScrapeState_Push(s, name);

  const upb_StringView encoding =
      upb_MiniDescriptor_EncodeMessage(message_type, s->syntax, s->a);
  upbc_ScrapeState_String(s, encoding);

  size_t len = 0;
  const google_protobuf_EnumDescriptorProto* const* enum_types =
      google_protobuf_DescriptorProto_enum_type(message_type, &len);
  upbc_Scrape_Enums(s, enum_types, len);

  const google_protobuf_FieldDescriptorProto* const* extension_types =
      google_protobuf_DescriptorProto_extension(message_type, &len);
  upbc_Scrape_Extensions(extension_types, len, s);

  const google_protobuf_DescriptorProto* const* nested_types =
      google_protobuf_DescriptorProto_nested_type(message_type, &len);
  upbc_Scrape_Messages(s, nested_types, len);

  upbc_ScrapeState_Pop(s, name);
}

static void upbc_Scrape_Messages(
    upbc_ScrapeState* s, const google_protobuf_DescriptorProto* const* message_types,
    size_t len) {
  for (size_t i = 0; i < len; i++) {
    upbc_Scrape_Message(s, message_types[i]);
  }
}

upbc_CodeGeneratorRequest* upbc_MakeCodeGeneratorRequest(
    google_protobuf_compiler_CodeGeneratorRequest* request, upb_Arena* a,
    upb_Status* status) {
  upbc_ScrapeState s;
  int err = UPB_SETJMP(s.err);
  if (err) {
    upb_Status_SetErrorFormat(status, "%s(): error %d", __func__, err);
    return NULL;
  }
  upbc_ScrapeState_Init(&s, a);

  size_t len = 0;
  const google_protobuf_FileDescriptorProto* const* file_types =
      google_protobuf_compiler_CodeGeneratorRequest_proto_file(request, &len);
  upbc_Scrape_Files(&s, file_types, len);

  upbc_CodeGeneratorRequest_set_request(s.out, request);
  return s.out;
}
