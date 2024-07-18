// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_PROTOS_PROTOS_EXTENSION_LOCK_H_
#define UPB_PROTOS_PROTOS_EXTENSION_LOCK_H_
#include "google/protobuf/hpb/extension_lock.h"
namespace protos::internal {
using hpb::internal::upb_extension_locker_global;
using hpb::internal::UpbExtensionLocker;
using hpb::internal::UpbExtensionUnlocker;
}  // namespace protos::internal
#endif
