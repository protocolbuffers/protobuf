// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_GENERATED_CODE_SUPPORT_H_
#define UPB_GENERATED_CODE_SUPPORT_H_

// This is a bit awkward; we want to conditionally include the fast decoder,
// but we generally don't let macros like UPB_FASTTABLE leak into user code.
// We can't #include "decode_fast.h" inside the port/def.inc, because the
// inc files strictly prohibit recursive inclusion, and decode_fast.h includes
// port/def.inc. So instead we use this two-part dance to conditionally include
// decode_fast.h.
#include "upb/port/def.inc"
#if UPB_FASTTABLE
#define UPB_INCLUDE_FAST_DECODE
#endif
#include "upb/port/undef.inc"

// IWYU pragma: begin_exports
#include "upb/base/upcast.h"
#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/array.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/map_gencode_util.h"
#include "upb/message/message.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/file.h"
#include "upb/mini_table/message.h"
#include "upb/mini_table/sub.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"
#ifdef UPB_INCLUDE_FAST_DECODE
#include "upb/wire/decode_fast/field_parsers.h"
#endif
// IWYU pragma: end_exports

#undef UPB_INCLUDE_FAST_DECODE

#endif  // UPB_GENERATED_CODE_SUPPORT_H_
