// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_HPB_H__
#define GOOGLE_PROTOBUF_HPB_HPB_H__

#include <type_traits>

#include "absl/base/attributes.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/hpb/arena.h"
#include "google/protobuf/hpb/backend/upb/interop.h"
#include "google/protobuf/hpb/extension.h"
#include "google/protobuf/hpb/internal/internal.h"
#include "google/protobuf/hpb/internal/message_lock.h"
#include "google/protobuf/hpb/internal/template_help.h"
#include "google/protobuf/hpb/ptr.h"
#include "google/protobuf/hpb/status.h"
#include "upb/wire/decode.h"

#ifdef HPB_BACKEND_UPB
#include "google/protobuf/hpb/backend/upb/upb.h"
#elif defined(HPB_BACKEND_CPP)
#include "google/protobuf/hpb/backend/cpp/cpp.h"
#else
#error invalid hpb backend
#endif

namespace hpb {

#ifdef HPB_BACKEND_UPB
namespace backend = hpb::internal::backend::upb;
#elif defined(HPB_BACKEND_CPP)
namespace backend = hpb::internal::backend::cpp;
#endif

template <typename T>
typename T::Proxy CreateMessage(Arena& arena) {
  return typename T::Proxy(upb_Message_New(T::minitable(), arena.ptr()),
                           arena.ptr());
}

template <typename T>
typename T::Proxy CloneMessage(Ptr<T> message, Arena& arena) {
  return internal::PrivateAccess::Proxy<T>(
      internal::DeepClone(interop::upb::GetMessage(message), T::minitable(),
                          arena.ptr()),
      arena.ptr());
}

// Deprecated; do not use. There is one extant caller which we plan to migrate.
// Tracking deletion TODO: b/385138477
template <typename T>
[[deprecated("Use CloneMessage(Ptr<T>, hpb::Arena&) instead.")]]
typename T::Proxy CloneMessage(Ptr<T> message, upb_Arena* arena) {
  return internal::PrivateAccess::Proxy<T>(
      internal::DeepClone(interop::upb::GetMessage(message), T::minitable(),
                          arena),
      arena);
}

template <typename T>
void DeepCopy(Ptr<const T> source_message, Ptr<T> target_message) {
  static_assert(!std::is_const_v<T>);
  internal::DeepCopy(interop::upb::GetMessage(target_message),
                     interop::upb::GetMessage(source_message), T::minitable(),
                     interop::upb::GetArena(target_message));
}

template <typename T>
void DeepCopy(Ptr<const T> source_message, T* target_message) {
  static_assert(!std::is_const_v<T>);
  DeepCopy(source_message, Ptr(target_message));
}

template <typename T>
void DeepCopy(const T* source_message, T* target_message) {
  static_assert(!std::is_const_v<T>);
  DeepCopy(Ptr(source_message), Ptr(target_message));
}

template <typename T>
void ClearMessage(internal::PtrOrRawMutable<T> message) {
  backend::ClearMessage(message);
}

template <typename T>
ABSL_MUST_USE_RESULT bool Parse(internal::PtrOrRaw<T> message,
                                absl::string_view bytes,
                                const ExtensionRegistry& extension_registry =
                                    ExtensionRegistry::EmptyRegistry()) {
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
                        const ExtensionRegistry& extension_registry =
                            ExtensionRegistry::EmptyRegistry()) {
  T message;
  auto* arena = interop::upb::GetArena(&message);
  upb_DecodeStatus status =
      upb_Decode(bytes.data(), bytes.size(), message.msg(),
                 interop::upb::GetMiniTable(&message),
                 internal::GetUpbExtensions(extension_registry),
                 /* options= */ 0, arena);
  if (status == kUpb_DecodeStatus_Ok) {
    return message;
  }
  return MessageDecodeError(status);
}

template <typename T>
absl::StatusOr<absl::string_view> Serialize(internal::PtrOrRaw<T> message,
                                            Arena& arena) {
  return backend::Serialize(message, arena);
}

}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_HPB_H__
