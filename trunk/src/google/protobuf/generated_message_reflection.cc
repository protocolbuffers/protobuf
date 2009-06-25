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

#include <algorithm>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {
namespace internal {

namespace { const string kEmptyString; }

int StringSpaceUsedExcludingSelf(const string& str) {
  const void* start = &str;
  const void* end = &str + 1;

  if (start <= str.data() && str.data() <= end) {
    // The string's data is stored inside the string object itself.
    return 0;
  } else {
    return str.capacity();
  }
}

bool ParseNamedEnum(const EnumDescriptor* descriptor,
                    const string& name,
                    int* value) {
  const EnumValueDescriptor* d = descriptor->FindValueByName(name);
  if (d == NULL) return false;
  *value = d->number();
  return true;
}

const string& NameOfEnum(const EnumDescriptor* descriptor, int value) {
  static string kEmptyString;
  const EnumValueDescriptor* d = descriptor->FindValueByNumber(value);
  return (d == NULL ? kEmptyString : d->name());
}

// ===================================================================
// Helpers for reporting usage errors (e.g. trying to use GetInt32() on
// a string field).

namespace {

void ReportReflectionUsageError(
    const Descriptor* descriptor, const FieldDescriptor* field,
    const char* method, const char* description) {
  GOOGLE_LOG(FATAL)
    << "Protocol Buffer reflection usage error:\n"
       "  Method      : google::protobuf::Reflection::" << method << "\n"
       "  Message type: " << descriptor->full_name() << "\n"
       "  Field       : " << field->full_name() << "\n"
       "  Problem     : " << description;
}

const char* cpptype_names_[FieldDescriptor::MAX_CPPTYPE + 1] = {
  "INVALID_CPPTYPE",
  "CPPTYPE_INT32",
  "CPPTYPE_INT64",
  "CPPTYPE_UINT32",
  "CPPTYPE_UINT64",
  "CPPTYPE_DOUBLE",
  "CPPTYPE_FLOAT",
  "CPPTYPE_BOOL",
  "CPPTYPE_ENUM",
  "CPPTYPE_STRING",
  "CPPTYPE_MESSAGE"
};

static void ReportReflectionUsageTypeError(
    const Descriptor* descriptor, const FieldDescriptor* field,
    const char* method,
    FieldDescriptor::CppType expected_type) {
  GOOGLE_LOG(FATAL)
    << "Protocol Buffer reflection usage error:\n"
       "  Method      : google::protobuf::Reflection::" << method << "\n"
       "  Message type: " << descriptor->full_name() << "\n"
       "  Field       : " << field->full_name() << "\n"
       "  Problem     : Field is not the right type for this message:\n"
       "    Expected  : " << cpptype_names_[expected_type] << "\n"
       "    Field type: " << cpptype_names_[field->cpp_type()];
}

static void ReportReflectionUsageEnumTypeError(
    const Descriptor* descriptor, const FieldDescriptor* field,
    const char* method, const EnumValueDescriptor* value) {
  GOOGLE_LOG(FATAL)
    << "Protocol Buffer reflection usage error:\n"
       "  Method      : google::protobuf::Reflection::" << method << "\n"
       "  Message type: " << descriptor->full_name() << "\n"
       "  Field       : " << field->full_name() << "\n"
       "  Problem     : Enum value did not match field type:\n"
       "    Expected  : " << field->enum_type()->full_name() << "\n"
       "    Actual    : " << value->full_name();
}

#define USAGE_CHECK(CONDITION, METHOD, ERROR_DESCRIPTION)                      \
  if (!(CONDITION))                                                            \
    ReportReflectionUsageError(descriptor_, field, #METHOD, ERROR_DESCRIPTION)
#define USAGE_CHECK_EQ(A, B, METHOD, ERROR_DESCRIPTION)                        \
  USAGE_CHECK((A) == (B), METHOD, ERROR_DESCRIPTION)
#define USAGE_CHECK_NE(A, B, METHOD, ERROR_DESCRIPTION)                        \
  USAGE_CHECK((A) != (B), METHOD, ERROR_DESCRIPTION)

#define USAGE_CHECK_TYPE(METHOD, CPPTYPE)                                      \
  if (field->cpp_type() != FieldDescriptor::CPPTYPE_##CPPTYPE)                 \
    ReportReflectionUsageTypeError(descriptor_, field, #METHOD,                \
                                   FieldDescriptor::CPPTYPE_##CPPTYPE)

#define USAGE_CHECK_ENUM_VALUE(METHOD)                                         \
  if (value->type() != field->enum_type())                                     \
    ReportReflectionUsageEnumTypeError(descriptor_, field, #METHOD, value)

#define USAGE_CHECK_MESSAGE_TYPE(METHOD)                                       \
  USAGE_CHECK_EQ(field->containing_type(), descriptor_,                        \
                 METHOD, "Field does not match message type.");
#define USAGE_CHECK_SINGULAR(METHOD)                                           \
  USAGE_CHECK_NE(field->label(), FieldDescriptor::LABEL_REPEATED, METHOD,      \
                 "Field is repeated; the method requires a singular field.")
#define USAGE_CHECK_REPEATED(METHOD)                                           \
  USAGE_CHECK_EQ(field->label(), FieldDescriptor::LABEL_REPEATED, METHOD,      \
                 "Field is singular; the method requires a repeated field.")

#define USAGE_CHECK_ALL(METHOD, LABEL, CPPTYPE)                       \
    USAGE_CHECK_MESSAGE_TYPE(METHOD);                                 \
    USAGE_CHECK_##LABEL(METHOD);                                      \
    USAGE_CHECK_TYPE(METHOD, CPPTYPE)

}  // namespace

// ===================================================================

GeneratedMessageReflection::GeneratedMessageReflection(
    const Descriptor* descriptor,
    const Message* default_instance,
    const int offsets[],
    int has_bits_offset,
    int unknown_fields_offset,
    int extensions_offset,
    const DescriptorPool* descriptor_pool,
    MessageFactory* factory,
    int object_size)
  : descriptor_       (descriptor),
    default_instance_ (default_instance),
    offsets_          (offsets),
    has_bits_offset_  (has_bits_offset),
    unknown_fields_offset_(unknown_fields_offset),
    extensions_offset_(extensions_offset),
    object_size_      (object_size),
    descriptor_pool_  ((descriptor_pool == NULL) ?
                         DescriptorPool::generated_pool() :
                         descriptor_pool),
    message_factory_  (factory) {
}

GeneratedMessageReflection::~GeneratedMessageReflection() {}

const UnknownFieldSet& GeneratedMessageReflection::GetUnknownFields(
    const Message& message) const {
  const void* ptr = reinterpret_cast<const uint8*>(&message) +
                    unknown_fields_offset_;
  return *reinterpret_cast<const UnknownFieldSet*>(ptr);
}
UnknownFieldSet* GeneratedMessageReflection::MutableUnknownFields(
    Message* message) const {
  void* ptr = reinterpret_cast<uint8*>(message) + unknown_fields_offset_;
  return reinterpret_cast<UnknownFieldSet*>(ptr);
}

int GeneratedMessageReflection::SpaceUsed(const Message& message) const {
  // object_size_ already includes the in-memory representation of each field
  // in the message, so we only need to account for additional memory used by
  // the fields.
  int total_size = object_size_;

  total_size += GetUnknownFields(message).SpaceUsedExcludingSelf();

  if (extensions_offset_ != -1) {
    total_size += GetExtensionSet(message).SpaceUsedExcludingSelf();
  }

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    if (field->is_repeated()) {
      total_size += GetRaw<GenericRepeatedField>(message, field)
                      .GenericSpaceUsedExcludingSelf();
    } else {
      switch (field->cpp_type()) {
        case FieldDescriptor::CPPTYPE_INT32 :
        case FieldDescriptor::CPPTYPE_INT64 :
        case FieldDescriptor::CPPTYPE_UINT32:
        case FieldDescriptor::CPPTYPE_UINT64:
        case FieldDescriptor::CPPTYPE_DOUBLE:
        case FieldDescriptor::CPPTYPE_FLOAT :
        case FieldDescriptor::CPPTYPE_BOOL  :
        case FieldDescriptor::CPPTYPE_ENUM  :
          // Field is inline, so we've already counted it.
          break;

        case FieldDescriptor::CPPTYPE_STRING: {
            const string* ptr = GetField<const string*>(message, field);

            // Initially, the string points to the default value stored in
            // the prototype. Only count the string if it has been changed
            // from the default value.
            const string* default_ptr = DefaultRaw<const string*>(field);

            if (ptr != default_ptr) {
              // string fields are represented by just a pointer, so also
              // include sizeof(string) as well.
              total_size += sizeof(*ptr) + StringSpaceUsedExcludingSelf(*ptr);
            }
          break;
        }

        case FieldDescriptor::CPPTYPE_MESSAGE:
          if (&message == default_instance_) {
            // For singular fields, the prototype just stores a pointer to the
            // external type's prototype, so there is no extra memory usage.
          } else {
            const Message* sub_message = GetRaw<const Message*>(message, field);
            if (sub_message != NULL) {
              total_size += sub_message->SpaceUsed();
            }
          }
          break;
      }
    }
  }

  return total_size;
}

void GeneratedMessageReflection::Swap(
    Message* message1,
    Message* message2) const {
  if (message1 == message2) return;

  GOOGLE_CHECK_EQ(message1->GetReflection(), this)
    << "Tried to swap using reflection object incompatible with message1.";

  GOOGLE_CHECK_EQ(message2->GetReflection(), this)
    << "Tried to swap using reflection object incompatible with message2.";

  uint32* has_bits1 = MutableHasBits(message1);
  uint32* has_bits2 = MutableHasBits(message2);
  int has_bits_size = (descriptor_->field_count() + 31) / 32;

  for (int i = 0; i < has_bits_size; i++) {
    std::swap(has_bits1[i], has_bits2[i]);
  }

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    if (field->is_repeated()) {
      MutableRaw<GenericRepeatedField>(message1, field)->GenericSwap(
          MutableRaw<GenericRepeatedField>(message2, field));
    } else {
      switch (field->cpp_type()) {
#define SWAP_VALUES(CPPTYPE, TYPE)                                           \
        case FieldDescriptor::CPPTYPE_##CPPTYPE:                             \
          swap(*MutableRaw<TYPE>(message1, field),                           \
               *MutableRaw<TYPE>(message2, field));                          \
          break;
          SWAP_VALUES(INT32 , int32 );
          SWAP_VALUES(INT64 , int64 );
          SWAP_VALUES(UINT32, uint32);
          SWAP_VALUES(UINT64, uint64);
          SWAP_VALUES(FLOAT , float );
          SWAP_VALUES(DOUBLE, double);
          SWAP_VALUES(BOOL  , bool  );
          SWAP_VALUES(ENUM  , int32 );
          SWAP_VALUES(STRING, string*);
          SWAP_VALUES(MESSAGE, Message*);
#undef SWAP_PRIMITIVE_VALUES
        default:
          GOOGLE_LOG(FATAL) << "Unimplemented type: " << field->cpp_type();
      }
    }
  }

  if (extensions_offset_ != -1) {
    MutableExtensionSet(message1)->Swap(MutableExtensionSet(message2));
  }

  MutableUnknownFields(message1)->Swap(MutableUnknownFields(message2));
}

// -------------------------------------------------------------------

bool GeneratedMessageReflection::HasField(const Message& message,
                                          const FieldDescriptor* field) const {
  USAGE_CHECK_MESSAGE_TYPE(HasField);
  USAGE_CHECK_SINGULAR(HasField);

  if (field->is_extension()) {
    return GetExtensionSet(message).Has(field->number());
  } else {
    return HasBit(message, field);
  }
}

int GeneratedMessageReflection::FieldSize(const Message& message,
                                          const FieldDescriptor* field) const {
  USAGE_CHECK_MESSAGE_TYPE(FieldSize);
  USAGE_CHECK_REPEATED(FieldSize);

  if (field->is_extension()) {
    return GetExtensionSet(message).ExtensionSize(field->number());
  } else {
    return GetRaw<GenericRepeatedField>(message, field).GenericSize();
  }
}

void GeneratedMessageReflection::ClearField(
    Message* message, const FieldDescriptor* field) const {
  USAGE_CHECK_MESSAGE_TYPE(ClearField);

  if (field->is_extension()) {
    MutableExtensionSet(message)->ClearExtension(field->number());
  } else if (!field->is_repeated()) {
    if (HasBit(*message, field)) {
      ClearBit(message, field);

      // We need to set the field back to its default value.
      switch (field->cpp_type()) {
#define CLEAR_TYPE(CPPTYPE, TYPE)                                            \
        case FieldDescriptor::CPPTYPE_##CPPTYPE:                             \
          *MutableRaw<TYPE>(message, field) =                                \
            field->default_value_##TYPE();                                   \
          break;

        CLEAR_TYPE(INT32 , int32 );
        CLEAR_TYPE(INT64 , int64 );
        CLEAR_TYPE(UINT32, uint32);
        CLEAR_TYPE(UINT64, uint64);
        CLEAR_TYPE(FLOAT , float );
        CLEAR_TYPE(DOUBLE, double);
        CLEAR_TYPE(BOOL  , bool  );
#undef CLEAR_TYPE

        case FieldDescriptor::CPPTYPE_ENUM:
          *MutableRaw<int>(message, field) =
            field->default_value_enum()->number();
          break;

        case FieldDescriptor::CPPTYPE_STRING: {
            const string* default_ptr = DefaultRaw<const string*>(field);
            string** value = MutableRaw<string*>(message, field);
            if (*value != default_ptr) {
              if (field->has_default_value()) {
                (*value)->assign(field->default_value_string());
              } else {
                (*value)->clear();
              }
            }
          break;
        }

        case FieldDescriptor::CPPTYPE_MESSAGE:
          (*MutableRaw<Message*>(message, field))->Clear();
          break;
      }
    }
  } else {
    MutableRaw<GenericRepeatedField>(message, field)->GenericClear();
  }
}

void GeneratedMessageReflection::RemoveLast(
    Message* message,
    const FieldDescriptor* field) const {
  USAGE_CHECK_MESSAGE_TYPE(RemoveLast);
  USAGE_CHECK_REPEATED(RemoveLast);

  if (field->is_extension()) {
    MutableExtensionSet(message)->RemoveLast(field->number());
  } else {
    MutableRaw<GenericRepeatedField>(message, field)->GenericRemoveLast();
  }
}

void GeneratedMessageReflection::SwapElements(
    Message* message,
    const FieldDescriptor* field,
    int index1,
    int index2) const {
  USAGE_CHECK_MESSAGE_TYPE(Swap);
  USAGE_CHECK_REPEATED(Swap);

  if (field->is_extension()) {
    MutableExtensionSet(message)->SwapElements(
                                      field->number(), index1, index2);
  } else {
    MutableRaw<GenericRepeatedField>(message, field)->GenericSwapElements(
                                                            index1, index2);
  }
}

namespace {
// Comparison functor for sorting FieldDescriptors by field number.
struct FieldNumberSorter {
  bool operator()(const FieldDescriptor* left,
                  const FieldDescriptor* right) const {
    return left->number() < right->number();
  }
};
}  // namespace

void GeneratedMessageReflection::ListFields(
    const Message& message,
    vector<const FieldDescriptor*>* output) const {
  output->clear();

  // Optimization:  The default instance never has any fields set.
  if (&message == default_instance_) return;

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    if (field->is_repeated()) {
      if (GetRaw<GenericRepeatedField>(message, field).GenericSize() > 0) {
        output->push_back(field);
      }
    } else {
      if (HasBit(message, field)) {
        output->push_back(field);
      }
    }
  }

  if (extensions_offset_ != -1) {
    GetExtensionSet(message).AppendToList(descriptor_, descriptor_pool_,
                                          output);
  }

  // ListFields() must sort output by field number.
  sort(output->begin(), output->end(), FieldNumberSorter());
}

// -------------------------------------------------------------------

#undef DEFINE_PRIMITIVE_ACCESSORS
#define DEFINE_PRIMITIVE_ACCESSORS(TYPENAME, TYPE, PASSTYPE, CPPTYPE)        \
  PASSTYPE GeneratedMessageReflection::Get##TYPENAME(                        \
      const Message& message, const FieldDescriptor* field) const {          \
    USAGE_CHECK_ALL(Get##TYPENAME, SINGULAR, CPPTYPE);                       \
    if (field->is_extension()) {                                             \
      return GetExtensionSet(message).Get##TYPENAME(                         \
        field->number(), field->default_value_##PASSTYPE());                 \
    } else {                                                                 \
      return GetField<TYPE>(message, field);                                 \
    }                                                                        \
  }                                                                          \
                                                                             \
  void GeneratedMessageReflection::Set##TYPENAME(                            \
      Message* message, const FieldDescriptor* field,                        \
      PASSTYPE value) const {                                                \
    USAGE_CHECK_ALL(Set##TYPENAME, SINGULAR, CPPTYPE);                       \
    if (field->is_extension()) {                                             \
      return MutableExtensionSet(message)->Set##TYPENAME(                    \
        field->number(), field->type(), value);                              \
    } else {                                                                 \
      SetField<TYPE>(message, field, value);                                 \
    }                                                                        \
  }                                                                          \
                                                                             \
  PASSTYPE GeneratedMessageReflection::GetRepeated##TYPENAME(                \
      const Message& message,                                                \
      const FieldDescriptor* field, int index) const {                       \
    USAGE_CHECK_ALL(GetRepeated##TYPENAME, REPEATED, CPPTYPE);               \
    if (field->is_extension()) {                                             \
      return GetExtensionSet(message).GetRepeated##TYPENAME(                 \
        field->number(), index);                                             \
    } else {                                                                 \
      return GetRepeatedField<TYPE>(message, field, index);                  \
    }                                                                        \
  }                                                                          \
                                                                             \
  void GeneratedMessageReflection::SetRepeated##TYPENAME(                    \
      Message* message, const FieldDescriptor* field,                        \
      int index, PASSTYPE value) const {                                     \
    USAGE_CHECK_ALL(SetRepeated##TYPENAME, REPEATED, CPPTYPE);               \
    if (field->is_extension()) {                                             \
      MutableExtensionSet(message)->SetRepeated##TYPENAME(                   \
        field->number(), index, value);                                      \
    } else {                                                                 \
      SetRepeatedField<TYPE>(message, field, index, value);                  \
    }                                                                        \
  }                                                                          \
                                                                             \
  void GeneratedMessageReflection::Add##TYPENAME(                            \
      Message* message, const FieldDescriptor* field,                        \
      PASSTYPE value) const {                                                \
    USAGE_CHECK_ALL(Add##TYPENAME, REPEATED, CPPTYPE);                       \
    if (field->is_extension()) {                                             \
      MutableExtensionSet(message)->Add##TYPENAME(                           \
        field->number(), field->type(), field->options().packed(), value);   \
    } else {                                                                 \
      AddField<TYPE>(message, field, value);                                 \
    }                                                                        \
  }

