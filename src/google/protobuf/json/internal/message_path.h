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

#ifndef GOOGLE_PROTOBUF_JSON_INTERNAL_MESSAGE_PATH_H__
#define GOOGLE_PROTOBUF_JSON_INTERNAL_MESSAGE_PATH_H__

#include <string>
#include <vector>

#include "absl/cleanup/cleanup.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace json_internal {
// A path in a Protobuf message, annotated specifically for producing nice
// errors.
class MessagePath {
 public:
  explicit MessagePath(absl::string_view message_root)
      : components_(
            {Component{FieldDescriptor::TYPE_MESSAGE, message_root, "", -1}}) {}

  // Pushes a new field name, along with an optional type name if it is
  // a message or enum.
  //
  // Returns an RAII object that will pop the field component on scope exit.
  auto Push(absl::string_view field_name, FieldDescriptor::Type type,
            absl::string_view type_name = "") {
    // -1 makes it so the first call to NextRepeated makes the index 0.
    components_.push_back(Component{type, type_name, field_name, -1});
    return absl::MakeCleanup([this] { components_.pop_back(); });
  }

  // Increments the index of this field, indicating it is a repeated field.
  //
  // The first time this is called, the field will be marked as repeated and
  // the index will become 0.
  void NextRepeated() { ++components_.back().repeated_index; }

  // Appends a description of the current state of the path to `out`.
  void Describe(std::string& out) const;

 private:
  struct Component {
    FieldDescriptor::Type type;
    absl::string_view type_name, field_name;
    int32_t repeated_index;
  };
  std::vector<Component> components_;
};
}  // namespace json_internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_JSON_INTERNAL_MESSAGE_PATH_H__
