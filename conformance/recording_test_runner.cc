// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "recording_test_runner.h"

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/base/nullability.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "test_runner.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unknown_field_set.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

uint64_t Fnv1a64(absl::string_view data) {
  constexpr uint64_t kOffsetBasis = 0xcbf29ce484222325u;
  constexpr uint64_t kPrime = 0x100000001b3u;
  uint64_t hash = kOffsetBasis;
  for (unsigned char c : data) {
    hash ^= static_cast<uint64_t>(c);
    hash *= kPrime;
  }
  return hash;
}

std::string CanonicalizeRequest(absl::string_view input) {
  ::conformance::ConformanceRequest request;
  std::string text;
  if (request.ParseFromString(input) &&
      TextFormat::PrintToString(request, &text)) {
    return text;
  }

  // Not a valid ConformanceRequest (in practice: a string field carrying
  // invalid UTF-8).  Canonicalize without the schema so the result is still
  // independent of the order the fields were serialized in.
  UnknownFieldSet fields;
  if (!fields.ParseFromString(input)) return std::string(input);
  std::vector<const UnknownField*> sorted;
  sorted.reserve(static_cast<size_t>(fields.field_count()));
  for (int i = 0; i < fields.field_count(); ++i) {
    sorted.push_back(&fields.field(i));
  }
  absl::c_stable_sort(sorted, [](const UnknownField* a, const UnknownField* b) {
    return a->number() < b->number();
  });
  UnknownFieldSet canonical;
  for (const UnknownField* field : sorted) canonical.AddField(*field);
  std::string bytes;
  if (!canonical.SerializeToString(&bytes)) return std::string(input);
  return bytes;
}

}  // namespace internal

RecordingTestRunner::RecordingTestRunner(
    ConformanceTestRunner* absl_nonnull delegate,
    std::ostream* absl_nonnull out)
    : delegate_(delegate), out_(out) {}

RecordingTestRunner::~RecordingTestRunner() = default;

std::string RecordingTestRunner::RunTest(absl::string_view test_name,
                                         absl::string_view input) {
  // Record and flush before delegating so that the request is captured even if
  // the runner aborts while handling it.
  *out_ << absl::StreamFormat(
               "%s %d %016x\n", test_name, input.size(),
               internal::Fnv1a64(internal::CanonicalizeRequest(input)))
        << std::flush;
  return delegate_->RunTest(test_name, input);
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