DEFINE_PRIMITIVE_ACCESSORS(Int32 , int32 , int32 , INT32 )
DEFINE_PRIMITIVE_ACCESSORS(Int64 , int64 , int64 , INT64 )
DEFINE_PRIMITIVE_ACCESSORS(UInt32, uint32, uint32, UINT32)
DEFINE_PRIMITIVE_ACCESSORS(UInt64, uint64, uint64, UINT64)
DEFINE_PRIMITIVE_ACCESSORS(Float , float , float , FLOAT )
DEFINE_PRIMITIVE_ACCESSORS(Double, double, double, DOUBLE)
DEFINE_PRIMITIVE_ACCESSORS(Bool  , bool  , bool  , BOOL  )
#undef DEFINE_PRIMITIVE_ACCESSORS

// -------------------------------------------------------------------

string GeneratedMessageReflection::GetString(
    const Message& message, const FieldDescriptor* field) const {
  USAGE_CHECK_ALL(GetString, SINGULAR, STRING);
  if (field->is_extension()) {
    return GetExtensionSet(message).GetString(field->number(),
                                              field->default_value_string());
  } else {
    return *GetField<const string*>(message, field);
  }
}

const string& GeneratedMessageReflection::GetStringReference(
    const Message& message,
    const FieldDescriptor* field, string* scratch) const {
  USAGE_CHECK_ALL(GetStringReference, SINGULAR, STRING);
  if (field->is_extension()) {
    return GetExtensionSet(message).GetString(field->number(),
                                              field->default_value_string());
  } else {
    return *GetField<const string*>(message, field);
  }
}


