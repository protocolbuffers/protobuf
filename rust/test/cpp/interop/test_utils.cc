// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstddef>
#include <cstdint>

#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "rust/cpp_kernel/serialized_data.h"
#include "rust/cpp_kernel/strings.h"
#include "rust/test/cpp/interop/interop_test.pb.h"

using google::protobuf::rust::SerializedData;
using google::protobuf::rust::SerializeMsg;
using rust_cpp_interop_test::InteropTestMessage;

extern "C" void MutateInteropTestMessage(InteropTestMessage* msg) {
  msg->set_i64(42);
  msg->set_bytes("something mysterious");
  msg->set_b(false);
}

extern "C" SerializedData SerializeInteropTestMessage(
    const InteropTestMessage* msg) {
  SerializedData data;
  ABSL_CHECK(SerializeMsg(msg, &data));
  return data;
}

extern "C" void DeleteInteropTestMessage(InteropTestMessage* msg) {
  delete msg;
}

extern "C" void* DeserializeInteropTestMessage(const void* data, size_t size) {
  auto* proto = new InteropTestMessage;
  proto->ParseFromArray(data, static_cast<int>(size));
  return proto;
}

extern "C" void* NewWithExtension() {
  auto* proto = new InteropTestMessage;
  proto->SetExtension(rust_cpp_interop_test::bytes_extension, "smuggled");
  return proto;
}

extern "C" google::protobuf::rust::PtrAndLen GetBytesExtension(
    const InteropTestMessage* proto) {
  absl::string_view bytes =
      proto->GetExtension(rust_cpp_interop_test::bytes_extension);
  return {bytes.data(), bytes.size()};
}

extern "C" int64_t TakeOwnershipAndGetOptionalInt64(InteropTestMessage* msg) {
  int64_t i = msg->i64();
  delete msg;
  return i;
}

extern "C" const void* GetConstStaticInteropTestMessage() {
  static const auto* msg = new InteropTestMessage;
  return msg;
}
