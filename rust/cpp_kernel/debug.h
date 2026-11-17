// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_RUST_CPP_KERNEL_DEBUG_H__
#define GOOGLE_PROTOBUF_RUST_CPP_KERNEL_DEBUG_H__

#include "google/protobuf/message_lite.h"
#include "rust/cpp_kernel/strings.h"

extern "C" {

google::protobuf::rust::RustStringRawParts proto2_rust_utf8_debug_string(
    const google::protobuf::MessageLite* msg);

}  // extern "C"

#endif  // GOOGLE_PROTOBUF_RUST_CPP_KERNEL_DEBUG_H__
