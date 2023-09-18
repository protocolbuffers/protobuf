// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_MAP_ENTRY_H__
#define GOOGLE_PROTOBUF_MAP_ENTRY_H__

#include <cstddef>
#include <cstdint>
#include <string>

#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/map_type_handler.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"
#include "google/protobuf/reflection_ops.h"
#include "google/protobuf/unknown_field_set.h"
#include "google/protobuf/wire_format_lite.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {
class Arena;
namespace internal {
template <typename Derived, typename Key, typename Value,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType>
class MapField;
}
}  // namespace protobuf
}  // namespace google

namespace google {
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
// int32_t    TYPE_INT32    int32_t          int32_t
// int32_t    TYPE_FIXED32  int32_t          int32_t
// string     TYPE_STRING   ArenaStringPtr   string
// FooEnum    TYPE_ENUM     int              int
// FooMessage TYPE_MESSAGE  FooMessage*      FooMessage
//
// The in-memory types of primitive types can be inferred from its proto type,
// while we need to explicitly specify the cpp type if proto type is
// TYPE_MESSAGE to infer the in-memory type.
template <typename Derived, typename Key, typename Value,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType>
class MapEntry : public Message {
  // Provide utilities to parse/serialize key/value.  Provide utilities to
  // manipulate internal stored type.
  using KeyTypeHandler = MapTypeHandler<kKeyFieldType, Key>;
  using ValueTypeHandler = MapTypeHandler<kValueFieldType, Value>;

  // Define internal memory layout. Strings and messages are stored as
  // pointers, while other types are stored as values.
  using KeyOnMemory = typename KeyTypeHandler::TypeOnMemory;
  using ValueOnMemory = typename ValueTypeHandler::TypeOnMemory;

  // Enum type cannot be used for MapTypeHandler::Read. Define a type
  // which will replace Enum with int.
  using KeyMapEntryAccessorType = typename KeyTypeHandler::MapEntryAccessorType;
  using ValueMapEntryAccessorType =
      typename ValueTypeHandler::MapEntryAccessorType;

  // Constants for field number.
  static const int kKeyFieldNumber = 1;
  static const int kValueFieldNumber = 2;

  // Constants for field tag.
  static const uint8_t kKeyTag =
      GOOGLE_PROTOBUF_WIRE_FORMAT_MAKE_TAG(kKeyFieldNumber, KeyTypeHandler::kWireType);
  static const uint8_t kValueTag = GOOGLE_PROTOBUF_WIRE_FORMAT_MAKE_TAG(
      kValueFieldNumber, ValueTypeHandler::kWireType);
  static const size_t kTagSize = 1;

 public:
  constexpr MapEntry()
      : key_(KeyTypeHandler::Constinit()),
        value_(ValueTypeHandler::Constinit()),
        _has_bits_{} {}

  explicit MapEntry(Arena* arena)
      : Message(arena),
        key_(KeyTypeHandler::Constinit()),
        value_(ValueTypeHandler::Constinit()),
        _has_bits_{} {}

  MapEntry(const MapEntry&) = delete;
  MapEntry& operator=(const MapEntry&) = delete;

  ~MapEntry() override {
    if (GetArenaForAllocation() != nullptr) return;
    Message::_internal_metadata_.template Delete<UnknownFieldSet>();
    KeyTypeHandler::DeleteNoArena(key_);
    ValueTypeHandler::DeleteNoArena(value_);
  }

  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;

  // accessors ======================================================

  inline const KeyMapEntryAccessorType& key() const {
    return KeyTypeHandler::GetExternalReference(key_);
  }
  inline const ValueMapEntryAccessorType& value() const {
    return ValueTypeHandler::DefaultIfNotInitialized(value_);
  }
  inline KeyMapEntryAccessorType* mutable_key() {
    set_has_key();
    return KeyTypeHandler::EnsureMutable(&key_, GetArenaForAllocation());
  }
  inline ValueMapEntryAccessorType* mutable_value() {
    set_has_value();
    return ValueTypeHandler::EnsureMutable(&value_, GetArenaForAllocation());
  }

  // implements MessageLite =========================================

  // MapEntry is for implementation only and this function isn't called
  // anywhere. Just provide a fake implementation here for MessageLite.
  std::string GetTypeName() const override { return ""; }

