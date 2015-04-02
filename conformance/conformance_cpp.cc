// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#include "conformance.pb.h"

using std::string;
using conformance::ConformanceRequest;
using conformance::ConformanceResponse;
using conformance::TestAllTypes;

int test_count = 0;
bool verbose = false;

bool CheckedRead(int fd, void *buf, size_t len) {
  size_t ofs = 0;
  while (len > 0) {
    ssize_t bytes_read = read(fd, (char*)buf + ofs, len);

    if (bytes_read == 0) return false;

    if (bytes_read < 0) {
      GOOGLE_LOG(FATAL) << "Error reading from test runner: " <<  strerror(errno);
    }

    len -= bytes_read;
    ofs += bytes_read;
  }

  return true;
}

void CheckedWrite(int fd, const void *buf, size_t len) {
  if (write(fd, buf, len) != len) {
    GOOGLE_LOG(FATAL) << "Error writing to test runner: " << strerror(errno);
  }
}

void DoTest(const ConformanceRequest& request, ConformanceResponse* response) {
  TestAllTypes test_message;

  switch (request.payload_case()) {
    case ConformanceRequest::kProtobufPayload:
      if (!test_message.ParseFromString(request.protobuf_payload())) {
        // Getting parse details would involve something like:
        //   http://stackoverflow.com/questions/22121922/how-can-i-get-more-details-about-errors-generated-during-protobuf-parsing-c
        response->set_parse_error("Parse error (no more details available).");
        return;
      }
      break;

    case ConformanceRequest::kJsonPayload:
      response->set_runtime_error("JSON input is not yet supported.");
      break;

    case ConformanceRequest::PAYLOAD_NOT_SET:
      GOOGLE_LOG(FATAL) << "Request didn't have payload.";
      break;
  }

  switch (request.requested_output()) {
    case ConformanceRequest::UNSPECIFIED:
      GOOGLE_LOG(FATAL) << "Unspecified output format";
      break;

    case ConformanceRequest::PROTOBUF:
      test_message.SerializeToString(response->mutable_protobuf_payload());
      break;

    case ConformanceRequest::JSON:
      response->set_runtime_error("JSON output is not yet supported.");
      break;
  }
}

bool DoTestIo() {
  string serialized_input;
  string serialized_output;
  ConformanceRequest request;
  ConformanceResponse response;
  uint32_t bytes;

  if (!CheckedRead(STDIN_FILENO, &bytes, sizeof(uint32_t))) {
    // EOF.
    return false;
  }

  serialized_input.resize(bytes);

  if (!CheckedRead(STDIN_FILENO, (char*)serialized_input.c_str(), bytes)) {
    GOOGLE_LOG(ERROR) << "Unexpected EOF on stdin. " << strerror(errno);
  }

  if (!request.ParseFromString(serialized_input)) {
    GOOGLE_LOG(FATAL) << "Parse of ConformanceRequest proto failed.";
    return false;
  }

  DoTest(request, &response);

  response.SerializeToString(&serialized_output);

  bytes = serialized_output.size();
  CheckedWrite(STDOUT_FILENO, &bytes, sizeof(uint32_t));
  CheckedWrite(STDOUT_FILENO, serialized_output.c_str(), bytes);

  if (verbose) {
    fprintf(stderr, "conformance-cpp: request=%s, response=%s\n",
            request.ShortDebugString().c_str(),
            response.ShortDebugString().c_str());
  }

  test_count++;

  return true;
}

int main() {
  while (1) {
    if (!DoTestIo()) {
      fprintf(stderr, "conformance-cpp: received EOF from test runner "
                      "after %d tests, exiting\n", test_count);
      return 0;
    }
  }
}
