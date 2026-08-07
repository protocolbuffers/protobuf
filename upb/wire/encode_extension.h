// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_ENCODE_EXTENSION_H_
#define UPB_WIRE_ENCODE_EXTENSION_H_

#include "upb/base/string_view.h"
#include "upb/mem/internal/arena.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

struct upb_Extension;

// Encodes an extension (`upb_Extension*`) to bytes.
//
// This can be used to encode an extension into the provided arena.
// Returns `kUpb_EncodeStatus_Ok` on success.
UPB_NODISCARD upb_EncodeStatus
upb_EncodeExtension(const struct upb_Extension* ext, struct upb_Arena* arena,
                    upb_StringView* view, int encode_options);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_ENCODE_EXTENSION_H_ */
