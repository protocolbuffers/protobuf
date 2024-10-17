// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <memory>

#include <gtest/gtest.h>
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/unittest_drop_unknown_fields.pb.h"

using unittest_drop_unknown_fields::Foo;
using unittest_drop_unknown_fields::FooWithExtraFields;

namespace google {
namespace protobuf {

TEST(DropUnknownFieldsTest, GeneratedMessage) {
  FooWithExtraFields foo_with_extra_fields;
  foo_with_extra_fields.set_int32_value(1);
  foo_with_extra_fields.set_enum_value(FooWithExtraFields::MOO);
  foo_with_extra_fields.set_extra_int32_value(2);

  Foo foo;
  ASSERT_TRUE(foo.ParseFromString(foo_with_extra_fields.SerializeAsString()));
  EXPECT_EQ(1, foo.int32_value());
  EXPECT_EQ(static_cast<int>(FooWithExtraFields::MOO),
            static_cast<int>(foo.enum_value()));
  EXPECT_FALSE(foo.GetReflection()->GetUnknownFields(foo).empty());

  ASSERT_TRUE(foo_with_extra_fields.ParseFromString(foo.SerializeAsString()));
  EXPECT_EQ(1, foo_with_extra_fields.int32_value());
  EXPECT_EQ(FooWithExtraFields::MOO, foo_with_extra_fields.enum_value());
  // The "extra_int32_value" field should not be lost.
  EXPECT_EQ(2, foo_with_extra_fields.extra_int32_value());
}

TEST(DropUnknownFieldsTest, DynamicMessage) {
  FooWithExtraFields foo_with_extra_fields;
  foo_with_extra_fields.set_int32_value(1);
  foo_with_extra_fields.set_enum_value(FooWithExtraFields::MOO);
  foo_with_extra_fields.set_extra_int32_value(2);

  DynamicMessageFactory factory;
  std::unique_ptr<Message> foo(factory.GetPrototype(Foo::descriptor())->New());
  ASSERT_TRUE(foo->ParseFromString(foo_with_extra_fields.SerializeAsString()));
  EXPECT_FALSE(foo->GetReflection()->GetUnknownFields(*foo).empty());

  ASSERT_TRUE(foo_with_extra_fields.ParseFromString(foo->SerializeAsString()));
  EXPECT_EQ(1, foo_with_extra_fields.int32_value());
  EXPECT_EQ(FooWithExtraFields::MOO, foo_with_extra_fields.enum_value());
  // The "extra_int32_value" field should not be lost.
  EXPECT_EQ(2, foo_with_extra_fields.extra_int32_value());
}

}  // namespace protobuf
}  // namespace google
