// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb_generator/get_used_fields.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "google/protobuf/descriptor.upb.h"
#include "google/protobuf/compiler/plugin.upb.h"
#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/mini_table/message.h"
#include "upb/reflection/def.h"
#include "upb/reflection/message.h"
#include "upb/wire/decode.h"

// Must be last.
#include "upb/port/def.inc"

#define upbdev_Err(...)           \
  {                               \
    fprintf(stderr, __VA_ARGS__); \
    exit(1);                      \
  }

typedef struct {
  char* buf;
  size_t size;
  size_t capacity;
  upb_Arena* arena;
} upbdev_StringBuf;

void upbdev_StringBuf_Add(upbdev_StringBuf* buf, const char* sym) {
  size_t len = strlen(sym);
  size_t need = buf->size + len + (buf->size != 0);
  if (need > buf->capacity) {
    size_t new_cap = UPB_MAX(buf->capacity, 32);
    while (need > new_cap) new_cap *= 2;
    buf->buf = upb_Arena_Realloc(buf->arena, buf->buf, buf->capacity, new_cap);
    buf->capacity = new_cap;
  }
  if (buf->size != 0) {
    buf->buf[buf->size++] = '\n';  // Separator
  }
  memcpy(buf->buf + buf->size, sym, len);
  buf->size = need;
}

void upbdev_VisitMessage(upbdev_StringBuf* buf, const upb_Message* msg,
                         const upb_MessageDef* m) {
  size_t iter = kUpb_Message_Begin;
  const upb_FieldDef* f;
  upb_MessageValue val;
  while (upb_Message_Next(msg, m, NULL, &f, &val, &iter)) {
    // This could be a duplicate, but we don't worry about it; we'll dedupe
    // one level up.
    upbdev_StringBuf_Add(buf, upb_FieldDef_FullName(f));

    if (upb_FieldDef_CType(f) != kUpb_CType_Message) continue;
    const upb_MessageDef* sub = upb_FieldDef_MessageSubDef(f);

    if (upb_FieldDef_IsMap(f)) {
      const upb_Map* map = val.map_val;
      size_t iter = kUpb_Map_Begin;
      upb_MessageValue map_key, map_val;
      while (upb_Map_Next(map, &map_key, &map_val, &iter)) {
        upbdev_VisitMessage(buf, map_val.msg_val, sub);
      }
    } else if (upb_FieldDef_IsRepeated(f)) {
      const upb_Array* arr = val.array_val;
      size_t n = upb_Array_Size(arr);
      for (size_t i = 0; i < n; i++) {
        upb_MessageValue val = upb_Array_Get(arr, i);
        upbdev_VisitMessage(buf, val.msg_val, sub);
      }
    } else {
      upbdev_VisitMessage(buf, val.msg_val, sub);
    }
  }
}

upb_StringView upbdev_GetUsedFields(const char* request, size_t request_size,
                                    const char* payload, size_t payload_size,
                                    const char* message_name,
                                    upb_Arena* arena) {
  upb_Arena* tmp_arena = upb_Arena_New();
  google_protobuf_compiler_CodeGeneratorRequest* request_proto =
      google_protobuf_compiler_CodeGeneratorRequest_parse(request, request_size,
                                                 tmp_arena);
  if (!request_proto) upbdev_Err("Couldn't parse request proto\n");

  size_t len;
  const google_protobuf_FileDescriptorProto* const* files =
      google_protobuf_compiler_CodeGeneratorRequest_proto_file(request_proto, &len);

  upb_DefPool* pool = upb_DefPool_New();
  for (size_t i = 0; i < len; i++) {
    const upb_FileDef* f = upb_DefPool_AddFile(pool, files[i], NULL);
    if (!f) upbdev_Err("could not add file to def pool\n");
  }

  const upb_MessageDef* m = upb_DefPool_FindMessageByName(pool, message_name);
  if (!m) upbdev_Err("Couldn't find message name\n");

  const upb_MiniTable* mt = upb_MessageDef_MiniTable(m);
  upb_Message* msg = upb_Message_New(mt, tmp_arena);
  upb_DecodeStatus st =
      upb_Decode(payload, payload_size, msg, mt, NULL, 0, tmp_arena);
  if (st != kUpb_DecodeStatus_Ok) upbdev_Err("Error parsing payload: %d\n", st);

  upbdev_StringBuf buf = {
      .buf = NULL,
      .size = 0,
      .capacity = 0,
      .arena = arena,
  };
  upbdev_VisitMessage(&buf, msg, m);
  return upb_StringView_FromDataAndSize(buf.buf, buf.size);
}
