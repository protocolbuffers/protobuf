// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/test_util.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {

void TestUtil::SetAllFields(unittest::TestAllTypes* message) {
  message->set_optional_int32   (101);
  message->set_optional_int64   (102);
  message->set_optional_uint32  (103);
  message->set_optional_uint64  (104);
  message->set_optional_sint32  (105);
  message->set_optional_sint64  (106);
  message->set_optional_fixed32 (107);
  message->set_optional_fixed64 (108);
  message->set_optional_sfixed32(109);
  message->set_optional_sfixed64(110);
  message->set_optional_float   (111);
  message->set_optional_double  (112);
  message->set_optional_bool    (true);
  message->set_optional_string  ("115");
  message->set_optional_bytes   ("116");

  message->mutable_optionalgroup           ()->set_a(117);
  message->mutable_optional_nested_message ()->set_bb(118);
  message->mutable_optional_foreign_message()->set_c(119);
  message->mutable_optional_import_message ()->set_d(120);

  message->set_optional_nested_enum (unittest::TestAllTypes::BAZ);
  message->set_optional_foreign_enum(unittest::FOREIGN_BAZ      );
  message->set_optional_import_enum (unittest_import::IMPORT_BAZ);

  // StringPiece and Cord fields are only accessible via reflection in the
  // open source release; see comments in compiler/cpp/string_field.cc.
  message->GetReflection()->SetString(
    message->GetDescriptor()->FindFieldByName("optional_string_piece"),
    "124");
  message->GetReflection()->SetString(
    message->GetDescriptor()->FindFieldByName("optional_cord"),
    "125");

  // -----------------------------------------------------------------

  message->add_repeated_int32   (201);
  message->add_repeated_int64   (202);
  message->add_repeated_uint32  (203);
  message->add_repeated_uint64  (204);
  message->add_repeated_sint32  (205);
  message->add_repeated_sint64  (206);
  message->add_repeated_fixed32 (207);
  message->add_repeated_fixed64 (208);
  message->add_repeated_sfixed32(209);
  message->add_repeated_sfixed64(210);
  message->add_repeated_float   (211);
  message->add_repeated_double  (212);
  message->add_repeated_bool    (true);
  message->add_repeated_string  ("215");
  message->add_repeated_bytes   ("216");

  message->add_repeatedgroup           ()->set_a(217);
  message->add_repeated_nested_message ()->set_bb(218);
  message->add_repeated_foreign_message()->set_c(219);
  message->add_repeated_import_message ()->set_d(220);

  message->add_repeated_nested_enum (unittest::TestAllTypes::BAR);
  message->add_repeated_foreign_enum(unittest::FOREIGN_BAR      );
  message->add_repeated_import_enum (unittest_import::IMPORT_BAR);

  message->GetReflection()->AddString(
    message->GetDescriptor()->FindFieldByName("repeated_string_piece"),
    "224");
  message->GetReflection()->AddString(
    message->GetDescriptor()->FindFieldByName("repeated_cord"),
    "225");

  // Add a second one of each field.
  message->add_repeated_int32   (301);
  message->add_repeated_int64   (302);
  message->add_repeated_uint32  (303);
  message->add_repeated_uint64  (304);
  message->add_repeated_sint32  (305);
  message->add_repeated_sint64  (306);
  message->add_repeated_fixed32 (307);
  message->add_repeated_fixed64 (308);
  message->add_repeated_sfixed32(309);
  message->add_repeated_sfixed64(310);
  message->add_repeated_float   (311);
  message->add_repeated_double  (312);
  message->add_repeated_bool    (false);
  message->add_repeated_string  ("315");
  message->add_repeated_bytes   ("316");

  message->add_repeatedgroup           ()->set_a(317);
  message->add_repeated_nested_message ()->set_bb(318);
  message->add_repeated_foreign_message()->set_c(319);
  message->add_repeated_import_message ()->set_d(320);

  message->add_repeated_nested_enum (unittest::TestAllTypes::BAZ);
  message->add_repeated_foreign_enum(unittest::FOREIGN_BAZ      );
  message->add_repeated_import_enum (unittest_import::IMPORT_BAZ);

  message->GetReflection()->AddString(
    message->GetDescriptor()->FindFieldByName("repeated_string_piece"),
    "324");
  message->GetReflection()->AddString(
    message->GetDescriptor()->FindFieldByName("repeated_cord"),
    "325");

  // -----------------------------------------------------------------

  message->set_default_int32   (401);
  message->set_default_int64   (402);
  message->set_default_uint32  (403);
  message->set_default_uint64  (404);
  message->set_default_sint32  (405);
  message->set_default_sint64  (406);
  message->set_default_fixed32 (407);
  message->set_default_fixed64 (408);
  message->set_default_sfixed32(409);
  message->set_default_sfixed64(410);
  message->set_default_float   (411);
  message->set_default_double  (412);
  message->set_default_bool    (false);
  message->set_default_string  ("415");
  message->set_default_bytes   ("416");

  message->set_default_nested_enum (unittest::TestAllTypes::FOO);
  message->set_default_foreign_enum(unittest::FOREIGN_FOO      );
  message->set_default_import_enum (unittest_import::IMPORT_FOO);

  message->GetReflection()->SetString(
    message->GetDescriptor()->FindFieldByName("default_string_piece"),
    "424");
  message->GetReflection()->SetString(
    message->GetDescriptor()->FindFieldByName("default_cord"),
    "425");
}

// -------------------------------------------------------------------

void TestUtil::ModifyRepeatedFields(unittest::TestAllTypes* message) {
  message->set_repeated_int32   (1, 501);
  message->set_repeated_int64   (1, 502);
  message->set_repeated_uint32  (1, 503);
  message->set_repeated_uint64  (1, 504);
  message->set_repeated_sint32  (1, 505);
  message->set_repeated_sint64  (1, 506);
  message->set_repeated_fixed32 (1, 507);
  message->set_repeated_fixed64 (1, 508);
  message->set_repeated_sfixed32(1, 509);
  message->set_repeated_sfixed64(1, 510);
  message->set_repeated_float   (1, 511);
  message->set_repeated_double  (1, 512);
  message->set_repeated_bool    (1, true);
  message->set_repeated_string  (1, "515");
  message->set_repeated_bytes   (1, "516");

  message->mutable_repeatedgroup           (1)->set_a(517);
  message->mutable_repeated_nested_message (1)->set_bb(518);
  message->mutable_repeated_foreign_message(1)->set_c(519);
  message->mutable_repeated_import_message (1)->set_d(520);

  message->set_repeated_nested_enum (1, unittest::TestAllTypes::FOO);
  message->set_repeated_foreign_enum(1, unittest::FOREIGN_FOO      );
  message->set_repeated_import_enum (1, unittest_import::IMPORT_FOO);

  message->GetReflection()->SetRepeatedString(
    message->GetDescriptor()->FindFieldByName("repeated_string_piece"),
    1, "424");
  message->GetReflection()->SetRepeatedString(
    message->GetDescriptor()->FindFieldByName("repeated_cord"),
    1, "425");
}

// -------------------------------------------------------------------

void TestUtil::ExpectAllFieldsSet(const unittest::TestAllTypes& message) {
  EXPECT_TRUE(message.has_optional_int32   ());
  EXPECT_TRUE(message.has_optional_int64   ());
  EXPECT_TRUE(message.has_optional_uint32  ());
  EXPECT_TRUE(message.has_optional_uint64  ());
  EXPECT_TRUE(message.has_optional_sint32  ());
  EXPECT_TRUE(message.has_optional_sint64  ());
  EXPECT_TRUE(message.has_optional_fixed32 ());
  EXPECT_TRUE(message.has_optional_fixed64 ());
  EXPECT_TRUE(message.has_optional_sfixed32());
  EXPECT_TRUE(message.has_optional_sfixed64());
  EXPECT_TRUE(message.has_optional_float   ());
  EXPECT_TRUE(message.has_optional_double  ());
  EXPECT_TRUE(message.has_optional_bool    ());
  EXPECT_TRUE(message.has_optional_string  ());
  EXPECT_TRUE(message.has_optional_bytes   ());

  EXPECT_TRUE(message.has_optionalgroup           ());
  EXPECT_TRUE(message.has_optional_nested_message ());
  EXPECT_TRUE(message.has_optional_foreign_message());
  EXPECT_TRUE(message.has_optional_import_message ());

  EXPECT_TRUE(message.optionalgroup           ().has_a());
  EXPECT_TRUE(message.optional_nested_message ().has_bb());
  EXPECT_TRUE(message.optional_foreign_message().has_c());
  EXPECT_TRUE(message.optional_import_message ().has_d());

  EXPECT_TRUE(message.has_optional_nested_enum ());
  EXPECT_TRUE(message.has_optional_foreign_enum());
  EXPECT_TRUE(message.has_optional_import_enum ());

  EXPECT_TRUE(message.has_optional_string_piece());
  EXPECT_TRUE(message.has_optional_cord());

  EXPECT_EQ(101  , message.optional_int32   ());
  EXPECT_EQ(102  , message.optional_int64   ());
  EXPECT_EQ(103  , message.optional_uint32  ());
  EXPECT_EQ(104  , message.optional_uint64  ());
  EXPECT_EQ(105  , message.optional_sint32  ());
  EXPECT_EQ(106  , message.optional_sint64  ());
  EXPECT_EQ(107  , message.optional_fixed32 ());
  EXPECT_EQ(108  , message.optional_fixed64 ());
  EXPECT_EQ(109  , message.optional_sfixed32());
  EXPECT_EQ(110  , message.optional_sfixed64());
  EXPECT_EQ(111  , message.optional_float   ());
  EXPECT_EQ(112  , message.optional_double  ());
  EXPECT_EQ(true , message.optional_bool    ());
  EXPECT_EQ("115", message.optional_string  ());
  EXPECT_EQ("116", message.optional_bytes   ());

  EXPECT_EQ(117, message.optionalgroup           ().a());
  EXPECT_EQ(118, message.optional_nested_message ().bb());
  EXPECT_EQ(119, message.optional_foreign_message().c());
  EXPECT_EQ(120, message.optional_import_message ().d());

  EXPECT_EQ(unittest::TestAllTypes::BAZ, message.optional_nested_enum ());
  EXPECT_EQ(unittest::FOREIGN_BAZ      , message.optional_foreign_enum());
  EXPECT_EQ(unittest_import::IMPORT_BAZ, message.optional_import_enum ());


  // -----------------------------------------------------------------

  ASSERT_EQ(2, message.repeated_int32_size   ());
  ASSERT_EQ(2, message.repeated_int64_size   ());
  ASSERT_EQ(2, message.repeated_uint32_size  ());
  ASSERT_EQ(2, message.repeated_uint64_size  ());
  ASSERT_EQ(2, message.repeated_sint32_size  ());
  ASSERT_EQ(2, message.repeated_sint64_size  ());
  ASSERT_EQ(2, message.repeated_fixed32_size ());
  ASSERT_EQ(2, message.repeated_fixed64_size ());
  ASSERT_EQ(2, message.repeated_sfixed32_size());
  ASSERT_EQ(2, message.repeated_sfixed64_size());
  ASSERT_EQ(2, message.repeated_float_size   ());
  ASSERT_EQ(2, message.repeated_double_size  ());
  ASSERT_EQ(2, message.repeated_bool_size    ());
  ASSERT_EQ(2, message.repeated_string_size  ());
  ASSERT_EQ(2, message.repeated_bytes_size   ());

  ASSERT_EQ(2, message.repeatedgroup_size           ());
  ASSERT_EQ(2, message.repeated_nested_message_size ());
  ASSERT_EQ(2, message.repeated_foreign_message_size());
  ASSERT_EQ(2, message.repeated_import_message_size ());
  ASSERT_EQ(2, message.repeated_nested_enum_size    ());
  ASSERT_EQ(2, message.repeated_foreign_enum_size   ());
  ASSERT_EQ(2, message.repeated_import_enum_size    ());

  ASSERT_EQ(2, message.repeated_string_piece_size());
  ASSERT_EQ(2, message.repeated_cord_size());

  EXPECT_EQ(201  , message.repeated_int32   (0));
  EXPECT_EQ(202  , message.repeated_int64   (0));
  EXPECT_EQ(203  , message.repeated_uint32  (0));
  EXPECT_EQ(204  , message.repeated_uint64  (0));
  EXPECT_EQ(205  , message.repeated_sint32  (0));
  EXPECT_EQ(206  , message.repeated_sint64  (0));
  EXPECT_EQ(207  , message.repeated_fixed32 (0));
  EXPECT_EQ(208  , message.repeated_fixed64 (0));
  EXPECT_EQ(209  , message.repeated_sfixed32(0));
  EXPECT_EQ(210  , message.repeated_sfixed64(0));
  EXPECT_EQ(211  , message.repeated_float   (0));
  EXPECT_EQ(212  , message.repeated_double  (0));
  EXPECT_EQ(true , message.repeated_bool    (0));
  EXPECT_EQ("215", message.repeated_string  (0));
  EXPECT_EQ("216", message.repeated_bytes   (0));

  EXPECT_EQ(217, message.repeatedgroup           (0).a());
  EXPECT_EQ(218, message.repeated_nested_message (0).bb());
  EXPECT_EQ(219, message.repeated_foreign_message(0).c());
  EXPECT_EQ(220, message.repeated_import_message (0).d());


  EXPECT_EQ(unittest::TestAllTypes::BAR, message.repeated_nested_enum (0));
  EXPECT_EQ(unittest::FOREIGN_BAR      , message.repeated_foreign_enum(0));
  EXPECT_EQ(unittest_import::IMPORT_BAR, message.repeated_import_enum (0));

  EXPECT_EQ(301  , message.repeated_int32   (1));
  EXPECT_EQ(302  , message.repeated_int64   (1));
  EXPECT_EQ(303  , message.repeated_uint32  (1));
  EXPECT_EQ(304  , message.repeated_uint64  (1));
  EXPECT_EQ(305  , message.repeated_sint32  (1));
  EXPECT_EQ(306  , message.repeated_sint64  (1));
  EXPECT_EQ(307  , message.repeated_fixed32 (1));
  EXPECT_EQ(308  , message.repeated_fixed64 (1));
  EXPECT_EQ(309  , message.repeated_sfixed32(1));
  EXPECT_EQ(310  , message.repeated_sfixed64(1));
  EXPECT_EQ(311  , message.repeated_float   (1));
  EXPECT_EQ(312  , message.repeated_double  (1));
  EXPECT_EQ(false, message.repeated_bool    (1));
  EXPECT_EQ("315", message.repeated_string  (1));
  EXPECT_EQ("316", message.repeated_bytes   (1));

  EXPECT_EQ(317, message.repeatedgroup           (1).a());
  EXPECT_EQ(318, message.repeated_nested_message (1).bb());
  EXPECT_EQ(319, message.repeated_foreign_message(1).c());
  EXPECT_EQ(320, message.repeated_import_message (1).d());

  EXPECT_EQ(unittest::TestAllTypes::BAZ, message.repeated_nested_enum (1));
  EXPECT_EQ(unittest::FOREIGN_BAZ      , message.repeated_foreign_enum(1));
  EXPECT_EQ(unittest_import::IMPORT_BAZ, message.repeated_import_enum (1));


  // -----------------------------------------------------------------

  EXPECT_TRUE(message.has_default_int32   ());
  EXPECT_TRUE(message.has_default_int64   ());
  EXPECT_TRUE(message.has_default_uint32  ());
  EXPECT_TRUE(message.has_default_uint64  ());
  EXPECT_TRUE(message.has_default_sint32  ());
  EXPECT_TRUE(message.has_default_sint64  ());
  EXPECT_TRUE(message.has_default_fixed32 ());
  EXPECT_TRUE(message.has_default_fixed64 ());
  EXPECT_TRUE(message.has_default_sfixed32());
  EXPECT_TRUE(message.has_default_sfixed64());
  EXPECT_TRUE(message.has_default_float   ());
  EXPECT_TRUE(message.has_default_double  ());
  EXPECT_TRUE(message.has_default_bool    ());
  EXPECT_TRUE(message.has_default_string  ());
  EXPECT_TRUE(message.has_default_bytes   ());

  EXPECT_TRUE(message.has_default_nested_enum ());
  EXPECT_TRUE(message.has_default_foreign_enum());
  EXPECT_TRUE(message.has_default_import_enum ());


  EXPECT_EQ(401  , message.default_int32   ());
  EXPECT_EQ(402  , message.default_int64   ());
  EXPECT_EQ(403  , message.default_uint32  ());
  EXPECT_EQ(404  , message.default_uint64  ());
  EXPECT_EQ(405  , message.default_sint32  ());
  EXPECT_EQ(406  , message.default_sint64  ());
  EXPECT_EQ(407  , message.default_fixed32 ());
  EXPECT_EQ(408  , message.default_fixed64 ());
  EXPECT_EQ(409  , message.default_sfixed32());
  EXPECT_EQ(410  , message.default_sfixed64());
  EXPECT_EQ(411  , message.default_float   ());
  EXPECT_EQ(412  , message.default_double  ());
  EXPECT_EQ(false, message.default_bool    ());
  EXPECT_EQ("415", message.default_string  ());
  EXPECT_EQ("416", message.default_bytes   ());

  EXPECT_EQ(unittest::TestAllTypes::FOO, message.default_nested_enum ());
  EXPECT_EQ(unittest::FOREIGN_FOO      , message.default_foreign_enum());
  EXPECT_EQ(unittest_import::IMPORT_FOO, message.default_import_enum ());

}

