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

#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/map_type_handler.h>
#include <google/protobuf/wire_format_lite_inl.h>

namespace google {
namespace protobuf {
class Arena;
}

namespace protobuf {
namespace internal {

// Register all MapEntry default instances so we can delete them in
// ShutdownProtobufLibrary().
void LIBPROTOBUF_EXPORT RegisterMapEntryDefaultInstance(
    MessageLite* default_instance);

// This is the common base class for MapEntry. It is used by MapFieldBase in
// reflection api, in which the static type of key and value is unknown.
class LIBPROTOBUF_EXPORT MapEntryBase : public Message {
 public:
  ::google::protobuf::Metadata GetMetadata() const {
    ::google::protobuf::Metadata metadata;
    metadata.descriptor = descriptor_;
    metadata.reflection = reflection_;
    return metadata;
  }

 protected:
  MapEntryBase() : descriptor_(NULL), reflection_(NULL) {  }
  virtual ~MapEntryBase() {}

  const Descriptor* descriptor_;
  const Reflection* reflection_;
};

// MapEntry is the returned google::protobuf::Message when calling AddMessage of
// google::protobuf::Reflection. In order to let it work with generated message
// reflection, its internal layout is the same as generated message with the
// same fields. However, in order to decide the internal layout of key/value, we
// need to know both their cpp type in generated api and proto type.
//
// cpp type | proto type  | internal layout
// int32      TYPE_INT32    int32
// int32      TYPE_FIXED32  int32
// FooEnum    TYPE_ENUM     int
// FooMessage TYPE_MESSAGE  FooMessage*
//
// The internal layouts of primitive types can be inferred from its proto type,
// while we need to explicitly tell cpp type if proto type is TYPE_MESSAGE to
// get internal layout.
// Moreover, default_enum_value is used to initialize enum field in proto2.
template <typename Key, typename Value, FieldDescriptor::Type KeyProtoType,
          FieldDescriptor::Type ValueProtoType, int default_enum_value>
class MapEntry : public MapEntryBase {
  // Handlers for key/value's proto field type. Used to infer internal layout
  // and provide parsing/serialization support.
  typedef MapProtoTypeHandler<KeyProtoType> KeyProtoHandler;
  typedef MapProtoTypeHandler<ValueProtoType> ValueProtoHandler;

  // Define key/value's internal stored type. Message is the only one whose
  // internal stored type cannot be inferred from its proto type.
  typedef typename KeyProtoHandler::CppType KeyProtoHandlerCppType;
  typedef typename ValueProtoHandler::CppType ValueProtoHandlerCppType;
  static const bool kIsKeyMessage = KeyProtoHandler::kIsMessage;
  static const bool kIsValueMessage = ValueProtoHandler::kIsMessage;
  typedef typename MapIf<kIsKeyMessage, Key, KeyProtoHandlerCppType>::type
      KeyCppType;
  typedef typename MapIf<kIsValueMessage, Value, ValueProtoHandlerCppType>::type
      ValCppType;

  // Handlers for key/value's internal stored type. Provide utilities to
  // manipulate internal stored type. We need it because some types are stored
  // as values and others are stored as pointers (Message and string), but we
  // need to keep the code in MapEntry unified instead of providing different
  // codes for each type.
  typedef MapCppTypeHandler<KeyCppType> KeyCppHandler;
  typedef MapCppTypeHandler<ValCppType> ValueCppHandler;

  // Define internal memory layout. Strings and messages are stored as
  // pointers, while other types are stored as values.
  static const bool kKeyIsStringOrMessage = KeyCppHandler::kIsStringOrMessage;
  static const bool kValIsStringOrMessage = ValueCppHandler::kIsStringOrMessage;
  typedef typename MapIf<kKeyIsStringOrMessage, KeyCppType*, KeyCppType>::type
      KeyBase;
  typedef typename MapIf<kValIsStringOrMessage, ValCppType*, ValCppType>::type
      ValueBase;

  // Abbreviation for MapEntry
  typedef typename google::protobuf::internal::MapEntry<
      Key, Value, KeyProtoType, ValueProtoType, default_enum_value> EntryType;

