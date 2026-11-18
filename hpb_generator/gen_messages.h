// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_HPB_GEN_MESSAGES_H__
#define GOOGLE_PROTOBUF_COMPILER_HPB_GEN_MESSAGES_H__

#include <vector>

#include "hpb_generator/context.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace hpb_generator {

void WriteMessageClassDeclarations(
    const google::protobuf::Descriptor* descriptor,
    const std::vector<const google::protobuf::FieldDescriptor*>& file_exts,
    const std::vector<const google::protobuf::EnumDescriptor*>& file_enums, Context& ctx);
void WriteMessageImplementation(
    const google::protobuf::Descriptor* descriptor,
    const std::vector<const google::protobuf::FieldDescriptor*>& file_exts, Context& ctx);
}  // namespace hpb_generator
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_HPB_GEN_MESSAGES_H__