// -------------------------------------------------------------------

void TestUtil::ExpectClear(const unittest::TestAllTypes& message) {
  // has_blah() should initially be false for all optional fields.
  EXPECT_FALSE(message.has_optional_int32   ());
  EXPECT_FALSE(message.has_optional_int64   ());
  EXPECT_FALSE(message.has_optional_uint32  ());
  EXPECT_FALSE(message.has_optional_uint64  ());
  EXPECT_FALSE(message.has_optional_sint32  ());
  EXPECT_FALSE(message.has_optional_sint64  ());
  EXPECT_FALSE(message.has_optional_fixed32 ());
  EXPECT_FALSE(message.has_optional_fixed64 ());
  EXPECT_FALSE(message.has_optional_sfixed32());
  EXPECT_FALSE(message.has_optional_sfixed64());
  EXPECT_FALSE(message.has_optional_float   ());
  EXPECT_FALSE(message.has_optional_double  ());
  EXPECT_FALSE(message.has_optional_bool    ());
  EXPECT_FALSE(message.has_optional_string  ());
  EXPECT_FALSE(message.has_optional_bytes   ());

  EXPECT_FALSE(message.has_optionalgroup           ());
  EXPECT_FALSE(message.has_optional_nested_message ());
  EXPECT_FALSE(message.has_optional_foreign_message());
  EXPECT_FALSE(message.has_optional_import_message ());

  EXPECT_FALSE(message.has_optional_nested_enum ());
  EXPECT_FALSE(message.has_optional_foreign_enum());
  EXPECT_FALSE(message.has_optional_import_enum ());

  EXPECT_FALSE(message.has_optional_string_piece());
  EXPECT_FALSE(message.has_optional_cord());

  // Optional fields without defaults are set to zero or something like it.
  EXPECT_EQ(0    , message.optional_int32   ());
  EXPECT_EQ(0    , message.optional_int64   ());
  EXPECT_EQ(0    , message.optional_uint32  ());
  EXPECT_EQ(0    , message.optional_uint64  ());
  EXPECT_EQ(0    , message.optional_sint32  ());
  EXPECT_EQ(0    , message.optional_sint64  ());
  EXPECT_EQ(0    , message.optional_fixed32 ());
  EXPECT_EQ(0    , message.optional_fixed64 ());
  EXPECT_EQ(0    , message.optional_sfixed32());
  EXPECT_EQ(0    , message.optional_sfixed64());
  EXPECT_EQ(0    , message.optional_float   ());
  EXPECT_EQ(0    , message.optional_double  ());
  EXPECT_EQ(false, message.optional_bool    ());
  EXPECT_EQ(""   , message.optional_string  ());
  EXPECT_EQ(""   , message.optional_bytes   ());

  // Embedded messages should also be clear.
  EXPECT_FALSE(message.optionalgroup           ().has_a());
  EXPECT_FALSE(message.optional_nested_message ().has_bb());
  EXPECT_FALSE(message.optional_foreign_message().has_c());
  EXPECT_FALSE(message.optional_import_message ().has_d());

  EXPECT_EQ(0, message.optionalgroup           ().a());
  EXPECT_EQ(0, message.optional_nested_message ().bb());
  EXPECT_EQ(0, message.optional_foreign_message().c());
  EXPECT_EQ(0, message.optional_import_message ().d());

  // Enums without defaults are set to the first value in the enum.
  EXPECT_EQ(unittest::TestAllTypes::FOO, message.optional_nested_enum ());
  EXPECT_EQ(unittest::FOREIGN_FOO      , message.optional_foreign_enum());
  EXPECT_EQ(unittest_import::IMPORT_FOO, message.optional_import_enum ());


  // Repeated fields are empty.
  EXPECT_EQ(0, message.repeated_int32_size   ());
  EXPECT_EQ(0, message.repeated_int64_size   ());
  EXPECT_EQ(0, message.repeated_uint32_size  ());
  EXPECT_EQ(0, message.repeated_uint64_size  ());
  EXPECT_EQ(0, message.repeated_sint32_size  ());
  EXPECT_EQ(0, message.repeated_sint64_size  ());
  EXPECT_EQ(0, message.repeated_fixed32_size ());
  EXPECT_EQ(0, message.repeated_fixed64_size ());
  EXPECT_EQ(0, message.repeated_sfixed32_size());
  EXPECT_EQ(0, message.repeated_sfixed64_size());
  EXPECT_EQ(0, message.repeated_float_size   ());
  EXPECT_EQ(0, message.repeated_double_size  ());
  EXPECT_EQ(0, message.repeated_bool_size    ());
  EXPECT_EQ(0, message.repeated_string_size  ());
  EXPECT_EQ(0, message.repeated_bytes_size   ());

  EXPECT_EQ(0, message.repeatedgroup_size           ());
  EXPECT_EQ(0, message.repeated_nested_message_size ());
  EXPECT_EQ(0, message.repeated_foreign_message_size());
  EXPECT_EQ(0, message.repeated_import_message_size ());
  EXPECT_EQ(0, message.repeated_nested_enum_size    ());
  EXPECT_EQ(0, message.repeated_foreign_enum_size   ());
  EXPECT_EQ(0, message.repeated_import_enum_size    ());

  EXPECT_EQ(0, message.repeated_string_piece_size());
  EXPECT_EQ(0, message.repeated_cord_size());

  // has_blah() should also be false for all default fields.
  EXPECT_FALSE(message.has_default_int32   ());
  EXPECT_FALSE(message.has_default_int64   ());
  EXPECT_FALSE(message.has_default_uint32  ());
  EXPECT_FALSE(message.has_default_uint64  ());
  EXPECT_FALSE(message.has_default_sint32  ());
  EXPECT_FALSE(message.has_default_sint64  ());
  EXPECT_FALSE(message.has_default_fixed32 ());
  EXPECT_FALSE(message.has_default_fixed64 ());
  EXPECT_FALSE(message.has_default_sfixed32());
  EXPECT_FALSE(message.has_default_sfixed64());
  EXPECT_FALSE(message.has_default_float   ());
  EXPECT_FALSE(message.has_default_double  ());
  EXPECT_FALSE(message.has_default_bool    ());
  EXPECT_FALSE(message.has_default_string  ());
  EXPECT_FALSE(message.has_default_bytes   ());

  EXPECT_FALSE(message.has_default_nested_enum ());
  EXPECT_FALSE(message.has_default_foreign_enum());
  EXPECT_FALSE(message.has_default_import_enum ());


  // Fields with defaults have their default values (duh).
  EXPECT_EQ( 41    , message.default_int32   ());
  EXPECT_EQ( 42    , message.default_int64   ());
  EXPECT_EQ( 43    , message.default_uint32  ());
  EXPECT_EQ( 44    , message.default_uint64  ());
  EXPECT_EQ(-45    , message.default_sint32  ());
  EXPECT_EQ( 46    , message.default_sint64  ());
  EXPECT_EQ( 47    , message.default_fixed32 ());
  EXPECT_EQ( 48    , message.default_fixed64 ());
  EXPECT_EQ( 49    , message.default_sfixed32());
  EXPECT_EQ(-50    , message.default_sfixed64());
  EXPECT_EQ( 51.5  , message.default_float   ());
  EXPECT_EQ( 52e3  , message.default_double  ());
  EXPECT_EQ(true   , message.default_bool    ());
  EXPECT_EQ("hello", message.default_string  ());
  EXPECT_EQ("world", message.default_bytes   ());

  EXPECT_EQ(unittest::TestAllTypes::BAR, message.default_nested_enum ());
  EXPECT_EQ(unittest::FOREIGN_BAR      , message.default_foreign_enum());
  EXPECT_EQ(unittest_import::IMPORT_BAR, message.default_import_enum ());

}

// -------------------------------------------------------------------