  // Constants for field number.
  static const int kKeyFieldNumber = 1;
  static const int kValueFieldNumber = 2;

  // Constants for field tag.
  static const uint8 kKeyTag   = GOOGLE_PROTOBUF_WIRE_FORMAT_MAKE_TAG(
      kKeyFieldNumber,   KeyProtoHandler::kWireType);
  static const uint8 kValueTag = GOOGLE_PROTOBUF_WIRE_FORMAT_MAKE_TAG(
      kValueFieldNumber, ValueProtoHandler::kWireType);
  static const int kTagSize = 1;

 public:
  ~MapEntry() {
    if (this == default_instance_) {
      delete reflection_;
    } else {
      KeyCppHandler::Delete(key_);
      ValueCppHandler::Delete(value_);
    }
  }

  // accessors ======================================================

  inline void set_key(const KeyCppType& key) {
    KeyCppHandler::EnsureMutable(&key_);
    KeyCppHandler::Merge(key, &key_);
    set_has_key();
  }
  virtual inline const KeyCppType& key() const {
    return KeyCppHandler::Reference(key_);
  }
  inline KeyCppType* mutable_key() {
    set_has_key();
    KeyCppHandler::EnsureMutable(&key_);
    return KeyCppHandler::Pointer(key_);
  }
  inline void set_value(const ValCppType& value) {
    ValueCppHandler::EnsureMutable(&value_);
    ValueCppHandler::Merge(value, &value_);
    set_has_value();
  }
  virtual inline const ValCppType& value() const {
    GOOGLE_CHECK(default_instance_ != NULL);
    return ValueCppHandler::DefaultIfNotInitialized(value_,
                                               default_instance_->value_);
  }
  inline ValCppType* mutable_value() {
    set_has_value();
    ValueCppHandler::EnsureMutable(&value_);
    return ValueCppHandler::Pointer(value_);
  }

  // implements Message =============================================

  bool MergePartialFromCodedStream(::google::protobuf::io::CodedInputStream* input) {
    uint32 tag;

    for (;;) {
      // 1) corrupted data: return false;
      // 2) unknown field: skip without putting into unknown field set;
      // 3) unknown enum value: keep it in parsing. In proto2, caller should
      // check the value and put this entry into containing message's unknown
      // field set if the value is an unknown enum. In proto3, caller doesn't
      // need to care whether the value is unknown enum;
      // 4) missing key/value: missed key/value will have default value. caller
      // should take this entry as if key/value is set to default value.
      tag = input->ReadTag();
      switch (tag) {
        case kKeyTag:
          if (!KeyProtoHandler::Read(input, mutable_key())) return false;
          set_has_key();
          if (!input->ExpectTag(kValueTag)) break;
          GOOGLE_FALLTHROUGH_INTENDED;

        case kValueTag:
          if (!ValueProtoHandler::Read(input, mutable_value())) return false;
          set_has_value();
          if (input->ExpectAtEnd()) return true;
          break;

        default:
          if (tag == 0 ||
              WireFormatLite::GetTagWireType(tag) ==
              WireFormatLite::WIRETYPE_END_GROUP) {
            return true;
          }
          if (!WireFormatLite::SkipField(input, tag)) return false;
          break;
      }
    }
  }

  int ByteSize() const {
    int size = 0;
    size += has_key() ? kTagSize + KeyProtoHandler::ByteSize(key()) : 0;
    size += has_value() ? kTagSize + ValueProtoHandler::ByteSize(value()) : 0;
    return size;
  }

  void SerializeWithCachedSizes(::google::protobuf::io::CodedOutputStream* output) const {
    KeyProtoHandler::Write(kKeyFieldNumber, key(), output);
    ValueProtoHandler::Write(kValueFieldNumber, value(), output);
  }

  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const {
    output = KeyProtoHandler::WriteToArray(kKeyFieldNumber, key(), output);
    output =
        ValueProtoHandler::WriteToArray(kValueFieldNumber, value(), output);
    return output;
  }

  int GetCachedSize() const {
    int size = 0;
    size += has_key() ? kTagSize + KeyProtoHandler::GetCachedSize(key()) : 0;
    size +=
        has_value() ? kTagSize + ValueProtoHandler::GetCachedSize(value()) : 0;
    return size;
  }

