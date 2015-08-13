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

#ifndef GOOGLE_PROTOBUF_TYPE_HANDLER_H__
#define GOOGLE_PROTOBUF_TYPE_HANDLER_H__

#include <google/protobuf/arena.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/wire_format_lite_inl.h>

namespace google {
namespace protobuf {
namespace internal {

// Used for compile time type selection. MapIf::type will be TrueType if Flag is
// true and FalseType otherwise.
template<bool Flag, typename TrueType, typename FalseType>
struct MapIf;

template<typename TrueType, typename FalseType>
struct MapIf<true, TrueType, FalseType> {
  typedef TrueType type;
};

template<typename TrueType, typename FalseType>
struct MapIf<false, TrueType, FalseType> {
  typedef FalseType type;
};

// In MapField, string and message are stored as pointer while others are stored
// as object. However, google::protobuf::Map has unified api. Functions in this class
// convert key/value to type wanted in api regardless how it's stored
// internally.
template <typename Type>
class MapCommonTypeHandler {
 public:
  static inline Type& Reference(Type* x) { return *x; }
  static inline Type& Reference(Type& x) { return x; }
  static inline const Type& Reference(const Type& x) { return x; }
  static inline Type* Pointer(Type* x) { return x; }
  static inline Type* Pointer(Type& x) { return &x; }
  static inline const Type* Pointer(const Type* x) { return x; }
  static inline const Type* Pointer(const Type& x) { return &x; }
};

// In proto2 Map, enum needs to be initialized to given default value, while
// other types' default value can be inferred from the type.
template <bool IsEnum, typename Type>
class MapValueInitializer {
 public:
  static inline void Initialize(Type& type, int default_enum_value);
};

template <typename Type>
class MapValueInitializer<true, Type> {
 public:
  static inline void Initialize(Type& value, int default_enum_value) {
    value = static_cast<Type>(default_enum_value);
  }
};

template <typename Type>
class MapValueInitializer<false, Type> {
 public:
  static inline void Initialize(Type& value, int default_enum_value) {}
};

template <typename Type, bool is_arena_constructable>
class MapArenaMessageCreator {
 public:
  // Use arena to create message if Type is arena constructable. Otherwise,
  // create the message on heap.
  static inline Type* CreateMessage(Arena* arena);
};
template <typename Type>
class MapArenaMessageCreator<Type, true> {
 public:
  static inline Type* CreateMessage(Arena* arena) {
    return Arena::CreateMessage<Type>(arena);
  }
};
template <typename Type>
class MapArenaMessageCreator<Type, false> {
 public:
  static inline Type* CreateMessage(Arena* arena) {
    return new Type;
  }
};

// Handlers for key/value stored type in MapField. ==================

// Handler for message
template <typename Type>
class MapCppTypeHandler : public MapCommonTypeHandler<Type> {
 public:
  static const bool kIsStringOrMessage = true;
  // SpaceUsedInMapEntry: Return bytes used by value in MapEntry, excluding
  // those already calculate in sizeof(MapField).
  static int SpaceUsedInMapEntry(const Type* value) {
    return value->SpaceUsed();
  }
  // Return bytes used by value in Map.
  static int SpaceUsedInMap(const Type& value) { return value.SpaceUsed(); }
  static inline void Clear(Type** value) {
    if (*value != NULL) (*value)->Clear();
  }
  static inline void ClearMaybeByDefaultEnum(Type** value,
                                             int default_enum_value) {
    if (*value != NULL) (*value)->Clear();
  }
  static inline void Merge(const Type& from, Type** to) {
    (*to)->MergeFrom(from);
  }

  static void Delete(const Type* ptr) { delete ptr; }