void TestUtil::ExpectRepeatedFieldsModified(
    const unittest::TestAllTypes& message) {
  // ModifyRepeatedFields only sets the second repeated element of each
  // field.  In addition to verifying this, we also verify that the first
  // element and size were *not* modified.
  ASSERT_EQ(2, message.repeated_int32_size   ());
  ASSERT_EQ(2, message.repeated_int64_size   ());
  ASSERT_EQ(2, message.repeated_uint32_size  ());
  ASSERT_EQ(2, message.repeated_uint64_size  ());
  ASSERT_EQ(2, message.repeated_sint32_size  ());
  ASSERT_EQ(2, message.repeated_sint64_size  ());
  ASSERT_EQ(2, message.repeated_fixed32_size ());
  ASSERT_EQ(2, message.repeated_fixed64_size ());
  ASSERT_EQ(2, message.repeated_sfixed32_size());
  ASSERT_EQ(2, message.repeated_sfixed64_size());
  ASSERT_EQ(2, message.repeated_float_size   ());
  ASSERT_EQ(2, message.repeated_double_size  ());
  ASSERT_EQ(2, message.repeated_bool_size    ());
  ASSERT_EQ(2, message.repeated_string_size  ());
  ASSERT_EQ(2, message.repeated_bytes_size   ());

  ASSERT_EQ(2, message.repeatedgroup_size           ());
  ASSERT_EQ(2, message.repeated_nested_message_size ());
  ASSERT_EQ(2, message.repeated_foreign_message_size());
  ASSERT_EQ(2, message.repeated_import_message_size ());
  ASSERT_EQ(2, message.repeated_nested_enum_size    ());
  ASSERT_EQ(2, message.repeated_foreign_enum_size   ());
  ASSERT_EQ(2, message.repeated_import_enum_size    ());

  ASSERT_EQ(2, message.repeated_string_piece_size());
  ASSERT_EQ(2, message.repeated_cord_size());

  EXPECT_EQ(201  , message.repeated_int32   (0));
  EXPECT_EQ(202  , message.repeated_int64   (0));
  EXPECT_EQ(203  , message.repeated_uint32  (0));
  EXPECT_EQ(204  , message.repeated_uint64  (0));
  EXPECT_EQ(205  , message.repeated_sint32  (0));
  EXPECT_EQ(206  , message.repeated_sint64  (0));
  EXPECT_EQ(207  , message.repeated_fixed32 (0));
  EXPECT_EQ(208  , message.repeated_fixed64 (0));
  EXPECT_EQ(209  , message.repeated_sfixed32(0));
  EXPECT_EQ(210  , message.repeated_sfixed64(0));
  EXPECT_EQ(211  , message.repeated_float   (0));
  EXPECT_EQ(212  , message.repeated_double  (0));
  EXPECT_EQ(true , message.repeated_bool    (0));
  EXPECT_EQ("215", message.repeated_string  (0));
  EXPECT_EQ("216", message.repeated_bytes   (0));

  EXPECT_EQ(217, message.repeatedgroup           (0).a());
  EXPECT_EQ(218, message.repeated_nested_message (0).bb());
  EXPECT_EQ(219, message.repeated_foreign_message(0).c());
  EXPECT_EQ(220, message.repeated_import_message (0).d());

  EXPECT_EQ(unittest::TestAllTypes::BAR, message.repeated_nested_enum (0));
  EXPECT_EQ(unittest::FOREIGN_BAR      , message.repeated_foreign_enum(0));
  EXPECT_EQ(unittest_import::IMPORT_BAR, message.repeated_import_enum (0));


  // Actually verify the second (modified) elements now.
  EXPECT_EQ(501  , message.repeated_int32   (1));
  EXPECT_EQ(502  , message.repeated_int64   (1));
  EXPECT_EQ(503  , message.repeated_uint32  (1));
  EXPECT_EQ(504  , message.repeated_uint64  (1));
  EXPECT_EQ(505  , message.repeated_sint32  (1));
  EXPECT_EQ(506  , message.repeated_sint64  (1));
  EXPECT_EQ(507  , message.repeated_fixed32 (1));
  EXPECT_EQ(508  , message.repeated_fixed64 (1));
  EXPECT_EQ(509  , message.repeated_sfixed32(1));
  EXPECT_EQ(510  , message.repeated_sfixed64(1));
  EXPECT_EQ(511  , message.repeated_float   (1));
  EXPECT_EQ(512  , message.repeated_double  (1));
  EXPECT_EQ(true , message.repeated_bool    (1));
  EXPECT_EQ("515", message.repeated_string  (1));
  EXPECT_EQ("516", message.repeated_bytes   (1));

  EXPECT_EQ(517, message.repeatedgroup           (1).a());
  EXPECT_EQ(518, message.repeated_nested_message (1).bb());
  EXPECT_EQ(519, message.repeated_foreign_message(1).c());
  EXPECT_EQ(520, message.repeated_import_message (1).d());

  EXPECT_EQ(unittest::TestAllTypes::FOO, message.repeated_nested_enum (1));
  EXPECT_EQ(unittest::FOREIGN_FOO      , message.repeated_foreign_enum(1));
  EXPECT_EQ(unittest_import::IMPORT_FOO, message.repeated_import_enum (1));

}

// ===================================================================
// Extensions
//
// All this code is exactly equivalent to the above code except that it's
// manipulating extension fields instead of normal ones.
//
// I gave up on the 80-char limit here.  Sorry.

void TestUtil::SetAllExtensions(unittest::TestAllExtensions* message) {
  message->SetExtension(unittest::optional_int32_extension   , 101);
  message->SetExtension(unittest::optional_int64_extension   , 102);
  message->SetExtension(unittest::optional_uint32_extension  , 103);
  message->SetExtension(unittest::optional_uint64_extension  , 104);
  message->SetExtension(unittest::optional_sint32_extension  , 105);
  message->SetExtension(unittest::optional_sint64_extension  , 106);
  message->SetExtension(unittest::optional_fixed32_extension , 107);
  message->SetExtension(unittest::optional_fixed64_extension , 108);
  message->SetExtension(unittest::optional_sfixed32_extension, 109);
  message->SetExtension(unittest::optional_sfixed64_extension, 110);
  message->SetExtension(unittest::optional_float_extension   , 111);
  message->SetExtension(unittest::optional_double_extension  , 112);
  message->SetExtension(unittest::optional_bool_extension    , true);
  message->SetExtension(unittest::optional_string_extension  , "115");
  message->SetExtension(unittest::optional_bytes_extension   , "116");

  message->MutableExtension(unittest::optionalgroup_extension           )->set_a(117);
  message->MutableExtension(unittest::optional_nested_message_extension )->set_bb(118);
  message->MutableExtension(unittest::optional_foreign_message_extension)->set_c(119);
  message->MutableExtension(unittest::optional_import_message_extension )->set_d(120);

  message->SetExtension(unittest::optional_nested_enum_extension , unittest::TestAllTypes::BAZ);
  message->SetExtension(unittest::optional_foreign_enum_extension, unittest::FOREIGN_BAZ      );
  message->SetExtension(unittest::optional_import_enum_extension , unittest_import::IMPORT_BAZ);

  message->SetExtension(unittest::optional_string_piece_extension, "124");
  message->SetExtension(unittest::optional_cord_extension, "125");

  // -----------------------------------------------------------------

  message->AddExtension(unittest::repeated_int32_extension   , 201);
  message->AddExtension(unittest::repeated_int64_extension   , 202);
  message->AddExtension(unittest::repeated_uint32_extension  , 203);
  message->AddExtension(unittest::repeated_uint64_extension  , 204);
  message->AddExtension(unittest::repeated_sint32_extension  , 205);
  message->AddExtension(unittest::repeated_sint64_extension  , 206);
  message->AddExtension(unittest::repeated_fixed32_extension , 207);
  message->AddExtension(unittest::repeated_fixed64_extension , 208);
  message->AddExtension(unittest::repeated_sfixed32_extension, 209);
  message->AddExtension(unittest::repeated_sfixed64_extension, 210);
  message->AddExtension(unittest::repeated_float_extension   , 211);
  message->AddExtension(unittest::repeated_double_extension  , 212);
  message->AddExtension(unittest::repeated_bool_extension    , true);
  message->AddExtension(unittest::repeated_string_extension  , "215");
  message->AddExtension(unittest::repeated_bytes_extension   , "216");

  message->AddExtension(unittest::repeatedgroup_extension           )->set_a(217);
  message->AddExtension(unittest::repeated_nested_message_extension )->set_bb(218);
  message->AddExtension(unittest::repeated_foreign_message_extension)->set_c(219);
  message->AddExtension(unittest::repeated_import_message_extension )->set_d(220);

  message->AddExtension(unittest::repeated_nested_enum_extension , unittest::TestAllTypes::BAR);
  message->AddExtension(unittest::repeated_foreign_enum_extension, unittest::FOREIGN_BAR      );
  message->AddExtension(unittest::repeated_import_enum_extension , unittest_import::IMPORT_BAR);

  message->AddExtension(unittest::repeated_string_piece_extension, "224");
  message->AddExtension(unittest::repeated_cord_extension, "225");

  // Add a second one of each field.
  message->AddExtension(unittest::repeated_int32_extension   , 301);
  message->AddExtension(unittest::repeated_int64_extension   , 302);
  message->AddExtension(unittest::repeated_uint32_extension  , 303);
  message->AddExtension(unittest::repeated_uint64_extension  , 304);
  message->AddExtension(unittest::repeated_sint32_extension  , 305);
  message->AddExtension(unittest::repeated_sint64_extension  , 306);
  message->AddExtension(unittest::repeated_fixed32_extension , 307);
  message->AddExtension(unittest::repeated_fixed64_extension , 308);
  message->AddExtension(unittest::repeated_sfixed32_extension, 309);
  message->AddExtension(unittest::repeated_sfixed64_extension, 310);
  message->AddExtension(unittest::repeated_float_extension   , 311);
  message->AddExtension(unittest::repeated_double_extension  , 312);
  message->AddExtension(unittest::repeated_bool_extension    , false);
  message->AddExtension(unittest::repeated_string_extension  , "315");
  message->AddExtension(unittest::repeated_bytes_extension   , "316");

  message->AddExtension(unittest::repeatedgroup_extension           )->set_a(317);
  message->AddExtension(unittest::repeated_nested_message_extension )->set_bb(318);
  message->AddExtension(unittest::repeated_foreign_message_extension)->set_c(319);
  message->AddExtension(unittest::repeated_import_message_extension )->set_d(320);

  message->AddExtension(unittest::repeated_nested_enum_extension , unittest::TestAllTypes::BAZ);
  message->AddExtension(unittest::repeated_foreign_enum_extension, unittest::FOREIGN_BAZ      );
  message->AddExtension(unittest::repeated_import_enum_extension , unittest_import::IMPORT_BAZ);

  message->AddExtension(unittest::repeated_string_piece_extension, "324");
  message->AddExtension(unittest::repeated_cord_extension, "325");

  // -----------------------------------------------------------------

  message->SetExtension(unittest::default_int32_extension   , 401);
  message->SetExtension(unittest::default_int64_extension   , 402);
  message->SetExtension(unittest::default_uint32_extension  , 403);
  message->SetExtension(unittest::default_uint64_extension  , 404);
  message->SetExtension(unittest::default_sint32_extension  , 405);
  message->SetExtension(unittest::default_sint64_extension  , 406);
  message->SetExtension(unittest::default_fixed32_extension , 407);
  message->SetExtension(unittest::default_fixed64_extension , 408);
  message->SetExtension(unittest::default_sfixed32_extension, 409);
  message->SetExtension(unittest::default_sfixed64_extension, 410);
  message->SetExtension(unittest::default_float_extension   , 411);
  message->SetExtension(unittest::default_double_extension  , 412);
  message->SetExtension(unittest::default_bool_extension    , false);
  message->SetExtension(unittest::default_string_extension  , "415");
  message->SetExtension(unittest::default_bytes_extension   , "416");

  message->SetExtension(unittest::default_nested_enum_extension , unittest::TestAllTypes::FOO);
  message->SetExtension(unittest::default_foreign_enum_extension, unittest::FOREIGN_FOO      );
  message->SetExtension(unittest::default_import_enum_extension , unittest_import::IMPORT_FOO);

  message->SetExtension(unittest::default_string_piece_extension, "424");
  message->SetExtension(unittest::default_cord_extension, "425");
}

// -------------------------------------------------------------------

void TestUtil::SetAllFieldsAndExtensions(
    unittest::TestFieldOrderings* message) {
  GOOGLE_CHECK(message);
  message->set_my_int(1);
  message->set_my_string("foo");
  message->set_my_float(1.0);
  message->SetExtension(unittest::my_extension_int, 23);
  message->SetExtension(unittest::my_extension_string, "bar");
}

// -------------------------------------------------------------------

void TestUtil::ModifyRepeatedExtensions(unittest::TestAllExtensions* message) {
  message->SetExtension(unittest::repeated_int32_extension   , 1, 501);
  message->SetExtension(unittest::repeated_int64_extension   , 1, 502);
  message->SetExtension(unittest::repeated_uint32_extension  , 1, 503);
  message->SetExtension(unittest::repeated_uint64_extension  , 1, 504);
  message->SetExtension(unittest::repeated_sint32_extension  , 1, 505);
  message->SetExtension(unittest::repeated_sint64_extension  , 1, 506);
  message->SetExtension(unittest::repeated_fixed32_extension , 1, 507);
  message->SetExtension(unittest::repeated_fixed64_extension , 1, 508);
  message->SetExtension(unittest::repeated_sfixed32_extension, 1, 509);
  message->SetExtension(unittest::repeated_sfixed64_extension, 1, 510);
  message->SetExtension(unittest::repeated_float_extension   , 1, 511);
  message->SetExtension(unittest::repeated_double_extension  , 1, 512);
  message->SetExtension(unittest::repeated_bool_extension    , 1, true);
  message->SetExtension(unittest::repeated_string_extension  , 1, "515");
  message->SetExtension(unittest::repeated_bytes_extension   , 1, "516");

  message->MutableExtension(unittest::repeatedgroup_extension           , 1)->set_a(517);
  message->MutableExtension(unittest::repeated_nested_message_extension , 1)->set_bb(518);
  message->MutableExtension(unittest::repeated_foreign_message_extension, 1)->set_c(519);
  message->MutableExtension(unittest::repeated_import_message_extension , 1)->set_d(520);

  message->SetExtension(unittest::repeated_nested_enum_extension , 1, unittest::TestAllTypes::FOO);
  message->SetExtension(unittest::repeated_foreign_enum_extension, 1, unittest::FOREIGN_FOO      );
  message->SetExtension(unittest::repeated_import_enum_extension , 1, unittest_import::IMPORT_FOO);

  message->SetExtension(unittest::repeated_string_piece_extension, 1, "524");
  message->SetExtension(unittest::repeated_cord_extension, 1, "525");
}

// -------------------------------------------------------------------

