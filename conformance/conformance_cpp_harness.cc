// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/conformance_cpp_harness.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/any.pb.h"
#include "google/protobuf/api.pb.h"
#include "google/protobuf/duration.pb.h"
#include "google/protobuf/empty.pb.h"
#include "google/protobuf/field_mask.pb.h"
#include "google/protobuf/struct.pb.h"
#include "google/protobuf/timestamp.pb.h"
#include "google/protobuf/type.pb.h"
#include "google/protobuf/wrappers.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/conformance.pb.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "conformance/test_protos/test_messages_edition_unstable.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
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

using ::conformance::ConformanceAction;
using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::conformance::ConformanceResults;
using ::conformance::DiscardUnknownFieldsAction;
using ::conformance::MergeAction;
using ::conformance::NewAction;
using ::conformance::ParseAction;
using ::conformance::SerializeAction;
using ::conformance::SerializeResult;
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

// Force-links the reflection data of the message types `Ts` into the binary
// and returns the files that define them.  Calling Ts::descriptor() already
// references the generated descriptor data, so the messages are linked in
// either way; LinkMessageReflection is kept to state the intent.
template <typename... Ts>
std::vector<const FileDescriptor*> LinkMessageReflectionAndGetFiles() {
  (google::protobuf::LinkMessageReflection<Ts>(), ...);
  return {Ts::descriptor()->file()...};
}

// Builds `file`, after its transitive dependencies, in `pool` from the
// FileDescriptorProto it was built from.  Files already in the pool are
// skipped.  A file that fails to build is fatal, right after the pool has
// logged what is wrong with it.
void BuildFileWithDependencies(const FileDescriptor& file,
                               DescriptorPool& pool) {
  if (pool.FindFileByName(file.name()) != nullptr) return;
  for (int i = 0; i < file.dependency_count(); ++i) {
    BuildFileWithDependencies(*file.dependency(i), pool);
  }
  FileDescriptorProto proto;
  file.CopyTo(&proto);
  ABSL_CHECK(pool.BuildFile(proto) != nullptr)
      << "failed to build " << file.name();
}

// -----------------------------------------------------------------------------
// The parsers and serializers, shared by both protocol versions so that the
// same test yields the same bytes whichever version carries it.  A parse
// failure is returned as the text of the parse_error to answer with, a
// serialize failure as the text of the serialize_error; the option handling
// (JSON leniency about unknown fields, printing unknown fields in text format)
// is here and nowhere else.
// -----------------------------------------------------------------------------

// Parses the wire-format `data` into `message`, replacing its contents.
absl::Status ParseBinaryPayload(absl::string_view data, Message& message) {
  if (!message.ParseFromString(data)) {
    return absl::InvalidArgumentError(
        "parse error (no more details available)");
  }
  return absl::OkStatus();
}

// Parses the JSON `data` into `message`, replacing its contents; unknown JSON
// fields fail the parse unless `ignore_unknown_fields`.
absl::Status ParseJsonPayload(absl::string_view data,
                              bool ignore_unknown_fields, Message& message) {
  JsonParseOptions options;
  options.ignore_unknown_fields = ignore_unknown_fields;
  absl::Status status = JsonStringToMessage(data, &message, options);
  if (!status.ok()) {
    return absl::InvalidArgumentError(
        absl::StrCat("parse error: ", status.message()));
  }
  return absl::OkStatus();
}

// Parses the text-format `data` into `message`, replacing its contents.
absl::Status ParseTextPayload(absl::string_view data, Message& message) {
  if (!TextFormat::ParseFromString(data, &message)) {
    return absl::InvalidArgumentError(
        "parse error (no more details available)");
  }
  return absl::OkStatus();
}

// The wire-format serialization of `message`.  Serializing to the wire format
// can't fail for the test messages (they have no required fields that the
// serializer checks), so a failure is a bug in the harness rather than a
// serialize_error.
std::string SerializeBinaryPayload(const Message& message) {
  std::string data;
  ABSL_CHECK(message.SerializeToString(&data));
  return data;
}

// The JSON serialization of `message`, or the serialize_error text.
absl::StatusOr<std::string> SerializeJsonPayload(const Message& message) {
  std::string data;
  absl::Status status = MessageToJsonString(message, &data);
  if (!status.ok()) {
    return absl::InternalError(
        absl::StrCat("failed to serialize JSON output: ", status.message()));
  }
  return data;
}

