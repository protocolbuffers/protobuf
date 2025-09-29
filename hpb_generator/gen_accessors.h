// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_HPB_GEN_ACCESSORS_H__
#define GOOGLE_PROTOBUF_COMPILER_HPB_GEN_ACCESSORS_H__

#include "hpb_generator/context.h"
#include "hpb_generator/gen_utils.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace hpb_generator {

void WriteFieldAccessorsInHeader(const google::protobuf::Descriptor* desc, Context& ctx);
void WriteAccessorsInSource(const google::protobuf::Descriptor* desc, Context& ctx);

void WriteUsingAccessorsInHeader(const google::protobuf::Descriptor* desc,
                                 MessageClassType handle_type, Context& ctx);
void WriteOneofAccessorsInHeader(const google::protobuf::Descriptor* desc, Context& ctx);
}  // namespace hpb_generator
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_HPB_GEN_ACCESSORS_H__
