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

#include <google/protobuf/stubs/hash.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/repeated_field.h>

namespace google {
namespace protobuf {
namespace internal {

// -------------------------------------------------------------------
// Lookup functions

const FieldDescriptor*
ExtensionSet::FindKnownExtensionOrDie(int number) const {
  const FieldDescriptor* descriptor =
    descriptor_pool_->FindExtensionByNumber(*extendee_, number);
  if (descriptor == NULL) {
    // This extension doesn't exist, so we have to crash.  However, let's
    // try to provide an informative error message.
    if (descriptor_pool_ == DescriptorPool::generated_pool() &&
        message_factory_ == MessageFactory::generated_factory()) {
      // This is probably the ExtensionSet for a generated class.
      GOOGLE_LOG(FATAL) << ": No extension is registered for \""
                 << (*extendee_)->full_name() << "\" with number "
                 << number << ".  Perhaps you were trying to access it via "
                    "the Reflection interface, but you provided a "
                    "FieldDescriptor which did not come from a linked-in "
                    "message type?  This is not permitted; linkin-in message "
                    "types cannot use non-linked-in extensions.  Try "
                    "converting to a DynamicMessage first.";
    } else {
      // This is probably a DynamicMessage.
      GOOGLE_LOG(FATAL) << ": No extension is registered for \""
                 << (*extendee_)->full_name() << "\" with number "
                 << number << ".  If you were using a DynamicMessage, "
                    "remember that you are only allowed to access extensions "
                    "which are defined in the DescriptorPool which you passed "
                    "to DynamicMessageFactory's constructor.";
    }
  }
  return descriptor;
}

const Message*
ExtensionSet::GetPrototype(const Descriptor* message_type) const {
  return message_factory_->GetPrototype(message_type);
}

// ===================================================================
// Constructors and basic methods.

ExtensionSet::ExtensionSet(const Descriptor* const* extendee,
                           const DescriptorPool* pool,
                           MessageFactory* factory)
  : extendee_(extendee),
    descriptor_pool_(pool),
    message_factory_(factory) {
}

ExtensionSet::~ExtensionSet() {
  for (map<int, Extension>::iterator iter = extensions_.begin();
       iter != extensions_.end(); ++iter) {
    iter->second.Free();
  }
}

void ExtensionSet::AppendToList(vector<const FieldDescriptor*>* output) const {
  for (map<int, Extension>::const_iterator iter = extensions_.begin();
       iter != extensions_.end(); ++iter) {
    bool has = false;
    if (iter->second.descriptor->is_repeated()) {
      has = iter->second.GetSize() > 0;
    } else {
      has = !iter->second.is_cleared;
    }

    if (has) {
      output->push_back(iter->second.descriptor);
    }
  }
}

bool ExtensionSet::Has(int number) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  if (iter == extensions_.end()) return false;
  GOOGLE_DCHECK(!iter->second.descriptor->is_repeated());
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

#define GOOGLE_DCHECK_TYPE(DESCRIPTOR, LABEL, CPPTYPE)                           \
  GOOGLE_DCHECK_EQ(DESCRIPTOR->label(), FieldDescriptor::LABEL_##LABEL);         \
  GOOGLE_DCHECK_EQ(DESCRIPTOR->cpp_type(), FieldDescriptor::CPPTYPE_##CPPTYPE)

// -------------------------------------------------------------------
// Primitives

#define PRIMITIVE_ACCESSORS(UPPERCASE, LOWERCASE, CAMELCASE)                   \
                                                                               \
LOWERCASE ExtensionSet::Get##CAMELCASE(int number) const {                     \
  map<int, Extension>::const_iterator iter = extensions_.find(number);         \
  if (iter == extensions_.end()) {                                             \
    /* Not present.  Return the default value. */                              \
    const FieldDescriptor* descriptor = FindKnownExtensionOrDie(number);       \
    GOOGLE_DCHECK_TYPE(descriptor, OPTIONAL, UPPERCASE);                              \
    return descriptor->default_value_##LOWERCASE();                            \
  } else {                                                                     \
    GOOGLE_DCHECK_TYPE(iter->second.descriptor, OPTIONAL, UPPERCASE);                 \
    return iter->second.LOWERCASE##_value;                                     \
  }                                                                            \
}                                                                              \
                                                                               \
void ExtensionSet::Set##CAMELCASE(int number, LOWERCASE value) {               \
  Extension* extension = &extensions_[number];                                 \
  if (extension->descriptor == NULL) {                                         \
    /* Not previoulsy present.  Initialize it. */                              \
    const FieldDescriptor* descriptor = FindKnownExtensionOrDie(number);       \
    GOOGLE_DCHECK_TYPE(descriptor, OPTIONAL, UPPERCASE);                              \
    extension->descriptor = descriptor;                                        \
    extension->LOWERCASE##_value = descriptor->default_value_##LOWERCASE();    \
  } else {                                                                     \
    GOOGLE_DCHECK_TYPE(extension->descriptor, OPTIONAL, UPPERCASE);                   \
    extension->is_cleared = false;                                             \
  }                                                                            \
  extension->LOWERCASE##_value = value;                                        \
}                                                                              \
                                                                               \
LOWERCASE ExtensionSet::GetRepeated##CAMELCASE(int number, int index) const {  \
  map<int, Extension>::const_iterator iter = extensions_.find(number);         \
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty)."; \
  GOOGLE_DCHECK_TYPE(iter->second.descriptor, REPEATED, UPPERCASE);                   \
  return iter->second.repeated_##LOWERCASE##_value->Get(index);                \
}                                                                              \
                                                                               \
void ExtensionSet::SetRepeated##CAMELCASE(                                     \
    int number, int index, LOWERCASE value) {                                  \
  map<int, Extension>::iterator iter = extensions_.find(number);               \
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty)."; \
  GOOGLE_DCHECK_TYPE(iter->second.descriptor, REPEATED, UPPERCASE);                   \
  iter->second.repeated_##LOWERCASE##_value->Set(index, value);                \
}                                                                              \
                                                                               \
void ExtensionSet::Add##CAMELCASE(int number, LOWERCASE value) {               \
  Extension* extension = &extensions_[number];                                 \
  if (extension->descriptor == NULL) {                                         \
    /* Not previoulsy present.  Initialize it. */                              \
    const FieldDescriptor* descriptor = FindKnownExtensionOrDie(number);       \
    GOOGLE_DCHECK_TYPE(descriptor, REPEATED, UPPERCASE);                              \
    extension->repeated_##LOWERCASE##_value = new RepeatedField<LOWERCASE>();  \
    extension->descriptor = descriptor;                                        \
  } else {                                                                     \
    GOOGLE_DCHECK_TYPE(extension->descriptor, REPEATED, UPPERCASE);                   \
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

int ExtensionSet::GetEnum(int number) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  if (iter == extensions_.end()) {
    // Not present.  Return the default value.
    const FieldDescriptor* descriptor = FindKnownExtensionOrDie(number);
    GOOGLE_DCHECK_TYPE(descriptor, OPTIONAL, ENUM);
    return descriptor->default_value_enum()->number();
  } else {
    GOOGLE_DCHECK_TYPE(iter->second.descriptor, OPTIONAL, ENUM);
    return iter->second.enum_value;
  }
}

void ExtensionSet::SetEnum(int number, int value) {
  Extension* extension = &extensions_[number];
  if (extension->descriptor == NULL) {
    // Not previoulsy present.  Initialize it.
    const FieldDescriptor* descriptor = FindKnownExtensionOrDie(number);
    GOOGLE_DCHECK_TYPE(descriptor, OPTIONAL, ENUM);
    extension->descriptor = descriptor;
    extension->enum_value = descriptor->default_value_enum()->number();
  } else {
    GOOGLE_DCHECK_TYPE(extension->descriptor, OPTIONAL, ENUM);
    extension->is_cleared = false;
  }
  GOOGLE_DCHECK(extension->descriptor->enum_type()->FindValueByNumber(value) != NULL);
  extension->enum_value = value;
}

int ExtensionSet::GetRepeatedEnum(int number, int index) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty).";
  GOOGLE_DCHECK_TYPE(iter->second.descriptor, REPEATED, ENUM);
  return iter->second.repeated_enum_value->Get(index);
}

