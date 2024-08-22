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
#include "google/protobuf/unittest.pb.h"

using google::protobuf::rust::SerializedData;
using google::protobuf::rust::SerializeMsg;

extern "C" void MutateTestAllTypes(protobuf_unittest::TestAllTypes* msg) {
  msg->set_optional_int64(42);
  msg->set_optional_bytes("something mysterious");
  msg->set_optional_bool(false);
}

extern "C" SerializedData SerializeTestAllTypes(
    const protobuf_unittest::TestAllTypes* msg) {
  SerializedData data(nullptr, 0);
  ABSL_CHECK(SerializeMsg(msg, &data));
  return data;
}

extern "C" void DeleteTestAllTypes(protobuf_unittest::TestAllTypes* msg) {
  delete msg;
}

extern "C" void* DeserializeTestAllTypes(const void* data, size_t size) {
  auto* proto = new protobuf_unittest::TestAllTypes;
  proto->ParseFromArray(data, static_cast<int>(size));
  return proto;
}

extern "C" void* NewWithExtension() {
  auto* proto = new protobuf_unittest::TestAllExtensions;
  proto->SetExtension(protobuf_unittest::optional_bytes_extension, "smuggled");
  return proto;
}

extern "C" google::protobuf::rust::PtrAndLen GetBytesExtension(
    const protobuf_unittest::TestAllExtensions* proto) {
  absl::string_view bytes =
      proto->GetExtension(protobuf_unittest::optional_bytes_extension);
  return {bytes.data(), bytes.size()};
}

extern "C" int32_t TakeOwnershipAndGetOptionalInt32(
    protobuf_unittest::TestAllTypes* msg) {
  int32_t i = msg->optional_int32();
  delete msg;
  return i;
}

extern "C" const void* GetConstStaticTestAllTypes() {
  static const auto* msg = new protobuf_unittest::TestAllTypes;
  return msg;
}
