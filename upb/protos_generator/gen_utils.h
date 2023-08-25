// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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
//     * Neither the name of Google LLC nor the names of its
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