void ExtensionSet::SetRepeatedEnum(int number, int index, int value) {
  map<int, Extension>::iterator iter = extensions_.find(number);
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty).";
  GOOGLE_DCHECK_TYPE(iter->second.descriptor, REPEATED, ENUM);
  GOOGLE_DCHECK(iter->second.descriptor->enum_type()
             ->FindValueByNumber(value) != NULL);
  iter->second.repeated_enum_value->Set(index, value);
}

void ExtensionSet::AddEnum(int number, int value) {
  Extension* extension = &extensions_[number];
  if (extension->descriptor == NULL) {
    // Not previoulsy present.  Initialize it.
    const FieldDescriptor* descriptor = FindKnownExtensionOrDie(number);
    GOOGLE_DCHECK_TYPE(descriptor, REPEATED, ENUM);
    extension->repeated_enum_value = new RepeatedField<int>();
    extension->descriptor = descriptor;
  } else {
    GOOGLE_DCHECK_TYPE(extension->descriptor, REPEATED, ENUM);
  }
  GOOGLE_DCHECK(extension->descriptor->enum_type()->FindValueByNumber(value) != NULL);
  extension->repeated_enum_value->Add(value);
}

// -------------------------------------------------------------------
// Strings

const string& ExtensionSet::GetString(int number) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  if (iter == extensions_.end()) {
    // Not present.  Return the default value.
    const FieldDescriptor* descriptor = FindKnownExtensionOrDie(number);
    GOOGLE_DCHECK_TYPE(descriptor, OPTIONAL, STRING);
    return descriptor->default_value_string();
  } else {
    GOOGLE_DCHECK_TYPE(iter->second.descriptor, OPTIONAL, STRING);
    return *iter->second.string_value;
  }
}

