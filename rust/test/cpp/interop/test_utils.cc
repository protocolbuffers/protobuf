// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstddef>

#include "absl/strings/string_view.h"
#include "google/protobuf/rust/cpp_kernel/cpp_api.h"
#include "google/protobuf/unittest.pb.h"

extern "C" void MutateTestAllTypes(protobuf_unittest::TestAllTypes* msg) {
  msg->set_optional_int64(42);
  msg->set_optional_bytes("something mysterious");
  msg->set_optional_bool(false);
}

extern "C" google::protobuf::rust_internal::SerializedData SerializeTestAllTypes(
    const protobuf_unittest::TestAllTypes* msg) {
  return google::protobuf::rust_internal::SerializeMsg(msg);
}

extern "C" void* DeserializeTestAllTypes(const void* data, size_t size) {
  auto* proto = new protobuf_unittest::TestAllTypes;
  proto->ParseFromArray(data, size);
  return proto;
}

extern "C" void* NewWithExtension() {
  auto* proto = new protobuf_unittest::TestAllExtensions;
  proto->SetExtension(protobuf_unittest::optional_bytes_extension, "smuggled");
  return proto;
}

extern "C" google::protobuf::rust_internal::PtrAndLen GetBytesExtension(
    const protobuf_unittest::TestAllExtensions* proto) {
  absl::string_view bytes =
      proto->GetExtension(protobuf_unittest::optional_bytes_extension);
  return {bytes.data(), bytes.size()};
}
