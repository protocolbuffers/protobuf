// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_PROTOS_GENERATOR_ENUMS_H_
#define UPB_PROTOS_GENERATOR_ENUMS_H_

#include "google/protobuf/descriptor.h"
#include "protos_generator/output.h"

namespace protos_generator {

namespace protobuf = ::google::protobuf;

std::string EnumTypeName(const protobuf::EnumDescriptor* enum_descriptor);
std::string EnumValueSymbolInNameSpace(
    const protobuf::EnumDescriptor* desc,
    const protobuf::EnumValueDescriptor* value);
void WriteHeaderEnumForwardDecls(
    std::vector<const protobuf::EnumDescriptor*>& enums, Output& output);
void WriteEnumDeclarations(
    const std::vector<const protobuf::EnumDescriptor*>& enums, Output& output);

}  // namespace protos_generator

#endif  // UPB_PROTOS_GENERATOR_ENUMS_H_
