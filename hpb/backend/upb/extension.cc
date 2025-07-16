// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_table/extension.h"

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "hpb/backend/upb/extension.h"
#include "hpb/internal/message_lock.h"
#include "hpb/status.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/message.h"

namespace hpb::internal {

absl::Status MoveExtension(upb_Message* message, upb_Arena* message_arena,
                           const upb_MiniTableExtension* ext,
                           upb_Message* extension, upb_Arena* extension_arena) {
  if (message_arena != extension_arena &&
      // Try fuse, if fusing is not allowed or fails, create copy of extension.
      !upb_Arena_Fuse(message_arena, extension_arena)) {
    extension = DeepClone(extension, upb_MiniTableExtension_GetSubMessage(ext),
                          message_arena);
  }
  return upb_Message_SetExtension(message, ext, &extension, message_arena)
             ? absl::OkStatus()
             : MessageAllocationError();
}

absl::Status SetExtension(upb_Message* message, upb_Arena* message_arena,
                          const upb_MiniTableExtension* ext,
                          const upb_Message* extension) {
  // Clone extension into target message arena.
  extension = DeepClone(extension, upb_MiniTableExtension_GetSubMessage(ext),
                        message_arena);
  return upb_Message_SetExtension(message, ext, &extension, message_arena)
             ? absl::OkStatus()
             : MessageAllocationError();
}

void SetAliasExtension(upb_Message* message, upb_Arena* message_arena,
                       const upb_MiniTableExtension* ext,
                       upb_Message* extension, upb_Arena* extension_arena) {
  ABSL_CHECK(upb_Arena_IsFused(message_arena, extension_arena));
  upb_Message_SetExtension(message, ext, &extension, message_arena);
}
}  // namespace hpb::internal