// The text-format serialization of `message`, with its unknown fields printed
// by field number iff `print_unknown_fields` and dropped otherwise.
std::string SerializeTextPayload(const Message& message,
                                 bool print_unknown_fields) {
  TextFormat::Printer printer;
  printer.SetHideUnknownFields(!print_unknown_fields);
  std::string data;
  ABSL_CHECK(printer.PrintToString(message, &data));
  return data;
}

// -----------------------------------------------------------------------------
// Protocol version 2: the actions.  Each Run*Action() runs one action against
// `messages`, the handles created so far, and returns the error to answer
// with if it failed (see ActionError), nullopt otherwise.  The caller reports
// the error under the action's index.
// -----------------------------------------------------------------------------

// The error of a failed action: which field of ConformanceResponse.result it
// is reported in, and its text.
struct ActionError {
  enum class Kind {
    kParseError,      // ConformanceResponse.parse_error
    kSerializeError,  // ConformanceResponse.serialize_error
    kRuntimeError,    // ConformanceResponse.runtime_error
  };
  Kind kind;
  std::string message;
};

using MessageHandles = absl::flat_hash_map<int, std::unique_ptr<Message>>;

ActionError RuntimeError(std::string message) {
  return {ActionError::Kind::kRuntimeError, std::move(message)};
}

// The message held as handle `id`, or null (and the runtime_error to answer
// with in `error`) if no action created it.
Message* FindMessage(const MessageHandles& messages, int id,
                     absl::optional<ActionError>& error) {
  auto it = messages.find(id);
  if (it == messages.end()) {
    error = RuntimeError(absl::StrCat(
        "message ", id, " does not exist: no earlier action ", "created it"));
    return nullptr;
  }
  return it->second.get();
}

// The runtime_error for creating handle `id` as a message of the type named
// `type`, whose prototype the caller looked up (`prototype`, null if there is
// no such type): the type doesn't exist, or the handle already does.  Nullopt
// if the message may be created.
absl::optional<ActionError> CheckCanCreate(int id, absl::string_view type,
                                           const Message* prototype,
                                           const MessageHandles& messages) {
  if (prototype == nullptr) {
    return RuntimeError(absl::StrCat("No such message type: ", type));
  }
  if (messages.contains(id)) {
    return RuntimeError(absl::StrCat(
        "message ", id, " already exists: an earlier action created it"));
  }
  return absl::nullopt;
}

// ParseAction: creates handle `id` from the payload.  A payload that doesn't
// parse is a parse_error, and the handle isn't created.
absl::optional<ActionError> RunParseAction(const ParseAction& parse,
                                           const Message* prototype,
                                           MessageHandles& messages) {
  absl::optional<ActionError> error =
      CheckCanCreate(parse.id(), parse.type(), prototype, messages);
  if (error.has_value()) return error;

  std::unique_ptr<Message> message(prototype->New());
  absl::Status status;
  switch (parse.payload_case()) {
    case ParseAction::kBinary:
      status = ParseBinaryPayload(parse.binary().data(), *message);
      break;
    case ParseAction::kJson:
      status = ParseJsonPayload(parse.json().data(),
                                parse.json().ignore_unknown_fields(), *message);
      break;
    case ParseAction::kText:
      status = ParseTextPayload(parse.text().data(), *message);
      break;
    case ParseAction::PAYLOAD_NOT_SET:
      // A payload kind of a newer protocol version (an unknown field here),
      // or none at all.
      return RuntimeError(
          absl::StrCat("parse action for message ", parse.id(),
                       " has no payload this testee knows (it implements "
                       "protocol version ",
                       CppConformanceHarness::kProtocolVersion, ")"));
      // No default: a new member of the oneof must be handled here.
  }
  if (!status.ok()) {
    return ActionError{ActionError::Kind::kParseError,
                       std::string(status.message())};
  }
  messages[parse.id()] = std::move(message);
  return absl::nullopt;
}

// NewAction: creates handle `id` as an empty message.
absl::optional<ActionError> RunNewAction(const NewAction& new_message,
                                         const Message* prototype,
                                         MessageHandles& messages) {
  absl::optional<ActionError> error =
      CheckCanCreate(new_message.id(), new_message.type(), prototype, messages);
  if (error.has_value()) return error;
  messages[new_message.id()] = std::unique_ptr<Message>(prototype->New());
  return absl::nullopt;
}

