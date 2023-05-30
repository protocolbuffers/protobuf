// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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
//     * Neither the name of Google Inc. nor the names of its
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

#ifndef GOOGLE_PROTOBUF_MAP_FIELD_INL_H__
#define GOOGLE_PROTOBUF_MAP_FIELD_INL_H__

#include <memory>
#include <string>
#include <tuple>
#include <type_traits>

#include "absl/base/casts.h"
#include "google/protobuf/map.h"
#include "google/protobuf/map_field.h"
#include "google/protobuf/map_type_handler.h"
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
void TypeDefinedMapFieldBase<Key, T>::SetMapIteratorValue(
    MapIterator* map_iter) const {
  if (map_iter->iter_.Equals(UntypedMapBase::EndIterator())) return;
  auto iter = typename Map<Key, T>::const_iterator(map_iter->iter_);
  SetMapKey(&map_iter->key_, iter->first);
  map_iter->value_.SetValueOrCopy(&iter->second);
}

template <typename Key, typename T>
bool TypeDefinedMapFieldBase<Key, T>::ContainsMapKey(
    const MapKey& map_key) const {
  return GetMap().contains(UnwrapMapKey<Key>(map_key));
}

template <typename Key, typename T>
bool TypeDefinedMapFieldBase<Key, T>::InsertOrLookupMapValueNoSync(
    const MapKey& map_key, MapValueRef* val) {
  auto res = map_.try_emplace(UnwrapMapKey<Key>(map_key));
  val->SetValue(&res.first->second);
  return res.second;
}

template <typename Key, typename T>
bool TypeDefinedMapFieldBase<Key, T>::LookupMapValue(
    const MapKey& map_key, MapValueConstRef* val) const {
  const auto& map = GetMap();
  auto iter = map.find(UnwrapMapKey<Key>(map_key));
  if (map.end() == iter) {
    return false;
  }
  val->SetValueOrCopy(&iter->second);
  return true;
}

template <typename Key, typename T>
bool TypeDefinedMapFieldBase<Key, T>::DeleteMapValue(const MapKey& map_key) {
  return MutableMap()->erase(UnwrapMapKey<Key>(map_key));
}

template <typename Key, typename T>
void TypeDefinedMapFieldBase<Key, T>::Swap(MapFieldBase* other) {
  MapFieldBase::Swap(other);
  auto* other_field = DownCast<TypeDefinedMapFieldBase*>(other);
  map_.Swap(&other_field->map_);
}

template <typename Key, typename T>
void TypeDefinedMapFieldBase<Key, T>::MergeFrom(const MapFieldBase& other) {
  SyncMapWithRepeatedField();
  const auto& other_field = static_cast<const TypeDefinedMapFieldBase&>(other);
  other_field.SyncMapWithRepeatedField();
  internal::MapMergeFrom(map_, other_field.map_);
  SetMapDirty();
}

template <typename Key, typename T>
size_t TypeDefinedMapFieldBase<Key, T>::SpaceUsedExcludingSelfNoLock() const {
  size_t size = 0;
  if (auto* p = maybe_payload()) {
    size += p->repeated_field.SpaceUsedExcludingSelfLong();
  }
  // We can't compile this expression for DynamicMapField even though it is
  // never used at runtime, so disable it at compile time.
  std::get<std::is_same<Map<Key, T>, Map<MapKey, MapValueRef>>::value>(
      std::make_tuple(
          [&](const auto& map) { size += map.SpaceUsedExcludingSelfLong(); },
          [](const auto&) {}))(map_);

  return size;
}

template <typename Key, typename T>
void TypeDefinedMapFieldBase<Key, T>::UnsafeShallowSwap(MapFieldBase* other) {
  InternalSwap(DownCast<TypeDefinedMapFieldBase*>(other));
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
const Message* MapField<Derived, Key, T, kKeyFieldType,
                        kValueFieldType>::GetPrototype() const {
  return Derived::internal_default_instance();
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_MAP_FIELD_INL_H__
