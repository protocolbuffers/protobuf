// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stdint.h>

#include <string>

#include <gtest/gtest.h>
#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.hpp"
#include "upb/message/message.h"
#include "upb/port/def.inc"
#include "upb/reflection/def.hpp"
#include "upb/reflection/descriptor_bootstrap.h"
#include "upb/test/custom_options.upb.h"
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

  upb_StringView data;
  uintptr_t iter = kUpb_Message_UnknownBegin;
  ASSERT_TRUE(upb_Message_NextUnknown(UPB_UPCAST(options), &data, &iter));
  EXPECT_EQ(std::string(data.data, data.size),
            // 7739037: 9
            "\xE8\xE9\xC2\x1D\011");
  EXPECT_FALSE(upb_Message_NextUnknown(UPB_UPCAST(options), &data, &iter));
}

TEST(EditionsTest, ImportOptionLinked) {
  // Test that linked option dependencies don't show up in unknown fields. This
  // also actually *uses* the linked options to guarantee linkage and make the
  // previous test pass.

  upb::Arena arena;
  upb::DefPool defpool;
  upb::MessageDefPtr md(upb_test_2023_EditionsMessage_getmsgdef(defpool.ptr()));
  const google_protobuf_MessageOptions* options = md.options();
  EXPECT_EQ(upb_message_opt(options), 87);
}

TEST(EditionsTest, ConstructProto) {
  // Doesn't do anything except construct the proto. This just verifies that
  // the generated code compiles successfully.
  upb::Arena arena;
  upb_test_2023_EditionsMessage_new(arena.ptr());
}
