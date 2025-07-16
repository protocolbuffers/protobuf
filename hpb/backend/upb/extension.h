// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_BACKEND_UPB_EXTENSION_H__
#define GOOGLE_PROTOBUF_HPB_BACKEND_UPB_EXTENSION_H__

#include "absl/status/status.h"
#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension.h"

namespace hpb {
namespace internal {
absl::Status MoveExtension(upb_Message* message, upb_Arena* message_arena,
                           const upb_MiniTableExtension* ext,
                           upb_Message* extension, upb_Arena* extension_arena);

absl::Status SetExtension(upb_Message* message, upb_Arena* message_arena,
                          const upb_MiniTableExtension* ext,
                          const upb_Message* extension);

void SetAliasExtension(upb_Message* message, upb_Arena* message_arena,
                       const upb_MiniTableExtension* ext,
                       upb_Message* extension, upb_Arena* extension_arena);
}  // namespace internal
}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_BACKEND_UPB_EXTENSION_H__