  // Assign default value to given instance.
  static inline void AssignDefaultValue(Type** value) {
    *value = const_cast<Type*>(&Type::default_instance());
  }
  // Initialize value when constructing MapEntry
  static inline void Initialize(Type** x, Arena* arena) { *x = NULL; }
  // Same as above, but use default_enum_value to initialize enum type value.
  static inline void InitializeMaybeByDefaultEnum(
      Type** x, int default_enum_value, Arena* arena) {
    *x = NULL;
  }
  // Initialize value for the first time mutable accessor is called.
  static inline void EnsureMutable(Type** value, Arena* arena) {
    if (*value == NULL) {
      *value =
          MapArenaMessageCreator<Type, Arena::is_arena_constructable<Type>::
                                           type::value>::CreateMessage(arena);
    }
  }
  // Return default instance if value is not initialized when calling const
  // reference accessor.
  static inline const Type& DefaultIfNotInitialized(const Type* value,
                                                    const Type* default_value) {
    return value != NULL ? *value : *default_value;
  }
  // Check if all required fields have values set.
  static inline bool IsInitialized(Type* value) {
    return value->IsInitialized();
  }
};

// Handler for string.
template <>
class MapCppTypeHandler<string> : public MapCommonTypeHandler<string> {
 public:
  static const bool kIsStringOrMessage = true;
  static inline void Merge(const string& from, string** to) { **to = from; }
  static inline void Clear(string** value) { (*value)->clear(); }
  static inline void ClearMaybeByDefaultEnum(string** value, int default_enum) {
    (*value)->clear();
  }
  static inline int SpaceUsedInMapEntry(const string* value) {
    return sizeof(*value) + StringSpaceUsedExcludingSelf(*value);
  }
  static inline int SpaceUsedInMap(const string& value) {
    return sizeof(value) + StringSpaceUsedExcludingSelf(value);
  }
  static void Delete(const string* ptr) {
    if (ptr != &::google::protobuf::internal::GetEmptyString()) delete ptr;
  }
  static inline void AssignDefaultValue(string** value) {}
  static inline void Initialize(string** value, Arena* arena) {
    *value = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyString());
    if (arena != NULL) arena->Own(*value);
  }
  static inline void InitializeMaybeByDefaultEnum(
      string** value, int default_enum_value, Arena* arena) {
    *value = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyString());
    if (arena != NULL) arena->Own(*value);
  }
  static inline void EnsureMutable(string** value, Arena* arena) {
    if (*value == &::google::protobuf::internal::GetEmptyString()) {
      *value = Arena::Create<string>(arena);
    }
  }
  static inline const string& DefaultIfNotInitialized(
      const string* value,
      const string* default_value) {
    return value != default_value ? *value : *default_value;
  }
  static inline bool IsInitialized(string* value) { return true; }
};

// Base class for primitive type handlers.
template <typename Type>
class MapPrimitiveTypeHandler : public MapCommonTypeHandler<Type> {
 public:
  static const bool kIsStringOrMessage = false;
  static inline void Delete(const Type& x) {}
  static inline void Merge(const Type& from, Type* to) { *to = from; }
  static inline int SpaceUsedInMapEntry(const Type& value) { return 0; }
  static inline int SpaceUsedInMap(const Type& value) { return sizeof(Type); }
  static inline void AssignDefaultValue(Type* value) {}
  static inline const Type& DefaultIfNotInitialized(
      const Type& value, const Type& default_value) {
    return value;
  }
  static inline bool IsInitialized(const Type& value) { return true; }
};

// Handlers for primitive types.
#define PRIMITIVE_HANDLER(CType)                                              \
  template <>                                                                 \
  class MapCppTypeHandler<CType> : public MapPrimitiveTypeHandler<CType> {    \
   public:                                                                    \
    static inline void Clear(CType* value) { *value = 0; }                    \
    static inline void ClearMaybeByDefaultEnum(CType* value,                  \
                                               int default_enum_value) {      \
      *value = static_cast<CType>(default_enum_value);                        \
    }                                                                         \
    static inline void Initialize(CType* value, Arena* arena) { *value = 0; } \
    static inline void InitializeMaybeByDefaultEnum(CType* value,             \
                                                    int default_enum_value,   \
                                                    Arena* arena) {           \
      *value = static_cast<CType>(default_enum_value);                        \
    }                                                                         \
    static inline void EnsureMutable(CType* value, Arena* arena) {}           \
  };

PRIMITIVE_HANDLER(int32 )
PRIMITIVE_HANDLER(int64 )
PRIMITIVE_HANDLER(uint32)
PRIMITIVE_HANDLER(uint64)
PRIMITIVE_HANDLER(double)
PRIMITIVE_HANDLER(float )
PRIMITIVE_HANDLER(bool  )

#undef PRIMITIVE_HANDLER

// Define constants for given wire field type
template <WireFormatLite::FieldType field_type>
class MapWireFieldTypeTraits {};

#define TYPE_TRAITS(FieldType, CType, WireFormatType, IsMessage, IsEnum) \
  template <>                                                            \
  class MapWireFieldTypeTraits<WireFormatLite::TYPE_##FieldType> {       \
   public:                                                               \
    typedef CType CppType;                                               \
    static const bool kIsMessage = IsMessage;                            \
    static const bool kIsEnum = IsEnum;                                  \
    static const WireFormatLite::WireType kWireType =                    \
        WireFormatLite::WIRETYPE_##WireFormatType;                       \
  };

