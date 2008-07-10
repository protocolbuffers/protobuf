// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// This header is logically internal, but is made public because it is used
// from protocol-compiler-generated code, which may reside in other components.

#ifndef GOOGLE_PROTOBUF_GENERATED_MESSAGE_REFLECTION_H__
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_REFLECTION_H__

#include <string>
#include <vector>
#include <google/protobuf/message.h>
#include <google/protobuf/unknown_field_set.h>


// Generated code needs this to have been forward-declared.  Easier to do it
// here than to print it inside every .pb.h file.
namespace google {
namespace protobuf { class EnumDescriptor; }

namespace protobuf {
namespace internal {

// Defined in this file.
class GeneratedMessageReflection;

// Defined in other files.
class ExtensionSet;             // extension_set.h

// THIS CLASS IS NOT INTENDED FOR DIRECT USE.  It is intended for use
// by generated code.  This class is just a big hack that reduces code
// size.
//
// A GeneratedMessageReflection is an implementation of Message::Reflection
// which expects all fields to be backed by simple variables located in
// memory.  The locations are given using a base pointer and a set of
// offsets.
//
// It is required that the user represents fields of each type in a standard
// way, so that GeneratedMessageReflection can cast the void* pointer to
// the appropriate type.  For primitive fields and string fields, each field
// should be represented using the obvious C++ primitive type.  Enums and
// Messages are different:
//  - Singular Message fields are stored as a pointer to a Message.  These
//    should start out NULL, except for in the default instance where they
//    should start out pointing to other default instances.
//  - Enum fields are stored as an int.  This int must always contain
//    a valid value, such that EnumDescriptor::FindValueByNumber() would
//    not return NULL.
//  - Repeated fields are stored as RepeatedFields or RepeatedPtrFields
//    of whatever type the individual field would be.  Strings and
//    Messages use RepeatedPtrFields while everything else uses
//    RepeatedFields.
class LIBPROTOBUF_EXPORT GeneratedMessageReflection : public Message::Reflection {
 public:
  // Constructs a GeneratedMessageReflection.
  // Parameters:
  //   descriptor:    The descriptor for the message type being implemented.
  //   base:          Pointer to the location where the message object is
  //                  stored.
  //   default_base:  Pointer to the location where the message's default
  //                  instance is stored.  This is only used to obtain
  //                  pointers to default instances of embedded messages,
  //                  which GetMessage() will return if the particular sub-
  //                  message has not been initialized yet.  (Thus, all
  //                  embedded message fields *must* have non-NULL pointers
  //                  in the default instance.)
  //   offsets:       An array of bits giving the byte offsets, relative to
  //                  "base" and "default_base", of each field.  These can
  //                  be computed at compile time using the
  //                  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET() macro, defined
  //                  below.
  //   has_bits:      An array of uint32s of size descriptor->field_count()/32,
  //                  rounded up.  This is a bitfield where each bit indicates
  //                  whether or not the corresponding field of the message
  //                  has been initialized.  The bit for field index i is
  //                  obtained by the expression:
  //                    has_bits[i / 32] & (1 << (i % 32))
  //   extensions:    The ExtensionSet for this message, or NULL if the
  //                  message type has no extension ranges.
  GeneratedMessageReflection(const Descriptor* descriptor,
                             void* base, const void* default_base,
                             const int offsets[], uint32 has_bits[],
                             ExtensionSet* extensions);
  ~GeneratedMessageReflection();

  inline const UnknownFieldSet& unknown_fields() const {
    return unknown_fields_;
  }
  inline UnknownFieldSet* mutable_unknown_fields() {
    return &unknown_fields_;
  }

  // implements Message::Reflection ----------------------------------

  const UnknownFieldSet& GetUnknownFields() const;
  UnknownFieldSet* MutableUnknownFields();

  bool HasField(const FieldDescriptor* field) const;
  int FieldSize(const FieldDescriptor* field) const;
  void ClearField(const FieldDescriptor* field);
  void ListFields(vector<const FieldDescriptor*>* output) const;

