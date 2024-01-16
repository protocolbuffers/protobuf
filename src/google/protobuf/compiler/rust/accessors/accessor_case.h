// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_RUST_ACCESSORS_ACCESSOR_CASE_H__
#define GOOGLE_PROTOBUF_COMPILER_RUST_ACCESSORS_ACCESSOR_CASE_H__

// GenerateAccessorMsgImpl is reused for all three types of $Msg$, $Msg$Mut and
// $Msg$View; this enum signifies which case we are handling so corresponding
// adjustments can be made (for example: to not emit any mutation accessors
// on $Msg$View).
enum class AccessorCase {
  OWNED,
  MUT,
  VIEW,
};

#endif  // GOOGLE_PROTOBUF_COMPILER_RUST_ACCESSORS_ACCESSOR_CASE_H__