void GeneratedMessageReflection::SetString(
    Message* message, const FieldDescriptor* field,
    const string& value) const {
  USAGE_CHECK_ALL(SetString, SINGULAR, STRING);
  if (field->is_extension()) {
    return MutableExtensionSet(message)->SetString(field->number(),
                                                   field->type(), value);
  } else {
    string** ptr = MutableField<string*>(message, field);
    if (*ptr == DefaultRaw<const string*>(field)) {
      *ptr = new string(value);
    } else {
      (*ptr)->assign(value);
    }
  }
}


string GeneratedMessageReflection::GetRepeatedString(
    const Message& message, const FieldDescriptor* field, int index) const {
  USAGE_CHECK_ALL(GetRepeatedString, REPEATED, STRING);
  if (field->is_extension()) {
    return GetExtensionSet(message).GetRepeatedString(field->number(), index);
  } else {
    return GetRepeatedField<string>(message, field, index);
  }
}

const string& GeneratedMessageReflection::GetRepeatedStringReference(
    const Message& message, const FieldDescriptor* field,
    int index, string* scratch) const {
  USAGE_CHECK_ALL(GetRepeatedStringReference, REPEATED, STRING);
  if (field->is_extension()) {
    return GetExtensionSet(message).GetRepeatedString(field->number(), index);
  } else {
    return GetRepeatedField<string>(message, field, index);
  }
}


