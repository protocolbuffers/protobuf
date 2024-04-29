// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef UPB_PROTOS_PROTOS_H_
#define UPB_PROTOS_PROTOS_H_

#include <type_traits>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "upb/base/status.hpp"
#include "upb/mem/arena.hpp"
#include "upb/message/copy.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/extension.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

namespace protos {

using Arena = ::upb::Arena;
class ExtensionRegistry;

template <typename T>
using Proxy = std::conditional_t<std::is_const<T>::value,
                                 typename std::remove_const_t<T>::CProxy,
                                 typename T::Proxy>;

// Provides convenient access to Proxy and CProxy message types.
//
// Using rebinding and handling of const, Ptr<Message> and Ptr<const Message>
// allows copying const with T* const and avoids using non-copyable Proxy types
// directly.
template <typename T>
class Ptr final {
 public:
  Ptr() = delete;

  // Implicit conversions
  Ptr(T* m) : p_(m) {}                // NOLINT
  Ptr(const Proxy<T>* p) : p_(*p) {}  // NOLINT
  Ptr(Proxy<T> p) : p_(p) {}          // NOLINT
  Ptr(const Ptr& m) = default;

  Ptr& operator=(Ptr v) & {
    Proxy<T>::Rebind(p_, v.p_);
    return *this;
  }

  Proxy<T> operator*() const { return p_; }
  Proxy<T>* operator->() const {
    return const_cast<Proxy<T>*>(std::addressof(p_));
  }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wclass-conversion"
#endif
  template <typename U = T, std::enable_if_t<!std::is_const<U>::value, int> = 0>
  operator Ptr<const T>() const {
    Proxy<const T> p(p_);
    return Ptr<const T>(&p);
  }
#ifdef __clang__
#pragma clang diagnostic pop
#endif

 private:
  Ptr(void* msg, upb_Arena* arena) : p_(msg, arena) {}  // NOLINT

  friend class Ptr<const T>;
  friend typename T::Access;

