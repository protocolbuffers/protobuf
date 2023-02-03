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

// IWYU pragma: private, include "upb/reflection/def.h"

#ifndef UPB_REFLECTION_FILE_DEF_H_
#define UPB_REFLECTION_FILE_DEF_H_

#include "upb/reflection/common.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

const upb_FileDef* upb_FileDef_Dependency(const upb_FileDef* f, int i);
int upb_FileDef_DependencyCount(const upb_FileDef* f);
bool upb_FileDef_HasOptions(const upb_FileDef* f);
const char* upb_FileDef_Name(const upb_FileDef* f);
const UPB_DESC(FileOptions) * upb_FileDef_Options(const upb_FileDef* f);
const char* upb_FileDef_Package(const upb_FileDef* f);
const char* upb_FileDef_Edition(const upb_FileDef* f);
const upb_DefPool* upb_FileDef_Pool(const upb_FileDef* f);

const upb_FileDef* upb_FileDef_PublicDependency(const upb_FileDef* f, int i);
int upb_FileDef_PublicDependencyCount(const upb_FileDef* f);

const upb_ServiceDef* upb_FileDef_Service(const upb_FileDef* f, int i);
int upb_FileDef_ServiceCount(const upb_FileDef* f);

upb_Syntax upb_FileDef_Syntax(const upb_FileDef* f);

const upb_EnumDef* upb_FileDef_TopLevelEnum(const upb_FileDef* f, int i);
int upb_FileDef_TopLevelEnumCount(const upb_FileDef* f);

const upb_FieldDef* upb_FileDef_TopLevelExtension(const upb_FileDef* f, int i);
int upb_FileDef_TopLevelExtensionCount(const upb_FileDef* f);

const upb_MessageDef* upb_FileDef_TopLevelMessage(const upb_FileDef* f, int i);
int upb_FileDef_TopLevelMessageCount(const upb_FileDef* f);

const upb_FileDef* upb_FileDef_WeakDependency(const upb_FileDef* f, int i);
int upb_FileDef_WeakDependencyCount(const upb_FileDef* f);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_REFLECTION_FILE_DEF_H_ */
