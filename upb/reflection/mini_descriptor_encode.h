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

#ifndef UPB_REFLECTION_MINI_DESCRIPTOR_ENCODE_H_
#define UPB_REFLECTION_MINI_DESCRIPTOR_ENCODE_H_

#include "upb/reflection/common.h"

// Must be last.
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Creates and returns a mini descriptor string for an enum, or NULL on error.
const char* upb_MiniDescriptor_EncodeEnum(const upb_EnumDef* e, upb_Arena* a);

// Creates and returns a mini descriptor string for a field, or NULL on error.
const char* upb_MiniDescriptor_EncodeField(const upb_FieldDef* f, upb_Arena* a);

// Creates and returns a mini descriptor string for a message, or NULL on error.
const char* upb_MiniDescriptor_EncodeMessage(const upb_MessageDef* m,
                                             upb_Arena* a);

// EVERYTHING BELOW THIS LINE IS INTERNAL - DO NOT USE /////////////////////////

// Creates and returns a mini descriptor string for an enum, or NULL on error.
// If the values in the enum happen to be defined in ascending order (when cast
// to uint32_t) then |sorted| should be NULL. Otherwise it must point to an
// array containing pointers to the enum value defs in sorted order.
const char* _upb_MiniDescriptor_EncodeEnum(const upb_EnumDef* e,
                                           const upb_EnumValueDef** sorted,
                                           upb_Arena* a);

// Creates and returns a mini descriptor string for a field, or NULL on error.
const char* _upb_MiniDescriptor_EncodeField(const upb_FieldDef* f,
                                            upb_Arena* a);

// Creates and returns a mini descriptor string for a message, or NULL on error.
const char* _upb_MiniDescriptor_EncodeMessage(const upb_MessageDef* m,
                                              upb_Arena* a);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif /* UPB_REFLECTION_MINI_DESCRIPTOR_ENCODE_H_ */
