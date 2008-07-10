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

#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace internal {

void ReflectionOps::Copy(const Descriptor* descriptor,
                         const Message::Reflection& from,
                         Message::Reflection* to) {
  if (&from == to) return;
  Clear(descriptor, to);
  Merge(descriptor, from, to);
}

void ReflectionOps::Merge(const Descriptor* descriptor,
                          const Message::Reflection& from,
                          Message::Reflection* to) {
  GOOGLE_CHECK_NE(&from, to);
  vector<const FieldDescriptor*> fields;
  from.ListFields(&fields);
  for (int i = 0; i < fields.size(); i++) {
    const FieldDescriptor* field = fields[i];

    if (field->is_repeated()) {
      int count = from.FieldSize(field);
      for (int j = 0; j < count; j++) {
        switch (field->cpp_type()) {
#define HANDLE_TYPE(CPPTYPE, METHOD)                                     \
          case FieldDescriptor::CPPTYPE_##CPPTYPE:                       \
            to->Add##METHOD(field,                                       \
              from.GetRepeated##METHOD(field, j));                       \
            break;

          HANDLE_TYPE(INT32 , Int32 );
          HANDLE_TYPE(INT64 , Int64 );
          HANDLE_TYPE(UINT32, UInt32);
          HANDLE_TYPE(UINT64, UInt64);
          HANDLE_TYPE(FLOAT , Float );
          HANDLE_TYPE(DOUBLE, Double);
          HANDLE_TYPE(BOOL  , Bool  );
          HANDLE_TYPE(STRING, String);
          HANDLE_TYPE(ENUM  , Enum  );
#undef HANDLE_TYPE

          case FieldDescriptor::CPPTYPE_MESSAGE:
            to->AddMessage(field)->MergeFrom(
              from.GetRepeatedMessage(field, j));
            break;
        }
      }
    } else if (from.HasField(field)) {
      switch (field->cpp_type()) {
#define HANDLE_TYPE(CPPTYPE, METHOD)                                        \
        case FieldDescriptor::CPPTYPE_##CPPTYPE:                            \
          to->Set##METHOD(field, from.Get##METHOD(field));                  \
          break;

        HANDLE_TYPE(INT32 , Int32 );
        HANDLE_TYPE(INT64 , Int64 );
        HANDLE_TYPE(UINT32, UInt32);
        HANDLE_TYPE(UINT64, UInt64);
        HANDLE_TYPE(FLOAT , Float );
        HANDLE_TYPE(DOUBLE, Double);
        HANDLE_TYPE(BOOL  , Bool  );
        HANDLE_TYPE(STRING, String);
        HANDLE_TYPE(ENUM  , Enum  );
#undef HANDLE_TYPE

        case FieldDescriptor::CPPTYPE_MESSAGE:
          to->MutableMessage(field)->MergeFrom(
            from.GetMessage(field));
          break;
      }
    }
  }

  to->MutableUnknownFields()->MergeFrom(from.GetUnknownFields());
}

void ReflectionOps::Clear(const Descriptor* descriptor,
                          Message::Reflection* reflection) {
  vector<const FieldDescriptor*> fields;
  reflection->ListFields(&fields);
  for (int i = 0; i < fields.size(); i++) {
    reflection->ClearField(fields[i]);
  }

  reflection->MutableUnknownFields()->Clear();
}

bool ReflectionOps::IsInitialized(const Descriptor* descriptor,
                                  const Message::Reflection& reflection) {
  // Check required fields of this message.
  for (int i = 0; i < descriptor->field_count(); i++) {
    if (descriptor->field(i)->is_required()) {
      if (!reflection.HasField(descriptor->field(i))) {
        return false;
      }
    }
  }

  // Check that sub-messages are initialized.
  vector<const FieldDescriptor*> fields;
  reflection.ListFields(&fields);
  for (int i = 0; i < fields.size(); i++) {
    const FieldDescriptor* field = fields[i];
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (field->is_repeated()) {
        int size = reflection.FieldSize(field);

        for (int i = 0; i < size; i++) {
          if (!reflection.GetRepeatedMessage(field, i).IsInitialized()) {
            return false;
          }
        }
      } else {
        if (reflection.HasField(field) &&
            !reflection.GetMessage(field).IsInitialized()) {
          return false;
        }
      }
    }
  }

  return true;
}

void ReflectionOps::DiscardUnknownFields(
    const Descriptor* descriptor,
    Message::Reflection* reflection) {
  reflection->MutableUnknownFields()->Clear();

  vector<const FieldDescriptor*> fields;
  reflection->ListFields(&fields);
  for (int i = 0; i < fields.size(); i++) {
    const FieldDescriptor* field = fields[i];
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (field->is_repeated()) {
        int size = reflection->FieldSize(field);
        for (int i = 0; i < size; i++) {
          reflection->MutableRepeatedMessage(field, i)->DiscardUnknownFields();
        }
      } else {
        if (reflection->HasField(field)) {
          reflection->MutableMessage(field)->DiscardUnknownFields();
        }
      }
    }
  }
}

static string SubMessagePrefix(const string& prefix,
                               const FieldDescriptor* field,
                               int index) {
  string result(prefix);
  if (field->is_extension()) {
    result.append("(");
    result.append(field->full_name());
    result.append(")");
  } else {
    result.append(field->name());
  }
  if (index != -1) {
    result.append("[");
    result.append(SimpleItoa(index));
    result.append("]");
  }
  result.append(".");
  return result;
}

void ReflectionOps::FindInitializationErrors(
    const Descriptor* descriptor,
    const Message::Reflection& reflection,
    const string& prefix,
    vector<string>* errors) {
  // Check required fields of this message.
  for (int i = 0; i < descriptor->field_count(); i++) {
    if (descriptor->field(i)->is_required()) {
      if (!reflection.HasField(descriptor->field(i))) {
        errors->push_back(prefix + descriptor->field(i)->name());
      }
    }
  }

  // Check sub-messages.
  vector<const FieldDescriptor*> fields;
  reflection.ListFields(&fields);
  for (int i = 0; i < fields.size(); i++) {
    const FieldDescriptor* field = fields[i];
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {

      if (field->is_repeated()) {
        int size = reflection.FieldSize(field);

        for (int i = 0; i < size; i++) {
          const Message& sub_message = reflection.GetRepeatedMessage(field, i);
          FindInitializationErrors(field->message_type(),
                                   *sub_message.GetReflection(),
                                   SubMessagePrefix(prefix, field, i),
                                   errors);
        }
      } else {
        if (reflection.HasField(field)) {
          const Message& sub_message = reflection.GetMessage(field);
          FindInitializationErrors(field->message_type(),
                                   *sub_message.GetReflection(),
                                   SubMessagePrefix(prefix, field, -1),
                                   errors);
        }
      }
    }
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
