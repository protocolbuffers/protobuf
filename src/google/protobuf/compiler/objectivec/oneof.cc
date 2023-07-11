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

#include "google/protobuf/compiler/objectivec/oneof.h"

#include <string>

#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

OneofGenerator::OneofGenerator(const OneofDescriptor* descriptor)
    : descriptor_(descriptor) {
  variables_["enum_name"] = OneofEnumName(descriptor_);
  variables_["name"] = OneofName(descriptor_);
  variables_["capitalized_name"] = OneofNameCapitalized(descriptor_);
  variables_["raw_index"] = absl::StrCat(descriptor_->index());
  const Descriptor* msg_descriptor = descriptor_->containing_type();
  variables_["owning_message_class"] = ClassName(msg_descriptor);
}

void OneofGenerator::SetOneofIndexBase(int index_base) {
  int index = descriptor_->index() + index_base;
  // Flip the sign to mark it as a oneof.
  variables_["index"] = absl::StrCat(-index);
}

void OneofGenerator::GenerateCaseEnum(io::Printer* printer) const {
  auto vars = printer->WithVars(variables_);
  printer->Emit({{"cases",
                  [&] {
                    for (int j = 0; j < descriptor_->field_count(); j++) {
                      const FieldDescriptor* field = descriptor_->field(j);
                      printer->Emit(
                          {{"field_name", FieldNameCapitalized(field)},
                           {"field_number", field->number()}},
                          R"objc(
                            $enum_name$_$field_name$ = $field_number$,
                          )objc");
                    }
                  }}},
                R"objc(
                  typedef GPB_ENUM($enum_name$) {
                    $enum_name$_GPBUnsetOneOfCase = 0,
                    $cases$
                  };
                )objc");
  printer->Emit("\n");
}

void OneofGenerator::GeneratePublicCasePropertyDeclaration(
    io::Printer* printer) const {
  auto vars = printer->WithVars(variables_);
  printer->Emit(
      {{"comments", [&] { EmitCommentsString(printer, descriptor_); }}},
      R"objc(
        $comments$;
        @property(nonatomic, readonly) $enum_name$ $name$OneOfCase;
      )objc");
  printer->Emit("\n");
}

void OneofGenerator::GenerateClearFunctionDeclaration(
    io::Printer* printer) const {
  auto vars = printer->WithVars(variables_);
  printer->Emit(R"objc(
      /**
       * Clears whatever value was set for the oneof '$name$'.
       **/
      void $owning_message_class$_Clear$capitalized_name$OneOfCase($owning_message_class$ *message);
    )objc");
}

void OneofGenerator::GeneratePropertyImplementation(
    io::Printer* printer) const {
  auto vars = printer->WithVars(variables_);
  printer->Emit(R"objc(
    @dynamic $name$OneOfCase;
  )objc");
}

void OneofGenerator::GenerateClearFunctionImplementation(
    io::Printer* printer) const {
  auto vars = printer->WithVars(variables_);
  printer->Emit(R"objc(
    void $owning_message_class$_Clear$capitalized_name$OneOfCase($owning_message_class$ *message) {
      GPBDescriptor *descriptor = [$owning_message_class$ descriptor];
      GPBOneofDescriptor *oneof = [descriptor.oneofs objectAtIndex:$raw_index$];
      GPBClearOneof(message, oneof);
    }
  )objc");
}

std::string OneofGenerator::DescriptorName() const {
  return variables_.find("name")->second;
}

std::string OneofGenerator::HasIndexAsString() const {
  return variables_.find("index")->second;
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
