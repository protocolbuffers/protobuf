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

template void InternalMetadata::DoClear<UnknownFieldSet>();
template void InternalMetadata::DoMergeFrom<UnknownFieldSet>(
    const UnknownFieldSet& other);
template void InternalMetadata::DoSwap<UnknownFieldSet>(UnknownFieldSet* other);
template void InternalMetadata::DeleteOutOfLineHelper<UnknownFieldSet>();

}  // namespace internal
}  // namespace protobuf
}  // namespace google
