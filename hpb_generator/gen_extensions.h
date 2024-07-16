// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PROTOBUF_COMPILER_HBP_GEN_EXTENSIONS_H_
#define PROTOBUF_COMPILER_HBP_GEN_EXTENSIONS_H_

#include "google/protobuf/descriptor.h"
#include "google/protobuf/compiler/hpb/output.h"

namespace google::protobuf::hpb_generator {

namespace protobuf = ::proto2;

void WriteExtensionIdentifiersHeader(
    const std::vector<const protobuf::FieldDescriptor*>& extensions,
    Output& output);
void WriteExtensionIdentifierHeader(const protobuf::FieldDescriptor* ext,
                                    Output& output);
void WriteExtensionIdentifiers(
    const std::vector<const protobuf::FieldDescriptor*>& extensions,
    Output& output);
void WriteExtensionIdentifier(const protobuf::FieldDescriptor* ext,
                              Output& output);

}  // namespace protobuf
}  // namespace google::hpb_generator

#endif  // PROTOBUF_COMPILER_HBP_GEN_EXTENSIONS_H_
