// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_no_field_presence.pb.h"

namespace google {
namespace protobuf {
namespace {

using ::proto2_nofieldpresence_unittest::ExplicitForeignMessage;
using ::proto2_nofieldpresence_unittest::FOREIGN_BAZ;
using ::proto2_nofieldpresence_unittest::FOREIGN_FOO;
using ::proto2_nofieldpresence_unittest::ForeignMessage;
using ::proto2_nofieldpresence_unittest::TestAllTypes;
using ::testing::Eq;
using ::testing::Gt;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::StrEq;
using ::testing::UnorderedPointwise;

// Helper: checks that all fields have default (zero/empty) values.
void CheckDefaultValues(const TestAllTypes& m) {
  EXPECT_EQ(0, m.optional_int32());
  EXPECT_EQ(0, m.optional_int64());
  EXPECT_EQ(0, m.optional_uint32());
  EXPECT_EQ(0, m.optional_uint64());
  EXPECT_EQ(0, m.optional_sint32());
  EXPECT_EQ(0, m.optional_sint64());
  EXPECT_EQ(0, m.optional_fixed32());
  EXPECT_EQ(0, m.optional_fixed64());
  EXPECT_EQ(0, m.optional_sfixed32());
  EXPECT_EQ(0, m.optional_sfixed64());
  EXPECT_EQ(0, m.optional_float());
  EXPECT_EQ(0, m.optional_double());
  EXPECT_EQ(false, m.optional_bool());
  EXPECT_EQ(0, m.optional_string().size());
  EXPECT_EQ(0, m.optional_bytes().size());

  EXPECT_EQ(false, m.has_optional_nested_message());
  // accessor for message fields returns default instance when not present
  EXPECT_EQ(0, m.optional_nested_message().bb());
  EXPECT_EQ(false, m.has_optional_proto2_message());
  // Embedded proto2 messages still have proto2 semantics, e.g. non-zero default
  // values. Here the submessage is not present but its accessor returns the
  // default instance.
  EXPECT_EQ(41, m.optional_proto2_message().default_int32());
  EXPECT_EQ(false, m.has_optional_foreign_message());
  EXPECT_EQ(TestAllTypes::FOO, m.optional_nested_enum());
  EXPECT_EQ(FOREIGN_FOO, m.optional_foreign_enum());

  EXPECT_EQ(0, m.optional_string_piece().size());

  EXPECT_EQ(0, m.repeated_int32_size());
  EXPECT_EQ(0, m.repeated_int64_size());
  EXPECT_EQ(0, m.repeated_uint32_size());
  EXPECT_EQ(0, m.repeated_uint64_size());
  EXPECT_EQ(0, m.repeated_sint32_size());
  EXPECT_EQ(0, m.repeated_sint64_size());
  EXPECT_EQ(0, m.repeated_fixed32_size());
  EXPECT_EQ(0, m.repeated_fixed64_size());
  EXPECT_EQ(0, m.repeated_sfixed32_size());
  EXPECT_EQ(0, m.repeated_sfixed64_size());
  EXPECT_EQ(0, m.repeated_float_size());
  EXPECT_EQ(0, m.repeated_double_size());
  EXPECT_EQ(0, m.repeated_bool_size());
  EXPECT_EQ(0, m.repeated_string_size());
  EXPECT_EQ(0, m.repeated_bytes_size());
  EXPECT_EQ(0, m.repeated_nested_message_size());
  EXPECT_EQ(0, m.repeated_foreign_message_size());
  EXPECT_EQ(0, m.repeated_proto2_message_size());
  EXPECT_EQ(0, m.repeated_nested_enum_size());
  EXPECT_EQ(0, m.repeated_foreign_enum_size());
  EXPECT_EQ(0, m.repeated_string_piece_size());
  EXPECT_EQ(0, m.repeated_lazy_message_size());
  EXPECT_EQ(TestAllTypes::ONEOF_FIELD_NOT_SET, m.oneof_field_case());
}

void FillValues(TestAllTypes* m) {
  m->set_optional_int32(100);
  m->set_optional_int64(101);
  m->set_optional_uint32(102);
  m->set_optional_uint64(103);
  m->set_optional_sint32(104);
  m->set_optional_sint64(105);
  m->set_optional_fixed32(106);
  m->set_optional_fixed64(107);
  m->set_optional_sfixed32(108);
  m->set_optional_sfixed64(109);
  m->set_optional_float(110.0);
  m->set_optional_double(111.0);
  m->set_optional_bool(true);
  m->set_optional_string("asdf");
  m->set_optional_bytes("jkl;");
  m->mutable_optional_nested_message()->set_bb(42);
  m->mutable_optional_foreign_message()->set_c(43);
  m->mutable_optional_proto2_message()->set_optional_int32(44);
  m->set_optional_nested_enum(TestAllTypes::BAZ);
  m->set_optional_foreign_enum(FOREIGN_BAZ);
  m->set_optional_string_piece("test");
  m->mutable_optional_lazy_message()->set_bb(45);
  m->add_repeated_int32(100);
  m->add_repeated_int64(101);
  m->add_repeated_uint32(102);
  m->add_repeated_uint64(103);
  m->add_repeated_sint32(104);
  m->add_repeated_sint64(105);
  m->add_repeated_fixed32(106);
  m->add_repeated_fixed64(107);
  m->add_repeated_sfixed32(108);
  m->add_repeated_sfixed64(109);
  m->add_repeated_float(110.0);
  m->add_repeated_double(111.0);
  m->add_repeated_bool(true);
  m->add_repeated_string("asdf");
  m->add_repeated_bytes("jkl;");
  m->add_repeated_nested_message()->set_bb(46);
  m->add_repeated_foreign_message()->set_c(47);
  m->add_repeated_proto2_message()->set_optional_int32(48);
  m->add_repeated_nested_enum(TestAllTypes::BAZ);
  m->add_repeated_foreign_enum(FOREIGN_BAZ);
  m->add_repeated_string_piece("test");
  m->add_repeated_lazy_message()->set_bb(49);

  m->set_oneof_uint32(1);
  m->mutable_oneof_nested_message()->set_bb(50);
  m->set_oneof_string("test");  // only this one remains set
}

void CheckNonDefaultValues(const TestAllTypes& m) {
  EXPECT_EQ(100, m.optional_int32());
  EXPECT_EQ(101, m.optional_int64());
  EXPECT_EQ(102, m.optional_uint32());
  EXPECT_EQ(103, m.optional_uint64());
  EXPECT_EQ(104, m.optional_sint32());
  EXPECT_EQ(105, m.optional_sint64());
  EXPECT_EQ(106, m.optional_fixed32());
  EXPECT_EQ(107, m.optional_fixed64());
  EXPECT_EQ(108, m.optional_sfixed32());
  EXPECT_EQ(109, m.optional_sfixed64());
  EXPECT_EQ(110.0, m.optional_float());
  EXPECT_EQ(111.0, m.optional_double());
  EXPECT_EQ(true, m.optional_bool());
  EXPECT_EQ("asdf", m.optional_string());
  EXPECT_EQ("jkl;", m.optional_bytes());
  EXPECT_EQ(true, m.has_optional_nested_message());
  EXPECT_EQ(42, m.optional_nested_message().bb());
  EXPECT_EQ(true, m.has_optional_foreign_message());
  EXPECT_EQ(43, m.optional_foreign_message().c());
  EXPECT_EQ(true, m.has_optional_proto2_message());
  EXPECT_EQ(44, m.optional_proto2_message().optional_int32());
  EXPECT_EQ(TestAllTypes::BAZ, m.optional_nested_enum());
  EXPECT_EQ(FOREIGN_BAZ, m.optional_foreign_enum());
  EXPECT_EQ("test", m.optional_string_piece());
  EXPECT_EQ(true, m.has_optional_lazy_message());
  EXPECT_EQ(45, m.optional_lazy_message().bb());

  EXPECT_EQ(1, m.repeated_int32_size());
  EXPECT_EQ(100, m.repeated_int32(0));
  EXPECT_EQ(1, m.repeated_int64_size());
  EXPECT_EQ(101, m.repeated_int64(0));
  EXPECT_EQ(1, m.repeated_uint32_size());
  EXPECT_EQ(102, m.repeated_uint32(0));
  EXPECT_EQ(1, m.repeated_uint64_size());
  EXPECT_EQ(103, m.repeated_uint64(0));
  EXPECT_EQ(1, m.repeated_sint32_size());
  EXPECT_EQ(104, m.repeated_sint32(0));
  EXPECT_EQ(1, m.repeated_sint64_size());
  EXPECT_EQ(105, m.repeated_sint64(0));
  EXPECT_EQ(1, m.repeated_fixed32_size());
  EXPECT_EQ(106, m.repeated_fixed32(0));
  EXPECT_EQ(1, m.repeated_fixed64_size());
  EXPECT_EQ(107, m.repeated_fixed64(0));
  EXPECT_EQ(1, m.repeated_sfixed32_size());
  EXPECT_EQ(108, m.repeated_sfixed32(0));
  EXPECT_EQ(1, m.repeated_sfixed64_size());
  EXPECT_EQ(109, m.repeated_sfixed64(0));
  EXPECT_EQ(1, m.repeated_float_size());
  EXPECT_EQ(110.0, m.repeated_float(0));
  EXPECT_EQ(1, m.repeated_double_size());
  EXPECT_EQ(111.0, m.repeated_double(0));
  EXPECT_EQ(1, m.repeated_bool_size());
  EXPECT_EQ(true, m.repeated_bool(0));
  EXPECT_EQ(1, m.repeated_string_size());
  EXPECT_EQ("asdf", m.repeated_string(0));
  EXPECT_EQ(1, m.repeated_bytes_size());
  EXPECT_EQ("jkl;", m.repeated_bytes(0));
  EXPECT_EQ(1, m.repeated_nested_message_size());
  EXPECT_EQ(46, m.repeated_nested_message(0).bb());
  EXPECT_EQ(1, m.repeated_foreign_message_size());
  EXPECT_EQ(47, m.repeated_foreign_message(0).c());
  EXPECT_EQ(1, m.repeated_proto2_message_size());
  EXPECT_EQ(48, m.repeated_proto2_message(0).optional_int32());
  EXPECT_EQ(1, m.repeated_nested_enum_size());
  EXPECT_EQ(TestAllTypes::BAZ, m.repeated_nested_enum(0));
  EXPECT_EQ(1, m.repeated_foreign_enum_size());
  EXPECT_EQ(FOREIGN_BAZ, m.repeated_foreign_enum(0));
  EXPECT_EQ(1, m.repeated_string_piece_size());
  EXPECT_EQ("test", m.repeated_string_piece(0));
  EXPECT_EQ(1, m.repeated_lazy_message_size());
  EXPECT_EQ(49, m.repeated_lazy_message(0).bb());

  EXPECT_EQ(TestAllTypes::kOneofString, m.oneof_field_case());
  EXPECT_EQ("test", m.oneof_string());
}

TEST(NoFieldPresenceTest, BasicMessageTest) {
  TestAllTypes message;
  // Check default values, fill all fields, check values. We just want to
  // exercise the basic getters/setter paths here to make sure no
  // field-presence-related changes broke these.
  CheckDefaultValues(message);
  FillValues(&message);
  CheckNonDefaultValues(message);

  // Clear() should be equivalent to getting a freshly-constructed message.
  message.Clear();
  CheckDefaultValues(message);
}

TEST(NoFieldPresenceTest, MessageFieldPresenceTest) {
  // check that presence still works properly for message fields.
  TestAllTypes message;
  EXPECT_EQ(false, message.has_optional_nested_message());
  // Getter should fetch default instance, and not cause the field to become
  // present.
  EXPECT_EQ(0, message.optional_nested_message().bb());
  EXPECT_EQ(false, message.has_optional_nested_message());
  message.mutable_optional_nested_message()->set_bb(42);
  EXPECT_EQ(true, message.has_optional_nested_message());
  message.clear_optional_nested_message();
  EXPECT_EQ(false, message.has_optional_nested_message());

  // Likewise for a lazy message field.
  EXPECT_EQ(false, message.has_optional_lazy_message());
  // Getter should fetch default instance, and not cause the field to become
  // present.
  EXPECT_EQ(0, message.optional_lazy_message().bb());
  EXPECT_EQ(false, message.has_optional_lazy_message());
  message.mutable_optional_lazy_message()->set_bb(42);
  EXPECT_EQ(true, message.has_optional_lazy_message());
  message.clear_optional_lazy_message();
  EXPECT_EQ(false, message.has_optional_lazy_message());

  // Test field presence of a message field on the default instance.
  EXPECT_EQ(false,
            TestAllTypes::default_instance().has_optional_nested_message());
}

TEST(NoFieldPresenceTest, MergeFromDefaultStringFieldTest) {
  // As an optimization, we maintain a default string in memory and messages
  // with uninitialized fields will be constructed with a pointer to this
  // default string object. The destructor should clear the field only when it
  // is "set" to a nondefault object.
  TestAllTypes src, dst;
  dst.MergeFrom(src);

  dst.Clear();
}

TEST(NoFieldPresenceTest, MergeFromAllocatedStringFieldTest) {
  // As an optimization, we maintain a default string in memory and messages
  // with uninitialized fields will be constructed with a pointer to this
  // default string object. The destructor should clear the field only when it
  // is "set" to a nondefault object.
  TestAllTypes src, dst;

  src.mutable_optional_string();  // this causes a memory allocation.
  dst.MergeFrom(src);

  dst.Clear();
}

TEST(NoFieldPresenceTest, MergeFromEmptyStringFieldTest) {
  // As an optimization, we maintain a default string in memory and messages
  // with uninitialized fields will be constructed with a pointer to this
  // default string object. The destructor should clear the field only when it
  // is "set" to a nondefault object.
  TestAllTypes src, dst;

  // set one field to zero.
  src.set_optional_string("");
  dst.MergeFrom(src);

  dst.Clear();
}

TEST(NoFieldPresenceTest, CopyTwiceDefaultStringFieldTest) {
  // As an optimization, we maintain a default string in memory and messages
  // with uninitialized fields will be constructed with a pointer to this
  // default string object. The destructor should clear the field only when it
  // is "set" to a nondefault object.
  TestAllTypes src, dst;

  dst = src;
  dst = src;
}

TEST(NoFieldPresenceTest, CopyTwiceAllocatedStringFieldTest) {
  // As an optimization, we maintain a default string in memory and messages
  // with uninitialized fields will be constructed with a pointer to this
  // default string object. The destructor should clear the field only when it
  // is "set" to a nondefault object.
  TestAllTypes src, dst;

  src.mutable_optional_string();  // this causes a memory allocation.

  dst = src;
  dst = src;
}

TEST(NoFieldPresenceTest, CopyTwiceEmptyStringFieldTest) {
  // As an optimization, we maintain a default string in memory and messages
  // with uninitialized fields will be constructed with a pointer to this
  // default string object. The destructor should clear the field only when it
  // is "set" to a nondefault object.
  TestAllTypes src, dst;

  // set one field to zero.
  src.set_optional_string("");

  dst = src;
  dst = src;
}

class NoFieldPresenceSwapFieldTest : public testing::Test {
 protected:
  NoFieldPresenceSwapFieldTest()
      : m1_(),
        m2_(),
        r1_(m1_.GetReflection()),
        r2_(m2_.GetReflection()),
        d1_(m1_.GetDescriptor()),
        d2_(m2_.GetDescriptor()) {}

