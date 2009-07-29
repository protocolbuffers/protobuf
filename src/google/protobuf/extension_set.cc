// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/stubs/hash.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/stubs/map-util.h>

namespace google {
namespace protobuf {
namespace internal {

namespace {

inline WireFormatLite::FieldType real_type(FieldType type) {
  GOOGLE_DCHECK(type > 0 && type <= WireFormatLite::MAX_FIELD_TYPE);
  return static_cast<WireFormatLite::FieldType>(type);
}

inline WireFormatLite::CppType cpp_type(FieldType type) {
  return WireFormatLite::FieldTypeToCppType(real_type(type));
}

// Registry stuff.
struct ExtensionInfo {
  inline ExtensionInfo(FieldType type, bool is_repeated, bool is_packed)
      : type(type), is_repeated(is_repeated), is_packed(is_packed) {}

  FieldType type;
  bool is_repeated;
  bool is_packed;

  union {
    ExtensionSet::EnumValidityFunc* enum_is_valid;
    const MessageLite* message_prototype;
  };
};

typedef hash_map<pair<const MessageLite*, int>,
                 ExtensionInfo> ExtensionRegistry;
ExtensionRegistry* registry_ = NULL;
GoogleOnceType registry_init_;

void DeleteRegistry() {
  delete registry_;
  registry_ = NULL;
}

void InitRegistry() {
  registry_ = new ExtensionRegistry;
  internal::OnShutdown(&DeleteRegistry);
}

// This function is only called at startup, so there is no need for thread-
// safety.
void Register(const MessageLite* containing_type,
              int number, ExtensionInfo info) {
  ::google::protobuf::GoogleOnceInit(&registry_init_, &InitRegistry);

  if (!InsertIfNotPresent(registry_, make_pair(containing_type, number),
                          info)) {
    GOOGLE_LOG(FATAL) << "Multiple extension registrations for type \""
               << containing_type->GetTypeName()
               << "\", field number " << number << ".";
  }
}

const ExtensionInfo* FindRegisteredExtension(
    const MessageLite* containing_type, int number) {
  return (registry_ == NULL) ? NULL :
         FindOrNull(*registry_, make_pair(containing_type, number));
}

}  // namespace

void ExtensionSet::RegisterExtension(const MessageLite* containing_type,
                                     int number, FieldType type,
                                     bool is_repeated, bool is_packed) {
  GOOGLE_CHECK_NE(type, WireFormatLite::TYPE_ENUM);
  GOOGLE_CHECK_NE(type, WireFormatLite::TYPE_MESSAGE);
  GOOGLE_CHECK_NE(type, WireFormatLite::TYPE_GROUP);
  ExtensionInfo info(type, is_repeated, is_packed);
  Register(containing_type, number, info);
}

void ExtensionSet::RegisterEnumExtension(const MessageLite* containing_type,
                                         int number, FieldType type,
                                         bool is_repeated, bool is_packed,
                                         EnumValidityFunc* is_valid) {
  GOOGLE_CHECK_EQ(type, WireFormatLite::TYPE_ENUM);
  ExtensionInfo info(type, is_repeated, is_packed);
  info.enum_is_valid = is_valid;
  Register(containing_type, number, info);
}

void ExtensionSet::RegisterMessageExtension(const MessageLite* containing_type,
                                            int number, FieldType type,
                                            bool is_repeated, bool is_packed,
                                            const MessageLite* prototype) {
  GOOGLE_CHECK(type == WireFormatLite::TYPE_MESSAGE ||
        type == WireFormatLite::TYPE_GROUP);
  ExtensionInfo info(type, is_repeated, is_packed);
  info.message_prototype = prototype;
  Register(containing_type, number, info);
}


// ===================================================================
// Constructors and basic methods.

ExtensionSet::ExtensionSet() {}

ExtensionSet::~ExtensionSet() {
  for (map<int, Extension>::iterator iter = extensions_.begin();
       iter != extensions_.end(); ++iter) {
    iter->second.Free();
  }
}

// Defined in extension_set_heavy.cc.
// void ExtensionSet::AppendToList(const Descriptor* containing_type,
//                                 const DescriptorPool* pool,
//                                 vector<const FieldDescriptor*>* output) const

bool ExtensionSet::Has(int number) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  if (iter == extensions_.end()) return false;
  GOOGLE_DCHECK(!iter->second.is_repeated);
  return !iter->second.is_cleared;
}

int ExtensionSet::ExtensionSize(int number) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  if (iter == extensions_.end()) return false;
  return iter->second.GetSize();
}

void ExtensionSet::ClearExtension(int number) {
  map<int, Extension>::iterator iter = extensions_.find(number);
  if (iter == extensions_.end()) return;
  iter->second.Clear();
}

// ===================================================================
// Field accessors

namespace {

enum Cardinality {
  REPEATED,
  OPTIONAL
};

}  // namespace

#define GOOGLE_DCHECK_TYPE(EXTENSION, LABEL, CPPTYPE)                             \
  GOOGLE_DCHECK_EQ((EXTENSION).is_repeated ? REPEATED : OPTIONAL, LABEL);         \
  GOOGLE_DCHECK_EQ(cpp_type((EXTENSION).type), WireFormatLite::CPPTYPE_##CPPTYPE)

// -------------------------------------------------------------------
// Primitives

#define PRIMITIVE_ACCESSORS(UPPERCASE, LOWERCASE, CAMELCASE)                   \
                                                                               \
LOWERCASE ExtensionSet::Get##CAMELCASE(int number,                             \
                                       LOWERCASE default_value) const {        \
  map<int, Extension>::const_iterator iter = extensions_.find(number);         \
  if (iter == extensions_.end() || iter->second.is_cleared) {                  \
    return default_value;                                                      \
  } else {                                                                     \
    GOOGLE_DCHECK_TYPE(iter->second, OPTIONAL, UPPERCASE);                            \
    return iter->second.LOWERCASE##_value;                                     \
  }                                                                            \
}                                                                              \
                                                                               \
void ExtensionSet::Set##CAMELCASE(int number, FieldType type,                  \
                                  LOWERCASE value) {                           \
  Extension* extension;                                                        \
  if (MaybeNewExtension(number, &extension)) {                                 \
    extension->type = type;                                                    \
    GOOGLE_DCHECK_EQ(cpp_type(extension->type), WireFormatLite::CPPTYPE_##UPPERCASE); \
    extension->is_repeated = false;                                            \
  } else {                                                                     \
    GOOGLE_DCHECK_TYPE(*extension, OPTIONAL, UPPERCASE);                              \
  }                                                                            \
  extension->is_cleared = false;                                               \
  extension->LOWERCASE##_value = value;                                        \
}                                                                              \
                                                                               \
LOWERCASE ExtensionSet::GetRepeated##CAMELCASE(int number, int index) const {  \
  map<int, Extension>::const_iterator iter = extensions_.find(number);         \
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty)."; \
  GOOGLE_DCHECK_TYPE(iter->second, REPEATED, UPPERCASE);                              \
  return iter->second.repeated_##LOWERCASE##_value->Get(index);                \
}                                                                              \
                                                                               \
void ExtensionSet::SetRepeated##CAMELCASE(                                     \
    int number, int index, LOWERCASE value) {                                  \
  map<int, Extension>::iterator iter = extensions_.find(number);               \
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty)."; \
  GOOGLE_DCHECK_TYPE(iter->second, REPEATED, UPPERCASE);                              \
  iter->second.repeated_##LOWERCASE##_value->Set(index, value);                \
}                                                                              \
                                                                               \
