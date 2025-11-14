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
#include "hpb/arena.h"
#include "hpb/extension.h"
#include "hpb/internal/template_help.h"
#include "hpb/multibackend.h"
#include "hpb/options.h"
#include "hpb/ptr.h"
#include "hpb/status.h"

#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
#include "hpb/backend/upb/upb.h"
#elif HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_CPP
#include "hpb/backend/cpp/cpp.h"
#else
#error hpb backend unknown
#endif

namespace hpb {

template <typename T>
typename T::Proxy CreateMessage(Arena& arena) {
  return backend::CreateMessage<T>(arena);
}

template <typename T>
typename T::Proxy CloneMessage(Ptr<T> message, Arena& arena) {
  return backend::CloneMessage<T>(message, arena);
}

template <typename T>
void DeepCopy(Ptr<const T> source_message, Ptr<T> target_message) {
  static_assert(!std::is_const_v<T>);
  backend::DeepCopy(source_message, target_message);
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

// Note that the default extension registry is the the generated registry.
template <typename T>
hpb::StatusOr<T> Parse(absl::string_view bytes, ParseOptions options) {
  return backend::Parse<T>(bytes, options);
}

#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
template <typename T>
ABSL_MUST_USE_RESULT bool Parse(internal::PtrOrRaw<T> message,
                                absl::string_view bytes,
                                const ExtensionRegistry& extension_registry =
                                    ExtensionRegistry::generated_registry()) {
  return backend::Parse(message, bytes, extension_registry);
}

// Deprecated. Use the overload that returns hpb::StatusOr<T> instead.
// Note that the default extension registry is the empty registry.
template <typename T>
ABSL_DEPRECATED("Prefer the overload that returns hpb::StatusOr<T>")
absl::StatusOr<T> Parse(absl::string_view bytes,
                        const ExtensionRegistry& extension_registry =
                            ExtensionRegistry::empty_registry()) {
  return backend::Parse<T>(bytes, extension_registry);
}
#endif

template <typename T>
absl::StatusOr<absl::string_view> Serialize(internal::PtrOrRaw<T> message,
                                            Arena& arena) {
  return backend::Serialize(message, arena);
}

}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_HPB_H__