  // Returns a field descriptor that corresponds to the field name.
  // Note that different messages would still return the same field descriptor.
  const FieldDescriptor* FindFieldByName(absl::string_view field_name) {
    const FieldDescriptor* f1 = d1_->FindFieldByName(field_name);
    const FieldDescriptor* f2 = d2_->FindFieldByName(field_name);

    // We actually ensure uniqueness of *field descriptors* even if we try to
    // obtain them from different *message descriptors*.
    ABSL_CHECK_EQ(f1, f2);
    return f1;
  }

  TestAllTypes m1_;
  TestAllTypes m2_;
  const Reflection* r1_;
  const Reflection* r2_;
  const Descriptor* d1_;
  const Descriptor* d2_;
};

TEST_F(NoFieldPresenceSwapFieldTest, ReflectionSwapFieldScalarNonZeroTest) {
  m1_.set_optional_int32(1);
  m2_.set_optional_int32(2);

  const FieldDescriptor* f = FindFieldByName("optional_int32");
  r1_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped.
  EXPECT_TRUE(r1_->HasField(m1_, f));
  EXPECT_TRUE(r2_->HasField(m2_, f));
  EXPECT_EQ(2, m1_.optional_int32());
  EXPECT_EQ(1, m2_.optional_int32());

  // It doesn't matter which reflection or descriptor gets used; swapping should
  // still work if m2_'s descriptor is provided.
  r2_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped again.
  EXPECT_TRUE(r1_->HasField(m1_, f));
  EXPECT_TRUE(r2_->HasField(m2_, f));
  EXPECT_EQ(1, m1_.optional_int32());
  EXPECT_EQ(2, m2_.optional_int32());
}

TEST_F(NoFieldPresenceSwapFieldTest, ReflectionSwapFieldScalarOneZeroTest) {
  m1_.set_optional_int32(1);

  const FieldDescriptor* f = FindFieldByName("optional_int32");
  r1_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped.
  EXPECT_FALSE(r1_->HasField(m1_, f));
  EXPECT_TRUE(r2_->HasField(m2_, f));
  EXPECT_EQ(0, m1_.optional_int32());
  EXPECT_EQ(1, m2_.optional_int32());

  // It doesn't matter which reflection or descriptor gets used; swapping should
  // still work if m2_'s descriptor is provided.
  r2_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped again.
  EXPECT_TRUE(r1_->HasField(m1_, f));
  EXPECT_FALSE(r2_->HasField(m2_, f));
  EXPECT_EQ(1, m1_.optional_int32());
  EXPECT_EQ(0, m2_.optional_int32());
}

TEST_F(NoFieldPresenceSwapFieldTest, ReflectionSwapFieldScalarBothZeroTest) {
  m1_.set_optional_int32(0);  // setting an int field to zero should be noop

  const FieldDescriptor* f = FindFieldByName("optional_int32");
  r1_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped.
  EXPECT_FALSE(r1_->HasField(m1_, f));
  EXPECT_FALSE(r2_->HasField(m2_, f));
  EXPECT_EQ(0, m1_.optional_int32());
  EXPECT_EQ(0, m2_.optional_int32());

  // It doesn't matter which reflection or descriptor gets used; swapping should
  // still work if m2_'s descriptor is provided.
  r2_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped again.
  EXPECT_FALSE(r1_->HasField(m1_, f));
  EXPECT_FALSE(r2_->HasField(m2_, f));
  EXPECT_EQ(0, m1_.optional_int32());
  EXPECT_EQ(0, m2_.optional_int32());
}

TEST_F(NoFieldPresenceSwapFieldTest, ReflectionSwapFieldRepeatedNonZeroTest) {
  m1_.add_repeated_int32(1);
  m2_.add_repeated_int32(2);
  m2_.add_repeated_int32(22);

  const FieldDescriptor* f = FindFieldByName("repeated_int32");
  r1_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped.
  EXPECT_EQ(r1_->FieldSize(m1_, f), 2);
  EXPECT_EQ(r2_->FieldSize(m2_, f), 1);
  EXPECT_THAT(m1_.repeated_int32(), UnorderedPointwise(Eq(), {2, 22}));
  EXPECT_THAT(m2_.repeated_int32(), UnorderedPointwise(Eq(), {1}));

  // It doesn't matter which reflection or descriptor gets used; swapping should
  // still work if m2_'s descriptor is provided.
  r2_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped again.
  EXPECT_EQ(r1_->FieldSize(m1_, f), 1);
  EXPECT_EQ(r2_->FieldSize(m2_, f), 2);
  EXPECT_THAT(m1_.repeated_int32(), UnorderedPointwise(Eq(), {1}));
  EXPECT_THAT(m2_.repeated_int32(), UnorderedPointwise(Eq(), {2, 22}));
}

TEST_F(NoFieldPresenceSwapFieldTest, ReflectionSwapFieldRepeatedOneZeroTest) {
  m1_.add_repeated_int32(1);

  const FieldDescriptor* f = FindFieldByName("repeated_int32");
  r1_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped.
  EXPECT_EQ(r1_->FieldSize(m1_, f), 0);
  EXPECT_EQ(r2_->FieldSize(m2_, f), 1);
  EXPECT_THAT(m1_.repeated_int32(), IsEmpty());
  EXPECT_THAT(m2_.repeated_int32(), UnorderedPointwise(Eq(), {1}));

  // It doesn't matter which reflection or descriptor gets used; swapping should
  // still work if m2_'s descriptor is provided.
  r2_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped again.
  EXPECT_EQ(r1_->FieldSize(m1_, f), 1);
  EXPECT_EQ(r2_->FieldSize(m2_, f), 0);
  EXPECT_THAT(m1_.repeated_int32(), UnorderedPointwise(Eq(), {1}));
  EXPECT_THAT(m2_.repeated_int32(), IsEmpty());
}

TEST_F(NoFieldPresenceSwapFieldTest,
       ReflectionSwapFieldRepeatedExplicitZeroTest) {
  // For repeated fields, explicitly adding zero would cause it to be added into
  // the repeated field.
  m1_.add_repeated_int32(0);

  const FieldDescriptor* f = FindFieldByName("repeated_int32");
  r1_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped.
  EXPECT_EQ(r1_->FieldSize(m1_, f), 0);
  EXPECT_EQ(r2_->FieldSize(m2_, f), 1);
  EXPECT_THAT(m1_.repeated_int32(), IsEmpty());
  EXPECT_THAT(m2_.repeated_int32(), UnorderedPointwise(Eq(), {0}));

  // It doesn't matter which reflection or descriptor gets used; swapping should
  // still work if m2_'s descriptor is provided.
  r2_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped again.
  EXPECT_EQ(r1_->FieldSize(m1_, f), 1);
  EXPECT_EQ(r2_->FieldSize(m2_, f), 0);
  EXPECT_THAT(m1_.repeated_int32(), UnorderedPointwise(Eq(), {0}));
  EXPECT_THAT(m2_.repeated_int32(), IsEmpty());
}

TEST_F(NoFieldPresenceSwapFieldTest,
       ReflectionSwapFieldOneofFieldDescriptorTest) {
  m1_.set_oneof_uint32(1);
  m2_.set_oneof_string("test");

  // NOTE: Calling swap on any field descriptor within the oneof works --
  // even a completely unrelated field.
  const FieldDescriptor* never_set_field = d1_->FindFieldByName("oneof_enum");

  r1_->SwapFields(&m1_, &m2_, /*fields=*/{never_set_field});

  // Fields should be swapped.
  EXPECT_FALSE(r1_->HasField(m1_, never_set_field));
  EXPECT_FALSE(r1_->HasField(m2_, never_set_field));
  EXPECT_TRUE(m1_.has_oneof_string());
  EXPECT_TRUE(m2_.has_oneof_uint32());
  EXPECT_EQ(m1_.oneof_string(), "test");
  EXPECT_EQ(m2_.oneof_uint32(), 1);

  // Calling oneof accessors on a swapped-out field will give the default value.
  EXPECT_FALSE(m1_.has_oneof_uint32());
  EXPECT_FALSE(m2_.has_oneof_string());
  EXPECT_EQ(m1_.oneof_uint32(), 0);
  EXPECT_THAT(m2_.oneof_string(), IsEmpty());
}

TEST_F(NoFieldPresenceSwapFieldTest,
       ReflectionSwapFieldOneofFieldMultipleIdenticalDescriptorTest) {
  m1_.set_oneof_uint32(1);
  m2_.set_oneof_string("test");

  // NOTE: Calling swap on any field descriptor within the oneof works --
  // even a completely unrelated field.
  const FieldDescriptor* never_set_field = d1_->FindFieldByName("oneof_enum");
  const FieldDescriptor* f1 = d1_->FindFieldByName("oneof_uint32");
  const FieldDescriptor* f2 = d2_->FindFieldByName("oneof_string");

  // Multiple instances of the identical descriptor is ignored.
  r1_->SwapFields(&m1_, &m2_, /*fields=*/{never_set_field, never_set_field});

  // Fields should be swapped (just once).
  EXPECT_EQ(m1_.oneof_string(), "test");
  EXPECT_EQ(m2_.oneof_uint32(), 1);

  // Multiple instances of the identical descriptor is ignored.
  r2_->SwapFields(&m1_, &m2_, /*fields=*/{f1, f2, never_set_field});

  // Fields should be swapped (just once).
  EXPECT_TRUE(m1_.has_oneof_uint32());
  EXPECT_TRUE(m2_.has_oneof_string());
  EXPECT_TRUE(r1_->HasField(m1_, f1));
  EXPECT_TRUE(r2_->HasField(m2_, f2));
  EXPECT_EQ(m1_.oneof_uint32(), 1);
  EXPECT_EQ(m2_.oneof_string(), "test");

  // Calling oneof accessors on a swapped-out field will give the default value.
  EXPECT_FALSE(m1_.has_oneof_string());
  EXPECT_FALSE(m2_.has_oneof_uint32());
  EXPECT_FALSE(r1_->HasField(m1_, d1_->FindFieldByName("oneof_string")));
  EXPECT_FALSE(r2_->HasField(m2_, d2_->FindFieldByName("oneof_uint32")));
  EXPECT_THAT(m1_.oneof_string(), IsEmpty());
  EXPECT_EQ(m2_.oneof_uint32(), 0);
}

TEST_F(NoFieldPresenceSwapFieldTest, ReflectionSwapFieldOneofNonZeroTest) {
  m1_.set_oneof_uint32(1);
  m2_.set_oneof_string("test");

  const FieldDescriptor* f = FindFieldByName("oneof_uint32");
  r1_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped.
  EXPECT_TRUE(m1_.has_oneof_string());
  EXPECT_TRUE(m2_.has_oneof_uint32());
  EXPECT_TRUE(r1_->HasField(m1_, d1_->FindFieldByName("oneof_string")));
  EXPECT_TRUE(r2_->HasField(m2_, f));
  EXPECT_EQ(m1_.oneof_string(), "test");
  EXPECT_EQ(m2_.oneof_uint32(), 1);

  // It doesn't matter which reflection or descriptor gets used; swapping should
  // still work if m2_'s descriptor is provided.
  r2_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped.
  EXPECT_TRUE(m1_.has_oneof_uint32());
  EXPECT_TRUE(m2_.has_oneof_string());
  EXPECT_TRUE(r1_->HasField(m1_, f));
  EXPECT_TRUE(r2_->HasField(m2_, d2_->FindFieldByName("oneof_string")));
  EXPECT_EQ(m1_.oneof_uint32(), 1);
  EXPECT_EQ(m2_.oneof_string(), "test");
}

TEST_F(NoFieldPresenceSwapFieldTest, ReflectionSwapFieldOneofDefaultTest) {
  m1_.set_oneof_uint32(1);

  const FieldDescriptor* f = FindFieldByName("oneof_uint32");
  r1_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped.
  EXPECT_FALSE(r1_->HasField(m1_, d1_->FindFieldByName("oneof_string")));
  EXPECT_TRUE(r2_->HasField(m2_, f));
  EXPECT_FALSE(m1_.has_oneof_string());
  EXPECT_EQ(m2_.oneof_uint32(), 1);

  // It doesn't matter which reflection or descriptor gets used; swapping should
  // still work if m2_'s descriptor is provided.
  r2_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped.
  EXPECT_TRUE(r1_->HasField(m1_, f));
  EXPECT_FALSE(r2_->HasField(m2_, d2_->FindFieldByName("oneof_string")));
  EXPECT_EQ(m1_.oneof_uint32(), 1);
  EXPECT_FALSE(m2_.has_oneof_string());
}

TEST_F(NoFieldPresenceSwapFieldTest, ReflectionSwapFieldOneofExplicitZeroTest) {
  // Oneof fields essentially have explicit presence -- if set to zero, they
  // will still be considered present.
  m1_.set_oneof_uint32(0);

  const FieldDescriptor* f = FindFieldByName("oneof_uint32");
  r1_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped.
  EXPECT_FALSE(r1_->HasField(m1_, f));
  EXPECT_TRUE(r2_->HasField(m2_, f));
  EXPECT_FALSE(m1_.has_oneof_uint32());
  EXPECT_TRUE(m2_.has_oneof_uint32());
  EXPECT_EQ(m2_.oneof_uint32(), 0);

  // It doesn't matter which reflection or descriptor gets used; swapping should
  // still work if m2_'s descriptor is provided.
  r2_->SwapFields(&m1_, &m2_, /*fields=*/{f});

  // Fields should be swapped.
  EXPECT_TRUE(r1_->HasField(m1_, f));
  EXPECT_FALSE(r2_->HasField(m2_, f));
  EXPECT_TRUE(m1_.has_oneof_uint32());
  EXPECT_EQ(m1_.oneof_uint32(), 0);
  EXPECT_FALSE(m2_.has_oneof_uint32());
}

class NoFieldPresenceListFieldsTest : public testing::Test {
 protected:
  NoFieldPresenceListFieldsTest()
      : message_(), r_(message_.GetReflection()), fields_() {
    // Check initial state: scalars not present (due to need to be consistent
    // with MergeFrom()), message fields not present, oneofs not present.
    r_->ListFields(message_, &fields_);
    ABSL_CHECK(fields_.empty());
  }

