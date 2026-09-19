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
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "conformance/conformance.pb.h"
#include "conformance_cpp_harness.h"
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

absl::Status ReadFd(int fd, char* buf, size_t len) {
  while (len > 0) {
    ssize_t bytes_read = read(fd, buf, len);

    if (bytes_read == 0) {
      return absl::DataLossError("unexpected EOF");
    }

    if (bytes_read < 0) {
      return absl::ErrnoToStatus(errno, "error reading from test runner");
    }

    len -= bytes_read;
    buf += bytes_read;
  }
  return absl::OkStatus();
}

absl::Status WriteFd(int fd, const void* buf, size_t len) {
  if (static_cast<size_t>(write(fd, buf, len)) != len) {
    return absl::ErrnoToStatus(errno, "error reading to test runner");
  }
  return absl::OkStatus();
}

// Reads one request from stdin, runs it through `harness`, and writes the
// response to stdout. Returns Ok(true) if we're done processing requests.
absl::StatusOr<bool> ServeConformanceRequest(
    const CppConformanceHarness& harness) {
  uint32_t in_len;
  if (!ReadFd(STDIN_FILENO, reinterpret_cast<char*>(&in_len), sizeof(in_len))
           .ok()) {
    // EOF means we're done.
    return true;
  }
  in_len = google::protobuf::internal::little_endian::ToHost(in_len);

  std::string serialized_input;
  serialized_input.resize(in_len);
  RETURN_IF_ERROR(ReadFd(STDIN_FILENO, &serialized_input[0], in_len));

  ConformanceRequest request;
  ABSL_CHECK(request.ParseFromString(serialized_input));

  absl::StatusOr<ConformanceResponse> response = harness.RunTest(request);
  RETURN_IF_ERROR(response.status());

  std::string serialized_output;
  // TODO: Remove this suppression.
  (void)response->SerializeToString(&serialized_output);

  uint32_t out_len = google::protobuf::internal::little_endian::FromHost(
      static_cast<uint32_t>(serialized_output.size()));

  RETURN_IF_ERROR(WriteFd(STDOUT_FILENO, &out_len, sizeof(out_len)));
  RETURN_IF_ERROR(WriteFd(STDOUT_FILENO, serialized_output.data(),
                          serialized_output.size()));

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
