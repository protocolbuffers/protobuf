// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_PROTOS_PROTOS_INTERNAL_H_
#define UPB_PROTOS_PROTOS_INTERNAL_H_

#include "upb/mem/arena.h"
#include "upb/message/message.h"

namespace protos::internal {

// Moves ownership of a message created in a source arena.
//
// Utility function to provide a way to move ownership across languages or VMs.
template <typename T>
T MoveMessage(upb_Message* msg, upb_Arena* arena) {
  return T(msg, arena);
}

}  // namespace protos::internal
#endif
