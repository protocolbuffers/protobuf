// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_HPB_GEN_UTILS_H__
#define GOOGLE_PROTOBUF_COMPILER_HPB_GEN_UTILS_H__

#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace hpb_generator {

enum class MessageClassType {
  kMessage,
  kMessageCProxy,
  kMessageProxy,
  kMessageAccess,
};

inline bool IsMapEntryMessage(const google::protobuf::Descriptor* descriptor) {
  return descriptor->options().map_entry();
}
std::vector<const google::protobuf::EnumDescriptor*> SortedEnums(
    const google::protobuf::FileDescriptor* file);
std::vector<const google::protobuf::Descriptor*> SortedMessages(
    const google::protobuf::FileDescriptor* file);
std::vector<const google::protobuf::FieldDescriptor*> SortedExtensions(
    const google::protobuf::FileDescriptor* file);
std::vector<const google::protobuf::FieldDescriptor*> FieldNumberOrder(
    const google::protobuf::Descriptor* message);

std::string ToCamelCase(absl::string_view input, bool lower_first);

std::string DefaultValue(const FieldDescriptor* field);

}  // namespace hpb_generator
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_HPB_GEN_UTILS_H__
