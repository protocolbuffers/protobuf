// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_BACKEND_UPB_EXTENSION_H__
#define GOOGLE_PROTOBUF_HPB_BACKEND_UPB_EXTENSION_H__

#include <cstdint>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "hpb/backend/upb/interop.h"
#include "hpb/internal/message_lock.h"
#include "hpb/internal/template_help.h"
#include "hpb/status.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"

namespace hpb {
class ExtensionRegistry;

template <typename T>
class RepeatedField;

namespace internal {
template <typename Extendee, typename Extension>
class ExtensionIdentifier;

absl::Status MoveExtension(upb_Message* message, upb_Arena* message_arena,
                           const upb_MiniTableExtension* ext,
                           upb_Message* extension, upb_Arena* extension_arena);

absl::Status SetExtension(upb_Message* message, upb_Arena* message_arena,
                          const upb_MiniTableExtension* ext,
                          const upb_Message* extension);

void SetAliasExtension(upb_Message* message, upb_Arena* message_arena,
                       const upb_MiniTableExtension* ext,
                       upb_Message* extension, upb_Arena* extension_arena);

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

template <typename T>
struct UpbExtensionTrait<hpb::RepeatedField<T>> {
  using ReturnType = typename RepeatedField<T>::CProxy;
  using DefaultType = std::false_type;

  template <typename Msg, typename Id>
  static constexpr ReturnType Get(Msg message, const Id& id) {
    auto upb_arr = upb_Message_GetExtensionArray(
        hpb::interop::upb::GetMessage(message), id.mini_table_ext());
    return ReturnType(upb_arr, hpb::interop::upb::GetArena(message));
  }
};
#define UPB_EXT_PRIMITIVE(CppType, UpbFunc)                              \
  template <>                                                            \
  struct UpbExtensionTrait<CppType> {                                    \
    using DefaultType = CppType;                                         \
    using ReturnType = CppType;                                          \
                                                                         \
    template <typename Msg, typename Id>                                 \
    static constexpr ReturnType Get(Msg message, const Id& id) {         \
      auto default_val = internal::PrivateAccess::GetDefaultValue(id);   \
      return upb_Message_GetExtension##UpbFunc(                          \
          interop::upb::GetMessage(message), id.mini_table_ext(),        \
          default_val);                                                  \
    }                                                                    \
    template <typename Msg, typename Id>                                 \
    static absl::Status Set(Msg message, const Id& id, CppType value) {  \
      bool res = upb_Message_SetExtension##UpbFunc(                      \
          interop::upb::GetMessage(message), id.mini_table_ext(), value, \
          interop::upb::GetArena(message));                              \
      return res ? absl::OkStatus() : MessageAllocationError();          \
    }                                                                    \
  }

UPB_EXT_PRIMITIVE(bool, Bool);
UPB_EXT_PRIMITIVE(int32_t, Int32);
UPB_EXT_PRIMITIVE(int64_t, Int64);
UPB_EXT_PRIMITIVE(uint32_t, UInt32);
UPB_EXT_PRIMITIVE(uint64_t, UInt64);
UPB_EXT_PRIMITIVE(float, Float);
UPB_EXT_PRIMITIVE(double, Double);

#undef UPB_EXT_PRIMITIVE

template <>
struct UpbExtensionTrait<absl::string_view> {
  using DefaultType = absl::string_view;
  using ReturnType = absl::string_view;

  template <typename Msg, typename Id>
  static constexpr ReturnType Get(Msg message, const Id& id) {
    auto default_val = hpb::internal::PrivateAccess::GetDefaultValue(id);
    upb_StringView result = upb_Message_GetExtensionString(
        hpb::interop::upb::GetMessage(message), id.mini_table_ext(),
        upb_StringView_FromDataAndSize(default_val.data(), default_val.size()));
    return absl::string_view(result.data, result.size);
  }

  template <typename Msg, typename Id>
  static absl::Status Set(Msg message, const Id& id, absl::string_view value) {
    auto upb_value = upb_StringView_FromDataAndSize(value.data(), value.size());
    bool res = upb_Message_SetExtensionString(interop::upb::GetMessage(message),
                                              id.mini_table_ext(), upb_value,
                                              interop::upb::GetArena(message));
    return res ? absl::OkStatus() : MessageAllocationError();
  }
};

// TODO: b/375460289 - flesh out non-promotional msg support that does
// not return an error if missing but the default msg
template <typename T>
struct UpbExtensionTrait<T> {
  using DefaultType = std::false_type;
  using ReturnType = Ptr<const T>;
  template <typename Msg, typename Id>
  static constexpr absl::StatusOr<ReturnType> Get(Msg message, const Id& id) {
    upb_MessageValue value;
    const bool ok = internal::GetOrPromoteExtension(
        interop::upb::GetMessage(message), id.mini_table_ext(),
        interop::upb::GetArena(message), &value);
    if (!ok) {
      return ExtensionNotFoundError(
          upb_MiniTableExtension_Number(id.mini_table_ext()));
    }
    return Ptr<const T>(interop::upb::MakeCHandle<T>(
        value.msg_val, hpb::interop::upb::GetArena(message)));
  }

  template <typename Msg, typename Id>
  static absl::Status Set(Msg message, const Id& id, const T& value) {
    return Set(message, id, &value);
  }

  template <typename Msg, typename Id>
  static absl::Status Set(Msg message, const Id& id, T&& value) {
    T local = std::move(value);
    return internal::MoveExtension(
        interop::upb::GetMessage(message), interop::upb::GetArena(message),
        id.mini_table_ext(), interop::upb::GetMessage(&local),
        interop::upb::GetArena(&local));
  }

  template <typename Msg, typename Id>
  static absl::Status Set(Msg message, const Id& id, Ptr<const T> value) {
    return hpb::internal::SetExtension(
        interop::upb::GetMessage(message), interop::upb::GetArena(message),
        id.mini_table_ext(), interop::upb::GetMessage(value));
  }
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
      typename UpbExtensionTrait<ExtensionType>::DefaultType val,
      uint32_t number)
      : mini_table_ext_(mte), default_val_(val), number_(number) {}

  constexpr uint32_t number() const { return number_; }

  const upb_MiniTableExtension* mini_table_ext_;

  typename UpbExtensionTrait<ExtensionType>::ReturnType default_value() const {
    if constexpr (IsHpbClass<ExtensionType>) {
      return ExtensionType::default_instance();
    } else {
      return default_val_;
    }
  }

  typename UpbExtensionTrait<ExtensionType>::DefaultType default_val_;

  uint32_t number_;

  friend struct PrivateAccess;
};

upb_ExtensionRegistry* GetUpbExtensions(
    const ExtensionRegistry& extension_registry);
}  // namespace internal
}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_BACKEND_UPB_EXTENSION_H__
