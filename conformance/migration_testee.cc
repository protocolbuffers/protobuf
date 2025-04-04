// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <ios>
#include <string>
#include <utility>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "conformance/conformance.pb.h"
#include "conformance/migration_list.pb.h"
#include "google/protobuf/endian.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/json_util.h"
#include "google/protobuf/stubs/status_macros.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

ABSL_FLAG(std::string, output, "", "The output file to write requests to.");

namespace google {
namespace protobuf {
namespace {
using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::conformance::MigrationList;
using ::google::protobuf::util::JsonParseOptions;

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

void TruncatePayload(std::string* payload) {
  // We shouldn't need over 1000 bytes of the payload to uniquely identify it.
  // Dealing with the full payload of performance tests is cumbersome.
  if (payload->size() > 1000) {
    payload->resize(1000);
  }
}

void SanitizeRequest(ConformanceRequest* request) {
  if (request->has_protobuf_payload()) {
    TruncatePayload(request->mutable_protobuf_payload());
  }
  if (request->has_json_payload()) {
    TruncatePayload(request->mutable_json_payload());
  }
  if (request->has_text_payload()) {
    TruncatePayload(request->mutable_text_payload());
  }
}

class Harness {
 public:
  void Write(ConformanceRequest request) {
    std::ofstream file;
    file.open(absl::GetFlag(FLAGS_output), std::ios_base::app);
    MigrationList list;
    SanitizeRequest(&request);
    *list.add_requests() = std::move(request);
    std::string out;
    ABSL_CHECK(TextFormat::PrintToString(list, &out));
    file << out;
  }

  absl::StatusOr<bool> ServeConformanceRequest() {
    uint32_t in_len;
    if (!ReadFd(STDIN_FILENO, reinterpret_cast<char*>(&in_len), sizeof(in_len))
             .ok()) {
      // EOF means we're done.
      return true;
    }
    in_len = internal::little_endian::ToHost(in_len);

    std::string serialized_input;
    serialized_input.resize(in_len);
    RETURN_IF_ERROR(ReadFd(STDIN_FILENO, &serialized_input[0], in_len));

    ConformanceRequest request;
    ABSL_CHECK(request.ParseFromString(serialized_input));

    Write(std::move(request));

    ConformanceResponse response;
    response.set_skipped("skipping all tests");

    std::string serialized_output;
    response.SerializeToString(&serialized_output);

    uint32_t out_len = internal::little_endian::FromHost(
        static_cast<uint32_t>(serialized_output.size()));

    RETURN_IF_ERROR(WriteFd(STDOUT_FILENO, &out_len, sizeof(out_len)));
    RETURN_IF_ERROR(WriteFd(STDOUT_FILENO, serialized_output.data(),
                            serialized_output.size()));
    return false;
  }
};

}  // namespace
}  // namespace protobuf
}  // namespace google

int main(int argc, char* argv[]) {
  absl::ParseCommandLine(argc, argv);
  google::protobuf::Harness harness;
  while (true) {
    auto is_done = harness.ServeConformanceRequest();
    if (!is_done.ok()) {
      ABSL_LOG(FATAL) << is_done.status();
    }
    if (*is_done) {
      break;
    }
  }
}

#include "google/protobuf/port_undef.inc"
