// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_INTERNAL_COMPARE_UNKNOWN_H_
#define UPB_MESSAGE_INTERNAL_COMPARE_UNKNOWN_H_

#include <stddef.h>

#include "upb/message/compare.h"
#include "upb/message/message.h"
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

upb_UnknownCompareResult UPB_PRIVATE(_upb_Message_UnknownFieldsAreEqual)(
    const upb_Message* msg1, const upb_Message* msg2, int max_depth);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_INTERNAL_COMPARE_UNKNOWN_H_ */
