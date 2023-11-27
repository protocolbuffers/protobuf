// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "protos_generator/gen_utils.h"

#include <algorithm>
#include <string>
#include <vector>

#include "absl/strings/ascii.h"

namespace protos_generator {

namespace protobuf = ::google::protobuf;

void AddEnums(const protobuf::Descriptor* message,
              std::vector<const protobuf::EnumDescriptor*>* enums) {
  enums->reserve(enums->size() + message->enum_type_count());
  for (int i = 0; i < message->enum_type_count(); i++) {
    enums->push_back(message->enum_type(i));
  }
  for (int i = 0; i < message->nested_type_count(); i++) {
    AddEnums(message->nested_type(i), enums);
  }
}

std::vector<const protobuf::EnumDescriptor*> SortedEnums(
    const protobuf::FileDescriptor* file) {
  std::vector<const protobuf::EnumDescriptor*> enums;
  enums.reserve(file->enum_type_count());
  for (int i = 0; i < file->enum_type_count(); i++) {
    enums.push_back(file->enum_type(i));
  }
  for (int i = 0; i < file->message_type_count(); i++) {
    AddEnums(file->message_type(i), &enums);
  }
  return enums;
}

void AddMessages(const protobuf::Descriptor* message,
                 std::vector<const protobuf::Descriptor*>* messages) {
  messages->push_back(message);
  for (int i = 0; i < message->nested_type_count(); i++) {
    AddMessages(message->nested_type(i), messages);
  }
}

std::vector<const protobuf::Descriptor*> SortedMessages(
    const protobuf::FileDescriptor* file) {
  std::vector<const protobuf::Descriptor*> messages;
  for (int i = 0; i < file->message_type_count(); i++) {
    AddMessages(file->message_type(i), &messages);
  }
  return messages;
}

void AddExtensionsFromMessage(
    const protobuf::Descriptor* message,
    std::vector<const protobuf::FieldDescriptor*>* exts) {
  for (int i = 0; i < message->extension_count(); i++) {
    exts->push_back(message->extension(i));
  }
  for (int i = 0; i < message->nested_type_count(); i++) {
    AddExtensionsFromMessage(message->nested_type(i), exts);
  }
}

std::vector<const protobuf::FieldDescriptor*> SortedExtensions(
    const protobuf::FileDescriptor* file) {
  const int extension_count = file->extension_count();
  const int message_type_count = file->message_type_count();

  std::vector<const protobuf::FieldDescriptor*> ret;
  ret.reserve(extension_count + message_type_count);

  for (int i = 0; i < extension_count; i++) {
    ret.push_back(file->extension(i));
  }
  for (int i = 0; i < message_type_count; i++) {
    AddExtensionsFromMessage(file->message_type(i), &ret);
  }

  return ret;
}

std::vector<const protobuf::FieldDescriptor*> FieldNumberOrder(
    const protobuf::Descriptor* message) {
  std::vector<const protobuf::FieldDescriptor*> fields;
  fields.reserve(message->field_count());
  for (int i = 0; i < message->field_count(); i++) {
    fields.push_back(message->field(i));
  }
  std::sort(fields.begin(), fields.end(),
            [](const protobuf::FieldDescriptor* a,
               const protobuf::FieldDescriptor* b) {
              return a->number() < b->number();
            });
  return fields;
}

std::string ToCamelCase(const std::string& input, bool lower_first) {
  bool capitalize_next = !lower_first;
  std::string result;
  result.reserve(input.size());

  for (char character : input) {
    if (character == '_') {
      capitalize_next = true;
    } else if (capitalize_next) {
      result.push_back(absl::ascii_toupper(character));
      capitalize_next = false;
    } else {
      result.push_back(character);
    }
  }

  // Lower-case the first letter.
  if (lower_first && !result.empty()) {
    result[0] = absl::ascii_tolower(result[0]);
  }

  return result;
}

}  // namespace protos_generator
