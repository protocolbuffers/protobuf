// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_ONE_PLATFORM_DESCRIPTOR_POOL_QUIRKS_H__
#define GOOGLE_PROTOBUF_ONE_PLATFORM_DESCRIPTOR_POOL_QUIRKS_H__

#include "absl/status/status.h"
#include "google/protobuf/descriptor.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

class DescriptorPool;

class OnePlatformDescriptorPoolQuirks final {
 public:
  // Enables OnePlatform quirks for the provided descriptor pool. The usage of
  // this class and method requires approval from CEL and protobuf leads. It is
  // intended to be a short-term workaround.
  //
  // 1. Treats all enums as "scoped", that is enum values are only required to
  // be unique amongst the enum itself and not among the siblings of the enum as
  // required by protobuf. A side effect is that
  // `{DescriptorPool,FileDescriptor,Descriptor}::FindEnumValueByName` will
  // always return `nullptr`. Instead you must exclusively use
  // `EnumDescriptor::FindValueByName`.
  static absl::Status Enable(google::protobuf::DescriptorPool& pool) {
    if (pool.build_started_) {
      return absl::FailedPreconditionError(
          "OnePlatformDescriptorPoolQuirks::Enable must be called before "
          "building any descriptors");
    }
    pool.one_platform_quirks_ = true;
    return absl::OkStatus();
  }
};

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif
