// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PROTOBUF_HPB_HPB_H_
#define PROTOBUF_HPB_HPB_H_

#include <cstdint>
#include <type_traits>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/hpb/arena.h"
#include "google/protobuf/hpb/backend/upb/interop.h"
#include "google/protobuf/hpb/extension.h"
#include "google/protobuf/hpb/internal/internal.h"
#include "google/protobuf/hpb/internal/template_help.h"
#include "google/protobuf/hpb/ptr.h"
#include "upb/base/status.hpp"
#include "upb/mem/arena.hpp"
#include "upb/message/copy.h"
#include "upb/mini_table/extension.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

#ifdef HPB_BACKEND_UPB
#include "google/protobuf/hpb/backend/upb/upb.h"
#else
#error hpb backend must be specified
#endif

namespace hpb {
class ExtensionRegistry;

// TODO: update bzl and move to upb runtime / protos.cc.
inline upb_StringView UpbStrFromStringView(absl::string_view str,
                                           upb_Arena* arena) {
  const size_t str_size = str.size();
  char* buffer = static_cast<char*>(upb_Arena_Malloc(arena, str_size));
  memcpy(buffer, str.data(), str_size);
  return upb_StringView_FromDataAndSize(buffer, str_size);
}

// This type exists to work around an absl type that has not yet been
// released.
struct SourceLocation {
  static SourceLocation current() { return {}; }
  absl::string_view file_name() { return "<unknown>"; }
  int line() { return 0; }
};

absl::Status MessageAllocationError(
    SourceLocation loc = SourceLocation::current());

absl::Status ExtensionNotFoundError(
    int extension_number, SourceLocation loc = SourceLocation::current());

absl::Status MessageDecodeError(upb_DecodeStatus status,
                                SourceLocation loc = SourceLocation::current());

absl::Status MessageEncodeError(upb_EncodeStatus status,
                                SourceLocation loc = SourceLocation::current());

namespace internal {

absl::StatusOr<absl::string_view> Serialize(const upb_Message* message,
                                            const upb_MiniTable* mini_table,
                                            upb_Arena* arena, int options);

bool HasExtensionOrUnknown(const upb_Message* msg,
                           const upb_MiniTableExtension* eid);

bool GetOrPromoteExtension(upb_Message* msg, const upb_MiniTableExtension* eid,
                           upb_Arena* arena, upb_MessageValue* value);

void DeepCopy(upb_Message* target, const upb_Message* source,
              const upb_MiniTable* mini_table, upb_Arena* arena);

upb_Message* DeepClone(const upb_Message* source,
                       const upb_MiniTable* mini_table, upb_Arena* arena);

absl::Status MoveExtension(upb_Message* message, upb_Arena* message_arena,
                           const upb_MiniTableExtension* ext,
                           upb_Message* extension, upb_Arena* extension_arena);

absl::Status SetExtension(upb_Message* message, upb_Arena* message_arena,
                          const upb_MiniTableExtension* ext,
                          const upb_Message* extension);

}  // namespace internal

#ifdef HPB_BACKEND_UPB
namespace backend = ::hpb::internal::backend::upb;
#endif

template <typename T, typename Extendee, typename Extension,
          typename = hpb::internal::EnableIfHpbClass<T>>
ABSL_MUST_USE_RESULT bool HasExtension(
    Ptr<T> message,
    const ::hpb::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  return ::hpb::internal::HasExtensionOrUnknown(
      hpb::interop::upb::GetMessage(message), id.mini_table_ext());
}

template <typename T, typename Extendee, typename Extension,
          typename = hpb::internal::EnableIfHpbClass<T>>
ABSL_MUST_USE_RESULT bool HasExtension(
    const T* message,
    const ::hpb::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  return HasExtension(Ptr(message), id);
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClass<T>,
          typename = hpb::internal::EnableIfMutableProto<T>>
void ClearExtension(
    Ptr<T> message,
    const ::hpb::internal::ExtensionIdentifier<T, Extension>& id) {
  static_assert(!std::is_const_v<T>, "");
  upb_Message_ClearExtension(hpb::interop::upb::GetMessage(message),
                             id.mini_table_ext());
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClass<T>>
void ClearExtension(
    T* message, const ::hpb::internal::ExtensionIdentifier<T, Extension>& id) {
  ClearExtension(Ptr(message), id);
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClass<T>,
          typename = hpb::internal::EnableIfMutableProto<T>>
absl::Status SetExtension(
    Ptr<T> message,
    const ::hpb::internal::ExtensionIdentifier<T, Extension>& id,
    const Extension& value) {
  static_assert(!std::is_const_v<T>);
  auto* message_arena = static_cast<upb_Arena*>(message->GetInternalArena());
  return ::hpb::internal::SetExtension(hpb::interop::upb::GetMessage(message),
                                       message_arena, id.mini_table_ext(),
                                       hpb::interop::upb::GetMessage(&value));
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClass<T>,
          typename = hpb::internal::EnableIfMutableProto<T>>
absl::Status SetExtension(
    Ptr<T> message,
    const ::hpb::internal::ExtensionIdentifier<T, Extension>& id,
    Ptr<Extension> value) {
  static_assert(!std::is_const_v<T>);
  auto* message_arena = static_cast<upb_Arena*>(message->GetInternalArena());
  return ::hpb::internal::SetExtension(hpb::interop::upb::GetMessage(message),
                                       message_arena, id.mini_table_ext(),
                                       hpb::interop::upb::GetMessage(value));
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClass<T>,
          typename = hpb::internal::EnableIfMutableProto<T>>
absl::Status SetExtension(
    Ptr<T> message,
    const ::hpb::internal::ExtensionIdentifier<T, Extension>& id,
    Extension&& value) {
  Extension ext = std::move(value);
  static_assert(!std::is_const_v<T>);
  auto* message_arena = static_cast<upb_Arena*>(message->GetInternalArena());
  auto* extension_arena = static_cast<upb_Arena*>(ext.GetInternalArena());
  return ::hpb::internal::MoveExtension(hpb::interop::upb::GetMessage(message),
                                        message_arena, id.mini_table_ext(),
                                        hpb::interop::upb::GetMessage(&ext),
                                        extension_arena);
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClass<T>>
absl::Status SetExtension(
    T* message, const ::hpb::internal::ExtensionIdentifier<T, Extension>& id,
    const Extension& value) {
  return ::hpb::SetExtension(Ptr(message), id, value);
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClass<T>>
absl::Status SetExtension(
    T* message, const ::hpb::internal::ExtensionIdentifier<T, Extension>& id,
    Extension&& value) {
  return ::hpb::SetExtension(Ptr(message), id, std::forward<Extension>(value));
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClass<T>>
absl::Status SetExtension(
    T* message, const ::hpb::internal::ExtensionIdentifier<T, Extension>& id,
    Ptr<Extension> value) {
  return ::hpb::SetExtension(Ptr(message), id, value);
}

template <typename T, typename Extendee, typename Extension,
          typename = hpb::internal::EnableIfHpbClass<T>>
absl::StatusOr<Ptr<const Extension>> GetExtension(
    Ptr<T> message,
    const ::hpb::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  // TODO: Fix const correctness issues.
  upb_MessageValue value;
  const bool ok = ::hpb::internal::GetOrPromoteExtension(
      const_cast<upb_Message*>(hpb::interop::upb::GetMessage(message)),
      id.mini_table_ext(), hpb::interop::upb::GetArena(message), &value);
  if (!ok) {
    return ExtensionNotFoundError(
        upb_MiniTableExtension_Number(id.mini_table_ext()));
  }
  return Ptr<const Extension>(::hpb::interop::upb::MakeCHandle<Extension>(
      (upb_Message*)value.msg_val, hpb::interop::upb::GetArena(message)));
}

template <typename T, typename Extendee, typename Extension,
          typename = hpb::internal::EnableIfHpbClass<T>>
absl::StatusOr<Ptr<const Extension>> GetExtension(
    const T* message,
    const ::hpb::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  return GetExtension(Ptr(message), id);
}

template <typename T, typename Extension>
constexpr uint32_t ExtensionNumber(
    ::hpb::internal::ExtensionIdentifier<T, Extension> id) {
  return ::hpb::internal::PrivateAccess::GetExtensionNumber(id);
}

template <typename T>
typename T::Proxy CreateMessage(::hpb::Arena& arena) {
  return typename T::Proxy(upb_Message_New(T::minitable(), arena.ptr()),
                           arena.ptr());
}

template <typename T>
typename T::Proxy CloneMessage(Ptr<T> message, upb_Arena* arena) {
  return ::hpb::internal::PrivateAccess::Proxy<T>(
      ::hpb::internal::DeepClone(hpb::interop::upb::GetMessage(message),
                                 T::minitable(), arena),
      arena);
}

template <typename T>
void DeepCopy(Ptr<const T> source_message, Ptr<T> target_message) {
  static_assert(!std::is_const_v<T>);
  ::hpb::internal::DeepCopy(
      hpb::interop::upb::GetMessage(target_message),
      hpb::interop::upb::GetMessage(source_message), T::minitable(),
      static_cast<upb_Arena*>(target_message->GetInternalArena()));
}

template <typename T>
void DeepCopy(Ptr<const T> source_message, T* target_message) {
  static_assert(!std::is_const_v<T>);
  DeepCopy(source_message, Ptr(target_message));
}

template <typename T>
void DeepCopy(const T* source_message, Ptr<T> target_message) {
  static_assert(!std::is_const_v<T>);
  DeepCopy(Ptr(source_message), target_message);
}

template <typename T>
void DeepCopy(const T* source_message, T* target_message) {
  static_assert(!std::is_const_v<T>);
  DeepCopy(Ptr(source_message), Ptr(target_message));
}

template <typename T>
void ClearMessage(hpb::internal::PtrOrRaw<T> message) {
  backend::ClearMessage(message);
}

template <typename T>
ABSL_MUST_USE_RESULT bool Parse(Ptr<T> message, absl::string_view bytes) {
  static_assert(!std::is_const_v<T>);
  upb_Message_Clear(hpb::interop::upb::GetMessage(message),
                    ::hpb::interop::upb::GetMiniTable(message));
  auto* arena = static_cast<upb_Arena*>(message->GetInternalArena());
  return upb_Decode(bytes.data(), bytes.size(),
                    hpb::interop::upb::GetMessage(message),
                    ::hpb::interop::upb::GetMiniTable(message),
                    /* extreg= */ nullptr, /* options= */ 0,
                    arena) == kUpb_DecodeStatus_Ok;
}

template <typename T>
ABSL_MUST_USE_RESULT bool Parse(
    Ptr<T> message, absl::string_view bytes,
    const ::hpb::ExtensionRegistry& extension_registry) {
  static_assert(!std::is_const_v<T>);
  upb_Message_Clear(hpb::interop::upb::GetMessage(message),
                    ::hpb::interop::upb::GetMiniTable(message));
  auto* arena = static_cast<upb_Arena*>(message->GetInternalArena());
  return upb_Decode(bytes.data(), bytes.size(),
                    hpb::interop::upb::GetMessage(message),
                    ::hpb::interop::upb::GetMiniTable(message),
                    /* extreg= */
                    ::hpb::internal::GetUpbExtensions(extension_registry),
                    /* options= */ 0, arena) == kUpb_DecodeStatus_Ok;
}

template <typename T>
ABSL_MUST_USE_RESULT bool Parse(
    T* message, absl::string_view bytes,
    const ::hpb::ExtensionRegistry& extension_registry) {
  static_assert(!std::is_const_v<T>);
  return Parse(Ptr(message, bytes, extension_registry));
}

template <typename T>
ABSL_MUST_USE_RESULT bool Parse(T* message, absl::string_view bytes) {
  static_assert(!std::is_const_v<T>);
  upb_Message_Clear(hpb::interop::upb::GetMessage(message),
                    ::hpb::interop::upb::GetMiniTable(message));
  auto* arena = static_cast<upb_Arena*>(message->GetInternalArena());
  return upb_Decode(bytes.data(), bytes.size(),
                    hpb::interop::upb::GetMessage(message),
                    ::hpb::interop::upb::GetMiniTable(message),
                    /* extreg= */ nullptr, /* options= */ 0,
                    arena) == kUpb_DecodeStatus_Ok;
}

template <typename T>
absl::StatusOr<T> Parse(absl::string_view bytes, int options = 0) {
  T message;
  auto* arena = static_cast<upb_Arena*>(message.GetInternalArena());
  upb_DecodeStatus status =
      upb_Decode(bytes.data(), bytes.size(), message.msg(),
                 ::hpb::interop::upb::GetMiniTable(&message),
                 /* extreg= */ nullptr, /* options= */ 0, arena);
  if (status == kUpb_DecodeStatus_Ok) {
    return message;
  }
  return MessageDecodeError(status);
}

template <typename T>
absl::StatusOr<T> Parse(absl::string_view bytes,
                        const ::hpb::ExtensionRegistry& extension_registry,
                        int options = 0) {
  T message;
  auto* arena = static_cast<upb_Arena*>(message.GetInternalArena());
  upb_DecodeStatus status =
      upb_Decode(bytes.data(), bytes.size(), message.msg(),
                 ::hpb::interop::upb::GetMiniTable(&message),
                 ::hpb::internal::GetUpbExtensions(extension_registry),
                 /* options= */ 0, arena);
  if (status == kUpb_DecodeStatus_Ok) {
    return message;
  }
  return MessageDecodeError(status);
}

template <typename T>
absl::StatusOr<absl::string_view> Serialize(const T* message, upb::Arena& arena,
                                            int options = 0) {
  return ::hpb::internal::Serialize(hpb::interop::upb::GetMessage(message),
                                    ::hpb::interop::upb::GetMiniTable(message),
                                    arena.ptr(), options);
}

template <typename T>
absl::StatusOr<absl::string_view> Serialize(Ptr<T> message, upb::Arena& arena,
                                            int options = 0) {
  return ::hpb::internal::Serialize(hpb::interop::upb::GetMessage(message),
                                    ::hpb::interop::upb::GetMiniTable(message),
                                    arena.ptr(), options);
}

}  // namespace hpb

#endif  // PROTOBUF_HPB_HPB_H_
