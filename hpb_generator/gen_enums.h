// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_HPB_GEN_ENUMS_H__
#define GOOGLE_PROTOBUF_COMPILER_HPB_GEN_ENUMS_H__

#include "hpb_generator/context.h"
#include "google/protobuf/descriptor.h"

namespace google::protobuf::hpb_generator {

namespace protobuf = ::proto2;

std::string EnumTypeName(const protobuf::EnumDescriptor* enum_descriptor);
std::string EnumValueSymbolInNameSpace(
    const protobuf::EnumDescriptor* desc,
    const protobuf::EnumValueDescriptor* value);
void WriteEnumDeclarations(
    const std::vector<const protobuf::EnumDescriptor*>& enums, Context& ctx);

}  // namespace protobuf
}  // namespace google::hpb_generator

#endif  // GOOGLE_PROTOBUF_COMPILER_HPB_GEN_ENUMS_H__
