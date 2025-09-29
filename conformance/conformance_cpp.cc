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
#include <memory>
#include <string>

#include "google/protobuf/any.pb.h"
#include "google/protobuf/api.pb.h"
#include "google/protobuf/duration.pb.h"
#include "google/protobuf/empty.pb.h"
#include "google/protobuf/field_mask.pb.h"
#include "google/protobuf/struct.pb.h"
#include "google/protobuf/timestamp.pb.h"
#include "google/protobuf/type.pb.h"
#include "google/protobuf/wrappers.pb.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "conformance/conformance.pb.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "editions/golden/test_messages_proto2_editions.pb.h"
#include "editions/golden/test_messages_proto3_editions.pb.h"
#include "google/protobuf/endian.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/message.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/json_util.h"
#include "google/protobuf/util/type_resolver.h"
#include "google/protobuf/util/type_resolver_util.h"
#include "google/protobuf/stubs/status_macros.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace {
using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::google::protobuf::util::JsonParseOptions;
using ::google::protobuf::util::JsonStringToMessage;
using ::google::protobuf::util::MessageToJsonString;
using ::google::protobuf::util::NewTypeResolverForDescriptorPool;
using ::google::protobuf::util::TypeResolver;
using ::protobuf_test_messages::editions::TestAllTypesEdition2023;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;
using ::protobuf_test_messages::proto3::TestAllTypesProto3;
using TestAllTypesProto2Editions =
    ::protobuf_test_messages::editions::proto2::TestAllTypesProto2;
using TestAllTypesProto3Editions =
    ::protobuf_test_messages::editions::proto3::TestAllTypesProto3;

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
    google::protobuf::LinkMessageReflection<TestAllTypesEdition2023>();
    google::protobuf::LinkMessageReflection<TestAllTypesProto2Editions>();
    google::protobuf::LinkMessageReflection<TestAllTypesProto3Editions>();

    // Force link one wkt from each wkt file.
    google::protobuf::LinkMessageReflection<google::protobuf::Any>();
    google::protobuf::LinkMessageReflection<google::protobuf::Api>();
    google::protobuf::LinkMessageReflection<google::protobuf::Duration>();
    google::protobuf::LinkMessageReflection<google::protobuf::Empty>();
    google::protobuf::LinkMessageReflection<google::protobuf::FieldMask>();
    google::protobuf::LinkMessageReflection<google::protobuf::Struct>();
    google::protobuf::LinkMessageReflection<google::protobuf::Timestamp>();
    google::protobuf::LinkMessageReflection<google::protobuf::Type>();
    google::protobuf::LinkMessageReflection<google::protobuf::DoubleValue>();

    resolver_.reset(NewTypeResolverForDescriptorPool(
        "type.googleapis.com", DescriptorPool::generated_pool()));
  }

  absl::StatusOr<ConformanceResponse> RunTest(
      const ConformanceRequest& request);

  // Returns Ok(true) if we're done processing requests.
  absl::StatusOr<bool> ServeConformanceRequest();

 private:
  bool verbose_ = false;
  std::unique_ptr<TypeResolver> resolver_;
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
  std::string type_url =
      absl::StrCat("type.googleapis.com/", request.message_type());

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
      absl::Status status = JsonStringToMessage(request.json_payload(),
                                                test_message.get(), options);
      if (!status.ok()) {
        response.set_parse_error(
            absl::StrCat("parse error: ", status.message()));
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
      absl::Status status =
          MessageToJsonString(*test_message, response.mutable_json_payload());
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
  in_len = internal::little_endian::ToHost(in_len);

  std::string serialized_input;
  serialized_input.resize(in_len);
  RETURN_IF_ERROR(ReadFd(STDIN_FILENO, &serialized_input[0], in_len));

  ConformanceRequest request;
  ABSL_CHECK(request.ParseFromString(serialized_input));

  absl::StatusOr<ConformanceResponse> response = RunTest(request);
  RETURN_IF_ERROR(response.status());

  std::string serialized_output;
  response->SerializeToString(&serialized_output);

  uint32_t out_len = internal::little_endian::FromHost(
      static_cast<uint32_t>(serialized_output.size()));

  RETURN_IF_ERROR(WriteFd(STDOUT_FILENO, &out_len, sizeof(out_len)));
  RETURN_IF_ERROR(WriteFd(STDOUT_FILENO, serialized_output.data(),
                          serialized_output.size()));

  if (verbose_) {
    ABSL_LOG(INFO) << "conformance-cpp: request="
                   << google::protobuf::ShortFormat(request)
                   << ", response=" << google::protobuf::ShortFormat(*response);
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

#include "google/protobuf/port_undef.inc"
