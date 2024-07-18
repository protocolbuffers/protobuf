#include "rust/cpp_kernel/debug.h"

#include <string>

#include "google/protobuf/message_lite.h"
#include "rust/cpp_kernel/strings.h"

extern "C" {

google::protobuf::rust::RustStringRawParts proto2_rust_utf8_debug_string(
    const google::protobuf::MessageLite* msg) {
  std::string text = google::protobuf::Utf8Format(*msg);
  return google::protobuf::rust::RustStringRawParts(text);
}

}  // extern "C"
