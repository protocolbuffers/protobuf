// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/recording_test_runner.h"

#include <ostream>
#include <string>

#include "absl/base/nullability.h"
#include "absl/crc/crc32c.h"
#include "absl/log/absl_check.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "conformance/test_runner.h"
#include "google/protobuf/unknown_field_set.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::conformance::ConformanceRequest;

// The two kinds of field the runner sends: strings and bytes are rendered
// escaped and quoted, enums and bools as integers.
void AppendField(absl::string_view name, absl::string_view value,
                 std::string* absl_nonnull out) {
  absl::StrAppend(out, name, ": \"", absl::CHexEscape(value), "\"\n");
}

void AppendField(absl::string_view name, int value,
                 std::string* absl_nonnull out) {
  absl::StrAppend(out, name, ": ", value, "\n");
}

// Renders a serialized ConformanceRequest as one "name: value" line per set
// field, in field-number order.
std::string CanonicalizeRequest(absl::string_view input) {
  ConformanceRequest request;
  ABSL_CHECK(request.ParseFromString(input))
      << "input is not a serialized ConformanceRequest";
  ABSL_CHECK(request.unknown_fields().empty())
      << "unknown ConformanceRequest field "
      << request.unknown_fields().field(0).number();
  ABSL_CHECK(!request.has_jspb_encoding_options())
      << "ConformanceRequest.jspb_encoding_options is never sent by the runner";

  std::string canonical;
  if (request.has_protobuf_payload()) {
    AppendField("protobuf_payload", request.protobuf_payload(), &canonical);
  }
  if (request.has_json_payload()) {
    AppendField("json_payload", request.json_payload(), &canonical);
  }
  if (request.requested_output_format() != ::conformance::UNSPECIFIED) {
    AppendField("requested_output_format", request.requested_output_format(),
                &canonical);
  }
  if (!request.message_type().empty()) {
    AppendField("message_type", request.message_type(), &canonical);
  }
  if (request.test_category() != ::conformance::UNSPECIFIED_TEST) {
    AppendField("test_category", request.test_category(), &canonical);
  }
  if (request.has_jspb_payload()) {
    AppendField("jspb_payload", request.jspb_payload(), &canonical);
  }
  if (request.has_text_payload()) {
    AppendField("text_payload", request.text_payload(), &canonical);
  }
  if (request.print_unknown_fields()) {
    AppendField("print_unknown_fields", 1, &canonical);
  }
  return canonical;
}

}  // namespace

RecordingTestRunner::RecordingTestRunner(
    ConformanceTestRunner* absl_nonnull delegate,
    std::ostream* absl_nonnull out)
    : delegate_(delegate), out_(out) {}

RecordingTestRunner::~RecordingTestRunner() = default;

std::string RecordingTestRunner::RunTest(absl::string_view test_name,
                                         absl::string_view input) {
  // Record and flush before delegating so that the request is captured even if
  // the runner aborts while handling it.
  *out_ << absl::StrCat(test_name, " ", input.size(), " ",
                        absl::ComputeCrc32c(CanonicalizeRequest(input)), "\n")
        << std::flush;
  return delegate_->RunTest(test_name, input);
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
