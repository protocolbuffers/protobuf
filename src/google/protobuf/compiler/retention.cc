// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

#include "google/protobuf/compiler/retention.h"

#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"

namespace google {
namespace protobuf {
namespace compiler {

namespace {
// Recursively strips any options with source retention from the message.
void StripMessage(Message& m) {
  const Reflection* reflection = m.GetReflection();
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(m, &fields);
  for (const FieldDescriptor* field : fields) {
    if (field->options().retention() == FieldOptions::RETENTION_SOURCE) {
      reflection->ClearField(&m, field);
    } else if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
      if (field->is_repeated()) {
        int field_size = reflection->FieldSize(m, field);
        for (int i = 0; i < field_size; ++i) {
          StripMessage(*reflection->MutableRepeatedMessage(&m, field, i));
        }
      } else {
        StripMessage(*reflection->MutableMessage(&m, field));
      }
    }
  }
}
}  // namespace

FileDescriptorProto StripSourceRetentionOptions(const FileDescriptor& file) {
  FileDescriptorProto file_proto;
  file.CopyTo(&file_proto);

  // We need to look up the descriptor in file.pool() so that we can get a
  // descriptor which knows about any custom options that were used in the
  // .proto file.
  const Descriptor* descriptor =
      file.pool()->FindMessageTypeByName(FileDescriptorProto().GetTypeName());

  if (descriptor == nullptr) {
    // If the pool does not contain the descriptor for FileDescriptorProto,
    // then this proto file does not transitively depend on descriptor.proto,
    // in which case we know there are no custom options to worry about.
    StripMessage(file_proto);
  } else {
    // The options message may have custom options set on it, and these would
    // ordinarily appear as unknown fields since they are not linked into
    // protoc. Using a dynamic message allows us to see these custom options.
    // To convert back and forth between the generated type and the dynamic
    // message, we have to serialize one and parse that into the other.
    DynamicMessageFactory factory;
    std::unique_ptr<Message> dynamic_message(
        factory.GetPrototype(descriptor)->New());
    std::string serialized;
    ABSL_CHECK(file_proto.SerializeToString(&serialized));
    ABSL_CHECK(dynamic_message->ParseFromString(serialized));
    StripMessage(*dynamic_message);
    ABSL_CHECK(dynamic_message->SerializeToString(&serialized));
    ABSL_CHECK(file_proto.ParseFromString(serialized));
  }

  return file_proto;
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
