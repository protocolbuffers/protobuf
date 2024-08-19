// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PROTOBUF_HPB_EXTENSION_LOCK_H_
#define PROTOBUF_HPB_EXTENSION_LOCK_H_

#include <atomic>

#include "upb/mini_table/message.h"

class upb_Message;
class upb_MiniTable;
class upb_Arena;

namespace hpb::internal {

upb_Message* LockedDeepClone(const upb_Message* source,
                             const upb_MiniTable* mini_table, upb_Arena* arena);

void LockedDeepCopy(upb_Message* target, const upb_Message* source,
                    const upb_MiniTable* mini_table, upb_Arena* arena);

// TODO: Temporary locking api for cross-language
// concurrency issue around extension api that uses lazy promotion
// from unknown data to upb_MiniTableExtension. Will be replaced by
// a core runtime solution in the future.
//
// Any api(s) using unknown or extension data (GetOrPromoteExtension,
// Serialize and others) call lock/unlock to provide a way for
// mixed language implementations to avoid race conditions)
using UpbExtensionUnlocker = void (*)(const void*);
using UpbExtensionLocker = UpbExtensionUnlocker (*)(const void*);

// TODO: Expose as function instead of global.
extern std::atomic<UpbExtensionLocker> upb_extension_locker_global;

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
}  // namespace hpb::internal

#endif  // PROTOBUF_HPB_EXTENSION_LOCK_H_
