// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_PROTOS_GENERATOR_GEN_MESSAGES_H_
#define UPB_PROTOS_GENERATOR_GEN_MESSAGES_H_

#include "google/protobuf/descriptor.h"
#include "protos_generator/output.h"

namespace protos_generator {
namespace protobuf = ::google::protobuf;

void WriteMessageClassDeclarations(
    const protobuf::Descriptor* descriptor,
    const std::vector<const protobuf::FieldDescriptor*>& file_exts,
    const std::vector<const protobuf::EnumDescriptor*>& file_enums,
    Output& output);
void WriteMessageImplementation(
    const protobuf::Descriptor* descriptor,
    const std::vector<const protobuf::FieldDescriptor*>& file_exts,
    Output& output);
}  // namespace protos_generator

#endif  // UPB_PROTOS_GENERATOR_GEN_MESSAGES_H_
