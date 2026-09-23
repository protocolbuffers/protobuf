// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file contains a program for running the test suite in a separate
// process.  The other alternative is to run the suite in-process.  See
// conformance.proto for pros/cons of these two options.
//
// This program will fork the process under test and communicate with it over
// its stdin/stdout:
//
//     +--------+   pipe   +----------+
//     | tester | <------> | testee   |
//     |        |          |          |
//     |  C++   |          | any lang |
//     +--------+          +----------+
//
// The tester contains all of the test cases and their expected output.
// The testee is a simple program written in the target language that reads
// each test case and attempts to produce acceptable output for it.
//
// Every test consists of a ConformanceRequest/ConformanceResponse
// request/reply pair.  The protocol on the pipe is simply:
//
//   1. tester sends 4-byte length N (little endian)
//   2. tester sends N bytes representing a ConformanceRequest proto
//   3. testee sends 4-byte length M (little endian)
//   4. testee sends M bytes representing a ConformanceResponse proto

#include "conformance/fork_pipe_runner.h"

#include <cstdint>
#include <string>

#include "absl/log/absl_log.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "google/protobuf/endian.h"

namespace google {
namespace protobuf {

std::string ForkPipeRunner::RunTest(absl::string_view test_name,
                                    absl::string_view request) {
  if (!IsTestProgramRunning()) {
    SpawnTestProgram();
  }
  current_test_name_ = std::string(test_name);

  uint32_t len =
      internal::little_endian::FromHost(static_cast<uint32_t>(request.size()));

  if (!TryWrite(&len, sizeof(uint32_t)) ||
      !TryWrite(request.data(), request.size())) {
    return ReportFailure(/*timed_out=*/false, "error writing to child");
  }

  ReadResult read_result = TryRead(&len, sizeof(uint32_t));
  if (read_result != ReadResult::kOk) return ReportReadFailure(read_result);

  len = internal::little_endian::ToHost(len);
  std::string response(len, '\0');
  read_result = TryRead(&response[0], len);
  if (read_result != ReadResult::kOk) return ReportReadFailure(read_result);
  return response;
}

std::string ForkPipeRunner::ReportReadFailure(ReadResult read_result) {
  absl::string_view what_failed;
  switch (read_result) {
    case ReadResult::kTimeout:
      what_failed = "child timed out";
      break;
    case ReadResult::kEof:
      what_failed = "child closed its output without responding";
      break;
    default:
      what_failed = "error reading from child";
      break;
  }
  return ReportFailure(read_result == ReadResult::kTimeout, what_failed);
}

std::string ForkPipeRunner::ReportFailure(bool timed_out,
                                          absl::string_view what_failed) {
  // The exchange with the testee failed: it exited, crashed, or hung.  It is
  // shut down and the outcome classified by the platform's implementation;
  // the next RunTest() call will spawn a fresh testee.
  const std::string error_msg = GetTestProgramFailure(what_failed);
  ABSL_LOG(INFO) << error_msg;

  conformance::ConformanceResponse response;
  if (timed_out) {
    response.set_timeout_error(error_msg);
  } else {
    response.set_runtime_error(error_msg);
  }
  std::string serialized;
  // TODO: Remove this suppression.
  (void)response.SerializeToString(&serialized);
  return serialized;
}

}  // namespace protobuf
}  // namespace google