// MergeAction: `to`.MergeFrom(`from`), both of which must exist, be of the
// same type and be different messages.
absl::optional<ActionError> RunMergeAction(const MergeAction& merge,
                                           MessageHandles& messages) {
  absl::optional<ActionError> error;
  const Message* from = FindMessage(messages, merge.from(), error);
  if (from == nullptr) return error;
  Message* to = FindMessage(messages, merge.to(), error);
  if (to == nullptr) return error;
  if (from == to) {
    // Message::MergeFrom() check-fails on its own instance (the reflection
    // path's ReflectionOps::Merge insists the two differ), which would take
    // the testee down; the protocol rules it out (see MergeAction).
    return RuntimeError(
        absl::StrCat("cannot merge message ", merge.to(), " into itself"));
  }
  if (from->GetDescriptor() != to->GetDescriptor()) {
    return RuntimeError(absl::StrCat(
        "cannot merge message ", merge.from(), " (",
        from->GetDescriptor()->full_name(), ") into message ", merge.to(), " (",
        to->GetDescriptor()->full_name(), "): the types differ"));
  }
  to->MergeFrom(*from);
  return absl::nullopt;
}

// DiscardUnknownFieldsAction: Message::DiscardUnknownFields() on `id`.
absl::optional<ActionError> RunDiscardUnknownFieldsAction(
    const DiscardUnknownFieldsAction& discard, MessageHandles& messages) {
  absl::optional<ActionError> error;
  Message* message = FindMessage(messages, discard.id(), error);
  if (message == nullptr) return error;
  message->DiscardUnknownFields();
  return absl::nullopt;
}

// SerializeAction: appends the serialization of `id` in the requested format
// to `results`.  A message that doesn't serialize is a serialize_error.
absl::optional<ActionError> RunSerializeAction(const SerializeAction& serialize,
                                               const MessageHandles& messages,
                                               ConformanceResults& results) {
  absl::optional<ActionError> error;
  const Message* message = FindMessage(messages, serialize.id(), error);
  if (message == nullptr) return error;

  SerializeResult result;
  switch (serialize.format_case()) {
    case SerializeAction::kBinary:
      result.set_binary(SerializeBinaryPayload(*message));
      break;
    case SerializeAction::kJson: {
      absl::StatusOr<std::string> json = SerializeJsonPayload(*message);
      if (!json.ok()) {
        return ActionError{ActionError::Kind::kSerializeError,
                           std::string(json.status().message())};
      }
      result.set_json(*std::move(json));
      break;
    }
    case SerializeAction::kText:
      result.set_text(SerializeTextPayload(
          *message, serialize.text().print_unknown_fields()));
      break;
    case SerializeAction::FORMAT_NOT_SET:
      // A format of a newer protocol version (an unknown field here), or
      // none at all.
      return RuntimeError(
          absl::StrCat("serialize action for message ", serialize.id(),
                       " has no format this testee knows (it implements "
                       "protocol version ",
                       CppConformanceHarness::kProtocolVersion, ")"));
      // No default: a new member of the oneof must be handled here.
  }
  *results.add_serialized() = std::move(result);
  return absl::nullopt;
}

}  // namespace