void TestUtil::ExpectAllExtensionsSet(
    const unittest::TestAllExtensions& message) {
  EXPECT_TRUE(message.HasExtension(unittest::optional_int32_extension   ));
  EXPECT_TRUE(message.HasExtension(unittest::optional_int64_extension   ));
  EXPECT_TRUE(message.HasExtension(unittest::optional_uint32_extension  ));
  EXPECT_TRUE(message.HasExtension(unittest::optional_uint64_extension  ));
  EXPECT_TRUE(message.HasExtension(unittest::optional_sint32_extension  ));
  EXPECT_TRUE(message.HasExtension(unittest::optional_sint64_extension  ));
  EXPECT_TRUE(message.HasExtension(unittest::optional_fixed32_extension ));
  EXPECT_TRUE(message.HasExtension(unittest::optional_fixed64_extension ));
  EXPECT_TRUE(message.HasExtension(unittest::optional_sfixed32_extension));
  EXPECT_TRUE(message.HasExtension(unittest::optional_sfixed64_extension));
  EXPECT_TRUE(message.HasExtension(unittest::optional_float_extension   ));
  EXPECT_TRUE(message.HasExtension(unittest::optional_double_extension  ));
  EXPECT_TRUE(message.HasExtension(unittest::optional_bool_extension    ));
  EXPECT_TRUE(message.HasExtension(unittest::optional_string_extension  ));
  EXPECT_TRUE(message.HasExtension(unittest::optional_bytes_extension   ));

  EXPECT_TRUE(message.HasExtension(unittest::optionalgroup_extension           ));
  EXPECT_TRUE(message.HasExtension(unittest::optional_nested_message_extension ));
  EXPECT_TRUE(message.HasExtension(unittest::optional_foreign_message_extension));
  EXPECT_TRUE(message.HasExtension(unittest::optional_import_message_extension ));

  EXPECT_TRUE(message.GetExtension(unittest::optionalgroup_extension           ).has_a());
  EXPECT_TRUE(message.GetExtension(unittest::optional_nested_message_extension ).has_bb());
  EXPECT_TRUE(message.GetExtension(unittest::optional_foreign_message_extension).has_c());
  EXPECT_TRUE(message.GetExtension(unittest::optional_import_message_extension ).has_d());

  EXPECT_TRUE(message.HasExtension(unittest::optional_nested_enum_extension ));
  EXPECT_TRUE(message.HasExtension(unittest::optional_foreign_enum_extension));
  EXPECT_TRUE(message.HasExtension(unittest::optional_import_enum_extension ));

  EXPECT_TRUE(message.HasExtension(unittest::optional_string_piece_extension));
  EXPECT_TRUE(message.HasExtension(unittest::optional_cord_extension));

  EXPECT_EQ(101  , message.GetExtension(unittest::optional_int32_extension   ));
  EXPECT_EQ(102  , message.GetExtension(unittest::optional_int64_extension   ));
  EXPECT_EQ(103  , message.GetExtension(unittest::optional_uint32_extension  ));
  EXPECT_EQ(104  , message.GetExtension(unittest::optional_uint64_extension  ));
  EXPECT_EQ(105  , message.GetExtension(unittest::optional_sint32_extension  ));
  EXPECT_EQ(106  , message.GetExtension(unittest::optional_sint64_extension  ));
  EXPECT_EQ(107  , message.GetExtension(unittest::optional_fixed32_extension ));
  EXPECT_EQ(108  , message.GetExtension(unittest::optional_fixed64_extension ));
  EXPECT_EQ(109  , message.GetExtension(unittest::optional_sfixed32_extension));
  EXPECT_EQ(110  , message.GetExtension(unittest::optional_sfixed64_extension));
  EXPECT_EQ(111  , message.GetExtension(unittest::optional_float_extension   ));
  EXPECT_EQ(112  , message.GetExtension(unittest::optional_double_extension  ));
  EXPECT_EQ(true , message.GetExtension(unittest::optional_bool_extension    ));
  EXPECT_EQ("115", message.GetExtension(unittest::optional_string_extension  ));
  EXPECT_EQ("116", message.GetExtension(unittest::optional_bytes_extension   ));

  EXPECT_EQ(117, message.GetExtension(unittest::optionalgroup_extension           ).a());
  EXPECT_EQ(118, message.GetExtension(unittest::optional_nested_message_extension ).bb());
  EXPECT_EQ(119, message.GetExtension(unittest::optional_foreign_message_extension).c());
  EXPECT_EQ(120, message.GetExtension(unittest::optional_import_message_extension ).d());

  EXPECT_EQ(unittest::TestAllTypes::BAZ, message.GetExtension(unittest::optional_nested_enum_extension ));
  EXPECT_EQ(unittest::FOREIGN_BAZ      , message.GetExtension(unittest::optional_foreign_enum_extension));
  EXPECT_EQ(unittest_import::IMPORT_BAZ, message.GetExtension(unittest::optional_import_enum_extension ));

  EXPECT_EQ("124", message.GetExtension(unittest::optional_string_piece_extension));
  EXPECT_EQ("125", message.GetExtension(unittest::optional_cord_extension));

  // -----------------------------------------------------------------

  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_int32_extension   ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_int64_extension   ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_uint32_extension  ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_uint64_extension  ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_sint32_extension  ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_sint64_extension  ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_fixed32_extension ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_fixed64_extension ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_sfixed32_extension));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_sfixed64_extension));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_float_extension   ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_double_extension  ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_bool_extension    ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_string_extension  ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_bytes_extension   ));

  ASSERT_EQ(2, message.ExtensionSize(unittest::repeatedgroup_extension           ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_nested_message_extension ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_foreign_message_extension));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_import_message_extension ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_nested_enum_extension    ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_foreign_enum_extension   ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_import_enum_extension    ));

  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_string_piece_extension));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_cord_extension));

  EXPECT_EQ(201  , message.GetExtension(unittest::repeated_int32_extension   , 0));
  EXPECT_EQ(202  , message.GetExtension(unittest::repeated_int64_extension   , 0));
  EXPECT_EQ(203  , message.GetExtension(unittest::repeated_uint32_extension  , 0));
  EXPECT_EQ(204  , message.GetExtension(unittest::repeated_uint64_extension  , 0));
  EXPECT_EQ(205  , message.GetExtension(unittest::repeated_sint32_extension  , 0));
  EXPECT_EQ(206  , message.GetExtension(unittest::repeated_sint64_extension  , 0));
  EXPECT_EQ(207  , message.GetExtension(unittest::repeated_fixed32_extension , 0));
  EXPECT_EQ(208  , message.GetExtension(unittest::repeated_fixed64_extension , 0));
  EXPECT_EQ(209  , message.GetExtension(unittest::repeated_sfixed32_extension, 0));
  EXPECT_EQ(210  , message.GetExtension(unittest::repeated_sfixed64_extension, 0));
  EXPECT_EQ(211  , message.GetExtension(unittest::repeated_float_extension   , 0));
  EXPECT_EQ(212  , message.GetExtension(unittest::repeated_double_extension  , 0));
  EXPECT_EQ(true , message.GetExtension(unittest::repeated_bool_extension    , 0));
  EXPECT_EQ("215", message.GetExtension(unittest::repeated_string_extension  , 0));
  EXPECT_EQ("216", message.GetExtension(unittest::repeated_bytes_extension   , 0));

  EXPECT_EQ(217, message.GetExtension(unittest::repeatedgroup_extension           , 0).a());
  EXPECT_EQ(218, message.GetExtension(unittest::repeated_nested_message_extension , 0).bb());
  EXPECT_EQ(219, message.GetExtension(unittest::repeated_foreign_message_extension, 0).c());
  EXPECT_EQ(220, message.GetExtension(unittest::repeated_import_message_extension , 0).d());

  EXPECT_EQ(unittest::TestAllTypes::BAR, message.GetExtension(unittest::repeated_nested_enum_extension , 0));
  EXPECT_EQ(unittest::FOREIGN_BAR      , message.GetExtension(unittest::repeated_foreign_enum_extension, 0));
  EXPECT_EQ(unittest_import::IMPORT_BAR, message.GetExtension(unittest::repeated_import_enum_extension , 0));

  EXPECT_EQ("224", message.GetExtension(unittest::repeated_string_piece_extension, 0));
  EXPECT_EQ("225", message.GetExtension(unittest::repeated_cord_extension, 0));

  EXPECT_EQ(301  , message.GetExtension(unittest::repeated_int32_extension   , 1));
  EXPECT_EQ(302  , message.GetExtension(unittest::repeated_int64_extension   , 1));
  EXPECT_EQ(303  , message.GetExtension(unittest::repeated_uint32_extension  , 1));
  EXPECT_EQ(304  , message.GetExtension(unittest::repeated_uint64_extension  , 1));
  EXPECT_EQ(305  , message.GetExtension(unittest::repeated_sint32_extension  , 1));
  EXPECT_EQ(306  , message.GetExtension(unittest::repeated_sint64_extension  , 1));
  EXPECT_EQ(307  , message.GetExtension(unittest::repeated_fixed32_extension , 1));
  EXPECT_EQ(308  , message.GetExtension(unittest::repeated_fixed64_extension , 1));
  EXPECT_EQ(309  , message.GetExtension(unittest::repeated_sfixed32_extension, 1));
  EXPECT_EQ(310  , message.GetExtension(unittest::repeated_sfixed64_extension, 1));
  EXPECT_EQ(311  , message.GetExtension(unittest::repeated_float_extension   , 1));
  EXPECT_EQ(312  , message.GetExtension(unittest::repeated_double_extension  , 1));
  EXPECT_EQ(false, message.GetExtension(unittest::repeated_bool_extension    , 1));
  EXPECT_EQ("315", message.GetExtension(unittest::repeated_string_extension  , 1));
  EXPECT_EQ("316", message.GetExtension(unittest::repeated_bytes_extension   , 1));

  EXPECT_EQ(317, message.GetExtension(unittest::repeatedgroup_extension           , 1).a());
  EXPECT_EQ(318, message.GetExtension(unittest::repeated_nested_message_extension , 1).bb());
  EXPECT_EQ(319, message.GetExtension(unittest::repeated_foreign_message_extension, 1).c());
  EXPECT_EQ(320, message.GetExtension(unittest::repeated_import_message_extension , 1).d());

  EXPECT_EQ(unittest::TestAllTypes::BAZ, message.GetExtension(unittest::repeated_nested_enum_extension , 1));
  EXPECT_EQ(unittest::FOREIGN_BAZ      , message.GetExtension(unittest::repeated_foreign_enum_extension, 1));
  EXPECT_EQ(unittest_import::IMPORT_BAZ, message.GetExtension(unittest::repeated_import_enum_extension , 1));

  EXPECT_EQ("324", message.GetExtension(unittest::repeated_string_piece_extension, 1));
  EXPECT_EQ("325", message.GetExtension(unittest::repeated_cord_extension, 1));

  // -----------------------------------------------------------------

  EXPECT_TRUE(message.HasExtension(unittest::default_int32_extension   ));
  EXPECT_TRUE(message.HasExtension(unittest::default_int64_extension   ));
  EXPECT_TRUE(message.HasExtension(unittest::default_uint32_extension  ));
  EXPECT_TRUE(message.HasExtension(unittest::default_uint64_extension  ));
  EXPECT_TRUE(message.HasExtension(unittest::default_sint32_extension  ));
  EXPECT_TRUE(message.HasExtension(unittest::default_sint64_extension  ));
  EXPECT_TRUE(message.HasExtension(unittest::default_fixed32_extension ));
  EXPECT_TRUE(message.HasExtension(unittest::default_fixed64_extension ));
  EXPECT_TRUE(message.HasExtension(unittest::default_sfixed32_extension));
  EXPECT_TRUE(message.HasExtension(unittest::default_sfixed64_extension));
  EXPECT_TRUE(message.HasExtension(unittest::default_float_extension   ));
  EXPECT_TRUE(message.HasExtension(unittest::default_double_extension  ));
  EXPECT_TRUE(message.HasExtension(unittest::default_bool_extension    ));
  EXPECT_TRUE(message.HasExtension(unittest::default_string_extension  ));
  EXPECT_TRUE(message.HasExtension(unittest::default_bytes_extension   ));

  EXPECT_TRUE(message.HasExtension(unittest::default_nested_enum_extension ));
  EXPECT_TRUE(message.HasExtension(unittest::default_foreign_enum_extension));
  EXPECT_TRUE(message.HasExtension(unittest::default_import_enum_extension ));

  EXPECT_TRUE(message.HasExtension(unittest::default_string_piece_extension));
  EXPECT_TRUE(message.HasExtension(unittest::default_cord_extension));

  EXPECT_EQ(401  , message.GetExtension(unittest::default_int32_extension   ));
  EXPECT_EQ(402  , message.GetExtension(unittest::default_int64_extension   ));
  EXPECT_EQ(403  , message.GetExtension(unittest::default_uint32_extension  ));
  EXPECT_EQ(404  , message.GetExtension(unittest::default_uint64_extension  ));
  EXPECT_EQ(405  , message.GetExtension(unittest::default_sint32_extension  ));
  EXPECT_EQ(406  , message.GetExtension(unittest::default_sint64_extension  ));
  EXPECT_EQ(407  , message.GetExtension(unittest::default_fixed32_extension ));
  EXPECT_EQ(408  , message.GetExtension(unittest::default_fixed64_extension ));
  EXPECT_EQ(409  , message.GetExtension(unittest::default_sfixed32_extension));
  EXPECT_EQ(410  , message.GetExtension(unittest::default_sfixed64_extension));
  EXPECT_EQ(411  , message.GetExtension(unittest::default_float_extension   ));
  EXPECT_EQ(412  , message.GetExtension(unittest::default_double_extension  ));
  EXPECT_EQ(false, message.GetExtension(unittest::default_bool_extension    ));
  EXPECT_EQ("415", message.GetExtension(unittest::default_string_extension  ));
  EXPECT_EQ("416", message.GetExtension(unittest::default_bytes_extension   ));

  EXPECT_EQ(unittest::TestAllTypes::FOO, message.GetExtension(unittest::default_nested_enum_extension ));
  EXPECT_EQ(unittest::FOREIGN_FOO      , message.GetExtension(unittest::default_foreign_enum_extension));
  EXPECT_EQ(unittest_import::IMPORT_FOO, message.GetExtension(unittest::default_import_enum_extension ));

  EXPECT_EQ("424", message.GetExtension(unittest::default_string_piece_extension));
  EXPECT_EQ("425", message.GetExtension(unittest::default_cord_extension));
}

// -------------------------------------------------------------------