void GeneratedMessageReflection::SetRepeatedString(
    Message* message, const FieldDescriptor* field,
    int index, const string& value) const {
  USAGE_CHECK_ALL(SetRepeatedString, REPEATED, STRING);
  if (field->is_extension()) {
    MutableExtensionSet(message)->SetRepeatedString(
      field->number(), index, value);
  } else {
    SetRepeatedField<string>(message, field, index, value);
  }
}


void GeneratedMessageReflection::AddString(
    Message* message, const FieldDescriptor* field,
    const string& value) const {
  USAGE_CHECK_ALL(AddString, REPEATED, STRING);
  if (field->is_extension()) {
    MutableExtensionSet(message)->AddString(field->number(),
                                            field->type(), value);
  } else {
    AddField<string>(message, field, value);
  }
}


// -------------------------------------------------------------------

const EnumValueDescriptor* GeneratedMessageReflection::GetEnum(
    const Message& message, const FieldDescriptor* field) const {
  USAGE_CHECK_ALL(GetEnum, SINGULAR, ENUM);

  int value;
  if (field->is_extension()) {
    value = GetExtensionSet(message).GetEnum(
      field->number(), field->default_value_enum()->number());
  } else {
    value = GetField<int>(message, field);
  }
  const EnumValueDescriptor* result =
    field->enum_type()->FindValueByNumber(value);
  GOOGLE_CHECK(result != NULL);
  return result;
}