  bool IsInitialized() const { return ValueCppHandler::IsInitialized(value_); }

  Message* New() const {
    MapEntry* entry = new MapEntry;
    entry->descriptor_ = descriptor_;
    entry->reflection_ = reflection_;
    entry->default_instance_ = default_instance_;
    return entry;
  }

  int SpaceUsed() const {
    int size = sizeof(MapEntry);
    size += KeyCppHandler::SpaceUsedInMapEntry(key_);
    size += ValueCppHandler::SpaceUsedInMapEntry(value_);
    return size;
  }

  void CopyFrom(const ::google::protobuf::Message& from) {
    Clear();
    MergeFrom(from);
  }

  void MergeFrom(const ::google::protobuf::Message& from) {
    GOOGLE_CHECK_NE(&from, this);
    const MapEntry* source = dynamic_cast_if_available<const MapEntry*>(&from);
    if (source == NULL) {
      ReflectionOps::Merge(from, this);
    } else {
      MergeFrom(*source);
    }
  }

  void CopyFrom(const MapEntry& from) {
    Clear();
    MergeFrom(from);
  }

  void MergeFrom(const MapEntry& from) {
    if (from._has_bits_[0]) {
      if (from.has_key()) {
        KeyCppHandler::EnsureMutable(&key_);
        KeyCppHandler::Merge(from.key(), &key_);
        set_has_key();
      }
      if (from.has_value()) {
        ValueCppHandler::EnsureMutable(&value_);
        ValueCppHandler::Merge(from.value(), &value_);
        set_has_value();
      }
    }
  }

  void Clear() {
    KeyCppHandler::Clear(&key_);
    ValueCppHandler::ClearMaybeByDefaultEnum(&value_, default_enum_value);
    clear_has_key();
    clear_has_value();
  }

  void InitAsDefaultInstance() {
    KeyCppHandler::AssignDefaultValue(&key_);
    ValueCppHandler::AssignDefaultValue(&value_);
  }

  // Create default MapEntry instance for given descriptor. Descriptor has to be
  // given when creating default MapEntry instance because different map field
  // may have the same type and MapEntry class. The given descriptor is needed
  // to distinguish instances of the same MapEntry class.
  static MapEntry* CreateDefaultInstance(const Descriptor* descriptor) {
    MapEntry* entry = new MapEntry();
    const Reflection* reflection = new GeneratedMessageReflection(
        descriptor, entry, offsets_,
        GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(MapEntry, _has_bits_),
        GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(MapEntry, _unknown_fields_), -1,
        DescriptorPool::generated_pool(),
        ::google::protobuf::MessageFactory::generated_factory(), sizeof(MapEntry), -1);
    entry->descriptor_ = descriptor;
    entry->reflection_ = reflection;
    entry->default_instance_ = entry;
    entry->InitAsDefaultInstance();
    RegisterMapEntryDefaultInstance(entry);
    return entry;
  }

  // Create a MapEntry for given key and value from google::protobuf::Map in
  // serialization. This function is only called when value is enum. Enum is
  // treated differently because its type in MapEntry is int and its type in
  // google::protobuf::Map is enum. We cannot create a reference to int from an enum.
  static MapEntry* EnumWrap(const Key& key, const Value value) {
    return new MapEnumEntryWrapper<Key, Value, KeyProtoType, ValueProtoType,
                                   default_enum_value>(key, value);
  }

  // Like above, but for all the other types. This avoids value copy to create
  // MapEntry from google::protobuf::Map in serialization.
  static MapEntry* Wrap(const Key& key, const Value& value) {
    return new MapEntryWrapper<Key, Value, KeyProtoType, ValueProtoType,
                               default_enum_value>(key, value);
  }

 protected:
  void set_has_key() { _has_bits_[0] |= 0x00000001u; }
  bool has_key() const { return (_has_bits_[0] & 0x00000001u) != 0; }
  void clear_has_key() { _has_bits_[0] &= ~0x00000001u; }
  void set_has_value() { _has_bits_[0] |= 0x00000002u; }
  bool has_value() const { return (_has_bits_[0] & 0x00000002u) != 0; }
  void clear_has_value() { _has_bits_[0] &= ~0x00000002u; }