void ExtensionSet::Add##CAMELCASE(int number, FieldType type,                  \
                                  bool packed, LOWERCASE value) {              \
  Extension* extension;                                                        \
  if (MaybeNewExtension(number, &extension)) {                                 \
    extension->type = type;                                                    \
    GOOGLE_DCHECK_EQ(cpp_type(extension->type), WireFormatLite::CPPTYPE_##UPPERCASE); \
    extension->is_repeated = true;                                             \
    extension->is_packed = packed;                                             \
    extension->repeated_##LOWERCASE##_value = new RepeatedField<LOWERCASE>();  \
  } else {                                                                     \
    GOOGLE_DCHECK_TYPE(*extension, REPEATED, UPPERCASE);                              \
    GOOGLE_DCHECK_EQ(extension->is_packed, packed);                                   \
  }                                                                            \
  extension->repeated_##LOWERCASE##_value->Add(value);                         \
}

PRIMITIVE_ACCESSORS( INT32,  int32,  Int32)
PRIMITIVE_ACCESSORS( INT64,  int64,  Int64)
PRIMITIVE_ACCESSORS(UINT32, uint32, UInt32)
PRIMITIVE_ACCESSORS(UINT64, uint64, UInt64)
PRIMITIVE_ACCESSORS( FLOAT,  float,  Float)
PRIMITIVE_ACCESSORS(DOUBLE, double, Double)
PRIMITIVE_ACCESSORS(  BOOL,   bool,   Bool)

#undef PRIMITIVE_ACCESSORS

// -------------------------------------------------------------------
// Enums

int ExtensionSet::GetEnum(int number, int default_value) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  if (iter == extensions_.end() || iter->second.is_cleared) {
    // Not present.  Return the default value.
    return default_value;
  } else {
    GOOGLE_DCHECK_TYPE(iter->second, OPTIONAL, ENUM);
    return iter->second.enum_value;
  }
}

void ExtensionSet::SetEnum(int number, FieldType type, int value) {
  Extension* extension;
  if (MaybeNewExtension(number, &extension)) {
    extension->type = type;
    GOOGLE_DCHECK_EQ(cpp_type(extension->type), WireFormatLite::CPPTYPE_ENUM);
    extension->is_repeated = false;
  } else {
    GOOGLE_DCHECK_TYPE(*extension, OPTIONAL, ENUM);
  }
  extension->is_cleared = false;
  extension->enum_value = value;
}

int ExtensionSet::GetRepeatedEnum(int number, int index) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty).";
  GOOGLE_DCHECK_TYPE(iter->second, REPEATED, ENUM);
  return iter->second.repeated_enum_value->Get(index);
}

void ExtensionSet::SetRepeatedEnum(int number, int index, int value) {
  map<int, Extension>::iterator iter = extensions_.find(number);
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty).";
  GOOGLE_DCHECK_TYPE(iter->second, REPEATED, ENUM);
  iter->second.repeated_enum_value->Set(index, value);
}

void ExtensionSet::AddEnum(int number, FieldType type,
                           bool packed, int value) {
  Extension* extension;
  if (MaybeNewExtension(number, &extension)) {
    extension->type = type;
    GOOGLE_DCHECK_EQ(cpp_type(extension->type), WireFormatLite::CPPTYPE_ENUM);
    extension->is_repeated = true;
    extension->is_packed = packed;
    extension->repeated_enum_value = new RepeatedField<int>();
  } else {
    GOOGLE_DCHECK_TYPE(*extension, REPEATED, ENUM);
    GOOGLE_DCHECK_EQ(extension->is_packed, packed);
  }
  extension->repeated_enum_value->Add(value);
}

// -------------------------------------------------------------------
// Strings

const string& ExtensionSet::GetString(int number,
                                      const string& default_value) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  if (iter == extensions_.end() || iter->second.is_cleared) {
    // Not present.  Return the default value.
    return default_value;
  } else {
    GOOGLE_DCHECK_TYPE(iter->second, OPTIONAL, STRING);
    return *iter->second.string_value;
  }
}

string* ExtensionSet::MutableString(int number, FieldType type) {
  Extension* extension;
  if (MaybeNewExtension(number, &extension)) {
    extension->type = type;
    GOOGLE_DCHECK_EQ(cpp_type(extension->type), WireFormatLite::CPPTYPE_STRING);
    extension->is_repeated = false;
    extension->string_value = new string;
  } else {
    GOOGLE_DCHECK_TYPE(*extension, OPTIONAL, STRING);
  }
  extension->is_cleared = false;
  return extension->string_value;
}

const string& ExtensionSet::GetRepeatedString(int number, int index) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty).";
  GOOGLE_DCHECK_TYPE(iter->second, REPEATED, STRING);
  return iter->second.repeated_string_value->Get(index);
}

string* ExtensionSet::MutableRepeatedString(int number, int index) {
  map<int, Extension>::iterator iter = extensions_.find(number);
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty).";
  GOOGLE_DCHECK_TYPE(iter->second, REPEATED, STRING);
  return iter->second.repeated_string_value->Mutable(index);
}

string* ExtensionSet::AddString(int number, FieldType type) {
  Extension* extension;
  if (MaybeNewExtension(number, &extension)) {
    extension->type = type;
    GOOGLE_DCHECK_EQ(cpp_type(extension->type), WireFormatLite::CPPTYPE_STRING);
    extension->is_repeated = true;
    extension->is_packed = false;
    extension->repeated_string_value = new RepeatedPtrField<string>();
  } else {
    GOOGLE_DCHECK_TYPE(*extension, REPEATED, STRING);
  }
  return extension->repeated_string_value->Add();
}

// -------------------------------------------------------------------
// Messages

const MessageLite& ExtensionSet::GetMessage(
    int number, const MessageLite& default_value) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  if (iter == extensions_.end()) {
    // Not present.  Return the default value.
    return default_value;
  } else {
    GOOGLE_DCHECK_TYPE(iter->second, OPTIONAL, MESSAGE);
    return *iter->second.message_value;
  }
}

// Defined in extension_set_heavy.cc.
// const MessageLite& ExtensionSet::GetMessage(int number,
//                                             const Descriptor* message_type,
//                                             MessageFactory* factory) const

