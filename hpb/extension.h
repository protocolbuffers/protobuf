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

#include "absl/base/attributes.h"
#include "hpb/arena.h"
#include "hpb/backend/upb/extension.h"
#include "hpb/backend/upb/interop.h"
#include "hpb/internal/message_lock.h"
#include "hpb/internal/template_help.h"
#include "hpb/multibackend.h"
#include "hpb/ptr.h"
#include "upb/message/accessors.h"
#include "upb/mini_table/extension_registry.h"

namespace hpb {
// upb has a notion of an ExtensionRegistry. We expect most callers to use
// the generated registry, which utilizes upb linker arrays. It is also possible
// to call hpb funcs with hpb::ExtensionRegistry::empty_registry().
//
// Since google::protobuf::cpp only has the generated registry, hpb funcs
// that use an extension registry must be invoked with
// hpb::ExtensionRegistry::generated_registry(). Note that
// hpb::ExtensionRegistry::empty_registry() does not even exist
// for the cpp backend.
class ExtensionRegistry {
 public:
#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
  // The lifetimes of the ExtensionRegistry and the Arena are disparate, but
  // the Arena must outlive the ExtensionRegistry.
  explicit ExtensionRegistry(const hpb::Arena& arena)
      : registry_(
            upb_ExtensionRegistry_New(hpb::interop::upb::UnwrapArena(arena))) {}

  template <typename ExtensionIdentifier>
  void AddExtension(const ExtensionIdentifier& id) {
    if (registry_) {
      auto* extension = id.mini_table_ext();
      upb_ExtensionRegistryStatus status =
          upb_ExtensionRegistry_AddArray(registry_, &extension, 1);
      if (status != kUpb_ExtensionRegistryStatus_Ok) {
        registry_ = nullptr;
      }
    }
  }

  static const ExtensionRegistry& empty_registry() {
    static const ExtensionRegistry* r = new ExtensionRegistry();
    return *r;
  }
#endif

  static const ExtensionRegistry& generated_registry() {
    static const ExtensionRegistry* r = NewGeneratedRegistry();
    return *r;
  }

 private:
#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
  friend upb_ExtensionRegistry* ::hpb::internal::GetUpbExtensions(
      const ExtensionRegistry& extension_registry);
  upb_ExtensionRegistry* registry_;
#endif
  // TODO: b/379100963 - Introduce ShutdownHpbLibrary
  static const ExtensionRegistry* NewGeneratedRegistry() {
#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
    static hpb::Arena* global_arena = new hpb::Arena();
    ExtensionRegistry* registry = new ExtensionRegistry(*global_arena);
    upb_ExtensionRegistry_AddAllLinkedExtensions(registry->registry_);
    return registry;
#elif HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_CPP
    ExtensionRegistry* registry = new ExtensionRegistry();
    return registry;
#else
#error "Unsupported hpb backend"
#endif
  }
  explicit ExtensionRegistry() = default;
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

/**
 * Sets the extension to provided value.
 *
 * `message` is the model and may be passed in as a `T*` or a `Ptr<T>`.
 *
 * `id` is the ExtensionIdentifier provided by hpb gencode.
 *
 * `value` is the value to set the extension to.
 *  For message extension it can bind to `const Input&`, `Input&&`,
 *  or `Ptr<const Input>`.
 *  For rvalue references, if the arenas match, the extension is moved.
 *  If the arenas differ, a deep copy is performed.
 */
template <int&... DeductionBarrier, typename T, typename Extension,
          typename Input>
auto SetExtension(
    hpb::internal::PtrOrRawMutable<T> message,
    const internal::ExtensionIdentifier<internal::RemovePtrT<T>, Extension>& id,
    Input&& value)
    -> decltype(internal::UpbExtensionTrait<Extension>::Set(
        message, id, std::forward<Input>(value))) {
  return internal::UpbExtensionTrait<Extension>::Set(
      message, id, std::forward<Input>(value));
}

template <typename T, typename Extension,
          typename = hpb::internal::EnableIfHpbClassThatHasExtensions<T>,
          typename = hpb::internal::EnableIfMutableProto<T>>
void SetAliasExtension(
    Ptr<T> message,
    const ::hpb::internal::ExtensionIdentifier<T, Extension>& id,
    Ptr<Extension> value) {
  static_assert(!std::is_const_v<T>);
  auto* message_arena = hpb::interop::upb::GetArena(message);
  auto* extension_arena = hpb::interop::upb::GetArena(value);
  return ::hpb::internal::SetAliasExtension(
      hpb::interop::upb::GetMessage(message), message_arena,
      id.mini_table_ext(), hpb::interop::upb::GetMessage(value),
      extension_arena);
}

template <typename T, typename Extendee, typename Extension,
          typename = hpb::internal::EnableIfHpbClassThatHasExtensions<T>>
absl::StatusOr<typename internal::UpbExtensionTrait<Extension>::ReturnType>
GetExtension(
    Ptr<T> message,
    const ::hpb::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  return hpb::internal::UpbExtensionTrait<Extension>::Get(message, id);
}

template <typename T, typename Extendee, typename Extension,
          typename = hpb::internal::EnableIfHpbClassThatHasExtensions<T>>
absl::StatusOr<typename internal::UpbExtensionTrait<Extension>::ReturnType>
GetExtension(
    const T* message,
    const hpb::internal::ExtensionIdentifier<Extendee, Extension>& id) {
  return GetExtension(Ptr(message), id);
}

template <typename T, typename Extension>
constexpr uint32_t ExtensionNumber(
    const internal::ExtensionIdentifier<T, Extension>& id) {
  return internal::PrivateAccess::GetExtensionNumber(id);
}

}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_EXTENSION_H__
