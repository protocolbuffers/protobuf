// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_EXTENSION_H__
#define GOOGLE_PROTOBUF_HPB_EXTENSION_H__

#include <cstdint>
#include <type_traits>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "google/protobuf/hpb/backend/upb/interop.h"
#include "google/protobuf/hpb/internal/message_lock.h"
#include "google/protobuf/hpb/internal/template_help.h"
#include "google/protobuf/hpb/ptr.h"
#include "google/protobuf/hpb/status.h"
#include "upb/mem/arena.hpp"
#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"

namespace hpb {
class ExtensionRegistry;

namespace internal {

absl::Status MoveExtension(upb_Message* message, upb_Arena* message_arena,
                           const upb_MiniTableExtension* ext,
                           upb_Message* extension, upb_Arena* extension_arena);

absl::Status SetExtension(upb_Message* message, upb_Arena* message_arena,
                          const upb_MiniTableExtension* ext,
                          const upb_Message* extension);

/**
 * Trait that maps upb extension types to the corresponding
 * return value: ubp_MessageValue.
 *
 * All partial specializations must have:
 * - DefaultType: the type of the default value.
 * - ReturnType: the type of the return value.
 * - kGetter: the corresponding upb_MessageValue upb_Message_GetExtension* func
 */
template <typename T, typename = void>
struct UpbExtensionTrait;

template <>
struct UpbExtensionTrait<int32_t> {
  using DefaultType = int32_t;
  using ReturnType = int32_t;
  static constexpr auto kGetter = upb_Message_GetExtensionInt32;
  static constexpr auto kSetter = upb_Message_SetExtensionInt32;
};

template <>
struct UpbExtensionTrait<int64_t> {
  using DefaultType = int64_t;
  using ReturnType = int64_t;
  static constexpr auto kGetter = upb_Message_GetExtensionInt64;
  static constexpr auto kSetter = upb_Message_SetExtensionInt64;
};

// TODO: b/375460289 - flesh out non-promotional msg support that does
// not return an error if missing but the default msg
template <typename T>
struct UpbExtensionTrait<T> {
  using DefaultType = int;
  using ReturnType = int;
  using DefaultFuncType = void (*)();
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
class ExtensionIdentifier {
 public:
  using Extension = ExtensionType;
  using Extendee = ExtendeeType;

  // Placeholder for extant legacy callers, avoid use if possible
  const upb_MiniTableExtension* mini_table_ext() const {
    return mini_table_ext_;
  }

 private:
  constexpr explicit ExtensionIdentifier(
      const upb_MiniTableExtension* mte,
      typename UpbExtensionTrait<ExtensionType>::DefaultType val)
      : mini_table_ext_(mte), default_val_(val) {}

  constexpr uint32_t number() const {
    return upb_MiniTableExtension_Number(mini_table_ext_);
  }

  const upb_MiniTableExtension* mini_table_ext_;

  typename UpbExtensionTrait<ExtensionType>::ReturnType default_value() const {
    return default_val_;
  }

  typename UpbExtensionTrait<ExtensionType>::DefaultType default_val_;