void TestUtil::ExpectExtensionsClear(
    const unittest::TestAllExtensions& message) {
  string serialized;
  ASSERT_TRUE(message.SerializeToString(&serialized));
  EXPECT_EQ("", serialized);
  EXPECT_EQ(0, message.ByteSize());

  // has_blah() should initially be false for all optional fields.
  EXPECT_FALSE(message.HasExtension(unittest::optional_int32_extension   ));
  EXPECT_FALSE(message.HasExtension(unittest::optional_int64_extension   ));
  EXPECT_FALSE(message.HasExtension(unittest::optional_uint32_extension  ));
  EXPECT_FALSE(message.HasExtension(unittest::optional_uint64_extension  ));
  EXPECT_FALSE(message.HasExtension(unittest::optional_sint32_extension  ));
  EXPECT_FALSE(message.HasExtension(unittest::optional_sint64_extension  ));
  EXPECT_FALSE(message.HasExtension(unittest::optional_fixed32_extension ));
  EXPECT_FALSE(message.HasExtension(unittest::optional_fixed64_extension ));
  EXPECT_FALSE(message.HasExtension(unittest::optional_sfixed32_extension));
  EXPECT_FALSE(message.HasExtension(unittest::optional_sfixed64_extension));
  EXPECT_FALSE(message.HasExtension(unittest::optional_float_extension   ));
  EXPECT_FALSE(message.HasExtension(unittest::optional_double_extension  ));
  EXPECT_FALSE(message.HasExtension(unittest::optional_bool_extension    ));
  EXPECT_FALSE(message.HasExtension(unittest::optional_string_extension  ));
  EXPECT_FALSE(message.HasExtension(unittest::optional_bytes_extension   ));

  EXPECT_FALSE(message.HasExtension(unittest::optionalgroup_extension           ));
  EXPECT_FALSE(message.HasExtension(unittest::optional_nested_message_extension ));
  EXPECT_FALSE(message.HasExtension(unittest::optional_foreign_message_extension));
  EXPECT_FALSE(message.HasExtension(unittest::optional_import_message_extension ));

  EXPECT_FALSE(message.HasExtension(unittest::optional_nested_enum_extension ));
  EXPECT_FALSE(message.HasExtension(unittest::optional_foreign_enum_extension));
  EXPECT_FALSE(message.HasExtension(unittest::optional_import_enum_extension ));

  EXPECT_FALSE(message.HasExtension(unittest::optional_string_piece_extension));
  EXPECT_FALSE(message.HasExtension(unittest::optional_cord_extension));

  // Optional fields without defaults are set to zero or something like it.
  EXPECT_EQ(0    , message.GetExtension(unittest::optional_int32_extension   ));
  EXPECT_EQ(0    , message.GetExtension(unittest::optional_int64_extension   ));
  EXPECT_EQ(0    , message.GetExtension(unittest::optional_uint32_extension  ));
  EXPECT_EQ(0    , message.GetExtension(unittest::optional_uint64_extension  ));
  EXPECT_EQ(0    , message.GetExtension(unittest::optional_sint32_extension  ));
  EXPECT_EQ(0    , message.GetExtension(unittest::optional_sint64_extension  ));
  EXPECT_EQ(0    , message.GetExtension(unittest::optional_fixed32_extension ));
  EXPECT_EQ(0    , message.GetExtension(unittest::optional_fixed64_extension ));
  EXPECT_EQ(0    , message.GetExtension(unittest::optional_sfixed32_extension));
  EXPECT_EQ(0    , message.GetExtension(unittest::optional_sfixed64_extension));
  EXPECT_EQ(0    , message.GetExtension(unittest::optional_float_extension   ));
  EXPECT_EQ(0    , message.GetExtension(unittest::optional_double_extension  ));
  EXPECT_EQ(false, message.GetExtension(unittest::optional_bool_extension    ));
  EXPECT_EQ(""   , message.GetExtension(unittest::optional_string_extension  ));
  EXPECT_EQ(""   , message.GetExtension(unittest::optional_bytes_extension   ));

  // Embedded messages should also be clear.
  EXPECT_FALSE(message.GetExtension(unittest::optionalgroup_extension           ).has_a());
  EXPECT_FALSE(message.GetExtension(unittest::optional_nested_message_extension ).has_bb());
  EXPECT_FALSE(message.GetExtension(unittest::optional_foreign_message_extension).has_c());
  EXPECT_FALSE(message.GetExtension(unittest::optional_import_message_extension ).has_d());

  EXPECT_EQ(0, message.GetExtension(unittest::optionalgroup_extension           ).a());
  EXPECT_EQ(0, message.GetExtension(unittest::optional_nested_message_extension ).bb());
  EXPECT_EQ(0, message.GetExtension(unittest::optional_foreign_message_extension).c());
  EXPECT_EQ(0, message.GetExtension(unittest::optional_import_message_extension ).d());

  // Enums without defaults are set to the first value in the enum.
  EXPECT_EQ(unittest::TestAllTypes::FOO, message.GetExtension(unittest::optional_nested_enum_extension ));
  EXPECT_EQ(unittest::FOREIGN_FOO      , message.GetExtension(unittest::optional_foreign_enum_extension));
  EXPECT_EQ(unittest_import::IMPORT_FOO, message.GetExtension(unittest::optional_import_enum_extension ));

  EXPECT_EQ("", message.GetExtension(unittest::optional_string_piece_extension));
  EXPECT_EQ("", message.GetExtension(unittest::optional_cord_extension));

  // Repeated fields are empty.
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_int32_extension   ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_int64_extension   ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_uint32_extension  ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_uint64_extension  ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_sint32_extension  ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_sint64_extension  ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_fixed32_extension ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_fixed64_extension ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_sfixed32_extension));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_sfixed64_extension));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_float_extension   ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_double_extension  ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_bool_extension    ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_string_extension  ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_bytes_extension   ));

  EXPECT_EQ(0, message.ExtensionSize(unittest::repeatedgroup_extension           ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_nested_message_extension ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_foreign_message_extension));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_import_message_extension ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_nested_enum_extension    ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_foreign_enum_extension   ));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_import_enum_extension    ));

  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_string_piece_extension));
  EXPECT_EQ(0, message.ExtensionSize(unittest::repeated_cord_extension));

  // has_blah() should also be false for all default fields.
  EXPECT_FALSE(message.HasExtension(unittest::default_int32_extension   ));
  EXPECT_FALSE(message.HasExtension(unittest::default_int64_extension   ));
  EXPECT_FALSE(message.HasExtension(unittest::default_uint32_extension  ));
  EXPECT_FALSE(message.HasExtension(unittest::default_uint64_extension  ));
  EXPECT_FALSE(message.HasExtension(unittest::default_sint32_extension  ));
  EXPECT_FALSE(message.HasExtension(unittest::default_sint64_extension  ));
  EXPECT_FALSE(message.HasExtension(unittest::default_fixed32_extension ));
  EXPECT_FALSE(message.HasExtension(unittest::default_fixed64_extension ));
  EXPECT_FALSE(message.HasExtension(unittest::default_sfixed32_extension));
  EXPECT_FALSE(message.HasExtension(unittest::default_sfixed64_extension));
  EXPECT_FALSE(message.HasExtension(unittest::default_float_extension   ));
  EXPECT_FALSE(message.HasExtension(unittest::default_double_extension  ));
  EXPECT_FALSE(message.HasExtension(unittest::default_bool_extension    ));
  EXPECT_FALSE(message.HasExtension(unittest::default_string_extension  ));
  EXPECT_FALSE(message.HasExtension(unittest::default_bytes_extension   ));

  EXPECT_FALSE(message.HasExtension(unittest::default_nested_enum_extension ));
  EXPECT_FALSE(message.HasExtension(unittest::default_foreign_enum_extension));
  EXPECT_FALSE(message.HasExtension(unittest::default_import_enum_extension ));

  EXPECT_FALSE(message.HasExtension(unittest::default_string_piece_extension));
  EXPECT_FALSE(message.HasExtension(unittest::default_cord_extension));

  // Fields with defaults have their default values (duh).
  EXPECT_EQ( 41    , message.GetExtension(unittest::default_int32_extension   ));
  EXPECT_EQ( 42    , message.GetExtension(unittest::default_int64_extension   ));
  EXPECT_EQ( 43    , message.GetExtension(unittest::default_uint32_extension  ));
  EXPECT_EQ( 44    , message.GetExtension(unittest::default_uint64_extension  ));
  EXPECT_EQ(-45    , message.GetExtension(unittest::default_sint32_extension  ));
  EXPECT_EQ( 46    , message.GetExtension(unittest::default_sint64_extension  ));
  EXPECT_EQ( 47    , message.GetExtension(unittest::default_fixed32_extension ));
  EXPECT_EQ( 48    , message.GetExtension(unittest::default_fixed64_extension ));
  EXPECT_EQ( 49    , message.GetExtension(unittest::default_sfixed32_extension));
  EXPECT_EQ(-50    , message.GetExtension(unittest::default_sfixed64_extension));
  EXPECT_EQ( 51.5  , message.GetExtension(unittest::default_float_extension   ));
  EXPECT_EQ( 52e3  , message.GetExtension(unittest::default_double_extension  ));
  EXPECT_EQ(true   , message.GetExtension(unittest::default_bool_extension    ));
  EXPECT_EQ("hello", message.GetExtension(unittest::default_string_extension  ));
  EXPECT_EQ("world", message.GetExtension(unittest::default_bytes_extension   ));

  EXPECT_EQ(unittest::TestAllTypes::BAR, message.GetExtension(unittest::default_nested_enum_extension ));
  EXPECT_EQ(unittest::FOREIGN_BAR      , message.GetExtension(unittest::default_foreign_enum_extension));
  EXPECT_EQ(unittest_import::IMPORT_BAR, message.GetExtension(unittest::default_import_enum_extension ));

  EXPECT_EQ("abc", message.GetExtension(unittest::default_string_piece_extension));
  EXPECT_EQ("123", message.GetExtension(unittest::default_cord_extension));
}

// -------------------------------------------------------------------

void TestUtil::ExpectRepeatedExtensionsModified(
    const unittest::TestAllExtensions& message) {
  // ModifyRepeatedFields only sets the second repeated element of each
  // field.  In addition to verifying this, we also verify that the first
  // element and size were *not* modified.
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_int32_extension   ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_int64_extension   ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_uint32_extension  ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_uint64_extension  ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_sint32_extension  ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_sint64_extension  ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_fixed32_extension ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_fixed64_extension ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_sfixed32_extension));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_sfixed64_extension));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_float_extension   ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_double_extension  ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_bool_extension    ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_string_extension  ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_bytes_extension   ));

  ASSERT_EQ(2, message.ExtensionSize(unittest::repeatedgroup_extension           ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_nested_message_extension ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_foreign_message_extension));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_import_message_extension ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_nested_enum_extension    ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_foreign_enum_extension   ));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_import_enum_extension    ));

  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_string_piece_extension));
  ASSERT_EQ(2, message.ExtensionSize(unittest::repeated_cord_extension));

  EXPECT_EQ(201  , message.GetExtension(unittest::repeated_int32_extension   , 0));
  EXPECT_EQ(202  , message.GetExtension(unittest::repeated_int64_extension   , 0));
  EXPECT_EQ(203  , message.GetExtension(unittest::repeated_uint32_extension  , 0));
  EXPECT_EQ(204  , message.GetExtension(unittest::repeated_uint64_extension  , 0));
  EXPECT_EQ(205  , message.GetExtension(unittest::repeated_sint32_extension  , 0));
  EXPECT_EQ(206  , message.GetExtension(unittest::repeated_sint64_extension  , 0));
  EXPECT_EQ(207  , message.GetExtension(unittest::repeated_fixed32_extension , 0));
  EXPECT_EQ(208  , message.GetExtension(unittest::repeated_fixed64_extension , 0));
  EXPECT_EQ(209  , message.GetExtension(unittest::repeated_sfixed32_extension, 0));
  EXPECT_EQ(210  , message.GetExtension(unittest::repeated_sfixed64_extension, 0));
  EXPECT_EQ(211  , message.GetExtension(unittest::repeated_float_extension   , 0));
  EXPECT_EQ(212  , message.GetExtension(unittest::repeated_double_extension  , 0));
  EXPECT_EQ(true , message.GetExtension(unittest::repeated_bool_extension    , 0));
  EXPECT_EQ("215", message.GetExtension(unittest::repeated_string_extension  , 0));
  EXPECT_EQ("216", message.GetExtension(unittest::repeated_bytes_extension   , 0));

  EXPECT_EQ(217, message.GetExtension(unittest::repeatedgroup_extension           , 0).a());
  EXPECT_EQ(218, message.GetExtension(unittest::repeated_nested_message_extension , 0).bb());
  EXPECT_EQ(219, message.GetExtension(unittest::repeated_foreign_message_extension, 0).c());
  EXPECT_EQ(220, message.GetExtension(unittest::repeated_import_message_extension , 0).d());

  EXPECT_EQ(unittest::TestAllTypes::BAR, message.GetExtension(unittest::repeated_nested_enum_extension , 0));
  EXPECT_EQ(unittest::FOREIGN_BAR      , message.GetExtension(unittest::repeated_foreign_enum_extension, 0));
  EXPECT_EQ(unittest_import::IMPORT_BAR, message.GetExtension(unittest::repeated_import_enum_extension , 0));

  EXPECT_EQ("224", message.GetExtension(unittest::repeated_string_piece_extension, 0));
  EXPECT_EQ("225", message.GetExtension(unittest::repeated_cord_extension, 0));

  // Actually verify the second (modified) elements now.
  EXPECT_EQ(501  , message.GetExtension(unittest::repeated_int32_extension   , 1));
  EXPECT_EQ(502  , message.GetExtension(unittest::repeated_int64_extension   , 1));
  EXPECT_EQ(503  , message.GetExtension(unittest::repeated_uint32_extension  , 1));
  EXPECT_EQ(504  , message.GetExtension(unittest::repeated_uint64_extension  , 1));
  EXPECT_EQ(505  , message.GetExtension(unittest::repeated_sint32_extension  , 1));
  EXPECT_EQ(506  , message.GetExtension(unittest::repeated_sint64_extension  , 1));
  EXPECT_EQ(507  , message.GetExtension(unittest::repeated_fixed32_extension , 1));
  EXPECT_EQ(508  , message.GetExtension(unittest::repeated_fixed64_extension , 1));
  EXPECT_EQ(509  , message.GetExtension(unittest::repeated_sfixed32_extension, 1));
  EXPECT_EQ(510  , message.GetExtension(unittest::repeated_sfixed64_extension, 1));
  EXPECT_EQ(511  , message.GetExtension(unittest::repeated_float_extension   , 1));
  EXPECT_EQ(512  , message.GetExtension(unittest::repeated_double_extension  , 1));
  EXPECT_EQ(true , message.GetExtension(unittest::repeated_bool_extension    , 1));
  EXPECT_EQ("515", message.GetExtension(unittest::repeated_string_extension  , 1));
  EXPECT_EQ("516", message.GetExtension(unittest::repeated_bytes_extension   , 1));

  EXPECT_EQ(517, message.GetExtension(unittest::repeatedgroup_extension           , 1).a());
  EXPECT_EQ(518, message.GetExtension(unittest::repeated_nested_message_extension , 1).bb());
  EXPECT_EQ(519, message.GetExtension(unittest::repeated_foreign_message_extension, 1).c());
  EXPECT_EQ(520, message.GetExtension(unittest::repeated_import_message_extension , 1).d());

  EXPECT_EQ(unittest::TestAllTypes::FOO, message.GetExtension(unittest::repeated_nested_enum_extension , 1));
  EXPECT_EQ(unittest::FOREIGN_FOO      , message.GetExtension(unittest::repeated_foreign_enum_extension, 1));
  EXPECT_EQ(unittest_import::IMPORT_FOO, message.GetExtension(unittest::repeated_import_enum_extension , 1));

  EXPECT_EQ("524", message.GetExtension(unittest::repeated_string_piece_extension, 1));
  EXPECT_EQ("525", message.GetExtension(unittest::repeated_cord_extension, 1));
}

