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

#include <memory>
#include <string>
#include <utility>

#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/json_util.h"
#include "google/protobuf/util/type_resolver_util.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "conformance/conformance.pb.h"
#include "conformance/conformance.pb.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/util/type_resolver.h"
#include "google/protobuf/stubs/status_macros.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace {
using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::google::protobuf::util::BinaryToJsonString;
using ::google::protobuf::util::JsonParseOptions;
using ::google::protobuf::util::JsonToBinaryString;
using ::google::protobuf::util::NewTypeResolverForDescriptorPool;
using ::google::protobuf::util::TypeResolver;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;
using ::protobuf_test_messages::proto3::TestAllTypesProto3;

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

class Harness {
 public:
  Harness() {
    google::protobuf::LinkMessageReflection<TestAllTypesProto2>();
    google::protobuf::LinkMessageReflection<TestAllTypesProto3>();

    resolver_.reset(NewTypeResolverForDescriptorPool(
        "type.googleapis.com", DescriptorPool::generated_pool()));
    type_url_ = absl::StrCat("type.googleapis.com/",
                             TestAllTypesProto3::GetDescriptor()->full_name());
  }

  absl::StatusOr<ConformanceResponse> RunTest(
      const ConformanceRequest& request);

  // Returns Ok(true) if we're done processing requests.
  absl::StatusOr<bool> ServeConformanceRequest();

 private:
  bool verbose_ = false;
  std::unique_ptr<TypeResolver> resolver_;
  std::string type_url_;
};

absl::StatusOr<ConformanceResponse> Harness::RunTest(
    const ConformanceRequest& request) {
  const Descriptor* descriptor =
      DescriptorPool::generated_pool()->FindMessageTypeByName(
          request.message_type());
  if (descriptor == nullptr) {
    return absl::NotFoundError(
        absl::StrCat("No such message type: ", request.message_type()));
  }

  std::unique_ptr<Message> test_message(
      MessageFactory::generated_factory()->GetPrototype(descriptor)->New());
  ConformanceResponse response;

  switch (request.payload_case()) {
    case ConformanceRequest::kProtobufPayload: {
      if (!test_message->ParseFromString(request.protobuf_payload())) {
        response.set_parse_error("parse error (no more details available)");
        return response;
      }
      break;
    }

    case ConformanceRequest::kJsonPayload: {
      JsonParseOptions options;
      options.ignore_unknown_fields =
          (request.test_category() ==
           conformance::JSON_IGNORE_UNKNOWN_PARSING_TEST);

      std::string proto_binary;
      absl::Status status =
          JsonToBinaryString(resolver_.get(), type_url_, request.json_payload(),
                             &proto_binary, options);
      if (!status.ok()) {
        response.set_parse_error(
            absl::StrCat("parse error: ", status.message()));
        return response;
      }

      if (!test_message->ParseFromString(proto_binary)) {
        response.set_runtime_error(
            "parsing JSON generated invalid proto output");
        return response;
      }

      break;
    }

    case ConformanceRequest::kTextPayload: {
      if (!TextFormat::ParseFromString(request.text_payload(),
                                       test_message.get())) {
        response.set_parse_error("parse error (no more details available)");
        return response;
      }
      break;
    }

    case ConformanceRequest::PAYLOAD_NOT_SET:
      return absl::InvalidArgumentError("request didn't have payload");

    default:
      return absl::InvalidArgumentError(
          absl::StrCat("unknown payload type", request.payload_case()));
  }

  switch (request.requested_output_format()) {
    case conformance::UNSPECIFIED:
      return absl::InvalidArgumentError("unspecified output format");

    case conformance::PROTOBUF: {
      ABSL_CHECK(
          test_message->SerializeToString(response.mutable_protobuf_payload()));
      break;
    }

    case conformance::JSON: {
      std::string proto_binary;
      ABSL_CHECK(test_message->SerializeToString(&proto_binary));
      absl::Status status =
          BinaryToJsonString(resolver_.get(), type_url_, proto_binary,
                             response.mutable_json_payload());
      if (!status.ok()) {
        response.set_serialize_error(absl::StrCat(
            "failed to serialize JSON output: ", status.message()));
      }
      break;
    }

    case conformance::TEXT_FORMAT: {
      TextFormat::Printer printer;
      printer.SetHideUnknownFields(!request.print_unknown_fields());
      ABSL_CHECK(printer.PrintToString(*test_message,
                                       response.mutable_text_payload()));
      break;
    }

    default:
      return absl::InvalidArgumentError(absl::StrCat(
          "unknown output format", request.requested_output_format()));
  }

  return response;
}

absl::StatusOr<bool> Harness::ServeConformanceRequest() {
  uint32_t in_len;
  if (!ReadFd(STDIN_FILENO, reinterpret_cast<char*>(&in_len), sizeof(in_len))
           .ok()) {
    // EOF means we're done.
    return true;
  }

  std::string serialized_input;
  serialized_input.resize(in_len);
  RETURN_IF_ERROR(ReadFd(STDIN_FILENO, &serialized_input[0], in_len));

  ConformanceRequest request;
  ABSL_CHECK(request.ParseFromString(serialized_input));

  absl::StatusOr<ConformanceResponse> response = RunTest(request);
  RETURN_IF_ERROR(response.status());

  std::string serialized_output;
  response->SerializeToString(&serialized_output);

  uint32_t out_len = static_cast<uint32_t>(serialized_output.size());
  RETURN_IF_ERROR(WriteFd(STDOUT_FILENO, &out_len, sizeof(out_len)));
  RETURN_IF_ERROR(WriteFd(STDOUT_FILENO, serialized_output.data(), out_len));

  if (verbose_) {
    ABSL_LOG(INFO) << "conformance-cpp: request=" << request.ShortDebugString()
                   << ", response=" << response->ShortDebugString();
  }
  return false;
}
}  // namespace
}  // namespace protobuf
}  // namespace google

int main() {
  google::protobuf::Harness harness;
  int total_runs = 0;
  while (true) {
    auto is_done = harness.ServeConformanceRequest();
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
