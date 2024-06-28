#include "rust/cpp_kernel/debug.h"

#include <string>

#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "rust/cpp_kernel/strings.h"

extern "C" {

google::protobuf::rust_internal::RustStringRawParts utf8_debug_string(
    const google::protobuf::Message* msg) {
  std::string text = google::protobuf::Utf8Format(*msg);
  return google::protobuf::rust_internal::RustStringRawParts(text);
}

google::protobuf::rust_internal::RustStringRawParts utf8_debug_string_lite(
    const google::protobuf::MessageLite* msg) {
  std::string text = google::protobuf::Utf8Format(*msg);
  return google::protobuf::rust_internal::RustStringRawParts(text);
}

}  // extern "C"
