// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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

#ifndef GOOGLE_PROTOBUF_FEATURE_RESOLVER_H__
#define GOOGLE_PROTOBUF_FEATURE_RESOLVER_H__

#include <memory>
#include <string>
#include <utility>

#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/dynamic_message.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

// These helpers implement the unique behaviors of edition features.  For more
// details, see go/protobuf-editions-features.
class PROTOBUF_EXPORT FeatureResolver {
 public:
  FeatureResolver(FeatureResolver&&) = default;
  FeatureResolver& operator=(FeatureResolver&&) = delete;

  // Compiles a set of FeatureSet extensions into a mapping of edition to unique
  // defaults.  This is the most complicated part of feature resolution, and by
  // abstracting this out into an intermediate message, we can make feature
  // resolution significantly more portable.
  static absl::StatusOr<FeatureSetDefaults> CompileDefaults(
      const Descriptor* feature_set,
      absl::Span<const FieldDescriptor* const> extensions,
      Edition minimum_edition, Edition maximum_edition);

  // Creates a new FeatureResolver at a specific edition.  This calculates the
  // default feature set for that edition, using the output of CompileDefaults.
  static absl::StatusOr<FeatureResolver> Create(
      Edition edition, const FeatureSetDefaults& defaults);

  // Creates a new feature set using inheritance and default behavior. This is
  // designed to be called recursively, and the parent feature set is expected
  // to be a fully merged one.  The returned FeatureSet will be fully resolved
  // for any extensions that were used to construct the defaults.
  absl::StatusOr<FeatureSet> MergeFeatures(
      const FeatureSet& merged_parent, const FeatureSet& unmerged_child) const;

 private:
  explicit FeatureResolver(FeatureSet defaults)
      : defaults_(std::move(defaults)) {}

  FeatureSet defaults_;
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_FEATURE_RESOLVER_H__

#include "google/protobuf/port_undef.inc"
