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

void ReflectionOps::Copy(const Message& from, Message* to) {
  if (&from == to) return;
  Clear(to);
  Merge(from, to);
}

void ReflectionOps::Merge(const Message& from, Message* to) {
  GOOGLE_CHECK_NE(&from, to);

  const Descriptor* descriptor = from.GetDescriptor();
  GOOGLE_CHECK_EQ(to->GetDescriptor(), descriptor)
    << "Tried to merge messages of different types.";

  const Reflection* from_reflection = from.GetReflection();
  const Reflection* to_reflection = to->GetReflection();

  vector<const FieldDescriptor*> fields;
  from_reflection->ListFields(from, &fields);
  for (int i = 0; i < fields.size(); i++) {
    const FieldDescriptor* field = fields[i];

    if (field->is_repeated()) {
      int count = from_reflection->FieldSize(from, field);
      for (int j = 0; j < count; j++) {
        switch (field->cpp_type()) {
#define HANDLE_TYPE(CPPTYPE, METHOD)                                     \
          case FieldDescriptor::CPPTYPE_##CPPTYPE:                       \
            to_reflection->Add##METHOD(to, field,                        \
              from_reflection->GetRepeated##METHOD(from, field, j));     \
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
            to_reflection->AddMessage(to, field)->MergeFrom(
              from_reflection->GetRepeatedMessage(from, field, j));
            break;
        }
      }
    } else {
      switch (field->cpp_type()) {
#define HANDLE_TYPE(CPPTYPE, METHOD)                                        \
        case FieldDescriptor::CPPTYPE_##CPPTYPE:                            \
          to_reflection->Set##METHOD(to, field,                             \
            from_reflection->Get##METHOD(from, field));                     \
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
          to_reflection->MutableMessage(to, field)->MergeFrom(
            from_reflection->GetMessage(from, field));
          break;
      }
    }
  }

  to_reflection->MutableUnknownFields(to)->MergeFrom(
    from_reflection->GetUnknownFields(from));
}

void ReflectionOps::Clear(Message* message) {
  const Reflection* reflection = message->GetReflection();

  vector<const FieldDescriptor*> fields;
  reflection->ListFields(*message, &fields);
  for (int i = 0; i < fields.size(); i++) {
    reflection->ClearField(message, fields[i]);
  }

  reflection->MutableUnknownFields(message)->Clear();
}

bool ReflectionOps::IsInitialized(const Message& message) {
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* reflection = message.GetReflection();

  // Check required fields of this message.
  for (int i = 0; i < descriptor->field_count(); i++) {
    if (descriptor->field(i)->is_required()) {
      if (!reflection->HasField(message, descriptor->field(i))) {
        return false;
      }
    }
  }

  // Check that sub-messages are initialized.
  vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);
  for (int i = 0; i < fields.size(); i++) {
    const FieldDescriptor* field = fields[i];
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (field->is_repeated()) {
        int size = reflection->FieldSize(message, field);

        for (int i = 0; i < size; i++) {
          if (!reflection->GetRepeatedMessage(message, field, i)
                          .IsInitialized()) {
            return false;
          }
        }
      } else {
        if (!reflection->GetMessage(message, field).IsInitialized()) {
          return false;
        }
      }
    }
  }

  return true;
}

void ReflectionOps::DiscardUnknownFields(Message* message) {
  const Reflection* reflection = message->GetReflection();

  reflection->MutableUnknownFields(message)->Clear();

  vector<const FieldDescriptor*> fields;
  reflection->ListFields(*message, &fields);
  for (int i = 0; i < fields.size(); i++) {
    const FieldDescriptor* field = fields[i];
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (field->is_repeated()) {
        int size = reflection->FieldSize(*message, field);
        for (int i = 0; i < size; i++) {
          reflection->MutableRepeatedMessage(message, field, i)
                    ->DiscardUnknownFields();
        }
      } else {
        reflection->MutableMessage(message, field)->DiscardUnknownFields();
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
    const Message& message,
    const string& prefix,
    vector<string>* errors) {
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* reflection = message.GetReflection();

  // Check required fields of this message.
  for (int i = 0; i < descriptor->field_count(); i++) {
    if (descriptor->field(i)->is_required()) {
      if (!reflection->HasField(message, descriptor->field(i))) {
        errors->push_back(prefix + descriptor->field(i)->name());
      }
    }
  }

  // Check sub-messages.
  vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);
  for (int i = 0; i < fields.size(); i++) {
    const FieldDescriptor* field = fields[i];
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {

      if (field->is_repeated()) {
        int size = reflection->FieldSize(message, field);

        for (int i = 0; i < size; i++) {
          const Message& sub_message =
            reflection->GetRepeatedMessage(message, field, i);
          FindInitializationErrors(sub_message,
                                   SubMessagePrefix(prefix, field, i),
                                   errors);
        }
      } else {
        const Message& sub_message = reflection->GetMessage(message, field);
        FindInitializationErrors(sub_message,
                                 SubMessagePrefix(prefix, field, -1),
                                 errors);
      }
    }
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
