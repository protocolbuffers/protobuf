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

// Declarations common to all public def types.

#ifndef UPB_REFLECTION_COMMON_H_
#define UPB_REFLECTION_COMMON_H_

// begin:google_only
// #ifndef UPB_BOOTSTRAP_STAGE0
// #include "net/proto2/proto/descriptor.upb.h"
// #else
// #include "google/protobuf/descriptor.upb.h"
// #endif
// end:google_only

// begin:github_only
#include "google/protobuf/descriptor.upb.h"
// end:github_only

typedef enum { kUpb_Syntax_Proto2 = 2, kUpb_Syntax_Proto3 = 3 } upb_Syntax;

// Forward declarations for circular references.
typedef struct upb_DefPool upb_DefPool;
typedef struct upb_EnumDef upb_EnumDef;
typedef struct upb_EnumReservedRange upb_EnumReservedRange;
typedef struct upb_EnumValueDef upb_EnumValueDef;
typedef struct upb_ExtensionRange upb_ExtensionRange;
typedef struct upb_FieldDef upb_FieldDef;
typedef struct upb_FileDef upb_FileDef;
typedef struct upb_MessageDef upb_MessageDef;
typedef struct upb_MessageReservedRange upb_MessageReservedRange;
typedef struct upb_MethodDef upb_MethodDef;
typedef struct upb_OneofDef upb_OneofDef;
typedef struct upb_ServiceDef upb_ServiceDef;

// EVERYTHING BELOW THIS LINE IS INTERNAL - DO NOT USE /////////////////////////

typedef struct upb_DefBuilder upb_DefBuilder;

#endif /* UPB_REFLECTION_COMMON_H_ */
