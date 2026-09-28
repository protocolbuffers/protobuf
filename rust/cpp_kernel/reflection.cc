#include <string>

#include "absl/log/absl_check.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "rust/cpp_kernel/strings.h"
#include "google/protobuf/text_format.h"

extern "C" {

// Converts a message to its TextFormat representation.
google::protobuf::rust::RustStringRawParts proto2_rust_Message_print_to_text_format(
    const google::protobuf::MessageLite* m) {
  const auto* msg = google::protobuf::DynamicCastMessage<google::protobuf::Message>(m);
  // This should always hold because the Rust API requires `WithReflection`.
  ABSL_CHECK(msg != nullptr) << "Message does not support reflection.";
  std::string text;
  ABSL_CHECK(google::protobuf::TextFormat::PrintToString(*msg, &text));
  return google::protobuf::rust::RustStringRawParts(text);
}

// Parses a message from its TextFormat representation.
bool proto2_rust_Message_parse_from_text_format(google::protobuf::MessageLite* m,
                                                google::protobuf::rust::PtrAndLen input) {
  auto* msg = google::protobuf::DynamicCastMessage<google::protobuf::Message>(m);
  // This should always hold because the Rust API requires `WithReflection`.
  ABSL_CHECK(msg != nullptr) << "Message does not support reflection.";
  return google::protobuf::TextFormat::ParseFromString(input.AsStringView(), msg);
}

}  // extern "C"
