// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PROTOBUF_COMPILER_HBP_GEN_ACCESSORS_H_
#define PROTOBUF_COMPILER_HBP_GEN_ACCESSORS_H_

#include "google/protobuf/descriptor.h"
#include "google/protobuf/compiler/hpb/gen_utils.h"
#include "google/protobuf/compiler/hpb/output.h"

namespace google::protobuf::hpb_generator {

namespace protobuf = ::proto2;

void WriteFieldAccessorsInHeader(const protobuf::Descriptor* desc,
                                 Output& output);
void WriteAccessorsInSource(const protobuf::Descriptor* desc, Output& output);
void WriteUsingAccessorsInHeader(const protobuf::Descriptor* desc,
                                 MessageClassType handle_type, Output& output);
void WriteOneofAccessorsInHeader(const protobuf::Descriptor* desc,
                                 Output& output);
}  // namespace protobuf
}  // namespace google::hpb_generator

#endif  // PROTOBUF_COMPILER_HBP_GEN_ACCESSORS_H_
