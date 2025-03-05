// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_ENDIAN_H__
#define GOOGLE_PROTOBUF_ENDIAN_H__

#include "absl/numeric/bits.h"

namespace google {
namespace protobuf {
namespace internal {
namespace little_endian {

template <typename T>
T FromHost(T value) {
  if constexpr (absl::endian::native == absl::endian::big) {
    return absl::byteswap(value);
  } else {
    return value;
  }
}

template <typename T>
T ToHost(T value) {
  return FromHost(value);
}

}  // namespace little_endian

namespace big_endian {

template <typename T>
T FromHost(T value) {
  if constexpr (absl::endian::native == absl::endian::big) {
    return value;
  } else {
    return absl::byteswap(value);
  }
}

template <typename T>
T ToHost(T value) {
  return FromHost(value);
}

}  // namespace big_endian

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_ENDIAN_H__