  Proxy<T> p_;
};

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

template <typename T>
typename T::Proxy CreateMessage(::protos::Arena& arena) {
  return typename T::Proxy(upb_Message_New(T::minitable(), arena.ptr()),
                           arena.ptr());
}

// begin:github_only
// This type exists to work around an absl type that has not yet been
// released.
struct SourceLocation {
  static SourceLocation current() { return {}; }
  absl::string_view file_name() { return "<unknown>"; }
  int line() { return 0; }
};
// end:github_only

// begin:google_only
// using SourceLocation = absl::SourceLocation;
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
struct PrivateAccess {
  template <typename T>
  static auto* GetInternalMsg(T&& message) {
    return message->msg();
  }
  template <typename T>
  static auto Proxy(void* p, upb_Arena* arena) {
    return typename T::Proxy(p, arena);
  }
  template <typename T>
  static auto CProxy(const void* p, upb_Arena* arena) {
    return typename T::CProxy(p, arena);
  }
};

template <typename T>
auto* GetInternalMsg(T&& message) {
  return PrivateAccess::GetInternalMsg(std::forward<T>(message));
}

template <typename T>
T CreateMessage() {
  return T();
}

template <typename T>
typename T::Proxy CreateMessageProxy(void* msg, upb_Arena* arena) {
  return typename T::Proxy(msg, arena);
}

template <typename T>
typename T::CProxy CreateMessage(const upb_Message* msg, upb_Arena* arena) {
  return PrivateAccess::CProxy<T>(msg, arena);
}

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

const upb_Message_Extension* GetOrPromoteExtension(
    upb_Message* msg, const upb_MiniTableExtension* eid, upb_Arena* arena);

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

template <typename T>
void DeepCopy(Ptr<const T> source_message, Ptr<T> target_message) {
  static_assert(!std::is_const_v<T>);
  ::protos::internal::DeepCopy(
      internal::GetInternalMsg(target_message),
      internal::GetInternalMsg(source_message), T::minitable(),
      static_cast<upb_Arena*>(target_message->GetInternalArena()));
}

template <typename T>
typename T::Proxy CloneMessage(Ptr<T> message, upb_Arena* arena) {
  return internal::PrivateAccess::Proxy<T>(
      ::protos::internal::DeepClone(internal::GetInternalMsg(message),
                                    T::minitable(), arena),
      arena);
}

template <typename T>
void DeepCopy(Ptr<const T> source_message, T* target_message) {
  static_assert(!std::is_const_v<T>);
  DeepCopy(source_message, protos::Ptr(target_message));
}

template <typename T>
void DeepCopy(const T* source_message, Ptr<T> target_message) {
  static_assert(!std::is_const_v<T>);
  DeepCopy(protos::Ptr(source_message), target_message);
}

template <typename T>
void DeepCopy(const T* source_message, T* target_message) {
  static_assert(!std::is_const_v<T>);
  DeepCopy(protos::Ptr(source_message), protos::Ptr(target_message));
}

template <typename T>
void ClearMessage(Ptr<T> message) {
  static_assert(!std::is_const_v<T>, "");
  upb_Message_Clear(internal::GetInternalMsg(message), T::minitable());
}

template <typename T>
void ClearMessage(T* message) {
  ClearMessage(protos::Ptr(message));
}

class ExtensionRegistry {
 public:
  ExtensionRegistry(
      const std::vector<const ::protos::internal::ExtensionMiniTableProvider*>&
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
  friend upb_ExtensionRegistry* ::protos::internal::GetUpbExtensions(
      const ExtensionRegistry& extension_registry);
  upb_ExtensionRegistry* registry_;
};

template <typename T>
using EnableIfProtosClass = std::enable_if_t<
    std::is_base_of<typename T::Access, T>::value &&
    std::is_base_of<typename T::Access, typename T::ExtendableType>::value>;

template <typename T>
using EnableIfMutableProto = std::enable_if_t<!std::is_const<T>::value>;

template <typename T, typename Extendee, typename Extension,
          typename = EnableIfProtosClass<T>>
ABSL_MUST_USE_RESULT bool HasExtension(
    Ptr<T> message,
    const ::protos::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  return ::protos::internal::HasExtensionOrUnknown(
      ::protos::internal::GetInternalMsg(message), id.mini_table_ext());
}

template <typename T, typename Extendee, typename Extension,
          typename = EnableIfProtosClass<T>>
ABSL_MUST_USE_RESULT bool HasExtension(
    const T* message,
    const ::protos::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  return HasExtension(protos::Ptr(message), id);
}

template <typename T, typename Extendee, typename Extension,
          typename = EnableIfProtosClass<T>, typename = EnableIfMutableProto<T>>
void ClearExtension(
    Ptr<T> message,
    const ::protos::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  static_assert(!std::is_const_v<T>, "");
  _upb_Message_ClearExtensionField(internal::GetInternalMsg(message),
                                   id.mini_table_ext());
}

template <typename T, typename Extendee, typename Extension,
          typename = EnableIfProtosClass<T>>
void ClearExtension(
    T* message,
    const ::protos::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  ClearExtension(::protos::Ptr(message), id);
}

template <typename T, typename Extendee, typename Extension,
          typename = EnableIfProtosClass<T>, typename = EnableIfMutableProto<T>>
absl::Status SetExtension(
    Ptr<T> message,
    const ::protos::internal::ExtensionIdentifier<Extendee, Extension>& id,
    const Extension& value) {
  static_assert(!std::is_const_v<T>);
  auto* message_arena = static_cast<upb_Arena*>(message->GetInternalArena());
  return ::protos::internal::SetExtension(internal::GetInternalMsg(message),
                                          message_arena, id.mini_table_ext(),
                                          internal::GetInternalMsg(&value));
}

template <typename T, typename Extendee, typename Extension,
          typename = EnableIfProtosClass<T>, typename = EnableIfMutableProto<T>>
absl::Status SetExtension(
    Ptr<T> message,
    const ::protos::internal::ExtensionIdentifier<Extendee, Extension>& id,
    Extension&& value) {
  Extension ext = std::move(value);
  static_assert(!std::is_const_v<T>);
  auto* message_arena = static_cast<upb_Arena*>(message->GetInternalArena());
  auto* extension_arena = static_cast<upb_Arena*>(ext.GetInternalArena());
  return ::protos::internal::MoveExtension(
      internal::GetInternalMsg(message), message_arena, id.mini_table_ext(),
      internal::GetInternalMsg(&ext), extension_arena);
}

template <typename T, typename Extendee, typename Extension,
          typename = EnableIfProtosClass<T>>
absl::Status SetExtension(
    T* message,
    const ::protos::internal::ExtensionIdentifier<Extendee, Extension>& id,
    const Extension& value) {
  return ::protos::SetExtension(::protos::Ptr(message), id, value);
}

template <typename T, typename Extendee, typename Extension,
          typename = EnableIfProtosClass<T>>
absl::Status SetExtension(
    T* message,
    const ::protos::internal::ExtensionIdentifier<Extendee, Extension>& id,
    Extension&& value) {
  return ::protos::SetExtension(::protos::Ptr(message), id,
                                std::forward<Extension>(value));
}

template <typename T, typename Extendee, typename Extension,
          typename = EnableIfProtosClass<T>>
absl::StatusOr<Ptr<const Extension>> GetExtension(
    Ptr<T> message,
    const ::protos::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  // TODO: Fix const correctness issues.
  const upb_Message_Extension* ext = ::protos::internal::GetOrPromoteExtension(
      const_cast<upb_Message*>(internal::GetInternalMsg(message)),
      id.mini_table_ext(), ::protos::internal::GetArena(message));
  if (!ext) {
    return ExtensionNotFoundError(id.mini_table_ext()->field.number);
  }
  return Ptr<const Extension>(::protos::internal::CreateMessage<Extension>(
      ext->data.ptr, ::protos::internal::GetArena(message)));
}

template <typename T, typename Extendee, typename Extension,
          typename = EnableIfProtosClass<T>>
absl::StatusOr<Ptr<const Extension>> GetExtension(
    const T* message,
    const ::protos::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  return GetExtension(protos::Ptr(message), id);
}

template <typename T>
ABSL_MUST_USE_RESULT bool Parse(Ptr<T> message, absl::string_view bytes) {
  static_assert(!std::is_const_v<T>);
  upb_Message_Clear(internal::GetInternalMsg(message),
                    ::protos::internal::GetMiniTable(message));
  auto* arena = static_cast<upb_Arena*>(message->GetInternalArena());
  return upb_Decode(bytes.data(), bytes.size(),
                    internal::GetInternalMsg(message),
                    ::protos::internal::GetMiniTable(message),
                    /* extreg= */ nullptr, /* options= */ 0,
                    arena) == kUpb_DecodeStatus_Ok;
}

template <typename T>
ABSL_MUST_USE_RESULT bool Parse(
    Ptr<T> message, absl::string_view bytes,
    const ::protos::ExtensionRegistry& extension_registry) {
  static_assert(!std::is_const_v<T>);
  upb_Message_Clear(internal::GetInternalMsg(message),
                    ::protos::internal::GetMiniTable(message));
  auto* arena = static_cast<upb_Arena*>(message->GetInternalArena());
  return upb_Decode(bytes.data(), bytes.size(),
                    internal::GetInternalMsg(message),
                    ::protos::internal::GetMiniTable(message),
                    /* extreg= */
                    ::protos::internal::GetUpbExtensions(extension_registry),
                    /* options= */ 0, arena) == kUpb_DecodeStatus_Ok;
}

template <typename T>
ABSL_MUST_USE_RESULT bool Parse(
    T* message, absl::string_view bytes,
    const ::protos::ExtensionRegistry& extension_registry) {
  static_assert(!std::is_const_v<T>);
  return Parse(protos::Ptr(message, bytes, extension_registry));
}

template <typename T>
ABSL_MUST_USE_RESULT bool Parse(T* message, absl::string_view bytes) {
  static_assert(!std::is_const_v<T>);
  upb_Message_Clear(internal::GetInternalMsg(message),
                    ::protos::internal::GetMiniTable(message));
  auto* arena = static_cast<upb_Arena*>(message->GetInternalArena());
  return upb_Decode(bytes.data(), bytes.size(),
                    internal::GetInternalMsg(message),
                    ::protos::internal::GetMiniTable(message),
                    /* extreg= */ nullptr, /* options= */ 0,
                    arena) == kUpb_DecodeStatus_Ok;
}

template <typename T>
absl::StatusOr<T> Parse(absl::string_view bytes, int options = 0) {
  T message;
  auto* arena = static_cast<upb_Arena*>(message.GetInternalArena());
  upb_DecodeStatus status =
      upb_Decode(bytes.data(), bytes.size(), message.msg(),
                 ::protos::internal::GetMiniTable(&message),
                 /* extreg= */ nullptr, /* options= */ 0, arena);
  if (status == kUpb_DecodeStatus_Ok) {
    return message;
  }
  return MessageDecodeError(status);
}

template <typename T>
absl::StatusOr<T> Parse(absl::string_view bytes,
                        const ::protos::ExtensionRegistry& extension_registry,
                        int options = 0) {
  T message;
  auto* arena = static_cast<upb_Arena*>(message.GetInternalArena());
  upb_DecodeStatus status =
      upb_Decode(bytes.data(), bytes.size(), message.msg(),
                 ::protos::internal::GetMiniTable(&message),
                 ::protos::internal::GetUpbExtensions(extension_registry),
                 /* options= */ 0, arena);
  if (status == kUpb_DecodeStatus_Ok) {
    return message;
  }
  return MessageDecodeError(status);
}

template <typename T>
absl::StatusOr<absl::string_view> Serialize(const T* message, upb::Arena& arena,
                                            int options = 0) {
  return ::protos::internal::Serialize(
      internal::GetInternalMsg(message),
      ::protos::internal::GetMiniTable(message), arena.ptr(), options);
}

template <typename T>
absl::StatusOr<absl::string_view> Serialize(Ptr<T> message, upb::Arena& arena,
                                            int options = 0) {
  return ::protos::internal::Serialize(
      internal::GetInternalMsg(message),
      ::protos::internal::GetMiniTable(message), arena.ptr(), options);
}

}  // namespace protos

#endif  // UPB_PROTOS_PROTOS_H_