MessageLite* ExtensionSet::MutableMessage(int number, FieldType type,
                                          const MessageLite& prototype) {
  Extension* extension;
  if (MaybeNewExtension(number, &extension)) {
    extension->type = type;
    GOOGLE_DCHECK_EQ(cpp_type(extension->type), WireFormatLite::CPPTYPE_MESSAGE);
    extension->is_repeated = false;
    extension->message_value = prototype.New();
  } else {
    GOOGLE_DCHECK_TYPE(*extension, OPTIONAL, MESSAGE);
  }
  extension->is_cleared = false;
  return extension->message_value;
}

// Defined in extension_set_heavy.cc.
// MessageLite* ExtensionSet::MutableMessage(int number, FieldType type,
//                                           const Descriptor* message_type,
//                                           MessageFactory* factory)

const MessageLite& ExtensionSet::GetRepeatedMessage(
    int number, int index) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty).";
  GOOGLE_DCHECK_TYPE(iter->second, REPEATED, MESSAGE);
  return iter->second.repeated_message_value->Get(index);
}

MessageLite* ExtensionSet::MutableRepeatedMessage(int number, int index) {
  map<int, Extension>::iterator iter = extensions_.find(number);
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty).";
  GOOGLE_DCHECK_TYPE(iter->second, REPEATED, MESSAGE);
  return iter->second.repeated_message_value->Mutable(index);
}

MessageLite* ExtensionSet::AddMessage(int number, FieldType type,
                                      const MessageLite& prototype) {
  Extension* extension;
  if (MaybeNewExtension(number, &extension)) {
    extension->type = type;
    GOOGLE_DCHECK_EQ(cpp_type(extension->type), WireFormatLite::CPPTYPE_MESSAGE);
    extension->is_repeated = true;
    extension->repeated_message_value =
      new RepeatedPtrField<MessageLite>();
  } else {
    GOOGLE_DCHECK_TYPE(*extension, REPEATED, MESSAGE);
  }

  // RepeatedPtrField<MessageLite> does not know how to Add() since it cannot
  // allocate an abstract object, so we have to be tricky.
  MessageLite* result = extension->repeated_message_value
      ->AddFromCleared<internal::GenericTypeHandler<MessageLite> >();
  if (result == NULL) {
    result = prototype.New();
    extension->repeated_message_value->AddAllocated(result);
  }
  return result;
}

// Defined in extension_set_heavy.cc.
// MessageLite* ExtensionSet::AddMessage(int number, FieldType type,
//                                       const Descriptor* message_type,
//                                       MessageFactory* factory)

#undef GOOGLE_DCHECK_TYPE

void ExtensionSet::RemoveLast(int number) {
  map<int, Extension>::iterator iter = extensions_.find(number);
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty).";

  Extension* extension = &iter->second;
  GOOGLE_DCHECK(extension->is_repeated);

  switch(cpp_type(extension->type)) {
    case WireFormatLite::CPPTYPE_INT32:
      extension->repeated_int32_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_INT64:
      extension->repeated_int64_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_UINT32:
      extension->repeated_uint32_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_UINT64:
      extension->repeated_uint64_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_FLOAT:
      extension->repeated_float_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_DOUBLE:
      extension->repeated_double_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_BOOL:
      extension->repeated_bool_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_ENUM:
      extension->repeated_enum_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_STRING:
      extension->repeated_string_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_MESSAGE:
      extension->repeated_message_value->RemoveLast();
      break;
  }
}

void ExtensionSet::SwapElements(int number, int index1, int index2) {
  map<int, Extension>::iterator iter = extensions_.find(number);
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty).";

  Extension* extension = &iter->second;
  GOOGLE_DCHECK(extension->is_repeated);

  switch(cpp_type(extension->type)) {
    case WireFormatLite::CPPTYPE_INT32:
      extension->repeated_int32_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_INT64:
      extension->repeated_int64_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_UINT32:
      extension->repeated_uint32_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_UINT64:
      extension->repeated_uint64_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_FLOAT:
      extension->repeated_float_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_DOUBLE:
      extension->repeated_double_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_BOOL:
      extension->repeated_bool_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_ENUM:
      extension->repeated_enum_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_STRING:
      extension->repeated_string_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_MESSAGE:
      extension->repeated_message_value->SwapElements(index1, index2);
      break;
  }
}

// ===================================================================

void ExtensionSet::Clear() {
  for (map<int, Extension>::iterator iter = extensions_.begin();
       iter != extensions_.end(); ++iter) {
    iter->second.Clear();
  }
}

void ExtensionSet::MergeFrom(const ExtensionSet& other) {
  for (map<int, Extension>::const_iterator iter = other.extensions_.begin();
       iter != other.extensions_.end(); ++iter) {
    const Extension& other_extension = iter->second;

    if (other_extension.is_repeated) {
      Extension* extension;
      bool is_new = MaybeNewExtension(iter->first, &extension);
      if (is_new) {
        // Extension did not already exist in set.
        extension->type = other_extension.type;
        extension->is_repeated = true;
      } else {
        GOOGLE_DCHECK_EQ(extension->type, other_extension.type);
        GOOGLE_DCHECK(extension->is_repeated);
      }

      switch (cpp_type(other_extension.type)) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE, REPEATED_TYPE)             \
        case WireFormatLite::CPPTYPE_##UPPERCASE:                    \
          if (is_new) {                                              \
            extension->repeated_##LOWERCASE##_value =                \
              new REPEATED_TYPE;                                     \
          }                                                          \
          extension->repeated_##LOWERCASE##_value->MergeFrom(        \
            *other_extension.repeated_##LOWERCASE##_value);          \
          break;

        HANDLE_TYPE(  INT32,   int32, RepeatedField   <  int32>);
        HANDLE_TYPE(  INT64,   int64, RepeatedField   <  int64>);
        HANDLE_TYPE( UINT32,  uint32, RepeatedField   < uint32>);
        HANDLE_TYPE( UINT64,  uint64, RepeatedField   < uint64>);
        HANDLE_TYPE(  FLOAT,   float, RepeatedField   <  float>);
        HANDLE_TYPE( DOUBLE,  double, RepeatedField   < double>);
        HANDLE_TYPE(   BOOL,    bool, RepeatedField   <   bool>);
        HANDLE_TYPE(   ENUM,    enum, RepeatedField   <    int>);
        HANDLE_TYPE( STRING,  string, RepeatedPtrField< string>);
#undef HANDLE_TYPE

        case WireFormatLite::CPPTYPE_MESSAGE:
          if (is_new) {
            extension->repeated_message_value =
              new RepeatedPtrField<MessageLite>();
          }
          // We can't call RepeatedPtrField<MessageLite>::MergeFrom() because
          // it would attempt to allocate new objects.
          RepeatedPtrField<MessageLite>* other_repeated_message =
              other_extension.repeated_message_value;
          for (int i = 0; i < other_repeated_message->size(); i++) {
            const MessageLite& other_message = other_repeated_message->Get(i);
            MessageLite* target = extension->repeated_message_value
                     ->AddFromCleared<GenericTypeHandler<MessageLite> >();
            if (target == NULL) {
              target = other_message.New();
              extension->repeated_message_value->AddAllocated(target);
            }
            target->CheckTypeAndMergeFrom(other_message);
          }
          break;
      }
    } else {
      if (!other_extension.is_cleared) {
        switch (cpp_type(other_extension.type)) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE, CAMELCASE)                         \
          case WireFormatLite::CPPTYPE_##UPPERCASE:                          \
            Set##CAMELCASE(iter->first, other_extension.type,                \
                           other_extension.LOWERCASE##_value);               \
            break;

          HANDLE_TYPE( INT32,  int32,  Int32);
          HANDLE_TYPE( INT64,  int64,  Int64);
          HANDLE_TYPE(UINT32, uint32, UInt32);
          HANDLE_TYPE(UINT64, uint64, UInt64);
          HANDLE_TYPE( FLOAT,  float,  Float);
          HANDLE_TYPE(DOUBLE, double, Double);
          HANDLE_TYPE(  BOOL,   bool,   Bool);
          HANDLE_TYPE(  ENUM,   enum,   Enum);
#undef HANDLE_TYPE
          case WireFormatLite::CPPTYPE_STRING:
            SetString(iter->first, other_extension.type,
                      *other_extension.string_value);
            break;
          case WireFormatLite::CPPTYPE_MESSAGE:
            MutableMessage(iter->first, other_extension.type,
                           *other_extension.message_value)
              ->CheckTypeAndMergeFrom(*other_extension.message_value);
            break;
        }
      }
    }
  }
}

