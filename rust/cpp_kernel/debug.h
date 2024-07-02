#ifndef GOOGLE_PROTOBUF_RUST_CPP_KERNEL_DEBUG_H__
#define GOOGLE_PROTOBUF_RUST_CPP_KERNEL_DEBUG_H__

#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "rust/cpp_kernel/strings.h"

extern "C" {

google::protobuf::rust::RustStringRawParts proto2_rust_utf8_debug_string(
    const google::protobuf::Message* msg);

google::protobuf::rust::RustStringRawParts proto2_rust_utf8_debug_string_lite(
    const google::protobuf::MessageLite* msg);

}  // extern "C"

#endif  // GOOGLE_PROTOBUF_RUST_CPP_KERNEL_DEBUG_H__
