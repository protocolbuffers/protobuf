// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
#include "google/protobuf/reflection_ops.h"

#include <string>
#include <vector>

#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/map_field.h"
#include "google/protobuf/port.h"
#include "google/protobuf/unknown_field_set.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

static const Reflection* GetReflectionOrDie(const Message& m) {
  const Reflection* r = m.GetReflection();
  if (r == nullptr) {
    const Descriptor* d = m.GetDescriptor();
    // RawMessage is one known type for which GetReflection() returns nullptr.
    ABSL_LOG(FATAL) << "Message does not support reflection (type "
                    << (d ? d->name() : "unknown") << ").";
  }
  return r;
}

void ReflectionOps::Copy(const Message& from, Message* to) {
  if (&from == to) return;
  Clear(to);
  Merge(from, to);
}

void ReflectionOps::Merge(const Message& from, Message* to) {
  ABSL_CHECK_NE(&from, to);

  const Descriptor* descriptor = from.GetDescriptor();
  ABSL_CHECK_EQ(to->GetDescriptor(), descriptor)
      << "Tried to merge messages of different types "
      << "(merge " << descriptor->full_name() << " to "
      << to->GetDescriptor()->full_name() << ")";

  const Reflection* from_reflection = GetReflectionOrDie(from);
  const Reflection* to_reflection = GetReflectionOrDie(*to);
  bool is_from_generated = (from_reflection->GetMessageFactory() ==
                            google::protobuf::MessageFactory::generated_factory());
  bool is_to_generated = (to_reflection->GetMessageFactory() ==
                          google::protobuf::MessageFactory::generated_factory());

  std::vector<const FieldDescriptor*> fields;
  from_reflection->ListFields(from, &fields);
  for (const FieldDescriptor* field : fields) {
    if (field->is_repeated()) {
      // Use map reflection if both are in map status and have the
      // same map type to avoid sync with repeated field.
      // Note: As from and to messages have the same descriptor, the
      // map field types are the same if they are both generated
      // messages or both dynamic messages.
      if (is_from_generated == is_to_generated && field->is_map()) {
        const MapFieldBase* from_field =
            from_reflection->GetMapData(from, field);
        MapFieldBase* to_field = to_reflection->MutableMapData(to, field);
        if (to_field->IsMapValid() && from_field->IsMapValid()) {
          to_field->MergeFrom(*from_field);
          continue;
        }
      }
      int count = from_reflection->FieldSize(from, field);
      for (int j = 0; j < count; j++) {
        switch (field->cpp_type()) {
#define HANDLE_TYPE(CPPTYPE, METHOD)                                      \
  case FieldDescriptor::CPPTYPE_##CPPTYPE:                                \
    to_reflection->Add##METHOD(                                           \
        to, field, from_reflection->GetRepeated##METHOD(from, field, j)); \
    break;

          HANDLE_TYPE(INT32, Int32);
          HANDLE_TYPE(INT64, Int64);
          HANDLE_TYPE(UINT32, UInt32);
          HANDLE_TYPE(UINT64, UInt64);
          HANDLE_TYPE(FLOAT, Float);
          HANDLE_TYPE(DOUBLE, Double);
          HANDLE_TYPE(BOOL, Bool);
          HANDLE_TYPE(STRING, String);
          HANDLE_TYPE(ENUM, Enum);
#undef HANDLE_TYPE

          case FieldDescriptor::CPPTYPE_MESSAGE:
            const Message& from_child =
                from_reflection->GetRepeatedMessage(from, field, j);
            if (from_reflection == to_reflection) {
              to_reflection
                  ->AddMessage(to, field,
                               from_child.GetReflection()->GetMessageFactory())
                  ->MergeFrom(from_child);
            } else {
              to_reflection->AddMessage(to, field)->MergeFrom(from_child);
            }
            break;
        }
      }
    } else {
      switch (field->cpp_type()) {
#define HANDLE_TYPE(CPPTYPE, METHOD)                                       \
  case FieldDescriptor::CPPTYPE_##CPPTYPE:                                 \
    to_reflection->Set##METHOD(to, field,                                  \
                               from_reflection->Get##METHOD(from, field)); \
    break;

        HANDLE_TYPE(INT32, Int32);
        HANDLE_TYPE(INT64, Int64);
        HANDLE_TYPE(UINT32, UInt32);
        HANDLE_TYPE(UINT64, UInt64);
        HANDLE_TYPE(FLOAT, Float);
        HANDLE_TYPE(DOUBLE, Double);
        HANDLE_TYPE(BOOL, Bool);
        HANDLE_TYPE(STRING, String);
        HANDLE_TYPE(ENUM, Enum);
#undef HANDLE_TYPE

        case FieldDescriptor::CPPTYPE_MESSAGE:
          const Message& from_child = from_reflection->GetMessage(from, field);
          if (from_reflection == to_reflection) {
            to_reflection
                ->MutableMessage(
                    to, field, from_child.GetReflection()->GetMessageFactory())
                ->MergeFrom(from_child);
          } else {
            to_reflection->MutableMessage(to, field)->MergeFrom(from_child);
          }
          break;
      }
    }
  }

  if (!from_reflection->GetUnknownFields(from).empty()) {
    to_reflection->MutableUnknownFields(to)->MergeFrom(
        from_reflection->GetUnknownFields(from));
  }
}