TYPE_TRAITS(MESSAGE , MessageLite, LENGTH_DELIMITED, true, false)
TYPE_TRAITS(STRING  , string ,  LENGTH_DELIMITED, false, false)
TYPE_TRAITS(BYTES   , string ,  LENGTH_DELIMITED, false, false)
TYPE_TRAITS(INT64   , int64  ,  VARINT , false, false)
TYPE_TRAITS(UINT64  , uint64 ,  VARINT , false, false)
TYPE_TRAITS(INT32   , int32  ,  VARINT , false, false)
TYPE_TRAITS(UINT32  , uint32 ,  VARINT , false, false)
TYPE_TRAITS(SINT64  , int64  ,  VARINT , false, false)
TYPE_TRAITS(SINT32  , int32  ,  VARINT , false, false)
TYPE_TRAITS(ENUM    , int    ,  VARINT , false, true )
TYPE_TRAITS(DOUBLE  , double ,  FIXED64, false, false)
TYPE_TRAITS(FLOAT   , float  ,  FIXED32, false, false)
TYPE_TRAITS(FIXED64 , uint64 ,  FIXED64, false, false)
TYPE_TRAITS(FIXED32 , uint32 ,  FIXED32, false, false)
TYPE_TRAITS(SFIXED64, int64  ,  FIXED64, false, false)
TYPE_TRAITS(SFIXED32, int32  ,  FIXED32, false, false)
TYPE_TRAITS(BOOL    , bool   ,  VARINT , false, false)

#undef TYPE_TRAITS

template <WireFormatLite::FieldType field_type>
class MapWireFieldTypeHandler {
 public:
  // Internal stored type in MapEntryLite for given wire field type.
  typedef typename MapWireFieldTypeTraits<field_type>::CppType CppType;
  // Corresponding wire type for field type.
  static const WireFormatLite::WireType kWireType =
      MapWireFieldTypeTraits<field_type>::kWireType;
  // Whether wire type is for message.
  static const bool kIsMessage = MapWireFieldTypeTraits<field_type>::kIsMessage;
  // Whether wire type is for enum.
  static const bool kIsEnum = MapWireFieldTypeTraits<field_type>::kIsEnum;

  // Functions used in parsing and serialization. ===================
  template <typename ValueType>
  static inline int ByteSize(const ValueType& value);
  template <typename ValueType>
  static inline int GetCachedSize(const ValueType& value);
  template <typename ValueType>
  static inline bool Read(io::CodedInputStream* input, ValueType* value);
  static inline void Write(int field, const CppType& value,
                           io::CodedOutputStream* output);
  static inline uint8* WriteToArray(int field, const CppType& value,
                                    uint8* output);
};

template <>
template <typename ValueType>
inline int MapWireFieldTypeHandler<WireFormatLite::TYPE_MESSAGE>::ByteSize(
    const ValueType& value) {
  return WireFormatLite::MessageSizeNoVirtual(value);
}