  size_t SpaceUsedLong() const final {
    size_t size = sizeof(Derived);
    size += KeyTypeHandler::SpaceUsedInMapEntryLong(this->key_);
    size += ValueTypeHandler::SpaceUsedInMapEntryLong(this->value_);
    return size;
  }

  void CheckTypeAndMergeFrom(const MessageLite& other) override {
    MergeFromInternal(*::google::protobuf::internal::DownCast<const Derived*>(&other));
  }

  const char* _InternalParse(const char* ptr, ParseContext* ctx) final {
    while (!ctx->Done(&ptr)) {
      uint32_t tag;
      ptr = ReadTag(ptr, &tag);
      GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
      if (tag == kKeyTag) {
        set_has_key();
        KeyMapEntryAccessorType* key = mutable_key();
        ptr = KeyTypeHandler::Read(ptr, ctx, key);
        if (!Derived::ValidateKey(key)) return nullptr;
      } else if (tag == kValueTag) {
        set_has_value();
        ValueMapEntryAccessorType* value = mutable_value();
        ptr = ValueTypeHandler::Read(ptr, ctx, value);
        if (!Derived::ValidateValue(value)) return nullptr;
      } else {
        if (tag == 0 || WireFormatLite::GetTagWireType(tag) ==
                            WireFormatLite::WIRETYPE_END_GROUP) {
          ctx->SetLastTag(tag);
          return ptr;
        }
        ptr = UnknownFieldParse(tag, static_cast<std::string*>(nullptr), ptr,
                                ctx);
      }
      GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
    }
    return ptr;
  }

  size_t ByteSizeLong() const override {
    size_t size = 0;
    size += kTagSize + static_cast<size_t>(KeyTypeHandler::ByteSize(key()));
    size += kTagSize + static_cast<size_t>(ValueTypeHandler::ByteSize(value()));
    return size;
  }

  ::uint8_t* _InternalSerialize(
      ::uint8_t* ptr, io::EpsCopyOutputStream* stream) const override {
    ptr = KeyTypeHandler::Write(kKeyFieldNumber, key(), ptr, stream);
    return ValueTypeHandler::Write(kValueFieldNumber, value(), ptr, stream);
  }

  bool IsInitialized() const override {
    return ValueTypeHandler::IsInitialized(value_);
  }

  Message* New(Arena* arena) const override {
    Derived* entry = Arena::CreateMessage<Derived>(arena);
    return entry;
  }

  void Clear() final {
    KeyTypeHandler::Clear(&key_, GetArenaForAllocation());
    ValueTypeHandler::Clear(&value_, GetArenaForAllocation());
    clear_has_key();
    clear_has_value();
  }

 protected:
  friend class google::protobuf::Arena;
  template <typename C, typename K, typename V, WireFormatLite::FieldType,
            WireFormatLite::FieldType>
  friend class MapField;

  // We can't declare this function directly here as it would hide the other
  // overload (const Message&).
  void MergeFromInternal(const MapEntry& from) {
    if (from._has_bits_[0]) {
      if (from.has_key()) {
        KeyTypeHandler::EnsureMutable(&key_, GetArenaForAllocation());
        KeyTypeHandler::Merge(from.key(), &key_, GetArenaForAllocation());
        set_has_key();
      }
      if (from.has_value()) {
        ValueTypeHandler::EnsureMutable(&value_, GetArenaForAllocation());
        ValueTypeHandler::Merge(from.value(), &value_, GetArenaForAllocation());
        set_has_value();
      }
    }
  }

  void set_has_key() { _has_bits_[0] |= 0x00000001u; }
  bool has_key() const { return (_has_bits_[0] & 0x00000001u) != 0; }
  void clear_has_key() { _has_bits_[0] &= ~0x00000001u; }
  void set_has_value() { _has_bits_[0] |= 0x00000002u; }
  bool has_value() const { return (_has_bits_[0] & 0x00000002u) != 0; }
  void clear_has_value() { _has_bits_[0] &= ~0x00000002u; }

  KeyOnMemory key_;
  ValueOnMemory value_;
  uint32_t _has_bits_[1];
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_MAP_ENTRY_H__
