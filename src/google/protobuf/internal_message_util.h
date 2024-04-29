// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// This file contains miscellaneous (non-lite) helper code not suitable to
// generated_message_util.h. This should not be used directly by users.

#ifndef GOOGLE_PROTOBUF_INTERNAL_MESSAGE_UTIL_H__
#define GOOGLE_PROTOBUF_INTERNAL_MESSAGE_UTIL_H__

#include "google/protobuf/message.h"

namespace google {
namespace protobuf {
namespace internal {

// Walks the entire message tree and eager parses all lazy fields.
void EagerParseLazyField(Message& message);

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_INTERNAL_MESSAGE_UTIL_H__