CppConformanceHarness::CppConformanceHarness(
    MessageImplementation implementation)
    : implementation_(implementation) {
  // The test messages, and one well-known type from each well-known type
  // file (the test messages depend on them, and Any payloads and the harness's
  // own tests name them).
  std::vector<const FileDescriptor*> files = LinkMessageReflectionAndGetFiles<
      TestAllTypesProto2, TestAllTypesProto3, TestAllTypesEdition2023,
      TestAllTypesEditionUnstable, TestAllTypesProto2Editions,
      TestAllTypesProto3Editions, google::protobuf::Any, google::protobuf::Api,
      google::protobuf::Duration, google::protobuf::Empty,
      google::protobuf::FieldMask, google::protobuf::Struct,
      google::protobuf::Timestamp, google::protobuf::Type,
      google::protobuf::DoubleValue>();

  switch (implementation) {
    case MessageImplementation::kGenerated:
      pool_ = DescriptorPool::generated_pool();
      factory_ = MessageFactory::generated_factory();
      break;

    case MessageImplementation::kDynamic:
      // A pool of its own, built eagerly from FileDescriptorProtos rather than
      // an underlay of the generated pool, so that nothing about the
      // descriptors (features, options, ...) comes from the generated code,
      // and a file that doesn't build fails right here rather than as a
      // missing message type in the first request.
      dynamic_pool_ = std::make_unique<DescriptorPool>();
      for (const FileDescriptor* file : files) {
        BuildFileWithDependencies(*file, *dynamic_pool_);
      }
      dynamic_factory_ = std::make_unique<DynamicMessageFactory>();
      // The default, made explicit: the descriptors aren't the generated
      // pool's anyway, so every message, sub-messages included, is dynamic.
      dynamic_factory_->SetDelegateToGeneratedFactory(false);
      pool_ = dynamic_pool_.get();
      factory_ = dynamic_factory_.get();
      break;
  }
  ABSL_CHECK(pool_ != nullptr && factory_ != nullptr)
      << "unknown MessageImplementation " << static_cast<int>(implementation);
}

const Message* CppConformanceHarness::FindPrototype(
    absl::string_view full_name) const {
  const Descriptor* descriptor = pool_->FindMessageTypeByName(full_name);
  if (descriptor == nullptr) return nullptr;
  return factory_->GetPrototype(descriptor);
}

absl::StatusOr<ConformanceResponse> CppConformanceHarness::RunTest(
    const ConformanceRequest& request) const {
  absl::StatusOr<ConformanceResponse> response;
  if (request.protocol_version() > kProtocolVersion) {
    // The runner never sends this on purpose (it learns the version below
    // from the handshake), so it is a runner bug or a version mismatch worth
    // failing loudly on, not a skip.
    response.emplace();
    response->set_runtime_error(absl::StrCat(
        "unsupported protocol version ", request.protocol_version(),
        " (testee supports ", kProtocolVersion, ")"));
  } else if (request.protocol_version() >= 2) {
    response = RunActions(request);
  } else {
    response = RunLegacyTest(request);
  }
  // On every response, whatever the request's version: this is what tells the
  // runner's discovery handshake a version 2 testee from a version 1 one.
  if (response.ok()) response->set_protocol_version(kProtocolVersion);
  return response;
}

