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
#include "google/protobuf/hpb/ptr.h"

namespace hpb::internal::backend::cpp {

// hpb(cpp) backend stubs.

template <typename T>
typename T::Proxy CreateMessage(Arena& arena) {
  return typename T::Proxy();
}

template <typename T>
typename T::Proxy CloneMessage(Ptr<T> message, Arena& arena) {
  abort();
}

template <typename T>
void ClearMessage(PtrOrRawMutable<T> message) {
  abort();
}

template <typename T>
void DeepCopy(Ptr<const T> source_message, Ptr<T> target_message) {
  abort();
}

template <typename T>
absl::string_view Serialize(PtrOrRaw<T> message, Arena& arena) {
  abort();
}

}  // namespace hpb::internal::backend::cpp

#endif  // GOOGLE_PROTOBUF_HPB_BACKEND_CPP_CPP_H__
