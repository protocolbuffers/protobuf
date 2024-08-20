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
#include "google/protobuf/hpb/internal/internal.h"
#include "google/protobuf/hpb/internal/template_help.h"
#include "google/protobuf/hpb/ptr.h"
#include "upb/base/status.hpp"
#include "upb/mem/arena.hpp"
#include "upb/message/copy.h"
#include "upb/mini_table/extension.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

namespace hpb {
class ExtensionRegistry;
using Arena = ::upb::Arena;

// TODO: b/354766950 - Move upb-specific chunks out of hpb header
inline absl::string_view UpbStrToStringView(upb_StringView str) {
  return absl::string_view(str.data, str.size);
}

// TODO: update bzl and move to upb runtime / protos.cc.
inline upb_StringView UpbStrFromStringView(absl::string_view str,
                                           upb_Arena* arena) {
  const size_t str_size = str.size();
  char* buffer = static_cast<char*>(upb_Arena_Malloc(arena, str_size));
  memcpy(buffer, str.data(), str_size);
  return upb_StringView_FromDataAndSize(buffer, str_size);
}

// begin:github_only
// // This type exists to work around an absl type that has not yet been
// // released.
// struct SourceLocation {
//   static SourceLocation current() { return {}; }
//   absl::string_view file_name() { return "<unknown>"; }
//   int line() { return 0; }
// };
// end:github_only

// begin:google_only
using SourceLocation = absl::SourceLocation;
// end:google_only

absl::Status MessageAllocationError(
    SourceLocation loc = SourceLocation::current());

absl::Status ExtensionNotFoundError(
    int extension_number, SourceLocation loc = SourceLocation::current());

absl::Status MessageDecodeError(upb_DecodeStatus status,
                                SourceLocation loc = SourceLocation::current());

absl::Status MessageEncodeError(upb_EncodeStatus status,
                                SourceLocation loc = SourceLocation::current());

namespace internal {
class ExtensionMiniTableProvider {
 public:
  constexpr explicit ExtensionMiniTableProvider(
      const upb_MiniTableExtension* mini_table_ext)
      : mini_table_ext_(mini_table_ext) {}
  const upb_MiniTableExtension* mini_table_ext() const {
    return mini_table_ext_;
  }

 private:
  const upb_MiniTableExtension* mini_table_ext_;
};

// -------------------------------------------------------------------
// ExtensionIdentifier
// This is the type of actual extension objects.  E.g. if you have:
//   extend Foo {
//     optional MyExtension bar = 1234;
//   }
// then "bar" will be defined in C++ as:
//   ExtensionIdentifier<Foo, MyExtension> bar(&namespace_bar_ext);
template <typename ExtendeeType, typename ExtensionType>
class ExtensionIdentifier : public ExtensionMiniTableProvider {
 public:
  using Extension = ExtensionType;
  using Extendee = ExtendeeType;

  constexpr explicit ExtensionIdentifier(
      const upb_MiniTableExtension* mini_table_ext)
      : ExtensionMiniTableProvider(mini_table_ext) {}

 private:
  constexpr uint32_t number() const {
    return upb_MiniTableExtension_Number(mini_table_ext());
  }
  friend class PrivateAccess;
};

template <typename T>
upb_Arena* GetArena(Ptr<T> message) {
  return static_cast<upb_Arena*>(message->GetInternalArena());
}

template <typename T>
upb_Arena* GetArena(T* message) {
  return static_cast<upb_Arena*>(message->GetInternalArena());
}

template <typename T>
const upb_MiniTable* GetMiniTable(const T*) {
  return T::minitable();
}

template <typename T>
const upb_MiniTable* GetMiniTable(Ptr<T>) {
  return T::minitable();
}

upb_ExtensionRegistry* GetUpbExtensions(
    const ExtensionRegistry& extension_registry);

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

class ExtensionRegistry {
 public:
  ExtensionRegistry(
      const std::vector<const internal::ExtensionMiniTableProvider*>&
          extensions,
      const upb::Arena& arena)
      : registry_(upb_ExtensionRegistry_New(arena.ptr())) {
    if (registry_) {
      for (const auto& ext_provider : extensions) {
        const auto* ext = ext_provider->mini_table_ext();
        bool success = upb_ExtensionRegistry_AddArray(registry_, &ext, 1);
        if (!success) {
          registry_ = nullptr;
          break;
        }
      }
    }
  }

