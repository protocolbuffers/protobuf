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

#ifndef UPB_REFLECTION_FIELD_DEF_H_
#define UPB_REFLECTION_FIELD_DEF_H_

#include "upb/base/string_view.h"
#include "upb/reflection/common.h"

// Must be last.
#include "upb/port/def.inc"

// Maximum field number allowed for FieldDefs.
// This is an inherent limit of the protobuf wire format.
#define kUpb_MaxFieldNumber ((1 << 29) - 1)

#ifdef __cplusplus
extern "C" {
#endif

const upb_OneofDef* upb_FieldDef_ContainingOneof(const upb_FieldDef* f);
const upb_MessageDef* upb_FieldDef_ContainingType(const upb_FieldDef* f);
upb_CType upb_FieldDef_CType(const upb_FieldDef* f);
upb_MessageValue upb_FieldDef_Default(const upb_FieldDef* f);
const upb_EnumDef* upb_FieldDef_EnumSubDef(const upb_FieldDef* f);
const upb_MessageDef* upb_FieldDef_ExtensionScope(const upb_FieldDef* f);
const upb_FileDef* upb_FieldDef_File(const upb_FieldDef* f);
const char* upb_FieldDef_FullName(const upb_FieldDef* f);
bool upb_FieldDef_HasDefault(const upb_FieldDef* f);
bool upb_FieldDef_HasJsonName(const upb_FieldDef* f);
bool upb_FieldDef_HasOptions(const upb_FieldDef* f);
bool upb_FieldDef_HasPresence(const upb_FieldDef* f);
bool upb_FieldDef_HasSubDef(const upb_FieldDef* f);
uint32_t upb_FieldDef_Index(const upb_FieldDef* f);
bool upb_FieldDef_IsExtension(const upb_FieldDef* f);
bool upb_FieldDef_IsMap(const upb_FieldDef* f);
bool upb_FieldDef_IsOptional(const upb_FieldDef* f);
bool upb_FieldDef_IsPacked(const upb_FieldDef* f);
bool upb_FieldDef_IsPrimitive(const upb_FieldDef* f);
bool upb_FieldDef_IsRepeated(const upb_FieldDef* f);
bool upb_FieldDef_IsRequired(const upb_FieldDef* f);
bool upb_FieldDef_IsString(const upb_FieldDef* f);
bool upb_FieldDef_IsSubMessage(const upb_FieldDef* f);
const char* upb_FieldDef_JsonName(const upb_FieldDef* f);
upb_Label upb_FieldDef_Label(const upb_FieldDef* f);
const upb_MessageDef* upb_FieldDef_MessageSubDef(const upb_FieldDef* f);

// Creates a mini descriptor string for a field, returns true on success.
bool upb_FieldDef_MiniDescriptorEncode(const upb_FieldDef* f, upb_Arena* a,
                                       upb_StringView* out);

const upb_MiniTableField* upb_FieldDef_MiniTable(const upb_FieldDef* f);
const char* upb_FieldDef_Name(const upb_FieldDef* f);
uint32_t upb_FieldDef_Number(const upb_FieldDef* f);
const UPB_DESC(FieldOptions) * upb_FieldDef_Options(const upb_FieldDef* f);
const upb_OneofDef* upb_FieldDef_RealContainingOneof(const upb_FieldDef* f);
upb_FieldType upb_FieldDef_Type(const upb_FieldDef* f);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_REFLECTION_FIELD_DEF_H_ */