string* ExtensionSet::MutableString(int number) {
  Extension* extension = &extensions_[number];
  if (extension->descriptor == NULL) {
    // Not previoulsy present.  Initialize it.
    const FieldDescriptor* descriptor = FindKnownExtensionOrDie(number);
    GOOGLE_DCHECK_TYPE(descriptor, OPTIONAL, STRING);
    extension->descriptor = descriptor;
    extension->string_value = new string;
  } else {
    GOOGLE_DCHECK_TYPE(extension->descriptor, OPTIONAL, STRING);
    extension->is_cleared = false;
  }
  return extension->string_value;
}

const string& ExtensionSet::GetRepeatedString(int number, int index) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty).";
  GOOGLE_DCHECK_TYPE(iter->second.descriptor, REPEATED, STRING);
  return iter->second.repeated_string_value->Get(index);
}

string* ExtensionSet::MutableRepeatedString(int number, int index) {
  map<int, Extension>::iterator iter = extensions_.find(number);
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty).";
  GOOGLE_DCHECK_TYPE(iter->second.descriptor, REPEATED, STRING);
  return iter->second.repeated_string_value->Mutable(index);
}

string* ExtensionSet::AddString(int number) {
  Extension* extension = &extensions_[number];
  if (extension->descriptor == NULL) {
    // Not previoulsy present.  Initialize it.
    const FieldDescriptor* descriptor = FindKnownExtensionOrDie(number);
    GOOGLE_DCHECK_TYPE(descriptor, REPEATED, STRING);
    extension->repeated_string_value = new RepeatedPtrField<string>();
    extension->descriptor = descriptor;
  } else {
    GOOGLE_DCHECK_TYPE(extension->descriptor, REPEATED, STRING);
  }
  return extension->repeated_string_value->Add();
}

// -------------------------------------------------------------------
// Messages

const Message& ExtensionSet::GetMessage(int number) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  if (iter == extensions_.end()) {
    // Not present.  Return the default value.
    const FieldDescriptor* descriptor = FindKnownExtensionOrDie(number);
    GOOGLE_DCHECK_TYPE(descriptor, OPTIONAL, MESSAGE);
    return *GetPrototype(descriptor->message_type());
  } else {
    GOOGLE_DCHECK_TYPE(iter->second.descriptor, OPTIONAL, MESSAGE);
    return *iter->second.message_value;
  }
}

Message* ExtensionSet::MutableMessage(int number) {
  Extension* extension = &extensions_[number];
  if (extension->descriptor == NULL) {
    // Not previoulsy present.  Initialize it.
    const FieldDescriptor* descriptor = FindKnownExtensionOrDie(number);
    GOOGLE_DCHECK_TYPE(descriptor, OPTIONAL, MESSAGE);
    extension->descriptor = descriptor;
    extension->message_value = GetPrototype(descriptor->message_type())->New();
  } else {
    GOOGLE_DCHECK_TYPE(extension->descriptor, OPTIONAL, MESSAGE);
    extension->is_cleared = false;
  }
  return extension->message_value;
}