// -------------------------------------------------------------------

void TestUtil::ExpectAllFieldsAndExtensionsInOrder(const string& serialized) {
  // We set each field individually, serialize separately, and concatenate all
  // the strings in canonical order to determine the expected serialization.
  string expected;
  unittest::TestFieldOrderings message;
  message.set_my_int(1);  // Field 1.
  message.AppendToString(&expected);
  message.Clear();
  message.SetExtension(unittest::my_extension_int, 23);  // Field 5.
  message.AppendToString(&expected);
  message.Clear();
  message.set_my_string("foo");  // Field 11.
  message.AppendToString(&expected);
  message.Clear();
  message.SetExtension(unittest::my_extension_string, "bar");  // Field 50.
  message.AppendToString(&expected);
  message.Clear();
  message.set_my_float(1.0);  // Field 101.
  message.AppendToString(&expected);
  message.Clear();

  // We don't EXPECT_EQ() since we don't want to print raw bytes to stdout.
  EXPECT_TRUE(serialized == expected);
}

// ===================================================================

TestUtil::ReflectionTester::ReflectionTester(
    const Descriptor* base_descriptor)
  : base_descriptor_(base_descriptor) {

  const DescriptorPool* pool = base_descriptor->file()->pool();

  nested_b_ =
    pool->FindFieldByName("protobuf_unittest.TestAllTypes.NestedMessage.bb");
  foreign_c_ =
    pool->FindFieldByName("protobuf_unittest.ForeignMessage.c");
  import_d_ =
    pool->FindFieldByName("protobuf_unittest_import.ImportMessage.d");
  nested_foo_ =
    pool->FindEnumValueByName("protobuf_unittest.TestAllTypes.FOO");
  nested_bar_ =
    pool->FindEnumValueByName("protobuf_unittest.TestAllTypes.BAR");
  nested_baz_ =
    pool->FindEnumValueByName("protobuf_unittest.TestAllTypes.BAZ");
  foreign_foo_ =
    pool->FindEnumValueByName("protobuf_unittest.FOREIGN_FOO");
  foreign_bar_ =
    pool->FindEnumValueByName("protobuf_unittest.FOREIGN_BAR");
  foreign_baz_ =
    pool->FindEnumValueByName("protobuf_unittest.FOREIGN_BAZ");
  import_foo_ =
    pool->FindEnumValueByName("protobuf_unittest_import.IMPORT_FOO");
  import_bar_ =
    pool->FindEnumValueByName("protobuf_unittest_import.IMPORT_BAR");
  import_baz_ =
    pool->FindEnumValueByName("protobuf_unittest_import.IMPORT_BAZ");

  if (base_descriptor_->name() == "TestAllExtensions") {
    group_a_ =
      pool->FindFieldByName("protobuf_unittest.OptionalGroup_extension.a");
    repeated_group_a_ =
      pool->FindFieldByName("protobuf_unittest.RepeatedGroup_extension.a");
  } else {
    group_a_ =
      pool->FindFieldByName("protobuf_unittest.TestAllTypes.OptionalGroup.a");
    repeated_group_a_ =
      pool->FindFieldByName("protobuf_unittest.TestAllTypes.RepeatedGroup.a");
  }

  EXPECT_TRUE(group_a_          != NULL);
  EXPECT_TRUE(repeated_group_a_ != NULL);
  EXPECT_TRUE(nested_b_         != NULL);
  EXPECT_TRUE(foreign_c_        != NULL);
  EXPECT_TRUE(import_d_         != NULL);
  EXPECT_TRUE(nested_foo_       != NULL);
  EXPECT_TRUE(nested_bar_       != NULL);
  EXPECT_TRUE(nested_baz_       != NULL);
  EXPECT_TRUE(foreign_foo_      != NULL);
  EXPECT_TRUE(foreign_bar_      != NULL);
  EXPECT_TRUE(foreign_baz_      != NULL);
  EXPECT_TRUE(import_foo_       != NULL);
  EXPECT_TRUE(import_bar_       != NULL);
  EXPECT_TRUE(import_baz_       != NULL);
}

// Shorthand to get a FieldDescriptor for a field of unittest::TestAllTypes.
const FieldDescriptor* TestUtil::ReflectionTester::F(const string& name) {
  const FieldDescriptor* result = NULL;
  if (base_descriptor_->name() == "TestAllExtensions") {
    result = base_descriptor_->file()->FindExtensionByName(name + "_extension");
  } else {
    result = base_descriptor_->FindFieldByName(name);
  }
  GOOGLE_CHECK(result != NULL);
  return result;
}

// -------------------------------------------------------------------

void TestUtil::ReflectionTester::SetAllFieldsViaReflection(
    Message::Reflection* message) {
  message->SetInt32 (F("optional_int32"   ), 101);
  message->SetInt64 (F("optional_int64"   ), 102);
  message->SetUInt32(F("optional_uint32"  ), 103);
  message->SetUInt64(F("optional_uint64"  ), 104);
  message->SetInt32 (F("optional_sint32"  ), 105);
  message->SetInt64 (F("optional_sint64"  ), 106);
  message->SetUInt32(F("optional_fixed32" ), 107);
  message->SetUInt64(F("optional_fixed64" ), 108);
  message->SetInt32 (F("optional_sfixed32"), 109);
  message->SetInt64 (F("optional_sfixed64"), 110);
  message->SetFloat (F("optional_float"   ), 111);
  message->SetDouble(F("optional_double"  ), 112);
  message->SetBool  (F("optional_bool"    ), true);
  message->SetString(F("optional_string"  ), "115");
  message->SetString(F("optional_bytes"   ), "116");

  message->MutableMessage(F("optionalgroup"))
         ->GetReflection()->SetInt32(group_a_, 117);
  message->MutableMessage(F("optional_nested_message"))
         ->GetReflection()->SetInt32(nested_b_, 118);
  message->MutableMessage(F("optional_foreign_message"))
         ->GetReflection()->SetInt32(foreign_c_, 119);
  message->MutableMessage(F("optional_import_message"))
         ->GetReflection()->SetInt32(import_d_, 120);

  message->SetEnum(F("optional_nested_enum" ),  nested_baz_);
  message->SetEnum(F("optional_foreign_enum"), foreign_baz_);
  message->SetEnum(F("optional_import_enum" ),  import_baz_);

  message->SetString(F("optional_string_piece"), "124");
  message->SetString(F("optional_cord"), "125");

  // -----------------------------------------------------------------

  message->AddInt32 (F("repeated_int32"   ), 201);
  message->AddInt64 (F("repeated_int64"   ), 202);
  message->AddUInt32(F("repeated_uint32"  ), 203);
  message->AddUInt64(F("repeated_uint64"  ), 204);
  message->AddInt32 (F("repeated_sint32"  ), 205);
  message->AddInt64 (F("repeated_sint64"  ), 206);
  message->AddUInt32(F("repeated_fixed32" ), 207);
  message->AddUInt64(F("repeated_fixed64" ), 208);
  message->AddInt32 (F("repeated_sfixed32"), 209);
  message->AddInt64 (F("repeated_sfixed64"), 210);
  message->AddFloat (F("repeated_float"   ), 211);
  message->AddDouble(F("repeated_double"  ), 212);
  message->AddBool  (F("repeated_bool"    ), true);
  message->AddString(F("repeated_string"  ), "215");
  message->AddString(F("repeated_bytes"   ), "216");

  message->AddMessage(F("repeatedgroup"))
         ->GetReflection()->SetInt32(repeated_group_a_, 217);
  message->AddMessage(F("repeated_nested_message"))
         ->GetReflection()->SetInt32(nested_b_, 218);
  message->AddMessage(F("repeated_foreign_message"))
         ->GetReflection()->SetInt32(foreign_c_, 219);
  message->AddMessage(F("repeated_import_message"))
         ->GetReflection()->SetInt32(import_d_, 220);

  message->AddEnum(F("repeated_nested_enum" ),  nested_bar_);
  message->AddEnum(F("repeated_foreign_enum"), foreign_bar_);
  message->AddEnum(F("repeated_import_enum" ),  import_bar_);

  message->AddString(F("repeated_string_piece"), "224");
  message->AddString(F("repeated_cord"), "225");

  // Add a second one of each field.
  message->AddInt32 (F("repeated_int32"   ), 301);
  message->AddInt64 (F("repeated_int64"   ), 302);
  message->AddUInt32(F("repeated_uint32"  ), 303);
  message->AddUInt64(F("repeated_uint64"  ), 304);
  message->AddInt32 (F("repeated_sint32"  ), 305);
  message->AddInt64 (F("repeated_sint64"  ), 306);
  message->AddUInt32(F("repeated_fixed32" ), 307);
  message->AddUInt64(F("repeated_fixed64" ), 308);
  message->AddInt32 (F("repeated_sfixed32"), 309);
  message->AddInt64 (F("repeated_sfixed64"), 310);
  message->AddFloat (F("repeated_float"   ), 311);
  message->AddDouble(F("repeated_double"  ), 312);
  message->AddBool  (F("repeated_bool"    ), false);
  message->AddString(F("repeated_string"  ), "315");
  message->AddString(F("repeated_bytes"   ), "316");

  message->AddMessage(F("repeatedgroup"))
         ->GetReflection()->SetInt32(repeated_group_a_, 317);
  message->AddMessage(F("repeated_nested_message"))
         ->GetReflection()->SetInt32(nested_b_, 318);
  message->AddMessage(F("repeated_foreign_message"))
         ->GetReflection()->SetInt32(foreign_c_, 319);
  message->AddMessage(F("repeated_import_message"))
         ->GetReflection()->SetInt32(import_d_, 320);

  message->AddEnum(F("repeated_nested_enum" ),  nested_baz_);
  message->AddEnum(F("repeated_foreign_enum"), foreign_baz_);
  message->AddEnum(F("repeated_import_enum" ),  import_baz_);

  message->AddString(F("repeated_string_piece"), "324");
  message->AddString(F("repeated_cord"), "325");

  // -----------------------------------------------------------------

  message->SetInt32 (F("default_int32"   ), 401);
  message->SetInt64 (F("default_int64"   ), 402);
  message->SetUInt32(F("default_uint32"  ), 403);
  message->SetUInt64(F("default_uint64"  ), 404);
  message->SetInt32 (F("default_sint32"  ), 405);
  message->SetInt64 (F("default_sint64"  ), 406);
  message->SetUInt32(F("default_fixed32" ), 407);
  message->SetUInt64(F("default_fixed64" ), 408);
  message->SetInt32 (F("default_sfixed32"), 409);
  message->SetInt64 (F("default_sfixed64"), 410);
  message->SetFloat (F("default_float"   ), 411);
  message->SetDouble(F("default_double"  ), 412);
  message->SetBool  (F("default_bool"    ), false);
  message->SetString(F("default_string"  ), "415");
  message->SetString(F("default_bytes"   ), "416");

  message->SetEnum(F("default_nested_enum" ),  nested_foo_);
  message->SetEnum(F("default_foreign_enum"), foreign_foo_);
  message->SetEnum(F("default_import_enum" ),  import_foo_);

  message->SetString(F("default_string_piece"), "424");
  message->SetString(F("default_cord"), "425");
}

// -------------------------------------------------------------------

