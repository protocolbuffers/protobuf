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
//
// Contains methods defined in extension_set.h which cannot be part of the
// lite library because they use descriptors or reflection.

#include <google/protobuf/extension_set.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/wire_format_lite_inl.h>

namespace google {
namespace protobuf {
namespace internal {

void ExtensionSet::AppendToList(const Descriptor* containing_type,
                                const DescriptorPool* pool,
                                vector<const FieldDescriptor*>* output) const {
  for (map<int, Extension>::const_iterator iter = extensions_.begin();
       iter != extensions_.end(); ++iter) {
    bool has = false;
    if (iter->second.is_repeated) {
      has = iter->second.GetSize() > 0;
    } else {
      has = !iter->second.is_cleared;
    }

    if (has) {
      // TODO(kenton): Looking up each field by number is somewhat unfortunate.
      //   Is there a better way?  The problem is that descriptors are lazily-
      //   initialized, so they might not even be constructed until
      //   AppendToList() is called.

      if (iter->second.descriptor == NULL) {
        output->push_back(pool->FindExtensionByNumber(
            containing_type, iter->first));
      } else {
        output->push_back(iter->second.descriptor);
      }
    }
  }
}

inline FieldDescriptor::Type real_type(FieldType type) {
  GOOGLE_DCHECK(type > 0 && type <= FieldDescriptor::MAX_TYPE);
  return static_cast<FieldDescriptor::Type>(type);
}

inline FieldDescriptor::CppType cpp_type(FieldType type) {
  return FieldDescriptor::TypeToCppType(
      static_cast<FieldDescriptor::Type>(type));
}

#define GOOGLE_DCHECK_TYPE(EXTENSION, LABEL, CPPTYPE)                            \
  GOOGLE_DCHECK_EQ((EXTENSION).is_repeated ? FieldDescriptor::LABEL_REPEATED     \
                                  : FieldDescriptor::LABEL_OPTIONAL,      \
            FieldDescriptor::LABEL_##LABEL);                              \
  GOOGLE_DCHECK_EQ(cpp_type((EXTENSION).type), FieldDescriptor::CPPTYPE_##CPPTYPE)

const MessageLite& ExtensionSet::GetMessage(int number,
                                            const Descriptor* message_type,
                                            MessageFactory* factory) const {
  map<int, Extension>::const_iterator iter = extensions_.find(number);
  if (iter == extensions_.end() || iter->second.is_cleared) {
    // Not present.  Return the default value.
    return *factory->GetPrototype(message_type);
  } else {
    GOOGLE_DCHECK_TYPE(iter->second, OPTIONAL, MESSAGE);
    return *iter->second.message_value;
  }
}

MessageLite* ExtensionSet::MutableMessage(const FieldDescriptor* descriptor,
                                          MessageFactory* factory) {
  Extension* extension;
  if (MaybeNewExtension(descriptor->number(), descriptor, &extension)) {
    extension->type = descriptor->type();
    GOOGLE_DCHECK_EQ(cpp_type(extension->type), FieldDescriptor::CPPTYPE_MESSAGE);
    extension->is_repeated = false;
    extension->is_packed = false;
    const MessageLite* prototype =
        factory->GetPrototype(descriptor->message_type());
    GOOGLE_CHECK(prototype != NULL);
    extension->message_value = prototype->New();
  } else {
    GOOGLE_DCHECK_TYPE(*extension, OPTIONAL, MESSAGE);
  }
  extension->is_cleared = false;
  return extension->message_value;
}

MessageLite* ExtensionSet::AddMessage(const FieldDescriptor* descriptor,
                                      MessageFactory* factory) {
  Extension* extension;
  if (MaybeNewExtension(descriptor->number(), descriptor, &extension)) {
    extension->type = descriptor->type();
    GOOGLE_DCHECK_EQ(cpp_type(extension->type), FieldDescriptor::CPPTYPE_MESSAGE);
    extension->is_repeated = true;
    extension->repeated_message_value =
      new RepeatedPtrField<MessageLite>();
  } else {
    GOOGLE_DCHECK_TYPE(*extension, REPEATED, MESSAGE);
  }

  // RepeatedPtrField<Message> does not know how to Add() since it cannot
  // allocate an abstract object, so we have to be tricky.
  MessageLite* result = extension->repeated_message_value
      ->AddFromCleared<internal::GenericTypeHandler<MessageLite> >();
  if (result == NULL) {
    const MessageLite* prototype;
    if (extension->repeated_message_value->size() == 0) {
      prototype = factory->GetPrototype(descriptor->message_type());
      GOOGLE_CHECK(prototype != NULL);
    } else {
      prototype = &extension->repeated_message_value->Get(0);
    }
    result = prototype->New();
    extension->repeated_message_value->AddAllocated(result);
  }
  return result;
}

static bool ValidateEnumUsingDescriptor(const void* arg, int number) {
  return reinterpret_cast<const EnumDescriptor*>(arg)
      ->FindValueByNumber(number) != NULL;
}

bool DescriptorPoolExtensionFinder::Find(int number, ExtensionInfo* output) {
  const FieldDescriptor* extension =
      pool_->FindExtensionByNumber(containing_type_, number);
  if (extension == NULL) {
    return false;
  } else {
    output->type = extension->type();
    output->is_repeated = extension->is_repeated();
    output->is_packed = extension->options().packed();
    output->descriptor = extension;
    if (extension->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      output->message_prototype =
          factory_->GetPrototype(extension->message_type());
      GOOGLE_CHECK(output->message_prototype != NULL)
          << "Extension factory's GetPrototype() returned NULL for extension: "
          << extension->full_name();
    } else if (extension->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
      output->enum_is_valid = ValidateEnumUsingDescriptor;
      output->enum_is_valid_arg = extension->enum_type();
    }

    return true;
  }
}

bool ExtensionSet::ParseField(uint32 tag, io::CodedInputStream* input,
                              const Message* containing_type,
                              UnknownFieldSet* unknown_fields) {
  UnknownFieldSetFieldSkipper skipper(unknown_fields);
  if (input->GetExtensionPool() == NULL) {
    GeneratedExtensionFinder finder(containing_type);
    return ParseField(tag, input, &finder, &skipper);
  } else {
    DescriptorPoolExtensionFinder finder(input->GetExtensionPool(),
                                         input->GetExtensionFactory(),
                                         containing_type->GetDescriptor());
    return ParseField(tag, input, &finder, &skipper);
  }
}

bool ExtensionSet::ParseMessageSet(io::CodedInputStream* input,
                                   const Message* containing_type,
                                   UnknownFieldSet* unknown_fields) {
  UnknownFieldSetFieldSkipper skipper(unknown_fields);
  if (input->GetExtensionPool() == NULL) {
    GeneratedExtensionFinder finder(containing_type);
    return ParseMessageSet(input, &finder, &skipper);
  } else {
    DescriptorPoolExtensionFinder finder(input->GetExtensionPool(),
                                         input->GetExtensionFactory(),
                                         containing_type->GetDescriptor());
    return ParseMessageSet(input, &finder, &skipper);
  }
}

int ExtensionSet::SpaceUsedExcludingSelf() const {
  int total_size =
      extensions_.size() * sizeof(map<int, Extension>::value_type);
  for (map<int, Extension>::const_iterator iter = extensions_.begin(),
       end = extensions_.end();
       iter != end;
       ++iter) {
    total_size += iter->second.SpaceUsedExcludingSelf();
  }
  return total_size;
}

inline int ExtensionSet::RepeatedMessage_SpaceUsedExcludingSelf(
    RepeatedPtrFieldBase* field) {
  return field->SpaceUsedExcludingSelf<GenericTypeHandler<Message> >();
}

int ExtensionSet::Extension::SpaceUsedExcludingSelf() const {
  int total_size = 0;
  if (is_repeated) {
    switch (cpp_type(type)) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)                          \
      case FieldDescriptor::CPPTYPE_##UPPERCASE:                   \
        total_size += sizeof(*repeated_##LOWERCASE##_value) +      \
            repeated_##LOWERCASE##_value->SpaceUsedExcludingSelf();\
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
#undef HANDLE_TYPE

      case FieldDescriptor::CPPTYPE_MESSAGE:
        // repeated_message_value is actually a RepeatedPtrField<MessageLite>,
        // but MessageLite has no SpaceUsed(), so we must directly call
        // RepeatedPtrFieldBase::SpaceUsedExcludingSelf() with a different type
        // handler.
        total_size += sizeof(*repeated_message_value) +
            RepeatedMessage_SpaceUsedExcludingSelf(repeated_message_value);
        break;
    }
  } else {
    switch (cpp_type(type)) {
      case FieldDescriptor::CPPTYPE_STRING:
        total_size += sizeof(*string_value) +
                      StringSpaceUsedExcludingSelf(*string_value);
        break;
      case FieldDescriptor::CPPTYPE_MESSAGE:
        total_size += down_cast<Message*>(message_value)->SpaceUsed();
        break;
      default:
        // No extra storage costs for primitive types.
        break;
    }
  }
  return total_size;
}

// The Serialize*ToArray methods are only needed in the heavy library, as
// the lite library only generates SerializeWithCachedSizes.
uint8* ExtensionSet::SerializeWithCachedSizesToArray(
    int start_field_number, int end_field_number,
    uint8* target) const {
  map<int, Extension>::const_iterator iter;
  for (iter = extensions_.lower_bound(start_field_number);
       iter != extensions_.end() && iter->first < end_field_number;
       ++iter) {
    target = iter->second.SerializeFieldWithCachedSizesToArray(iter->first,
                                                               target);
  }
  return target;
}

uint8* ExtensionSet::SerializeMessageSetWithCachedSizesToArray(
    uint8* target) const {
  map<int, Extension>::const_iterator iter;
  for (iter = extensions_.begin(); iter != extensions_.end(); ++iter) {
    target = iter->second.SerializeMessageSetItemWithCachedSizesToArray(
        iter->first, target);
  }
  return target;
}

uint8* ExtensionSet::Extension::SerializeFieldWithCachedSizesToArray(
    int number, uint8* target) const {
  if (is_repeated) {
    if (is_packed) {
      if (cached_size == 0) return target;

      target = WireFormatLite::WriteTagToArray(number,
          WireFormatLite::WIRETYPE_LENGTH_DELIMITED, target);
      target = WireFormatLite::WriteInt32NoTagToArray(cached_size, target);

      switch (real_type(type)) {
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)                        \
        case FieldDescriptor::TYPE_##UPPERCASE:                             \
          for (int i = 0; i < repeated_##LOWERCASE##_value->size(); i++) {  \
            target = WireFormatLite::Write##CAMELCASE##NoTagToArray(        \
              repeated_##LOWERCASE##_value->Get(i), target);                \
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
        case FieldDescriptor::TYPE_##UPPERCASE:                             \
          for (int i = 0; i < repeated_##LOWERCASE##_value->size(); i++) {  \
            target = WireFormatLite::Write##CAMELCASE##ToArray(number,      \
              repeated_##LOWERCASE##_value->Get(i), target);                \
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
      case FieldDescriptor::TYPE_##UPPERCASE:                    \
        target = WireFormatLite::Write##CAMELCASE##ToArray(      \
            number, VALUE, target); \
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
  return target;
}

uint8* ExtensionSet::Extension::SerializeMessageSetItemWithCachedSizesToArray(
    int number,
    uint8* target) const {
  if (type != WireFormatLite::TYPE_MESSAGE || is_repeated) {
    // Not a valid MessageSet extension, but serialize it the normal way.
    GOOGLE_LOG(WARNING) << "Invalid message set extension.";
    return SerializeFieldWithCachedSizesToArray(number, target);
  }

  if (is_cleared) return target;

  // Start group.
  target = io::CodedOutputStream::WriteTagToArray(
      WireFormatLite::kMessageSetItemStartTag, target);
  // Write type ID.
  target = WireFormatLite::WriteUInt32ToArray(
      WireFormatLite::kMessageSetTypeIdNumber, number, target);
  // Write message.
  target = WireFormatLite::WriteMessageToArray(
      WireFormatLite::kMessageSetMessageNumber, *message_value, target);
  // End group.
  target = io::CodedOutputStream::WriteTagToArray(
      WireFormatLite::kMessageSetItemEndTag, target);
  return target;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