void ExtensionSet::Swap(ExtensionSet* x) {
  extensions_.swap(x->extensions_);
}

bool ExtensionSet::IsInitialized() const {
  // Extensions are never required.  However, we need to check that all
  // embedded messages are initialized.
  for (map<int, Extension>::const_iterator iter = extensions_.begin();
       iter != extensions_.end(); ++iter) {
    const Extension& extension = iter->second;
    if (cpp_type(extension.type) == WireFormatLite::CPPTYPE_MESSAGE) {
      if (extension.is_repeated) {
        for (int i = 0; i < extension.repeated_message_value->size(); i++) {
          if (!extension.repeated_message_value->Get(i).IsInitialized()) {
            return false;
          }
        }
      } else {
        if (!extension.is_cleared) {
          if (!extension.message_value->IsInitialized()) return false;
        }
      }
    }
  }

  return true;
}

bool ExtensionSet::ParseField(uint32 tag, io::CodedInputStream* input,
                              const MessageLite* containing_type,
                              FieldSkipper* field_skipper) {
  int number = WireFormatLite::GetTagFieldNumber(tag);
  WireFormatLite::WireType wire_type = WireFormatLite::GetTagWireType(tag);

  const ExtensionInfo* extension =
      FindRegisteredExtension(containing_type, number);

  bool is_unknown;
  if (extension == NULL) {
    is_unknown = true;
  } else if (extension->is_packed) {
    is_unknown = (wire_type != WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
  } else {
    WireFormatLite::WireType expected_wire_type =
        WireFormatLite::WireTypeForFieldType(real_type(extension->type));
    is_unknown = (wire_type != expected_wire_type);
  }

  if (is_unknown) {
    field_skipper->SkipField(input, tag);
  } else if (extension->is_packed) {
    uint32 size;
    if (!input->ReadVarint32(&size)) return false;
    io::CodedInputStream::Limit limit = input->PushLimit(size);

    switch (extension->type) {
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, CPP_CAMELCASE, CPP_LOWERCASE)        \
      case WireFormatLite::TYPE_##UPPERCASE:                                   \
        while (input->BytesUntilLimit() > 0) {                                 \
          CPP_LOWERCASE value;                                                 \
          if (!WireFormatLite::Read##CAMELCASE(input, &value)) return false;   \
          Add##CPP_CAMELCASE(number, WireFormatLite::TYPE_##UPPERCASE,         \
                             true, value);                                     \
        }                                                                      \
        break

      HANDLE_TYPE(   INT32,    Int32,   Int32,   int32);
      HANDLE_TYPE(   INT64,    Int64,   Int64,   int64);
      HANDLE_TYPE(  UINT32,   UInt32,  UInt32,  uint32);
      HANDLE_TYPE(  UINT64,   UInt64,  UInt64,  uint64);
      HANDLE_TYPE(  SINT32,   SInt32,   Int32,   int32);
      HANDLE_TYPE(  SINT64,   SInt64,   Int64,   int64);
      HANDLE_TYPE( FIXED32,  Fixed32,  UInt32,  uint32);
      HANDLE_TYPE( FIXED64,  Fixed64,  UInt64,  uint64);
      HANDLE_TYPE(SFIXED32, SFixed32,   Int32,   int32);
      HANDLE_TYPE(SFIXED64, SFixed64,   Int64,   int64);
      HANDLE_TYPE(   FLOAT,    Float,   Float,   float);
      HANDLE_TYPE(  DOUBLE,   Double,  Double,  double);
      HANDLE_TYPE(    BOOL,     Bool,    Bool,    bool);
#undef HANDLE_TYPE

      case WireFormatLite::TYPE_ENUM:
        while (input->BytesUntilLimit() > 0) {
          int value;
          if (!WireFormatLite::ReadEnum(input, &value)) return false;
          if (extension->enum_is_valid(value)) {
            AddEnum(number, WireFormatLite::TYPE_ENUM, true, value);
          }
        }
        break;

      case WireFormatLite::TYPE_STRING:
      case WireFormatLite::TYPE_BYTES:
      case WireFormatLite::TYPE_GROUP:
      case WireFormatLite::TYPE_MESSAGE:
        GOOGLE_LOG(FATAL) << "Non-primitive types can't be packed.";
        break;
    }

    input->PopLimit(limit);
  } else {
    switch (extension->type) {
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, CPP_CAMELCASE, CPP_LOWERCASE)        \
      case WireFormatLite::TYPE_##UPPERCASE: {                                 \
        CPP_LOWERCASE value;                                                   \
        if (!WireFormatLite::Read##CAMELCASE(input, &value)) return false;     \
        if (extension->is_repeated) {                                          \
          Add##CPP_CAMELCASE(number, WireFormatLite::TYPE_##UPPERCASE,         \
                             false, value);                                    \
        } else {                                                               \
          Set##CPP_CAMELCASE(number, WireFormatLite::TYPE_##UPPERCASE, value); \
        }                                                                      \
      } break

      HANDLE_TYPE(   INT32,    Int32,   Int32,   int32);
      HANDLE_TYPE(   INT64,    Int64,   Int64,   int64);
      HANDLE_TYPE(  UINT32,   UInt32,  UInt32,  uint32);
      HANDLE_TYPE(  UINT64,   UInt64,  UInt64,  uint64);
      HANDLE_TYPE(  SINT32,   SInt32,   Int32,   int32);
      HANDLE_TYPE(  SINT64,   SInt64,   Int64,   int64);
      HANDLE_TYPE( FIXED32,  Fixed32,  UInt32,  uint32);
      HANDLE_TYPE( FIXED64,  Fixed64,  UInt64,  uint64);
      HANDLE_TYPE(SFIXED32, SFixed32,   Int32,   int32);
      HANDLE_TYPE(SFIXED64, SFixed64,   Int64,   int64);
      HANDLE_TYPE(   FLOAT,    Float,   Float,   float);
      HANDLE_TYPE(  DOUBLE,   Double,  Double,  double);
      HANDLE_TYPE(    BOOL,     Bool,    Bool,    bool);
#undef HANDLE_TYPE

      case WireFormatLite::TYPE_ENUM: {
        int value;
        if (!WireFormatLite::ReadEnum(input, &value)) return false;

        if (!extension->enum_is_valid(value)) {
          // Invalid value.  Treat as unknown.
          field_skipper->SkipUnknownEnum(number, value);
        } else if (extension->is_repeated) {
          AddEnum(number, WireFormatLite::TYPE_ENUM, false, value);
        } else {
          SetEnum(number, WireFormatLite::TYPE_ENUM, value);
        }
        break;
      }

      case WireFormatLite::TYPE_STRING:  {
        string* value = extension->is_repeated ?
          AddString(number, WireFormatLite::TYPE_STRING) :
          MutableString(number, WireFormatLite::TYPE_STRING);
        if (!WireFormatLite::ReadString(input, value)) return false;
        break;
      }

      case WireFormatLite::TYPE_BYTES:  {
        string* value = extension->is_repeated ?
          AddString(number, WireFormatLite::TYPE_STRING) :
          MutableString(number, WireFormatLite::TYPE_STRING);
        if (!WireFormatLite::ReadBytes(input, value)) return false;
        break;
      }

      case WireFormatLite::TYPE_GROUP: {
        MessageLite* value = extension->is_repeated ?
            AddMessage(number, WireFormatLite::TYPE_GROUP,
                       *extension->message_prototype) :
            MutableMessage(number, WireFormatLite::TYPE_GROUP,
                           *extension->message_prototype);
        if (!WireFormatLite::ReadGroup(number, input, value)) return false;
        break;
      }

      case WireFormatLite::TYPE_MESSAGE: {
        MessageLite* value = extension->is_repeated ?
            AddMessage(number, WireFormatLite::TYPE_MESSAGE,
                       *extension->message_prototype) :
            MutableMessage(number, WireFormatLite::TYPE_MESSAGE,
                           *extension->message_prototype);
        if (!WireFormatLite::ReadMessage(input, value)) return false;
        break;
      }
    }
  }

  return true;
}