void TestUtil::ReflectionTester::ExpectAllFieldsSetViaReflection(
    const Message::Reflection& message) {
  string scratch;

  EXPECT_TRUE(message.HasField(F("optional_int32"   )));
  EXPECT_TRUE(message.HasField(F("optional_int64"   )));
  EXPECT_TRUE(message.HasField(F("optional_uint32"  )));
  EXPECT_TRUE(message.HasField(F("optional_uint64"  )));
  EXPECT_TRUE(message.HasField(F("optional_sint32"  )));
  EXPECT_TRUE(message.HasField(F("optional_sint64"  )));
  EXPECT_TRUE(message.HasField(F("optional_fixed32" )));
  EXPECT_TRUE(message.HasField(F("optional_fixed64" )));
  EXPECT_TRUE(message.HasField(F("optional_sfixed32")));
  EXPECT_TRUE(message.HasField(F("optional_sfixed64")));
  EXPECT_TRUE(message.HasField(F("optional_float"   )));
  EXPECT_TRUE(message.HasField(F("optional_double"  )));
  EXPECT_TRUE(message.HasField(F("optional_bool"    )));
  EXPECT_TRUE(message.HasField(F("optional_string"  )));
  EXPECT_TRUE(message.HasField(F("optional_bytes"   )));

  EXPECT_TRUE(message.HasField(F("optionalgroup"           )));
  EXPECT_TRUE(message.HasField(F("optional_nested_message" )));
  EXPECT_TRUE(message.HasField(F("optional_foreign_message")));
  EXPECT_TRUE(message.HasField(F("optional_import_message" )));

  EXPECT_TRUE(message.GetMessage(F("optionalgroup"))
                     .GetReflection()->HasField(group_a_));
  EXPECT_TRUE(message.GetMessage(F("optional_nested_message"))
                     .GetReflection()->HasField(nested_b_));
  EXPECT_TRUE(message.GetMessage(F("optional_foreign_message"))
                     .GetReflection()->HasField(foreign_c_));
  EXPECT_TRUE(message.GetMessage(F("optional_import_message"))
                     .GetReflection()->HasField(import_d_));

  EXPECT_TRUE(message.HasField(F("optional_nested_enum" )));
  EXPECT_TRUE(message.HasField(F("optional_foreign_enum")));
  EXPECT_TRUE(message.HasField(F("optional_import_enum" )));

  EXPECT_TRUE(message.HasField(F("optional_string_piece")));
  EXPECT_TRUE(message.HasField(F("optional_cord")));

  EXPECT_EQ(101  , message.GetInt32 (F("optional_int32"   )));
  EXPECT_EQ(102  , message.GetInt64 (F("optional_int64"   )));
  EXPECT_EQ(103  , message.GetUInt32(F("optional_uint32"  )));
  EXPECT_EQ(104  , message.GetUInt64(F("optional_uint64"  )));
  EXPECT_EQ(105  , message.GetInt32 (F("optional_sint32"  )));
  EXPECT_EQ(106  , message.GetInt64 (F("optional_sint64"  )));
  EXPECT_EQ(107  , message.GetUInt32(F("optional_fixed32" )));
  EXPECT_EQ(108  , message.GetUInt64(F("optional_fixed64" )));
  EXPECT_EQ(109  , message.GetInt32 (F("optional_sfixed32")));
  EXPECT_EQ(110  , message.GetInt64 (F("optional_sfixed64")));
  EXPECT_EQ(111  , message.GetFloat (F("optional_float"   )));
  EXPECT_EQ(112  , message.GetDouble(F("optional_double"  )));
  EXPECT_EQ(true , message.GetBool  (F("optional_bool"    )));
  EXPECT_EQ("115", message.GetString(F("optional_string"  )));
  EXPECT_EQ("116", message.GetString(F("optional_bytes"   )));

  EXPECT_EQ("115", message.GetStringReference(F("optional_string"), &scratch));
  EXPECT_EQ("116", message.GetStringReference(F("optional_bytes" ), &scratch));

  EXPECT_EQ(117, message.GetMessage(F("optionalgroup"))
                        .GetReflection()->GetInt32(group_a_));
  EXPECT_EQ(118, message.GetMessage(F("optional_nested_message"))
                        .GetReflection()->GetInt32(nested_b_));
  EXPECT_EQ(119, message.GetMessage(F("optional_foreign_message"))
                        .GetReflection()->GetInt32(foreign_c_));
  EXPECT_EQ(120, message.GetMessage(F("optional_import_message"))
                        .GetReflection()->GetInt32(import_d_));

  EXPECT_EQ( nested_baz_, message.GetEnum(F("optional_nested_enum" )));
  EXPECT_EQ(foreign_baz_, message.GetEnum(F("optional_foreign_enum")));
  EXPECT_EQ( import_baz_, message.GetEnum(F("optional_import_enum" )));

  EXPECT_EQ("124", message.GetString(F("optional_string_piece")));
  EXPECT_EQ("124", message.GetStringReference(F("optional_string_piece"),
                                              &scratch));

  EXPECT_EQ("125", message.GetString(F("optional_cord")));
  EXPECT_EQ("125", message.GetStringReference(F("optional_cord"),
                                              &scratch));

  // -----------------------------------------------------------------

  ASSERT_EQ(2, message.FieldSize(F("repeated_int32"   )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_int64"   )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_uint32"  )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_uint64"  )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_sint32"  )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_sint64"  )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_fixed32" )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_fixed64" )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_sfixed32")));
  ASSERT_EQ(2, message.FieldSize(F("repeated_sfixed64")));
  ASSERT_EQ(2, message.FieldSize(F("repeated_float"   )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_double"  )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_bool"    )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_string"  )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_bytes"   )));

  ASSERT_EQ(2, message.FieldSize(F("repeatedgroup"           )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_nested_message" )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_foreign_message")));
  ASSERT_EQ(2, message.FieldSize(F("repeated_import_message" )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_nested_enum"    )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_foreign_enum"   )));
  ASSERT_EQ(2, message.FieldSize(F("repeated_import_enum"    )));

  ASSERT_EQ(2, message.FieldSize(F("repeated_string_piece")));
  ASSERT_EQ(2, message.FieldSize(F("repeated_cord")));

  EXPECT_EQ(201  , message.GetRepeatedInt32 (F("repeated_int32"   ), 0));
  EXPECT_EQ(202  , message.GetRepeatedInt64 (F("repeated_int64"   ), 0));
  EXPECT_EQ(203  , message.GetRepeatedUInt32(F("repeated_uint32"  ), 0));
  EXPECT_EQ(204  , message.GetRepeatedUInt64(F("repeated_uint64"  ), 0));
  EXPECT_EQ(205  , message.GetRepeatedInt32 (F("repeated_sint32"  ), 0));
  EXPECT_EQ(206  , message.GetRepeatedInt64 (F("repeated_sint64"  ), 0));
  EXPECT_EQ(207  , message.GetRepeatedUInt32(F("repeated_fixed32" ), 0));
  EXPECT_EQ(208  , message.GetRepeatedUInt64(F("repeated_fixed64" ), 0));
  EXPECT_EQ(209  , message.GetRepeatedInt32 (F("repeated_sfixed32"), 0));
  EXPECT_EQ(210  , message.GetRepeatedInt64 (F("repeated_sfixed64"), 0));
  EXPECT_EQ(211  , message.GetRepeatedFloat (F("repeated_float"   ), 0));
  EXPECT_EQ(212  , message.GetRepeatedDouble(F("repeated_double"  ), 0));
  EXPECT_EQ(true , message.GetRepeatedBool  (F("repeated_bool"    ), 0));
  EXPECT_EQ("215", message.GetRepeatedString(F("repeated_string"  ), 0));
  EXPECT_EQ("216", message.GetRepeatedString(F("repeated_bytes"   ), 0));

  EXPECT_EQ("215", message.GetRepeatedStringReference(F("repeated_string"),
                                                      0, &scratch));
  EXPECT_EQ("216", message.GetRepeatedStringReference(F("repeated_bytes"),
                                                      0, &scratch));

  EXPECT_EQ(217, message.GetRepeatedMessage(F("repeatedgroup"), 0)
                        .GetReflection()->GetInt32(repeated_group_a_));
  EXPECT_EQ(218, message.GetRepeatedMessage(F("repeated_nested_message"), 0)
                        .GetReflection()->GetInt32(nested_b_));
  EXPECT_EQ(219, message.GetRepeatedMessage(F("repeated_foreign_message"), 0)
                        .GetReflection()->GetInt32(foreign_c_));
  EXPECT_EQ(220, message.GetRepeatedMessage(F("repeated_import_message"), 0)
                        .GetReflection()->GetInt32(import_d_));

  EXPECT_EQ( nested_bar_, message.GetRepeatedEnum(F("repeated_nested_enum" ),0));
  EXPECT_EQ(foreign_bar_, message.GetRepeatedEnum(F("repeated_foreign_enum"),0));
  EXPECT_EQ( import_bar_, message.GetRepeatedEnum(F("repeated_import_enum" ),0));

  EXPECT_EQ("224", message.GetRepeatedString(F("repeated_string_piece"), 0));
  EXPECT_EQ("224", message.GetRepeatedStringReference(
                        F("repeated_string_piece"), 0, &scratch));

  EXPECT_EQ("225", message.GetRepeatedString(F("repeated_cord"), 0));
  EXPECT_EQ("225", message.GetRepeatedStringReference(
                        F("repeated_cord"), 0, &scratch));

  EXPECT_EQ(301  , message.GetRepeatedInt32 (F("repeated_int32"   ), 1));
  EXPECT_EQ(302  , message.GetRepeatedInt64 (F("repeated_int64"   ), 1));
  EXPECT_EQ(303  , message.GetRepeatedUInt32(F("repeated_uint32"  ), 1));
  EXPECT_EQ(304  , message.GetRepeatedUInt64(F("repeated_uint64"  ), 1));
  EXPECT_EQ(305  , message.GetRepeatedInt32 (F("repeated_sint32"  ), 1));
  EXPECT_EQ(306  , message.GetRepeatedInt64 (F("repeated_sint64"  ), 1));
  EXPECT_EQ(307  , message.GetRepeatedUInt32(F("repeated_fixed32" ), 1));
  EXPECT_EQ(308  , message.GetRepeatedUInt64(F("repeated_fixed64" ), 1));
  EXPECT_EQ(309  , message.GetRepeatedInt32 (F("repeated_sfixed32"), 1));
  EXPECT_EQ(310  , message.GetRepeatedInt64 (F("repeated_sfixed64"), 1));
  EXPECT_EQ(311  , message.GetRepeatedFloat (F("repeated_float"   ), 1));
  EXPECT_EQ(312  , message.GetRepeatedDouble(F("repeated_double"  ), 1));
  EXPECT_EQ(false, message.GetRepeatedBool  (F("repeated_bool"    ), 1));
  EXPECT_EQ("315", message.GetRepeatedString(F("repeated_string"  ), 1));
  EXPECT_EQ("316", message.GetRepeatedString(F("repeated_bytes"   ), 1));

  EXPECT_EQ("315", message.GetRepeatedStringReference(F("repeated_string"),
                                                      1, &scratch));
  EXPECT_EQ("316", message.GetRepeatedStringReference(F("repeated_bytes"),
                                                      1, &scratch));

  EXPECT_EQ(317, message.GetRepeatedMessage(F("repeatedgroup"), 1)
                        .GetReflection()->GetInt32(repeated_group_a_));
  EXPECT_EQ(318, message.GetRepeatedMessage(F("repeated_nested_message"), 1)
                        .GetReflection()->GetInt32(nested_b_));
  EXPECT_EQ(319, message.GetRepeatedMessage(F("repeated_foreign_message"), 1)
                        .GetReflection()->GetInt32(foreign_c_));
  EXPECT_EQ(320, message.GetRepeatedMessage(F("repeated_import_message"), 1)
                        .GetReflection()->GetInt32(import_d_));

  EXPECT_EQ( nested_baz_, message.GetRepeatedEnum(F("repeated_nested_enum" ),1));
  EXPECT_EQ(foreign_baz_, message.GetRepeatedEnum(F("repeated_foreign_enum"),1));
  EXPECT_EQ( import_baz_, message.GetRepeatedEnum(F("repeated_import_enum" ),1));

  EXPECT_EQ("324", message.GetRepeatedString(F("repeated_string_piece"), 1));
  EXPECT_EQ("324", message.GetRepeatedStringReference(
                        F("repeated_string_piece"), 1, &scratch));

  EXPECT_EQ("325", message.GetRepeatedString(F("repeated_cord"), 1));
  EXPECT_EQ("325", message.GetRepeatedStringReference(
                        F("repeated_cord"), 1, &scratch));

  // -----------------------------------------------------------------

  EXPECT_TRUE(message.HasField(F("default_int32"   )));
  EXPECT_TRUE(message.HasField(F("default_int64"   )));
  EXPECT_TRUE(message.HasField(F("default_uint32"  )));
  EXPECT_TRUE(message.HasField(F("default_uint64"  )));
  EXPECT_TRUE(message.HasField(F("default_sint32"  )));
  EXPECT_TRUE(message.HasField(F("default_sint64"  )));
  EXPECT_TRUE(message.HasField(F("default_fixed32" )));
  EXPECT_TRUE(message.HasField(F("default_fixed64" )));
  EXPECT_TRUE(message.HasField(F("default_sfixed32")));
  EXPECT_TRUE(message.HasField(F("default_sfixed64")));
  EXPECT_TRUE(message.HasField(F("default_float"   )));
  EXPECT_TRUE(message.HasField(F("default_double"  )));
  EXPECT_TRUE(message.HasField(F("default_bool"    )));
  EXPECT_TRUE(message.HasField(F("default_string"  )));
  EXPECT_TRUE(message.HasField(F("default_bytes"   )));

  EXPECT_TRUE(message.HasField(F("default_nested_enum" )));
  EXPECT_TRUE(message.HasField(F("default_foreign_enum")));
  EXPECT_TRUE(message.HasField(F("default_import_enum" )));

  EXPECT_TRUE(message.HasField(F("default_string_piece")));
  EXPECT_TRUE(message.HasField(F("default_cord")));

  EXPECT_EQ(401  , message.GetInt32 (F("default_int32"   )));
  EXPECT_EQ(402  , message.GetInt64 (F("default_int64"   )));
  EXPECT_EQ(403  , message.GetUInt32(F("default_uint32"  )));
  EXPECT_EQ(404  , message.GetUInt64(F("default_uint64"  )));
  EXPECT_EQ(405  , message.GetInt32 (F("default_sint32"  )));
  EXPECT_EQ(406  , message.GetInt64 (F("default_sint64"  )));
  EXPECT_EQ(407  , message.GetUInt32(F("default_fixed32" )));
  EXPECT_EQ(408  , message.GetUInt64(F("default_fixed64" )));
  EXPECT_EQ(409  , message.GetInt32 (F("default_sfixed32")));
  EXPECT_EQ(410  , message.GetInt64 (F("default_sfixed64")));
  EXPECT_EQ(411  , message.GetFloat (F("default_float"   )));
  EXPECT_EQ(412  , message.GetDouble(F("default_double"  )));
  EXPECT_EQ(false, message.GetBool  (F("default_bool"    )));
  EXPECT_EQ("415", message.GetString(F("default_string"  )));
  EXPECT_EQ("416", message.GetString(F("default_bytes"   )));

  EXPECT_EQ("415", message.GetStringReference(F("default_string"), &scratch));
  EXPECT_EQ("416", message.GetStringReference(F("default_bytes" ), &scratch));

  EXPECT_EQ( nested_foo_, message.GetEnum(F("default_nested_enum" )));
  EXPECT_EQ(foreign_foo_, message.GetEnum(F("default_foreign_enum")));
  EXPECT_EQ( import_foo_, message.GetEnum(F("default_import_enum" )));

  EXPECT_EQ("424", message.GetString(F("default_string_piece")));
  EXPECT_EQ("424", message.GetStringReference(F("default_string_piece"),
                                              &scratch));

  EXPECT_EQ("425", message.GetString(F("default_cord")));
  EXPECT_EQ("425", message.GetStringReference(F("default_cord"), &scratch));
}