  TestAllTypes message_;
  const Reflection* r_;
  std::vector<const FieldDescriptor*> fields_;
};

TEST_F(NoFieldPresenceListFieldsTest, ScalarTest) {
  // Check zero/empty-means-not-present semantics.
  message_.set_optional_int32(0);
  r_->ListFields(message_, &fields_);
  EXPECT_TRUE(fields_.empty());

  message_.Clear();
  message_.set_optional_int32(42);
  r_->ListFields(message_, &fields_);
  EXPECT_EQ(1, fields_.size());
}

TEST_F(NoFieldPresenceListFieldsTest, MessageTest) {
  // Message fields always have explicit presence.
  message_.mutable_optional_nested_message();
  r_->ListFields(message_, &fields_);
  EXPECT_EQ(1, fields_.size());

  fields_.clear();
  message_.Clear();
  message_.mutable_optional_nested_message()->set_bb(123);
  r_->ListFields(message_, &fields_);
  EXPECT_EQ(1, fields_.size());
}

TEST_F(NoFieldPresenceListFieldsTest, OneOfTest) {
  // Oneof fields behave essentially like an explicit presence field.
  message_.set_oneof_uint32(0);
  r_->ListFields(message_, &fields_);
  EXPECT_EQ(1, fields_.size());

  fields_.clear();
  // Note:
  // we don't clear message_ -- oneof must only maintain one present field.
  message_.set_oneof_uint32(42);
  r_->ListFields(message_, &fields_);
  EXPECT_EQ(1, fields_.size());
}

TEST(NoFieldPresenceTest, ReflectionHasFieldTest) {
  // check that HasField reports true on all scalar fields. Check that it
  // behaves properly for message fields.

  TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  // Check initial state: scalars not present (due to need to be consistent with
  // MergeFrom()), message fields not present, oneofs not present.
  for (int i = 0; i < desc->field_count(); i++) {
    const FieldDescriptor* field = desc->field(i);
    if (field->is_repeated()) continue;
    EXPECT_EQ(false, r->HasField(message, field));
  }

  // Test field presence of a message field on the default instance.
  const FieldDescriptor* msg_field =
      desc->FindFieldByName("optional_nested_message");
  EXPECT_EQ(false, r->HasField(TestAllTypes::default_instance(), msg_field));

  // Fill all fields, expect everything to report true (check oneofs below).
  FillValues(&message);
  for (int i = 0; i < desc->field_count(); i++) {
    const FieldDescriptor* field = desc->field(i);
    if (field->is_repeated() || field->containing_oneof()) {
      continue;
    }
    if (internal::cpp::IsStringFieldWithPrivatizedAccessors(*field)) {
      continue;
    }
    EXPECT_EQ(true, r->HasField(message, field));
  }

  message.Clear();

  // Check zero/empty-means-not-present semantics.
  const FieldDescriptor* field_int32 = desc->FindFieldByName("optional_int32");
  const FieldDescriptor* field_double =
      desc->FindFieldByName("optional_double");
  const FieldDescriptor* field_string =
      desc->FindFieldByName("optional_string");

  EXPECT_EQ(false, r->HasField(message, field_int32));
  EXPECT_EQ(false, r->HasField(message, field_double));
  EXPECT_EQ(false, r->HasField(message, field_string));

  message.set_optional_int32(42);
  EXPECT_EQ(true, r->HasField(message, field_int32));
  message.set_optional_int32(0);
  EXPECT_EQ(false, r->HasField(message, field_int32));

  message.set_optional_double(42.0);
  EXPECT_EQ(true, r->HasField(message, field_double));
  message.set_optional_double(0.0);
  EXPECT_EQ(false, r->HasField(message, field_double));

  message.set_optional_string("test");
  EXPECT_EQ(true, r->HasField(message, field_string));
  message.set_optional_string("");
  EXPECT_EQ(false, r->HasField(message, field_string));
}

TEST(NoFieldPresenceTest, ReflectionClearFieldTest) {
  TestAllTypes message;

  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_int32 = desc->FindFieldByName("optional_int32");
  const FieldDescriptor* field_double =
      desc->FindFieldByName("optional_double");
  const FieldDescriptor* field_string =
      desc->FindFieldByName("optional_string");
  const FieldDescriptor* field_message =
      desc->FindFieldByName("optional_nested_message");
  const FieldDescriptor* field_lazy =
      desc->FindFieldByName("optional_lazy_message");

  message.set_optional_int32(42);
  r->ClearField(&message, field_int32);
  EXPECT_EQ(0, message.optional_int32());

  message.set_optional_double(42.0);
  r->ClearField(&message, field_double);
  EXPECT_EQ(0.0, message.optional_double());

  message.set_optional_string("test");
  r->ClearField(&message, field_string);
  EXPECT_EQ("", message.optional_string());

  message.mutable_optional_nested_message()->set_bb(1234);
  r->ClearField(&message, field_message);
  EXPECT_FALSE(message.has_optional_nested_message());
  EXPECT_EQ(0, message.optional_nested_message().bb());

  message.mutable_optional_lazy_message()->set_bb(42);
  r->ClearField(&message, field_lazy);
  EXPECT_FALSE(message.has_optional_lazy_message());
  EXPECT_EQ(0, message.optional_lazy_message().bb());
}

TEST(NoFieldPresenceTest, HasFieldOneofsTest) {
  // check that HasField behaves properly for oneofs.
  TestAllTypes message;

  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();
  const FieldDescriptor* desc_oneof_uint32 =
      desc->FindFieldByName("oneof_uint32");
  const FieldDescriptor* desc_oneof_nested_message =
      desc->FindFieldByName("oneof_nested_message");
  const FieldDescriptor* desc_oneof_string =
      desc->FindFieldByName("oneof_string");
  ABSL_CHECK(desc_oneof_uint32 != nullptr);
  ABSL_CHECK(desc_oneof_nested_message != nullptr);
  ABSL_CHECK(desc_oneof_string != nullptr);

  EXPECT_EQ(false, r->HasField(message, desc_oneof_uint32));
  EXPECT_EQ(false, r->HasField(message, desc_oneof_nested_message));
  EXPECT_EQ(false, r->HasField(message, desc_oneof_string));

  message.set_oneof_string("test");
  EXPECT_EQ(false, r->HasField(message, desc_oneof_uint32));
  EXPECT_EQ(false, r->HasField(message, desc_oneof_nested_message));
  EXPECT_EQ(true, r->HasField(message, desc_oneof_string));
  message.mutable_oneof_nested_message()->set_bb(42);
  EXPECT_EQ(false, r->HasField(message, desc_oneof_uint32));
  EXPECT_EQ(true, r->HasField(message, desc_oneof_nested_message));
  EXPECT_EQ(false, r->HasField(message, desc_oneof_string));

  message.Clear();
  EXPECT_EQ(false, r->HasField(message, desc_oneof_uint32));
  EXPECT_EQ(false, r->HasField(message, desc_oneof_nested_message));
  EXPECT_EQ(false, r->HasField(message, desc_oneof_string));
}

TEST(NoFieldPresenceTest, MergeFromIfNonzeroTest) {
  // check that MergeFrom copies if nonzero/nondefault only.
  TestAllTypes source, dest;

  dest.set_optional_int32(42);
  dest.set_optional_string("test");
  source.set_optional_int32(0);
  source.set_optional_string("");
  // MergeFrom() copies only if present in serialization, i.e., non-zero.
  dest.MergeFrom(source);
  EXPECT_EQ(42, dest.optional_int32());
  EXPECT_EQ("test", dest.optional_string());

  source.set_optional_int32(84);
  source.set_optional_string("test2");
  dest.MergeFrom(source);
  EXPECT_EQ(84, dest.optional_int32());
  EXPECT_EQ("test2", dest.optional_string());
}

TEST(NoFieldPresenceTest, ParseEmptyStringFromWire) {
  ASSERT_EQ(TestAllTypes::GetDescriptor()->FindFieldByNumber(15)->name(),
            "optional_bytes");

  // Input wire tag: 0172 (octal) which is 01 111 010
  //   Field number 15 with wire type LEN
  // Explicitly specify LEN to be zero, then it's basically an empty string
  //   encoded on the wire.
  absl::string_view wire("\172\x00",  // 3:LEN 0
                         2);

  TestAllTypes message;
  message.MergeFromString(wire);

  // Implicit-presence fields don't have hazzers, so we can only verify that the
  // empty bytes field is not overwritten.
  EXPECT_THAT(message.optional_bytes(), IsEmpty());

  std::string output_data;
  EXPECT_TRUE(message.SerializeToString(&output_data));
  EXPECT_THAT(output_data, IsEmpty());
}

TEST(MessageTest, ParseEmptyStringFromWireOverwritesExistingField) {
  TestAllTypes message;
  ASSERT_EQ(TestAllTypes::GetDescriptor()->FindFieldByNumber(15)->name(),
            "optional_bytes");
  message.set_optional_bytes("hello");

  // Input wire tag: 0172 (octal) which is 01 111 010
  //   Field number 15 with wire type LEN
  // Explicitly specify LEN to be zero, then it's basically an empty string
  //   encoded on the wire.
  absl::string_view wire("\172\x00",  // 3:LEN 0
                         2);
  message.MergeFromString(wire);

  // Implicit-presence fields don't have hazzers, so we can only verify that the
  // empty bytes field is overwritten.
  EXPECT_THAT(message.optional_bytes(), IsEmpty());

  // Since string field is overwritten to be empty, this message will not
  // serialize.
  std::string output_data;
  EXPECT_TRUE(message.SerializeToString(&output_data));
  EXPECT_THAT(output_data, IsEmpty());
}

TEST(MessageTest, MergeEmptyMessageFromWire) {
  // Input wire tag: 9A 01 (hex) which is 10011010 00000001
  //   Field number 19 with wire type LEN
  // Explicitly specify LEN to be zero, then it's basically an empty message
  //   encoded on the wire.
  absl::string_view wire("\x9A\x01\x00", 3);

  TestAllTypes message;
  ASSERT_EQ(TestAllTypes::GetDescriptor()->FindFieldByNumber(19)->name(),
            "optional_foreign_message");
  message.MergeFromString(wire);

  // Message fields always have explicit presence, so serializing the message
  // will write the original bytes back out onto the wire.
  std::string output_data;
  EXPECT_TRUE(message.SerializeToString(&output_data));
  EXPECT_EQ(output_data, wire);
}

TEST(MessageTest, MergeEmptyMessageFromWireDoesNotOverwiteExisting) {
  // Input wire tag: 9A 01 (hex) which is 10011010 00000001
  //   Field number 19 with wire type LEN
  // Explicitly specify LEN to be zero, then it's basically an empty message
  //   encoded on the wire.
  absl::string_view wire("\x9A\x01\x00", 3);

  TestAllTypes message;
  ASSERT_EQ(TestAllTypes::GetDescriptor()->FindFieldByNumber(19)->name(),
            "optional_foreign_message");

  message.mutable_optional_foreign_message()->set_c(12);
  std::string original_output_data;
  EXPECT_TRUE(message.SerializeToString(&original_output_data));

  message.MergeFromString(wire);
  EXPECT_TRUE(message.has_optional_foreign_message());
  EXPECT_EQ(message.optional_foreign_message().c(), 12);

  std::string output_data;
  EXPECT_TRUE(message.SerializeToString(&output_data));
  EXPECT_NE(output_data, wire);
  EXPECT_EQ(output_data, original_output_data);
}

TEST(NoFieldPresenceTest, ExtraZeroesInWireParseTest) {
  // check extra serialized zeroes on the wire are parsed into the object.
  ForeignMessage dest;
  dest.set_c(42);
  ASSERT_EQ(42, dest.c());

  // ExplicitForeignMessage has the same fields as ForeignMessage, but with
  // explicit presence instead of implicit presence.
  ExplicitForeignMessage source;
  source.set_c(0);
  std::string wire = source.SerializeAsString();
  ASSERT_THAT(wire, StrEq(absl::string_view{"\x08\x00", 2}));

  // The "parse" operation clears all fields before merging from wire.
  ASSERT_TRUE(dest.ParseFromString(wire));
  EXPECT_EQ(0, dest.c());
  std::string dest_data;
  EXPECT_TRUE(dest.SerializeToString(&dest_data));
  EXPECT_TRUE(dest_data.empty());
}

TEST(NoFieldPresenceTest, ExtraZeroesInWireMergeTest) {
  // check explicit zeros on the wire are merged into an implicit one.
  ForeignMessage dest;
  dest.set_c(42);
  ASSERT_EQ(42, dest.c());

  // ExplicitForeignMessage has the same fields as ForeignMessage, but with
  // explicit presence instead of implicit presence.
  ExplicitForeignMessage source;
  source.set_c(0);
  std::string wire = source.SerializeAsString();
  ASSERT_THAT(wire, StrEq(absl::string_view{"\x08\x00", 2}));

  // TODO: b/356132170 -- Add conformance tests to ensure this behaviour is
  //                      well-defined.
  // As implemented, the C++ "merge" operation does not distinguish between
  // implicit and explicit fields when reading from the wire.
  ASSERT_TRUE(dest.MergeFromString(wire));
  // If zero is present on the wire, the original value is overwritten, even
  // though this is specified as an "implicit presence" field.
  EXPECT_EQ(0, dest.c());
  std::string dest_data;
  EXPECT_TRUE(dest.SerializeToString(&dest_data));
  EXPECT_TRUE(dest_data.empty());
}

TEST(NoFieldPresenceTest, ExtraZeroesInWireLastWins) {
  // check that, when the same field is present multiple times on the wire, we
  // always take the last one -- even if it is a zero.

  absl::string_view wire{"\x08\x01\x08\x00", /*len=*/4};  // note the null-byte.
  ForeignMessage dest;

  // TODO: b/356132170 -- Add conformance tests to ensure this behaviour is
  //                      well-defined.
  // As implemented, the C++ "merge" operation does not distinguish between
  // implicit and explicit fields when reading from the wire.
  ASSERT_TRUE(dest.MergeFromString(wire));
  // If the same field is present multiple times on the wire, "last one wins".
  // i.e. -- the last seen field content will always overwrite, even if it's
  // zero and the field is implicit presence.
  EXPECT_EQ(0, dest.c());
  std::string dest_data;
  EXPECT_TRUE(dest.SerializeToString(&dest_data));
  EXPECT_TRUE(dest_data.empty());
}

TEST(NoFieldPresenceTest, IsInitializedTest) {
  // Check that IsInitialized works properly.
  proto2_nofieldpresence_unittest::TestProto2Required message;

  EXPECT_EQ(true, message.IsInitialized());
  message.mutable_proto2()->set_a(1);
  EXPECT_EQ(false, message.IsInitialized());
  message.mutable_proto2()->set_b(1);
  EXPECT_EQ(false, message.IsInitialized());
  message.mutable_proto2()->set_c(1);
  EXPECT_EQ(true, message.IsInitialized());
}

// TODO: b/358616816 - `if constexpr` can be used here once C++17 is baseline.
template <typename T>
bool TestSerialize(const MessageLite& message, T* output);

template <>
bool TestSerialize<std::string>(const MessageLite& message,
                                std::string* output) {
  return message.SerializeToString(output);
}

template <>
bool TestSerialize<absl::Cord>(const MessageLite& message, absl::Cord* output) {
  return message.SerializeToString(output);
}

template <typename T>
class NoFieldPresenceSerializeTest : public testing::Test {
 public:
  T& GetOutputSinkRef() { return value_; }
  std::string GetOutput() { return std::string{value_}; }

