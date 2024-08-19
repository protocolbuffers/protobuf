// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/hpb/internal/message_lock.h"

#include <atomic>

#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/copy.h"
#include "upb/message/message.h"
#include "upb/message/value.h"
#include "upb/message/promote.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/message.h"

namespace hpb::internal {

std::atomic<UpbExtensionLocker> upb_extension_locker_global;

upb_Message* LockedDeepClone(const upb_Message* source,
                             const upb_MiniTable* mini_table,
                             upb_Arena* arena) {
  MessageLock msg_lock(source);
  return upb_Message_DeepClone(source, mini_table, arena);
}

void LockedDeepCopy(upb_Message* target, const upb_Message* source,
                    const upb_MiniTable* mini_table, upb_Arena* arena) {
  MessageLock msg_lock(source);
  upb_Message_DeepCopy(target, source, mini_table, arena);
}

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

}  // namespace hpb::internal
