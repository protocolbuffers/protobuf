// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_UPB_UTIL_DEF_TO_PROTO_FUZZ_IMPL_H__
#define GOOGLE_UPB_UPB_UTIL_DEF_TO_PROTO_FUZZ_IMPL_H__

#include "google/protobuf/descriptor.pb.h"

namespace upb_test {

void RoundTripDescriptor(const google::protobuf::FileDescriptorSet& set);

}  // namespace upb_test

#endif  // GOOGLE_UPB_UPB_UTIL_DEF_TO_PROTO_FUZZ_IMPL_H__