// -------------------------------------------------------------------

void TestUtil::ReflectionTester::ExpectClearViaReflection(
    const Message::Reflection& message) {
  string scratch;

  // has_blah() should initially be false for all optional fields.
  EXPECT_FALSE(message.HasField(F("optional_int32"   )));
  EXPECT_FALSE(message.HasField(F("optional_int64"   )));
  EXPECT_FALSE(message.HasField(F("optional_uint32"  )));
  EXPECT_FALSE(message.HasField(F("optional_uint64"  )));
  EXPECT_FALSE(message.HasField(F("optional_sint32"  )));
  EXPECT_FALSE(message.HasField(F("optional_sint64"  )));
  EXPECT_FALSE(message.HasField(F("optional_fixed32" )));
  EXPECT_FALSE(message.HasField(F("optional_fixed64" )));
  EXPECT_FALSE(message.HasField(F("optional_sfixed32")));
  EXPECT_FALSE(message.HasField(F("optional_sfixed64")));
  EXPECT_FALSE(message.HasField(F("optional_float"   )));
  EXPECT_FALSE(message.HasField(F("optional_double"  )));
  EXPECT_FALSE(message.HasField(F("optional_bool"    )));
  EXPECT_FALSE(message.HasField(F("optional_string"  )));
  EXPECT_FALSE(message.HasField(F("optional_bytes"   )));

  EXPECT_FALSE(message.HasField(F("optionalgroup"           )));
  EXPECT_FALSE(message.HasField(F("optional_nested_message" )));
  EXPECT_FALSE(message.HasField(F("optional_foreign_message")));
  EXPECT_FALSE(message.HasField(F("optional_import_message" )));

  EXPECT_FALSE(message.HasField(F("optional_nested_enum" )));
  EXPECT_FALSE(message.HasField(F("optional_foreign_enum")));
  EXPECT_FALSE(message.HasField(F("optional_import_enum" )));

  EXPECT_FALSE(message.HasField(F("optional_string_piece")));
  EXPECT_FALSE(message.HasField(F("optional_cord")));

  // Optional fields without defaults are set to zero or something like it.
  EXPECT_EQ(0    , message.GetInt32 (F("optional_int32"   )));
  EXPECT_EQ(0    , message.GetInt64 (F("optional_int64"   )));
  EXPECT_EQ(0    , message.GetUInt32(F("optional_uint32"  )));
  EXPECT_EQ(0    , message.GetUInt64(F("optional_uint64"  )));
  EXPECT_EQ(0    , message.GetInt32 (F("optional_sint32"  )));
  EXPECT_EQ(0    , message.GetInt64 (F("optional_sint64"  )));
  EXPECT_EQ(0    , message.GetUInt32(F("optional_fixed32" )));
  EXPECT_EQ(0    , message.GetUInt64(F("optional_fixed64" )));
  EXPECT_EQ(0    , message.GetInt32 (F("optional_sfixed32")));
  EXPECT_EQ(0    , message.GetInt64 (F("optional_sfixed64")));
  EXPECT_EQ(0    , message.GetFloat (F("optional_float"   )));
  EXPECT_EQ(0    , message.GetDouble(F("optional_double"  )));
  EXPECT_EQ(false, message.GetBool  (F("optional_bool"    )));
  EXPECT_EQ(""   , message.GetString(F("optional_string"  )));
  EXPECT_EQ(""   , message.GetString(F("optional_bytes"   )));

  EXPECT_EQ("", message.GetStringReference(F("optional_string"), &scratch));
  EXPECT_EQ("", message.GetStringReference(F("optional_bytes" ), &scratch));

  // Embedded messages should also be clear.
  EXPECT_FALSE(message.GetMessage(F("optionalgroup"))
                      .GetReflection()->HasField(group_a_));
  EXPECT_FALSE(message.GetMessage(F("optional_nested_message"))
                      .GetReflection()->HasField(nested_b_));
  EXPECT_FALSE(message.GetMessage(F("optional_foreign_message"))
                      .GetReflection()->HasField(foreign_c_));
  EXPECT_FALSE(message.GetMessage(F("optional_import_message"))
                      .GetReflection()->HasField(import_d_));

  EXPECT_EQ(0, message.GetMessage(F("optionalgroup"))
                      .GetReflection()->GetInt32(group_a_));
  EXPECT_EQ(0, message.GetMessage(F("optional_nested_message"))
                      .GetReflection()->GetInt32(nested_b_));
  EXPECT_EQ(0, message.GetMessage(F("optional_foreign_message"))
                      .GetReflection()->GetInt32(foreign_c_));
  EXPECT_EQ(0, message.GetMessage(F("optional_import_message"))
                      .GetReflection()->GetInt32(import_d_));

  // Enums without defaults are set to the first value in the enum.
  EXPECT_EQ( nested_foo_, message.GetEnum(F("optional_nested_enum" )));
  EXPECT_EQ(foreign_foo_, message.GetEnum(F("optional_foreign_enum")));
  EXPECT_EQ( import_foo_, message.GetEnum(F("optional_import_enum" )));

  EXPECT_EQ("", message.GetString(F("optional_string_piece")));
  EXPECT_EQ("", message.GetStringReference(F("optional_string_piece"),
                                           &scratch));

  EXPECT_EQ("", message.GetString(F("optional_cord")));
  EXPECT_EQ("", message.GetStringReference(F("optional_cord"), &scratch));

  // Repeated fields are empty.
  EXPECT_EQ(0, message.FieldSize(F("repeated_int32"   )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_int64"   )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_uint32"  )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_uint64"  )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_sint32"  )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_sint64"  )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_fixed32" )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_fixed64" )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_sfixed32")));
  EXPECT_EQ(0, message.FieldSize(F("repeated_sfixed64")));
  EXPECT_EQ(0, message.FieldSize(F("repeated_float"   )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_double"  )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_bool"    )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_string"  )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_bytes"   )));

  EXPECT_EQ(0, message.FieldSize(F("repeatedgroup"           )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_nested_message" )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_foreign_message")));
  EXPECT_EQ(0, message.FieldSize(F("repeated_import_message" )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_nested_enum"    )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_foreign_enum"   )));
  EXPECT_EQ(0, message.FieldSize(F("repeated_import_enum"    )));

  EXPECT_EQ(0, message.FieldSize(F("repeated_string_piece")));
  EXPECT_EQ(0, message.FieldSize(F("repeated_cord")));

  // has_blah() should also be false for all default fields.
  EXPECT_FALSE(message.HasField(F("default_int32"   )));
  EXPECT_FALSE(message.HasField(F("default_int64"   )));
  EXPECT_FALSE(message.HasField(F("default_uint32"  )));
  EXPECT_FALSE(message.HasField(F("default_uint64"  )));
  EXPECT_FALSE(message.HasField(F("default_sint32"  )));
  EXPECT_FALSE(message.HasField(F("default_sint64"  )));
  EXPECT_FALSE(message.HasField(F("default_fixed32" )));
  EXPECT_FALSE(message.HasField(F("default_fixed64" )));
  EXPECT_FALSE(message.HasField(F("default_sfixed32")));
  EXPECT_FALSE(message.HasField(F("default_sfixed64")));
  EXPECT_FALSE(message.HasField(F("default_float"   )));
  EXPECT_FALSE(message.HasField(F("default_double"  )));
  EXPECT_FALSE(message.HasField(F("default_bool"    )));
  EXPECT_FALSE(message.HasField(F("default_string"  )));
  EXPECT_FALSE(message.HasField(F("default_bytes"   )));

  EXPECT_FALSE(message.HasField(F("default_nested_enum" )));
  EXPECT_FALSE(message.HasField(F("default_foreign_enum")));
  EXPECT_FALSE(message.HasField(F("default_import_enum" )));

  EXPECT_FALSE(message.HasField(F("default_string_piece")));
  EXPECT_FALSE(message.HasField(F("default_cord")));

  // Fields with defaults have their default values (duh).
  EXPECT_EQ( 41    , message.GetInt32 (F("default_int32"   )));
  EXPECT_EQ( 42    , message.GetInt64 (F("default_int64"   )));
  EXPECT_EQ( 43    , message.GetUInt32(F("default_uint32"  )));
  EXPECT_EQ( 44    , message.GetUInt64(F("default_uint64"  )));
  EXPECT_EQ(-45    , message.GetInt32 (F("default_sint32"  )));
  EXPECT_EQ( 46    , message.GetInt64 (F("default_sint64"  )));
  EXPECT_EQ( 47    , message.GetUInt32(F("default_fixed32" )));
  EXPECT_EQ( 48    , message.GetUInt64(F("default_fixed64" )));
  EXPECT_EQ( 49    , message.GetInt32 (F("default_sfixed32")));
  EXPECT_EQ(-50    , message.GetInt64 (F("default_sfixed64")));
  EXPECT_EQ( 51.5  , message.GetFloat (F("default_float"   )));
  EXPECT_EQ( 52e3  , message.GetDouble(F("default_double"  )));
  EXPECT_EQ(true   , message.GetBool  (F("default_bool"    )));
  EXPECT_EQ("hello", message.GetString(F("default_string"  )));
  EXPECT_EQ("world", message.GetString(F("default_bytes"   )));

  EXPECT_EQ("hello", message.GetStringReference(F("default_string"), &scratch));
  EXPECT_EQ("world", message.GetStringReference(F("default_bytes" ), &scratch));

  EXPECT_EQ( nested_bar_, message.GetEnum(F("default_nested_enum" )));
  EXPECT_EQ(foreign_bar_, message.GetEnum(F("default_foreign_enum")));
  EXPECT_EQ( import_bar_, message.GetEnum(F("default_import_enum" )));

  EXPECT_EQ("abc", message.GetString(F("default_string_piece")));
  EXPECT_EQ("abc", message.GetStringReference(F("default_string_piece"),
                                              &scratch));

  EXPECT_EQ("123", message.GetString(F("default_cord")));
  EXPECT_EQ("123", message.GetStringReference(F("default_cord"), &scratch));
}

// -------------------------------------------------------------------

void TestUtil::ReflectionTester::ModifyRepeatedFieldsViaReflection(
    Message::Reflection* message) {
  message->SetRepeatedInt32 (F("repeated_int32"   ), 1, 501);
  message->SetRepeatedInt64 (F("repeated_int64"   ), 1, 502);
  message->SetRepeatedUInt32(F("repeated_uint32"  ), 1, 503);
  message->SetRepeatedUInt64(F("repeated_uint64"  ), 1, 504);
  message->SetRepeatedInt32 (F("repeated_sint32"  ), 1, 505);
  message->SetRepeatedInt64 (F("repeated_sint64"  ), 1, 506);
  message->SetRepeatedUInt32(F("repeated_fixed32" ), 1, 507);
  message->SetRepeatedUInt64(F("repeated_fixed64" ), 1, 508);
  message->SetRepeatedInt32 (F("repeated_sfixed32"), 1, 509);
  message->SetRepeatedInt64 (F("repeated_sfixed64"), 1, 510);
  message->SetRepeatedFloat (F("repeated_float"   ), 1, 511);
  message->SetRepeatedDouble(F("repeated_double"  ), 1, 512);
  message->SetRepeatedBool  (F("repeated_bool"    ), 1, true);
  message->SetRepeatedString(F("repeated_string"  ), 1, "515");
  message->SetRepeatedString(F("repeated_bytes"   ), 1, "516");

  message->MutableRepeatedMessage(F("repeatedgroup"), 1)
         ->GetReflection()->SetInt32(repeated_group_a_, 517);
  message->MutableRepeatedMessage(F("repeated_nested_message"), 1)
         ->GetReflection()->SetInt32(nested_b_, 518);
  message->MutableRepeatedMessage(F("repeated_foreign_message"), 1)
         ->GetReflection()->SetInt32(foreign_c_, 519);
  message->MutableRepeatedMessage(F("repeated_import_message"), 1)
         ->GetReflection()->SetInt32(import_d_, 520);

  message->SetRepeatedEnum(F("repeated_nested_enum" ), 1,  nested_foo_);
  message->SetRepeatedEnum(F("repeated_foreign_enum"), 1, foreign_foo_);
  message->SetRepeatedEnum(F("repeated_import_enum" ), 1,  import_foo_);

  message->SetRepeatedString(F("repeated_string_piece"), 1, "524");
  message->SetRepeatedString(F("repeated_cord"), 1, "525");
}

}  // namespace protobuf
}  // namespace google
