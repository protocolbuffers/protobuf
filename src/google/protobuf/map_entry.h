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

#ifndef GOOGLE_PROTOBUF_MAP_ENTRY_H__
#define GOOGLE_PROTOBUF_MAP_ENTRY_H__

#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/map_entry_lite.h>
#include <google/protobuf/map_type_handler.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/wire_format_lite_inl.h>

namespace google {
namespace protobuf {
class Arena;
namespace internal {
template <typename Derived, typename Key, typename Value,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType, int default_enum_value>
class MapField;
}
}

namespace protobuf {
namespace internal {

// MapEntry is the returned google::protobuf::Message when calling AddMessage of
// google::protobuf::Reflection. In order to let it work with generated message
// reflection, its in-memory type is the same as generated message with the same
// fields. However, in order to decide the in-memory type of key/value, we need
// to know both their cpp type in generated api and proto type. In
// implementation, all in-memory types have related wire format functions to
// support except ArenaStringPtr. Therefore, we need to define another type with
// supporting wire format functions. Since this type is only used as return type
// of MapEntry accessors, it's named MapEntry accessor type.
//
// cpp type:               the type visible to users in public API.
// proto type:             WireFormatLite::FieldType of the field.
// in-memory type:         type of the data member used to stored this field.
// MapEntry accessor type: type used in MapEntry getters/mutators to access the
//                         field.
//
// cpp type | proto type  | in-memory type | MapEntry accessor type
// int32      TYPE_INT32    int32            int32
// int32      TYPE_FIXED32  int32            int32
// string     TYPE_STRING   ArenaStringPtr   string
// FooEnum    TYPE_ENUM     int              int
// FooMessage TYPE_MESSAGE  FooMessage*      FooMessage
//
// The in-memory types of primitive types can be inferred from its proto type,
// while we need to explicitly specify the cpp type if proto type is
// TYPE_MESSAGE to infer the in-memory type.  Moreover, default_enum_value is
// used to initialize enum field in proto2.
template <typename Derived, typename Key, typename Value,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType, int default_enum_value>
class MapEntry
    : public MapEntryImpl<Derived, Message, Key, Value, kKeyFieldType,
                          kValueFieldType, default_enum_value> {
 public:
  // Create default MapEntry instance for given descriptor. Descriptor has to be
  // given when creating default MapEntry instance because different map field
  // may have the same type and MapEntry class. The given descriptor is needed
  // to distinguish instances of the same MapEntry class.
  static const Reflection* CreateReflection(const Descriptor* descriptor,
                                            const Derived* entry) {
    ReflectionSchema schema = {
        entry,
        offsets_,
        has_bits_,
        GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(MapEntry, _has_bits_),
        GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(MapEntry,
                                                       _internal_metadata_),
        -1,
        -1,
        sizeof(MapEntry)};
    const Reflection* reflection = new GeneratedMessageReflection(
        descriptor, schema, DescriptorPool::generated_pool(),
        MessageFactory::generated_factory());
    return reflection;
  }

  MapEntry() : _internal_metadata_(NULL) {}
  explicit MapEntry(Arena* arena)
      : MapEntryImpl<Derived, Message, Key, Value, kKeyFieldType,
                     kValueFieldType, default_enum_value>(arena),
        _internal_metadata_(arena) {}
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;

 private:
  static uint32 offsets_[2];
  static uint32 has_bits_[2];
  InternalMetadataWithArena _internal_metadata_;

  friend class ::google::protobuf::Arena;
  template <typename C, typename K, typename V,
            WireFormatLite::FieldType k_wire_type, WireFormatLite::FieldType,
            int default_enum>
  friend class internal::MapField;
  friend class internal::GeneratedMessageReflection;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(MapEntry);
};

template <typename Derived, typename Key, typename Value,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType, int default_enum_value>
uint32 MapEntry<Derived, Key, Value, kKeyFieldType, kValueFieldType,
                default_enum_value>::offsets_[2] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(MapEntry, key_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(MapEntry, value_),
};

template <typename Derived, typename Key, typename Value,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType, int default_enum_value>
uint32 MapEntry<Derived, Key, Value, kKeyFieldType, kValueFieldType,
                default_enum_value>::has_bits_[2] = {0, 1};

}  // namespace internal
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_MAP_ENTRY_H__