  int32  GetInt32 (const FieldDescriptor* field) const;
  int64  GetInt64 (const FieldDescriptor* field) const;
  uint32 GetUInt32(const FieldDescriptor* field) const;
  uint64 GetUInt64(const FieldDescriptor* field) const;
  float  GetFloat (const FieldDescriptor* field) const;
  double GetDouble(const FieldDescriptor* field) const;
  bool   GetBool  (const FieldDescriptor* field) const;
  string GetString(const FieldDescriptor* field) const;
  const string& GetStringReference(const FieldDescriptor* field,
                                   string* scratch) const;
  const EnumValueDescriptor* GetEnum(const FieldDescriptor* field) const;
  const Message& GetMessage(const FieldDescriptor* field) const;

  void SetInt32 (const FieldDescriptor* field, int32  value);
  void SetInt64 (const FieldDescriptor* field, int64  value);
  void SetUInt32(const FieldDescriptor* field, uint32 value);
  void SetUInt64(const FieldDescriptor* field, uint64 value);
  void SetFloat (const FieldDescriptor* field, float  value);
  void SetDouble(const FieldDescriptor* field, double value);
  void SetBool  (const FieldDescriptor* field, bool   value);
  void SetString(const FieldDescriptor* field,
                 const string& value);
  void SetEnum  (const FieldDescriptor* field,
                 const EnumValueDescriptor* value);
  Message* MutableMessage(const FieldDescriptor* field);

  int32  GetRepeatedInt32 (const FieldDescriptor* field, int index) const;
  int64  GetRepeatedInt64 (const FieldDescriptor* field, int index) const;
  uint32 GetRepeatedUInt32(const FieldDescriptor* field, int index) const;
  uint64 GetRepeatedUInt64(const FieldDescriptor* field, int index) const;
  float  GetRepeatedFloat (const FieldDescriptor* field, int index) const;
  double GetRepeatedDouble(const FieldDescriptor* field, int index) const;
  bool   GetRepeatedBool  (const FieldDescriptor* field, int index) const;
  string GetRepeatedString(const FieldDescriptor* field, int index) const;
  const string& GetRepeatedStringReference(const FieldDescriptor* field,
                                           int index, string* scratch) const;
  const EnumValueDescriptor* GetRepeatedEnum(const FieldDescriptor* field,
                                             int index) const;
  const Message& GetRepeatedMessage(const FieldDescriptor* field,
                                    int index) const;

  // Set the value of a field.
  void SetRepeatedInt32 (const FieldDescriptor* field, int index, int32  value);
  void SetRepeatedInt64 (const FieldDescriptor* field, int index, int64  value);
  void SetRepeatedUInt32(const FieldDescriptor* field, int index, uint32 value);
  void SetRepeatedUInt64(const FieldDescriptor* field, int index, uint64 value);
  void SetRepeatedFloat (const FieldDescriptor* field, int index, float  value);
  void SetRepeatedDouble(const FieldDescriptor* field, int index, double value);
  void SetRepeatedBool  (const FieldDescriptor* field, int index, bool   value);
  void SetRepeatedString(const FieldDescriptor* field, int index,
                         const string& value);
  void SetRepeatedEnum  (const FieldDescriptor* field, int index,
                         const EnumValueDescriptor* value);
  // Get a mutable pointer to a field with a message type.
  Message* MutableRepeatedMessage(const FieldDescriptor* field, int index);

  void AddInt32 (const FieldDescriptor* field, int32  value);
  void AddInt64 (const FieldDescriptor* field, int64  value);
  void AddUInt32(const FieldDescriptor* field, uint32 value);
  void AddUInt64(const FieldDescriptor* field, uint64 value);
  void AddFloat (const FieldDescriptor* field, float  value);
  void AddDouble(const FieldDescriptor* field, double value);
  void AddBool  (const FieldDescriptor* field, bool   value);
  void AddString(const FieldDescriptor* field, const string& value);
  void AddEnum(const FieldDescriptor* field, const EnumValueDescriptor* value);
  Message* AddMessage(const FieldDescriptor* field);

  const FieldDescriptor* FindKnownExtensionByName(const string& name) const;
  const FieldDescriptor* FindKnownExtensionByNumber(int number) const;

 private:
  friend class GeneratedMessage;

  const Descriptor* descriptor_;
  void* base_;
  const void* default_base_;
  const int* offsets_;

