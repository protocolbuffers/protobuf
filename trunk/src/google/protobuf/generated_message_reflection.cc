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

// ===================================================================
// Helpers for reporting usage errors (e.g. trying to use GetInt32() on
// a string field).

namespace {

void ReportReflectionUsageError(
    const Descriptor* descriptor, const FieldDescriptor* field,
    const char* method, const char* description) {
  GOOGLE_LOG(FATAL)
    << "Protocol Buffer reflection usage error:\n"
       "  Method      : google::protobuf::Message::Reflection::" << method << "\n"
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
       "  Method      : google::protobuf::Message::Reflection::" << method << "\n"
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
       "  Method      : google::protobuf::Message::Reflection::" << method << "\n"
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
    void* base, const void* default_base,
    const int offsets[], uint32 has_bits[],
    ExtensionSet* extensions)
  : descriptor_  (descriptor),
    base_        (base),
    default_base_(default_base),
    offsets_     (offsets),
    has_bits_    (has_bits),
    extensions_  (extensions) {
}

GeneratedMessageReflection::~GeneratedMessageReflection() {}

const UnknownFieldSet& GeneratedMessageReflection::GetUnknownFields() const {
  return unknown_fields_;
}
UnknownFieldSet* GeneratedMessageReflection::MutableUnknownFields() {
  return &unknown_fields_;
}

// -------------------------------------------------------------------

bool GeneratedMessageReflection::HasField(const FieldDescriptor* field) const {
  USAGE_CHECK_MESSAGE_TYPE(HasField);
  USAGE_CHECK_SINGULAR(HasField);

  if (field->is_extension()) {
    return extensions_->Has(field->number());
  } else {
    return HasBit(field);
  }
}

int GeneratedMessageReflection::FieldSize(const FieldDescriptor* field) const {
  USAGE_CHECK_MESSAGE_TYPE(HasField);
  USAGE_CHECK_REPEATED(HasField);

  if (field->is_extension()) {
    return extensions_->ExtensionSize(field->number());
  } else {
    return GetRaw<GenericRepeatedField>(field).GenericSize();
  }
}

void GeneratedMessageReflection::ClearField(const FieldDescriptor* field) {
  USAGE_CHECK_MESSAGE_TYPE(ClearField);

  if (field->is_extension()) {
    extensions_->ClearExtension(field->number());
  } else if (!field->is_repeated()) {
    if (HasBit(field)) {
      ClearBit(field);

      // We need to set the field back to its default value.
      switch (field->cpp_type()) {
#define CLEAR_TYPE(CPPTYPE, TYPE)                                            \
        case FieldDescriptor::CPPTYPE_##CPPTYPE:                             \
          *MutableRaw<TYPE>(field) = field->default_value_##TYPE();          \
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
          *MutableRaw<int>(field) = field->default_value_enum()->number();
          break;

        case FieldDescriptor::CPPTYPE_STRING: {
            const string* default_ptr = DefaultRaw<const string*>(field);
            string** value = MutableRaw<string*>(field);
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
          (*MutableRaw<Message*>(field))->Clear();
          break;
      }
    }
  } else {
    MutableRaw<GenericRepeatedField>(field)->GenericClear();
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
    vector<const FieldDescriptor*>* output) const {
  output->clear();

  // Optimization:  The default instance never has any fields set.
  if (base_ == default_base_) return;

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    if (field->is_repeated()) {
      if (GetRaw<GenericRepeatedField>(field).GenericSize() > 0) {
        output->push_back(field);
      }
    } else {
      if (HasBit(field)) {
        output->push_back(field);
      }
    }
  }

  if (extensions_ != NULL) {
    extensions_->AppendToList(output);
  }

  // ListFields() must sort output by field number.
  sort(output->begin(), output->end(), FieldNumberSorter());
}

// -------------------------------------------------------------------

#undef DEFINE_PRIMITIVE_ACCESSORS
#define DEFINE_PRIMITIVE_ACCESSORS(TYPENAME, TYPE, PASSTYPE, CPPTYPE)        \
  PASSTYPE GeneratedMessageReflection::Get##TYPENAME(                        \
                                       const FieldDescriptor* field) const { \
    USAGE_CHECK_ALL(Get##TYPENAME, SINGULAR, CPPTYPE);                       \
    if (field->is_extension()) {                                             \
      return extensions_->Get##TYPENAME(field->number());                    \
    } else {                                                                 \
      return GetField<TYPE>(field);                                          \
    }                                                                        \
  }                                                                          \
                                                                             \
  void GeneratedMessageReflection::Set##TYPENAME(                            \
      const FieldDescriptor* field, PASSTYPE value) {                        \
    USAGE_CHECK_ALL(Set##TYPENAME, SINGULAR, CPPTYPE);                       \
    if (field->is_extension()) {                                             \
      return extensions_->Set##TYPENAME(field->number(), value);             \
    } else {                                                                 \
      SetField<TYPE>(field, value);                                          \
    }                                                                        \
  }                                                                          \
                                                                             \
  PASSTYPE GeneratedMessageReflection::GetRepeated##TYPENAME(                \
      const FieldDescriptor* field, int index) const {                       \
    USAGE_CHECK_ALL(GetRepeated##TYPENAME, REPEATED, CPPTYPE);               \
    if (field->is_extension()) {                                             \
      return extensions_->GetRepeated##TYPENAME(field->number(), index);     \
    } else {                                                                 \
      return GetRepeatedField<TYPE>(field, index);                           \
    }                                                                        \
  }                                                                          \
                                                                             \
  void GeneratedMessageReflection::SetRepeated##TYPENAME(                    \
      const FieldDescriptor* field, int index, PASSTYPE value) {             \
    USAGE_CHECK_ALL(SetRepeated##TYPENAME, REPEATED, CPPTYPE);               \
    if (field->is_extension()) {                                             \
      extensions_->SetRepeated##TYPENAME(field->number(), index, value);     \
    } else {                                                                 \
      SetRepeatedField<TYPE>(field, index, value);                           \
    }                                                                        \
  }                                                                          \
                                                                             \
  void GeneratedMessageReflection::Add##TYPENAME(                            \
      const FieldDescriptor* field, PASSTYPE value) {                        \
    USAGE_CHECK_ALL(Add##TYPENAME, REPEATED, CPPTYPE);                       \
    if (field->is_extension()) {                                             \
      extensions_->Add##TYPENAME(field->number(), value);                    \
    } else {                                                                 \
      AddField<TYPE>(field, value);                                          \
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
    const FieldDescriptor* field) const {
  USAGE_CHECK_ALL(GetString, SINGULAR, STRING);
  if (field->is_extension()) {
    return extensions_->GetString(field->number());
  } else {
    return *GetField<const string*>(field);
  }
}

const string& GeneratedMessageReflection::GetStringReference(
    const FieldDescriptor* field, string* scratch) const {
  USAGE_CHECK_ALL(GetStringReference, SINGULAR, STRING);
  if (field->is_extension()) {
    return extensions_->GetString(field->number());
  } else {
    return *GetField<const string*>(field);
  }
}


void GeneratedMessageReflection::SetString(
    const FieldDescriptor* field, const string& value) {
  USAGE_CHECK_ALL(SetString, SINGULAR, STRING);
  if (field->is_extension()) {
    return extensions_->SetString(field->number(), value);
  } else {
    string** ptr = MutableField<string*>(field);
    if (*ptr == DefaultRaw<const string*>(field)) {
      *ptr = new string(value);
    } else {
      (*ptr)->assign(value);
    }
  }
}


string GeneratedMessageReflection::GetRepeatedString(
    const FieldDescriptor* field, int index) const {
  USAGE_CHECK_ALL(GetRepeatedString, REPEATED, STRING);
  if (field->is_extension()) {
    return extensions_->GetRepeatedString(field->number(), index);
  } else {
    return GetRepeatedField<string>(field, index);
  }
}

const string& GeneratedMessageReflection::GetRepeatedStringReference(
    const FieldDescriptor* field, int index, string* scratch) const {
  USAGE_CHECK_ALL(GetRepeatedStringReference, REPEATED, STRING);
  if (field->is_extension()) {
    return extensions_->GetRepeatedString(field->number(), index);
  } else {
    return GetRepeatedField<string>(field, index);
  }
}


void GeneratedMessageReflection::SetRepeatedString(
    const FieldDescriptor* field, int index, const string& value) {
  USAGE_CHECK_ALL(SetRepeatedString, REPEATED, STRING);
  if (field->is_extension()) {
    extensions_->SetRepeatedString(field->number(), index, value);
  } else {
    SetRepeatedField<string>(field, index, value);
  }
}


void GeneratedMessageReflection::AddString(
    const FieldDescriptor* field, const string& value) {
  USAGE_CHECK_ALL(AddString, REPEATED, STRING);
  if (field->is_extension()) {
    extensions_->AddString(field->number(), value);
  } else {
    AddField<string>(field, value);
  }
}


// -------------------------------------------------------------------

const EnumValueDescriptor* GeneratedMessageReflection::GetEnum(
    const FieldDescriptor* field) const {
  USAGE_CHECK_ALL(GetEnum, SINGULAR, ENUM);

  int value;
  if (field->is_extension()) {
    value = extensions_->GetEnum(field->number());
  } else {
    value = GetField<int>(field);
  }
  const EnumValueDescriptor* result =
    field->enum_type()->FindValueByNumber(value);
  GOOGLE_CHECK(result != NULL);
  return result;
}

void GeneratedMessageReflection::SetEnum(const FieldDescriptor* field,
                                         const EnumValueDescriptor* value) {
  USAGE_CHECK_ALL(SetEnum, SINGULAR, ENUM);
  USAGE_CHECK_ENUM_VALUE(SetEnum);

  if (field->is_extension()) {
    extensions_->SetEnum(field->number(), value->number());
  } else {
    SetField<int>(field, value->number());
  }
}

const EnumValueDescriptor* GeneratedMessageReflection::GetRepeatedEnum(
    const FieldDescriptor* field, int index) const {
  USAGE_CHECK_ALL(GetRepeatedEnum, REPEATED, ENUM);

  int value;
  if (field->is_extension()) {
    value = extensions_->GetRepeatedEnum(field->number(), index);
  } else {
    value = GetRepeatedField<int>(field, index);
  }
  const EnumValueDescriptor* result =
    field->enum_type()->FindValueByNumber(value);
  GOOGLE_CHECK(result != NULL);
  return result;
}

void GeneratedMessageReflection::SetRepeatedEnum(
    const FieldDescriptor* field, int index,
    const EnumValueDescriptor* value) {
  USAGE_CHECK_ALL(SetRepeatedEnum, REPEATED, ENUM);
  USAGE_CHECK_ENUM_VALUE(SetRepeatedEnum);

  if (field->is_extension()) {
    extensions_->SetRepeatedEnum(field->number(), index, value->number());
  } else {
    SetRepeatedField<int>(field, index, value->number());
  }
}

void GeneratedMessageReflection::AddEnum(const FieldDescriptor* field,
                                         const EnumValueDescriptor* value) {
  USAGE_CHECK_ALL(AddEnum, REPEATED, ENUM);
  USAGE_CHECK_ENUM_VALUE(AddEnum);

  if (field->is_extension()) {
    extensions_->AddEnum(field->number(), value->number());
  } else {
    AddField<int>(field, value->number());
  }
}

// -------------------------------------------------------------------

const Message& GeneratedMessageReflection::GetMessage(
    const FieldDescriptor* field) const {
  USAGE_CHECK_ALL(GetMessage, SINGULAR, MESSAGE);

  if (field->is_extension()) {
    return extensions_->GetMessage(field->number());
  } else {
    const Message* result = GetRaw<const Message*>(field);
    if (result == NULL) {
      result = DefaultRaw<const Message*>(field);
    }
    return *result;
  }
}

Message* GeneratedMessageReflection::MutableMessage(
    const FieldDescriptor* field) {
  USAGE_CHECK_ALL(MutableMessage, SINGULAR, MESSAGE);

  if (field->is_extension()) {
    return extensions_->MutableMessage(field->number());
  } else {
    Message** result = MutableField<Message*>(field);
    if (*result == NULL) {
      const Message* default_message = DefaultRaw<const Message*>(field);
      *result = default_message->New();
      (*result)->CopyFrom(*default_message);
    }
    return *result;
  }
}

const Message& GeneratedMessageReflection::GetRepeatedMessage(
    const FieldDescriptor* field, int index) const {
  USAGE_CHECK_ALL(GetRepeatedMessage, REPEATED, MESSAGE);

  if (field->is_extension()) {
    return extensions_->GetRepeatedMessage(field->number(), index);
  } else {
    return GetRepeatedField<Message>(field, index);
  }
}

Message* GeneratedMessageReflection::MutableRepeatedMessage(
    const FieldDescriptor* field, int index) {
  USAGE_CHECK_ALL(MutableRepeatedMessage, REPEATED, MESSAGE);

  if (field->is_extension()) {
    return extensions_->MutableRepeatedMessage(field->number(), index);
  } else {
    return MutableRepeatedField<Message>(field, index);
  }
}

Message* GeneratedMessageReflection::AddMessage(const FieldDescriptor* field) {
  USAGE_CHECK_ALL(AddMessage, REPEATED, MESSAGE);

  if (field->is_extension()) {
    return extensions_->AddMessage(field->number());
  } else {
    return AddField<Message>(field);
  }
}

// -------------------------------------------------------------------

const FieldDescriptor* GeneratedMessageReflection::FindKnownExtensionByName(
    const string& name) const {
  if (extensions_ == NULL) return NULL;
  return extensions_->FindKnownExtensionByName(name);
}

const FieldDescriptor* GeneratedMessageReflection::FindKnownExtensionByNumber(
    int number) const {
  if (extensions_ == NULL) return NULL;
  return extensions_->FindKnownExtensionByNumber(number);
}

// ===================================================================
// Some private helpers.

// These simple template accessors obtain pointers (or references) to
// the given field.
template <typename Type>
inline const Type& GeneratedMessageReflection::GetRaw(
    const FieldDescriptor* field) const {
  const void* ptr = reinterpret_cast<const uint8*>(base_) +
                    offsets_[field->index()];
  return *reinterpret_cast<const Type*>(ptr);
}

template <typename Type>
inline Type* GeneratedMessageReflection::MutableRaw(
    const FieldDescriptor* field) {
  void* ptr = reinterpret_cast<uint8*>(base_) + offsets_[field->index()];
  return reinterpret_cast<Type*>(ptr);
}

template <typename Type>
inline const Type& GeneratedMessageReflection::DefaultRaw(
    const FieldDescriptor* field) const {
  const void* ptr = reinterpret_cast<const uint8*>(default_base_) +
                    offsets_[field->index()];
  return *reinterpret_cast<const Type*>(ptr);
}

// Simple accessors for manipulating has_bits_.
inline bool GeneratedMessageReflection::HasBit(
    const FieldDescriptor* field) const {
  return has_bits_[field->index() / 32] & (1 << (field->index() % 32));
}

inline void GeneratedMessageReflection::SetBit(
    const FieldDescriptor* field) {
  has_bits_[field->index() / 32] |= (1 << (field->index() % 32));
}

inline void GeneratedMessageReflection::ClearBit(
    const FieldDescriptor* field) {
  has_bits_[field->index() / 32] &= ~(1 << (field->index() % 32));
}

// Template implementations of basic accessors.  Inline because each
// template instance is only called from one location.  These are
// used for all types except messages.
template <typename Type>
inline const Type& GeneratedMessageReflection::GetField(
    const FieldDescriptor* field) const {
  return GetRaw<Type>(field);
}

template <typename Type>
inline void GeneratedMessageReflection::SetField(
    const FieldDescriptor* field, const Type& value) {
  *MutableRaw<Type>(field) = value;
  SetBit(field);
}

template <typename Type>
inline Type* GeneratedMessageReflection::MutableField(
    const FieldDescriptor* field) {
  SetBit(field);
  return MutableRaw<Type>(field);
}

template <typename Type>
inline const Type& GeneratedMessageReflection::GetRepeatedField(
    const FieldDescriptor* field, int index) const {
  return *reinterpret_cast<const Type*>(
    GetRaw<GenericRepeatedField>(field).GenericGet(index));
}

template <typename Type>
inline void GeneratedMessageReflection::SetRepeatedField(
    const FieldDescriptor* field, int index, const Type& value) {
  GenericRepeatedField* repeated = MutableRaw<GenericRepeatedField>(field);
  *reinterpret_cast<Type*>(repeated->GenericMutable(index)) = value;
}

template <typename Type>
inline Type* GeneratedMessageReflection::MutableRepeatedField(
    const FieldDescriptor* field, int index) {
  GenericRepeatedField* repeated = MutableRaw<GenericRepeatedField>(field);
  return reinterpret_cast<Type*>(repeated->GenericMutable(index));
}

template <typename Type>
inline void GeneratedMessageReflection::AddField(
    const FieldDescriptor* field, const Type& value) {
  GenericRepeatedField* repeated = MutableRaw<GenericRepeatedField>(field);
  *reinterpret_cast<Type*>(repeated->GenericAdd()) = value;
}

template <typename Type>
inline Type* GeneratedMessageReflection::AddField(
    const FieldDescriptor* field) {
  GenericRepeatedField* repeated = MutableRaw<GenericRepeatedField>(field);
  return reinterpret_cast<Type*>(repeated->GenericAdd());
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