#define BYTE_SIZE(FieldType, DeclaredType)                             \
  template <>                                                          \
  template <typename ValueType>                                        \
  inline int                                                           \
  MapWireFieldTypeHandler<WireFormatLite::TYPE_##FieldType>::ByteSize( \
      const ValueType& value) {                                        \
    return WireFormatLite::DeclaredType##Size(value);                  \
  }

BYTE_SIZE(STRING, String)
BYTE_SIZE(BYTES , Bytes)
BYTE_SIZE(INT64 , Int64)
BYTE_SIZE(UINT64, UInt64)
BYTE_SIZE(INT32 , Int32)
BYTE_SIZE(UINT32, UInt32)
BYTE_SIZE(SINT64, SInt64)
BYTE_SIZE(SINT32, SInt32)
BYTE_SIZE(ENUM  , Enum)

#undef BYTE_SIZE

#define FIXED_BYTE_SIZE(FieldType, DeclaredType)                       \
  template <>                                                          \
  template <typename ValueType>                                        \
  inline int                                                           \
  MapWireFieldTypeHandler<WireFormatLite::TYPE_##FieldType>::ByteSize( \
      const ValueType& value) {                                        \
    return WireFormatLite::k##DeclaredType##Size;                      \
  }

FIXED_BYTE_SIZE(DOUBLE  , Double)
FIXED_BYTE_SIZE(FLOAT   , Float)
FIXED_BYTE_SIZE(FIXED64 , Fixed64)
FIXED_BYTE_SIZE(FIXED32 , Fixed32)
FIXED_BYTE_SIZE(SFIXED64, SFixed64)
FIXED_BYTE_SIZE(SFIXED32, SFixed32)
FIXED_BYTE_SIZE(BOOL    , Bool)

#undef FIXED_BYTE_SIZE

template <>
template <typename ValueType>
inline int MapWireFieldTypeHandler<
    WireFormatLite::TYPE_MESSAGE>::GetCachedSize(const ValueType& value) {
  return WireFormatLite::LengthDelimitedSize(value.GetCachedSize());
}

#define GET_CACHED_SIZE(FieldType, DeclaredType)                            \
  template <>                                                               \
  template <typename ValueType>                                             \
  inline int                                                                \
  MapWireFieldTypeHandler<WireFormatLite::TYPE_##FieldType>::GetCachedSize( \
      const ValueType& value) {                                             \
    return WireFormatLite::DeclaredType##Size(value);                       \
  }

GET_CACHED_SIZE(STRING, String)
GET_CACHED_SIZE(BYTES , Bytes)
GET_CACHED_SIZE(INT64 , Int64)
GET_CACHED_SIZE(UINT64, UInt64)
GET_CACHED_SIZE(INT32 , Int32)
GET_CACHED_SIZE(UINT32, UInt32)
GET_CACHED_SIZE(SINT64, SInt64)
GET_CACHED_SIZE(SINT32, SInt32)
GET_CACHED_SIZE(ENUM  , Enum)

#undef GET_CACHED_SIZE

#define GET_FIXED_CACHED_SIZE(FieldType, DeclaredType)                      \
  template <>                                                               \
  template <typename ValueType>                                             \
  inline int                                                                \
  MapWireFieldTypeHandler<WireFormatLite::TYPE_##FieldType>::GetCachedSize( \
      const ValueType& value) {                                             \
    return WireFormatLite::k##DeclaredType##Size;                           \
  }

GET_FIXED_CACHED_SIZE(DOUBLE  , Double)
GET_FIXED_CACHED_SIZE(FLOAT   , Float)
GET_FIXED_CACHED_SIZE(FIXED64 , Fixed64)
GET_FIXED_CACHED_SIZE(FIXED32 , Fixed32)
GET_FIXED_CACHED_SIZE(SFIXED64, SFixed64)
GET_FIXED_CACHED_SIZE(SFIXED32, SFixed32)
GET_FIXED_CACHED_SIZE(BOOL    , Bool)

#undef GET_FIXED_CACHED_SIZE

