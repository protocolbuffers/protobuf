// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/hpb/hpb.h"

#include <atomic>
#include <cstddef>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/hpb/internal/message_lock.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/copy.h"
#include "upb/message/message.h"
#include "upb/message/promote.h"
#include "upb/message/value.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

namespace hpb {

absl::Status MessageAllocationError(SourceLocation loc) {
  return absl::Status(absl::StatusCode::kUnknown,
                      "Upb message allocation error");
}

absl::Status ExtensionNotFoundError(int ext_number, SourceLocation loc) {
  return absl::Status(absl::StatusCode::kUnknown,
                      absl::StrFormat("Extension %d not found", ext_number));
}

absl::Status MessageEncodeError(upb_EncodeStatus s, SourceLocation loc) {
  return absl::Status(absl::StatusCode::kUnknown, "Encoding error");
}

absl::Status MessageDecodeError(upb_DecodeStatus status, SourceLocation loc

) {
  return absl::Status(absl::StatusCode::kUnknown, "Upb message parse error");
}

namespace internal {

/**
 * MessageLock(msg) acquires lock on msg when constructed and releases it when
 * destroyed.
 */
class MessageLock {
 public:
  explicit MessageLock(const upb_Message* msg) : msg_(msg) {
    UpbExtensionLocker locker =
        upb_extension_locker_global.load(std::memory_order_acquire);
    unlocker_ = (locker != nullptr) ? locker(msg) : nullptr;
  }
  MessageLock(const MessageLock&) = delete;
  void operator=(const MessageLock&) = delete;
  ~MessageLock() {
    if (unlocker_ != nullptr) {
      unlocker_(msg_);
    }
  }

 private:
  const upb_Message* msg_;
  UpbExtensionUnlocker unlocker_;
};

bool HasExtensionOrUnknown(const upb_Message* msg,
                           const upb_MiniTableExtension* eid) {
  MessageLock msg_lock(msg);
  if (upb_Message_HasExtension(msg, eid)) return true;

  const int number = upb_MiniTableExtension_Number(eid);
  return upb_Message_FindUnknown(msg, number, 0).status == kUpb_FindUnknown_Ok;
}

bool GetOrPromoteExtension(upb_Message* msg, const upb_MiniTableExtension* eid,
                           upb_Arena* arena, upb_MessageValue* value) {
  MessageLock msg_lock(msg);
  upb_GetExtension_Status ext_status = upb_Message_GetOrPromoteExtension(
      (upb_Message*)msg, eid, 0, arena, value);
  return ext_status == kUpb_GetExtension_Ok;
}

absl::StatusOr<absl::string_view> Serialize(const upb_Message* message,
                                            const upb_MiniTable* mini_table,
                                            upb_Arena* arena, int options) {
  MessageLock msg_lock(message);
  size_t len;
  char* ptr;
  upb_EncodeStatus status =
      upb_Encode(message, mini_table, options, arena, &ptr, &len);
  if (status == kUpb_EncodeStatus_Ok) {
    return absl::string_view(ptr, len);
  }
  return MessageEncodeError(status);
}

void DeepCopy(upb_Message* target, const upb_Message* source,
              const upb_MiniTable* mini_table, upb_Arena* arena) {
  MessageLock msg_lock(source);
  upb_Message_DeepCopy(target, source, mini_table, arena);
}

upb_Message* DeepClone(const upb_Message* source,
                       const upb_MiniTable* mini_table, upb_Arena* arena) {
  MessageLock msg_lock(source);
  return upb_Message_DeepClone(source, mini_table, arena);
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
