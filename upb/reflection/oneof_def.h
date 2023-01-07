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

#ifndef UPB_REFLECTION_ONEOF_DEF_H_
#define UPB_REFLECTION_ONEOF_DEF_H_

#include "upb/reflection/common.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

const upb_MessageDef* upb_OneofDef_ContainingType(const upb_OneofDef* o);
const upb_FieldDef* upb_OneofDef_Field(const upb_OneofDef* o, int i);
int upb_OneofDef_FieldCount(const upb_OneofDef* o);
const char* upb_OneofDef_FullName(const upb_OneofDef* o);
bool upb_OneofDef_HasOptions(const upb_OneofDef* o);
uint32_t upb_OneofDef_Index(const upb_OneofDef* o);
bool upb_OneofDef_IsSynthetic(const upb_OneofDef* o);
const upb_FieldDef* upb_OneofDef_LookupName(const upb_OneofDef* o,
                                            const char* name);
const upb_FieldDef* upb_OneofDef_LookupNameWithSize(const upb_OneofDef* o,
                                                    const char* name,
                                                    size_t size);
const upb_FieldDef* upb_OneofDef_LookupNumber(const upb_OneofDef* o,
                                              uint32_t num);
const char* upb_OneofDef_Name(const upb_OneofDef* o);
int upb_OneofDef_numfields(const upb_OneofDef* o);
const UPB_DESC(OneofOptions) * upb_OneofDef_Options(const upb_OneofDef* o);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_REFLECTION_ONEOF_DEF_H_ */