 private:
  // Serializing a generated message containing map field involves serializing
  // key-value pairs from google::protobuf::Map. The wire format of each key-value pair
  // after serialization should be the same as that of a MapEntry message
  // containing the same key and value inside it.  However, google::protobuf::Map doesn't
  // store key and value as MapEntry message, which disables us to use existing
  // code to serialize message. In order to use existing code to serialize
  // message, we need to construct a MapEntry from key-value pair. But it
  // involves copy of key and value to construct a MapEntry. In order to avoid
  // this copy in constructing a MapEntry, we need the following class which
  // only takes references of given key and value.
  template <typename KeyNested, typename ValueNested,
            FieldDescriptor::Type KeyProtoNested,
            FieldDescriptor::Type ValueProtoNested, int default_enum>
  class MapEntryWrapper
      : public MapEntry<KeyNested, ValueNested, KeyProtoNested,
                        ValueProtoNested, default_enum> {
    typedef MapEntry<KeyNested, ValueNested, KeyProtoNested, ValueProtoNested,
                     default_enum_value> Base;
    typedef typename Base::KeyCppType KeyCppType;
    typedef typename Base::ValCppType ValCppType;

   public:
    MapEntryWrapper(const KeyNested& key, const ValueNested& value)
        : key_(key), value_(value) {
      Base::set_has_key();
      Base::set_has_value();
    }
    inline const KeyCppType& key() const { return key_; }
    inline const ValCppType& value() const { return value_; }

   private:
    const Key& key_;
    const Value& value_;
  };

  // Like above, but for enum value only, which stores value instead of
  // reference of value field inside. This is needed because the type of value
  // field in constructor is an enum, while we need to store it as an int. If we
  // initialize a reference to int with a reference to enum, compiler will
  // generate a temporary int from enum and initialize the reference to int with
  // the temporary.
  template <typename KeyNested, typename ValueNested,
            FieldDescriptor::Type KeyProtoNested,
            FieldDescriptor::Type ValueProtoNested, int default_enum>
  class MapEnumEntryWrapper
      : public MapEntry<KeyNested, ValueNested, KeyProtoNested,
                        ValueProtoNested, default_enum> {
    typedef MapEntry<KeyNested, ValueNested, KeyProtoNested, ValueProtoNested,
                     default_enum> Base;
    typedef typename Base::KeyCppType KeyCppType;
    typedef typename Base::ValCppType ValCppType;

   public:
    MapEnumEntryWrapper(const KeyNested& key, const ValueNested& value)
        : key_(key), value_(value) {
      Base::set_has_key();
      Base::set_has_value();
    }
    inline const KeyCppType& key() const { return key_; }
    inline const ValCppType& value() const { return value_; }

   private:
    const KeyCppType& key_;
    const ValCppType value_;
  };

  MapEntry() : default_instance_(NULL) {
    KeyCppHandler::Initialize(&key_);
    ValueCppHandler::InitializeMaybeByDefaultEnum(&value_, default_enum_value);
    _has_bits_[0] = 0;
  }

  KeyBase key_;
  ValueBase value_;
  static int offsets_[2];
  UnknownFieldSet _unknown_fields_;
  uint32 _has_bits_[1];
  MapEntry* default_instance_;

  friend class ::google::protobuf::Arena;
  template <typename K, typename V,
            FieldDescriptor::Type KType,
            FieldDescriptor::Type VType, int default_enum>
  friend class internal::MapField;
  friend class LIBPROTOBUF_EXPORT internal::GeneratedMessageReflection;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(MapEntry);
};

template <typename Key, typename Value, FieldDescriptor::Type KeyProtoType,
          FieldDescriptor::Type ValueProtoType, int default_enum_value>
int MapEntry<Key, Value, KeyProtoType, ValueProtoType,
             default_enum_value>::offsets_[2] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(MapEntry, key_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(MapEntry, value_),
};

}  // namespace internal
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_MAP_ENTRY_H__
