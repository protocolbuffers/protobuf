// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_BACKEND_UPB_UPB_H__
#define GOOGLE_PROTOBUF_HPB_BACKEND_UPB_UPB_H__

#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/hpb/arena.h"
#include "google/protobuf/hpb/backend/upb/interop.h"
#include "google/protobuf/hpb/internal/message_lock.h"
#include "google/protobuf/hpb/internal/template_help.h"
#include "google/protobuf/hpb/ptr.h"
#include "upb/text/debug_string.h"

namespace hpb::internal::backend::upb {

template <typename T>
void ClearMessage(hpb::internal::PtrOrRawMutable<T> message) {
  auto ptr = Ptr(message);
  auto minitable = hpb::interop::upb::GetMiniTable(ptr);
  upb_Message_Clear(hpb::interop::upb::GetMessage(ptr), minitable);
}

template <typename T>
absl::StatusOr<absl::string_view> Serialize(hpb::internal::PtrOrRaw<T> message,
                                            hpb::Arena& arena) {
  return hpb::internal::Serialize(hpb::interop::upb::GetMessage(message),
                                  ::hpb::interop::upb::GetMiniTable(message),
                                  arena.ptr(), 0);
}

template <typename T>
std::string DebugString(hpb::internal::PtrOrRaw<T> message) {
  auto msg = interop::upb::GetMessage(message);
  auto minitable = interop::upb::GetMiniTable(message);
  auto size = upb_DebugString(msg, minitable, 0, nullptr, 0);
  std::vector<char> buf(size + 1);  // +1 for '\0'
  upb_DebugString(msg, minitable, 0, &buf[0], size + 1);
  return std::string(buf.data(), size);
}

}  // namespace hpb::internal::backend::upb

#endif  // GOOGLE_PROTOBUF_HPB_BACKEND_UPB_UPB_H__
