// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The C++ conformance testee. Speaks the length-prefixed stdin/stdout protocol
// described in fork_pipe_runner.cc and delegates each request to
// CppConformanceHarness.

#include <errno.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "conformance/conformance.pb.h"
#include "conformance/conformance_cpp_harness.h"
#include "google/protobuf/endian.h"
#include "google/protobuf/message.h"
#include "google/protobuf/stubs/status_macros.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;

// Set to true to log every request/response pair.
constexpr bool kVerbose = false;

absl::Status ReadAll(FILE* file, char* buf, size_t len) {
  while (len > 0) {
    size_t bytes_read = fread(buf, 1, len, file);

    if (bytes_read == 0) {
      if (feof(file)) {
        return absl::DataLossError("unexpected EOF");
      }
      return absl::ErrnoToStatus(errno, "error reading from test runner");
    }

    len -= bytes_read;
    buf += bytes_read;
  }
  return absl::OkStatus();
}

absl::Status WriteAll(FILE* file, const void* buf, size_t len) {
  if (fwrite(buf, 1, len, file) != len) {
    return absl::ErrnoToStatus(errno, "error writing to test runner");
  }
  return absl::OkStatus();
}

// Reads one request from stdin, runs it through `harness`, and writes the
// response to stdout. Returns Ok(true) if we're done processing requests.
absl::StatusOr<bool> ServeConformanceRequest(
    const CppConformanceHarness& harness) {
  uint32_t in_len;
  if (!ReadAll(stdin, reinterpret_cast<char*>(&in_len), sizeof(in_len)).ok()) {
    // EOF means we're done.
    return true;
  }
  in_len = google::protobuf::internal::little_endian::ToHost(in_len);

  std::string serialized_input;
  serialized_input.resize(in_len);
  RETURN_IF_ERROR(ReadAll(stdin, &serialized_input[0], in_len));

  ConformanceRequest request;
  ABSL_CHECK(request.ParseFromString(serialized_input));

  absl::StatusOr<ConformanceResponse> response = harness.RunTest(request);
  RETURN_IF_ERROR(response.status());

  std::string serialized_output;
  // TODO: Remove this suppression.
  (void)response->SerializeToString(&serialized_output);

  uint32_t out_len = google::protobuf::internal::little_endian::FromHost(
      static_cast<uint32_t>(serialized_output.size()));

  RETURN_IF_ERROR(WriteAll(stdout, &out_len, sizeof(out_len)));
  RETURN_IF_ERROR(
      WriteAll(stdout, serialized_output.data(), serialized_output.size()));
  if (fflush(stdout) != 0) {
    return absl::ErrnoToStatus(errno, "error flushing to test runner");
  }

  if (kVerbose) {
    ABSL_LOG(INFO) << "conformance-cpp: request="
                   << google::protobuf::ShortFormat(request)
                   << ", response=" << google::protobuf::ShortFormat(*response);
  }
  return false;
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

int main() {
#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  google::protobuf::conformance::CppConformanceHarness harness;
  int total_runs = 0;
  while (true) {
    absl::StatusOr<bool> is_done =
        google::protobuf::conformance::ServeConformanceRequest(harness);
    if (!is_done.ok()) {
      ABSL_LOG(FATAL) << is_done.status();
    }
    if (*is_done) {
      break;
    }
    total_runs++;
  }
  ABSL_LOG(INFO) << "conformance-cpp: received EOF from test runner after "
                 << total_runs << " tests";
}