const Message& ExtensionSet::GetRepeatedMessage(int number, int index) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty).";
  GOOGLE_DCHECK_TYPE(iter->second.descriptor, REPEATED, MESSAGE);
  return iter->second.repeated_message_value->Get(index);
}

Message* ExtensionSet::MutableRepeatedMessage(int number, int index) {
  map<int, Extension>::iterator iter = extensions_.find(number);
  GOOGLE_CHECK(iter != extensions_.end()) << "Index out-of-bounds (field is empty).";
  GOOGLE_DCHECK_TYPE(iter->second.descriptor, REPEATED, MESSAGE);
  return iter->second.repeated_message_value->Mutable(index);
}

Message* ExtensionSet::AddMessage(int number) {
  Extension* extension = &extensions_[number];
  if (extension->descriptor == NULL) {
    // Not previoulsy present.  Initialize it.
    const FieldDescriptor* descriptor = FindKnownExtensionOrDie(number);
    GOOGLE_DCHECK_TYPE(descriptor, REPEATED, MESSAGE);
    extension->repeated_message_value =
      new RepeatedPtrField<Message>(GetPrototype(descriptor->message_type()));
    extension->descriptor = descriptor;
  } else {
    GOOGLE_DCHECK_TYPE(extension->descriptor, REPEATED, MESSAGE);
  }
  return extension->repeated_message_value->Add();
}

#undef GOOGLE_DCHECK_TYPE

// ===================================================================

void ExtensionSet::Clear() {
  for (map<int, Extension>::iterator iter = extensions_.begin();
       iter != extensions_.end(); ++iter) {
    iter->second.Clear();
  }
}

namespace {

// A helper function for merging RepeatedFields...
// TODO(kenton):  Implement this as a method of RepeatedField?  Make generated
//   MergeFrom methods use it?

template <typename Type>
void MergeRepeatedFields(const RepeatedField<Type>& source,
                         RepeatedField<Type>* destination) {
  destination->Reserve(destination->size() + source.size());
  for (int i = 0; i < source.size(); i++) {
    destination->Add(source.Get(i));
  }
}

void MergeRepeatedFields(const RepeatedPtrField<string>& source,
                         RepeatedPtrField<string>* destination) {
  destination->Reserve(destination->size() + source.size());
  for (int i = 0; i < source.size(); i++) {
    destination->Add()->assign(source.Get(i));
  }
}

void MergeRepeatedFields(const RepeatedPtrField<Message>& source,
                         RepeatedPtrField<Message>* destination) {
  destination->Reserve(destination->size() + source.size());
  for (int i = 0; i < source.size(); i++) {
    destination->Add()->MergeFrom(source.Get(i));
  }
}

}  // namespace

void ExtensionSet::MergeFrom(const ExtensionSet& other) {
  GOOGLE_DCHECK_EQ(*extendee_, *other.extendee_);

  for (map<int, Extension>::const_iterator iter = other.extensions_.begin();
       iter != other.extensions_.end(); ++iter) {
    const FieldDescriptor* field = iter->second.descriptor;
    if (field->is_repeated()) {
      const Extension& other_extension = iter->second;
      Extension* extension = &extensions_[iter->first];
      switch (field->cpp_type()) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE, REPEATED_TYPE)             \
        case FieldDescriptor::CPPTYPE_##UPPERCASE:                   \
          if (extension->descriptor == NULL) {                       \
            extension->descriptor = field;                           \
            extension->repeated_##LOWERCASE##_value =                \
              new REPEATED_TYPE;                                     \
          }                                                          \
          MergeRepeatedFields(                                       \
            *other_extension.repeated_##LOWERCASE##_value,           \
            extension->repeated_##LOWERCASE##_value);                \
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

        case FieldDescriptor::CPPTYPE_MESSAGE:
          if (extension->descriptor == NULL) {
            extension->descriptor = field;
            extension->repeated_message_value = new RepeatedPtrField<Message>(
              other_extension.repeated_message_value->prototype());
          }
          MergeRepeatedFields(
            *other_extension.repeated_message_value,
            extension->repeated_message_value);
          break;
      }
    } else {
      if (!iter->second.is_cleared) {
        switch (field->cpp_type()) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE, CAMELCASE)                       \
          case FieldDescriptor::CPPTYPE_##UPPERCASE:                       \
            Set##CAMELCASE(iter->first, iter->second.LOWERCASE##_value);   \
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
          case FieldDescriptor::CPPTYPE_STRING:
            SetString(iter->first, *iter->second.string_value);
            break;
          case FieldDescriptor::CPPTYPE_MESSAGE:
            MutableMessage(iter->first)->MergeFrom(*iter->second.message_value);
            break;
        }
      }
    }
  }
}