bool ExtensionSet::ParseField(uint32 tag, io::CodedInputStream* input,
                              const MessageLite* containing_type) {
  FieldSkipper skipper;
  return ParseField(tag, input, containing_type, &skipper);
}

// Defined in extension_set_heavy.cc.
// bool ExtensionSet::ParseField(uint32 tag, io::CodedInputStream* input,
//                               const MessageLite* containing_type,
//                               UnknownFieldSet* unknown_fields)

bool ExtensionSet::ParseMessageSet(io::CodedInputStream* input,
                                   const MessageLite* containing_type,
                                   FieldSkipper* field_skipper) {
  while (true) {
    uint32 tag = input->ReadTag();
    switch (tag) {
      case 0:
        return true;
      case WireFormatLite::kMessageSetItemStartTag:
        if (!ParseMessageSetItem(input, containing_type, field_skipper)) {
          return false;
        }
        break;
      default:
        if (!ParseField(tag, input, containing_type, field_skipper)) {
          return false;
        }
        break;
    }
  }
}

bool ExtensionSet::ParseMessageSet(io::CodedInputStream* input,
                                   const MessageLite* containing_type) {
  FieldSkipper skipper;
  return ParseMessageSet(input, containing_type, &skipper);
}

// Defined in extension_set_heavy.cc.
// bool ExtensionSet::ParseMessageSet(io::CodedInputStream* input,
//                                    const MessageLite* containing_type,
//                                    UnknownFieldSet* unknown_fields);

bool ExtensionSet::ParseMessageSetItem(io::CodedInputStream* input,
                                       const MessageLite* containing_type,
                                       FieldSkipper* field_skipper) {
  // TODO(kenton):  It would be nice to share code between this and
  // WireFormatLite::ParseAndMergeMessageSetItem(), but I think the
  // differences would be hard to factor out.

  // This method parses a group which should contain two fields:
  //   required int32 type_id = 2;
  //   required data message = 3;

  // Once we see a type_id, we'll construct a fake tag for this extension
  // which is the tag it would have had under the proto2 extensions wire
  // format.
  uint32 fake_tag = 0;

  // If we see message data before the type_id, we'll append it to this so
  // we can parse it later.  This will probably never happen in practice,
  // as no MessageSet encoder I know of writes the message before the type ID.
  // But, it's technically valid so we should allow it.
  // TODO(kenton):  Use a Cord instead?  Do I care?
  string message_data;

  while (true) {
    uint32 tag = input->ReadTag();
    if (tag == 0) return false;

    switch (tag) {
      case WireFormatLite::kMessageSetTypeIdTag: {
        uint32 type_id;
        if (!input->ReadVarint32(&type_id)) return false;
        fake_tag = WireFormatLite::MakeTag(type_id,
            WireFormatLite::WIRETYPE_LENGTH_DELIMITED);

        if (!message_data.empty()) {
          // We saw some message data before the type_id.  Have to parse it
          // now.
          io::CodedInputStream sub_input(
              reinterpret_cast<const uint8*>(message_data.data()),
              message_data.size());
          if (!ParseField(fake_tag, &sub_input,
                          containing_type, field_skipper)) {
            return false;
          }
          message_data.clear();
        }

        break;
      }

      case WireFormatLite::kMessageSetMessageTag: {
        if (fake_tag == 0) {
          // We haven't seen a type_id yet.  Append this data to message_data.
          string temp;
          uint32 length;
          if (!input->ReadVarint32(&length)) return false;
          if (!input->ReadString(&temp, length)) return false;
          message_data.append(temp);
        } else {
          // Already saw type_id, so we can parse this directly.
          if (!ParseField(fake_tag, input,
                          containing_type, field_skipper)) {
            return false;
          }
        }

        break;
      }

      case WireFormatLite::kMessageSetItemEndTag: {
        return true;
      }

      default: {
        if (!field_skipper->SkipField(input, tag)) return false;
      }
    }
  }
}

