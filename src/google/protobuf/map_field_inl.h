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

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
MapField<Key, T, kKeyFieldType, kValueFieldType, default_enum_value>::MapField()
    : default_entry_(NULL) {}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
MapField<Key, T, kKeyFieldType, kValueFieldType, default_enum_value>::MapField(
    Arena* arena)
    : MapFieldBase(arena),
      MapFieldLite<Key, T, kKeyFieldType, kValueFieldType, default_enum_value>(
          arena),
      default_entry_(NULL) {}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
MapField<Key, T, kKeyFieldType, kValueFieldType, default_enum_value>::MapField(
    const Message* default_entry)
    : default_entry_(down_cast<const EntryType*>(default_entry)) {}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
MapField<Key, T, kKeyFieldType, kValueFieldType, default_enum_value>::MapField(
    Arena* arena, const Message* default_entry)
    : MapFieldBase(arena),
      MapFieldLite<Key, T, kKeyFieldType, kValueFieldType, default_enum_value>(
          arena),
      default_entry_(down_cast<const EntryType*>(default_entry)) {}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
MapField<Key, T, kKeyFieldType, kValueFieldType,
         default_enum_value>::~MapField() {}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
int
MapField<Key, T, kKeyFieldType, kValueFieldType,
         default_enum_value>::size() const {
  SyncMapWithRepeatedField();
  return MapFieldLiteType::GetInternalMap().size();
}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
void
MapField<Key, T, kKeyFieldType, kValueFieldType,
         default_enum_value>::Clear() {
  SyncMapWithRepeatedField();
  MapFieldLiteType::MutableInternalMap()->clear();
  SetMapDirty();
}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
const Map<Key, T>&
MapField<Key, T, kKeyFieldType, kValueFieldType,
         default_enum_value>::GetMap() const {
  SyncMapWithRepeatedField();
  return MapFieldLiteType::GetInternalMap();
}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
Map<Key, T>*
MapField<Key, T, kKeyFieldType, kValueFieldType,
         default_enum_value>::MutableMap() {
  SyncMapWithRepeatedField();
  Map<Key, T>* result = MapFieldLiteType::MutableInternalMap();
  SetMapDirty();
  return result;
}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
void
MapField<Key, T, kKeyFieldType, kValueFieldType,
         default_enum_value>::MergeFrom(
    const MapFieldLiteType& other) {
  const MapField& down_other = down_cast<const MapField&>(other);
  SyncMapWithRepeatedField();
  down_other.SyncMapWithRepeatedField();
  MapFieldLiteType::MergeFrom(other);
  SetMapDirty();
}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
void
MapField<Key, T, kKeyFieldType, kValueFieldType,
         default_enum_value>::Swap(
    MapFieldLiteType* other) {
  MapField* down_other = down_cast<MapField*>(other);
  std::swap(repeated_field_, down_other->repeated_field_);
  MapFieldLiteType::Swap(other);
  std::swap(state_, down_other->state_);
}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
void
MapField<Key, T, kKeyFieldType, kValueFieldType,
         default_enum_value>::SetEntryDescriptor(
    const Descriptor** descriptor) {
  entry_descriptor_ = descriptor;
}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
void
MapField<Key, T, kKeyFieldType, kValueFieldType,
         default_enum_value>::SetAssignDescriptorCallback(void (*callback)()) {
  assign_descriptor_callback_ = callback;
}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
const Map<Key, T>&
MapField<Key, T, kKeyFieldType, kValueFieldType,
         default_enum_value>::GetInternalMap() const {
  return MapFieldLiteType::GetInternalMap();
}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
Map<Key, T>*
MapField<Key, T, kKeyFieldType, kValueFieldType,
         default_enum_value>::MutableInternalMap() {
  return MapFieldLiteType::MutableInternalMap();
}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
void
MapField<Key, T, kKeyFieldType, kValueFieldType,
         default_enum_value>::SyncRepeatedFieldWithMapNoLock() const {
  if (repeated_field_ == NULL) {
    if (MapFieldBase::arena_ == NULL) {
      repeated_field_ = new RepeatedPtrField<Message>();
    } else {
      repeated_field_ = Arena::CreateMessage<RepeatedPtrField<Message> >(
          MapFieldBase::arena_);
    }
  }
  const Map<Key, T>& map = GetInternalMap();
  RepeatedPtrField<EntryType>* repeated_field =
      reinterpret_cast<RepeatedPtrField<EntryType>*>(repeated_field_);

  repeated_field->Clear();

  for (typename Map<Key, T>::const_iterator it = map.begin();
       it != map.end(); ++it) {
    InitDefaultEntryOnce();
    GOOGLE_CHECK(default_entry_ != NULL);
    EntryType* new_entry =
        down_cast<EntryType*>(default_entry_->New(MapFieldBase::arena_));
    repeated_field->AddAllocated(new_entry);
    (*new_entry->mutable_key()) = it->first;
    (*new_entry->mutable_value()) = it->second;
  }
}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
void
MapField<Key, T, kKeyFieldType, kValueFieldType,
         default_enum_value>::SyncMapWithRepeatedFieldNoLock() const {
  Map<Key, T>* map = const_cast<MapField*>(this)->MutableInternalMap();
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

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
int
MapField<Key, T, kKeyFieldType, kValueFieldType,
         default_enum_value>::SpaceUsedExcludingSelfNoLock() const {
  int size = 0;
  if (repeated_field_ != NULL) {
    size += repeated_field_->SpaceUsedExcludingSelf();
  }
  Map<Key, T>* map = const_cast<MapField*>(this)->MutableInternalMap();
  size += sizeof(*map);
  for (typename Map<Key, T>::iterator it = map->begin();
       it != map->end(); ++it) {
    size += KeyHandler::SpaceUsedInMap(it->first);
    size += ValHandler::SpaceUsedInMap(it->second);
  }
  return size;
}

template <typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType,
          int default_enum_value>
void
MapField<Key, T, kKeyFieldType, kValueFieldType,
         default_enum_value>::InitDefaultEntryOnce()
    const {
  if (default_entry_ == NULL) {
    InitMetadataOnce();
    GOOGLE_CHECK(*entry_descriptor_ != NULL);
    default_entry_ = down_cast<const EntryType*>(
        MessageFactory::generated_factory()->GetPrototype(*entry_descriptor_));
  }
}

}  // namespace internal
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_MAP_FIELD_INL_H__
