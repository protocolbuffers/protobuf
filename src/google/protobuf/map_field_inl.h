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
#ifndef _SHARED_PTR_H
#include <google/protobuf/stubs/shared_ptr.h>
#endif

#include <google/protobuf/map_field.h>
#include <google/protobuf/map_type_handler.h>

namespace google {
namespace protobuf {
namespace internal {

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
MapField<Key, T, KeyProto, ValueProto, default_enum_value>::MapField()
    : default_entry_(NULL) {
  MapFieldBase::base_map_ = new Map<Key, T>;
  SetDefaultEnumValue();
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
MapField<Key, T, KeyProto, ValueProto, default_enum_value>::MapField(
    const Message* default_entry)
    : default_entry_(down_cast<const EntryType*>(default_entry)) {
  MapFieldBase::base_map_ = new Map<Key, T>;
  SetDefaultEnumValue();
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
MapField<Key, T, KeyProto, ValueProto, default_enum_value>::~MapField() {
  delete reinterpret_cast<Map<Key, T>*>(MapFieldBase::base_map_);
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
int MapField<Key, T, KeyProto, ValueProto, default_enum_value>::size() const {
  SyncMapWithRepeatedField();
  return GetInternalMap().size();
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
void MapField<Key, T, KeyProto, ValueProto, default_enum_value>::Clear() {
  SyncMapWithRepeatedField();
  MutableInternalMap()->clear();
  SetMapDirty();
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
const Map<Key, T>&
MapField<Key, T, KeyProto, ValueProto, default_enum_value>::GetMap() const {
  SyncMapWithRepeatedField();
  return GetInternalMap();
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
Map<Key, T>*
MapField<Key, T, KeyProto, ValueProto, default_enum_value>::MutableMap() {
  SyncMapWithRepeatedField();
  Map<Key, T>* result = MutableInternalMap();
  SetMapDirty();
  return result;
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
void MapField<Key, T, KeyProto, ValueProto, default_enum_value>::MergeFrom(
    const MapField& other) {
  SyncMapWithRepeatedField();
  other.SyncMapWithRepeatedField();

  Map<Key, T>* map = MutableInternalMap();
  const Map<Key, T>& other_map = other.GetInternalMap();
  for (typename Map<Key, T>::const_iterator it = other_map.begin();
       it != other_map.end(); ++it) {
    (*map)[it->first] = it->second;
  }
  SetMapDirty();
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
void MapField<Key, T, KeyProto, ValueProto, default_enum_value>::Swap(
    MapField* other) {
  std::swap(repeated_field_, other->repeated_field_);
  std::swap(base_map_, other->base_map_);
  std::swap(state_, other->state_);
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
void
MapField<Key, T, KeyProto, ValueProto, default_enum_value>::SetEntryDescriptor(
    const Descriptor** descriptor) {
  entry_descriptor_ = descriptor;
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
void
MapField<Key, T, KeyProto, ValueProto,
         default_enum_value>::SetAssignDescriptorCallback(void (*callback)()) {
  assign_descriptor_callback_ = callback;
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
void MapField<Key, T, KeyProto, ValueProto,
              default_enum_value>::SetDefaultEnumValue() {
  MutableInternalMap()->SetDefaultEnumValue(default_enum_value);
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
MapEntry<Key, T, KeyProto, ValueProto, default_enum_value>*
MapField<Key, T, KeyProto, ValueProto, default_enum_value>::NewEntry() const {
  // The MapEntry instance created here is only used in generated code for
  // parsing. It doesn't have default instance, descriptor or reflection,
  // because these are not needed in parsing and will prevent us from using it
  // for parsing MessageLite.
  return new EntryType();
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
MapEntry<Key, T, KeyProto, ValueProto, default_enum_value>*
MapField<Key, T, KeyProto, ValueProto, default_enum_value>::NewEntryWrapper(
    const Key& key, const T& t) const {
  return EntryType::Wrap(key, t);
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
MapEntry<Key, T, KeyProto, ValueProto, default_enum_value>*
MapField<Key, T, KeyProto, ValueProto, default_enum_value>::NewEnumEntryWrapper(
    const Key& key, const T t) const {
  return EntryType::EnumWrap(key, t);
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
const Map<Key, T>& MapField<Key, T, KeyProto, ValueProto,
                            default_enum_value>::GetInternalMap() const {
  return *reinterpret_cast<Map<Key, T>*>(MapFieldBase::base_map_);
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
Map<Key, T>* MapField<Key, T, KeyProto, ValueProto,
                      default_enum_value>::MutableInternalMap() {
  return reinterpret_cast<Map<Key, T>*>(MapFieldBase::base_map_);
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
void MapField<Key, T, KeyProto, ValueProto,
              default_enum_value>::SyncRepeatedFieldWithMapNoLock() const {
  if (repeated_field_ == NULL) {
    repeated_field_ = new RepeatedPtrField<Message>();
  }
  const Map<Key, T>& map =
      *static_cast<const Map<Key, T>*>(MapFieldBase::base_map_);
  RepeatedPtrField<EntryType>* repeated_field =
      reinterpret_cast<RepeatedPtrField<EntryType>*>(repeated_field_);

  repeated_field->Clear();

  for (typename Map<Key, T>::const_iterator it = map.begin();
       it != map.end(); ++it) {
    InitDefaultEntryOnce();
    GOOGLE_CHECK(default_entry_ != NULL);
    EntryType* new_entry = down_cast<EntryType*>(default_entry_->New());
    repeated_field->AddAllocated(new_entry);
    new_entry->set_key(it->first);
    new_entry->set_value(it->second);
  }
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
void MapField<Key, T, KeyProto, ValueProto,
              default_enum_value>::SyncMapWithRepeatedFieldNoLock() const {
  Map<Key, T>* map = reinterpret_cast<Map<Key, T>*>(MapFieldBase::base_map_);
  RepeatedPtrField<EntryType>* repeated_field =
      reinterpret_cast<RepeatedPtrField<EntryType>*>(repeated_field_);
  map->clear();
  for (typename RepeatedPtrField<EntryType>::iterator it =
           repeated_field->begin(); it != repeated_field->end(); ++it) {
    // Cast is needed because Map's api and internal storage is different when
    // value is enum. For enum, we cannot cast an int to enum. Thus, we have to
    // copy value. For other types, they have same exposed api type and internal
    // stored type. We should not introduce value copy for them. We achieve this
    // by casting to value for enum while casting to reference for other types.
    (*map)[it->key()] = static_cast<CastValueType>(it->value());
  }
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
int MapField<Key, T, KeyProto, ValueProto,
             default_enum_value>::SpaceUsedExcludingSelfNoLock() const {
  int size = 0;
  if (repeated_field_ != NULL) {
    size += repeated_field_->SpaceUsedExcludingSelf();
  }
  Map<Key, T>* map = reinterpret_cast<Map<Key, T>*>(MapFieldBase::base_map_);
  size += sizeof(*map);
  for (typename Map<Key, T>::iterator it = map->begin();
       it != map->end(); ++it) {
    size += KeyHandler::SpaceUsedInMap(it->first);
    size += ValHandler::SpaceUsedInMap(it->second);
  }
  return size;
}

template <typename Key, typename T, FieldDescriptor::Type KeyProto,
          FieldDescriptor::Type ValueProto, int default_enum_value>
void MapField<Key, T, KeyProto, ValueProto,
              default_enum_value>::InitDefaultEntryOnce() const {
  if (default_entry_ == NULL) {
    InitMetadataOnce();
    GOOGLE_CHECK(*entry_descriptor_ != NULL);
    default_entry_ = down_cast<const EntryType*>(
        MessageFactory::generated_factory()->GetPrototype(*entry_descriptor_));
  }
}

template <typename Key, typename T>
bool AllAreInitialized(const Map<Key, T>& t) {
  for (typename Map<Key, T>::const_iterator it = t.begin(); it != t.end();
       ++it) {
    if (!it->second.IsInitialized()) return false;
  }
  return true;
}

}  // namespace internal
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_MAP_FIELD_INL_H__
