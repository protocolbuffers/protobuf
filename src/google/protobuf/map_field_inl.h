// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_MAP_FIELD_INL_H__
#define GOOGLE_PROTOBUF_MAP_FIELD_INL_H__

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>

#include "absl/base/casts.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/map.h"
#include "google/protobuf/map_field.h"
#include "google/protobuf/map_type_handler.h"
#include "google/protobuf/message.h"
#include "google/protobuf/port.h"

// must be last
#include "google/protobuf/port_def.inc"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {
namespace internal {
// ------------------------TypeDefinedMapFieldBase---------------
template <typename Key, typename T>
void TypeDefinedMapFieldBase<Key, T>::MergeFromImpl(MapFieldBase& base,
                                                    const MapFieldBase& other) {
  auto& self = static_cast<TypeDefinedMapFieldBase&>(base);
  self.SyncMapWithRepeatedField();
  const auto& other_field = static_cast<const TypeDefinedMapFieldBase&>(other);
  other_field.SyncMapWithRepeatedField();
  internal::MapMergeFrom(self.map_, other_field.map_);
  self.SetMapDirty();
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_MAP_FIELD_INL_H__
