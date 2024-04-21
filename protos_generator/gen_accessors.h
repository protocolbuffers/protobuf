// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_PROTOS_GENERATOR_ACCESSORS_H_
#define UPB_PROTOS_GENERATOR_ACCESSORS_H_

#include "google/protobuf/descriptor.h"
#include "protos_generator/gen_utils.h"
#include "protos_generator/output.h"

namespace protos_generator {

namespace protobuf = ::google::protobuf;

void WriteFieldAccessorsInHeader(const protobuf::Descriptor* desc,
                                 Output& output);
void WriteAccessorsInSource(const protobuf::Descriptor* desc, Output& output);
void WriteUsingAccessorsInHeader(const protobuf::Descriptor* desc,
                                 MessageClassType handle_type, Output& output);
void WriteOneofAccessorsInHeader(const protobuf::Descriptor* desc,
                                 Output& output);
}  // namespace protos_generator

#endif  // UPB_PROTOS_GENERATOR_ACCESSORS_H_
