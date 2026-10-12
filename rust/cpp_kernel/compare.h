#ifndef GOOGLE_PROTOBUF_RUST_CPP_KERNEL_COMPARE_H__
#define GOOGLE_PROTOBUF_RUST_CPP_KERNEL_COMPARE_H__

#include "google/protobuf/message_lite.h"

extern "C" {

bool proto2_rust_messagelite_equals(const google::protobuf::MessageLite* msg1,
                                    const google::protobuf::MessageLite* msg2);

bool proto2_rust_messagelite_partially_equals(
    const google::protobuf::MessageLite* actual, const google::protobuf::MessageLite* expected);

}  // extern "C"

#endif  // GOOGLE_PROTOBUF_RUST_CPP_KERNEL_COMPARE_H__