 protected:
  // Cargo-culted from:
  // https://google.github.io/googletest/reference/testing.html#TYPED_TEST_SUITE
  T value_;
};

using SerializableOutputTypes = ::testing::Types<std::string, absl::Cord>;

// https://google.github.io/googletest/reference/testing.html#TYPED_TEST_SUITE
// Providing the NameGenerator produces slightly more readable output in the
// test invocation summary (type names are displayed instead of numbers).
class NameGenerator {
 public:
  template <typename T>
  static std::string GetName(int) {
    if constexpr (std::is_same_v<T, std::string>) {
      return "string";
    } else if constexpr (std::is_same_v<T, absl::Cord>) {
      return "Cord";
    } else {
      static_assert(
          std::is_same_v<T, std::string> || std::is_same_v<T, absl::Cord>,
          "unsupported type");
    }
  }
};

TYPED_TEST_SUITE(NoFieldPresenceSerializeTest, SerializableOutputTypes,
                 NameGenerator);

TYPED_TEST(NoFieldPresenceSerializeTest, DontSerializeDefaultValuesTest) {
  // check that serialized data contains only non-zero numeric fields/non-empty
  // string/byte fields.
  TestAllTypes message;
  TypeParam& output_sink = this->GetOutputSinkRef();

  // All default values -> no output.
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_EQ(0, this->GetOutput().size());

  // Zero values -> still no output.
  message.set_optional_int32(0);
  message.set_optional_int64(0);
  message.set_optional_uint32(0);
  message.set_optional_uint64(0);
  message.set_optional_sint32(0);
  message.set_optional_sint64(0);
  message.set_optional_fixed32(0);
  message.set_optional_fixed64(0);
  message.set_optional_sfixed32(0);
  message.set_optional_sfixed64(0);
  message.set_optional_float(0);
  message.set_optional_double(0);
  message.set_optional_bool(false);
  message.set_optional_string("");
  message.set_optional_bytes("");
  message.set_optional_nested_enum(TestAllTypes::FOO);  // first enum entry
  message.set_optional_foreign_enum(FOREIGN_FOO);       // first enum entry
  message.set_optional_string_piece("");

  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_EQ(0, this->GetOutput().size());

  message.set_optional_int32(1);
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_EQ(2, this->GetOutput().size());
  EXPECT_EQ("\x08\x01", this->GetOutput());

  message.set_optional_int32(0);
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_EQ(0, this->GetOutput().size());
}

TYPED_TEST(NoFieldPresenceSerializeTest, NullMutableSerializesEmpty) {
  // Check that, if mutable_foo() was called, but fields were not modified,
  // nothing is serialized on the wire.
  TestAllTypes message;
  TypeParam& output_sink = this->GetOutputSinkRef();

  // All default values -> no output.
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_TRUE(this->GetOutput().empty());

  // No-op mutable calls -> no output.
  message.mutable_optional_string();
  message.mutable_optional_bytes();
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_TRUE(this->GetOutput().empty());

  // Assign to nonempty string -> some output.
  *message.mutable_optional_bytes() = "bar";
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_THAT(this->GetOutput().size(),
              Gt(3));  // 3-byte-long string + tag/value + len
}

TYPED_TEST(NoFieldPresenceSerializeTest, SetAllocatedAndReleaseTest) {
  // Check that setting an empty string via set_allocated_foo behaves properly;
  // Check that serializing after release_foo does not generate output for foo.
  TestAllTypes message;
  TypeParam& output_sink = this->GetOutputSinkRef();

  // All default values -> no output.
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_TRUE(this->GetOutput().empty());

  auto allocated_bytes = std::make_unique<std::string>("test");
  message.set_allocated_optional_bytes(allocated_bytes.release());
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_THAT(this->GetOutput().size(),
              Gt(4));  // 4-byte-long string + tag/value + len

  size_t former_output_size = this->GetOutput().size();

  auto allocated_string = std::make_unique<std::string>("");
  message.set_allocated_optional_string(allocated_string.release());
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  // empty string not serialized.
  EXPECT_EQ(former_output_size, this->GetOutput().size());

  auto bytes_ptr = absl::WrapUnique(message.release_optional_bytes());
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_TRUE(
      this->GetOutput().empty());  // released fields are not serialized.
}

TYPED_TEST(NoFieldPresenceSerializeTest, LazyMessageFieldHasBit) {
  // Check that has-bit interaction with lazy message works (has-bit before and
  // after lazy decode).
  TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();
  const FieldDescriptor* field = desc->FindFieldByName("optional_lazy_message");
  ABSL_CHECK(field != nullptr);

  EXPECT_EQ(false, message.has_optional_lazy_message());
  EXPECT_EQ(false, r->HasField(message, field));

  message.mutable_optional_lazy_message()->set_bb(42);
  EXPECT_EQ(true, message.has_optional_lazy_message());
  EXPECT_EQ(true, r->HasField(message, field));

  // Serialize and parse with a new message object so that lazy field on new
  // object is in unparsed state.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  TestAllTypes message2;
  message2.ParseFromString(this->GetOutput());

  EXPECT_EQ(true, message2.has_optional_lazy_message());
  EXPECT_EQ(true, r->HasField(message2, field));

  // Access field to force lazy parse.
  EXPECT_EQ(42, message.optional_lazy_message().bb());
  EXPECT_EQ(true, message2.has_optional_lazy_message());
  EXPECT_EQ(true, r->HasField(message2, field));
}

TYPED_TEST(NoFieldPresenceSerializeTest, OneofPresence) {
  TestAllTypes message;
  // oneof fields still have field presence -- ensure that this goes on the wire
  // even though its value is the empty string.
  message.set_oneof_string("");
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  // Tag: 113 --> tag is (113 << 3) | 2 (length delimited) = 906
  // varint: 0x8a 0x07
  // Length: 0x00
  EXPECT_EQ(3, this->GetOutput().size());
  EXPECT_EQ(static_cast<char>(0x8a), this->GetOutput().at(0));
  EXPECT_EQ(static_cast<char>(0x07), this->GetOutput().at(1));
  EXPECT_EQ(static_cast<char>(0x00), this->GetOutput().at(2));

  message.Clear();
  EXPECT_TRUE(message.ParseFromString(this->GetOutput()));
  EXPECT_EQ(TestAllTypes::kOneofString, message.oneof_field_case());

  // Also test int32 and enum fields.
  message.Clear();
  message.set_oneof_uint32(0);  // would not go on wire if ordinary field.
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_EQ(3, this->GetOutput().size());
  EXPECT_TRUE(message.ParseFromString(this->GetOutput()));
  EXPECT_EQ(TestAllTypes::kOneofUint32, message.oneof_field_case());

  message.Clear();
  message.set_oneof_enum(TestAllTypes::FOO);  // FOO is the default value.
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_EQ(3, this->GetOutput().size());
  EXPECT_TRUE(message.ParseFromString(this->GetOutput()));
  EXPECT_EQ(TestAllTypes::kOneofEnum, message.oneof_field_case());

  message.Clear();
  message.set_oneof_string("test");
  message.clear_oneof_string();
  EXPECT_EQ(0, message.ByteSizeLong());
}

}  // namespace
}  // namespace protobuf
}  // namespace google