void GeneratedMessageReflection::SetEnum(
    Message* message, const FieldDescriptor* field,
    const EnumValueDescriptor* value) const {
  USAGE_CHECK_ALL(SetEnum, SINGULAR, ENUM);
  USAGE_CHECK_ENUM_VALUE(SetEnum);

  if (field->is_extension()) {
    MutableExtensionSet(message)->SetEnum(field->number(), field->type(),
                                          value->number());
  } else {
    SetField<int>(message, field, value->number());
  }
}

const EnumValueDescriptor* GeneratedMessageReflection::GetRepeatedEnum(
    const Message& message, const FieldDescriptor* field, int index) const {
  USAGE_CHECK_ALL(GetRepeatedEnum, REPEATED, ENUM);

  int value;
  if (field->is_extension()) {
    value = GetExtensionSet(message).GetRepeatedEnum(field->number(), index);
  } else {
    value = GetRepeatedField<int>(message, field, index);
  }
  const EnumValueDescriptor* result =
    field->enum_type()->FindValueByNumber(value);
  GOOGLE_CHECK(result != NULL);
  return result;
}

void GeneratedMessageReflection::SetRepeatedEnum(
    Message* message,
    const FieldDescriptor* field, int index,
    const EnumValueDescriptor* value) const {
  USAGE_CHECK_ALL(SetRepeatedEnum, REPEATED, ENUM);
  USAGE_CHECK_ENUM_VALUE(SetRepeatedEnum);

  if (field->is_extension()) {
    MutableExtensionSet(message)->SetRepeatedEnum(
      field->number(), index, value->number());
  } else {
    SetRepeatedField<int>(message, field, index, value->number());
  }
}