  friend struct PrivateAccess;
};

upb_ExtensionRegistry* GetUpbExtensions(
    const ExtensionRegistry& extension_registry);

}  // namespace internal

class ExtensionRegistry {
 public:
  ExtensionRegistry(
      const std::vector<const upb_MiniTableExtension*>& extensions,
      const upb::Arena& arena)
      : registry_(upb_ExtensionRegistry_New(arena.ptr())) {
    if (registry_) {
      for (const auto extension : extensions) {
        const auto* ext = extension;
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
          typename = hpb::internal::EnableIfHpbClassThatHasExtensions<T>>
ABSL_MUST_USE_RESULT bool HasExtension(
    Ptr<T> message,
    const ::hpb::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  return ::hpb::internal::HasExtensionOrUnknown(
      hpb::interop::upb::GetMessage(message), id.mini_table_ext());
}

template <typename T, typename Extendee, typename Extension,
          typename = hpb::internal::EnableIfHpbClassThatHasExtensions<T>>
ABSL_MUST_USE_RESULT bool HasExtension(
    const T* message,
    const ::hpb::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  return HasExtension(Ptr(message), id);
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClassThatHasExtensions<T>,
          typename = hpb::internal::EnableIfMutableProto<T>>
void ClearExtension(
    Ptr<T> message,
    const ::hpb::internal::ExtensionIdentifier<T, Extension>& id) {
  static_assert(!std::is_const_v<T>, "");
  upb_Message_ClearExtension(hpb::interop::upb::GetMessage(message),
                             id.mini_table_ext());
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClassThatHasExtensions<T>>
void ClearExtension(
    T* message, const ::hpb::internal::ExtensionIdentifier<T, Extension>& id) {
  ClearExtension(Ptr(message), id);
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClassThatHasExtensions<T>,
          typename = hpb::internal::EnableIfMutableProto<T>>
absl::Status SetExtension(
    Ptr<T> message,
    const ::hpb::internal::ExtensionIdentifier<T, Extension>& id,
    const Extension& value) {
  if constexpr (std::is_integral_v<Extension>) {
    bool res = hpb::internal::UpbExtensionTrait<Extension>::kSetter(
        hpb::interop::upb::GetMessage(message), id.mini_table_ext(), value,
        hpb::interop::upb::GetArena(message));
    return res ? absl::OkStatus() : MessageAllocationError();
  } else {
    static_assert(!std::is_const_v<T>);
    auto* message_arena = hpb::interop::upb::GetArena(message);
    return ::hpb::internal::SetExtension(hpb::interop::upb::GetMessage(message),
                                         message_arena, id.mini_table_ext(),
                                         hpb::interop::upb::GetMessage(&value));
  }
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClassThatHasExtensions<T>,
          typename = hpb::internal::EnableIfMutableProto<T>>
absl::Status SetExtension(
    Ptr<T> message,
    const ::hpb::internal::ExtensionIdentifier<T, Extension>& id,
    Ptr<Extension> value) {
  static_assert(!std::is_const_v<T>);
  auto* message_arena = hpb::interop::upb::GetArena(message);
  return ::hpb::internal::SetExtension(hpb::interop::upb::GetMessage(message),
                                       message_arena, id.mini_table_ext(),
                                       hpb::interop::upb::GetMessage(value));
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClassThatHasExtensions<T>,
          typename = hpb::internal::EnableIfMutableProto<T>>
absl::Status SetExtension(
    Ptr<T> message,
    const ::hpb::internal::ExtensionIdentifier<T, Extension>& id,
    Extension&& value) {
  if constexpr (std::is_integral_v<Extension>) {
    bool res = hpb::internal::UpbExtensionTrait<Extension>::kSetter(
        hpb::interop::upb::GetMessage(message), id.mini_table_ext(), value,
        hpb::interop::upb::GetArena(message));
    return res ? absl::OkStatus() : MessageAllocationError();
  } else {
    Extension ext = std::forward<Extension>(value);
    static_assert(!std::is_const_v<T>);
    auto* message_arena = hpb::interop::upb::GetArena(message);
    auto* extension_arena = hpb::interop::upb::GetArena(&ext);
    return ::hpb::internal::MoveExtension(
        hpb::interop::upb::GetMessage(message), message_arena,
        id.mini_table_ext(), hpb::interop::upb::GetMessage(&ext),
        extension_arena);
  }
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClassThatHasExtensions<T>>
absl::Status SetExtension(
    T* message, const ::hpb::internal::ExtensionIdentifier<T, Extension>& id,
    const Extension& value) {
  return ::hpb::SetExtension(Ptr(message), id, value);
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClassThatHasExtensions<T>>
absl::Status SetExtension(
    T* message, const ::hpb::internal::ExtensionIdentifier<T, Extension>& id,
    Extension&& value) {
  return ::hpb::SetExtension(Ptr(message), id, std::forward<Extension>(value));
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClassThatHasExtensions<T>>
absl::Status SetExtension(
    T* message, const ::hpb::internal::ExtensionIdentifier<T, Extension>& id,
    Ptr<Extension> value) {
  return ::hpb::SetExtension(Ptr(message), id, value);
}

template <typename T, typename Extendee, typename Extension,
          typename = hpb::internal::EnableIfHpbClassThatHasExtensions<T>>
absl::StatusOr<Ptr<const Extension>> GetExtension(
    Ptr<T> message,
    const ::hpb::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  upb_MessageValue value;
  const bool ok = ::hpb::internal::GetOrPromoteExtension(
      hpb::interop::upb::GetMessage(message), id.mini_table_ext(),
      hpb::interop::upb::GetArena(message), &value);
  if (!ok) {
    return ExtensionNotFoundError(
        upb_MiniTableExtension_Number(id.mini_table_ext()));
  }
  return Ptr<const Extension>(::hpb::interop::upb::MakeCHandle<Extension>(
      (upb_Message*)value.msg_val, hpb::interop::upb::GetArena(message)));
}

template <typename T, typename Extendee, typename Extension,
          typename = hpb::internal::EnableIfHpbClassThatHasExtensions<T>>
decltype(auto) GetExtension(
    const T* message,
    const hpb::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  if constexpr (std::is_integral_v<Extension>) {
    auto default_val = hpb::internal::PrivateAccess::GetDefaultValue(id);
    absl::StatusOr<Extension> res =
        hpb::internal::UpbExtensionTrait<Extension>::kGetter(
            hpb::interop::upb::GetMessage(message), id.mini_table_ext(),
            default_val);
    return res;
  } else {
    return GetExtension(Ptr(message), id);
  }
}

template <typename T, typename Extension>
constexpr uint32_t ExtensionNumber(
    const internal::ExtensionIdentifier<T, Extension>& id) {
  return internal::PrivateAccess::GetExtensionNumber(id);
}

}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_EXTENSION_H__
