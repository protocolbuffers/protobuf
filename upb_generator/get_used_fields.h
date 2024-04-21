// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_GENERATOR_GET_USED_FIELDS
#define UPB_GENERATOR_GET_USED_FIELDS

#include "upb/base/status.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Consume |buf|, deserialize it to a Code_Generator_Request proto, then
// upb_Code_Generator_Request, and return it as a JSON-encoded string.
UPB_API upb_StringView upbdev_GetUsedFields(
    const char* request, size_t request_size, const char* payload,
    size_t payload_size, const char* message_name, upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_GENERATOR_GET_USED_FIELDS
