// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_BACKEND_UPB_INTEROP_H__
#define GOOGLE_PROTOBUF_HPB_BACKEND_UPB_INTEROP_H__

// The sole public header in hpb/backend/upb

#include "google/protobuf/hpb/internal/internal.h"
#include "google/protobuf/hpb/ptr.h"
#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/message.h"

namespace hpb::interop::upb {

template <typename T>
const upb_MiniTable* GetMiniTable(const T*) {
  return T::minitable();
}

template <typename T>
const upb_MiniTable* GetMiniTable(Ptr<T>) {
  return T::minitable();
}

/**
 * Creates a const Handle to a upb message.
 *
 * The supplied arena must outlive the hpb handle.
 * All messages reachable from from the upb message must
 * outlive the hpb handle.
 *
 * That is:
 * upb allows message M on arena A to point to message M' on
 * arena A'. As a user of hpb, you must guarantee that both A and A'
 * outlive M and M'. In practice, this is enforced by using upb::Fuse,
 * or manual tracking.
 *
 * The upb message must not be mutated directly while the handle is alive.
 *
 * TODO: b/361596328 - revisit GetArena for CHandles
 */
template <typename T>
typename T::CProxy MakeCHandle(const upb_Message* msg, upb_Arena* arena) {
  return hpb::internal::PrivateAccess::CProxy<T>(msg, arena);
}

}  // namespace hpb::interop::upb

#endif  // GOOGLE_PROTOBUF_HPB_BACKEND_UPB_INTEROP_H__
