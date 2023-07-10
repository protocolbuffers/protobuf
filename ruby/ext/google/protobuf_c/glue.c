// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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
//     * Neither the name of Google Inc. nor the names of its
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

// -----------------------------------------------------------------------------
// Exposing inlined UPB functions. Strictly free of dependencies on
// Ruby interpreter internals.

#include "ruby-upb.h"

upb_Arena* Arena_create() {
  return upb_Arena_Init(NULL, 0, &upb_alloc_global);
}

google_protobuf_FileDescriptorProto* FileDescriptorProto_parse (const char* serialized_file_proto, size_t length) {
  upb_Arena* arena = Arena_create();
  return google_protobuf_FileDescriptorProto_parse(
          serialized_file_proto,
          length, arena);
}

bool API_PENDING_upb_MapIterator_Next(const upb_Map* map, size_t* iter) {
  return upb_MapIterator_Next(map, iter);
}
bool API_PENDING_upb_MapIterator_Done(const upb_Map* map, size_t iter) {
  return upb_MapIterator_Done(map, iter);
}
upb_MessageValue API_PENDING_upb_MapIterator_Key(const upb_Map* map, size_t iter) {
  return upb_MapIterator_Key(map, iter);
}
upb_MessageValue API_PENDING_upb_MapIterator_Value(const upb_Map* map, size_t iter) {
  return upb_MapIterator_Value(map, iter);
}
upb_EncodeStatus API_PENDING_upb_Encode(const void* msg, const upb_MiniTable* l,
                            int options, upb_Arena* arena, char** buf,
                            size_t* size) {
  return upb_Encode(msg, l, options, arena, buf, size);
}

void API_PENDING_upb_DefPool_Free(upb_DefPool* s) {
    return upb_DefPool_Free(s);
}

upb_DefPool* API_PENDING_upb_DefPool_New(void) {
    return upb_DefPool_New();
}

const upb_MessageDef* API_PENDING_upb_DefPool_FindMessageByName(const upb_DefPool* s,
                                                    const char* sym) {
  return upb_DefPool_FindMessageByName(s, sym);
}

