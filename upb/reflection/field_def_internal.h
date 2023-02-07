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

#ifndef UPB_REFLECTION_FIELD_DEF_INTERNAL_H_
#define UPB_REFLECTION_FIELD_DEF_INTERNAL_H_

#include "upb/reflection/field_def.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

upb_FieldDef* _upb_FieldDef_At(const upb_FieldDef* f, int i);

const upb_MiniTableExtension* _upb_FieldDef_ExtensionMiniTable(
    const upb_FieldDef* f);
bool _upb_FieldDef_IsClosedEnum(const upb_FieldDef* f);
bool _upb_FieldDef_IsProto3Optional(const upb_FieldDef* f);
int _upb_FieldDef_LayoutIndex(const upb_FieldDef* f);
uint64_t _upb_FieldDef_Modifiers(const upb_FieldDef* f);
void _upb_FieldDef_Resolve(upb_DefBuilder* ctx, const char* prefix,
                           upb_FieldDef* f);
void _upb_FieldDef_BuildMiniTableExtension(upb_DefBuilder* ctx,
                                           const upb_FieldDef* f);

// Allocate and initialize an array of |n| extensions (field defs).
upb_FieldDef* _upb_Extensions_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(FieldDescriptorProto) * const* protos, const char* prefix,
    upb_MessageDef* m);

// Allocate and initialize an array of |n| field defs.
upb_FieldDef* _upb_FieldDefs_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(FieldDescriptorProto) * const* protos, const char* prefix,
    upb_MessageDef* m, bool* is_sorted);

// Allocate and return a list of pointers to the |n| field defs in |ff|,
// sorted by field number.
const upb_FieldDef** _upb_FieldDefs_Sorted(const upb_FieldDef* f, int n,
                                           upb_Arena* a);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_REFLECTION_FIELD_DEF_INTERNAL_H_ */
