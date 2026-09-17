#include <string>

#include "absl/log/absl_check.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "rust/cpp_kernel/strings.h"
#include "google/protobuf/text_format.h"

// This file is only built into `cpp_api`, never `cpp_api_lite`, so it can
// depend on the full runtime unconditionally and needs no
// PROTOBUF_RUST_LITE_RUNTIME guards. See `cpp_kernel/reflection.rs` for the
// Rust side of the same split.

extern "C" {

// Returns a pointer to the descriptor of the message, or nullptr if
// the message is not google::protobuf::Message.
const void* proto2_rust_Message_get_descriptor(const google::protobuf::MessageLite* m) {
  auto msg = google::protobuf::DynamicCastMessage<google::protobuf::Message>(m);
  if (msg == nullptr) {
    return nullptr;
  }
  return msg->GetDescriptor();
}

// Converts a message to its TextFormat representation. The caller must have
// checked that the message has reflection (see `WithReflection`).
google::protobuf::rust::RustStringRawParts proto2_rust_Message_print_to_text_format(
    const google::protobuf::MessageLite* m) {
  const auto* msg = google::protobuf::DynamicCastMessage<google::protobuf::Message>(m);
  ABSL_CHECK(msg != nullptr) << "Message does not support reflection.";
  std::string text;
  ABSL_CHECK(google::protobuf::TextFormat::PrintToString(*msg, &text));
  return google::protobuf::rust::RustStringRawParts(text);
}

}  // extern "C"