const upb_EnumDef* API_PENDING_upb_DefPool_FindEnumByName(const upb_DefPool* s,
                                              const char* sym) {
  return upb_DefPool_FindEnumByName(s, sym);
}
const upb_FileDef* API_PENDING_upb_DefPool_AddFile(upb_DefPool* s,
                                       void *
                                           file_proto,
                                       upb_Status* status) {
  return upb_DefPool_AddFile(s, file_proto, status);
}
const upb_FileDef* API_PENDING_upb_EnumDef_File(const upb_EnumDef* e) {
  return upb_EnumDef_File(e);
}
const upb_EnumValueDef* API_PENDING_upb_EnumDef_FindValueByNameWithSize(
    const upb_EnumDef* e, const char* name, size_t size) {
  return upb_EnumDef_FindValueByNameWithSize(e, name, size);
}
const upb_EnumValueDef* API_PENDING_upb_EnumDef_FindValueByNumber(const upb_EnumDef* e,
                                                      int32_t num) {
  return upb_EnumDef_FindValueByNumber(e, num);
}
const char* API_PENDING_upb_EnumDef_FullName(const upb_EnumDef* e) {
    return upb_EnumDef_FullName(e);
}
const upb_EnumValueDef* API_PENDING_upb_EnumDef_Value(const upb_EnumDef* e, int i) {
    return upb_EnumDef_Value(e, i);
}
int API_PENDING_upb_EnumDef_ValueCount(const upb_EnumDef* e) {
    return upb_EnumDef_ValueCount(e);
}
const char* API_PENDING_upb_EnumValueDef_Name(const upb_EnumValueDef* v) {
    return upb_EnumValueDef_Name(v);
}
int32_t API_PENDING_upb_EnumValueDef_Number(const upb_EnumValueDef* v) {
    return upb_EnumValueDef_Number(v);
}
const upb_MessageDef* API_PENDING_upb_FieldDef_ContainingType(const upb_FieldDef* f) {
    return upb_FieldDef_ContainingType(f);
}
upb_CType API_PENDING_upb_FieldDef_CType(const upb_FieldDef* f) {
    return upb_FieldDef_CType(f);
}
upb_MessageValue API_PENDING_upb_FieldDef_Default(const upb_FieldDef* f) {
    return upb_FieldDef_Default(f);
}
const upb_EnumDef* API_PENDING_upb_FieldDef_EnumSubDef(const upb_FieldDef* f) {
    return upb_FieldDef_EnumSubDef(f);
}
const upb_FileDef* API_PENDING_upb_FieldDef_File(const upb_FieldDef* f) {
    return upb_FieldDef_File(f);
}
bool API_PENDING_upb_FieldDef_HasPresence(const upb_FieldDef* f) {
    return upb_FieldDef_HasPresence(f);
}
bool API_PENDING_upb_FieldDef_IsMap(const upb_FieldDef* f) {
    return upb_FieldDef_IsMap(f);
}
bool API_PENDING_upb_FieldDef_IsRepeated(const upb_FieldDef* f) {
    return upb_FieldDef_IsRepeated(f);
}
bool API_PENDING_upb_FieldDef_IsSubMessage(const upb_FieldDef* f) {
    return upb_FieldDef_IsSubMessage(f);
}
const char* API_PENDING_upb_FieldDef_JsonName(const upb_FieldDef* f) {
    return upb_FieldDef_JsonName(f);
}
upb_Label API_PENDING_upb_FieldDef_Label(const upb_FieldDef* f) {
    return upb_FieldDef_Label(f);
}
const upb_MessageDef* API_PENDING_upb_FieldDef_MessageSubDef(const upb_FieldDef* f) {
    return upb_FieldDef_MessageSubDef(f);
}
const char* API_PENDING_upb_FieldDef_Name(const upb_FieldDef* f) {
    return upb_FieldDef_Name(f);
}
uint32_t API_PENDING_upb_FieldDef_Number(const upb_FieldDef* f) {
    return upb_FieldDef_Number(f);
}
const upb_OneofDef* API_PENDING_upb_FieldDef_RealContainingOneof(const upb_FieldDef* f) {
    return upb_FieldDef_RealContainingOneof(f);
}
upb_FieldType API_PENDING_upb_FieldDef_Type(const upb_FieldDef* f) {
    return upb_FieldDef_Type(f);
}
const char* API_PENDING_upb_FileDef_Name(const upb_FileDef* f) {
    return upb_FileDef_Name(f);
}
const upb_DefPool* API_PENDING_upb_FileDef_Pool(const upb_FileDef* f) {
    return upb_FileDef_Pool(f);
}
upb_Syntax API_PENDING_upb_FileDef_Syntax(const upb_FileDef* f) {
    return upb_FileDef_Syntax(f);
}
const upb_FieldDef* API_PENDING_upb_MessageDef_Field(const upb_MessageDef* m, int i) {
    return upb_MessageDef_Field(m, i);
}
int API_PENDING_upb_MessageDef_FieldCount(const upb_MessageDef* m) {
    return upb_MessageDef_FieldCount(m);
}
const upb_FileDef* API_PENDING_upb_MessageDef_File(const upb_MessageDef* m) {
    return upb_MessageDef_File(m);
}
bool API_PENDING_upb_MessageDef_FindByNameWithSize(const upb_MessageDef* m,
                                       const char* name, size_t size,
                                       const upb_FieldDef** f,
                                       const upb_OneofDef** o) {
  return upb_MessageDef_FindByNameWithSize(m, name, size, f, o);
}
const upb_FieldDef* API_PENDING_upb_MessageDef_FindFieldByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t size) {
  return upb_MessageDef_FindFieldByNameWithSize(m, name, size);
}
const upb_FieldDef* API_PENDING_upb_MessageDef_FindFieldByNumber(const upb_MessageDef* m,
                                                     uint32_t i) {
   return upb_MessageDef_FindFieldByNumber(m, i);
}
const upb_OneofDef* API_PENDING_upb_MessageDef_FindOneofByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t size) {
  return upb_MessageDef_FindOneofByNameWithSize(m, name, size);
}
const char* API_PENDING_upb_MessageDef_FullName(const upb_MessageDef* m) {

    return upb_MessageDef_FullName(m);
}
const upb_MiniTable* API_PENDING_upb_MessageDef_MiniTable(const upb_MessageDef* m) {
    return upb_MessageDef_MiniTable(m);
}
const upb_OneofDef* API_PENDING_upb_MessageDef_Oneof(const upb_MessageDef* m, int i) {
    return upb_MessageDef_Oneof(m, i);
}
int API_PENDING_upb_MessageDef_OneofCount(const upb_MessageDef* m) {
    return upb_MessageDef_OneofCount(m);
}
upb_Syntax API_PENDING_upb_MessageDef_Syntax(const upb_MessageDef* m) {
    return upb_MessageDef_Syntax(m);
}
upb_WellKnown API_PENDING_upb_MessageDef_WellKnownType(const upb_MessageDef* m) {
    return upb_MessageDef_WellKnownType(m);
}
const upb_MessageDef* API_PENDING_upb_OneofDef_ContainingType(const upb_OneofDef* o) {
    return upb_OneofDef_ContainingType(o);
}
const upb_FieldDef* API_PENDING_upb_OneofDef_Field(const upb_OneofDef* o, int i) {
    return upb_OneofDef_Field(o, i);
}
int API_PENDING_upb_OneofDef_FieldCount(const upb_OneofDef* o) {
    return upb_OneofDef_FieldCount(o);
}
const char* API_PENDING_upb_OneofDef_Name(const upb_OneofDef* o) {
    return upb_OneofDef_Name(o);
}
bool API_PENDING_upb_JsonDecode(const char* buf, size_t size, upb_Message* msg,
                    const upb_MessageDef* m, const upb_DefPool* symtab,
                    int options, upb_Arena* arena, upb_Status* status) {
  return upb_JsonDecode(buf, size, msg, m, symtab, options, arena, status);
}
upb_MutableMessageValue API_PENDING_upb_Message_Mutable(upb_Message* msg,
                                            const upb_FieldDef* f,
                                            upb_Arena* a) {
  return upb_Message_Mutable(msg, f, a);
}
const upb_FieldDef* API_PENDING_upb_Message_WhichOneof(const upb_Message* msg,
                                           const upb_OneofDef* o) {
  return upb_Message_WhichOneof(msg, o);
}
void API_PENDING_upb_Message_ClearFieldByDef(upb_Message* msg, const upb_FieldDef* f) {
    return upb_Message_ClearFieldByDef(msg, f);
}

bool API_PENDING_upb_Message_HasFieldByDef(const upb_Message* msg, const upb_FieldDef* f) {
    return upb_Message_HasFieldByDef(msg, f);
}

upb_MessageValue API_PENDING_upb_Message_GetFieldByDef(const upb_Message* msg,
                                           const upb_FieldDef* f) {
    return upb_Message_GetFieldByDef(msg, f);
}
bool API_PENDING_upb_Message_SetFieldByDef(upb_Message* msg, const upb_FieldDef* f,
                               upb_MessageValue val, upb_Arena* a) {
  return upb_Message_SetFieldByDef(msg, f, val, a);
}
bool API_PENDING_upb_Message_DiscardUnknown(upb_Message* msg, const upb_MessageDef* m,
                                int maxdepth) {
  return upb_Message_DiscardUnknown(msg, m, maxdepth);
}
size_t API_PENDING_upb_JsonEncode(const upb_Message* msg, const upb_MessageDef* m,
                      const upb_DefPool* ext_pool, int options, char* buf,
                      size_t size, upb_Status* status) {
  return upb_JsonEncode(msg, m, ext_pool, options, buf, size, status);
}
