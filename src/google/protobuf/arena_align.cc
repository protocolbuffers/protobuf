// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/arena_align.h"

#include <cstddef>
#include <cstdint>

namespace google {
namespace protobuf {
namespace internal {

// There are still compilers (open source) requiring a definition for constexpr.
constexpr size_t ArenaAlignDefault::align;  // NOLINT

}  // namespace internal
}  // namespace protobuf
}  // namespace google
