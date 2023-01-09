// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/repeated_field.h"

#include <algorithm>
#include <string>

#include "google/protobuf/stubs/logging.h"
#include "google/protobuf/stubs/logging.h"
#include "absl/strings/cord.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {


template <>
PROTOBUF_EXPORT_TEMPLATE_DEFINE void RepeatedField<absl::Cord>::Clear() {
  for (int i = 0; i < current_size_; i++) {
    Mutable(i)->Clear();
  }
  ExchangeCurrentSize(0);
}

template <>
PROTOBUF_EXPORT_TEMPLATE_DEFINE size_t
RepeatedField<absl::Cord>::SpaceUsedExcludingSelfLong() const {
  size_t result = current_size_ * sizeof(absl::Cord);
  for (int i = 0; i < current_size_; i++) {
    // Estimate only.
    result += Get(i).size();
  }
  return result;
}

template <>
PROTOBUF_EXPORT_TEMPLATE_DEFINE void RepeatedField<absl::Cord>::Truncate(
    int new_size) {
  GOOGLE_ABSL_DCHECK_LE(new_size, current_size_);
  while (current_size_ > new_size) {
    RemoveLast();
  }
}

template <>
PROTOBUF_EXPORT_TEMPLATE_DEFINE void RepeatedField<absl::Cord>::Resize(
    int new_size, const absl::Cord& value) {
  GOOGLE_ABSL_DCHECK_GE(new_size, 0);
  if (new_size > current_size_) {
    Reserve(new_size);
    std::fill(&rep()->elements()[ExchangeCurrentSize(new_size)],
              &rep()->elements()[new_size], value);
  } else {
    while (current_size_ > new_size) {
      RemoveLast();
    }
  }
}

template <>
PROTOBUF_EXPORT_TEMPLATE_DEFINE void RepeatedField<absl::Cord>::MoveArray(
    absl::Cord* to, absl::Cord* from, int size) {
  for (int i = 0; i < size; i++) {
    swap(to[i], from[i]);
  }
}

template <>
PROTOBUF_EXPORT_TEMPLATE_DEFINE void RepeatedField<absl::Cord>::CopyArray(
    absl::Cord* to, const absl::Cord* from, int size) {
  for (int i = 0; i < size; i++) {
    to[i] = from[i];
  }
}


}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
