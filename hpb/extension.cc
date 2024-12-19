// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/hpb/extension.h"

#include "absl/status/status.h"
#include "google/protobuf/hpb/internal/message_lock.h"
#include "google/protobuf/hpb/status.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"

namespace hpb {
namespace internal {
upb_ExtensionRegistry* GetUpbExtensions(
    const ExtensionRegistry& extension_registry) {
  return extension_registry.registry_;
}

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

}  // namespace internal
}  // namespace hpb
