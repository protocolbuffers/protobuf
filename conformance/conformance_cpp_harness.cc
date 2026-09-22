// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/conformance_cpp_harness.h"

#include <memory>

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
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "conformance/conformance.pb.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "conformance/test_protos/test_messages_edition_unstable.pb.h"
#include "google/protobuf/descriptor.h"
#include "editions/golden/test_messages_proto2_editions.pb.h"
#include "editions/golden/test_messages_proto3_editions.pb.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/message.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/json_util.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::google::protobuf::json::JsonStringToMessage;
using ::google::protobuf::json::MessageToJsonString;
using ::google::protobuf::util::JsonParseOptions;
using ::protobuf_test_messages::edition_unstable::TestAllTypesEditionUnstable;
using ::protobuf_test_messages::editions::TestAllTypesEdition2023;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;
using ::protobuf_test_messages::proto3::TestAllTypesProto3;
using TestAllTypesProto2Editions =
    ::protobuf_test_messages::editions::proto2::TestAllTypesProto2;
using TestAllTypesProto3Editions =
    ::protobuf_test_messages::editions::proto3::TestAllTypesProto3;

}  // namespace

CppConformanceHarness::CppConformanceHarness() {
  google::protobuf::LinkMessageReflection<TestAllTypesProto2>();
  google::protobuf::LinkMessageReflection<TestAllTypesProto3>();
  google::protobuf::LinkMessageReflection<TestAllTypesEdition2023>();
  google::protobuf::LinkMessageReflection<TestAllTypesEditionUnstable>();
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
}

absl::StatusOr<ConformanceResponse> CppConformanceHarness::RunTest(
    const ConformanceRequest& request) const {
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

  // Optional request features this testee does not implement yet.
  if (request.discard_unknown_fields()) {
    response.set_skipped("discard_unknown_fields is not supported");
    return response;
  }
  if (request.merge_payload_case() !=
      ConformanceRequest::MERGE_PAYLOAD_NOT_SET) {
    response.set_skipped("merge_payload is not supported");
    return response;
  }

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
           ::conformance::JSON_IGNORE_UNKNOWN_PARSING_TEST);
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
    case ::conformance::UNSPECIFIED:
      return absl::InvalidArgumentError("unspecified output format");

    case ::conformance::PROTOBUF: {
      ABSL_CHECK(
          test_message->SerializeToString(response.mutable_protobuf_payload()));
      break;
    }

    case ::conformance::JSON: {
      absl::Status status =
          MessageToJsonString(*test_message, response.mutable_json_payload());
      if (!status.ok()) {
        response.set_serialize_error(absl::StrCat(
            "failed to serialize JSON output: ", status.message()));
      }
      break;
    }

    case ::conformance::TEXT_FORMAT: {
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

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
