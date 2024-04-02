// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_PROTOS_PROTOS_EXTENSION_LOCK_H_
#define UPB_PROTOS_PROTOS_EXTENSION_LOCK_H_

#include <atomic>

namespace protos::internal {

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

}  // namespace protos::internal

#endif  // UPB_PROTOS_PROTOS_EXTENSION_LOCK_H_
