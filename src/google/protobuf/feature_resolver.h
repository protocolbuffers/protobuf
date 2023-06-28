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
#include "absl/strings/string_view.h"
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

  // Creates a new FeatureResolver at a specific edition.  This validates the
  // built-in features for the given edition, and calculates the default feature
  // set.
  static absl::StatusOr<FeatureResolver> Create(absl::string_view edition,
                                                const Descriptor* descriptor);

  // Registers a potential extension of the FeatureSet proto.  Any visible
  // extensions will be used during merging.  Returns an error if the extension
  // is a feature extension but fails validation.
  absl::Status RegisterExtensions(const FileDescriptor& file);

  // Creates a new feature set using inheritance and default behavior. This is
  // designed to be called recursively, and the parent feature set is expected
  // to be a fully merged one.
  absl::StatusOr<FeatureSet> MergeFeatures(
      const FeatureSet& merged_parent, const FeatureSet& unmerged_child) const;

 private:
  FeatureResolver(absl::string_view edition, const Descriptor& descriptor,
                  std::unique_ptr<DynamicMessageFactory> message_factory,
                  std::unique_ptr<Message> defaults)
      : edition_(edition),
        descriptor_(descriptor),
        message_factory_(std::move(message_factory)),
        defaults_(std::move(defaults)) {}

  absl::Status RegisterExtensions(const Descriptor& message);
  absl::Status RegisterExtension(const FieldDescriptor& extension);

  std::string edition_;
  const Descriptor& descriptor_;
  absl::flat_hash_set<const FieldDescriptor*> extensions_;
  std::unique_ptr<DynamicMessageFactory> message_factory_;
  std::unique_ptr<Message> defaults_;
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_FEATURE_RESOLVER_H__

#include "google/protobuf/port_undef.inc"
