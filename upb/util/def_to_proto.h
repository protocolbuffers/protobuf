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

#ifndef UPB_UTIL_DEF_TO_PROTO_H_
#define UPB_UTIL_DEF_TO_PROTO_H_

#include "upb/def.h"

#ifdef __cplusplus
extern "C" {
#endif

// Functions for converting defs back to the equivalent descriptor proto.
// Ultimately the goal is that a round-trip proto->def->proto is lossless.  Each
// function returns a new proto created in arena `a`, or NULL if memory
// allocation failed.
google_protobuf_DescriptorProto* upb_MessageDef_ToProto(const upb_MessageDef* m,
                                                        upb_Arena* a);
google_protobuf_EnumDescriptorProto* upb_EnumDef_ToProto(const upb_EnumDef* e,
                                                         upb_Arena* a);
google_protobuf_EnumValueDescriptorProto* upb_EnumValueDef_ToProto(
    const upb_EnumValueDef* e, upb_Arena* a);
google_protobuf_FieldDescriptorProto* upb_FieldDef_ToProto(
    const upb_FieldDef* f, upb_Arena* a);
google_protobuf_OneofDescriptorProto* upb_OneofDef_ToProto(
    const upb_OneofDef* o, upb_Arena* a);
google_protobuf_FileDescriptorProto* upb_FileDef_ToProto(const upb_FileDef* f,
                                                         upb_Arena* a);
google_protobuf_MethodDescriptorProto* upb_MethodDef_ToProto(
    const upb_MethodDef* m, upb_Arena* a);
google_protobuf_ServiceDescriptorProto* upb_ServiceDef_ToProto(
    const upb_ServiceDef* s, upb_Arena* a);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UPB_UTIL_DEF_TO_PROTO_H_ */