void ReflectionOps::Clear(Message* message) {
  const Reflection* reflection = GetReflectionOrDie(*message);

  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(*message, &fields);
  for (const FieldDescriptor* field : fields) {
    reflection->ClearField(message, field);
  }

  if (reflection->GetInternalMetadata(*message).have_unknown_fields()) {
    reflection->MutableUnknownFields(message)->Clear();
  }
}

bool ReflectionOps::IsInitialized(const Message& message, bool check_fields,
                                  bool check_descendants) {
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* reflection = GetReflectionOrDie(message);
  if (const int field_count = descriptor->field_count()) {
    const FieldDescriptor* begin = descriptor->field(0);
    const FieldDescriptor* end = begin + field_count;
    ABSL_DCHECK_EQ(descriptor->field(field_count - 1), end - 1);

    if (check_fields) {
      // Check required fields of this message.
      for (const FieldDescriptor* field = begin; field != end; ++field) {
        if (field->is_required() && !reflection->HasField(message, field)) {
          return false;
        }
      }
    }

    if (check_descendants) {
      for (const FieldDescriptor* field = begin; field != end; ++field) {
        if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
          const Descriptor* message_type = field->message_type();
          if (ABSL_PREDICT_FALSE(message_type->options().map_entry())) {
            if (message_type->field(1)->cpp_type() ==
                FieldDescriptor::CPPTYPE_MESSAGE) {
              const MapFieldBase* map_field =
                  reflection->GetMapData(message, field);
              if (map_field->IsMapValid()) {
                MapIterator it(const_cast<Message*>(&message), field);
                MapIterator end_map(const_cast<Message*>(&message), field);
                for (map_field->MapBegin(&it), map_field->MapEnd(&end_map);
                     it != end_map; ++it) {
                  if (!it.GetValueRef().GetMessageValue().IsInitialized()) {
                    return false;
                  }
                }
              }
            }
          } else if (field->is_repeated()) {
            const int size = reflection->FieldSize(message, field);
            for (int j = 0; j < size; j++) {
              if (!reflection->GetRepeatedMessage(message, field, j)
                       .IsInitialized()) {
                return false;
              }
            }
          } else if (reflection->HasField(message, field)) {
            if (!reflection->GetMessage(message, field).IsInitialized()) {
              return false;
            }
          }
        }
      }
    }
  }
  if (check_descendants && reflection->HasExtensionSet(message)) {
    // Note that "extendee" is only referenced if the extension is lazily parsed
    // (e.g. LazyMessageExtensionImpl), which requires a verification function
    // to be generated.
    //
    // Dynamic messages would get null prototype from the generated message
    // factory but their verification functions are not generated. Therefore, it
    // it will always be eagerly parsed and "extendee" here will not be
    // referenced.
    const Message* extendee =
        MessageFactory::generated_factory()->GetPrototype(descriptor);
    if (!reflection->GetExtensionSet(message).IsInitialized(extendee)) {
      return false;
    }
  }
  return true;
}

