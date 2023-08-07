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

#ifndef THIRD_PARTY_UPB_PROTOS_GENERATOR_GEN_REPEATED_FIELDS_H_
#define THIRD_PARTY_UPB_PROTOS_GENERATOR_GEN_REPEATED_FIELDS_H_

#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "protos_generator/output.h"

namespace protos_generator {
namespace protobuf = ::google::protobuf;

void WriteRepeatedFieldUsingAccessors(const protobuf::FieldDescriptor* field,
                                      absl::string_view class_name,
                                      absl::string_view resolved_field_name,
                                      Output& output, bool read_only);

void WriteRepeatedFieldsInMessageHeader(const protobuf::Descriptor* desc,
                                        const protobuf::FieldDescriptor* field,
                                        absl::string_view resolved_field_name,
                                        absl::string_view resolved_upbc_name,
                                        Output& output);

void WriteRepeatedMessageAccessor(const protobuf::Descriptor* message,
                                  const protobuf::FieldDescriptor* field,
                                  absl::string_view resolved_field_name,
                                  absl::string_view class_name, Output& output);

void WriteRepeatedStringAccessor(const protobuf::Descriptor* message,
                                 const protobuf::FieldDescriptor* field,
                                 absl::string_view resolved_field_name,
                                 absl::string_view class_name, Output& output);

void WriteRepeatedScalarAccessor(const protobuf::Descriptor* message,
                                 const protobuf::FieldDescriptor* field,
                                 absl::string_view resolved_field_name,
                                 absl::string_view class_name, Output& output);

}  // namespace protos_generator

#endif  // THIRD_PARTY_UPB_PROTOS_GENERATOR_GEN_REPEATED_FIELDS_H_
