// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_INTERNAL_TYPES_H_
#define UPB_MINI_TABLE_INTERNAL_TYPES_H_

// This definition exists here (instead of in upb/message/) as an optimization
// so that minitable code can cleanly know the size of a upb_Message - which is
// also the minimum size of a minitable.

struct upb_Message {
  union {
    struct upb_Message_Internal* internal;
    double d;  // Forces same size for 32-bit/64-bit builds
  };
};

#endif /* UPB_MINI_TABLE_INTERNAL_TYPES_H_ */
