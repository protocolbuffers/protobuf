// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/reflection_mode.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

#if !defined(PROTOBUF_NO_THREADLOCAL)

#if defined(PROTOBUF_USE_DLLS) && defined(_WIN32)
ReflectionMode& ScopedReflectionMode::reflection_mode() {
  static PROTOBUF_THREAD_LOCAL ReflectionMode reflection_mode =
      ReflectionMode::kDefault;
  return reflection_mode;
}
#else
PROTOBUF_CONSTINIT PROTOBUF_THREAD_LOCAL ReflectionMode
    ScopedReflectionMode::reflection_mode_ = ReflectionMode::kDefault;
#endif

#endif

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
