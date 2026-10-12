// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/offset_ptr.h"

#include <cstddef>

#include "absl/log/absl_log.h"

namespace google {
namespace protobuf {
namespace internal {

void BasePointerInvalidSelfReference() noexcept {
  ABSL_LOG(FATAL) << "Nullable base pointer can't self reference.";
}

void BasePointerNonnullFailure() noexcept {
  ABSL_LOG(FATAL)
      << "Non-nullable base pointer constructed with a null pointer.";
}

void BasePointerOverflow(const void* ptr, const void* base) noexcept {
  ptrdiff_t diff =
      reinterpret_cast<const char*>(ptr) - reinterpret_cast<const char*>(base);
  ABSL_LOG(FATAL) << "Pointer out of scope for offset pointer: ptr=" << ptr
                  << " base=" << base << " diff=" << diff;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
