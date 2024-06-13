// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_GENERATOR_CODE_GENERATOR_REQUEST_H_
#define UPB_GENERATOR_CODE_GENERATOR_REQUEST_H_

#include "upb/mem/arena.h"
#include "upb/reflection/def.h"
#include "upb_generator/code_generator_request.upb.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

upb_CodeGeneratorRequest* upbc_MakeCodeGeneratorRequest(
    struct google_protobuf_compiler_CodeGeneratorRequest* request, upb_Arena* a,
    upb_Status* s);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_GENERATOR_CODE_GENERATOR_REQUEST_H_ */
