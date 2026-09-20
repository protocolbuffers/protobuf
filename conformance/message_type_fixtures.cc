// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/message_type_fixtures.h"

#include <string>

#include <gtest/gtest.h>
#include "conformance/binary_test_util.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace conformance {

std::string MessageTypeParamName(
    const testing::TestParamInfo<const Descriptor*>& info) {
  return ParamName(info.param);
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