void ExtensionSet::SerializeWithCachedSizes(
    int start_field_number, int end_field_number,
    io::CodedOutputStream* output) const {
  map<int, Extension>::const_iterator iter;
  for (iter = extensions_.lower_bound(start_field_number);
       iter != extensions_.end() && iter->first < end_field_number;
       ++iter) {
    iter->second.SerializeFieldWithCachedSizes(iter->first, output);
  }
}

uint8* ExtensionSet::SerializeWithCachedSizesToArray(
    int start_field_number, int end_field_number,
    uint8* target) const {
  // For now, just create an array output stream around the target and dispatch
  // to SerializeWithCachedSizes(). Give the array output stream kint32max
  // bytes; we will certainly write less than that. It is up to the caller to
  // ensure that the buffer has sufficient space.
  int written_bytes;
  {
    io::ArrayOutputStream array_stream(target, kint32max);
    io::CodedOutputStream output_stream(&array_stream);
    SerializeWithCachedSizes(start_field_number,
                             end_field_number,
                             &output_stream);
    written_bytes = output_stream.ByteCount();
    GOOGLE_DCHECK(!output_stream.HadError());
  }
  return target + written_bytes;
}

void ExtensionSet::SerializeMessageSetWithCachedSizes(
    io::CodedOutputStream* output) const {
  map<int, Extension>::const_iterator iter;
  for (iter = extensions_.begin(); iter != extensions_.end(); ++iter) {
    iter->second.SerializeMessageSetItemWithCachedSizes(iter->first, output);
  }
}

uint8* ExtensionSet::SerializeMessageSetWithCachedSizesToArray(
    uint8* target) const {
  // For now, just create an array output stream around the target and dispatch
  // to SerializeWithCachedSizes(). Give the array output stream kint32max
  // bytes; we will certainly write less than that. It is up to the caller to
  // ensure that the buffer has sufficient space.
  int written_bytes;
  {
    io::ArrayOutputStream array_stream(target, kint32max);
    io::CodedOutputStream output_stream(&array_stream);
    SerializeMessageSetWithCachedSizes(&output_stream);
    written_bytes = output_stream.ByteCount();
    GOOGLE_DCHECK(!output_stream.HadError());
  }
  return target + written_bytes;
}

int ExtensionSet::ByteSize() const {
  int total_size = 0;

  for (map<int, Extension>::const_iterator iter = extensions_.begin();
       iter != extensions_.end(); ++iter) {
    total_size += iter->second.ByteSize(iter->first);
  }

  return total_size;
}

int ExtensionSet::MessageSetByteSize() const {
  int total_size = 0;

  for (map<int, Extension>::const_iterator iter = extensions_.begin();
       iter != extensions_.end(); ++iter) {
    total_size += iter->second.MessageSetItemByteSize(iter->first);
  }

  return total_size;
}

// Defined in extension_set_heavy.cc.
// int ExtensionSet::SpaceUsedExcludingSelf() const

bool ExtensionSet::MaybeNewExtension(int number, Extension** result) {
  pair<map<int, Extension>::iterator, bool> insert_result =
      extensions_.insert(make_pair(number, Extension()));
  *result = &insert_result.first->second;
  return insert_result.second;
}

// ===================================================================
// Methods of ExtensionSet::Extension

void ExtensionSet::Extension::Clear() {
  if (is_repeated) {
    switch (cpp_type(type)) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)                          \
      case WireFormatLite::CPPTYPE_##UPPERCASE:                    \
        repeated_##LOWERCASE##_value->Clear();                     \
        break

      HANDLE_TYPE(  INT32,   int32);
      HANDLE_TYPE(  INT64,   int64);
      HANDLE_TYPE( UINT32,  uint32);
      HANDLE_TYPE( UINT64,  uint64);
      HANDLE_TYPE(  FLOAT,   float);
      HANDLE_TYPE( DOUBLE,  double);
      HANDLE_TYPE(   BOOL,    bool);
      HANDLE_TYPE(   ENUM,    enum);
      HANDLE_TYPE( STRING,  string);
      HANDLE_TYPE(MESSAGE, message);
#undef HANDLE_TYPE
    }
  } else {
    if (!is_cleared) {
      switch (cpp_type(type)) {
        case WireFormatLite::CPPTYPE_STRING:
          string_value->clear();
          break;
        case WireFormatLite::CPPTYPE_MESSAGE:
          message_value->Clear();
          break;
        default:
          // No need to do anything.  Get*() will return the default value
          // as long as is_cleared is true and Set*() will overwrite the
          // previous value.
          break;
      }

      is_cleared = true;
    }
  }
}

