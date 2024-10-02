// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_EXTENSION_H__
#define GOOGLE_PROTOBUF_HPB_EXTENSION_H__

#include <cstdint>
#include <vector>

#include "absl/base/attributes.h"
#include "google/protobuf/hpb/backend/upb/interop.h"
#include "google/protobuf/hpb/internal/message_lock.h"
#include "google/protobuf/hpb/internal/template_help.h"
#include "google/protobuf/hpb/ptr.h"
#include "upb/mem/arena.hpp"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"

namespace hpb {
class ExtensionRegistry;

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
  friend struct PrivateAccess;
};

upb_ExtensionRegistry* GetUpbExtensions(
    const ExtensionRegistry& extension_registry);

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

}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_EXTENSION_H__
