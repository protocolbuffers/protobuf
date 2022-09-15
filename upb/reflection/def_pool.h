/*
 * Copyright (c) 2009-2021, Google LLC
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

// IWYU pragma: private, include "third_party/upb/upb/reflection/def.h"

#ifndef UPB_REFLECTION_DEF_POOL_H_
#define UPB_REFLECTION_DEF_POOL_H_

#include "upb/reflection/common.h"
#include "upb/reflection/def_type.h"
#include "upb/string_view.h"

// Must be last.
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

void upb_DefPool_Free(upb_DefPool* s);

upb_DefPool* upb_DefPool_New(void);

const upb_MessageDef* upb_DefPool_FindMessageByName(const upb_DefPool* s,
                                                    const char* sym);

const upb_MessageDef* upb_DefPool_FindMessageByNameWithSize(
    const upb_DefPool* s, const char* sym, size_t len);

const upb_EnumDef* upb_DefPool_FindEnumByName(const upb_DefPool* s,
                                              const char* sym);

const upb_EnumValueDef* upb_DefPool_FindEnumByNameval(const upb_DefPool* s,
                                                      const char* sym);

const upb_FileDef* upb_DefPool_FindFileByName(const upb_DefPool* s,
                                              const char* name);

const upb_FileDef* upb_DefPool_FindFileByNameWithSize(const upb_DefPool* s,
                                                      const char* name,
                                                      size_t len);

const upb_FieldDef* upb_DefPool_FindExtensionByNameWithSize(
    const upb_DefPool* s, const char* name, size_t size);

const upb_FieldDef* upb_DefPool_FindExtensionByName(const upb_DefPool* s,
                                                    const char* sym);

const upb_ServiceDef* upb_DefPool_FindServiceByName(const upb_DefPool* s,
                                                    const char* name);

const upb_ServiceDef* upb_DefPool_FindServiceByNameWithSize(
    const upb_DefPool* s, const char* name, size_t size);

const upb_FileDef* upb_DefPool_FindFileContainingSymbol(const upb_DefPool* s,
                                                        const char* name);

const upb_FileDef* upb_DefPool_AddFile(
    upb_DefPool* s, const google_protobuf_FileDescriptorProto* file_proto,
    upb_Status* status);

const upb_FieldDef* upb_DefPool_FindExtensionByNumber(const upb_DefPool* s,
                                                      const upb_MessageDef* m,
                                                      int32_t fieldnum);

const upb_ExtensionRegistry* upb_DefPool_ExtensionRegistry(
    const upb_DefPool* s);

const upb_FieldDef** upb_DefPool_GetAllExtensions(const upb_DefPool* s,
                                                  const upb_MessageDef* m,
                                                  size_t* count);

// EVERYTHING BELOW THIS LINE IS INTERNAL - DO NOT USE /////////////////////////

// For generated code only: loads a generated descriptor.
typedef struct _upb_DefPool_Init {
  struct _upb_DefPool_Init** deps;  // Dependencies of this file.
  const upb_MiniTable_File* layout;
  const char* filename;
  upb_StringView descriptor;  // Serialized descriptor.
} _upb_DefPool_Init;

upb_Arena* _upb_DefPool_Arena(const upb_DefPool* s);
size_t _upb_DefPool_BytesLoaded(const upb_DefPool* s);

bool _upb_DefPool_Contains(const upb_DefPool* s, const char* sym);

upb_ExtensionRegistry* _upb_DefPool_ExtReg(const upb_DefPool* s);

bool _upb_DefPool_Insert(upb_DefPool* s, const char* sym, upb_value v);
bool _upb_DefPool_Insert2(upb_DefPool* s, const char* sym, size_t size,
                          upb_value v);

bool _upb_DefPool_InsertExt(upb_DefPool* s, const upb_MiniTable_Extension* ext,
                            upb_FieldDef* f, upb_Arena* a);

const void* _upb_DefPool_Lookup2(const upb_DefPool* s, const char* sym,
                                 size_t size, upb_deftype_t type);

bool _upb_DefPool_LookupAny2(const upb_DefPool* s, const char* sym, size_t size,
                             upb_value* v);

const upb_FieldDef* _upb_DefPool_FindExtensionByMiniTable(
    const upb_DefPool* s, const upb_MiniTable_Extension* ext);

// Should only be directly called by tests. This variant lets us suppress
// the use of compiled-in tables, forcing a rebuild of the tables at runtime.
bool _upb_DefPool_LoadDefInitEx(upb_DefPool* s, const _upb_DefPool_Init* init,
                                bool rebuild_minitable);

bool _upb_DefPool_LoadDefInit(upb_DefPool* s, const _upb_DefPool_Init* init);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif /* UPB_REFLECTION_DEF_POOL_H_ */