void ExtensionSet::Extension::SerializeFieldWithCachedSizes(
    int number,
    io::CodedOutputStream* output) const {
  if (is_repeated) {
    if (is_packed) {
      if (cached_size == 0) return;

      WireFormatLite::WriteTag(number,
          WireFormatLite::WIRETYPE_LENGTH_DELIMITED, output);
      output->WriteVarint32(cached_size);

      switch (real_type(type)) {
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)                        \
        case WireFormatLite::TYPE_##UPPERCASE:                              \
          for (int i = 0; i < repeated_##LOWERCASE##_value->size(); i++) {  \
            WireFormatLite::Write##CAMELCASE##NoTag(                        \
              repeated_##LOWERCASE##_value->Get(i), output);                \
          }                                                                 \
          break

        HANDLE_TYPE(   INT32,    Int32,   int32);
        HANDLE_TYPE(   INT64,    Int64,   int64);
        HANDLE_TYPE(  UINT32,   UInt32,  uint32);
        HANDLE_TYPE(  UINT64,   UInt64,  uint64);
        HANDLE_TYPE(  SINT32,   SInt32,   int32);
        HANDLE_TYPE(  SINT64,   SInt64,   int64);
        HANDLE_TYPE( FIXED32,  Fixed32,  uint32);
        HANDLE_TYPE( FIXED64,  Fixed64,  uint64);
        HANDLE_TYPE(SFIXED32, SFixed32,   int32);
        HANDLE_TYPE(SFIXED64, SFixed64,   int64);
        HANDLE_TYPE(   FLOAT,    Float,   float);
        HANDLE_TYPE(  DOUBLE,   Double,  double);
        HANDLE_TYPE(    BOOL,     Bool,    bool);
        HANDLE_TYPE(    ENUM,     Enum,    enum);
#undef HANDLE_TYPE

        case WireFormatLite::TYPE_STRING:
        case WireFormatLite::TYPE_BYTES:
        case WireFormatLite::TYPE_GROUP:
        case WireFormatLite::TYPE_MESSAGE:
          GOOGLE_LOG(FATAL) << "Non-primitive types can't be packed.";
          break;
      }
    } else {
      switch (real_type(type)) {
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)                        \
        case WireFormatLite::TYPE_##UPPERCASE:                              \
          for (int i = 0; i < repeated_##LOWERCASE##_value->size(); i++) {  \
            WireFormatLite::Write##CAMELCASE(number,                        \
              repeated_##LOWERCASE##_value->Get(i), output);                \
          }                                                                 \
          break

        HANDLE_TYPE(   INT32,    Int32,   int32);
        HANDLE_TYPE(   INT64,    Int64,   int64);
        HANDLE_TYPE(  UINT32,   UInt32,  uint32);
        HANDLE_TYPE(  UINT64,   UInt64,  uint64);
        HANDLE_TYPE(  SINT32,   SInt32,   int32);
        HANDLE_TYPE(  SINT64,   SInt64,   int64);
        HANDLE_TYPE( FIXED32,  Fixed32,  uint32);
        HANDLE_TYPE( FIXED64,  Fixed64,  uint64);
        HANDLE_TYPE(SFIXED32, SFixed32,   int32);
        HANDLE_TYPE(SFIXED64, SFixed64,   int64);
        HANDLE_TYPE(   FLOAT,    Float,   float);
        HANDLE_TYPE(  DOUBLE,   Double,  double);
        HANDLE_TYPE(    BOOL,     Bool,    bool);
        HANDLE_TYPE(  STRING,   String,  string);
        HANDLE_TYPE(   BYTES,    Bytes,  string);
        HANDLE_TYPE(    ENUM,     Enum,    enum);
        HANDLE_TYPE(   GROUP,    Group, message);
        HANDLE_TYPE( MESSAGE,  Message, message);
#undef HANDLE_TYPE
      }
    }
  } else if (!is_cleared) {
    switch (real_type(type)) {
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, VALUE)                 \
      case WireFormatLite::TYPE_##UPPERCASE:                     \
        WireFormatLite::Write##CAMELCASE(number, VALUE, output); \
        break

      HANDLE_TYPE(   INT32,    Int32,    int32_value);
      HANDLE_TYPE(   INT64,    Int64,    int64_value);
      HANDLE_TYPE(  UINT32,   UInt32,   uint32_value);
      HANDLE_TYPE(  UINT64,   UInt64,   uint64_value);
      HANDLE_TYPE(  SINT32,   SInt32,    int32_value);
      HANDLE_TYPE(  SINT64,   SInt64,    int64_value);
      HANDLE_TYPE( FIXED32,  Fixed32,   uint32_value);
      HANDLE_TYPE( FIXED64,  Fixed64,   uint64_value);
      HANDLE_TYPE(SFIXED32, SFixed32,    int32_value);
      HANDLE_TYPE(SFIXED64, SFixed64,    int64_value);
      HANDLE_TYPE(   FLOAT,    Float,    float_value);
      HANDLE_TYPE(  DOUBLE,   Double,   double_value);
      HANDLE_TYPE(    BOOL,     Bool,     bool_value);
      HANDLE_TYPE(  STRING,   String,  *string_value);
      HANDLE_TYPE(   BYTES,    Bytes,  *string_value);
      HANDLE_TYPE(    ENUM,     Enum,     enum_value);
      HANDLE_TYPE(   GROUP,    Group, *message_value);
      HANDLE_TYPE( MESSAGE,  Message, *message_value);
#undef HANDLE_TYPE
    }
  }
}

void ExtensionSet::Extension::SerializeMessageSetItemWithCachedSizes(
    int number,
    io::CodedOutputStream* output) const {
  if (type != WireFormatLite::TYPE_MESSAGE || is_repeated) {
    // Not a valid MessageSet extension, but serialize it the normal way.
    SerializeFieldWithCachedSizes(number, output);
    return;
  }

  if (is_cleared) return;

  // Start group.
  output->WriteVarint32(WireFormatLite::kMessageSetItemStartTag);

  // Write type ID.
  output->WriteVarint32(WireFormatLite::kMessageSetTypeIdTag);
  output->WriteVarint32(number);

  // Write message.
  output->WriteVarint32(WireFormatLite::kMessageSetMessageTag);

  output->WriteVarint32(message_value->GetCachedSize());
  message_value->SerializeWithCachedSizes(output);

  // End group.
  output->WriteVarint32(WireFormatLite::kMessageSetItemEndTag);
}

