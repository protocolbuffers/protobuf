// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_BACKEND_UPB_UPB_H__
#define GOOGLE_PROTOBUF_HPB_BACKEND_UPB_UPB_H__

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/hpb/arena.h"
#include "google/protobuf/hpb/backend/upb/interop.h"
#include "google/protobuf/hpb/internal/internal.h"
#include "google/protobuf/hpb/internal/message_lock.h"
#include "google/protobuf/hpb/internal/template_help.h"
#include "google/protobuf/hpb/ptr.h"

namespace hpb::internal::backend::upb {

template <typename T>
typename T::Proxy CreateMessage(Arena& arena) {
  return PrivateAccess::CreateMessage<T>(interop::upb::UnwrapArena(arena));
}

template <typename T>
typename T::Proxy CloneMessage(Ptr<T> message, Arena& arena) {
  return internal::PrivateAccess::Proxy<T>(
      internal::DeepClone(interop::upb::GetMessage(message), T::minitable(),
                          interop::upb::UnwrapArena(arena)),
      interop::upb::UnwrapArena(arena));
}

template <typename T>
void ClearMessage(PtrOrRawMutable<T> message) {
  auto ptr = Ptr(message);
  auto minitable = interop::upb::GetMiniTable(ptr);
  upb_Message_Clear(interop::upb::GetMessage(ptr), minitable);
}

template <typename T>
void DeepCopy(Ptr<const T> source_message, Ptr<T> target_message) {
  static_assert(!std::is_const_v<T>);
  internal::DeepCopy(interop::upb::GetMessage(target_message),
                     interop::upb::GetMessage(source_message), T::minitable(),
                     interop::upb::GetArena(target_message));
}

template <typename T>
absl::StatusOr<absl::string_view> Serialize(PtrOrRaw<T> message, Arena& arena) {
  return hpb::internal::Serialize(interop::upb::GetMessage(message),
                                  interop::upb::GetMiniTable(message),
                                  interop::upb::UnwrapArena(arena), 0);
}

}  // namespace hpb::internal::backend::upb

#endif  // GOOGLE_PROTOBUF_HPB_BACKEND_UPB_UPB_H__