bool ReflectionOps::IsInitialized(const Message& message) {
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* reflection = GetReflectionOrDie(message);

  // Check required fields of this message.
  {
    const int field_count = descriptor->field_count();
    for (int i = 0; i < field_count; i++) {
      if (descriptor->field(i)->is_required()) {
        if (!reflection->HasField(message, descriptor->field(i))) {
          return false;
        }
      }
    }
  }

  // Check that sub-messages are initialized.
  std::vector<const FieldDescriptor*> fields;
  // Should be safe to skip stripped fields because required fields are not
  // stripped.
  if (descriptor->options().map_entry()) {
    // MapEntry objects always check the value regardless of has bit.
    // We don't need to bother with the key.
    fields = {descriptor->map_value()};
  } else {
    reflection->ListFields(message, &fields);
  }
  for (const FieldDescriptor* field : fields) {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {

      if (field->is_map()) {
        const FieldDescriptor* value_field = field->message_type()->field(1);
        if (value_field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
          const MapFieldBase* map_field =
              reflection->GetMapData(message, field);
          if (map_field->IsMapValid()) {
            MapIterator iter(const_cast<Message*>(&message), field);
            MapIterator end(const_cast<Message*>(&message), field);
            for (map_field->MapBegin(&iter), map_field->MapEnd(&end);
                 iter != end; ++iter) {
              if (!iter.GetValueRef().GetMessageValue().IsInitialized()) {
                return false;
              }
            }
            continue;
          }
        } else {
          continue;
        }
      }

      if (field->is_repeated()) {
        int size = reflection->FieldSize(message, field);

        for (int j = 0; j < size; j++) {
          if (!reflection->GetRepeatedMessage(message, field, j)
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

static bool IsMapValueMessageTyped(const FieldDescriptor* map_field) {
  return map_field->message_type()->field(1)->cpp_type() ==
         FieldDescriptor::CPPTYPE_MESSAGE;
}

void ReflectionOps::DiscardUnknownFields(Message* message) {
  const Reflection* reflection = GetReflectionOrDie(*message);

  if (reflection->GetUnknownFields(*message).field_count() != 0) {
    reflection->MutableUnknownFields(message)->Clear();
  }

  // Walk through the fields of this message and DiscardUnknownFields on any
  // messages present.
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(*message, &fields);
  for (const FieldDescriptor* field : fields) {
    // Skip over non-message fields.
    if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
      continue;
    }
    // Discard the unknown fields in maps that contain message values.
    const MapFieldBase* map_field =
        field->is_map() ? reflection->MutableMapData(message, field) : nullptr;
    if (map_field != nullptr && map_field->IsMapValid()) {
      if (IsMapValueMessageTyped(field)) {
        MapIterator iter(message, field);
        MapIterator end(message, field);
        for (map_field->MapBegin(&iter), map_field->MapEnd(&end); iter != end;
             ++iter) {
          iter.MutableValueRef()->MutableMessageValue()->DiscardUnknownFields();
        }
      }
      // Discard every unknown field inside messages in a repeated field.
    } else if (field->is_repeated()) {
      int size = reflection->FieldSize(*message, field);
      for (int j = 0; j < size; j++) {
        reflection->MutableRepeatedMessage(message, field, j)
            ->DiscardUnknownFields();
      }
      // Discard the unknown fields inside an optional message.
    } else {
      reflection->MutableMessage(message, field)->DiscardUnknownFields();
    }
  }
}

static std::string SubMessagePrefix(const std::string& prefix,
                                    const FieldDescriptor* field, int index) {
  std::string result(prefix);
  if (field->is_extension()) {
    absl::StrAppend(&result, "(", field->full_name(), ")");
  } else {
    absl::StrAppend(&result, field->name());
  }
  if (index != -1) {
    absl::StrAppend(&result, "[", index, "]");
  }
  result.append(".");
  return result;
}

void ReflectionOps::FindInitializationErrors(const Message& message,
                                             const std::string& prefix,
                                             std::vector<std::string>* errors) {
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* reflection = GetReflectionOrDie(message);

  // Check required fields of this message.
  {
    const int field_count = descriptor->field_count();
    for (int i = 0; i < field_count; i++) {
      if (descriptor->field(i)->is_required()) {
        if (!reflection->HasField(message, descriptor->field(i))) {
          errors->push_back(absl::StrCat(prefix, descriptor->field(i)->name()));
        }
      }
    }
  }

  // Check sub-messages.
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);
  for (const FieldDescriptor* field : fields) {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {

      if (field->is_repeated()) {
        int size = reflection->FieldSize(message, field);

        for (int j = 0; j < size; j++) {
          const Message& sub_message =
              reflection->GetRepeatedMessage(message, field, j);
          FindInitializationErrors(sub_message,
                                   SubMessagePrefix(prefix, field, j), errors);
        }
      } else {
        const Message& sub_message = reflection->GetMessage(message, field);
        FindInitializationErrors(sub_message,
                                 SubMessagePrefix(prefix, field, -1), errors);
      }
    }
  }
}

void GenericSwap(Message* lhs, Message* rhs) {
  if (!internal::DebugHardenForceCopyInSwap()) {
    ABSL_DCHECK(lhs->GetArena() != rhs->GetArena());
    ABSL_DCHECK(lhs->GetArena() != nullptr || rhs->GetArena() != nullptr);
  }
  // At least one of these must have an arena, so make `rhs` point to it.
  Arena* arena = rhs->GetArena();
  if (arena == nullptr) {
    std::swap(lhs, rhs);
    arena = rhs->GetArena();
  }

  // Improve efficiency by placing the temporary on an arena so that messages
  // are copied twice rather than three times.
  Message* tmp = rhs->New(arena);
  tmp->CheckTypeAndMergeFrom(*lhs);
  lhs->Clear();
  lhs->CheckTypeAndMergeFrom(*rhs);
  if (internal::DebugHardenForceCopyInSwap()) {
    rhs->Clear();
    rhs->CheckTypeAndMergeFrom(*tmp);
    if (arena == nullptr) delete tmp;
  } else {
    rhs->GetReflection()->Swap(tmp, rhs);
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