template <>
inline void MapWireFieldTypeHandler<WireFormatLite::TYPE_MESSAGE>::Write(
    int field, const MessageLite& value, io::CodedOutputStream* output) {
  WireFormatLite::WriteMessageMaybeToArray(field, value, output);
}

template <>
inline uint8*
MapWireFieldTypeHandler<WireFormatLite::TYPE_MESSAGE>::WriteToArray(
    int field, const MessageLite& value, uint8* output) {
  return WireFormatLite::WriteMessageToArray(field, value, output);
}

#define WRITE_METHOD(FieldType, DeclaredType)                                  \
  template <>                                                                  \
  inline void                                                                  \
  MapWireFieldTypeHandler<WireFormatLite::TYPE_##FieldType>::Write(            \
      int field, const CppType& value, io::CodedOutputStream* output) {        \
    return WireFormatLite::Write##DeclaredType(field, value, output);          \
  }                                                                            \
  template <>                                                                  \
  inline uint8*                                                                \
  MapWireFieldTypeHandler<WireFormatLite::TYPE_##FieldType>::WriteToArray(     \
      int field, const CppType& value, uint8* output) {                        \
    return WireFormatLite::Write##DeclaredType##ToArray(field, value, output); \
  }

WRITE_METHOD(STRING  , String)
WRITE_METHOD(BYTES   , Bytes)
WRITE_METHOD(INT64   , Int64)
WRITE_METHOD(UINT64  , UInt64)
WRITE_METHOD(INT32   , Int32)
WRITE_METHOD(UINT32  , UInt32)
WRITE_METHOD(SINT64  , SInt64)
WRITE_METHOD(SINT32  , SInt32)
WRITE_METHOD(ENUM    , Enum)
WRITE_METHOD(DOUBLE  , Double)
WRITE_METHOD(FLOAT   , Float)
WRITE_METHOD(FIXED64 , Fixed64)
WRITE_METHOD(FIXED32 , Fixed32)
WRITE_METHOD(SFIXED64, SFixed64)
WRITE_METHOD(SFIXED32, SFixed32)
WRITE_METHOD(BOOL    , Bool)

#undef WRITE_METHOD

template <>
template <typename ValueType>
inline bool MapWireFieldTypeHandler<WireFormatLite::TYPE_MESSAGE>::Read(
    io::CodedInputStream* input, ValueType* value) {
  return WireFormatLite::ReadMessageNoVirtual(input, value);
}

template <>
template <typename ValueType>
inline bool MapWireFieldTypeHandler<WireFormatLite::TYPE_STRING>::Read(
    io::CodedInputStream* input, ValueType* value) {
  return WireFormatLite::ReadString(input, value);
}

template <>
template <typename ValueType>
inline bool MapWireFieldTypeHandler<WireFormatLite::TYPE_BYTES>::Read(
    io::CodedInputStream* input, ValueType* value) {
  return WireFormatLite::ReadBytes(input, value);
}

#define READ_METHOD(FieldType)                                                 \
  template <>                                                                  \
  template <typename ValueType>                                                \
  inline bool MapWireFieldTypeHandler<WireFormatLite::TYPE_##FieldType>::Read( \
      io::CodedInputStream* input, ValueType* value) {                         \
    return WireFormatLite::ReadPrimitive<CppType,                              \
                                         WireFormatLite::TYPE_##FieldType>(    \
        input, value);                                                         \
  }

READ_METHOD(INT64)
READ_METHOD(UINT64)
READ_METHOD(INT32)
READ_METHOD(UINT32)
READ_METHOD(SINT64)
READ_METHOD(SINT32)
READ_METHOD(ENUM)
READ_METHOD(DOUBLE)
READ_METHOD(FLOAT)
READ_METHOD(FIXED64)
READ_METHOD(FIXED32)
READ_METHOD(SFIXED64)
READ_METHOD(SFIXED32)
READ_METHOD(BOOL)

#undef READ_METHOD

}  // namespace internal
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_TYPE_HANDLER_H__
