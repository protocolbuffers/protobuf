// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_PROTOS_GENERATOR_GEN_UTILS_H_
#define UPB_PROTOS_GENERATOR_GEN_UTILS_H_

#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"

namespace protos_generator {

namespace protobuf = ::google::protobuf;

enum class MessageClassType {
  kMessage,
  kMessageCProxy,
  kMessageProxy,
  kMessageAccess,
};

inline bool IsMapEntryMessage(const protobuf::Descriptor* descriptor) {
  return descriptor->options().map_entry();
}
std::vector<const protobuf::EnumDescriptor*> SortedEnums(
    const protobuf::FileDescriptor* file);
std::vector<const protobuf::Descriptor*> SortedMessages(
    const protobuf::FileDescriptor* file);
std::vector<const protobuf::FieldDescriptor*> SortedExtensions(
    const protobuf::FileDescriptor* file);
std::vector<const protobuf::FieldDescriptor*> FieldNumberOrder(
    const protobuf::Descriptor* message);

std::string ToCamelCase(const std::string& input, bool lower_first);

}  // namespace protos_generator

#endif  // UPB_PROTOS_GENERATOR_GEN_UTILS_H_
