// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/unknown_field_set.h"

namespace google {
namespace protobuf {
namespace internal {

template <>
void InternalMetadata::DoClear<UnknownFieldSet>() {
  mutable_unknown_fields<UnknownFieldSet>()->Clear();
}

template <>
void InternalMetadata::DoMergeFrom<UnknownFieldSet>(
    const UnknownFieldSet& other) {
  mutable_unknown_fields<UnknownFieldSet>()->MergeFrom(other);
}

template <>
void InternalMetadata::DoSwap<UnknownFieldSet>(UnknownFieldSet* other) {
  mutable_unknown_fields<UnknownFieldSet>()->Swap(other);
}

template <>
void InternalMetadata::DeleteOutOfLineHelper<UnknownFieldSet>() {
  delete PtrValue<Container<UnknownFieldSet>>();
  // TODO:  This store is load-bearing.  Since we are destructing
  // the message at this point, see if we can eliminate it.
  ptr_ = nullptr;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