void GeneratedMessageReflection::AddEnum(
    Message* message, const FieldDescriptor* field,
    const EnumValueDescriptor* value) const {
  USAGE_CHECK_ALL(AddEnum, REPEATED, ENUM);
  USAGE_CHECK_ENUM_VALUE(AddEnum);

  if (field->is_extension()) {
    MutableExtensionSet(message)->AddEnum(field->number(), field->type(),
                                          field->options().packed(),
                                          value->number());
  } else {
    AddField<int>(message, field, value->number());
  }
}

// -------------------------------------------------------------------

const Message& GeneratedMessageReflection::GetMessage(
    const Message& message, const FieldDescriptor* field) const {
  USAGE_CHECK_ALL(GetMessage, SINGULAR, MESSAGE);

  if (field->is_extension()) {
    return GetExtensionSet(message).GetMessage(field->number(),
                                               field->message_type(),
                                               message_factory_);
  } else {
    const Message* result = GetRaw<const Message*>(message, field);
    if (result == NULL) {
      result = DefaultRaw<const Message*>(field);
    }
    return *result;
  }
}

Message* GeneratedMessageReflection::MutableMessage(
    Message* message, const FieldDescriptor* field) const {
  USAGE_CHECK_ALL(MutableMessage, SINGULAR, MESSAGE);

  if (field->is_extension()) {
    return MutableExtensionSet(message)->MutableMessage(field->number(),
                                                        field->type(),
                                                        field->message_type(),
                                                        message_factory_);
  } else {
    Message** result = MutableField<Message*>(message, field);
    if (*result == NULL) {
      const Message* default_message = DefaultRaw<const Message*>(field);
      *result = default_message->New();
    }
    return *result;
  }
}

