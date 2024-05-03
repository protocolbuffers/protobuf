// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_INTERNAL_TYPES_H_
#define UPB_MINI_TABLE_INTERNAL_TYPES_H_

typedef struct upb_Message_InternalData upb_Message_InternalData;

typedef struct {
  union {
    upb_Message_InternalData* internal;

    // Force 8-byte alignment, since the data members may contain members that
    // require 8-byte alignment.
    double d;
  };
} upb_Message_Internal;

#endif  // UPB_MINI_TABLE_INTERNAL_TYPES_H_
