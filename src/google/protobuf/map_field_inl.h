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
// UnwrapMapKey template. We're using overloading rather than template
// specialization so that we can return a value or reference type depending on
// `T`.
inline int32_t UnwrapMapKeyImpl(const MapKey& map_key, const int32_t*) {
  return map_key.GetInt32Value();
}
inline uint32_t UnwrapMapKeyImpl(const MapKey& map_key, const uint32_t*) {
  return map_key.GetUInt32Value();
}
inline int64_t UnwrapMapKeyImpl(const MapKey& map_key, const int64_t*) {
  return map_key.GetInt64Value();
}
inline uint64_t UnwrapMapKeyImpl(const MapKey& map_key, const uint64_t*) {
  return map_key.GetUInt64Value();
}
inline bool UnwrapMapKeyImpl(const MapKey& map_key, const bool*) {
  return map_key.GetBoolValue();
}
inline absl::string_view UnwrapMapKeyImpl(const MapKey& map_key,
                                          const std::string*) {
  return map_key.GetStringValue();
}

template <typename T>
decltype(auto) UnwrapMapKey(const MapKey& map_key) {
  return UnwrapMapKeyImpl(map_key, static_cast<T*>(nullptr));
}

// ------------------------TypeDefinedMapFieldBase---------------
template <typename Key, typename T>
bool TypeDefinedMapFieldBase<Key, T>::InsertOrLookupMapValueNoSyncImpl(
    MapFieldBase& map, const MapKey& map_key, MapValueRef* val) {
  auto res = static_cast<TypeDefinedMapFieldBase&>(map).map_.try_emplace(
      UnwrapMapKey<Key>(map_key));
  val->SetValue(&res.first->second);
  return res.second;
}

template <typename Key, typename T>
bool TypeDefinedMapFieldBase<Key, T>::DeleteMapValueImpl(
    MapFieldBase& map, const MapKey& map_key) {
  return static_cast<TypeDefinedMapFieldBase&>(map).MutableMap()->erase(
      UnwrapMapKey<Key>(map_key));
}

template <typename Key, typename T>
void TypeDefinedMapFieldBase<Key, T>::SwapImpl(MapFieldBase& lhs,
                                               MapFieldBase& rhs) {
  MapFieldBase::SwapPayload(lhs, rhs);
  static_cast<TypeDefinedMapFieldBase&>(lhs).map_.swap(
      static_cast<TypeDefinedMapFieldBase&>(rhs).map_);
}

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

// ----------------------------------------------------------------------

template <typename Derived, typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType>
const Message*
MapField<Derived, Key, T, kKeyFieldType, kValueFieldType>::GetPrototypeImpl(
    const MapFieldBase&) {
  return Derived::internal_default_instance();
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_MAP_FIELD_INL_H__