const Message& GeneratedMessageReflection::GetRepeatedMessage(
    const Message& message, const FieldDescriptor* field, int index) const {
  USAGE_CHECK_ALL(GetRepeatedMessage, REPEATED, MESSAGE);

  if (field->is_extension()) {
    return GetExtensionSet(message).GetRepeatedMessage(field->number(), index);
  } else {
    return GetRepeatedField<Message>(message, field, index);
  }
}

Message* GeneratedMessageReflection::MutableRepeatedMessage(
    Message* message, const FieldDescriptor* field, int index) const {
  USAGE_CHECK_ALL(MutableRepeatedMessage, REPEATED, MESSAGE);

  if (field->is_extension()) {
    return MutableExtensionSet(message)->MutableRepeatedMessage(
      field->number(), index);
  } else {
    return MutableRepeatedField<Message>(message, field, index);
  }
}

Message* GeneratedMessageReflection::AddMessage(
    Message* message, const FieldDescriptor* field) const {
  USAGE_CHECK_ALL(AddMessage, REPEATED, MESSAGE);

  if (field->is_extension()) {
    return MutableExtensionSet(message)->AddMessage(field->number(),
                                                    field->type(),
                                                    field->message_type(),
                                                    message_factory_);
  } else {
    return AddField<Message>(message, field);
  }
}

// -------------------------------------------------------------------

const FieldDescriptor* GeneratedMessageReflection::FindKnownExtensionByName(
    const string& name) const {
  if (extensions_offset_ == -1) return NULL;

  const FieldDescriptor* result = descriptor_pool_->FindExtensionByName(name);
  if (result != NULL && result->containing_type() == descriptor_) {
    return result;
  }

  if (descriptor_->options().message_set_wire_format()) {
    // MessageSet extensions may be identified by type name.
    const Descriptor* type = descriptor_pool_->FindMessageTypeByName(name);
    if (type != NULL) {
      // Look for a matching extension in the foreign type's scope.
      for (int i = 0; i < type->extension_count(); i++) {
        const FieldDescriptor* extension = type->extension(i);
        if (extension->containing_type() == descriptor_ &&
            extension->type() == FieldDescriptor::TYPE_MESSAGE &&
            extension->is_optional() &&
            extension->message_type() == type) {
          // Found it.
          return extension;
        }
      }
    }
  }

  return NULL;
}

const FieldDescriptor* GeneratedMessageReflection::FindKnownExtensionByNumber(
    int number) const {
  if (extensions_offset_ == -1) return NULL;
  return descriptor_pool_->FindExtensionByNumber(descriptor_, number);
}

// ===================================================================
// Some private helpers.

// These simple template accessors obtain pointers (or references) to
// the given field.
template <typename Type>
inline const Type& GeneratedMessageReflection::GetRaw(
    const Message& message, const FieldDescriptor* field) const {
  const void* ptr = reinterpret_cast<const uint8*>(&message) +
                    offsets_[field->index()];
  return *reinterpret_cast<const Type*>(ptr);
}

template <typename Type>
inline Type* GeneratedMessageReflection::MutableRaw(
    Message* message, const FieldDescriptor* field) const {
  void* ptr = reinterpret_cast<uint8*>(message) + offsets_[field->index()];
  return reinterpret_cast<Type*>(ptr);
}

template <typename Type>
inline const Type& GeneratedMessageReflection::DefaultRaw(
    const FieldDescriptor* field) const {
  const void* ptr = reinterpret_cast<const uint8*>(default_instance_) +
                    offsets_[field->index()];
  return *reinterpret_cast<const Type*>(ptr);
}