int ExtensionSet::Extension::ByteSize(int number) const {
  int result = 0;

  if (is_repeated) {
    if (is_packed) {
      switch (real_type(type)) {
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)                        \
        case WireFormatLite::TYPE_##UPPERCASE:                              \
          for (int i = 0; i < repeated_##LOWERCASE##_value->size(); i++) {  \
            result += WireFormatLite::CAMELCASE##Size(                      \
              repeated_##LOWERCASE##_value->Get(i));                        \
          }                                                                 \
          break

        HANDLE_TYPE(   INT32,    Int32,   int32);
        HANDLE_TYPE(   INT64,    Int64,   int64);
        HANDLE_TYPE(  UINT32,   UInt32,  uint32);
        HANDLE_TYPE(  UINT64,   UInt64,  uint64);
        HANDLE_TYPE(  SINT32,   SInt32,   int32);
        HANDLE_TYPE(  SINT64,   SInt64,   int64);
        HANDLE_TYPE(    ENUM,     Enum,    enum);
#undef HANDLE_TYPE

        // Stuff with fixed size.
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)                        \
        case WireFormatLite::TYPE_##UPPERCASE:                              \
          result += WireFormatLite::k##CAMELCASE##Size *                    \
                    repeated_##LOWERCASE##_value->size();                   \
          break
        HANDLE_TYPE( FIXED32,  Fixed32, uint32);
        HANDLE_TYPE( FIXED64,  Fixed64, uint64);
        HANDLE_TYPE(SFIXED32, SFixed32,  int32);
        HANDLE_TYPE(SFIXED64, SFixed64,  int64);
        HANDLE_TYPE(   FLOAT,    Float,  float);
        HANDLE_TYPE(  DOUBLE,   Double, double);
        HANDLE_TYPE(    BOOL,     Bool,   bool);
#undef HANDLE_TYPE

        case WireFormatLite::TYPE_STRING:
        case WireFormatLite::TYPE_BYTES:
        case WireFormatLite::TYPE_GROUP:
        case WireFormatLite::TYPE_MESSAGE:
          GOOGLE_LOG(FATAL) << "Non-primitive types can't be packed.";
          break;
      }

      cached_size = result;
      if (result > 0) {
        result += io::CodedOutputStream::VarintSize32(result);
        result += io::CodedOutputStream::VarintSize32(
            WireFormatLite::MakeTag(number,
                WireFormatLite::WIRETYPE_LENGTH_DELIMITED));
      }
    } else {
      int tag_size = WireFormatLite::TagSize(number, real_type(type));

      switch (real_type(type)) {
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)                        \
        case WireFormatLite::TYPE_##UPPERCASE:                              \
          result += tag_size * repeated_##LOWERCASE##_value->size();        \
          for (int i = 0; i < repeated_##LOWERCASE##_value->size(); i++) {  \
            result += WireFormatLite::CAMELCASE##Size(                      \
              repeated_##LOWERCASE##_value->Get(i));                        \
          }                                                                 \
          break

        HANDLE_TYPE(   INT32,    Int32,   int32);
        HANDLE_TYPE(   INT64,    Int64,   int64);
        HANDLE_TYPE(  UINT32,   UInt32,  uint32);
        HANDLE_TYPE(  UINT64,   UInt64,  uint64);
        HANDLE_TYPE(  SINT32,   SInt32,   int32);
        HANDLE_TYPE(  SINT64,   SInt64,   int64);
        HANDLE_TYPE(  STRING,   String,  string);
        HANDLE_TYPE(   BYTES,    Bytes,  string);
        HANDLE_TYPE(    ENUM,     Enum,    enum);
        HANDLE_TYPE(   GROUP,    Group, message);
        HANDLE_TYPE( MESSAGE,  Message, message);
#undef HANDLE_TYPE

        // Stuff with fixed size.
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)                        \
        case WireFormatLite::TYPE_##UPPERCASE:                              \
          result += (tag_size + WireFormatLite::k##CAMELCASE##Size) *       \
                    repeated_##LOWERCASE##_value->size();                   \
          break
        HANDLE_TYPE( FIXED32,  Fixed32, uint32);
        HANDLE_TYPE( FIXED64,  Fixed64, uint64);
        HANDLE_TYPE(SFIXED32, SFixed32,  int32);
        HANDLE_TYPE(SFIXED64, SFixed64,  int64);
        HANDLE_TYPE(   FLOAT,    Float,  float);
        HANDLE_TYPE(  DOUBLE,   Double, double);
        HANDLE_TYPE(    BOOL,     Bool,   bool);
#undef HANDLE_TYPE
      }
    }
  } else if (!is_cleared) {
    result += WireFormatLite::TagSize(number, real_type(type));
    switch (real_type(type)) {
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)                      \
      case WireFormatLite::TYPE_##UPPERCASE:                              \
        result += WireFormatLite::CAMELCASE##Size(LOWERCASE);             \
        break

      HANDLE_TYPE(   INT32,    Int32,    int32_value);
      HANDLE_TYPE(   INT64,    Int64,    int64_value);
      HANDLE_TYPE(  UINT32,   UInt32,   uint32_value);
      HANDLE_TYPE(  UINT64,   UInt64,   uint64_value);
      HANDLE_TYPE(  SINT32,   SInt32,    int32_value);
      HANDLE_TYPE(  SINT64,   SInt64,    int64_value);
      HANDLE_TYPE(  STRING,   String,  *string_value);
      HANDLE_TYPE(   BYTES,    Bytes,  *string_value);
      HANDLE_TYPE(    ENUM,     Enum,     enum_value);
      HANDLE_TYPE(   GROUP,    Group, *message_value);
      HANDLE_TYPE( MESSAGE,  Message, *message_value);
#undef HANDLE_TYPE

      // Stuff with fixed size.
#define HANDLE_TYPE(UPPERCASE, CAMELCASE)                                 \
      case WireFormatLite::TYPE_##UPPERCASE:                              \
        result += WireFormatLite::k##CAMELCASE##Size;                     \
        break
      HANDLE_TYPE( FIXED32,  Fixed32);
      HANDLE_TYPE( FIXED64,  Fixed64);
      HANDLE_TYPE(SFIXED32, SFixed32);
      HANDLE_TYPE(SFIXED64, SFixed64);
      HANDLE_TYPE(   FLOAT,    Float);
      HANDLE_TYPE(  DOUBLE,   Double);
      HANDLE_TYPE(    BOOL,     Bool);
#undef HANDLE_TYPE
    }
  }

  return result;
}

int ExtensionSet::Extension::MessageSetItemByteSize(int number) const {
  if (type != WireFormatLite::TYPE_MESSAGE || is_repeated) {
    // Not a valid MessageSet extension, but compute the byte size for it the
    // normal way.
    return ByteSize(number);
  }

  if (is_cleared) return 0;

  int our_size = WireFormatLite::kMessageSetItemTagsSize;

  // type_id
  our_size += io::CodedOutputStream::VarintSize32(number);

  // message
  int message_size = message_value->ByteSize();

  our_size += io::CodedOutputStream::VarintSize32(message_size);
  our_size += message_size;

  return our_size;
}

int ExtensionSet::Extension::GetSize() const {
  GOOGLE_DCHECK(is_repeated);
  switch (cpp_type(type)) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)                        \
    case WireFormatLite::CPPTYPE_##UPPERCASE:                    \
      return repeated_##LOWERCASE##_value->size()

    HANDLE_TYPE(  INT32,   int32);
    HANDLE_TYPE(  INT64,   int64);
    HANDLE_TYPE( UINT32,  uint32);
    HANDLE_TYPE( UINT64,  uint64);
    HANDLE_TYPE(  FLOAT,   float);
    HANDLE_TYPE( DOUBLE,  double);
    HANDLE_TYPE(   BOOL,    bool);
    HANDLE_TYPE(   ENUM,    enum);
    HANDLE_TYPE( STRING,  string);
    HANDLE_TYPE(MESSAGE, message);
#undef HANDLE_TYPE
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return 0;
}

void ExtensionSet::Extension::Free() {
  if (is_repeated) {
    switch (cpp_type(type)) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)                          \
      case WireFormatLite::CPPTYPE_##UPPERCASE:                    \
        delete repeated_##LOWERCASE##_value;                       \
        break

      HANDLE_TYPE(  INT32,   int32);
      HANDLE_TYPE(  INT64,   int64);
      HANDLE_TYPE( UINT32,  uint32);
      HANDLE_TYPE( UINT64,  uint64);
      HANDLE_TYPE(  FLOAT,   float);
      HANDLE_TYPE( DOUBLE,  double);
      HANDLE_TYPE(   BOOL,    bool);
      HANDLE_TYPE(   ENUM,    enum);
      HANDLE_TYPE( STRING,  string);
      HANDLE_TYPE(MESSAGE, message);
#undef HANDLE_TYPE
    }
  } else {
    switch (cpp_type(type)) {
      case WireFormatLite::CPPTYPE_STRING:
        delete string_value;
        break;
      case WireFormatLite::CPPTYPE_MESSAGE:
        delete message_value;
        break;
      default:
        break;
    }
  }
}

// Defined in extension_set_heavy.cc.
// int ExtensionSet::Extension::SpaceUsedExcludingSelf() const

}  // namespace internal
}  // namespace protobuf
}  // namespace google