bool ExtensionSet::IsInitialized() const {
  // Extensions are never requried.  However, we need to check that all
  // embedded messages are initialized.
  for (map<int, Extension>::const_iterator iter = extensions_.begin();
       iter != extensions_.end(); ++iter) {
    const Extension& extension = iter->second;
    if (extension.descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (extension.descriptor->is_repeated()) {
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
                              Message* message) {
  const FieldDescriptor* field =
    message->GetReflection()
           ->FindKnownExtensionByNumber(WireFormat::GetTagFieldNumber(tag));

  return WireFormat::ParseAndMergeField(tag, field, message, input);
}

bool ExtensionSet::SerializeWithCachedSizes(
    int start_field_number, int end_field_number,
    const Message& message,
    io::CodedOutputStream* output) const {
  map<int, Extension>::const_iterator iter;
  for (iter = extensions_.lower_bound(start_field_number);
       iter != extensions_.end() && iter->first < end_field_number;
       ++iter) {
    if (!iter->second.SerializeFieldWithCachedSizes(message, output)) {
      return false;
    }
  }

  return true;
}

int ExtensionSet::ByteSize(const Message& message) const {
  int total_size = 0;

  for (map<int, Extension>::const_iterator iter = extensions_.begin();
       iter != extensions_.end(); ++iter) {
    total_size += iter->second.ByteSize(message);
  }

  return total_size;
}

// ===================================================================
// Methods of ExtensionSet::Extension

void ExtensionSet::Extension::Clear() {
  if (descriptor->is_repeated()) {
    switch (descriptor->cpp_type()) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)                          \
      case FieldDescriptor::CPPTYPE_##UPPERCASE:                   \
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
      switch (descriptor->cpp_type()) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)                                 \
        case FieldDescriptor::CPPTYPE_##UPPERCASE:                        \
          LOWERCASE##_value = descriptor->default_value_##LOWERCASE();    \
          break

        HANDLE_TYPE( INT32,  int32);
        HANDLE_TYPE( INT64,  int64);
        HANDLE_TYPE(UINT32, uint32);
        HANDLE_TYPE(UINT64, uint64);
        HANDLE_TYPE( FLOAT,  float);
        HANDLE_TYPE(DOUBLE, double);
        HANDLE_TYPE(  BOOL,   bool);
#undef HANDLE_TYPE
        case FieldDescriptor::CPPTYPE_ENUM:
          enum_value = descriptor->default_value_enum()->number();
          break;
        case FieldDescriptor::CPPTYPE_STRING:
          if (descriptor->has_default_value()) {
            string_value->assign(descriptor->default_value_string());
          } else {
            string_value->clear();
          }
          break;
        case FieldDescriptor::CPPTYPE_MESSAGE:
          message_value->Clear();
          break;
      }

      is_cleared = true;
    }
  }
}

bool ExtensionSet::Extension::SerializeFieldWithCachedSizes(
    const Message& message,
    io::CodedOutputStream* output) const {
  if (descriptor->is_repeated() || !is_cleared) {
    return WireFormat::SerializeFieldWithCachedSizes(
      descriptor, message, output);
  } else {
    return true;
  }
}

int64 ExtensionSet::Extension::ByteSize(const Message& message) const {
  if (descriptor->is_repeated() || !is_cleared) {
    return WireFormat::FieldByteSize(descriptor, message);
  } else {
    // Cleared, non-repeated field.
    return 0;
  }
}

int ExtensionSet::Extension::GetSize() const {
  GOOGLE_DCHECK(descriptor->is_repeated());
  switch (descriptor->cpp_type()) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)                        \
    case FieldDescriptor::CPPTYPE_##UPPERCASE:                   \
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
  if (descriptor->is_repeated()) {
    switch (descriptor->cpp_type()) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)                          \
      case FieldDescriptor::CPPTYPE_##UPPERCASE:                   \
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
    switch (descriptor->cpp_type()) {
      case FieldDescriptor::CPPTYPE_STRING:
        delete string_value;
        break;
      case FieldDescriptor::CPPTYPE_MESSAGE:
        delete message_value;
        break;
      default:
        break;
    }
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
