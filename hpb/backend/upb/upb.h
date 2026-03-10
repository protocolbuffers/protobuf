// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_BACKEND_UPB_UPB_H__
#define GOOGLE_PROTOBUF_HPB_BACKEND_UPB_UPB_H__

#include <cstddef>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "hpb/arena.h"
#include "hpb/backend/types.h"
#include "hpb/backend/upb/interop.h"
#include "hpb/extension.h"
#include "hpb/internal/internal.h"
#include "hpb/internal/message_lock.h"
#include "hpb/internal/template_help.h"
#include "hpb/options.h"
#include "hpb/ptr.h"
#include "hpb/status.h"
#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/wire/decode.h"

// Must be last.
#include "upb/port/def.inc"

namespace hpb::internal::backend::upb {

template <size_t N>
class DefaultInstance {
 public:
  static const upb_Message* msg() { return (upb_Message*)buffer_; }
  static upb_Arena* arena() { return arena_; }

 private:
  alignas(UPB_MALLOC_ALIGN) static constexpr char buffer_[N] = {0};
  static inline upb_Arena* arena_ = upb_Arena_New();
};

template <typename T>
typename T::Proxy CreateMessage(hpb::Arena& arena) {
  return PrivateAccess::CreateMessage<T>(hpb::interop::upb::UnwrapArena(arena));
}

template <typename T>
typename T::Proxy CloneMessage(Ptr<T> message, hpb::Arena& arena) {
  return internal::PrivateAccess::Proxy<T>(
      internal::DeepClone(interop::upb::GetMessage(message), T::minitable(),
                          hpb::interop::upb::UnwrapArena(arena)),
      hpb::interop::upb::UnwrapArena(arena));
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
absl::StatusOr<absl::string_view> Serialize(PtrOrRaw<T> message,
                                            hpb::Arena& arena) {
  return hpb::internal::Serialize(interop::upb::GetMessage(message),
                                  interop::upb::GetMiniTable(message),
                                  hpb::interop::upb::UnwrapArena(arena), 0);
}

template <typename T>
bool Parse(PtrOrRaw<T> message, absl::string_view bytes,
           const ExtensionRegistry& extension_registry) {
  static_assert(!std::is_const_v<T>);
  upb_Message_Clear(interop::upb::GetMessage(message),
                    interop::upb::GetMiniTable(message));
  auto* arena = interop::upb::GetArena(message);
  return upb_Decode(bytes.data(), bytes.size(),
                    interop::upb::GetMessage(message),
                    interop::upb::GetMiniTable(message),
                    internal::GetUpbExtensions(extension_registry),
                    /* options= */ 0, arena) == kUpb_DecodeStatus_Ok;
}

template <typename T>
absl::StatusOr<T> Parse(absl::string_view bytes,
                        const ExtensionRegistry& extension_registry) {
  T message;
  auto* arena = interop::upb::GetArena(&message);
  upb_DecodeStatus status =
      upb_Decode(bytes.data(), bytes.size(), interop::upb::GetMessage(&message),
                 interop::upb::GetMiniTable(&message),
                 internal::GetUpbExtensions(extension_registry),
                 /* options= */ 0, arena);
  if (status == kUpb_DecodeStatus_Ok) {
    return message;
  }
  return MessageDecodeError(status);
}

template <typename T>
hpb::StatusOr<T> Parse(absl::string_view bytes, ParseOptions options) {
  int flags = 0;
  if (options.alias_string) {
    flags |= kUpb_DecodeOption_AliasString;
  }
  T message;
  auto* arena = interop::upb::GetArena(&message);
  upb_DecodeStatus status = upb_Decode(
      bytes.data(), bytes.size(), interop::upb::GetMessage(&message),
      interop::upb::GetMiniTable(&message),
      internal::GetUpbExtensions(options.extension_registry), flags, arena);
  if (status == kUpb_DecodeStatus_Ok) {
    return hpb::StatusOr<T>(std::move(message));
  }
  return hpb::StatusOr<T>(internal::backend::Error(status));
}

}  // namespace hpb::internal::backend::upb

#include "upb/port/undef.inc"

#endif  // GOOGLE_PROTOBUF_HPB_BACKEND_UPB_UPB_H__
