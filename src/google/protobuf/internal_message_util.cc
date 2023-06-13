// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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

#include "google/protobuf/internal_message_util.h"

#include <vector>

#include "google/protobuf/map_field.h"

namespace google {
namespace protobuf {
namespace internal {

class MessageUtil {
 public:
  static const internal::MapFieldBase* GetMapData(
      const Reflection* reflection, const Message& message,
      const FieldDescriptor* field) {
    return reflection->GetMapData(message, field);
  }

  // Walks the entire message tree and eager parses all lazy fields.
  static void EagerParseLazyField(Message* message);
};

namespace {

// Returns true if the field is a map that contains non-message value.
bool IsNonMessageMap(const FieldDescriptor* field) {
  if (!field->is_map()) {
    return false;
  }
  constexpr int kValueIndex = 1;
  return field->message_type()->field(kValueIndex)->cpp_type() !=
         FieldDescriptor::CPPTYPE_MESSAGE;
}

inline bool UseMapIterator(const Reflection* reflection, const Message& message,
                           const FieldDescriptor* field) {
  return field->is_map() &&
         MessageUtil::GetMapData(reflection, message, field)->IsMapValid();
}

inline bool IsNonMessageField(const FieldDescriptor* field) {
  return field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE ||
         IsNonMessageMap(field);
}

}  // namespace

// To eagerly parse lazy fields in the entire message tree, mutates all the
// message fields (optional, repeated, extensions).
void MessageUtil::EagerParseLazyField(Message* message) {
  const Reflection* reflection = message->GetReflection();
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(*message, &fields);

  for (const auto* field : fields) {
    if (IsNonMessageField(field)) continue;

    if (!field->is_repeated()) {
      EagerParseLazyField(reflection->MutableMessage(message, field));
      continue;
    }

    // Map values cannot be lazy but their child message may be.
    if (UseMapIterator(reflection, *message, field)) {
      auto end = reflection->MapEnd(message, field);
      for (auto it = reflection->MapBegin(message, field); it != end; ++it) {
        EagerParseLazyField(it.MutableValueRef()->MutableMessageValue());
      }
      continue;
    }

    // Repeated messages cannot be lazy but their child messages may be.
    if (field->is_repeated()) {
      for (int i = 0, end = reflection->FieldSize(*message, field); i < end;
           ++i) {
        EagerParseLazyField(
            reflection->MutableRepeatedMessage(message, field, i));
      }
    }
  }
}

void EagerParseLazyField(Message& message) {
  MessageUtil::EagerParseLazyField(&message);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
