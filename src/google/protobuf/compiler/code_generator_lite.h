// Protocol Buffers - Google's data interchange format
// Copyright 2008-2024 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_CODE_GENERATOR_LITE_H__
#define GOOGLE_PROTOBUF_COMPILER_CODE_GENERATOR_LITE_H__

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/internal_feature_helper.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {

// Several code generators treat the parameter argument as holding a
// list of options separated by commas.  This helper function parses
// a set of comma-delimited name/value pairs: e.g.,
//   "foo=bar,baz,moo=corge"
// parses to the pairs:
//   ("foo", "bar"), ("baz", ""), ("moo", "corge")
PROTOC_EXPORT void ParseGeneratorParameter(
    absl::string_view, std::vector<std::pair<std::string, std::string> >*);

// Strips ".proto" or ".protodevel" from the end of a filename.
PROTOC_EXPORT std::string StripProto(absl::string_view filename);

// Returns true if the proto path corresponds to a known feature file.
PROTOC_EXPORT bool IsKnownFeatureProto(absl::string_view filename);

namespace generator_internal {

// For code generators and their helper APIs only: provides access to resolved
// features for the given extension.
template <typename DescriptorT, typename ExtType, uint8_t field_type,
          bool is_packed>
auto GetResolvedFeatureExtension(
    const DescriptorT& descriptor,
    const google::protobuf::internal::ExtensionIdentifier<
        FeatureSet, internal::MessageTypeTraits<ExtType>, field_type,
        is_packed>& extension) {
  return ::google::protobuf::internal::InternalFeatureHelper::GetResolvedFeatureExtension(
      descriptor, extension);
}

}  // namespace generator_internal

}  // namespace compiler

namespace internal {

PROTOC_EXPORT bool IsOss();

#ifndef PROTO2_OPENSOURCE
void SetIsOss(bool new_is_oss);
#endif

}  // namespace internal

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_COMPILER_CODE_GENERATOR_LITE_H__
