// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_BACKEND_CPP_CPP_H__
#define GOOGLE_PROTOBUF_HPB_BACKEND_CPP_CPP_H__

#include "absl/strings/string_view.h"
#include "google/protobuf/hpb/arena.h"
#include "google/protobuf/hpb/internal/template_help.h"

namespace hpb::internal::backend::cpp {

template <typename T>
void ClearMessage(hpb::internal::PtrOrRawMutable<T> message) {
  (void)message;
}

template <typename T>
absl::string_view Serialize(hpb::internal::PtrOrRaw<T> message,
                            hpb::Arena& arena) {
  return "stub";
}

}  // namespace hpb::internal::backend::cpp

#endif  // GOOGLE_PROTOBUF_HPB_BACKEND_CPP_CPP_H__