  // TODO(kenton):  These two pointers just point back into the message object.
  //   We could save space by removing them and using offsets instead.
  uint32* has_bits_;
  ExtensionSet* extensions_;

  // We put this directly in the GeneratedMessageReflection because every
  // message class needs it, and if we don't find any unknown fields, it
  // takes up only one pointer of space.
  UnknownFieldSet unknown_fields_;

  template <typename Type>
  inline const Type& GetRaw(const FieldDescriptor* field) const;
  template <typename Type>
  inline Type* MutableRaw(const FieldDescriptor* field);
  template <typename Type>
  inline const Type& DefaultRaw(const FieldDescriptor* field) const;
  inline const Message* GetMessagePrototype(const FieldDescriptor* field) const;

  inline bool HasBit(const FieldDescriptor* field) const;
  inline void SetBit(const FieldDescriptor* field);
  inline void ClearBit(const FieldDescriptor* field);

  template <typename Type>
  inline const Type& GetField(const FieldDescriptor* field) const;
  template <typename Type>
  inline void SetField(const FieldDescriptor* field, const Type& value);
  template <typename Type>
  inline Type* MutableField(const FieldDescriptor* field);
  template <typename Type>
  inline const Type& GetRepeatedField(const FieldDescriptor* field,
                                      int index) const;
  template <typename Type>
  inline void SetRepeatedField(const FieldDescriptor* field, int index,
                               const Type& value);
  template <typename Type>
  inline Type* MutableRepeatedField(const FieldDescriptor* field, int index);
  template <typename Type>
  inline void AddField(const FieldDescriptor* field, const Type& value);
  template <typename Type>
  inline Type* AddField(const FieldDescriptor* field);

  int GetExtensionNumberOrDie(const Descriptor* type) const;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(GeneratedMessageReflection);
};

// Returns the offset of the given field within the given aggregate type.
// This is equivalent to the ANSI C offsetof() macro.  However, according
// to the C++ standard, offsetof() only works on POD types, and GCC
// enforces this requirement with a warning.  In practice, this rule is
// unnecessarily strict; there is probably no compiler or platform on
// which the offsets of the direct fields of a class are non-constant.
// Fields inherited from superclasses *can* have non-constant offsets,
// but that's not what this macro will be used for.
//
// Note that we calculate relative to the pointer value 16 here since if we
// just use zero, GCC complains about dereferencing a NULL pointer.  We
// choose 16 rather than some other number just in case the compiler would
// be confused by an unaligned pointer.
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(TYPE, FIELD)    \
  (reinterpret_cast<const char*>(                             \
     &reinterpret_cast<const TYPE*>(16)->FIELD) -             \
   reinterpret_cast<const char*>(16))

// There are some places in proto2 where dynamic_cast would be useful as an
// optimization.  For example, take Message::MergeFrom(const Message& other).
// For a given generated message FooMessage, we generate these two methods:
//   void MergeFrom(const FooMessage& other);
//   void MergeFrom(const Message& other);
// The former method can be implemented directly in terms of FooMessage's
// inline accessors, but the latter method must work with the reflection
// interface.  However, if the parameter to the latter method is actually of
// type FooMessage, then we'd like to be able to just call the other method
// as an optimization.  So, we use dynamic_cast to check this.
//
// That said, dynamic_cast requires RTTI, which many people like to disable
// for performance and code size reasons.  When RTTI is not available, we
// still need to produce correct results.  So, in this case we have to fall
// back to using reflection, which is what we would have done anyway if the
// objects were not of the exact same class.
//
// dynamic_cast_if_available() implements this logic.  If RTTI is
// enabled, it does a dynamic_cast.  If RTTI is disabled, it just returns
// NULL.
//
// If you need to compile without RTTI, simply #define GOOGLE_PROTOBUF_NO_RTTI.
// On MSVC, this should be detected automatically.
template<typename To, typename From>
inline To dynamic_cast_if_available(From from) {
#if defined(GOOGLE_PROTOBUF_NO_RTTI) || (defined(_MSC_VER)&&!defined(_CPPRTTI))
  return NULL;
#else
  return dynamic_cast<To>(from);
#endif
}


}  // namespace internal
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_REFLECTION_H__
