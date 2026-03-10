// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb_generator/file_layout.h"

#include <string>
#include <unordered_set>
#include <vector>

#include "upb/reflection/def.hpp"
#include "upb_generator/common.h"

namespace upb {
namespace generator {

const char* kEnumsInit = "enums_layout";
const char* kExtensionsInit = "extensions_layout";
const char* kMessagesInit = "messages_layout";

void AddEnums(upb::MessageDefPtr message, std::vector<upb::EnumDefPtr>* enums,
              WhichEnums which) {
  enums->reserve(enums->size() + message.enum_type_count());
  for (int i = 0; i < message.enum_type_count(); i++) {
    upb::EnumDefPtr enum_type = message.enum_type(i);
    if (which == kAllEnums || enum_type.is_closed()) {
      enums->push_back(message.enum_type(i));
    }
  }
  for (int i = 0; i < message.nested_message_count(); i++) {
    AddEnums(message.nested_message(i), enums, which);
  }
}

std::vector<upb::EnumDefPtr> SortedEnums(upb::FileDefPtr file,
                                         WhichEnums which) {
  std::vector<upb::EnumDefPtr> enums;
  enums.reserve(file.toplevel_enum_count());
  for (int i = 0; i < file.toplevel_enum_count(); i++) {
    upb::EnumDefPtr top_level_enum = file.toplevel_enum(i);
    if (which == kAllEnums || top_level_enum.is_closed()) {
      enums.push_back(file.toplevel_enum(i));
    }
  }
  for (int i = 0; i < file.toplevel_message_count(); i++) {
    AddEnums(file.toplevel_message(i), &enums, which);
  }
  std::sort(enums.begin(), enums.end(),
            [](upb::EnumDefPtr a, upb::EnumDefPtr b) {
              return strcmp(a.full_name(), b.full_name()) < 0;
            });
  return enums;
}

std::vector<uint32_t> SortedUniqueEnumNumbers(upb::EnumDefPtr e) {
  std::vector<uint32_t> values;
  values.reserve(e.value_count());
  for (int i = 0; i < e.value_count(); i++) {
    values.push_back(static_cast<uint32_t>(e.value(i).number()));
  }
  std::sort(values.begin(), values.end());
  auto last = std::unique(values.begin(), values.end());
  values.erase(last, values.end());
  return values;
}

void AddMessages(upb::MessageDefPtr message,
                 std::vector<upb::MessageDefPtr>* messages) {
  messages->push_back(message);
  for (int i = 0; i < message.nested_message_count(); i++) {
    AddMessages(message.nested_message(i), messages);
  }
}

// Ordering must match upb/def.c!
//
// The ordering is significant because each upb_MessageDef* will point at the
// corresponding upb_MiniTable and we just iterate through the list without
// any search or lookup.
std::vector<upb::MessageDefPtr> SortedMessages(upb::FileDefPtr file) {
  std::vector<upb::MessageDefPtr> messages;
  for (int i = 0; i < file.toplevel_message_count(); i++) {
    AddMessages(file.toplevel_message(i), &messages);
  }
  return messages;
}

void AddExtensionsFromMessage(upb::MessageDefPtr message,
                              std::vector<upb::FieldDefPtr>* exts) {
  for (int i = 0; i < message.nested_extension_count(); i++) {
    exts->push_back(message.nested_extension(i));
  }
  for (int i = 0; i < message.nested_message_count(); i++) {
    AddExtensionsFromMessage(message.nested_message(i), exts);
  }
}

// Ordering must match upb/def.c!
//
// The ordering is significant because each upb_FieldDef* will point at the
// corresponding upb_MiniTableExtension and we just iterate through the list
// without any search or lookup.
std::vector<upb::FieldDefPtr> SortedExtensions(upb::FileDefPtr file) {
  std::vector<upb::FieldDefPtr> ret;
  ret.reserve(file.toplevel_extension_count());
  for (int i = 0; i < file.toplevel_extension_count(); i++) {
    ret.push_back(file.toplevel_extension(i));
  }
  for (int i = 0; i < file.toplevel_message_count(); i++) {
    AddExtensionsFromMessage(file.toplevel_message(i), &ret);
  }
  return ret;
}

std::vector<upb::FieldDefPtr> FieldNumberOrder(upb::MessageDefPtr message) {
  std::vector<upb::FieldDefPtr> fields;
  fields.reserve(message.field_count());
  for (int i = 0; i < message.field_count(); i++) {
    fields.push_back(message.field(i));
  }
  std::sort(fields.begin(), fields.end(),
            [](upb::FieldDefPtr a, upb::FieldDefPtr b) {
              return a.number() < b.number();
            });
  return fields;
}

}  // namespace generator
}  // namespace upb
