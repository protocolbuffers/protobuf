// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stdint.h>

#include <gtest/gtest.h>
#include "upb/base/descriptor_constants.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.hpp"
#include "upb/message/message.h"
#include "upb/port/def.inc"
#include "upb/reflection/def.hpp"
#include "upb/reflection/descriptor_bootstrap.h"
#include "upb/test/editions_test.upb.h"
#include "upb/test/editions_test.upbdefs.h"

TEST(EditionsTest, PlainField) {
  upb::DefPool defpool;
  upb::MessageDefPtr md(upb_test_2023_EditionsMessage_getmsgdef(defpool.ptr()));
  upb::FieldDefPtr f(md.FindFieldByName("plain_field"));
  EXPECT_TRUE(f.has_presence());
}

TEST(EditionsTest, ImplicitPresenceField) {
  upb::DefPool defpool;
  upb::MessageDefPtr md(upb_test_2023_EditionsMessage_getmsgdef(defpool.ptr()));
  upb::FieldDefPtr f(md.FindFieldByName("implicit_presence_field"));
  EXPECT_FALSE(f.has_presence());
}

TEST(EditionsTest, DelimitedField) {
  upb::DefPool defpool;
  upb::MessageDefPtr md(upb_test_2023_EditionsMessage_getmsgdef(defpool.ptr()));
  upb::FieldDefPtr f(md.FindFieldByName("delimited_field"));
  EXPECT_EQ(kUpb_FieldType_Group, f.type());
}

TEST(EditionsTest, RequiredField) {
  upb::DefPool defpool;
  upb::MessageDefPtr md(upb_test_2023_EditionsMessage_getmsgdef(defpool.ptr()));
  upb::FieldDefPtr f(md.FindFieldByName("required_field"));
  EXPECT_EQ(kUpb_Label_Required, f.label());
}

TEST(EditionsTest, ClosedEnum) {
  upb::DefPool defpool;
  upb::MessageDefPtr md(upb_test_2023_EditionsMessage_getmsgdef(defpool.ptr()));
  upb::FieldDefPtr f(md.FindFieldByName("enum_field"));
  ASSERT_TRUE(f.enum_subdef().is_closed());
}

TEST(EditionsTest, PackedField) {
  upb::DefPool defpool;
  upb::MessageDefPtr md(upb_test_2023_EditionsMessage_getmsgdef(defpool.ptr()));
  upb::FieldDefPtr f(md.FindFieldByName("unpacked_field"));
  ASSERT_FALSE(f.packed());
}

TEST(EditionsTest, ImportOptionUnlinked) {
  // Test that unlinked option dependencies show up in unknown fields.  These
  // are optional dependencies that may or may not be present in the binary.

  upb::Arena arena;
  upb::DefPool defpool;
  upb::MessageDefPtr md(upb_test_2023_EditionsMessage_getmsgdef(defpool.ptr()));
  const google_protobuf_MessageOptions* options = md.options();

  EXPECT_TRUE(upb_Message_HasUnknown(UPB_UPCAST(options)));
}

TEST(EditionsTest, ConstructProto) {
  // Doesn't do anything except construct the proto. This just verifies that
  // the generated code compiles successfully.
  upb::Arena arena;
  upb_test_2023_EditionsMessage_new(arena.ptr());
}