absl::StatusOr<ConformanceResponse> CppConformanceHarness::RunLegacyTest(
    const ConformanceRequest& request) const {
  const Message* prototype = FindPrototype(request.message_type());
  if (prototype == nullptr) {
    return absl::NotFoundError(
        absl::StrCat("No such message type: ", request.message_type()));
  }

  std::unique_ptr<Message> test_message(prototype->New());
  ConformanceResponse response;

  // The JSON payloads (input and merge) are lenient about unknown fields iff
  // the request's one category says so; the other formats don't care.
  const bool ignore_unknown_json_fields =
      request.test_category() ==
      ::conformance::JSON_IGNORE_UNKNOWN_PARSING_TEST;

  absl::Status status;
  switch (request.payload_case()) {
    case ConformanceRequest::kProtobufPayload:
      status = ParseBinaryPayload(request.protobuf_payload(), *test_message);
      break;

    case ConformanceRequest::kJsonPayload:
      status = ParseJsonPayload(request.json_payload(),
                                ignore_unknown_json_fields, *test_message);
      break;

    case ConformanceRequest::kTextPayload:
      status = ParseTextPayload(request.text_payload(), *test_message);
      break;

    case ConformanceRequest::PAYLOAD_NOT_SET:
      return absl::InvalidArgumentError("request didn't have payload");

    default:
      return absl::InvalidArgumentError(
          absl::StrCat("unknown payload type", request.payload_case()));
  }
  if (!status.ok()) {
    response.set_parse_error(status.message());
    return response;
  }

  // The merge payload is parsed into a fresh message that is then merged in,
  // exactly like the version 2 lowering of it, Parse(B) Merge(B -> A), so
  // that both versions merge the same way.  (Parsing JSON straight into the
  // message wouldn't be MergeFrom() anyway: the JSON parser clears a repeated
  // or message field the first time it sees it, and Clear()s
  // Struct/Value/ListValue outright.)
  if (request.merge_payload_case() !=
      ConformanceRequest::MERGE_PAYLOAD_NOT_SET) {
    std::unique_ptr<Message> merged(prototype->New());
    switch (request.merge_payload_case()) {
      case ConformanceRequest::kMergeProtobufPayload:
        status = ParseBinaryPayload(request.merge_protobuf_payload(), *merged);
        break;

      case ConformanceRequest::kMergeJsonPayload:
        status = ParseJsonPayload(request.merge_json_payload(),
                                  ignore_unknown_json_fields, *merged);
        break;

      case ConformanceRequest::kMergeTextPayload:
        status = ParseTextPayload(request.merge_text_payload(), *merged);
        break;

      case ConformanceRequest::MERGE_PAYLOAD_NOT_SET:
      default:
        return absl::InvalidArgumentError(absl::StrCat(
            "unknown merge payload type", request.merge_payload_case()));
    }
    if (!status.ok()) {
      response.set_parse_error(
          absl::StrCat("merge_payload ", status.message()));
      return response;
    }
    test_message->MergeFrom(*merged);
  }

  if (request.discard_unknown_fields()) {
    test_message->DiscardUnknownFields();
  }

  switch (request.requested_output_format()) {
    case ::conformance::UNSPECIFIED:
      return absl::InvalidArgumentError("unspecified output format");

    case ::conformance::PROTOBUF:
      response.set_protobuf_payload(SerializeBinaryPayload(*test_message));
      break;

    case ::conformance::JSON: {
      absl::StatusOr<std::string> json = SerializeJsonPayload(*test_message);
      if (!json.ok()) {
        response.set_serialize_error(json.status().message());
      } else {
        response.set_json_payload(*std::move(json));
      }
      break;
    }

    case ::conformance::TEXT_FORMAT:
      response.set_text_payload(
          SerializeTextPayload(*test_message, request.print_unknown_fields()));
      break;

    default:
      return absl::InvalidArgumentError(absl::StrCat(
          "unknown output format", request.requested_output_format()));
  }

  return response;
}

ConformanceResponse CppConformanceHarness::RunActions(
    const ConformanceRequest& request) const {
  ConformanceResponse response;
  MessageHandles messages;
  // Filled in as the actions run and moved into the response once all of them
  // succeeded; an error takes the result oneof instead.
  ConformanceResults results;

  for (int i = 0; i < request.actions_size(); ++i) {
    const ConformanceAction& action = request.actions(i);
    absl::optional<ActionError> error;
    switch (action.action_case()) {
      case ConformanceAction::kParse:
        error = RunParseAction(action.parse(),
                               FindPrototype(action.parse().type()), messages);
        break;
      case ConformanceAction::kNewMessage:
        error =
            RunNewAction(action.new_message(),
                         FindPrototype(action.new_message().type()), messages);
        break;
      case ConformanceAction::kMerge:
        error = RunMergeAction(action.merge(), messages);
        break;
      case ConformanceAction::kDiscardUnknownFields:
        error = RunDiscardUnknownFieldsAction(action.discard_unknown_fields(),
                                              messages);
        break;
      case ConformanceAction::kSerialize:
        error = RunSerializeAction(action.serialize(), messages, results);
        break;
      case ConformanceAction::ACTION_NOT_SET:
        // An action of a newer protocol version than this testee implements
        // (its member of the oneof is an unknown field here), or none at all.
        error = RuntimeError(absl::StrCat(
            "action ", i,
            " is of a kind this testee doesn't know (it implements protocol "
            "version ",
            kProtocolVersion, ")"));
        break;
        // No default: a new member of the oneof must be handled here.
    }
    if (!error.has_value()) continue;

    // Stop at the first failure and say which action it was.
    switch (error->kind) {
      case ActionError::Kind::kParseError:
        response.set_parse_error(std::move(error->message));
        break;
      case ActionError::Kind::kSerializeError:
        response.set_serialize_error(std::move(error->message));
        break;
      case ActionError::Kind::kRuntimeError:
        response.set_runtime_error(std::move(error->message));
        break;
    }
    response.set_failed_action(i);
    return response;
  }

  // Every action succeeded: the results, empty if nothing was serialized (the
  // oneof is still set, so that the answer isn't mistaken for no answer).
  *response.mutable_results() = std::move(results);
  return response;
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