 private:
  friend upb_ExtensionRegistry* ::hpb::internal::GetUpbExtensions(
      const ExtensionRegistry& extension_registry);
  upb_ExtensionRegistry* registry_;
};

template <typename T, typename Extendee, typename Extension,
          typename = hpb::internal::EnableIfHpbClass<T>>
ABSL_MUST_USE_RESULT bool HasExtension(
    Ptr<T> message,
    const ::hpb::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  return ::hpb::internal::HasExtensionOrUnknown(
      ::hpb::internal::GetInternalMsg(message), id.mini_table_ext());
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
  upb_Message_ClearExtension(hpb::internal::GetInternalMsg(message),
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
  return ::hpb::internal::SetExtension(hpb::internal::GetInternalMsg(message),
                                       message_arena, id.mini_table_ext(),
                                       hpb::internal::GetInternalMsg(&value));
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
  return ::hpb::internal::SetExtension(hpb::internal::GetInternalMsg(message),
                                       message_arena, id.mini_table_ext(),
                                       hpb::internal::GetInternalMsg(value));
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
  return ::hpb::internal::MoveExtension(hpb::internal::GetInternalMsg(message),
                                        message_arena, id.mini_table_ext(),
                                        hpb::internal::GetInternalMsg(&ext),
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
      const_cast<upb_Message*>(::hpb::internal::GetInternalMsg(message)),
      id.mini_table_ext(), ::hpb::internal::GetArena(message), &value);
  if (!ok) {
    return ExtensionNotFoundError(
        upb_MiniTableExtension_Number(id.mini_table_ext()));
  }
  return Ptr<const Extension>(::hpb::internal::CreateMessage<Extension>(
      (upb_Message*)value.msg_val, ::hpb::internal::GetArena(message)));
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
      ::hpb::internal::DeepClone(::hpb::internal::GetInternalMsg(message),
                                 T::minitable(), arena),
      arena);
}

template <typename T>
void DeepCopy(Ptr<const T> source_message, Ptr<T> target_message) {
  static_assert(!std::is_const_v<T>);
  ::hpb::internal::DeepCopy(
      hpb::internal::GetInternalMsg(target_message),
      hpb::internal::GetInternalMsg(source_message), T::minitable(),
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
  auto ptr = Ptr(message);
  auto minitable = hpb::internal::GetMiniTable(ptr);
  upb_Message_Clear(hpb::internal::GetInternalMsg(ptr), minitable);
}

template <typename T>
ABSL_MUST_USE_RESULT bool Parse(Ptr<T> message, absl::string_view bytes) {
  static_assert(!std::is_const_v<T>);
  upb_Message_Clear(::hpb::internal::GetInternalMsg(message),
                    ::hpb::internal::GetMiniTable(message));
  auto* arena = static_cast<upb_Arena*>(message->GetInternalArena());
  return upb_Decode(bytes.data(), bytes.size(),
                    ::hpb::internal::GetInternalMsg(message),
                    ::hpb::internal::GetMiniTable(message),
                    /* extreg= */ nullptr, /* options= */ 0,
                    arena) == kUpb_DecodeStatus_Ok;
}

template <typename T>
ABSL_MUST_USE_RESULT bool Parse(
    Ptr<T> message, absl::string_view bytes,
    const ::hpb::ExtensionRegistry& extension_registry) {
  static_assert(!std::is_const_v<T>);
  upb_Message_Clear(::hpb::internal::GetInternalMsg(message),
                    ::hpb::internal::GetMiniTable(message));
  auto* arena = static_cast<upb_Arena*>(message->GetInternalArena());
  return upb_Decode(bytes.data(), bytes.size(),
                    ::hpb::internal::GetInternalMsg(message),
                    ::hpb::internal::GetMiniTable(message),
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
  upb_Message_Clear(::hpb::internal::GetInternalMsg(message),
                    ::hpb::internal::GetMiniTable(message));
  auto* arena = static_cast<upb_Arena*>(message->GetInternalArena());
  return upb_Decode(bytes.data(), bytes.size(),
                    ::hpb::internal::GetInternalMsg(message),
                    ::hpb::internal::GetMiniTable(message),
                    /* extreg= */ nullptr, /* options= */ 0,
                    arena) == kUpb_DecodeStatus_Ok;
}

template <typename T>
absl::StatusOr<T> Parse(absl::string_view bytes, int options = 0) {
  T message;
  auto* arena = static_cast<upb_Arena*>(message.GetInternalArena());
  upb_DecodeStatus status =
      upb_Decode(bytes.data(), bytes.size(), message.msg(),
                 ::hpb::internal::GetMiniTable(&message),
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
                 ::hpb::internal::GetMiniTable(&message),
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
  return ::hpb::internal::Serialize(::hpb::internal::GetInternalMsg(message),
                                    ::hpb::internal::GetMiniTable(message),
                                    arena.ptr(), options);
}

template <typename T>
absl::StatusOr<absl::string_view> Serialize(Ptr<T> message, upb::Arena& arena,
                                            int options = 0) {
  return ::hpb::internal::Serialize(::hpb::internal::GetInternalMsg(message),
                                    ::hpb::internal::GetMiniTable(message),
                                    arena.ptr(), options);
}

}  // namespace hpb

#endif  // PROTOBUF_HPB_HPB_H_
