// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Fuzz test for signed integer overflow in ReadPackedFixed /
// ReadPackedVarintArrayWithField (parse_context.h).
// See: https://github.com/protocolbuffers/protobuf/pull/27611

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/descriptor.pb.h"
#include "testing/fuzzing/fuzztest.h"

namespace google {
namespace protobuf {
namespace {

// Build a dynamic message with packed repeated fields to exercise
// ReadPackedFixed and ReadPackedVarintArrayWithField.
const Message* BuildPrototype() {
  static DescriptorPool* pool = []() {
    auto* p = new DescriptorPool();
    FileDescriptorProto file;
    file.set_name("fuzz_packed.proto");
    file.set_syntax("proto2");
    auto* msg = file.add_message_type();
    msg->set_name("FuzzMsg");

    // packed int32 — ReadPackedVarintArrayWithField
    auto* f1 = msg->add_field();
    f1->set_name("packed_int32");
    f1->set_number(1);
    f1->set_type(FieldDescriptorProto::TYPE_INT32);
    f1->set_label(FieldDescriptorProto::LABEL_REPEATED);
    f1->mutable_options()->set_packed(true);

    // packed fixed32 — ReadPackedFixed<uint32_t>
    auto* f2 = msg->add_field();
    f2->set_name("packed_fixed32");
    f2->set_number(2);
    f2->set_type(FieldDescriptorProto::TYPE_FIXED32);
    f2->set_label(FieldDescriptorProto::LABEL_REPEATED);
    f2->mutable_options()->set_packed(true);

    // packed bool — ReadPackedVarintArrayWithField<bool>
    auto* f3 = msg->add_field();
    f3->set_name("packed_bool");
    f3->set_number(3);
    f3->set_type(FieldDescriptorProto::TYPE_BOOL);
    f3->set_label(FieldDescriptorProto::LABEL_REPEATED);
    f3->mutable_options()->set_packed(true);

    // packed fixed64 — ReadPackedFixed<uint64_t>
    auto* f4 = msg->add_field();
    f4->set_name("packed_fixed64");
    f4->set_number(4);
    f4->set_type(FieldDescriptorProto::TYPE_FIXED64);
    f4->set_label(FieldDescriptorProto::LABEL_REPEATED);
    f4->mutable_options()->set_packed(true);

    p->BuildFile(file);
    return p;
  }();

  static DynamicMessageFactory* factory = new DynamicMessageFactory(pool);
  const Descriptor* desc = pool->FindMessageTypeByName("FuzzMsg");
  return factory->GetPrototype(desc);
}

void ParsePackedField(std::string_view data) {
  const Message* prototype = BuildPrototype();
  if (!prototype) return;
  Message* msg = prototype->New();
  // Exercises ReadPackedFixed / ReadPackedVarintArrayWithField
  // with arbitrary fuzzer-supplied wire data.
  msg->ParseFromArray(data.data(), static_cast<int>(data.size()));
  delete msg;
}

FUZZ_TEST(PackedFieldOverflowFuzzTest, ParsePackedField);

}  // namespace
}  // namespace protobuf
}  // namespace google
