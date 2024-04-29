// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_MAP_FIELD_INL_H__
#define GOOGLE_PROTOBUF_MAP_FIELD_INL_H__

#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>

#include "absl/base/casts.h"
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
// UnwrapMapKey template
template <typename T>
T UnwrapMapKey(const MapKey& map_key);
template <>
inline int32_t UnwrapMapKey(const MapKey& map_key) {
  return map_key.GetInt32Value();
}
template <>
inline uint32_t UnwrapMapKey(const MapKey& map_key) {
  return map_key.GetUInt32Value();
}
template <>
inline int64_t UnwrapMapKey(const MapKey& map_key) {
  return map_key.GetInt64Value();
}
template <>
inline uint64_t UnwrapMapKey(const MapKey& map_key) {
  return map_key.GetUInt64Value();
}
template <>
inline bool UnwrapMapKey(const MapKey& map_key) {
  return map_key.GetBoolValue();
}
template <>
inline std::string UnwrapMapKey(const MapKey& map_key) {
  return map_key.GetStringValue();
}
template <>
inline MapKey UnwrapMapKey(const MapKey& map_key) {
  return map_key;
}

// SetMapKey
inline void SetMapKey(MapKey* map_key, int32_t value) {
  map_key->SetInt32Value(value);
}
inline void SetMapKey(MapKey* map_key, uint32_t value) {
  map_key->SetUInt32Value(value);
}
inline void SetMapKey(MapKey* map_key, int64_t value) {
  map_key->SetInt64Value(value);
}
inline void SetMapKey(MapKey* map_key, uint64_t value) {
  map_key->SetUInt64Value(value);
}
inline void SetMapKey(MapKey* map_key, bool value) {
  map_key->SetBoolValue(value);
}
inline void SetMapKey(MapKey* map_key, const std::string& value) {
  map_key->SetStringValue(value);
}
inline void SetMapKey(MapKey* map_key, const MapKey& value) {
  map_key->CopyFrom(value);
}

// ------------------------TypeDefinedMapFieldBase---------------
template <typename Key, typename T>
void TypeDefinedMapFieldBase<Key, T>::SetMapIteratorValueImpl(
    MapIterator* map_iter) {
  if (map_iter->iter_.Equals(UntypedMapBase::EndIterator())) return;
  auto iter = typename Map<Key, T>::const_iterator(map_iter->iter_);
  SetMapKey(&map_iter->key_, iter->first);
  map_iter->value_.SetValueOrCopy(&iter->second);
}

template <typename Key, typename T>
bool TypeDefinedMapFieldBase<Key, T>::InsertOrLookupMapValueNoSyncImpl(
    MapFieldBase& map, const MapKey& map_key, MapValueRef* val) {
  auto res = static_cast<TypeDefinedMapFieldBase&>(map).map_.try_emplace(
      UnwrapMapKey<Key>(map_key));
  val->SetValue(&res.first->second);
  return res.second;
}

template <typename Key, typename T>
bool TypeDefinedMapFieldBase<Key, T>::LookupMapValueImpl(
    const MapFieldBase& self, const MapKey& map_key, MapValueConstRef* val) {
  const auto& map = static_cast<const TypeDefinedMapFieldBase&>(self).GetMap();
  auto iter = map.find(UnwrapMapKey<Key>(map_key));
  if (map.end() == iter) {
    return false;
  }
  if (val != nullptr) {
    val->SetValueOrCopy(&iter->second);
  }
  return true;
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
  MapFieldBase::SwapImpl(lhs, rhs);
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

template <typename Key, typename T>
size_t TypeDefinedMapFieldBase<Key, T>::SpaceUsedExcludingSelfNoLockImpl(
    const MapFieldBase& map) {
  auto& self = static_cast<const TypeDefinedMapFieldBase&>(map);
  size_t size = 0;
  if (auto* p = self.maybe_payload()) {
    size += p->repeated_field.SpaceUsedExcludingSelfLong();
  }
  // We can't compile this expression for DynamicMapField even though it is
  // never used at runtime, so disable it at compile time.
  std::get<std::is_same<Map<Key, T>, Map<MapKey, MapValueRef>>::value>(
      std::make_tuple(
          [&](const auto& map) { size += map.SpaceUsedExcludingSelfLong(); },
          [](const auto&) {}))(self.map_);
  return size;
}

template <typename Key, typename T>
void TypeDefinedMapFieldBase<Key, T>::UnsafeShallowSwapImpl(MapFieldBase& lhs,
                                                            MapFieldBase& rhs) {
  static_cast<TypeDefinedMapFieldBase&>(lhs).InternalSwap(
      static_cast<TypeDefinedMapFieldBase*>(&rhs));
}

template <typename Key, typename T>
void TypeDefinedMapFieldBase<Key, T>::InternalSwap(
    TypeDefinedMapFieldBase* other) {
  MapFieldBase::InternalSwap(other);
  map_.InternalSwap(&other->map_);
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
