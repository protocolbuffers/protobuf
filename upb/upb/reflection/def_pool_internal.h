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

#ifndef UPB_REFLECTION_DEF_POOL_INTERNAL_H_
#define UPB_REFLECTION_DEF_POOL_INTERNAL_H_

#include "upb/mini_descriptor/decode.h"
#include "upb/reflection/def_pool.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

upb_Arena* _upb_DefPool_Arena(const upb_DefPool* s);
size_t _upb_DefPool_BytesLoaded(const upb_DefPool* s);
upb_ExtensionRegistry* _upb_DefPool_ExtReg(const upb_DefPool* s);

bool _upb_DefPool_InsertExt(upb_DefPool* s, const upb_MiniTableExtension* ext,
                            const upb_FieldDef* f);
bool _upb_DefPool_InsertSym(upb_DefPool* s, upb_StringView sym, upb_value v,
                            upb_Status* status);
bool _upb_DefPool_LookupSym(const upb_DefPool* s, const char* sym, size_t size,
                            upb_value* v);

void** _upb_DefPool_ScratchData(const upb_DefPool* s);
size_t* _upb_DefPool_ScratchSize(const upb_DefPool* s);
void _upb_DefPool_SetPlatform(upb_DefPool* s, upb_MiniTablePlatform platform);

// For generated code only: loads a generated descriptor.
typedef struct _upb_DefPool_Init {
  struct _upb_DefPool_Init** deps;  // Dependencies of this file.
  const upb_MiniTableFile* layout;
  const char* filename;
  upb_StringView descriptor;  // Serialized descriptor.
} _upb_DefPool_Init;

bool _upb_DefPool_LoadDefInit(upb_DefPool* s, const _upb_DefPool_Init* init);

// Should only be directly called by tests. This variant lets us suppress
// the use of compiled-in tables, forcing a rebuild of the tables at runtime.
bool _upb_DefPool_LoadDefInitEx(upb_DefPool* s, const _upb_DefPool_Init* init,
                                bool rebuild_minitable);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_REFLECTION_DEF_POOL_INTERNAL_H_ */
