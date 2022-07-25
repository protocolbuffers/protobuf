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

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_MESSAGE_SERIALIZATION_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_MESSAGE_SERIALIZATION_H__

#include <algorithm>
#include <vector>

#include <google/protobuf/io/printer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/compiler/java/field.h>
#include <google/protobuf/compiler/java/helpers.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

// Generates code to serialize a single extension range.
void GenerateSerializeExtensionRange(io::Printer* printer,
                                     const Descriptor::ExtensionRange* range);

// Generates code to serialize all fields and extension ranges for the specified
// message descriptor, sorting serialization calls in increasing order by field
// number.
//
// Templatized to support different field generator implementations.
template <typename FieldGenerator>
void GenerateSerializeFieldsAndExtensions(
    io::Printer* printer,
    const FieldGeneratorMap<FieldGenerator>& field_generators,
    const Descriptor* descriptor, const FieldDescriptor** sorted_fields) {
  std::vector<const Descriptor::ExtensionRange*> sorted_extensions;
  sorted_extensions.reserve(descriptor->extension_range_count());
  for (int i = 0; i < descriptor->extension_range_count(); ++i) {
    sorted_extensions.push_back(descriptor->extension_range(i));
  }
  std::sort(sorted_extensions.begin(), sorted_extensions.end(),
            ExtensionRangeOrdering());

  // Merge the fields and the extension ranges, both sorted by field number.
  for (int i = 0, j = 0;
       i < descriptor->field_count() || j < sorted_extensions.size();) {
    if (i == descriptor->field_count()) {
      GenerateSerializeExtensionRange(printer, sorted_extensions[j++]);
    } else if (j == sorted_extensions.size()) {
      field_generators.get(sorted_fields[i++])
          .GenerateSerializationCode(printer);
    } else if (sorted_fields[i]->number() < sorted_extensions[j]->start) {
      field_generators.get(sorted_fields[i++])
          .GenerateSerializationCode(printer);
    } else {
      GenerateSerializeExtensionRange(printer, sorted_extensions[j++]);
    }
  }
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_MESSAGE_SERIALIZATION_H__
