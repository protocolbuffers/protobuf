// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_ENDIAN_H__
#define GOOGLE_PROTOBUF_ENDIAN_H__

#include "absl/base/internal/endian.h"

namespace google {
namespace protobuf {
namespace internal {

namespace little_endian {

using absl::little_endian::FromHost;
using absl::little_endian::ToHost;

}  // namespace little_endian

namespace big_endian {

using absl::big_endian::FromHost;
using absl::big_endian::ToHost;

}  // namespace big_endian

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_ENDIAN_H__