inline const uint32* GeneratedMessageReflection::GetHasBits(
    const Message& message) const {
  const void* ptr = reinterpret_cast<const uint8*>(&message) + has_bits_offset_;
  return reinterpret_cast<const uint32*>(ptr);
}
inline uint32* GeneratedMessageReflection::MutableHasBits(
    Message* message) const {
  void* ptr = reinterpret_cast<uint8*>(message) + has_bits_offset_;
  return reinterpret_cast<uint32*>(ptr);
}

inline const ExtensionSet& GeneratedMessageReflection::GetExtensionSet(
    const Message& message) const {
  GOOGLE_DCHECK_NE(extensions_offset_, -1);
  const void* ptr = reinterpret_cast<const uint8*>(&message) +
                    extensions_offset_;
  return *reinterpret_cast<const ExtensionSet*>(ptr);
}
inline ExtensionSet* GeneratedMessageReflection::MutableExtensionSet(
    Message* message) const {
  GOOGLE_DCHECK_NE(extensions_offset_, -1);
  void* ptr = reinterpret_cast<uint8*>(message) + extensions_offset_;
  return reinterpret_cast<ExtensionSet*>(ptr);
}

// Simple accessors for manipulating has_bits_.
inline bool GeneratedMessageReflection::HasBit(
    const Message& message, const FieldDescriptor* field) const {
  return GetHasBits(message)[field->index() / 32] &
    (1 << (field->index() % 32));
}

inline void GeneratedMessageReflection::SetBit(
    Message* message, const FieldDescriptor* field) const {
  MutableHasBits(message)[field->index() / 32] |= (1 << (field->index() % 32));
}

inline void GeneratedMessageReflection::ClearBit(
    Message* message, const FieldDescriptor* field) const {
  MutableHasBits(message)[field->index() / 32] &= ~(1 << (field->index() % 32));
}

// Template implementations of basic accessors.  Inline because each
// template instance is only called from one location.  These are
// used for all types except messages.
template <typename Type>
inline const Type& GeneratedMessageReflection::GetField(
    const Message& message, const FieldDescriptor* field) const {
  return GetRaw<Type>(message, field);
}

template <typename Type>
inline void GeneratedMessageReflection::SetField(
    Message* message, const FieldDescriptor* field, const Type& value) const {
  *MutableRaw<Type>(message, field) = value;
  SetBit(message, field);
}

template <typename Type>
inline Type* GeneratedMessageReflection::MutableField(
    Message* message, const FieldDescriptor* field) const {
  SetBit(message, field);
  return MutableRaw<Type>(message, field);
}

template <typename Type>
inline const Type& GeneratedMessageReflection::GetRepeatedField(
    const Message& message, const FieldDescriptor* field, int index) const {
  return *reinterpret_cast<const Type*>(
    GetRaw<GenericRepeatedField>(message, field).GenericGet(index));
}

template <typename Type>
inline void GeneratedMessageReflection::SetRepeatedField(
    Message* message, const FieldDescriptor* field,
    int index, const Type& value) const {
  GenericRepeatedField* repeated =
    MutableRaw<GenericRepeatedField>(message, field);
  *reinterpret_cast<Type*>(repeated->GenericMutable(index)) = value;
}

template <typename Type>
inline Type* GeneratedMessageReflection::MutableRepeatedField(
    Message* message, const FieldDescriptor* field, int index) const {
  GenericRepeatedField* repeated =
    MutableRaw<GenericRepeatedField>(message, field);
  return reinterpret_cast<Type*>(repeated->GenericMutable(index));
}

template <typename Type>
inline void GeneratedMessageReflection::AddField(
    Message* message, const FieldDescriptor* field, const Type& value) const {
  GenericRepeatedField* repeated =
    MutableRaw<GenericRepeatedField>(message, field);
  *reinterpret_cast<Type*>(repeated->GenericAdd()) = value;
}

template <typename Type>
inline Type* GeneratedMessageReflection::AddField(
    Message* message, const FieldDescriptor* field) const {
  GenericRepeatedField* repeated =
    MutableRaw<GenericRepeatedField>(message, field);
  return reinterpret_cast<Type*>(repeated->GenericAdd());
}


}  // namespace internal
}  // namespace protobuf
}  // namespace google
