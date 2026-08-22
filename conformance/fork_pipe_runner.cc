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

#include "fork_pipe_runner.h"

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

  CheckedWrite(&len, sizeof(uint32_t));
  CheckedWrite(request.data(), request.size());

  std::string response;
  bool timed_out = false;
  if (!TryRead(&len, sizeof(uint32_t), &timed_out)) {
    conformance::ConformanceResponse response_obj;
    std::string error_msg = GetTestProgramFailure(timed_out);
    ABSL_LOG(INFO) << error_msg;
    if (timed_out) {
      response_obj.set_timeout_error(error_msg);
    } else {
      response_obj.set_runtime_error(error_msg);
    }
    // TODO: Remove this suppression.
    (void)response_obj.SerializeToString(&response);
    return response;
  }

  len = internal::little_endian::ToHost(len);
  response.resize(len);
  CheckedRead((void*)response.c_str(), len);
  return response;
}

}  // namespace protobuf
}  // namespace google
