// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_TEST_UTIL_H__
#define GOOGLE_PROTOBUF_TEST_UTIL_H__

#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_import.pb.h"
#include "google/protobuf/unittest_mset.pb.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
// This file doesn't use these declarations, but some .cc files do.
namespace unittest = ::proto2_unittest;
namespace unittest_import = ::proto2_unittest_import;

namespace TestUtil {

class ReflectionTester {
 public:
  // base_descriptor must be a descriptor for TestAllTypes or
  // TestAllExtensions.  In the former case, ReflectionTester fetches from
  // it the FieldDescriptors needed to use the reflection interface.  In
  // the latter case, ReflectionTester searches for extension fields in
  // its file.
  explicit ReflectionTester(const Descriptor* base_descriptor);
  ReflectionTester(const ReflectionTester&) = delete;
  ReflectionTester& operator=(const ReflectionTester&) = delete;

  void SetAllFieldsViaReflection(Message* message);
  void ModifyRepeatedFieldsViaReflection(Message* message);
  void ExpectAllFieldsSetViaReflection(const Message& message);
  void ExpectClearViaReflection(const Message& message);

  void SetPackedFieldsViaReflection(Message* message);
  void ExpectPackedFieldsSetViaReflection(const Message& message);

  void RemoveLastRepeatedsViaReflection(Message* message);
  void ReleaseLastRepeatedsViaReflection(Message* message,
                                         bool expect_extensions_notnull);
  void SwapRepeatedsViaReflection(Message* message);
  void SetAllocatedOptionalMessageFieldsToNullViaReflection(Message* message);
  static void SetAllocatedOptionalMessageFieldsToMessageViaReflection(
      Message* from_message, Message* to_message);

  enum MessageReleaseState {
    IS_NULL,
    CAN_BE_NULL,
    NOT_NULL,
  };
  void ExpectMessagesReleasedViaReflection(
      Message* message, MessageReleaseState expected_release_state);

  // Set and check functions for TestOneof2 messages. No need to construct
  // the ReflectionTester by TestAllTypes nor TestAllExtensions.
  static void SetOneofViaReflection(Message* message);
  static void ExpectOneofSetViaReflection(const Message& message);

 private:
  const FieldDescriptor* F(const std::string& name);

  const Descriptor* base_descriptor_;

  const FieldDescriptor* group_a_;
  const FieldDescriptor* repeated_group_a_;
  const FieldDescriptor* nested_b_;
  const FieldDescriptor* foreign_c_;
  const FieldDescriptor* import_d_;
  const FieldDescriptor* import_e_;

  const EnumValueDescriptor* nested_foo_;
  const EnumValueDescriptor* nested_bar_;
  const EnumValueDescriptor* nested_baz_;
  const EnumValueDescriptor* foreign_foo_;
  const EnumValueDescriptor* foreign_bar_;
  const EnumValueDescriptor* foreign_baz_;
  const EnumValueDescriptor* import_foo_;
  const EnumValueDescriptor* import_bar_;
  const EnumValueDescriptor* import_baz_;

  // We have to split this into three function otherwise it creates a stack
  // frame so large that it triggers a warning.
  void ExpectAllFieldsSetViaReflection1(const Message& message);
  void ExpectAllFieldsSetViaReflection2(const Message& message);
  void ExpectAllFieldsSetViaReflection3(const Message& message);
};

inline TestUtil::ReflectionTester::ReflectionTester(
    const Descriptor* base_descriptor)
    : base_descriptor_(base_descriptor) {
  const DescriptorPool* pool = base_descriptor->file()->pool();
  const absl::string_view package = base_descriptor->file()->package();
  const FieldDescriptor* import_descriptor = pool->FindFieldByName(
      absl::StrCat(package, ".TestAllTypes.optional_import_message"));
  const absl::string_view import_package =
      import_descriptor->message_type()->file()->package();

  nested_b_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestAllTypes.NestedMessage.bb"));
  foreign_c_ =
      pool->FindFieldByName(absl::StrCat(package, ".ForeignMessage.c"));
  import_d_ =
      pool->FindFieldByName(absl::StrCat(import_package, ".ImportMessage.d"));
  import_e_ = pool->FindFieldByName(
      absl::StrCat(import_package, ".PublicImportMessage.e"));
  nested_foo_ =
      pool->FindEnumValueByName(absl::StrCat(package, ".TestAllTypes.FOO"));
  nested_bar_ =
      pool->FindEnumValueByName(absl::StrCat(package, ".TestAllTypes.BAR"));
  nested_baz_ =
      pool->FindEnumValueByName(absl::StrCat(package, ".TestAllTypes.BAZ"));
  foreign_foo_ =
      pool->FindEnumValueByName(absl::StrCat(package, ".FOREIGN_FOO"));
  foreign_bar_ =
      pool->FindEnumValueByName(absl::StrCat(package, ".FOREIGN_BAR"));
  foreign_baz_ =
      pool->FindEnumValueByName(absl::StrCat(package, ".FOREIGN_BAZ"));
  import_foo_ =
      pool->FindEnumValueByName(absl::StrCat(import_package, ".IMPORT_FOO"));
  import_bar_ =
      pool->FindEnumValueByName(absl::StrCat(import_package, ".IMPORT_BAR"));
  import_baz_ =
      pool->FindEnumValueByName(absl::StrCat(import_package, ".IMPORT_BAZ"));

  if (base_descriptor_->name() == "TestAllExtensions") {
    group_a_ = pool->FindFieldByName(
        absl::StrCat(package, ".OptionalGroup_extension.a"));
    repeated_group_a_ = pool->FindFieldByName(
        absl::StrCat(package, ".RepeatedGroup_extension.a"));
  } else {
    group_a_ = pool->FindFieldByName(
        absl::StrCat(package, ".TestAllTypes.OptionalGroup.a"));
    repeated_group_a_ = pool->FindFieldByName(
        absl::StrCat(package, ".TestAllTypes.RepeatedGroup.a"));
  }

  EXPECT_TRUE(group_a_ != nullptr);
  EXPECT_TRUE(repeated_group_a_ != nullptr);
  EXPECT_TRUE(nested_b_ != nullptr);
  EXPECT_TRUE(foreign_c_ != nullptr);
  EXPECT_TRUE(import_d_ != nullptr);
  EXPECT_TRUE(import_e_ != nullptr);
  EXPECT_TRUE(nested_foo_ != nullptr);
  EXPECT_TRUE(nested_bar_ != nullptr);
  EXPECT_TRUE(nested_baz_ != nullptr);
  EXPECT_TRUE(foreign_foo_ != nullptr);
  EXPECT_TRUE(foreign_bar_ != nullptr);
  EXPECT_TRUE(foreign_baz_ != nullptr);
  EXPECT_TRUE(import_foo_ != nullptr);
  EXPECT_TRUE(import_bar_ != nullptr);
  EXPECT_TRUE(import_baz_ != nullptr);
}

// Shorthand to get a FieldDescriptor for a field of TestAllTypes.
inline const FieldDescriptor* TestUtil::ReflectionTester::F(
    const std::string& name) {
  const FieldDescriptor* result = nullptr;
  if (base_descriptor_->name() == "TestAllExtensions" ||
      base_descriptor_->name() == "TestPackedExtensions") {
    result = base_descriptor_->file()->FindExtensionByName(
        absl::StrCat(name, "_extension"));
  } else {
    result = base_descriptor_->FindFieldByName(name);
  }
  ABSL_CHECK(result != nullptr);
  return result;
}

// -------------------------------------------------------------------

inline void TestUtil::ReflectionTester::SetAllFieldsViaReflection(
    Message* message) {
  const Reflection* reflection = message->GetReflection();
  Message* sub_message;

  reflection->SetInt32(message, F("optional_int32"), 101);
  reflection->SetInt64(message, F("optional_int64"), 102);
  reflection->SetUInt32(message, F("optional_uint32"), 103);
  reflection->SetUInt64(message, F("optional_uint64"), 104);
  reflection->SetInt32(message, F("optional_sint32"), 105);
  reflection->SetInt64(message, F("optional_sint64"), 106);
  reflection->SetUInt32(message, F("optional_fixed32"), 107);
  reflection->SetUInt64(message, F("optional_fixed64"), 108);
  reflection->SetInt32(message, F("optional_sfixed32"), 109);
  reflection->SetInt64(message, F("optional_sfixed64"), 110);
  reflection->SetFloat(message, F("optional_float"), 111);
  reflection->SetDouble(message, F("optional_double"), 112);
  reflection->SetBool(message, F("optional_bool"), true);
  reflection->SetString(message, F("optional_string"), "115");
  reflection->SetString(message, F("optional_bytes"), "116");

  sub_message = reflection->MutableMessage(message, F("optionalgroup"));
  sub_message->GetReflection()->SetInt32(sub_message, group_a_, 117);
  sub_message =
      reflection->MutableMessage(message, F("optional_nested_message"));
  sub_message->GetReflection()->SetInt32(sub_message, nested_b_, 118);
  sub_message =
      reflection->MutableMessage(message, F("optional_foreign_message"));
  sub_message->GetReflection()->SetInt32(sub_message, foreign_c_, 119);
  sub_message =
      reflection->MutableMessage(message, F("optional_import_message"));
  sub_message->GetReflection()->SetInt32(sub_message, import_d_, 120);

  reflection->SetEnum(message, F("optional_nested_enum"), nested_baz_);
  reflection->SetEnum(message, F("optional_foreign_enum"), foreign_baz_);
  reflection->SetEnum(message, F("optional_import_enum"), import_baz_);

  reflection->SetString(message, F("optional_string_piece"), "124");
  reflection->SetString(message, F("optional_cord"), "125");
  reflection->SetString(message, F("optional_bytes_cord"),
                        "optional bytes cord");

  sub_message =
      reflection->MutableMessage(message, F("optional_public_import_message"));
  sub_message->GetReflection()->SetInt32(sub_message, import_e_, 126);

  sub_message = reflection->MutableMessage(message, F("optional_lazy_message"));
  sub_message->GetReflection()->SetInt32(sub_message, nested_b_, 127);

  sub_message = reflection->MutableMessage(
      message, F("optional_unverified_lazy_message"));
  sub_message->GetReflection()->SetInt32(sub_message, nested_b_, 128);

  // -----------------------------------------------------------------

  reflection->AddInt32(message, F("repeated_int32"), 201);
  reflection->AddInt64(message, F("repeated_int64"), 202);
  reflection->AddUInt32(message, F("repeated_uint32"), 203);
  reflection->AddUInt64(message, F("repeated_uint64"), 204);
  reflection->AddInt32(message, F("repeated_sint32"), 205);
  reflection->AddInt64(message, F("repeated_sint64"), 206);
  reflection->AddUInt32(message, F("repeated_fixed32"), 207);
  reflection->AddUInt64(message, F("repeated_fixed64"), 208);
  reflection->AddInt32(message, F("repeated_sfixed32"), 209);
  reflection->AddInt64(message, F("repeated_sfixed64"), 210);
  reflection->AddFloat(message, F("repeated_float"), 211);
  reflection->AddDouble(message, F("repeated_double"), 212);
  reflection->AddBool(message, F("repeated_bool"), true);
  reflection->AddString(message, F("repeated_string"), "215");
  reflection->AddString(message, F("repeated_bytes"), "216");

  sub_message = reflection->AddMessage(message, F("repeatedgroup"));
  sub_message->GetReflection()->SetInt32(sub_message, repeated_group_a_, 217);
  sub_message = reflection->AddMessage(message, F("repeated_nested_message"));
  sub_message->GetReflection()->SetInt32(sub_message, nested_b_, 218);
  sub_message = reflection->AddMessage(message, F("repeated_foreign_message"));
  sub_message->GetReflection()->SetInt32(sub_message, foreign_c_, 219);
  sub_message = reflection->AddMessage(message, F("repeated_import_message"));
  sub_message->GetReflection()->SetInt32(sub_message, import_d_, 220);
  sub_message = reflection->AddMessage(message, F("repeated_lazy_message"));
  sub_message->GetReflection()->SetInt32(sub_message, nested_b_, 227);

  reflection->AddEnum(message, F("repeated_nested_enum"), nested_bar_);
  reflection->AddEnum(message, F("repeated_foreign_enum"), foreign_bar_);
  reflection->AddEnum(message, F("repeated_import_enum"), import_bar_);

  reflection->AddString(message, F("repeated_string_piece"), "224");
  reflection->AddString(message, F("repeated_cord"), "225");

  // Add a second one of each field.
  reflection->AddInt32(message, F("repeated_int32"), 301);
  reflection->AddInt64(message, F("repeated_int64"), 302);
  reflection->AddUInt32(message, F("repeated_uint32"), 303);
  reflection->AddUInt64(message, F("repeated_uint64"), 304);
  reflection->AddInt32(message, F("repeated_sint32"), 305);
  reflection->AddInt64(message, F("repeated_sint64"), 306);
  reflection->AddUInt32(message, F("repeated_fixed32"), 307);
  reflection->AddUInt64(message, F("repeated_fixed64"), 308);
  reflection->AddInt32(message, F("repeated_sfixed32"), 309);
  reflection->AddInt64(message, F("repeated_sfixed64"), 310);
  reflection->AddFloat(message, F("repeated_float"), 311);
  reflection->AddDouble(message, F("repeated_double"), 312);
  reflection->AddBool(message, F("repeated_bool"), false);
  reflection->AddString(message, F("repeated_string"), "315");
  reflection->AddString(message, F("repeated_bytes"), "316");

  sub_message = reflection->AddMessage(message, F("repeatedgroup"));
  sub_message->GetReflection()->SetInt32(sub_message, repeated_group_a_, 317);
  sub_message = reflection->AddMessage(message, F("repeated_nested_message"));
  sub_message->GetReflection()->SetInt32(sub_message, nested_b_, 318);
  sub_message = reflection->AddMessage(message, F("repeated_foreign_message"));
  sub_message->GetReflection()->SetInt32(sub_message, foreign_c_, 319);
  sub_message = reflection->AddMessage(message, F("repeated_import_message"));
  sub_message->GetReflection()->SetInt32(sub_message, import_d_, 320);
  sub_message = reflection->AddMessage(message, F("repeated_lazy_message"));
  sub_message->GetReflection()->SetInt32(sub_message, nested_b_, 327);

  reflection->AddEnum(message, F("repeated_nested_enum"), nested_baz_);
  reflection->AddEnum(message, F("repeated_foreign_enum"), foreign_baz_);
  reflection->AddEnum(message, F("repeated_import_enum"), import_baz_);

  reflection->AddString(message, F("repeated_string_piece"), "324");
  reflection->AddString(message, F("repeated_cord"), "325");

  // -----------------------------------------------------------------

  reflection->SetInt32(message, F("default_int32"), 401);
  reflection->SetInt64(message, F("default_int64"), 402);
  reflection->SetUInt32(message, F("default_uint32"), 403);
  reflection->SetUInt64(message, F("default_uint64"), 404);
  reflection->SetInt32(message, F("default_sint32"), 405);
  reflection->SetInt64(message, F("default_sint64"), 406);
  reflection->SetUInt32(message, F("default_fixed32"), 407);
  reflection->SetUInt64(message, F("default_fixed64"), 408);
  reflection->SetInt32(message, F("default_sfixed32"), 409);
  reflection->SetInt64(message, F("default_sfixed64"), 410);
  reflection->SetFloat(message, F("default_float"), 411);
  reflection->SetDouble(message, F("default_double"), 412);
  reflection->SetBool(message, F("default_bool"), false);
  reflection->SetString(message, F("default_string"), "415");
  reflection->SetString(message, F("default_bytes"), "416");

  reflection->SetEnum(message, F("default_nested_enum"), nested_foo_);
  reflection->SetEnum(message, F("default_foreign_enum"), foreign_foo_);
  reflection->SetEnum(message, F("default_import_enum"), import_foo_);

  reflection->SetString(message, F("default_string_piece"), "424");
  reflection->SetString(message, F("default_cord"), "425");

  reflection->SetUInt32(message, F("oneof_uint32"), 601);
  sub_message = reflection->MutableMessage(message, F("oneof_nested_message"));
  sub_message->GetReflection()->SetInt32(sub_message, nested_b_, 602);
  reflection->SetString(message, F("oneof_string"), "603");
  reflection->SetString(message, F("oneof_bytes"), "604");
}

inline void TestUtil::ReflectionTester::SetOneofViaReflection(
    Message* message) {
  const Descriptor* descriptor = message->GetDescriptor();
  const Reflection* reflection = message->GetReflection();
  Message* sub_message = reflection->MutableMessage(
      message, descriptor->FindFieldByName("foo_lazy_message"));
  sub_message->GetReflection()->SetInt64(
      sub_message, sub_message->GetDescriptor()->FindFieldByName("moo_int"),
      100);

  reflection->SetString(message, descriptor->FindFieldByName("bar_cord"),
                        "101");
  reflection->SetInt32(message, descriptor->FindFieldByName("baz_int"), 102);
  reflection->SetString(message, descriptor->FindFieldByName("baz_string"),
                        "103");
}

inline void TestUtil::ReflectionTester::ExpectOneofSetViaReflection(
    const Message& message) {
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* reflection = message.GetReflection();
  std::string scratch;
  EXPECT_TRUE(reflection->HasField(
      message, descriptor->FindFieldByName("foo_lazy_message")));
  EXPECT_TRUE(
      reflection->HasField(message, descriptor->FindFieldByName("bar_cord")));
  EXPECT_TRUE(
      reflection->HasField(message, descriptor->FindFieldByName("baz_int")));
  EXPECT_TRUE(
      reflection->HasField(message, descriptor->FindFieldByName("baz_string")));

  const Message* sub_message = &reflection->GetMessage(
      message, descriptor->FindFieldByName("foo_lazy_message"));
  EXPECT_EQ(100, sub_message->GetReflection()->GetInt64(
                     *sub_message,
                     sub_message->GetDescriptor()->FindFieldByName("moo_int")));

  EXPECT_EQ("101", reflection->GetString(
                       message, descriptor->FindFieldByName("bar_cord")));
  EXPECT_EQ("101",
            reflection->GetStringReference(
                message, descriptor->FindFieldByName("bar_cord"), &scratch));

  EXPECT_EQ(102, reflection->GetInt32(message,
                                      descriptor->FindFieldByName("baz_int")));

  EXPECT_EQ("103", reflection->GetString(
                       message, descriptor->FindFieldByName("baz_string")));
  EXPECT_EQ("103",
            reflection->GetStringReference(
                message, descriptor->FindFieldByName("baz_string"), &scratch));
}

inline void TestUtil::ReflectionTester::SetPackedFieldsViaReflection(
    Message* message) {
  const Reflection* reflection = message->GetReflection();
  reflection->AddInt32(message, F("packed_int32"), 601);
  reflection->AddInt64(message, F("packed_int64"), 602);
  reflection->AddUInt32(message, F("packed_uint32"), 603);
  reflection->AddUInt64(message, F("packed_uint64"), 604);
  reflection->AddInt32(message, F("packed_sint32"), 605);
  reflection->AddInt64(message, F("packed_sint64"), 606);
  reflection->AddUInt32(message, F("packed_fixed32"), 607);
  reflection->AddUInt64(message, F("packed_fixed64"), 608);
  reflection->AddInt32(message, F("packed_sfixed32"), 609);
  reflection->AddInt64(message, F("packed_sfixed64"), 610);
  reflection->AddFloat(message, F("packed_float"), 611);
  reflection->AddDouble(message, F("packed_double"), 612);
  reflection->AddBool(message, F("packed_bool"), true);
  reflection->AddEnum(message, F("packed_enum"), foreign_bar_);

  reflection->AddInt32(message, F("packed_int32"), 701);
  reflection->AddInt64(message, F("packed_int64"), 702);
  reflection->AddUInt32(message, F("packed_uint32"), 703);
  reflection->AddUInt64(message, F("packed_uint64"), 704);
  reflection->AddInt32(message, F("packed_sint32"), 705);
  reflection->AddInt64(message, F("packed_sint64"), 706);
  reflection->AddUInt32(message, F("packed_fixed32"), 707);
  reflection->AddUInt64(message, F("packed_fixed64"), 708);
  reflection->AddInt32(message, F("packed_sfixed32"), 709);
  reflection->AddInt64(message, F("packed_sfixed64"), 710);
  reflection->AddFloat(message, F("packed_float"), 711);
  reflection->AddDouble(message, F("packed_double"), 712);
  reflection->AddBool(message, F("packed_bool"), false);
  reflection->AddEnum(message, F("packed_enum"), foreign_baz_);
}

// -------------------------------------------------------------------

inline void TestUtil::ReflectionTester::ExpectAllFieldsSetViaReflection(
    const Message& message) {
  // We have to split this into three function otherwise it creates a stack
  // frame so large that it triggers a warning.
  ExpectAllFieldsSetViaReflection1(message);
  ExpectAllFieldsSetViaReflection2(message);
  ExpectAllFieldsSetViaReflection3(message);
}

inline void TestUtil::ReflectionTester::ExpectAllFieldsSetViaReflection1(
    const Message& message) {
  const Reflection* reflection = message.GetReflection();
  std::string scratch;
  const Message* sub_message;

  EXPECT_TRUE(reflection->HasField(message, F("optional_int32")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_int64")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_uint32")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_uint64")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_sint32")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_sint64")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_fixed32")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_fixed64")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_sfixed32")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_sfixed64")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_float")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_double")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_bool")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_string")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_bytes")));

  EXPECT_TRUE(reflection->HasField(message, F("optionalgroup")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_nested_message")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_foreign_message")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_import_message")));
  EXPECT_TRUE(
      reflection->HasField(message, F("optional_public_import_message")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_lazy_message")));
  EXPECT_TRUE(
      reflection->HasField(message, F("optional_unverified_lazy_message")));

  sub_message = &reflection->GetMessage(message, F("optionalgroup"));
  EXPECT_TRUE(sub_message->GetReflection()->HasField(*sub_message, group_a_));
  sub_message = &reflection->GetMessage(message, F("optional_nested_message"));
  EXPECT_TRUE(sub_message->GetReflection()->HasField(*sub_message, nested_b_));
  sub_message = &reflection->GetMessage(message, F("optional_foreign_message"));
  EXPECT_TRUE(sub_message->GetReflection()->HasField(*sub_message, foreign_c_));
  sub_message = &reflection->GetMessage(message, F("optional_import_message"));
  EXPECT_TRUE(sub_message->GetReflection()->HasField(*sub_message, import_d_));
  sub_message =
      &reflection->GetMessage(message, F("optional_public_import_message"));
  EXPECT_TRUE(sub_message->GetReflection()->HasField(*sub_message, import_e_));
  sub_message = &reflection->GetMessage(message, F("optional_lazy_message"));
  EXPECT_TRUE(sub_message->GetReflection()->HasField(*sub_message, nested_b_));
  sub_message =
      &reflection->GetMessage(message, F("optional_unverified_lazy_message"));
  EXPECT_TRUE(sub_message->GetReflection()->HasField(*sub_message, nested_b_));

  EXPECT_TRUE(reflection->HasField(message, F("optional_nested_enum")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_foreign_enum")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_import_enum")));

  EXPECT_TRUE(reflection->HasField(message, F("optional_string_piece")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_cord")));
  EXPECT_TRUE(reflection->HasField(message, F("optional_bytes_cord")));

  EXPECT_EQ(101, reflection->GetInt32(message, F("optional_int32")));
  EXPECT_EQ(102, reflection->GetInt64(message, F("optional_int64")));
  EXPECT_EQ(103, reflection->GetUInt32(message, F("optional_uint32")));
  EXPECT_EQ(104, reflection->GetUInt64(message, F("optional_uint64")));
  EXPECT_EQ(105, reflection->GetInt32(message, F("optional_sint32")));
  EXPECT_EQ(106, reflection->GetInt64(message, F("optional_sint64")));
  EXPECT_EQ(107, reflection->GetUInt32(message, F("optional_fixed32")));
  EXPECT_EQ(108, reflection->GetUInt64(message, F("optional_fixed64")));
  EXPECT_EQ(109, reflection->GetInt32(message, F("optional_sfixed32")));
  EXPECT_EQ(110, reflection->GetInt64(message, F("optional_sfixed64")));
  EXPECT_EQ(111, reflection->GetFloat(message, F("optional_float")));
  EXPECT_EQ(112, reflection->GetDouble(message, F("optional_double")));
  EXPECT_TRUE(reflection->GetBool(message, F("optional_bool")));
  EXPECT_EQ("115", reflection->GetString(message, F("optional_string")));
  EXPECT_EQ("116", reflection->GetString(message, F("optional_bytes")));

  EXPECT_EQ("115", reflection->GetStringReference(message, F("optional_string"),
                                                  &scratch));
  EXPECT_EQ("116", reflection->GetStringReference(message, F("optional_bytes"),
                                                  &scratch));

  sub_message = &reflection->GetMessage(message, F("optionalgroup"));
  EXPECT_EQ(117,
            sub_message->GetReflection()->GetInt32(*sub_message, group_a_));
  sub_message = &reflection->GetMessage(message, F("optional_nested_message"));
  EXPECT_EQ(118,
            sub_message->GetReflection()->GetInt32(*sub_message, nested_b_));
  sub_message = &reflection->GetMessage(message, F("optional_foreign_message"));
  EXPECT_EQ(119,
            sub_message->GetReflection()->GetInt32(*sub_message, foreign_c_));
  sub_message = &reflection->GetMessage(message, F("optional_import_message"));
  EXPECT_EQ(120,
            sub_message->GetReflection()->GetInt32(*sub_message, import_d_));
  sub_message =
      &reflection->GetMessage(message, F("optional_public_import_message"));
  EXPECT_EQ(126,
            sub_message->GetReflection()->GetInt32(*sub_message, import_e_));
  sub_message = &reflection->GetMessage(message, F("optional_lazy_message"));
  EXPECT_EQ(127,
            sub_message->GetReflection()->GetInt32(*sub_message, nested_b_));
  sub_message =
      &reflection->GetMessage(message, F("optional_unverified_lazy_message"));
  EXPECT_EQ(128,
            sub_message->GetReflection()->GetInt32(*sub_message, nested_b_));

  EXPECT_EQ(nested_baz_,
            reflection->GetEnum(message, F("optional_nested_enum")));
  EXPECT_EQ(foreign_baz_,
            reflection->GetEnum(message, F("optional_foreign_enum")));
  EXPECT_EQ(import_baz_,
            reflection->GetEnum(message, F("optional_import_enum")));

  EXPECT_EQ("124", reflection->GetString(message, F("optional_string_piece")));
  EXPECT_EQ("124", reflection->GetStringReference(
                       message, F("optional_string_piece"), &scratch));

  EXPECT_EQ("125", reflection->GetString(message, F("optional_cord")));
  EXPECT_EQ("125", reflection->GetStringReference(message, F("optional_cord"),
                                                  &scratch));

  EXPECT_EQ("optional bytes cord",
            reflection->GetString(message, F("optional_bytes_cord")));
  EXPECT_EQ("optional bytes cord",
            reflection->GetStringReference(message, F("optional_bytes_cord"),
                                           &scratch));
  EXPECT_EQ("optional bytes cord",
            reflection->GetCord(message, F("optional_bytes_cord")));

  EXPECT_TRUE(reflection->HasField(message, F("oneof_bytes")));
  EXPECT_EQ("604", reflection->GetString(message, F("oneof_bytes")));

  if (base_descriptor_->name() == "TestAllTypes") {
    EXPECT_FALSE(reflection->HasField(message, F("oneof_uint32")));
    EXPECT_FALSE(reflection->HasField(message, F("oneof_string")));
  } else {
    EXPECT_TRUE(reflection->HasField(message, F("oneof_uint32")));
    EXPECT_TRUE(reflection->HasField(message, F("oneof_string")));
    EXPECT_EQ(601, reflection->GetUInt32(message, F("oneof_uint32")));
    EXPECT_EQ("603", reflection->GetString(message, F("oneof_string")));
    sub_message = &reflection->GetMessage(message, F("oneof_nested_message"));
    EXPECT_EQ(602,
              sub_message->GetReflection()->GetInt32(*sub_message, nested_b_));
  }
}

inline void TestUtil::ReflectionTester::ExpectAllFieldsSetViaReflection2(
    const Message& message) {
  const Reflection* reflection = message.GetReflection();
  std::string scratch;
  const Message* sub_message;

  // -----------------------------------------------------------------

  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_int32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_int64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_uint32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_uint64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_sint32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_sint64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_fixed32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_fixed64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_sfixed32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_sfixed64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_float")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_double")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_bool")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_string")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_bytes")));

  ASSERT_EQ(2, reflection->FieldSize(message, F("repeatedgroup")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_nested_message")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_foreign_message")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_import_message")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_lazy_message")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_nested_enum")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_foreign_enum")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_import_enum")));

  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_string_piece")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("repeated_cord")));

  EXPECT_EQ(201, reflection->GetRepeatedInt32(message, F("repeated_int32"), 0));
  EXPECT_EQ(202, reflection->GetRepeatedInt64(message, F("repeated_int64"), 0));
  EXPECT_EQ(203,
            reflection->GetRepeatedUInt32(message, F("repeated_uint32"), 0));
  EXPECT_EQ(204,
            reflection->GetRepeatedUInt64(message, F("repeated_uint64"), 0));
  EXPECT_EQ(205,
            reflection->GetRepeatedInt32(message, F("repeated_sint32"), 0));
  EXPECT_EQ(206,
            reflection->GetRepeatedInt64(message, F("repeated_sint64"), 0));
  EXPECT_EQ(207,
            reflection->GetRepeatedUInt32(message, F("repeated_fixed32"), 0));
  EXPECT_EQ(208,
            reflection->GetRepeatedUInt64(message, F("repeated_fixed64"), 0));
  EXPECT_EQ(209,
            reflection->GetRepeatedInt32(message, F("repeated_sfixed32"), 0));
  EXPECT_EQ(210,
            reflection->GetRepeatedInt64(message, F("repeated_sfixed64"), 0));
  EXPECT_EQ(211, reflection->GetRepeatedFloat(message, F("repeated_float"), 0));
  EXPECT_EQ(212,
            reflection->GetRepeatedDouble(message, F("repeated_double"), 0));
  EXPECT_TRUE(reflection->GetRepeatedBool(message, F("repeated_bool"), 0));
  EXPECT_EQ("215",
            reflection->GetRepeatedString(message, F("repeated_string"), 0));
  EXPECT_EQ("216",
            reflection->GetRepeatedString(message, F("repeated_bytes"), 0));

  EXPECT_EQ("215", reflection->GetRepeatedStringReference(
                       message, F("repeated_string"), 0, &scratch));
  EXPECT_EQ("216", reflection->GetRepeatedStringReference(
                       message, F("repeated_bytes"), 0, &scratch));

  sub_message = &reflection->GetRepeatedMessage(message, F("repeatedgroup"), 0);
  EXPECT_EQ(217, sub_message->GetReflection()->GetInt32(*sub_message,
                                                        repeated_group_a_));
  sub_message =
      &reflection->GetRepeatedMessage(message, F("repeated_nested_message"), 0);
  EXPECT_EQ(218,
            sub_message->GetReflection()->GetInt32(*sub_message, nested_b_));
  sub_message = &reflection->GetRepeatedMessage(
      message, F("repeated_foreign_message"), 0);
  EXPECT_EQ(219,
            sub_message->GetReflection()->GetInt32(*sub_message, foreign_c_));
  sub_message =
      &reflection->GetRepeatedMessage(message, F("repeated_import_message"), 0);
  EXPECT_EQ(220,
            sub_message->GetReflection()->GetInt32(*sub_message, import_d_));
  sub_message =
      &reflection->GetRepeatedMessage(message, F("repeated_lazy_message"), 0);
  EXPECT_EQ(227,
            sub_message->GetReflection()->GetInt32(*sub_message, nested_b_));

  EXPECT_EQ(nested_bar_,
            reflection->GetRepeatedEnum(message, F("repeated_nested_enum"), 0));
  EXPECT_EQ(foreign_bar_, reflection->GetRepeatedEnum(
                              message, F("repeated_foreign_enum"), 0));
  EXPECT_EQ(import_bar_,
            reflection->GetRepeatedEnum(message, F("repeated_import_enum"), 0));

  EXPECT_EQ("224", reflection->GetRepeatedString(
                       message, F("repeated_string_piece"), 0));
  EXPECT_EQ("224", reflection->GetRepeatedStringReference(
                       message, F("repeated_string_piece"), 0, &scratch));

  EXPECT_EQ("225",
            reflection->GetRepeatedString(message, F("repeated_cord"), 0));
  EXPECT_EQ("225", reflection->GetRepeatedStringReference(
                       message, F("repeated_cord"), 0, &scratch));

  EXPECT_EQ(301, reflection->GetRepeatedInt32(message, F("repeated_int32"), 1));
  EXPECT_EQ(302, reflection->GetRepeatedInt64(message, F("repeated_int64"), 1));
  EXPECT_EQ(303,
            reflection->GetRepeatedUInt32(message, F("repeated_uint32"), 1));
  EXPECT_EQ(304,
            reflection->GetRepeatedUInt64(message, F("repeated_uint64"), 1));
  EXPECT_EQ(305,
            reflection->GetRepeatedInt32(message, F("repeated_sint32"), 1));
  EXPECT_EQ(306,
            reflection->GetRepeatedInt64(message, F("repeated_sint64"), 1));
  EXPECT_EQ(307,
            reflection->GetRepeatedUInt32(message, F("repeated_fixed32"), 1));
  EXPECT_EQ(308,
            reflection->GetRepeatedUInt64(message, F("repeated_fixed64"), 1));
  EXPECT_EQ(309,
            reflection->GetRepeatedInt32(message, F("repeated_sfixed32"), 1));
  EXPECT_EQ(310,
            reflection->GetRepeatedInt64(message, F("repeated_sfixed64"), 1));
  EXPECT_EQ(311, reflection->GetRepeatedFloat(message, F("repeated_float"), 1));
  EXPECT_EQ(312,
            reflection->GetRepeatedDouble(message, F("repeated_double"), 1));
  EXPECT_FALSE(reflection->GetRepeatedBool(message, F("repeated_bool"), 1));
  EXPECT_EQ("315",
            reflection->GetRepeatedString(message, F("repeated_string"), 1));
  EXPECT_EQ("316",
            reflection->GetRepeatedString(message, F("repeated_bytes"), 1));

  EXPECT_EQ("315", reflection->GetRepeatedStringReference(
                       message, F("repeated_string"), 1, &scratch));
  EXPECT_EQ("316", reflection->GetRepeatedStringReference(
                       message, F("repeated_bytes"), 1, &scratch));

  sub_message = &reflection->GetRepeatedMessage(message, F("repeatedgroup"), 1);
  EXPECT_EQ(317, sub_message->GetReflection()->GetInt32(*sub_message,
                                                        repeated_group_a_));
  sub_message =
      &reflection->GetRepeatedMessage(message, F("repeated_nested_message"), 1);
  EXPECT_EQ(318,
            sub_message->GetReflection()->GetInt32(*sub_message, nested_b_));
  sub_message = &reflection->GetRepeatedMessage(
      message, F("repeated_foreign_message"), 1);
  EXPECT_EQ(319,
            sub_message->GetReflection()->GetInt32(*sub_message, foreign_c_));
  sub_message =
      &reflection->GetRepeatedMessage(message, F("repeated_import_message"), 1);
  EXPECT_EQ(320,
            sub_message->GetReflection()->GetInt32(*sub_message, import_d_));
  sub_message =
      &reflection->GetRepeatedMessage(message, F("repeated_lazy_message"), 1);
  EXPECT_EQ(327,
            sub_message->GetReflection()->GetInt32(*sub_message, nested_b_));

  EXPECT_EQ(nested_baz_,
            reflection->GetRepeatedEnum(message, F("repeated_nested_enum"), 1));
  EXPECT_EQ(foreign_baz_, reflection->GetRepeatedEnum(
                              message, F("repeated_foreign_enum"), 1));
  EXPECT_EQ(import_baz_,
            reflection->GetRepeatedEnum(message, F("repeated_import_enum"), 1));

  EXPECT_EQ("324", reflection->GetRepeatedString(
                       message, F("repeated_string_piece"), 1));
  EXPECT_EQ("324", reflection->GetRepeatedStringReference(
                       message, F("repeated_string_piece"), 1, &scratch));

  EXPECT_EQ("325",
            reflection->GetRepeatedString(message, F("repeated_cord"), 1));
  EXPECT_EQ("325", reflection->GetRepeatedStringReference(
                       message, F("repeated_cord"), 1, &scratch));
}

inline void TestUtil::ReflectionTester::ExpectAllFieldsSetViaReflection3(
    const Message& message) {
  const Reflection* reflection = message.GetReflection();
  std::string scratch;

  // -----------------------------------------------------------------

  EXPECT_TRUE(reflection->HasField(message, F("default_int32")));
  EXPECT_TRUE(reflection->HasField(message, F("default_int64")));
  EXPECT_TRUE(reflection->HasField(message, F("default_uint32")));
  EXPECT_TRUE(reflection->HasField(message, F("default_uint64")));
  EXPECT_TRUE(reflection->HasField(message, F("default_sint32")));
  EXPECT_TRUE(reflection->HasField(message, F("default_sint64")));
  EXPECT_TRUE(reflection->HasField(message, F("default_fixed32")));
  EXPECT_TRUE(reflection->HasField(message, F("default_fixed64")));
  EXPECT_TRUE(reflection->HasField(message, F("default_sfixed32")));
  EXPECT_TRUE(reflection->HasField(message, F("default_sfixed64")));
  EXPECT_TRUE(reflection->HasField(message, F("default_float")));
  EXPECT_TRUE(reflection->HasField(message, F("default_double")));
  EXPECT_TRUE(reflection->HasField(message, F("default_bool")));
  EXPECT_TRUE(reflection->HasField(message, F("default_string")));
  EXPECT_TRUE(reflection->HasField(message, F("default_bytes")));

  EXPECT_TRUE(reflection->HasField(message, F("default_nested_enum")));
  EXPECT_TRUE(reflection->HasField(message, F("default_foreign_enum")));
  EXPECT_TRUE(reflection->HasField(message, F("default_import_enum")));

  EXPECT_TRUE(reflection->HasField(message, F("default_string_piece")));
  EXPECT_TRUE(reflection->HasField(message, F("default_cord")));

  EXPECT_EQ(401, reflection->GetInt32(message, F("default_int32")));
  EXPECT_EQ(402, reflection->GetInt64(message, F("default_int64")));
  EXPECT_EQ(403, reflection->GetUInt32(message, F("default_uint32")));
  EXPECT_EQ(404, reflection->GetUInt64(message, F("default_uint64")));
  EXPECT_EQ(405, reflection->GetInt32(message, F("default_sint32")));
  EXPECT_EQ(406, reflection->GetInt64(message, F("default_sint64")));
  EXPECT_EQ(407, reflection->GetUInt32(message, F("default_fixed32")));
  EXPECT_EQ(408, reflection->GetUInt64(message, F("default_fixed64")));
  EXPECT_EQ(409, reflection->GetInt32(message, F("default_sfixed32")));
  EXPECT_EQ(410, reflection->GetInt64(message, F("default_sfixed64")));
  EXPECT_EQ(411, reflection->GetFloat(message, F("default_float")));
  EXPECT_EQ(412, reflection->GetDouble(message, F("default_double")));
  EXPECT_FALSE(reflection->GetBool(message, F("default_bool")));
  EXPECT_EQ("415", reflection->GetString(message, F("default_string")));
  EXPECT_EQ("416", reflection->GetString(message, F("default_bytes")));

  EXPECT_EQ("415", reflection->GetStringReference(message, F("default_string"),
                                                  &scratch));
  EXPECT_EQ("416", reflection->GetStringReference(message, F("default_bytes"),
                                                  &scratch));

  EXPECT_EQ(nested_foo_,
            reflection->GetEnum(message, F("default_nested_enum")));
  EXPECT_EQ(foreign_foo_,
            reflection->GetEnum(message, F("default_foreign_enum")));
  EXPECT_EQ(import_foo_,
            reflection->GetEnum(message, F("default_import_enum")));

  EXPECT_EQ("424", reflection->GetString(message, F("default_string_piece")));
  EXPECT_EQ("424", reflection->GetStringReference(
                       message, F("default_string_piece"), &scratch));

  EXPECT_EQ("425", reflection->GetString(message, F("default_cord")));
  EXPECT_EQ("425", reflection->GetStringReference(message, F("default_cord"),
                                                  &scratch));
}

inline void TestUtil::ReflectionTester::ExpectPackedFieldsSetViaReflection(
    const Message& message) {
  const Reflection* reflection = message.GetReflection();

  ASSERT_EQ(2, reflection->FieldSize(message, F("packed_int32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("packed_int64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("packed_uint32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("packed_uint64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("packed_sint32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("packed_sint64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("packed_fixed32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("packed_fixed64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("packed_sfixed32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("packed_sfixed64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("packed_float")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("packed_double")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("packed_bool")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("packed_enum")));

  EXPECT_EQ(601, reflection->GetRepeatedInt32(message, F("packed_int32"), 0));
  EXPECT_EQ(602, reflection->GetRepeatedInt64(message, F("packed_int64"), 0));
  EXPECT_EQ(603, reflection->GetRepeatedUInt32(message, F("packed_uint32"), 0));
  EXPECT_EQ(604, reflection->GetRepeatedUInt64(message, F("packed_uint64"), 0));
  EXPECT_EQ(605, reflection->GetRepeatedInt32(message, F("packed_sint32"), 0));
  EXPECT_EQ(606, reflection->GetRepeatedInt64(message, F("packed_sint64"), 0));
  EXPECT_EQ(607,
            reflection->GetRepeatedUInt32(message, F("packed_fixed32"), 0));
  EXPECT_EQ(608,
            reflection->GetRepeatedUInt64(message, F("packed_fixed64"), 0));
  EXPECT_EQ(609,
            reflection->GetRepeatedInt32(message, F("packed_sfixed32"), 0));
  EXPECT_EQ(610,
            reflection->GetRepeatedInt64(message, F("packed_sfixed64"), 0));
  EXPECT_EQ(611, reflection->GetRepeatedFloat(message, F("packed_float"), 0));
  EXPECT_EQ(612, reflection->GetRepeatedDouble(message, F("packed_double"), 0));
  EXPECT_TRUE(reflection->GetRepeatedBool(message, F("packed_bool"), 0));
  EXPECT_EQ(foreign_bar_,
            reflection->GetRepeatedEnum(message, F("packed_enum"), 0));

  EXPECT_EQ(701, reflection->GetRepeatedInt32(message, F("packed_int32"), 1));
  EXPECT_EQ(702, reflection->GetRepeatedInt64(message, F("packed_int64"), 1));
  EXPECT_EQ(703, reflection->GetRepeatedUInt32(message, F("packed_uint32"), 1));
  EXPECT_EQ(704, reflection->GetRepeatedUInt64(message, F("packed_uint64"), 1));
  EXPECT_EQ(705, reflection->GetRepeatedInt32(message, F("packed_sint32"), 1));
  EXPECT_EQ(706, reflection->GetRepeatedInt64(message, F("packed_sint64"), 1));
  EXPECT_EQ(707,
            reflection->GetRepeatedUInt32(message, F("packed_fixed32"), 1));
  EXPECT_EQ(708,
            reflection->GetRepeatedUInt64(message, F("packed_fixed64"), 1));
  EXPECT_EQ(709,
            reflection->GetRepeatedInt32(message, F("packed_sfixed32"), 1));
  EXPECT_EQ(710,
            reflection->GetRepeatedInt64(message, F("packed_sfixed64"), 1));
  EXPECT_EQ(711, reflection->GetRepeatedFloat(message, F("packed_float"), 1));
  EXPECT_EQ(712, reflection->GetRepeatedDouble(message, F("packed_double"), 1));
  EXPECT_FALSE(reflection->GetRepeatedBool(message, F("packed_bool"), 1));
  EXPECT_EQ(foreign_baz_,
            reflection->GetRepeatedEnum(message, F("packed_enum"), 1));
}

// -------------------------------------------------------------------

inline void TestUtil::ReflectionTester::ExpectClearViaReflection(
    const Message& message) {
  const Reflection* reflection = message.GetReflection();
  std::string scratch;
  const Message* sub_message;

  // has_blah() should initially be false for all optional fields.
  EXPECT_FALSE(reflection->HasField(message, F("optional_int32")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_int64")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_uint32")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_uint64")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_sint32")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_sint64")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_fixed32")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_fixed64")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_sfixed32")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_sfixed64")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_float")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_double")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_bool")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_string")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_bytes")));

  EXPECT_FALSE(reflection->HasField(message, F("optionalgroup")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_nested_message")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_foreign_message")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_import_message")));
  EXPECT_FALSE(
      reflection->HasField(message, F("optional_public_import_message")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_lazy_message")));
  EXPECT_FALSE(
      reflection->HasField(message, F("optional_unverified_lazy_message")));

  EXPECT_FALSE(reflection->HasField(message, F("optional_nested_enum")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_foreign_enum")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_import_enum")));

  EXPECT_FALSE(reflection->HasField(message, F("optional_string_piece")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_cord")));
  EXPECT_FALSE(reflection->HasField(message, F("optional_bytes_cord")));

  // Optional fields without defaults are set to zero or something like it.
  EXPECT_EQ(0, reflection->GetInt32(message, F("optional_int32")));
  EXPECT_EQ(0, reflection->GetInt64(message, F("optional_int64")));
  EXPECT_EQ(0, reflection->GetUInt32(message, F("optional_uint32")));
  EXPECT_EQ(0, reflection->GetUInt64(message, F("optional_uint64")));
  EXPECT_EQ(0, reflection->GetInt32(message, F("optional_sint32")));
  EXPECT_EQ(0, reflection->GetInt64(message, F("optional_sint64")));
  EXPECT_EQ(0, reflection->GetUInt32(message, F("optional_fixed32")));
  EXPECT_EQ(0, reflection->GetUInt64(message, F("optional_fixed64")));
  EXPECT_EQ(0, reflection->GetInt32(message, F("optional_sfixed32")));
  EXPECT_EQ(0, reflection->GetInt64(message, F("optional_sfixed64")));
  EXPECT_EQ(0, reflection->GetFloat(message, F("optional_float")));
  EXPECT_EQ(0, reflection->GetDouble(message, F("optional_double")));
  EXPECT_FALSE(reflection->GetBool(message, F("optional_bool")));
  EXPECT_EQ("", reflection->GetString(message, F("optional_string")));
  EXPECT_EQ("", reflection->GetString(message, F("optional_bytes")));

  EXPECT_EQ("", reflection->GetStringReference(message, F("optional_string"),
                                               &scratch));
  EXPECT_EQ("", reflection->GetStringReference(message, F("optional_bytes"),
                                               &scratch));

  // Embedded messages should also be clear.
  sub_message = &reflection->GetMessage(message, F("optionalgroup"));
  EXPECT_FALSE(sub_message->GetReflection()->HasField(*sub_message, group_a_));
  EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message, group_a_));
  sub_message = &reflection->GetMessage(message, F("optional_nested_message"));
  EXPECT_FALSE(sub_message->GetReflection()->HasField(*sub_message, nested_b_));
  EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message, nested_b_));
  sub_message = &reflection->GetMessage(message, F("optional_foreign_message"));
  EXPECT_FALSE(
      sub_message->GetReflection()->HasField(*sub_message, foreign_c_));
  EXPECT_EQ(0,
            sub_message->GetReflection()->GetInt32(*sub_message, foreign_c_));
  sub_message = &reflection->GetMessage(message, F("optional_import_message"));
  EXPECT_FALSE(sub_message->GetReflection()->HasField(*sub_message, import_d_));
  EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message, import_d_));
  sub_message =
      &reflection->GetMessage(message, F("optional_public_import_message"));
  EXPECT_FALSE(sub_message->GetReflection()->HasField(*sub_message, import_e_));
  EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message, import_e_));
  sub_message = &reflection->GetMessage(message, F("optional_lazy_message"));
  EXPECT_FALSE(sub_message->GetReflection()->HasField(*sub_message, nested_b_));
  EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message, nested_b_));
  sub_message =
      &reflection->GetMessage(message, F("optional_unverified_lazy_message"));
  EXPECT_FALSE(sub_message->GetReflection()->HasField(*sub_message, nested_b_));
  EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message, nested_b_));

  // Enums without defaults are set to the first value in the enum.
  EXPECT_EQ(nested_foo_,
            reflection->GetEnum(message, F("optional_nested_enum")));
  EXPECT_EQ(foreign_foo_,
            reflection->GetEnum(message, F("optional_foreign_enum")));
  EXPECT_EQ(import_foo_,
            reflection->GetEnum(message, F("optional_import_enum")));

  EXPECT_EQ("", reflection->GetString(message, F("optional_string_piece")));
  EXPECT_EQ("", reflection->GetStringReference(
                    message, F("optional_string_piece"), &scratch));

  EXPECT_EQ("", reflection->GetString(message, F("optional_cord")));
  EXPECT_EQ("", reflection->GetStringReference(message, F("optional_cord"),
                                               &scratch));

  EXPECT_EQ("", reflection->GetString(message, F("optional_bytes_cord")));
  EXPECT_EQ("", reflection->GetStringReference(
                    message, F("optional_bytes_cord"), &scratch));
  EXPECT_EQ("", reflection->GetCord(message, F("optional_bytes_cord")));

  // Repeated fields are empty.
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_int32")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_int64")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_uint32")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_uint64")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_sint32")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_sint64")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_fixed32")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_fixed64")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_sfixed32")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_sfixed64")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_float")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_double")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_bool")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_string")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_bytes")));

  EXPECT_EQ(0, reflection->FieldSize(message, F("repeatedgroup")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_nested_message")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_foreign_message")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_import_message")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_lazy_message")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_nested_enum")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_foreign_enum")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_import_enum")));

  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_string_piece")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("repeated_cord")));

  // has_blah() should also be false for all default fields.
  EXPECT_FALSE(reflection->HasField(message, F("default_int32")));
  EXPECT_FALSE(reflection->HasField(message, F("default_int64")));
  EXPECT_FALSE(reflection->HasField(message, F("default_uint32")));
  EXPECT_FALSE(reflection->HasField(message, F("default_uint64")));
  EXPECT_FALSE(reflection->HasField(message, F("default_sint32")));
  EXPECT_FALSE(reflection->HasField(message, F("default_sint64")));
  EXPECT_FALSE(reflection->HasField(message, F("default_fixed32")));
  EXPECT_FALSE(reflection->HasField(message, F("default_fixed64")));
  EXPECT_FALSE(reflection->HasField(message, F("default_sfixed32")));
  EXPECT_FALSE(reflection->HasField(message, F("default_sfixed64")));
  EXPECT_FALSE(reflection->HasField(message, F("default_float")));
  EXPECT_FALSE(reflection->HasField(message, F("default_double")));
  EXPECT_FALSE(reflection->HasField(message, F("default_bool")));
  EXPECT_FALSE(reflection->HasField(message, F("default_string")));
  EXPECT_FALSE(reflection->HasField(message, F("default_bytes")));

  EXPECT_FALSE(reflection->HasField(message, F("default_nested_enum")));
  EXPECT_FALSE(reflection->HasField(message, F("default_foreign_enum")));
  EXPECT_FALSE(reflection->HasField(message, F("default_import_enum")));

  EXPECT_FALSE(reflection->HasField(message, F("default_string_piece")));
  EXPECT_FALSE(reflection->HasField(message, F("default_cord")));

  // Fields with defaults have their default values (duh).
  EXPECT_EQ(41, reflection->GetInt32(message, F("default_int32")));
  EXPECT_EQ(42, reflection->GetInt64(message, F("default_int64")));
  EXPECT_EQ(43, reflection->GetUInt32(message, F("default_uint32")));
  EXPECT_EQ(44, reflection->GetUInt64(message, F("default_uint64")));
  EXPECT_EQ(-45, reflection->GetInt32(message, F("default_sint32")));
  EXPECT_EQ(46, reflection->GetInt64(message, F("default_sint64")));
  EXPECT_EQ(47, reflection->GetUInt32(message, F("default_fixed32")));
  EXPECT_EQ(48, reflection->GetUInt64(message, F("default_fixed64")));
  EXPECT_EQ(49, reflection->GetInt32(message, F("default_sfixed32")));
  EXPECT_EQ(-50, reflection->GetInt64(message, F("default_sfixed64")));
  EXPECT_EQ(51.5, reflection->GetFloat(message, F("default_float")));
  EXPECT_EQ(52e3, reflection->GetDouble(message, F("default_double")));
  EXPECT_TRUE(reflection->GetBool(message, F("default_bool")));
  EXPECT_EQ("hello", reflection->GetString(message, F("default_string")));
  EXPECT_EQ("world", reflection->GetString(message, F("default_bytes")));

  EXPECT_EQ("hello", reflection->GetStringReference(
                         message, F("default_string"), &scratch));
  EXPECT_EQ("world", reflection->GetStringReference(message, F("default_bytes"),
                                                    &scratch));

  EXPECT_EQ(nested_bar_,
            reflection->GetEnum(message, F("default_nested_enum")));
  EXPECT_EQ(foreign_bar_,
            reflection->GetEnum(message, F("default_foreign_enum")));
  EXPECT_EQ(import_bar_,
            reflection->GetEnum(message, F("default_import_enum")));

  EXPECT_EQ("abc", reflection->GetString(message, F("default_string_piece")));
  EXPECT_EQ("abc", reflection->GetStringReference(
                       message, F("default_string_piece"), &scratch));

  EXPECT_EQ("123", reflection->GetString(message, F("default_cord")));
  EXPECT_EQ("123", reflection->GetStringReference(message, F("default_cord"),
                                                  &scratch));
}

// -------------------------------------------------------------------

inline void TestUtil::ReflectionTester::ModifyRepeatedFieldsViaReflection(
    Message* message) {
  const Reflection* reflection = message->GetReflection();
  Message* sub_message;

  reflection->SetRepeatedInt32(message, F("repeated_int32"), 1, 501);
  reflection->SetRepeatedInt64(message, F("repeated_int64"), 1, 502);
  reflection->SetRepeatedUInt32(message, F("repeated_uint32"), 1, 503);
  reflection->SetRepeatedUInt64(message, F("repeated_uint64"), 1, 504);
  reflection->SetRepeatedInt32(message, F("repeated_sint32"), 1, 505);
  reflection->SetRepeatedInt64(message, F("repeated_sint64"), 1, 506);
  reflection->SetRepeatedUInt32(message, F("repeated_fixed32"), 1, 507);
  reflection->SetRepeatedUInt64(message, F("repeated_fixed64"), 1, 508);
  reflection->SetRepeatedInt32(message, F("repeated_sfixed32"), 1, 509);
  reflection->SetRepeatedInt64(message, F("repeated_sfixed64"), 1, 510);
  reflection->SetRepeatedFloat(message, F("repeated_float"), 1, 511);
  reflection->SetRepeatedDouble(message, F("repeated_double"), 1, 512);
  reflection->SetRepeatedBool(message, F("repeated_bool"), 1, true);
  reflection->SetRepeatedString(message, F("repeated_string"), 1, "515");
  reflection->SetRepeatedString(message, F("repeated_bytes"), 1, "516");

  sub_message =
      reflection->MutableRepeatedMessage(message, F("repeatedgroup"), 1);
  sub_message->GetReflection()->SetInt32(sub_message, repeated_group_a_, 517);
  sub_message = reflection->MutableRepeatedMessage(
      message, F("repeated_nested_message"), 1);
  sub_message->GetReflection()->SetInt32(sub_message, nested_b_, 518);
  sub_message = reflection->MutableRepeatedMessage(
      message, F("repeated_foreign_message"), 1);
  sub_message->GetReflection()->SetInt32(sub_message, foreign_c_, 519);
  sub_message = reflection->MutableRepeatedMessage(
      message, F("repeated_import_message"), 1);
  sub_message->GetReflection()->SetInt32(sub_message, import_d_, 520);
  sub_message = reflection->MutableRepeatedMessage(
      message, F("repeated_lazy_message"), 1);
  sub_message->GetReflection()->SetInt32(sub_message, nested_b_, 527);

  reflection->SetRepeatedEnum(message, F("repeated_nested_enum"), 1,
                              nested_foo_);
  reflection->SetRepeatedEnum(message, F("repeated_foreign_enum"), 1,
                              foreign_foo_);
  reflection->SetRepeatedEnum(message, F("repeated_import_enum"), 1,
                              import_foo_);

  reflection->SetRepeatedString(message, F("repeated_string_piece"), 1, "524");
  reflection->SetRepeatedString(message, F("repeated_cord"), 1, "525");
}

inline void TestUtil::ReflectionTester::RemoveLastRepeatedsViaReflection(
    Message* message) {
  const Reflection* reflection = message->GetReflection();

  std::vector<const FieldDescriptor*> output;
  reflection->ListFields(*message, &output);
  for (int i = 0; i < output.size(); ++i) {
    const FieldDescriptor* field = output[i];
    if (!field->is_repeated()) continue;

    reflection->RemoveLast(message, field);
  }
}

inline void TestUtil::ReflectionTester::ReleaseLastRepeatedsViaReflection(
    Message* message, bool expect_extensions_notnull) {
  const Reflection* reflection = message->GetReflection();

  std::vector<const FieldDescriptor*> output;
  reflection->ListFields(*message, &output);
  for (int i = 0; i < output.size(); ++i) {
    const FieldDescriptor* field = output[i];
    if (!field->is_repeated()) continue;
    if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) continue;

    Message* released = reflection->ReleaseLast(message, field);
    if (!field->is_extension() || expect_extensions_notnull) {
      ASSERT_TRUE(released != nullptr)
          << "ReleaseLast returned nullptr for: " << field->name();
    }
    delete released;
  }
}

inline void TestUtil::ReflectionTester::SwapRepeatedsViaReflection(
    Message* message) {
  const Reflection* reflection = message->GetReflection();

  std::vector<const FieldDescriptor*> output;
  reflection->ListFields(*message, &output);
  for (int i = 0; i < output.size(); ++i) {
    const FieldDescriptor* field = output[i];
    if (!field->is_repeated()) continue;

    reflection->SwapElements(message, field, 0, 1);
  }
}

inline void TestUtil::ReflectionTester::
    SetAllocatedOptionalMessageFieldsToNullViaReflection(Message* message) {
  const Reflection* reflection = message->GetReflection();

  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(*message, &fields);

  for (int i = 0; i < fields.size(); ++i) {
    const FieldDescriptor* field = fields[i];
    if (field->is_required() || field->is_repeated() ||
        field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE)
      continue;

    reflection->SetAllocatedMessage(message, nullptr, field);
  }
}

inline void TestUtil::ReflectionTester::
    SetAllocatedOptionalMessageFieldsToMessageViaReflection(
        Message* from_message, Message* to_message) {
  EXPECT_EQ(from_message->GetDescriptor(), to_message->GetDescriptor());
  const Reflection* from_reflection = from_message->GetReflection();
  const Reflection* to_reflection = to_message->GetReflection();

  std::vector<const FieldDescriptor*> fields;
  from_reflection->ListFields(*from_message, &fields);

  for (int i = 0; i < fields.size(); ++i) {
    const FieldDescriptor* field = fields[i];
    if (field->is_required() || field->is_repeated() ||
        field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE)
      continue;

    Message* sub_message = from_reflection->ReleaseMessage(from_message, field);
    to_reflection->SetAllocatedMessage(to_message, sub_message, field);
  }
}

inline void TestUtil::ReflectionTester::ExpectMessagesReleasedViaReflection(
    Message* message,
    TestUtil::ReflectionTester::MessageReleaseState expected_release_state) {
  const Reflection* reflection = message->GetReflection();

  static const char* fields[] = {
      "optionalgroup",
      "optional_nested_message",
      "optional_foreign_message",
      "optional_import_message",
  };
  for (int i = 0; i < ABSL_ARRAYSIZE(fields); i++) {
    Message* released = reflection->ReleaseMessage(message, F(fields[i]));
    switch (expected_release_state) {
      case IS_NULL:
        EXPECT_TRUE(released == nullptr);
        break;
      case NOT_NULL:
        EXPECT_TRUE(released != nullptr);
        break;
      case CAN_BE_NULL:
        break;
    }
    delete released;
    EXPECT_FALSE(reflection->HasField(*message, F(fields[i])));
  }
}

// Check that the passed-in serialization is the canonical serialization we
// expect for a TestFieldOrderings message filled in by
// SetAllFieldsAndExtensions().
inline void ExpectAllFieldsAndExtensionsInOrder(const std::string& serialized) {
  // We set each field individually, serialize separately, and concatenate all
  // the strings in canonical order to determine the expected serialization.
  std::string expected;
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

// Trait to gather all types and values used in the templates below for a
// particular namespace.

template <typename T>
struct TestUtilTraits;

// List all the extensions that are defined at namespace scope.
#define PROTOBUF_TEST_UTIL_DECLARE_INPUT_EXTS_(ns, X) \
  X(ns, my_extension_int);                            \
  X(ns, my_extension_string);                         \
  X(ns, optional_int32_extension);                    \
  X(ns, optional_int64_extension);                    \
  X(ns, optional_uint32_extension);                   \
  X(ns, optional_uint64_extension);                   \
  X(ns, optional_sint32_extension);                   \
  X(ns, optional_sint64_extension);                   \
  X(ns, optional_fixed32_extension);                  \
  X(ns, optional_fixed64_extension);                  \
  X(ns, optional_sfixed32_extension);                 \
  X(ns, optional_sfixed64_extension);                 \
  X(ns, optional_float_extension);                    \
  X(ns, optional_double_extension);                   \
  X(ns, optional_bool_extension);                     \
  X(ns, optional_string_extension);                   \
  X(ns, optional_bytes_extension);                    \
  X(ns, optionalgroup_extension);                     \
  X(ns, optional_nested_message_extension);           \
  X(ns, optional_foreign_message_extension);          \
  X(ns, optional_import_message_extension);           \
  X(ns, optional_nested_enum_extension);              \
  X(ns, optional_foreign_enum_extension);             \
  X(ns, optional_import_enum_extension);              \
  X(ns, optional_string_piece_extension);             \
  X(ns, optional_cord_extension);                     \
  X(ns, optional_bytes_cord_extension);               \
  X(ns, optional_public_import_message_extension);    \
  X(ns, optional_lazy_message_extension);             \
  X(ns, optional_unverified_lazy_message_extension);  \
  X(ns, default_bool_extension);                      \
  X(ns, default_bytes_extension);                     \
  X(ns, default_cord_extension);                      \
  X(ns, default_double_extension);                    \
  X(ns, default_fixed32_extension);                   \
  X(ns, default_fixed64_extension);                   \
  X(ns, default_float_extension);                     \
  X(ns, default_foreign_enum_extension);              \
  X(ns, default_import_enum_extension);               \
  X(ns, default_int32_extension);                     \
  X(ns, default_int64_extension);                     \
  X(ns, default_nested_enum_extension);               \
  X(ns, default_sfixed32_extension);                  \
  X(ns, default_sfixed64_extension);                  \
  X(ns, default_sint32_extension);                    \
  X(ns, default_sint64_extension);                    \
  X(ns, default_string_extension);                    \
  X(ns, default_string_piece_extension);              \
  X(ns, default_uint32_extension);                    \
  X(ns, default_uint64_extension);                    \
  X(ns, repeated_bool_extension);                     \
  X(ns, repeated_bytes_extension);                    \
  X(ns, repeated_cord_extension);                     \
  X(ns, repeated_double_extension);                   \
  X(ns, repeated_fixed32_extension);                  \
  X(ns, repeated_fixed64_extension);                  \
  X(ns, repeated_float_extension);                    \
  X(ns, repeated_foreign_enum_extension);             \
  X(ns, repeated_foreign_message_extension);          \
  X(ns, repeated_import_enum_extension);              \
  X(ns, repeated_import_message_extension);           \
  X(ns, repeated_int32_extension);                    \
  X(ns, repeated_int64_extension);                    \
  X(ns, repeated_lazy_message_extension);             \
  X(ns, repeated_nested_enum_extension);              \
  X(ns, repeated_nested_message_extension);           \
  X(ns, repeated_sfixed32_extension);                 \
  X(ns, repeated_sfixed64_extension);                 \
  X(ns, repeated_sint32_extension);                   \
  X(ns, repeated_sint64_extension);                   \
  X(ns, repeated_string_extension);                   \
  X(ns, repeated_string_piece_extension);             \
  X(ns, repeated_uint32_extension);                   \
  X(ns, repeated_uint64_extension);                   \
  X(ns, repeatedgroup_extension);                     \
  X(ns, oneof_uint32_extension);                      \
  X(ns, oneof_nested_message_extension);              \
  X(ns, oneof_string_extension);                      \
  X(ns, oneof_bytes_extension);                       \
  X(ns, unpacked_bool_extension);                     \
  X(ns, unpacked_double_extension);                   \
  X(ns, unpacked_enum_extension);                     \
  X(ns, unpacked_fixed32_extension);                  \
  X(ns, unpacked_fixed64_extension);                  \
  X(ns, unpacked_float_extension);                    \
  X(ns, unpacked_int32_extension);                    \
  X(ns, unpacked_int64_extension);                    \
  X(ns, unpacked_sfixed32_extension);                 \
  X(ns, unpacked_sfixed64_extension);                 \
  X(ns, unpacked_sint32_extension);                   \
  X(ns, unpacked_sint64_extension);                   \
  X(ns, unpacked_uint32_extension);                   \
  X(ns, unpacked_uint64_extension);                   \
  X(ns, packed_int32_extension);                      \
  X(ns, packed_int64_extension);                      \
  X(ns, packed_uint32_extension);                     \
  X(ns, packed_uint64_extension);                     \
  X(ns, packed_sint32_extension);                     \
  X(ns, packed_sint64_extension);                     \
  X(ns, packed_fixed32_extension);                    \
  X(ns, packed_fixed64_extension);                    \
  X(ns, packed_sfixed32_extension);                   \
  X(ns, packed_sfixed64_extension);                   \
  X(ns, packed_float_extension);                      \
  X(ns, packed_double_extension);                     \
  X(ns, packed_bool_extension);                       \
  X(ns, packed_enum_extension);

#define PROTOBUF_TEST_UTIL_DECLARE_INPUT_EXT_VARIABLE_(ns, name) \
  static constexpr auto& name = ns::name

// Declare traits for the namespaces ns and import_ns.
// It connects all namespace scoped names together.
// This is needed so that functions that take some type X can find
// declarations from the same namespace.
#define PROTOBUF_TEST_UTIL_DECLARE_INPUT_IMPL(ns, import_ns)                   \
  template <>                                                                  \
  struct google::protobuf::TestUtil::TestUtilTraits<::ns::TestAllExtensions> {           \
    PROTOBUF_TEST_UTIL_DECLARE_INPUT_EXTS_(                                    \
        ::ns, PROTOBUF_TEST_UTIL_DECLARE_INPUT_EXT_VARIABLE_);                 \
    using TestAllTypes = ::ns::TestAllTypes;                                   \
    using TestRequired = ::ns::TestRequired;                                   \
    using NestedTestMessageSetContainer = ::ns::NestedTestMessageSetContainer; \
    using OneBytes = ::ns::OneBytes;                                           \
    using OneString = ::ns::OneString;                                         \
    using MoreBytes = ::ns::MoreBytes;                                         \
    using MoreString = ::ns::MoreString;                                       \
    using TestEmptyMessage = ::ns::TestEmptyMessage;                           \
    using TestRepeatedScalarDifferentTagSizes =                                \
        ::ns::TestRepeatedScalarDifferentTagSizes;                             \
    using TestRecursiveMessage = ::ns::TestRecursiveMessage;                   \
    using Int32Message = ::ns::Int32Message;                                   \
    using Uint32Message = ::ns::Uint32Message;                                 \
    using Int64Message = ::ns::Int64Message;                                   \
    using Uint64Message = ::ns::Uint64Message;                                 \
    using BoolMessage = ::ns::BoolMessage;                                     \
    using RawMessageSet = ::ns::RawMessageSet;                                 \
    using TestFieldOrderings = ::ns::TestFieldOrderings;                       \
    using TestMessageSetExtension1 = ::ns::TestMessageSetExtension1;           \
    using TestMessageSetExtension2 = ::ns::TestMessageSetExtension2;           \
    using TestMessageSet =                                                     \
        decltype(TestMessageSetExtension1::message_set_extension)::Extendee;   \
    using TestOneofBackwardsCompatible = ::ns::TestOneofBackwardsCompatible;   \
    using TestOneof = ::ns::TestOneof;                                         \
    using TestOneof2 = ::ns::TestOneof2;                                       \
    using TestPackedExtensions = ::ns::TestPackedExtensions;                   \
    using TestUnpackedTypes = ::ns::TestUnpackedTypes;                         \
    using TestPackedTypes = ::ns::TestPackedTypes;                             \
    using TestAllExtensions = ::ns::TestAllExtensions;                         \
    static constexpr auto FOREIGN_FOO = ::ns::FOREIGN_FOO;                     \
    static constexpr auto FOREIGN_BAR = ::ns::FOREIGN_BAR;                     \
    static constexpr auto FOREIGN_BAZ = ::ns::FOREIGN_BAZ;                     \
    static constexpr auto IMPORT_FOO = ::import_ns::IMPORT_FOO;                \
    static constexpr auto IMPORT_BAR = ::import_ns::IMPORT_BAR;                \
    static constexpr auto IMPORT_BAZ = ::import_ns::IMPORT_BAZ;                \
  };                                                                           \
  template <>                                                                  \
  struct google::protobuf::TestUtil::TestUtilTraits<::ns::TestFieldOrderings>            \
      : google::protobuf::TestUtil::TestUtilTraits<::ns::TestAllExtensions> {};          \
  template <>                                                                  \
  struct google::protobuf::TestUtil::TestUtilTraits<::ns::TestAllTypes>                  \
      : google::protobuf::TestUtil::TestUtilTraits<::ns::TestAllExtensions> {};          \
  template <>                                                                  \
  struct google::protobuf::TestUtil::TestUtilTraits<::ns::TestPackedExtensions>          \
      : google::protobuf::TestUtil::TestUtilTraits<::ns::TestAllExtensions> {};          \
  template <>                                                                  \
  struct google::protobuf::TestUtil::TestUtilTraits<::ns::TestUnpackedExtensions>        \
      : google::protobuf::TestUtil::TestUtilTraits<::ns::TestAllExtensions> {}

#define PROTOBUF_TEST_UTIL_DECLARE_INPUT(ns_suffix)                 \
  PROTOBUF_TEST_UTIL_DECLARE_INPUT_IMPL(proto2_unittest##ns_suffix, \
                                        proto2_unittest_import##ns_suffix)

// Set every field in the message to a unique value.
template <typename TestAllTypes>
void SetAllFields(TestAllTypes* message);
template <typename TestAllTypes>
void SetOptionalFields(TestAllTypes* message);
template <typename TestAllTypes>
void AddRepeatedFields1(TestAllTypes* message);
template <typename TestAllTypes>
void AddRepeatedFields2(TestAllTypes* message);
template <typename TestAllTypes>
void SetDefaultFields(TestAllTypes* message);
template <typename TestAllTypes>
void SetOneofFields(TestAllTypes* message);
template <typename TestPackedTypes>
void SetPackedFields(TestPackedTypes* message);
template <typename TestUnpackedTypes>
void SetUnpackedFields(TestUnpackedTypes* message);
template <typename TestOneof2>
void SetOneof1(TestOneof2* message);
template <typename TestOneof2>
void SetOneof2(TestOneof2* message);
template <typename TestAllExtensions>
void SetAllExtensions(TestAllExtensions* message);
template <typename TestAllExtensions>
void SetOneofFieldsExtensions(TestAllExtensions* message);
template <typename TestFieldOrderings>
void SetAllFieldsAndExtensions(TestFieldOrderings* message);
template <typename TestPackedExtensions>
void SetPackedExtensions(TestPackedExtensions* message);

// Use the repeated versions of the set_*() accessors to modify all the
// repeated fields of the message (which should already have been
// initialized with Set*Fields()).  Set*Fields() itself only tests
// the add_*() accessors.
template <typename TestAllTypes>
void ModifyRepeatedFields(TestAllTypes* message);
template <typename TestAllExtensions>
void ModifyRepeatedExtensions(TestAllExtensions* message);

// Check that all fields have the values that they should have after
// Set*Fields() is called.
template <typename TestAllTypes>
void ExpectAllFieldsSet(const TestAllTypes& message);
template <typename TestPackedTypes>
void ExpectPackedFieldsSet(const TestPackedTypes& message);
template <typename TestUnpackedTypes>
void ExpectUnpackedFieldsSet(const TestUnpackedTypes& message);
template <typename TestOneof2>
void ExpectOneofSet1(const TestOneof2& message);
template <typename TestOneof2>
void ExpectOneofSet2(const TestOneof2& message);
template <typename TestPackedExtensions>
void ExpectPackedExtensionsSet(const TestPackedExtensions& message);
template <typename TestUnpackedExtensions>
void ExpectUnpackedExtensionsSet(const TestUnpackedExtensions& message);
template <typename TestPackedExtensions>
void ExpectLastRepeatedExtensionsRemovedPacked(
    const TestPackedExtensions& message);
template <typename TestAllTypes>
void ExpectLastRepeatedsReleased(const TestAllTypes& message);
template <typename TestAllExtensions>
void ExpectLastRepeatedExtensionsReleased(const TestAllExtensions& message);

// Expect that the message is modified as would be expected from
// Modify*Fields().
template <typename TestAllTypes>
void ExpectRepeatedFieldsModified(const TestAllTypes& message);
template <typename TestAllExtensions>
void ExpectRepeatedExtensionsModified(const TestAllExtensions& message);

// Check that all fields have their default values.
template <typename TestAllTypes>
void ExpectClear(const TestAllTypes& message);
template <typename TestOneof2>
void ExpectOneofClear(const TestOneof2& message);
template <typename TestAllExtensions>
void ExpectExtensionsClear(const TestAllExtensions& message);

// Check that all repeated fields have had their last elements removed.
template <typename TestAllTypes>
void ExpectLastRepeatedsRemoved(const TestAllTypes& message);
template <typename TestAllExtensions>
void ExpectLastRepeatedExtensionsRemoved(const TestAllExtensions& message);

// Check that all repeated fields have had their first and last elements
// swapped.
template <typename TestAllTypes>
void ExpectRepeatedsSwapped(const TestAllTypes& message);
template <typename TestAllExtensions>
void ExpectRepeatedExtensionsSwapped(const TestAllExtensions& message);

template <typename TestOneof2>
void ExpectAtMostOneFieldSetInOneof(const TestOneof2& message);

template <typename TestAllTypes>
void SetAllFields(TestAllTypes* message) {
  SetOptionalFields(message);
  AddRepeatedFields1(message);
  AddRepeatedFields2(message);
  SetDefaultFields(message);
  SetOneofFields(message);
}

// Helper aliases to extract the different enum types from the message type.
// Allow to find the types without using namespaces.
template <typename T>
auto ForeignEnumF() -> decltype(T().optional_foreign_enum());
template <typename T>
auto ForeignEnumF() -> decltype(T().packed_enum(0));
template <typename T>
auto ForeignEnumF() -> decltype(T().unpacked_enum(0));

template <typename T>
using ForeignEnum = decltype(ForeignEnumF<T>());

template <typename T>
using ImportEnum = decltype(T().optional_import_enum());

template <typename TestAllTypes>
void SetOptionalFields(TestAllTypes* message) {
  message->set_optional_int32(101);
  message->set_optional_int64(102);
  message->set_optional_uint32(103);
  message->set_optional_uint64(104);
  message->set_optional_sint32(105);
  message->set_optional_sint64(106);
  message->set_optional_fixed32(107);
  message->set_optional_fixed64(108);
  message->set_optional_sfixed32(109);
  message->set_optional_sfixed64(110);
  message->set_optional_float(111);
  message->set_optional_double(112);
  message->set_optional_bool(true);
  message->set_optional_string("115");
  message->set_optional_bytes("116");

  message->mutable_optionalgroup()->set_a(117);
  message->mutable_optional_nested_message()->set_bb(118);
  message->mutable_optional_foreign_message()->set_c(119);
  message->mutable_optional_import_message()->set_d(120);
  message->mutable_optional_public_import_message()->set_e(126);
  message->mutable_optional_lazy_message()->set_bb(127);
  message->mutable_optional_unverified_lazy_message()->set_bb(128);

  message->set_optional_nested_enum(TestAllTypes::BAZ);
  message->set_optional_foreign_enum(ForeignEnum<TestAllTypes>::FOREIGN_BAZ);
  message->set_optional_import_enum(ImportEnum<TestAllTypes>::IMPORT_BAZ);

  // StringPiece and Cord fields are only accessible via reflection in the
  // open source release; see comments in compiler/cpp/string_field.cc.
#ifndef PROTOBUF_TEST_NO_DESCRIPTORS
  message->GetReflection()->SetString(
      message,
      message->GetDescriptor()->FindFieldByName("optional_string_piece"),
      "124");
  message->GetReflection()->SetString(
      message, message->GetDescriptor()->FindFieldByName("optional_cord"),
      "125");
#endif  // !PROTOBUF_TEST_NO_DESCRIPTORS
  message->set_optional_bytes_cord("optional bytes cord");
}

// -------------------------------------------------------------------

template <typename TestAllTypes>
void AddRepeatedFields1(TestAllTypes* message) {
  message->add_repeated_int32(201);
  message->add_repeated_int64(202);
  message->add_repeated_uint32(203);
  message->add_repeated_uint64(204);
  message->add_repeated_sint32(205);
  message->add_repeated_sint64(206);
  message->add_repeated_fixed32(207);
  message->add_repeated_fixed64(208);
  message->add_repeated_sfixed32(209);
  message->add_repeated_sfixed64(210);
  message->add_repeated_float(211);
  message->add_repeated_double(212);
  message->add_repeated_bool(true);
  message->add_repeated_string("215");
  message->add_repeated_bytes("216");

  message->add_repeatedgroup()->set_a(217);
  message->add_repeated_nested_message()->set_bb(218);
  message->add_repeated_foreign_message()->set_c(219);
  message->add_repeated_import_message()->set_d(220);
  message->add_repeated_lazy_message()->set_bb(227);

  message->add_repeated_nested_enum(TestAllTypes::BAR);
  message->add_repeated_foreign_enum(ForeignEnum<TestAllTypes>::FOREIGN_BAR);
  message->add_repeated_import_enum(ImportEnum<TestAllTypes>::IMPORT_BAR);

#ifndef PROTOBUF_TEST_NO_DESCRIPTORS
  message->GetReflection()->AddString(
      message,
      message->GetDescriptor()->FindFieldByName("repeated_string_piece"),
      "224");
  message->GetReflection()->AddString(
      message, message->GetDescriptor()->FindFieldByName("repeated_cord"),
      "225");
#endif  // !PROTOBUF_TEST_NO_DESCRIPTORS
}

template <typename TestAllTypes>
void AddRepeatedFields2(TestAllTypes* message) {
  // Add a second one of each field.
  message->add_repeated_int32(301);
  message->add_repeated_int64(302);
  message->add_repeated_uint32(303);
  message->add_repeated_uint64(304);
  message->add_repeated_sint32(305);
  message->add_repeated_sint64(306);
  message->add_repeated_fixed32(307);
  message->add_repeated_fixed64(308);
  message->add_repeated_sfixed32(309);
  message->add_repeated_sfixed64(310);
  message->add_repeated_float(311);
  message->add_repeated_double(312);
  message->add_repeated_bool(false);
  message->add_repeated_string("315");
  message->add_repeated_bytes("316");

  message->add_repeatedgroup()->set_a(317);
  message->add_repeated_nested_message()->set_bb(318);
  message->add_repeated_foreign_message()->set_c(319);
  message->add_repeated_import_message()->set_d(320);
  message->add_repeated_lazy_message()->set_bb(327);

  message->add_repeated_nested_enum(TestAllTypes::BAZ);
  message->add_repeated_foreign_enum(ForeignEnum<TestAllTypes>::FOREIGN_BAZ);
  message->add_repeated_import_enum(ImportEnum<TestAllTypes>::IMPORT_BAZ);

#ifndef PROTOBUF_TEST_NO_DESCRIPTORS
  message->GetReflection()->AddString(
      message,
      message->GetDescriptor()->FindFieldByName("repeated_string_piece"),
      "324");
  message->GetReflection()->AddString(
      message, message->GetDescriptor()->FindFieldByName("repeated_cord"),
      "325");
#endif  // !PROTOBUF_TEST_NO_DESCRIPTORS
}

template <typename TestAllTypes>
void SetDefaultFields(TestAllTypes* message) {
  message->set_default_int32(401);
  message->set_default_int64(402);
  message->set_default_uint32(403);
  message->set_default_uint64(404);
  message->set_default_sint32(405);
  message->set_default_sint64(406);
  message->set_default_fixed32(407);
  message->set_default_fixed64(408);
  message->set_default_sfixed32(409);
  message->set_default_sfixed64(410);
  message->set_default_float(411);
  message->set_default_double(412);
  message->set_default_bool(false);
  message->set_default_string("415");
  message->set_default_bytes("416");

  message->set_default_nested_enum(TestAllTypes::FOO);
  message->set_default_foreign_enum(ForeignEnum<TestAllTypes>::FOREIGN_FOO);
  message->set_default_import_enum(ImportEnum<TestAllTypes>::IMPORT_FOO);

#ifndef PROTOBUF_TEST_NO_DESCRIPTORS
  message->GetReflection()->SetString(
      message,
      message->GetDescriptor()->FindFieldByName("default_string_piece"), "424");
  message->GetReflection()->SetString(
      message, message->GetDescriptor()->FindFieldByName("default_cord"),
      "425");
#endif  // !PROTOBUF_TEST_NO_DESCRIPTORS
}

template <typename TestAllTypes>
void ModifyRepeatedFields(TestAllTypes* message) {
  message->set_repeated_int32(1, 501);
  message->set_repeated_int64(1, 502);
  message->set_repeated_uint32(1, 503);
  message->set_repeated_uint64(1, 504);
  message->set_repeated_sint32(1, 505);
  message->set_repeated_sint64(1, 506);
  message->set_repeated_fixed32(1, 507);
  message->set_repeated_fixed64(1, 508);
  message->set_repeated_sfixed32(1, 509);
  message->set_repeated_sfixed64(1, 510);
  message->set_repeated_float(1, 511);
  message->set_repeated_double(1, 512);
  message->set_repeated_bool(1, true);
  message->set_repeated_string(1, "515");
  message->set_repeated_bytes(1, "516");

  message->mutable_repeatedgroup(1)->set_a(517);
  message->mutable_repeated_nested_message(1)->set_bb(518);
  message->mutable_repeated_foreign_message(1)->set_c(519);
  message->mutable_repeated_import_message(1)->set_d(520);
  message->mutable_repeated_lazy_message(1)->set_bb(527);

  message->set_repeated_nested_enum(1, TestAllTypes::FOO);
  message->set_repeated_foreign_enum(1, ForeignEnum<TestAllTypes>::FOREIGN_FOO);
  message->set_repeated_import_enum(1, ImportEnum<TestAllTypes>::IMPORT_FOO);

#ifndef PROTOBUF_TEST_NO_DESCRIPTORS
  message->GetReflection()->SetRepeatedString(
      message,
      message->GetDescriptor()->FindFieldByName("repeated_string_piece"), 1,
      "524");
  message->GetReflection()->SetRepeatedString(
      message, message->GetDescriptor()->FindFieldByName("repeated_cord"), 1,
      "525");
#endif  // !PROTOBUF_TEST_NO_DESCRIPTORS
}

template <typename TestAllTypes>
void SetOneofFields(TestAllTypes* message) {
  message->set_oneof_uint32(601);
  message->mutable_oneof_nested_message()->set_bb(602);
  message->mutable_oneof_lazy_nested_message()->set_bb(605);
  message->set_oneof_string("603");
#ifndef PROTOBUF_TEST_NO_DESCRIPTORS
  message->GetReflection()->SetString(
      message, message->GetDescriptor()->FindFieldByName("oneof_cord"), "606");
  message->GetReflection()->SetString(
      message, message->GetDescriptor()->FindFieldByName("oneof_string_piece"),
      "607");
#endif  // !PROTOBUF_TEST_NO_DESCRIPTORS
  // Must be last because tests are expecting it.
  message->set_oneof_bytes("604");
}

template <typename TestAllTypes>
void ExpectAllFieldsSet(const TestAllTypes& message) {
  EXPECT_TRUE(message.has_optional_int32());
  EXPECT_TRUE(message.has_optional_int64());
  EXPECT_TRUE(message.has_optional_uint32());
  EXPECT_TRUE(message.has_optional_uint64());
  EXPECT_TRUE(message.has_optional_sint32());
  EXPECT_TRUE(message.has_optional_sint64());
  EXPECT_TRUE(message.has_optional_fixed32());
  EXPECT_TRUE(message.has_optional_fixed64());
  EXPECT_TRUE(message.has_optional_sfixed32());
  EXPECT_TRUE(message.has_optional_sfixed64());
  EXPECT_TRUE(message.has_optional_float());
  EXPECT_TRUE(message.has_optional_double());
  EXPECT_TRUE(message.has_optional_bool());
  EXPECT_TRUE(message.has_optional_string());
  EXPECT_TRUE(message.has_optional_bytes());

  EXPECT_TRUE(message.has_optionalgroup());
  EXPECT_TRUE(message.has_optional_nested_message());
  EXPECT_TRUE(message.has_optional_foreign_message());
  EXPECT_TRUE(message.has_optional_import_message());
  EXPECT_TRUE(message.has_optional_public_import_message());
  EXPECT_TRUE(message.has_optional_lazy_message());
  EXPECT_TRUE(message.has_optional_unverified_lazy_message());

  EXPECT_TRUE(message.optionalgroup().has_a());
  EXPECT_TRUE(message.optional_nested_message().has_bb());
  EXPECT_TRUE(message.optional_foreign_message().has_c());
  EXPECT_TRUE(message.optional_import_message().has_d());
  EXPECT_TRUE(message.optional_public_import_message().has_e());
  EXPECT_TRUE(message.optional_lazy_message().has_bb());
  EXPECT_TRUE(message.optional_unverified_lazy_message().has_bb());

  EXPECT_TRUE(message.has_optional_nested_enum());
  EXPECT_TRUE(message.has_optional_foreign_enum());
  EXPECT_TRUE(message.has_optional_import_enum());

#ifndef PROTOBUF_TEST_NO_DESCRIPTORS
  EXPECT_TRUE(message.has_optional_string_piece());
  EXPECT_TRUE(message.has_optional_cord());
#endif
  EXPECT_TRUE(message.has_optional_bytes_cord());

  EXPECT_EQ(101, message.optional_int32());
  EXPECT_EQ(102, message.optional_int64());
  EXPECT_EQ(103, message.optional_uint32());
  EXPECT_EQ(104, message.optional_uint64());
  EXPECT_EQ(105, message.optional_sint32());
  EXPECT_EQ(106, message.optional_sint64());
  EXPECT_EQ(107, message.optional_fixed32());
  EXPECT_EQ(108, message.optional_fixed64());
  EXPECT_EQ(109, message.optional_sfixed32());
  EXPECT_EQ(110, message.optional_sfixed64());
  EXPECT_EQ(111, message.optional_float());
  EXPECT_EQ(112, message.optional_double());
  EXPECT_TRUE(message.optional_bool());
  EXPECT_EQ("115", message.optional_string());
  EXPECT_EQ("116", message.optional_bytes());

  EXPECT_EQ(117, message.optionalgroup().a());
  EXPECT_EQ(118, message.optional_nested_message().bb());
  EXPECT_EQ(119, message.optional_foreign_message().c());
  EXPECT_EQ(120, message.optional_import_message().d());
  EXPECT_EQ(126, message.optional_public_import_message().e());
  EXPECT_EQ(127, message.optional_lazy_message().bb());
  EXPECT_EQ(128, message.optional_unverified_lazy_message().bb());

  EXPECT_EQ(TestAllTypes::BAZ, message.optional_nested_enum());
  EXPECT_EQ(ForeignEnum<TestAllTypes>::FOREIGN_BAZ,
            message.optional_foreign_enum());
  EXPECT_EQ(ImportEnum<TestAllTypes>::IMPORT_BAZ,
            message.optional_import_enum());

  EXPECT_EQ("optional bytes cord", message.optional_bytes_cord());

  // -----------------------------------------------------------------

  ASSERT_EQ(2, message.repeated_int32_size());
  ASSERT_EQ(2, message.repeated_int64_size());
  ASSERT_EQ(2, message.repeated_uint32_size());
  ASSERT_EQ(2, message.repeated_uint64_size());
  ASSERT_EQ(2, message.repeated_sint32_size());
  ASSERT_EQ(2, message.repeated_sint64_size());
  ASSERT_EQ(2, message.repeated_fixed32_size());
  ASSERT_EQ(2, message.repeated_fixed64_size());
  ASSERT_EQ(2, message.repeated_sfixed32_size());
  ASSERT_EQ(2, message.repeated_sfixed64_size());
  ASSERT_EQ(2, message.repeated_float_size());
  ASSERT_EQ(2, message.repeated_double_size());
  ASSERT_EQ(2, message.repeated_bool_size());
  ASSERT_EQ(2, message.repeated_string_size());
  ASSERT_EQ(2, message.repeated_bytes_size());

  ASSERT_EQ(2, message.repeatedgroup_size());
  ASSERT_EQ(2, message.repeated_nested_message_size());
  ASSERT_EQ(2, message.repeated_foreign_message_size());
  ASSERT_EQ(2, message.repeated_import_message_size());
  ASSERT_EQ(2, message.repeated_lazy_message_size());
  ASSERT_EQ(2, message.repeated_nested_enum_size());
  ASSERT_EQ(2, message.repeated_foreign_enum_size());
  ASSERT_EQ(2, message.repeated_import_enum_size());

#ifndef PROTOBUF_TEST_NO_DESCRIPTORS
  ASSERT_EQ(2, message.repeated_string_piece_size());
  ASSERT_EQ(2, message.repeated_cord_size());
#endif

  EXPECT_EQ(201, message.repeated_int32(0));
  EXPECT_EQ(202, message.repeated_int64(0));
  EXPECT_EQ(203, message.repeated_uint32(0));
  EXPECT_EQ(204, message.repeated_uint64(0));
  EXPECT_EQ(205, message.repeated_sint32(0));
  EXPECT_EQ(206, message.repeated_sint64(0));
  EXPECT_EQ(207, message.repeated_fixed32(0));
  EXPECT_EQ(208, message.repeated_fixed64(0));
  EXPECT_EQ(209, message.repeated_sfixed32(0));
  EXPECT_EQ(210, message.repeated_sfixed64(0));
  EXPECT_EQ(211, message.repeated_float(0));
  EXPECT_EQ(212, message.repeated_double(0));
  EXPECT_TRUE(message.repeated_bool(0));
  EXPECT_EQ("215", message.repeated_string(0));
  EXPECT_EQ("216", message.repeated_bytes(0));

  EXPECT_EQ(217, message.repeatedgroup(0).a());
  EXPECT_EQ(218, message.repeated_nested_message(0).bb());
  EXPECT_EQ(219, message.repeated_foreign_message(0).c());
  EXPECT_EQ(220, message.repeated_import_message(0).d());
  EXPECT_EQ(227, message.repeated_lazy_message(0).bb());


  EXPECT_EQ(TestAllTypes::BAR, message.repeated_nested_enum(0));
  EXPECT_EQ(ForeignEnum<TestAllTypes>::FOREIGN_BAR,
            message.repeated_foreign_enum(0));
  EXPECT_EQ(ImportEnum<TestAllTypes>::IMPORT_BAR,
            message.repeated_import_enum(0));

  EXPECT_EQ(301, message.repeated_int32(1));
  EXPECT_EQ(302, message.repeated_int64(1));
  EXPECT_EQ(303, message.repeated_uint32(1));
  EXPECT_EQ(304, message.repeated_uint64(1));
  EXPECT_EQ(305, message.repeated_sint32(1));
  EXPECT_EQ(306, message.repeated_sint64(1));
  EXPECT_EQ(307, message.repeated_fixed32(1));
  EXPECT_EQ(308, message.repeated_fixed64(1));
  EXPECT_EQ(309, message.repeated_sfixed32(1));
  EXPECT_EQ(310, message.repeated_sfixed64(1));
  EXPECT_EQ(311, message.repeated_float(1));
  EXPECT_EQ(312, message.repeated_double(1));
  EXPECT_FALSE(message.repeated_bool(1));
  EXPECT_EQ("315", message.repeated_string(1));
  EXPECT_EQ("316", message.repeated_bytes(1));

  EXPECT_EQ(317, message.repeatedgroup(1).a());
  EXPECT_EQ(318, message.repeated_nested_message(1).bb());
  EXPECT_EQ(319, message.repeated_foreign_message(1).c());
  EXPECT_EQ(320, message.repeated_import_message(1).d());
  EXPECT_EQ(327, message.repeated_lazy_message(1).bb());

  EXPECT_EQ(TestAllTypes::BAZ, message.repeated_nested_enum(1));
  EXPECT_EQ(ForeignEnum<TestAllTypes>::FOREIGN_BAZ,
            message.repeated_foreign_enum(1));
  EXPECT_EQ(ImportEnum<TestAllTypes>::IMPORT_BAZ,
            message.repeated_import_enum(1));


  // -----------------------------------------------------------------

  EXPECT_TRUE(message.has_default_int32());
  EXPECT_TRUE(message.has_default_int64());
  EXPECT_TRUE(message.has_default_uint32());
  EXPECT_TRUE(message.has_default_uint64());
  EXPECT_TRUE(message.has_default_sint32());
  EXPECT_TRUE(message.has_default_sint64());
  EXPECT_TRUE(message.has_default_fixed32());
  EXPECT_TRUE(message.has_default_fixed64());
  EXPECT_TRUE(message.has_default_sfixed32());
  EXPECT_TRUE(message.has_default_sfixed64());
  EXPECT_TRUE(message.has_default_float());
  EXPECT_TRUE(message.has_default_double());
  EXPECT_TRUE(message.has_default_bool());
  EXPECT_TRUE(message.has_default_string());
  EXPECT_TRUE(message.has_default_bytes());

  EXPECT_TRUE(message.has_default_nested_enum());
  EXPECT_TRUE(message.has_default_foreign_enum());
  EXPECT_TRUE(message.has_default_import_enum());


  EXPECT_EQ(401, message.default_int32());
  EXPECT_EQ(402, message.default_int64());
  EXPECT_EQ(403, message.default_uint32());
  EXPECT_EQ(404, message.default_uint64());
  EXPECT_EQ(405, message.default_sint32());
  EXPECT_EQ(406, message.default_sint64());
  EXPECT_EQ(407, message.default_fixed32());
  EXPECT_EQ(408, message.default_fixed64());
  EXPECT_EQ(409, message.default_sfixed32());
  EXPECT_EQ(410, message.default_sfixed64());
  EXPECT_EQ(411, message.default_float());
  EXPECT_EQ(412, message.default_double());
  EXPECT_FALSE(message.default_bool());
  EXPECT_EQ("415", message.default_string());
  EXPECT_EQ("416", message.default_bytes());

  EXPECT_EQ(TestAllTypes::FOO, message.default_nested_enum());
  EXPECT_EQ(ForeignEnum<TestAllTypes>::FOREIGN_FOO,
            message.default_foreign_enum());
  EXPECT_EQ(ImportEnum<TestAllTypes>::IMPORT_FOO,
            message.default_import_enum());


  EXPECT_FALSE(message.has_oneof_uint32());
  EXPECT_FALSE(message.has_oneof_nested_message());
  EXPECT_FALSE(message.has_oneof_string());
  EXPECT_TRUE(message.has_oneof_bytes());

  EXPECT_EQ("604", message.oneof_bytes());
}

template <typename TestAllTypes>
void ExpectClear(const TestAllTypes& message) {
  // has_blah() should initially be false for all optional fields.
  EXPECT_FALSE(message.has_optional_int32());
  EXPECT_FALSE(message.has_optional_int64());
  EXPECT_FALSE(message.has_optional_uint32());
  EXPECT_FALSE(message.has_optional_uint64());
  EXPECT_FALSE(message.has_optional_sint32());
  EXPECT_FALSE(message.has_optional_sint64());
  EXPECT_FALSE(message.has_optional_fixed32());
  EXPECT_FALSE(message.has_optional_fixed64());
  EXPECT_FALSE(message.has_optional_sfixed32());
  EXPECT_FALSE(message.has_optional_sfixed64());
  EXPECT_FALSE(message.has_optional_float());
  EXPECT_FALSE(message.has_optional_double());
  EXPECT_FALSE(message.has_optional_bool());
  EXPECT_FALSE(message.has_optional_string());
  EXPECT_FALSE(message.has_optional_bytes());

  EXPECT_FALSE(message.has_optionalgroup());
  EXPECT_FALSE(message.has_optional_nested_message());
  EXPECT_FALSE(message.has_optional_foreign_message());
  EXPECT_FALSE(message.has_optional_import_message());
  EXPECT_FALSE(message.has_optional_public_import_message());
  EXPECT_FALSE(message.has_optional_lazy_message());
  EXPECT_FALSE(message.has_optional_unverified_lazy_message());

  EXPECT_FALSE(message.has_optional_nested_enum());
  EXPECT_FALSE(message.has_optional_foreign_enum());
  EXPECT_FALSE(message.has_optional_import_enum());

  EXPECT_FALSE(message.has_optional_string_piece());
  EXPECT_FALSE(message.has_optional_cord());
  EXPECT_FALSE(message.has_optional_bytes_cord());

  // Optional fields without defaults are set to zero or something like it.
  EXPECT_EQ(0, message.optional_int32());
  EXPECT_EQ(0, message.optional_int64());
  EXPECT_EQ(0, message.optional_uint32());
  EXPECT_EQ(0, message.optional_uint64());
  EXPECT_EQ(0, message.optional_sint32());
  EXPECT_EQ(0, message.optional_sint64());
  EXPECT_EQ(0, message.optional_fixed32());
  EXPECT_EQ(0, message.optional_fixed64());
  EXPECT_EQ(0, message.optional_sfixed32());
  EXPECT_EQ(0, message.optional_sfixed64());
  EXPECT_EQ(0, message.optional_float());
  EXPECT_EQ(0, message.optional_double());
  EXPECT_FALSE(message.optional_bool());
  EXPECT_EQ("", message.optional_string());
  EXPECT_EQ("", message.optional_bytes());

  // Embedded messages should also be clear.
  EXPECT_FALSE(message.optionalgroup().has_a());
  EXPECT_FALSE(message.optional_nested_message().has_bb());
  EXPECT_FALSE(message.optional_foreign_message().has_c());
  EXPECT_FALSE(message.optional_import_message().has_d());
  EXPECT_FALSE(message.optional_public_import_message().has_e());
  EXPECT_FALSE(message.optional_lazy_message().has_bb());
  EXPECT_FALSE(message.optional_unverified_lazy_message().has_bb());

  EXPECT_EQ(0, message.optionalgroup().a());
  EXPECT_EQ(0, message.optional_nested_message().bb());
  EXPECT_EQ(0, message.optional_foreign_message().c());
  EXPECT_EQ(0, message.optional_import_message().d());
  EXPECT_EQ(0, message.optional_public_import_message().e());
  EXPECT_EQ(0, message.optional_lazy_message().bb());
  EXPECT_EQ(0, message.optional_unverified_lazy_message().bb());

  // Enums without defaults are set to the first value in the enum.
  EXPECT_EQ(TestAllTypes::FOO, message.optional_nested_enum());
  EXPECT_EQ(ForeignEnum<TestAllTypes>::FOREIGN_FOO,
            message.optional_foreign_enum());
  EXPECT_EQ(ImportEnum<TestAllTypes>::IMPORT_FOO,
            message.optional_import_enum());

  EXPECT_EQ("", message.optional_bytes_cord());

  // Repeated fields are empty.
  EXPECT_EQ(0, message.repeated_int32_size());
  EXPECT_EQ(0, message.repeated_int64_size());
  EXPECT_EQ(0, message.repeated_uint32_size());
  EXPECT_EQ(0, message.repeated_uint64_size());
  EXPECT_EQ(0, message.repeated_sint32_size());
  EXPECT_EQ(0, message.repeated_sint64_size());
  EXPECT_EQ(0, message.repeated_fixed32_size());
  EXPECT_EQ(0, message.repeated_fixed64_size());
  EXPECT_EQ(0, message.repeated_sfixed32_size());
  EXPECT_EQ(0, message.repeated_sfixed64_size());
  EXPECT_EQ(0, message.repeated_float_size());
  EXPECT_EQ(0, message.repeated_double_size());
  EXPECT_EQ(0, message.repeated_bool_size());
  EXPECT_EQ(0, message.repeated_string_size());
  EXPECT_EQ(0, message.repeated_bytes_size());

  EXPECT_EQ(0, message.repeatedgroup_size());
  EXPECT_EQ(0, message.repeated_nested_message_size());
  EXPECT_EQ(0, message.repeated_foreign_message_size());
  EXPECT_EQ(0, message.repeated_import_message_size());
  EXPECT_EQ(0, message.repeated_lazy_message_size());
  EXPECT_EQ(0, message.repeated_nested_enum_size());
  EXPECT_EQ(0, message.repeated_foreign_enum_size());
  EXPECT_EQ(0, message.repeated_import_enum_size());

  EXPECT_EQ(0, message.repeated_string_piece_size());
  EXPECT_EQ(0, message.repeated_cord_size());

  // has_blah() should also be false for all default fields.
  EXPECT_FALSE(message.has_default_int32());
  EXPECT_FALSE(message.has_default_int64());
  EXPECT_FALSE(message.has_default_uint32());
  EXPECT_FALSE(message.has_default_uint64());
  EXPECT_FALSE(message.has_default_sint32());
  EXPECT_FALSE(message.has_default_sint64());
  EXPECT_FALSE(message.has_default_fixed32());
  EXPECT_FALSE(message.has_default_fixed64());
  EXPECT_FALSE(message.has_default_sfixed32());
  EXPECT_FALSE(message.has_default_sfixed64());
  EXPECT_FALSE(message.has_default_float());
  EXPECT_FALSE(message.has_default_double());
  EXPECT_FALSE(message.has_default_bool());
  EXPECT_FALSE(message.has_default_string());
  EXPECT_FALSE(message.has_default_bytes());

  EXPECT_FALSE(message.has_default_nested_enum());
  EXPECT_FALSE(message.has_default_foreign_enum());
  EXPECT_FALSE(message.has_default_import_enum());


  // Fields with defaults have their default values (duh).
  EXPECT_EQ(41, message.default_int32());
  EXPECT_EQ(42, message.default_int64());
  EXPECT_EQ(43, message.default_uint32());
  EXPECT_EQ(44, message.default_uint64());
  EXPECT_EQ(-45, message.default_sint32());
  EXPECT_EQ(46, message.default_sint64());
  EXPECT_EQ(47, message.default_fixed32());
  EXPECT_EQ(48, message.default_fixed64());
  EXPECT_EQ(49, message.default_sfixed32());
  EXPECT_EQ(-50, message.default_sfixed64());
  EXPECT_EQ(51.5, message.default_float());
  EXPECT_EQ(52e3, message.default_double());
  EXPECT_TRUE(message.default_bool());
  EXPECT_EQ("hello", message.default_string());
  EXPECT_EQ("world", message.default_bytes());

  EXPECT_EQ(TestAllTypes::BAR, message.default_nested_enum());
  EXPECT_EQ(ForeignEnum<TestAllTypes>::FOREIGN_BAR,
            message.default_foreign_enum());
  EXPECT_EQ(ImportEnum<TestAllTypes>::IMPORT_BAR,
            message.default_import_enum());


  EXPECT_FALSE(message.has_oneof_uint32());
  EXPECT_FALSE(message.has_oneof_nested_message());
  EXPECT_FALSE(message.has_oneof_string());
  EXPECT_FALSE(message.has_oneof_bytes());
}

template <typename TestAllTypes>
void ExpectRepeatedFieldsModified(const TestAllTypes& message) {
  // ModifyRepeatedFields only sets the second repeated element of each
  // field.  In addition to verifying this, we also verify that the first
  // element and size were *not* modified.
  ASSERT_EQ(2, message.repeated_int32_size());
  ASSERT_EQ(2, message.repeated_int64_size());
  ASSERT_EQ(2, message.repeated_uint32_size());
  ASSERT_EQ(2, message.repeated_uint64_size());
  ASSERT_EQ(2, message.repeated_sint32_size());
  ASSERT_EQ(2, message.repeated_sint64_size());
  ASSERT_EQ(2, message.repeated_fixed32_size());
  ASSERT_EQ(2, message.repeated_fixed64_size());
  ASSERT_EQ(2, message.repeated_sfixed32_size());
  ASSERT_EQ(2, message.repeated_sfixed64_size());
  ASSERT_EQ(2, message.repeated_float_size());
  ASSERT_EQ(2, message.repeated_double_size());
  ASSERT_EQ(2, message.repeated_bool_size());
  ASSERT_EQ(2, message.repeated_string_size());
  ASSERT_EQ(2, message.repeated_bytes_size());

  ASSERT_EQ(2, message.repeatedgroup_size());
  ASSERT_EQ(2, message.repeated_nested_message_size());
  ASSERT_EQ(2, message.repeated_foreign_message_size());
  ASSERT_EQ(2, message.repeated_import_message_size());
  ASSERT_EQ(2, message.repeated_lazy_message_size());
  ASSERT_EQ(2, message.repeated_nested_enum_size());
  ASSERT_EQ(2, message.repeated_foreign_enum_size());
  ASSERT_EQ(2, message.repeated_import_enum_size());

#ifndef PROTOBUF_TEST_NO_DESCRIPTORS
  ASSERT_EQ(2, message.repeated_string_piece_size());
  ASSERT_EQ(2, message.repeated_cord_size());
#endif

  EXPECT_EQ(201, message.repeated_int32(0));
  EXPECT_EQ(202, message.repeated_int64(0));
  EXPECT_EQ(203, message.repeated_uint32(0));
  EXPECT_EQ(204, message.repeated_uint64(0));
  EXPECT_EQ(205, message.repeated_sint32(0));
  EXPECT_EQ(206, message.repeated_sint64(0));
  EXPECT_EQ(207, message.repeated_fixed32(0));
  EXPECT_EQ(208, message.repeated_fixed64(0));
  EXPECT_EQ(209, message.repeated_sfixed32(0));
  EXPECT_EQ(210, message.repeated_sfixed64(0));
  EXPECT_EQ(211, message.repeated_float(0));
  EXPECT_EQ(212, message.repeated_double(0));
  EXPECT_TRUE(message.repeated_bool(0));
  EXPECT_EQ("215", message.repeated_string(0));
  EXPECT_EQ("216", message.repeated_bytes(0));

  EXPECT_EQ(217, message.repeatedgroup(0).a());
  EXPECT_EQ(218, message.repeated_nested_message(0).bb());
  EXPECT_EQ(219, message.repeated_foreign_message(0).c());
  EXPECT_EQ(220, message.repeated_import_message(0).d());
  EXPECT_EQ(227, message.repeated_lazy_message(0).bb());

  EXPECT_EQ(TestAllTypes::BAR, message.repeated_nested_enum(0));
  EXPECT_EQ(ForeignEnum<TestAllTypes>::FOREIGN_BAR,
            message.repeated_foreign_enum(0));
  EXPECT_EQ(ImportEnum<TestAllTypes>::IMPORT_BAR,
            message.repeated_import_enum(0));


  // Actually verify the second (modified) elements now.
  EXPECT_EQ(501, message.repeated_int32(1));
  EXPECT_EQ(502, message.repeated_int64(1));
  EXPECT_EQ(503, message.repeated_uint32(1));
  EXPECT_EQ(504, message.repeated_uint64(1));
  EXPECT_EQ(505, message.repeated_sint32(1));
  EXPECT_EQ(506, message.repeated_sint64(1));
  EXPECT_EQ(507, message.repeated_fixed32(1));
  EXPECT_EQ(508, message.repeated_fixed64(1));
  EXPECT_EQ(509, message.repeated_sfixed32(1));
  EXPECT_EQ(510, message.repeated_sfixed64(1));
  EXPECT_EQ(511, message.repeated_float(1));
  EXPECT_EQ(512, message.repeated_double(1));
  EXPECT_TRUE(message.repeated_bool(1));
  EXPECT_EQ("515", message.repeated_string(1));
  EXPECT_EQ("516", message.repeated_bytes(1));

  EXPECT_EQ(517, message.repeatedgroup(1).a());
  EXPECT_EQ(518, message.repeated_nested_message(1).bb());
  EXPECT_EQ(519, message.repeated_foreign_message(1).c());
  EXPECT_EQ(520, message.repeated_import_message(1).d());
  EXPECT_EQ(527, message.repeated_lazy_message(1).bb());

  EXPECT_EQ(TestAllTypes::FOO, message.repeated_nested_enum(1));
  EXPECT_EQ(ForeignEnum<TestAllTypes>::FOREIGN_FOO,
            message.repeated_foreign_enum(1));
  EXPECT_EQ(ImportEnum<TestAllTypes>::IMPORT_FOO,
            message.repeated_import_enum(1));

}

template <typename TestAllTypes>
void ExpectLastRepeatedsRemoved(const TestAllTypes& message) {
  ASSERT_EQ(1, message.repeated_int32_size());
  ASSERT_EQ(1, message.repeated_int64_size());
  ASSERT_EQ(1, message.repeated_uint32_size());
  ASSERT_EQ(1, message.repeated_uint64_size());
  ASSERT_EQ(1, message.repeated_sint32_size());
  ASSERT_EQ(1, message.repeated_sint64_size());
  ASSERT_EQ(1, message.repeated_fixed32_size());
  ASSERT_EQ(1, message.repeated_fixed64_size());
  ASSERT_EQ(1, message.repeated_sfixed32_size());
  ASSERT_EQ(1, message.repeated_sfixed64_size());
  ASSERT_EQ(1, message.repeated_float_size());
  ASSERT_EQ(1, message.repeated_double_size());
  ASSERT_EQ(1, message.repeated_bool_size());
  ASSERT_EQ(1, message.repeated_string_size());
  ASSERT_EQ(1, message.repeated_bytes_size());

  ASSERT_EQ(1, message.repeatedgroup_size());
  ASSERT_EQ(1, message.repeated_nested_message_size());
  ASSERT_EQ(1, message.repeated_foreign_message_size());
  ASSERT_EQ(1, message.repeated_import_message_size());
  ASSERT_EQ(1, message.repeated_import_message_size());
  ASSERT_EQ(1, message.repeated_nested_enum_size());
  ASSERT_EQ(1, message.repeated_foreign_enum_size());
  ASSERT_EQ(1, message.repeated_import_enum_size());

#ifndef PROTOBUF_TEST_NO_DESCRIPTORS
  ASSERT_EQ(1, message.repeated_string_piece_size());
  ASSERT_EQ(1, message.repeated_cord_size());
#endif

  // Test that the remaining element is the correct one.
  EXPECT_EQ(201, message.repeated_int32(0));
  EXPECT_EQ(202, message.repeated_int64(0));
  EXPECT_EQ(203, message.repeated_uint32(0));
  EXPECT_EQ(204, message.repeated_uint64(0));
  EXPECT_EQ(205, message.repeated_sint32(0));
  EXPECT_EQ(206, message.repeated_sint64(0));
  EXPECT_EQ(207, message.repeated_fixed32(0));
  EXPECT_EQ(208, message.repeated_fixed64(0));
  EXPECT_EQ(209, message.repeated_sfixed32(0));
  EXPECT_EQ(210, message.repeated_sfixed64(0));
  EXPECT_EQ(211, message.repeated_float(0));
  EXPECT_EQ(212, message.repeated_double(0));
  EXPECT_TRUE(message.repeated_bool(0));
  EXPECT_EQ("215", message.repeated_string(0));
  EXPECT_EQ("216", message.repeated_bytes(0));

  EXPECT_EQ(217, message.repeatedgroup(0).a());
  EXPECT_EQ(218, message.repeated_nested_message(0).bb());
  EXPECT_EQ(219, message.repeated_foreign_message(0).c());
  EXPECT_EQ(220, message.repeated_import_message(0).d());
  EXPECT_EQ(220, message.repeated_import_message(0).d());

  EXPECT_EQ(TestAllTypes::BAR, message.repeated_nested_enum(0));
  EXPECT_EQ(ForeignEnum<TestAllTypes>::FOREIGN_BAR,
            message.repeated_foreign_enum(0));
  EXPECT_EQ(ImportEnum<TestAllTypes>::IMPORT_BAR,
            message.repeated_import_enum(0));
}

template <typename TestAllTypes>
void ExpectRepeatedsSwapped(const TestAllTypes& message) {
  ASSERT_EQ(2, message.repeated_int32_size());
  ASSERT_EQ(2, message.repeated_int64_size());
  ASSERT_EQ(2, message.repeated_uint32_size());
  ASSERT_EQ(2, message.repeated_uint64_size());
  ASSERT_EQ(2, message.repeated_sint32_size());
  ASSERT_EQ(2, message.repeated_sint64_size());
  ASSERT_EQ(2, message.repeated_fixed32_size());
  ASSERT_EQ(2, message.repeated_fixed64_size());
  ASSERT_EQ(2, message.repeated_sfixed32_size());
  ASSERT_EQ(2, message.repeated_sfixed64_size());
  ASSERT_EQ(2, message.repeated_float_size());
  ASSERT_EQ(2, message.repeated_double_size());
  ASSERT_EQ(2, message.repeated_bool_size());
  ASSERT_EQ(2, message.repeated_string_size());
  ASSERT_EQ(2, message.repeated_bytes_size());

  ASSERT_EQ(2, message.repeatedgroup_size());
  ASSERT_EQ(2, message.repeated_nested_message_size());
  ASSERT_EQ(2, message.repeated_foreign_message_size());
  ASSERT_EQ(2, message.repeated_import_message_size());
  ASSERT_EQ(2, message.repeated_import_message_size());
  ASSERT_EQ(2, message.repeated_nested_enum_size());
  ASSERT_EQ(2, message.repeated_foreign_enum_size());
  ASSERT_EQ(2, message.repeated_import_enum_size());

#ifndef PROTOBUF_TEST_NO_DESCRIPTORS
  ASSERT_EQ(2, message.repeated_string_piece_size());
  ASSERT_EQ(2, message.repeated_cord_size());
#endif

  // Test that the first element and second element are flipped.
  EXPECT_EQ(201, message.repeated_int32(1));
  EXPECT_EQ(202, message.repeated_int64(1));
  EXPECT_EQ(203, message.repeated_uint32(1));
  EXPECT_EQ(204, message.repeated_uint64(1));
  EXPECT_EQ(205, message.repeated_sint32(1));
  EXPECT_EQ(206, message.repeated_sint64(1));
  EXPECT_EQ(207, message.repeated_fixed32(1));
  EXPECT_EQ(208, message.repeated_fixed64(1));
  EXPECT_EQ(209, message.repeated_sfixed32(1));
  EXPECT_EQ(210, message.repeated_sfixed64(1));
  EXPECT_EQ(211, message.repeated_float(1));
  EXPECT_EQ(212, message.repeated_double(1));
  EXPECT_TRUE(message.repeated_bool(1));
  EXPECT_EQ("215", message.repeated_string(1));
  EXPECT_EQ("216", message.repeated_bytes(1));

  EXPECT_EQ(217, message.repeatedgroup(1).a());
  EXPECT_EQ(218, message.repeated_nested_message(1).bb());
  EXPECT_EQ(219, message.repeated_foreign_message(1).c());
  EXPECT_EQ(220, message.repeated_import_message(1).d());
  EXPECT_EQ(220, message.repeated_import_message(1).d());

  EXPECT_EQ(TestAllTypes::BAR, message.repeated_nested_enum(1));
  EXPECT_EQ(ForeignEnum<TestAllTypes>::FOREIGN_BAR,
            message.repeated_foreign_enum(1));
  EXPECT_EQ(ImportEnum<TestAllTypes>::IMPORT_BAR,
            message.repeated_import_enum(1));

  EXPECT_EQ(301, message.repeated_int32(0));
  EXPECT_EQ(302, message.repeated_int64(0));
  EXPECT_EQ(303, message.repeated_uint32(0));
  EXPECT_EQ(304, message.repeated_uint64(0));
  EXPECT_EQ(305, message.repeated_sint32(0));
  EXPECT_EQ(306, message.repeated_sint64(0));
  EXPECT_EQ(307, message.repeated_fixed32(0));
  EXPECT_EQ(308, message.repeated_fixed64(0));
  EXPECT_EQ(309, message.repeated_sfixed32(0));
  EXPECT_EQ(310, message.repeated_sfixed64(0));
  EXPECT_EQ(311, message.repeated_float(0));
  EXPECT_EQ(312, message.repeated_double(0));
  EXPECT_FALSE(message.repeated_bool(0));
  EXPECT_EQ("315", message.repeated_string(0));
  EXPECT_EQ("316", message.repeated_bytes(0));

  EXPECT_EQ(317, message.repeatedgroup(0).a());
  EXPECT_EQ(318, message.repeated_nested_message(0).bb());
  EXPECT_EQ(319, message.repeated_foreign_message(0).c());
  EXPECT_EQ(320, message.repeated_import_message(0).d());
  EXPECT_EQ(320, message.repeated_import_message(0).d());

  EXPECT_EQ(TestAllTypes::BAZ, message.repeated_nested_enum(0));
  EXPECT_EQ(ForeignEnum<TestAllTypes>::FOREIGN_BAZ,
            message.repeated_foreign_enum(0));
  EXPECT_EQ(ImportEnum<TestAllTypes>::IMPORT_BAZ,
            message.repeated_import_enum(0));
}

template <typename TestPackedTypes>
void SetPackedFields(TestPackedTypes* message) {
  message->add_packed_int32(601);
  message->add_packed_int64(602);
  message->add_packed_uint32(603);
  message->add_packed_uint64(604);
  message->add_packed_sint32(605);
  message->add_packed_sint64(606);
  message->add_packed_fixed32(607);
  message->add_packed_fixed64(608);
  message->add_packed_sfixed32(609);
  message->add_packed_sfixed64(610);
  message->add_packed_float(611);
  message->add_packed_double(612);
  message->add_packed_bool(true);
  message->add_packed_enum(ForeignEnum<TestPackedTypes>::FOREIGN_BAR);
  // add a second one of each field
  message->add_packed_int32(701);
  message->add_packed_int64(702);
  message->add_packed_uint32(703);
  message->add_packed_uint64(704);
  message->add_packed_sint32(705);
  message->add_packed_sint64(706);
  message->add_packed_fixed32(707);
  message->add_packed_fixed64(708);
  message->add_packed_sfixed32(709);
  message->add_packed_sfixed64(710);
  message->add_packed_float(711);
  message->add_packed_double(712);
  message->add_packed_bool(false);
  message->add_packed_enum(ForeignEnum<TestPackedTypes>::FOREIGN_BAZ);
}

template <typename TestPackedTypes>
void ExpectPackedFieldsSet(const TestPackedTypes& message) {
  ASSERT_EQ(2, message.packed_int32_size());
  ASSERT_EQ(2, message.packed_int64_size());
  ASSERT_EQ(2, message.packed_uint32_size());
  ASSERT_EQ(2, message.packed_uint64_size());
  ASSERT_EQ(2, message.packed_sint32_size());
  ASSERT_EQ(2, message.packed_sint64_size());
  ASSERT_EQ(2, message.packed_fixed32_size());
  ASSERT_EQ(2, message.packed_fixed64_size());
  ASSERT_EQ(2, message.packed_sfixed32_size());
  ASSERT_EQ(2, message.packed_sfixed64_size());
  ASSERT_EQ(2, message.packed_float_size());
  ASSERT_EQ(2, message.packed_double_size());
  ASSERT_EQ(2, message.packed_bool_size());
  ASSERT_EQ(2, message.packed_enum_size());

  EXPECT_EQ(601, message.packed_int32(0));
  EXPECT_EQ(602, message.packed_int64(0));
  EXPECT_EQ(603, message.packed_uint32(0));
  EXPECT_EQ(604, message.packed_uint64(0));
  EXPECT_EQ(605, message.packed_sint32(0));
  EXPECT_EQ(606, message.packed_sint64(0));
  EXPECT_EQ(607, message.packed_fixed32(0));
  EXPECT_EQ(608, message.packed_fixed64(0));
  EXPECT_EQ(609, message.packed_sfixed32(0));
  EXPECT_EQ(610, message.packed_sfixed64(0));
  EXPECT_EQ(611, message.packed_float(0));
  EXPECT_EQ(612, message.packed_double(0));
  EXPECT_TRUE(message.packed_bool(0));
  EXPECT_EQ(ForeignEnum<TestPackedTypes>::FOREIGN_BAR, message.packed_enum(0));

  EXPECT_EQ(701, message.packed_int32(1));
  EXPECT_EQ(702, message.packed_int64(1));
  EXPECT_EQ(703, message.packed_uint32(1));
  EXPECT_EQ(704, message.packed_uint64(1));
  EXPECT_EQ(705, message.packed_sint32(1));
  EXPECT_EQ(706, message.packed_sint64(1));
  EXPECT_EQ(707, message.packed_fixed32(1));
  EXPECT_EQ(708, message.packed_fixed64(1));
  EXPECT_EQ(709, message.packed_sfixed32(1));
  EXPECT_EQ(710, message.packed_sfixed64(1));
  EXPECT_EQ(711, message.packed_float(1));
  EXPECT_EQ(712, message.packed_double(1));
  EXPECT_FALSE(message.packed_bool(1));
  EXPECT_EQ(ForeignEnum<TestPackedTypes>::FOREIGN_BAZ, message.packed_enum(1));
}

template <typename TestUnpackedTypes>
void SetUnpackedFields(TestUnpackedTypes* message) {
  // The values applied here must match those of SetPackedFields.

  message->add_unpacked_int32(601);
  message->add_unpacked_int64(602);
  message->add_unpacked_uint32(603);
  message->add_unpacked_uint64(604);
  message->add_unpacked_sint32(605);
  message->add_unpacked_sint64(606);
  message->add_unpacked_fixed32(607);
  message->add_unpacked_fixed64(608);
  message->add_unpacked_sfixed32(609);
  message->add_unpacked_sfixed64(610);
  message->add_unpacked_float(611);
  message->add_unpacked_double(612);
  message->add_unpacked_bool(true);
  message->add_unpacked_enum(ForeignEnum<TestUnpackedTypes>::FOREIGN_BAR);
  // add a second one of each field
  message->add_unpacked_int32(701);
  message->add_unpacked_int64(702);
  message->add_unpacked_uint32(703);
  message->add_unpacked_uint64(704);
  message->add_unpacked_sint32(705);
  message->add_unpacked_sint64(706);
  message->add_unpacked_fixed32(707);
  message->add_unpacked_fixed64(708);
  message->add_unpacked_sfixed32(709);
  message->add_unpacked_sfixed64(710);
  message->add_unpacked_float(711);
  message->add_unpacked_double(712);
  message->add_unpacked_bool(false);
  message->add_unpacked_enum(ForeignEnum<TestUnpackedTypes>::FOREIGN_BAZ);
}

// -------------------------------------------------------------------

template <typename TestUnpackedTypes>
void ExpectUnpackedFieldsSet(const TestUnpackedTypes& message) {
  // The values expected here must match those of ExpectPackedFieldsSet.

  ASSERT_EQ(2, message.unpacked_int32_size());
  ASSERT_EQ(2, message.unpacked_int64_size());
  ASSERT_EQ(2, message.unpacked_uint32_size());
  ASSERT_EQ(2, message.unpacked_uint64_size());
  ASSERT_EQ(2, message.unpacked_sint32_size());
  ASSERT_EQ(2, message.unpacked_sint64_size());
  ASSERT_EQ(2, message.unpacked_fixed32_size());
  ASSERT_EQ(2, message.unpacked_fixed64_size());
  ASSERT_EQ(2, message.unpacked_sfixed32_size());
  ASSERT_EQ(2, message.unpacked_sfixed64_size());
  ASSERT_EQ(2, message.unpacked_float_size());
  ASSERT_EQ(2, message.unpacked_double_size());
  ASSERT_EQ(2, message.unpacked_bool_size());
  ASSERT_EQ(2, message.unpacked_enum_size());

  EXPECT_EQ(601, message.unpacked_int32(0));
  EXPECT_EQ(602, message.unpacked_int64(0));
  EXPECT_EQ(603, message.unpacked_uint32(0));
  EXPECT_EQ(604, message.unpacked_uint64(0));
  EXPECT_EQ(605, message.unpacked_sint32(0));
  EXPECT_EQ(606, message.unpacked_sint64(0));
  EXPECT_EQ(607, message.unpacked_fixed32(0));
  EXPECT_EQ(608, message.unpacked_fixed64(0));
  EXPECT_EQ(609, message.unpacked_sfixed32(0));
  EXPECT_EQ(610, message.unpacked_sfixed64(0));
  EXPECT_EQ(611, message.unpacked_float(0));
  EXPECT_EQ(612, message.unpacked_double(0));
  EXPECT_TRUE(message.unpacked_bool(0));
  EXPECT_EQ(ForeignEnum<TestUnpackedTypes>::FOREIGN_BAR,
            message.unpacked_enum(0));

  EXPECT_EQ(701, message.unpacked_int32(1));
  EXPECT_EQ(702, message.unpacked_int64(1));
  EXPECT_EQ(703, message.unpacked_uint32(1));
  EXPECT_EQ(704, message.unpacked_uint64(1));
  EXPECT_EQ(705, message.unpacked_sint32(1));
  EXPECT_EQ(706, message.unpacked_sint64(1));
  EXPECT_EQ(707, message.unpacked_fixed32(1));
  EXPECT_EQ(708, message.unpacked_fixed64(1));
  EXPECT_EQ(709, message.unpacked_sfixed32(1));
  EXPECT_EQ(710, message.unpacked_sfixed64(1));
  EXPECT_EQ(711, message.unpacked_float(1));
  EXPECT_EQ(712, message.unpacked_double(1));
  EXPECT_FALSE(message.unpacked_bool(1));
  EXPECT_EQ(ForeignEnum<TestUnpackedTypes>::FOREIGN_BAZ,
            message.unpacked_enum(1));
}

template <typename TestOneof2>
void ExpectAtMostOneFieldSetInOneof(const TestOneof2& message) {
  int count = 0;
  if (message.has_foo_int()) count++;
  if (message.has_foo_string()) count++;
  if (message.has_foo_bytes()) count++;
  if (message.has_foo_enum()) count++;
  if (message.has_foo_message()) count++;
  if (message.has_foogroup()) count++;
  if (message.has_foo_lazy_message()) count++;
  if (message.has_foo_bytes_cord()) count++;
  EXPECT_LE(count, 1);
  count = 0;
  if (message.has_bar_int()) count++;
  if (message.has_bar_string()) count++;
  if (message.has_bar_bytes()) count++;
  if (message.has_bar_enum()) count++;
  EXPECT_TRUE(count == 0 || count == 1);
}

template <typename TestOneof2>
void SetOneof1(TestOneof2* message) {
  message->mutable_foo_lazy_message()->set_moo_int(100);
  message->set_bar_string("101");
  message->set_baz_int(102);
  message->set_baz_string("103");
}

template <typename TestOneof2>
void SetOneof2(TestOneof2* message) {
  message->set_foo_int(200);
  message->set_bar_enum(TestOneof2::BAZ);
  message->set_baz_int(202);
  message->set_baz_string("203");
}

template <typename TestOneof2>
void ExpectOneofSet1(const TestOneof2& message) {
  ExpectAtMostOneFieldSetInOneof(message);

  EXPECT_TRUE(message.has_foo_lazy_message());
  EXPECT_TRUE(message.foo_lazy_message().has_moo_int());

  EXPECT_TRUE(message.has_bar_string());
  EXPECT_TRUE(message.has_baz_int());
  EXPECT_TRUE(message.has_baz_string());

  ASSERT_EQ(0, message.foo_lazy_message().corge_int_size());

  EXPECT_EQ(100, message.foo_lazy_message().moo_int());
  EXPECT_EQ("101", message.bar_string());
  EXPECT_EQ(102, message.baz_int());
  EXPECT_EQ("103", message.baz_string());
}

template <typename TestOneof2>
void ExpectOneofSet2(const TestOneof2& message) {
  ExpectAtMostOneFieldSetInOneof(message);

  EXPECT_TRUE(message.has_foo_int());
  EXPECT_TRUE(message.has_bar_enum());
  EXPECT_TRUE(message.has_baz_int());
  EXPECT_TRUE(message.has_baz_string());

  EXPECT_EQ(200, message.foo_int());
  EXPECT_EQ(TestOneof2::BAZ, message.bar_enum());
  EXPECT_EQ(202, message.baz_int());
  EXPECT_EQ("203", message.baz_string());
}

template <typename TestOneof2>
void ExpectOneofClear(const TestOneof2& message) {
  EXPECT_FALSE(message.has_foo_int());
  EXPECT_FALSE(message.has_foo_string());
  EXPECT_FALSE(message.has_foo_bytes());
  EXPECT_FALSE(message.has_foo_enum());
  EXPECT_FALSE(message.has_foo_message());
  EXPECT_FALSE(message.has_foogroup());
  EXPECT_FALSE(message.has_foo_lazy_message());
  EXPECT_FALSE(message.has_foo_bytes_cord());

  EXPECT_FALSE(message.has_bar_int());
  EXPECT_FALSE(message.has_bar_string());
  EXPECT_FALSE(message.has_bar_bytes());
  EXPECT_FALSE(message.has_bar_enum());

  EXPECT_FALSE(message.has_baz_int());
  EXPECT_FALSE(message.has_baz_string());

  EXPECT_EQ(TestOneof2::FOO_NOT_SET, message.foo_case());
  EXPECT_EQ(TestOneof2::BAR_NOT_SET, message.bar_case());
}

template <typename TestAllExtensions>
void SetAllExtensions(TestAllExtensions* message) {
  using Traits = TestUtilTraits<TestAllExtensions>;
  message->SetExtension(Traits::optional_int32_extension, 101);
  message->SetExtension(Traits::optional_int64_extension, 102);
  message->SetExtension(Traits::optional_uint32_extension, 103);
  message->SetExtension(Traits::optional_uint64_extension, 104);
  message->SetExtension(Traits::optional_sint32_extension, 105);
  message->SetExtension(Traits::optional_sint64_extension, 106);
  message->SetExtension(Traits::optional_fixed32_extension, 107);
  message->SetExtension(Traits::optional_fixed64_extension, 108);
  message->SetExtension(Traits::optional_sfixed32_extension, 109);
  message->SetExtension(Traits::optional_sfixed64_extension, 110);
  message->SetExtension(Traits::optional_float_extension, 111);
  message->SetExtension(Traits::optional_double_extension, 112);
  message->SetExtension(Traits::optional_bool_extension, true);
  message->SetExtension(Traits::optional_string_extension, "115");
  message->SetExtension(Traits::optional_bytes_extension, "116");

  message->MutableExtension(Traits::optionalgroup_extension)->set_a(117);
  message->MutableExtension(Traits::optional_nested_message_extension)
      ->set_bb(118);
  message->MutableExtension(Traits::optional_foreign_message_extension)
      ->set_c(119);
  message->MutableExtension(Traits::optional_import_message_extension)
      ->set_d(120);

  message->SetExtension(Traits::optional_nested_enum_extension,
                        Traits::TestAllTypes::BAZ);
  message->SetExtension(Traits::optional_foreign_enum_extension,
                        Traits::FOREIGN_BAZ);
  message->SetExtension(Traits::optional_import_enum_extension,
                        Traits::IMPORT_BAZ);

  message->SetExtension(Traits::optional_string_piece_extension, "124");
  message->SetExtension(Traits::optional_cord_extension, "125");
  message->SetExtension(Traits::optional_bytes_cord_extension,
                        "optional bytes cord");

  message->MutableExtension(Traits::optional_public_import_message_extension)
      ->set_e(126);
  message->MutableExtension(Traits::optional_lazy_message_extension)
      ->set_bb(127);
  message->MutableExtension(Traits::optional_unverified_lazy_message_extension)
      ->set_bb(128);

  // -----------------------------------------------------------------

  message->AddExtension(Traits::repeated_int32_extension, 201);
  message->AddExtension(Traits::repeated_int64_extension, 202);
  message->AddExtension(Traits::repeated_uint32_extension, 203);
  message->AddExtension(Traits::repeated_uint64_extension, 204);
  message->AddExtension(Traits::repeated_sint32_extension, 205);
  message->AddExtension(Traits::repeated_sint64_extension, 206);
  message->AddExtension(Traits::repeated_fixed32_extension, 207);
  message->AddExtension(Traits::repeated_fixed64_extension, 208);
  message->AddExtension(Traits::repeated_sfixed32_extension, 209);
  message->AddExtension(Traits::repeated_sfixed64_extension, 210);
  message->AddExtension(Traits::repeated_float_extension, 211);
  message->AddExtension(Traits::repeated_double_extension, 212);
  message->AddExtension(Traits::repeated_bool_extension, true);
  message->AddExtension(Traits::repeated_string_extension, "215");
  message->AddExtension(Traits::repeated_bytes_extension, "216");

  message->AddExtension(Traits::repeatedgroup_extension)->set_a(217);
  message->AddExtension(Traits::repeated_nested_message_extension)->set_bb(218);
  message->AddExtension(Traits::repeated_foreign_message_extension)->set_c(219);
  message->AddExtension(Traits::repeated_import_message_extension)->set_d(220);
  message->AddExtension(Traits::repeated_lazy_message_extension)->set_bb(227);

  message->AddExtension(Traits::repeated_nested_enum_extension,
                        Traits::TestAllTypes::BAR);
  message->AddExtension(Traits::repeated_foreign_enum_extension,
                        Traits::FOREIGN_BAR);
  message->AddExtension(Traits::repeated_import_enum_extension,
                        Traits::IMPORT_BAR);

  message->AddExtension(Traits::repeated_string_piece_extension, "224");
  message->AddExtension(Traits::repeated_cord_extension, "225");

  // Add a second one of each field.
  message->AddExtension(Traits::repeated_int32_extension, 301);
  message->AddExtension(Traits::repeated_int64_extension, 302);
  message->AddExtension(Traits::repeated_uint32_extension, 303);
  message->AddExtension(Traits::repeated_uint64_extension, 304);
  message->AddExtension(Traits::repeated_sint32_extension, 305);
  message->AddExtension(Traits::repeated_sint64_extension, 306);
  message->AddExtension(Traits::repeated_fixed32_extension, 307);
  message->AddExtension(Traits::repeated_fixed64_extension, 308);
  message->AddExtension(Traits::repeated_sfixed32_extension, 309);
  message->AddExtension(Traits::repeated_sfixed64_extension, 310);
  message->AddExtension(Traits::repeated_float_extension, 311);
  message->AddExtension(Traits::repeated_double_extension, 312);
  message->AddExtension(Traits::repeated_bool_extension, false);
  message->AddExtension(Traits::repeated_string_extension, "315");
  message->AddExtension(Traits::repeated_bytes_extension, "316");

  message->AddExtension(Traits::repeatedgroup_extension)->set_a(317);
  message->AddExtension(Traits::repeated_nested_message_extension)->set_bb(318);
  message->AddExtension(Traits::repeated_foreign_message_extension)->set_c(319);
  message->AddExtension(Traits::repeated_import_message_extension)->set_d(320);
  message->AddExtension(Traits::repeated_lazy_message_extension)->set_bb(327);

  message->AddExtension(Traits::repeated_nested_enum_extension,
                        Traits::TestAllTypes::BAZ);
  message->AddExtension(Traits::repeated_foreign_enum_extension,
                        Traits::FOREIGN_BAZ);
  message->AddExtension(Traits::repeated_import_enum_extension,
                        Traits::IMPORT_BAZ);

  message->AddExtension(Traits::repeated_string_piece_extension, "324");
  message->AddExtension(Traits::repeated_cord_extension, "325");

  // -----------------------------------------------------------------

  message->SetExtension(Traits::default_int32_extension, 401);
  message->SetExtension(Traits::default_int64_extension, 402);
  message->SetExtension(Traits::default_uint32_extension, 403);
  message->SetExtension(Traits::default_uint64_extension, 404);
  message->SetExtension(Traits::default_sint32_extension, 405);
  message->SetExtension(Traits::default_sint64_extension, 406);
  message->SetExtension(Traits::default_fixed32_extension, 407);
  message->SetExtension(Traits::default_fixed64_extension, 408);
  message->SetExtension(Traits::default_sfixed32_extension, 409);
  message->SetExtension(Traits::default_sfixed64_extension, 410);
  message->SetExtension(Traits::default_float_extension, 411);
  message->SetExtension(Traits::default_double_extension, 412);
  message->SetExtension(Traits::default_bool_extension, false);
  message->SetExtension(Traits::default_string_extension, "415");
  message->SetExtension(Traits::default_bytes_extension, "416");

  message->SetExtension(Traits::default_nested_enum_extension,
                        Traits::TestAllTypes::FOO);
  message->SetExtension(Traits::default_foreign_enum_extension,
                        Traits::FOREIGN_FOO);
  message->SetExtension(Traits::default_import_enum_extension,
                        Traits::IMPORT_FOO);

  message->SetExtension(Traits::default_string_piece_extension, "424");
  message->SetExtension(Traits::default_cord_extension, "425");

  SetOneofFieldsExtensions(message);
}

template <typename TestAllExtensions>
void SetOneofFieldsExtensions(TestAllExtensions* message) {
  using Traits = TestUtilTraits<TestAllExtensions>;
  message->SetExtension(Traits::oneof_uint32_extension, 601);
  message->MutableExtension(Traits::oneof_nested_message_extension)
      ->set_bb(602);
  message->SetExtension(Traits::oneof_string_extension, "603");
  message->SetExtension(Traits::oneof_bytes_extension, "604");
}

template <typename TestAllExtensions>
void ModifyRepeatedExtensions(TestAllExtensions* message) {
  using Traits = TestUtilTraits<TestAllExtensions>;
  message->SetExtension(Traits::repeated_int32_extension, 1, 501);
  message->SetExtension(Traits::repeated_int64_extension, 1, 502);
  message->SetExtension(Traits::repeated_uint32_extension, 1, 503);
  message->SetExtension(Traits::repeated_uint64_extension, 1, 504);
  message->SetExtension(Traits::repeated_sint32_extension, 1, 505);
  message->SetExtension(Traits::repeated_sint64_extension, 1, 506);
  message->SetExtension(Traits::repeated_fixed32_extension, 1, 507);
  message->SetExtension(Traits::repeated_fixed64_extension, 1, 508);
  message->SetExtension(Traits::repeated_sfixed32_extension, 1, 509);
  message->SetExtension(Traits::repeated_sfixed64_extension, 1, 510);
  message->SetExtension(Traits::repeated_float_extension, 1, 511);
  message->SetExtension(Traits::repeated_double_extension, 1, 512);
  message->SetExtension(Traits::repeated_bool_extension, 1, true);
  message->SetExtension(Traits::repeated_string_extension, 1, "515");
  message->SetExtension(Traits::repeated_bytes_extension, 1, "516");

  message->MutableExtension(Traits::repeatedgroup_extension, 1)->set_a(517);
  message->MutableExtension(Traits::repeated_nested_message_extension, 1)
      ->set_bb(518);
  message->MutableExtension(Traits::repeated_foreign_message_extension, 1)
      ->set_c(519);
  message->MutableExtension(Traits::repeated_import_message_extension, 1)
      ->set_d(520);
  message->MutableExtension(Traits::repeated_lazy_message_extension, 1)
      ->set_bb(527);

  message->SetExtension(Traits::repeated_nested_enum_extension, 1,
                        Traits::TestAllTypes::FOO);
  message->SetExtension(Traits::repeated_foreign_enum_extension, 1,
                        Traits::FOREIGN_FOO);
  message->SetExtension(Traits::repeated_import_enum_extension, 1,
                        Traits::IMPORT_FOO);

  message->SetExtension(Traits::repeated_string_piece_extension, 1, "524");
  message->SetExtension(Traits::repeated_cord_extension, 1, "525");
}

template <typename TestAllExtensions>
void ExpectAllExtensionsSet(const TestAllExtensions& message) {
  using Traits = TestUtilTraits<TestAllExtensions>;
  EXPECT_TRUE(message.HasExtension(Traits::optional_int32_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_int64_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_uint32_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_uint64_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_sint32_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_sint64_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_fixed32_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_fixed64_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_sfixed32_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_sfixed64_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_float_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_double_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_bool_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_string_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_bytes_extension));

  EXPECT_TRUE(message.HasExtension(Traits::optionalgroup_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_nested_message_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_foreign_message_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_import_message_extension));
  EXPECT_TRUE(
      message.HasExtension(Traits::optional_public_import_message_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_lazy_message_extension));
  EXPECT_TRUE(
      message.HasExtension(Traits::optional_unverified_lazy_message_extension));

  EXPECT_TRUE(message.GetExtension(Traits::optionalgroup_extension).has_a());
  EXPECT_TRUE(
      message.GetExtension(Traits::optional_nested_message_extension).has_bb());
  EXPECT_TRUE(
      message.GetExtension(Traits::optional_foreign_message_extension).has_c());
  EXPECT_TRUE(
      message.GetExtension(Traits::optional_import_message_extension).has_d());
  EXPECT_TRUE(
      message.GetExtension(Traits::optional_public_import_message_extension)
          .has_e());
  EXPECT_TRUE(
      message.GetExtension(Traits::optional_lazy_message_extension).has_bb());
  EXPECT_TRUE(
      message.GetExtension(Traits::optional_unverified_lazy_message_extension)
          .has_bb());

  EXPECT_TRUE(message.HasExtension(Traits::optional_nested_enum_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_foreign_enum_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_import_enum_extension));

  EXPECT_TRUE(message.HasExtension(Traits::optional_string_piece_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_cord_extension));
  EXPECT_TRUE(message.HasExtension(Traits::optional_bytes_cord_extension));

  EXPECT_EQ(101, message.GetExtension(Traits::optional_int32_extension));
  EXPECT_EQ(102, message.GetExtension(Traits::optional_int64_extension));
  EXPECT_EQ(103, message.GetExtension(Traits::optional_uint32_extension));
  EXPECT_EQ(104, message.GetExtension(Traits::optional_uint64_extension));
  EXPECT_EQ(105, message.GetExtension(Traits::optional_sint32_extension));
  EXPECT_EQ(106, message.GetExtension(Traits::optional_sint64_extension));
  EXPECT_EQ(107, message.GetExtension(Traits::optional_fixed32_extension));
  EXPECT_EQ(108, message.GetExtension(Traits::optional_fixed64_extension));
  EXPECT_EQ(109, message.GetExtension(Traits::optional_sfixed32_extension));
  EXPECT_EQ(110, message.GetExtension(Traits::optional_sfixed64_extension));
  EXPECT_EQ(111, message.GetExtension(Traits::optional_float_extension));
  EXPECT_EQ(112, message.GetExtension(Traits::optional_double_extension));
  EXPECT_TRUE(message.GetExtension(Traits::optional_bool_extension));
  EXPECT_EQ("115", message.GetExtension(Traits::optional_string_extension));
  EXPECT_EQ("116", message.GetExtension(Traits::optional_bytes_extension));

  EXPECT_EQ(117, message.GetExtension(Traits::optionalgroup_extension).a());
  EXPECT_EQ(
      118,
      message.GetExtension(Traits::optional_nested_message_extension).bb());
  EXPECT_EQ(
      119,
      message.GetExtension(Traits::optional_foreign_message_extension).c());
  EXPECT_EQ(
      120, message.GetExtension(Traits::optional_import_message_extension).d());

  EXPECT_EQ(Traits::TestAllTypes::BAZ,
            message.GetExtension(Traits::optional_nested_enum_extension));
  EXPECT_EQ(Traits::FOREIGN_BAZ,
            message.GetExtension(Traits::optional_foreign_enum_extension));
  EXPECT_EQ(Traits::IMPORT_BAZ,
            message.GetExtension(Traits::optional_import_enum_extension));

  EXPECT_EQ("124",
            message.GetExtension(Traits::optional_string_piece_extension));
  EXPECT_EQ("125", message.GetExtension(Traits::optional_cord_extension));
  EXPECT_EQ("optional bytes cord",
            message.GetExtension(Traits::optional_bytes_cord_extension));
  EXPECT_EQ(
      126,
      message.GetExtension(Traits::optional_public_import_message_extension)
          .e());
  EXPECT_EQ(127,
            message.GetExtension(Traits::optional_lazy_message_extension).bb());
  EXPECT_EQ(
      128,
      message.GetExtension(Traits::optional_unverified_lazy_message_extension)
          .bb());

  // -----------------------------------------------------------------

  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_int32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_int64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_uint32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_uint64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_sint32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_sint64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_fixed32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_fixed64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_sfixed32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_sfixed64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_float_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_double_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_bool_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_string_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_bytes_extension));

  ASSERT_EQ(2, message.ExtensionSize(Traits::repeatedgroup_extension));
  ASSERT_EQ(2,
            message.ExtensionSize(Traits::repeated_nested_message_extension));
  ASSERT_EQ(2,
            message.ExtensionSize(Traits::repeated_foreign_message_extension));
  ASSERT_EQ(2,
            message.ExtensionSize(Traits::repeated_import_message_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_lazy_message_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_nested_enum_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_foreign_enum_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_import_enum_extension));

  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_string_piece_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_cord_extension));

  EXPECT_EQ(201, message.GetExtension(Traits::repeated_int32_extension, 0));
  EXPECT_EQ(202, message.GetExtension(Traits::repeated_int64_extension, 0));
  EXPECT_EQ(203, message.GetExtension(Traits::repeated_uint32_extension, 0));
  EXPECT_EQ(204, message.GetExtension(Traits::repeated_uint64_extension, 0));
  EXPECT_EQ(205, message.GetExtension(Traits::repeated_sint32_extension, 0));
  EXPECT_EQ(206, message.GetExtension(Traits::repeated_sint64_extension, 0));
  EXPECT_EQ(207, message.GetExtension(Traits::repeated_fixed32_extension, 0));
  EXPECT_EQ(208, message.GetExtension(Traits::repeated_fixed64_extension, 0));
  EXPECT_EQ(209, message.GetExtension(Traits::repeated_sfixed32_extension, 0));
  EXPECT_EQ(210, message.GetExtension(Traits::repeated_sfixed64_extension, 0));
  EXPECT_EQ(211, message.GetExtension(Traits::repeated_float_extension, 0));
  EXPECT_EQ(212, message.GetExtension(Traits::repeated_double_extension, 0));
  EXPECT_TRUE(message.GetExtension(Traits::repeated_bool_extension, 0));
  EXPECT_EQ("215", message.GetExtension(Traits::repeated_string_extension, 0));
  EXPECT_EQ("216", message.GetExtension(Traits::repeated_bytes_extension, 0));

  EXPECT_EQ(217, message.GetExtension(Traits::repeatedgroup_extension, 0).a());
  EXPECT_EQ(
      218,
      message.GetExtension(Traits::repeated_nested_message_extension, 0).bb());
  EXPECT_EQ(
      219,
      message.GetExtension(Traits::repeated_foreign_message_extension, 0).c());
  EXPECT_EQ(
      220,
      message.GetExtension(Traits::repeated_import_message_extension, 0).d());
  EXPECT_EQ(
      227,
      message.GetExtension(Traits::repeated_lazy_message_extension, 0).bb());

  EXPECT_EQ(Traits::TestAllTypes::BAR,
            message.GetExtension(Traits::repeated_nested_enum_extension, 0));
  EXPECT_EQ(Traits::FOREIGN_BAR,
            message.GetExtension(Traits::repeated_foreign_enum_extension, 0));
  EXPECT_EQ(Traits::IMPORT_BAR,
            message.GetExtension(Traits::repeated_import_enum_extension, 0));

  EXPECT_EQ("224",
            message.GetExtension(Traits::repeated_string_piece_extension, 0));
  EXPECT_EQ("225", message.GetExtension(Traits::repeated_cord_extension, 0));

  EXPECT_EQ(301, message.GetExtension(Traits::repeated_int32_extension, 1));
  EXPECT_EQ(302, message.GetExtension(Traits::repeated_int64_extension, 1));
  EXPECT_EQ(303, message.GetExtension(Traits::repeated_uint32_extension, 1));
  EXPECT_EQ(304, message.GetExtension(Traits::repeated_uint64_extension, 1));
  EXPECT_EQ(305, message.GetExtension(Traits::repeated_sint32_extension, 1));
  EXPECT_EQ(306, message.GetExtension(Traits::repeated_sint64_extension, 1));
  EXPECT_EQ(307, message.GetExtension(Traits::repeated_fixed32_extension, 1));
  EXPECT_EQ(308, message.GetExtension(Traits::repeated_fixed64_extension, 1));
  EXPECT_EQ(309, message.GetExtension(Traits::repeated_sfixed32_extension, 1));
  EXPECT_EQ(310, message.GetExtension(Traits::repeated_sfixed64_extension, 1));
  EXPECT_EQ(311, message.GetExtension(Traits::repeated_float_extension, 1));
  EXPECT_EQ(312, message.GetExtension(Traits::repeated_double_extension, 1));
  EXPECT_FALSE(message.GetExtension(Traits::repeated_bool_extension, 1));
  EXPECT_EQ("315", message.GetExtension(Traits::repeated_string_extension, 1));
  EXPECT_EQ("316", message.GetExtension(Traits::repeated_bytes_extension, 1));

  EXPECT_EQ(317, message.GetExtension(Traits::repeatedgroup_extension, 1).a());
  EXPECT_EQ(
      318,
      message.GetExtension(Traits::repeated_nested_message_extension, 1).bb());
  EXPECT_EQ(
      319,
      message.GetExtension(Traits::repeated_foreign_message_extension, 1).c());
  EXPECT_EQ(
      320,
      message.GetExtension(Traits::repeated_import_message_extension, 1).d());
  EXPECT_EQ(
      327,
      message.GetExtension(Traits::repeated_lazy_message_extension, 1).bb());

  EXPECT_EQ(Traits::TestAllTypes::BAZ,
            message.GetExtension(Traits::repeated_nested_enum_extension, 1));
  EXPECT_EQ(Traits::FOREIGN_BAZ,
            message.GetExtension(Traits::repeated_foreign_enum_extension, 1));
  EXPECT_EQ(Traits::IMPORT_BAZ,
            message.GetExtension(Traits::repeated_import_enum_extension, 1));

  EXPECT_EQ("324",
            message.GetExtension(Traits::repeated_string_piece_extension, 1));
  EXPECT_EQ("325", message.GetExtension(Traits::repeated_cord_extension, 1));

  // -----------------------------------------------------------------

  EXPECT_TRUE(message.HasExtension(Traits::default_int32_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_int64_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_uint32_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_uint64_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_sint32_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_sint64_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_fixed32_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_fixed64_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_sfixed32_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_sfixed64_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_float_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_double_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_bool_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_string_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_bytes_extension));

  EXPECT_TRUE(message.HasExtension(Traits::default_nested_enum_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_foreign_enum_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_import_enum_extension));

  EXPECT_TRUE(message.HasExtension(Traits::default_string_piece_extension));
  EXPECT_TRUE(message.HasExtension(Traits::default_cord_extension));

  EXPECT_EQ(401, message.GetExtension(Traits::default_int32_extension));
  EXPECT_EQ(402, message.GetExtension(Traits::default_int64_extension));
  EXPECT_EQ(403, message.GetExtension(Traits::default_uint32_extension));
  EXPECT_EQ(404, message.GetExtension(Traits::default_uint64_extension));
  EXPECT_EQ(405, message.GetExtension(Traits::default_sint32_extension));
  EXPECT_EQ(406, message.GetExtension(Traits::default_sint64_extension));
  EXPECT_EQ(407, message.GetExtension(Traits::default_fixed32_extension));
  EXPECT_EQ(408, message.GetExtension(Traits::default_fixed64_extension));
  EXPECT_EQ(409, message.GetExtension(Traits::default_sfixed32_extension));
  EXPECT_EQ(410, message.GetExtension(Traits::default_sfixed64_extension));
  EXPECT_EQ(411, message.GetExtension(Traits::default_float_extension));
  EXPECT_EQ(412, message.GetExtension(Traits::default_double_extension));
  EXPECT_FALSE(message.GetExtension(Traits::default_bool_extension));
  EXPECT_EQ("415", message.GetExtension(Traits::default_string_extension));
  EXPECT_EQ("416", message.GetExtension(Traits::default_bytes_extension));

  EXPECT_EQ(Traits::TestAllTypes::FOO,
            message.GetExtension(Traits::default_nested_enum_extension));
  EXPECT_EQ(Traits::FOREIGN_FOO,
            message.GetExtension(Traits::default_foreign_enum_extension));
  EXPECT_EQ(Traits::IMPORT_FOO,
            message.GetExtension(Traits::default_import_enum_extension));

  EXPECT_EQ("424",
            message.GetExtension(Traits::default_string_piece_extension));
  EXPECT_EQ("425", message.GetExtension(Traits::default_cord_extension));

  EXPECT_TRUE(message.HasExtension(Traits::oneof_uint32_extension));
  EXPECT_TRUE(
      message.GetExtension(Traits::oneof_nested_message_extension).has_bb());
  EXPECT_TRUE(message.HasExtension(Traits::oneof_string_extension));
  EXPECT_TRUE(message.HasExtension(Traits::oneof_bytes_extension));

  EXPECT_EQ(601, message.GetExtension(Traits::oneof_uint32_extension));
  EXPECT_EQ(602,
            message.GetExtension(Traits::oneof_nested_message_extension).bb());
  EXPECT_EQ("603", message.GetExtension(Traits::oneof_string_extension));
  EXPECT_EQ("604", message.GetExtension(Traits::oneof_bytes_extension));
}

template <typename TestAllExtensions>
void ExpectExtensionsClear(const TestAllExtensions& message) {
  using Traits = TestUtilTraits<TestAllExtensions>;
  std::string serialized;
  ASSERT_TRUE(message.SerializeToString(&serialized));
  EXPECT_EQ("", serialized);
  EXPECT_EQ(0, message.ByteSizeLong());

  // has_blah() should initially be false for all optional fields.
  EXPECT_FALSE(message.HasExtension(Traits::optional_int32_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_int64_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_uint32_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_uint64_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_sint32_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_sint64_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_fixed32_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_fixed64_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_sfixed32_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_sfixed64_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_float_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_double_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_bool_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_string_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_bytes_extension));

  EXPECT_FALSE(message.HasExtension(Traits::optionalgroup_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_nested_message_extension));
  EXPECT_FALSE(
      message.HasExtension(Traits::optional_foreign_message_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_import_message_extension));
  EXPECT_FALSE(
      message.HasExtension(Traits::optional_public_import_message_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_lazy_message_extension));
  EXPECT_FALSE(
      message.HasExtension(Traits::optional_unverified_lazy_message_extension));

  EXPECT_FALSE(message.HasExtension(Traits::optional_nested_enum_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_foreign_enum_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_import_enum_extension));

  EXPECT_FALSE(message.HasExtension(Traits::optional_string_piece_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_cord_extension));
  EXPECT_FALSE(message.HasExtension(Traits::optional_bytes_cord_extension));

  // Optional fields without defaults are set to zero or something like it.
  EXPECT_EQ(0, message.GetExtension(Traits::optional_int32_extension));
  EXPECT_EQ(0, message.GetExtension(Traits::optional_int64_extension));
  EXPECT_EQ(0, message.GetExtension(Traits::optional_uint32_extension));
  EXPECT_EQ(0, message.GetExtension(Traits::optional_uint64_extension));
  EXPECT_EQ(0, message.GetExtension(Traits::optional_sint32_extension));
  EXPECT_EQ(0, message.GetExtension(Traits::optional_sint64_extension));
  EXPECT_EQ(0, message.GetExtension(Traits::optional_fixed32_extension));
  EXPECT_EQ(0, message.GetExtension(Traits::optional_fixed64_extension));
  EXPECT_EQ(0, message.GetExtension(Traits::optional_sfixed32_extension));
  EXPECT_EQ(0, message.GetExtension(Traits::optional_sfixed64_extension));
  EXPECT_EQ(0, message.GetExtension(Traits::optional_float_extension));
  EXPECT_EQ(0, message.GetExtension(Traits::optional_double_extension));
  EXPECT_FALSE(message.GetExtension(Traits::optional_bool_extension));
  EXPECT_EQ("", message.GetExtension(Traits::optional_string_extension));
  EXPECT_EQ("", message.GetExtension(Traits::optional_bytes_extension));

  // Embedded messages should also be clear.
  EXPECT_FALSE(message.GetExtension(Traits::optionalgroup_extension).has_a());
  EXPECT_FALSE(
      message.GetExtension(Traits::optional_nested_message_extension).has_bb());
  EXPECT_FALSE(
      message.GetExtension(Traits::optional_foreign_message_extension).has_c());
  EXPECT_FALSE(
      message.GetExtension(Traits::optional_import_message_extension).has_d());
  EXPECT_FALSE(
      message.GetExtension(Traits::optional_public_import_message_extension)
          .has_e());
  EXPECT_FALSE(
      message.GetExtension(Traits::optional_lazy_message_extension).has_bb());
  EXPECT_FALSE(
      message.GetExtension(Traits::optional_unverified_lazy_message_extension)
          .has_bb());

  EXPECT_EQ(0, message.GetExtension(Traits::optionalgroup_extension).a());
  EXPECT_EQ(
      0, message.GetExtension(Traits::optional_nested_message_extension).bb());
  EXPECT_EQ(
      0, message.GetExtension(Traits::optional_foreign_message_extension).c());
  EXPECT_EQ(
      0, message.GetExtension(Traits::optional_import_message_extension).d());
  EXPECT_EQ(
      0, message.GetExtension(Traits::optional_public_import_message_extension)
             .e());
  EXPECT_EQ(0,
            message.GetExtension(Traits::optional_lazy_message_extension).bb());
  EXPECT_EQ(
      0,
      message.GetExtension(Traits::optional_unverified_lazy_message_extension)
          .bb());

  // Enums without defaults are set to the first value in the enum.
  EXPECT_EQ(Traits::TestAllTypes::FOO,
            message.GetExtension(Traits::optional_nested_enum_extension));
  EXPECT_EQ(Traits::FOREIGN_FOO,
            message.GetExtension(Traits::optional_foreign_enum_extension));
  EXPECT_EQ(Traits::IMPORT_FOO,
            message.GetExtension(Traits::optional_import_enum_extension));

  EXPECT_EQ("", message.GetExtension(Traits::optional_string_piece_extension));
  EXPECT_EQ("", message.GetExtension(Traits::optional_cord_extension));
  EXPECT_EQ("", message.GetExtension(Traits::optional_bytes_cord_extension));

  // Repeated fields are empty.
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_int32_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_int64_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_uint32_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_uint64_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_sint32_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_sint64_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_fixed32_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_fixed64_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_sfixed32_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_sfixed64_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_float_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_double_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_bool_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_string_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_bytes_extension));

  EXPECT_EQ(0, message.ExtensionSize(Traits::repeatedgroup_extension));
  EXPECT_EQ(0,
            message.ExtensionSize(Traits::repeated_nested_message_extension));
  EXPECT_EQ(0,
            message.ExtensionSize(Traits::repeated_foreign_message_extension));
  EXPECT_EQ(0,
            message.ExtensionSize(Traits::repeated_import_message_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_lazy_message_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_nested_enum_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_foreign_enum_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_import_enum_extension));

  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_string_piece_extension));
  EXPECT_EQ(0, message.ExtensionSize(Traits::repeated_cord_extension));

  // has_blah() should also be false for all default fields.
  EXPECT_FALSE(message.HasExtension(Traits::default_int32_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_int64_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_uint32_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_uint64_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_sint32_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_sint64_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_fixed32_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_fixed64_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_sfixed32_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_sfixed64_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_float_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_double_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_bool_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_string_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_bytes_extension));

  EXPECT_FALSE(message.HasExtension(Traits::default_nested_enum_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_foreign_enum_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_import_enum_extension));

  EXPECT_FALSE(message.HasExtension(Traits::default_string_piece_extension));
  EXPECT_FALSE(message.HasExtension(Traits::default_cord_extension));

  // Fields with defaults have their default values (duh).
  EXPECT_EQ(41, message.GetExtension(Traits::default_int32_extension));
  EXPECT_EQ(42, message.GetExtension(Traits::default_int64_extension));
  EXPECT_EQ(43, message.GetExtension(Traits::default_uint32_extension));
  EXPECT_EQ(44, message.GetExtension(Traits::default_uint64_extension));
  EXPECT_EQ(-45, message.GetExtension(Traits::default_sint32_extension));
  EXPECT_EQ(46, message.GetExtension(Traits::default_sint64_extension));
  EXPECT_EQ(47, message.GetExtension(Traits::default_fixed32_extension));
  EXPECT_EQ(48, message.GetExtension(Traits::default_fixed64_extension));
  EXPECT_EQ(49, message.GetExtension(Traits::default_sfixed32_extension));
  EXPECT_EQ(-50, message.GetExtension(Traits::default_sfixed64_extension));
  EXPECT_EQ(51.5, message.GetExtension(Traits::default_float_extension));
  EXPECT_EQ(52e3, message.GetExtension(Traits::default_double_extension));
  EXPECT_TRUE(message.GetExtension(Traits::default_bool_extension));
  EXPECT_EQ("hello", message.GetExtension(Traits::default_string_extension));
  EXPECT_EQ("world", message.GetExtension(Traits::default_bytes_extension));

  EXPECT_EQ(Traits::TestAllTypes::BAR,
            message.GetExtension(Traits::default_nested_enum_extension));
  EXPECT_EQ(Traits::FOREIGN_BAR,
            message.GetExtension(Traits::default_foreign_enum_extension));
  EXPECT_EQ(Traits::IMPORT_BAR,
            message.GetExtension(Traits::default_import_enum_extension));

  EXPECT_EQ("abc",
            message.GetExtension(Traits::default_string_piece_extension));
  EXPECT_EQ("123", message.GetExtension(Traits::default_cord_extension));

  EXPECT_FALSE(message.HasExtension(Traits::oneof_uint32_extension));
  EXPECT_FALSE(
      message.GetExtension(Traits::oneof_nested_message_extension).has_bb());
  EXPECT_FALSE(message.HasExtension(Traits::oneof_string_extension));
  EXPECT_FALSE(message.HasExtension(Traits::oneof_bytes_extension));
}

template <typename TestAllExtensions>
void ExpectRepeatedExtensionsModified(const TestAllExtensions& message) {
  using Traits = TestUtilTraits<TestAllExtensions>;
  // ModifyRepeatedFields only sets the second repeated element of each
  // field.  In addition to verifying this, we also verify that the first
  // element and size were *not* modified.
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_int32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_int64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_uint32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_uint64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_sint32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_sint64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_fixed32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_fixed64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_sfixed32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_sfixed64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_float_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_double_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_bool_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_string_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_bytes_extension));

  ASSERT_EQ(2, message.ExtensionSize(Traits::repeatedgroup_extension));
  ASSERT_EQ(2,
            message.ExtensionSize(Traits::repeated_nested_message_extension));
  ASSERT_EQ(2,
            message.ExtensionSize(Traits::repeated_foreign_message_extension));
  ASSERT_EQ(2,
            message.ExtensionSize(Traits::repeated_import_message_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_lazy_message_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_nested_enum_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_foreign_enum_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_import_enum_extension));

  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_string_piece_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_cord_extension));

  EXPECT_EQ(201, message.GetExtension(Traits::repeated_int32_extension, 0));
  EXPECT_EQ(202, message.GetExtension(Traits::repeated_int64_extension, 0));
  EXPECT_EQ(203, message.GetExtension(Traits::repeated_uint32_extension, 0));
  EXPECT_EQ(204, message.GetExtension(Traits::repeated_uint64_extension, 0));
  EXPECT_EQ(205, message.GetExtension(Traits::repeated_sint32_extension, 0));
  EXPECT_EQ(206, message.GetExtension(Traits::repeated_sint64_extension, 0));
  EXPECT_EQ(207, message.GetExtension(Traits::repeated_fixed32_extension, 0));
  EXPECT_EQ(208, message.GetExtension(Traits::repeated_fixed64_extension, 0));
  EXPECT_EQ(209, message.GetExtension(Traits::repeated_sfixed32_extension, 0));
  EXPECT_EQ(210, message.GetExtension(Traits::repeated_sfixed64_extension, 0));
  EXPECT_EQ(211, message.GetExtension(Traits::repeated_float_extension, 0));
  EXPECT_EQ(212, message.GetExtension(Traits::repeated_double_extension, 0));
  EXPECT_TRUE(message.GetExtension(Traits::repeated_bool_extension, 0));
  EXPECT_EQ("215", message.GetExtension(Traits::repeated_string_extension, 0));
  EXPECT_EQ("216", message.GetExtension(Traits::repeated_bytes_extension, 0));

  EXPECT_EQ(217, message.GetExtension(Traits::repeatedgroup_extension, 0).a());
  EXPECT_EQ(
      218,
      message.GetExtension(Traits::repeated_nested_message_extension, 0).bb());
  EXPECT_EQ(
      219,
      message.GetExtension(Traits::repeated_foreign_message_extension, 0).c());
  EXPECT_EQ(
      220,
      message.GetExtension(Traits::repeated_import_message_extension, 0).d());
  EXPECT_EQ(
      227,
      message.GetExtension(Traits::repeated_lazy_message_extension, 0).bb());

  EXPECT_EQ(Traits::TestAllTypes::BAR,
            message.GetExtension(Traits::repeated_nested_enum_extension, 0));
  EXPECT_EQ(Traits::FOREIGN_BAR,
            message.GetExtension(Traits::repeated_foreign_enum_extension, 0));
  EXPECT_EQ(Traits::IMPORT_BAR,
            message.GetExtension(Traits::repeated_import_enum_extension, 0));

  EXPECT_EQ("224",
            message.GetExtension(Traits::repeated_string_piece_extension, 0));
  EXPECT_EQ("225", message.GetExtension(Traits::repeated_cord_extension, 0));

  // Actually verify the second (modified) elements now.
  EXPECT_EQ(501, message.GetExtension(Traits::repeated_int32_extension, 1));
  EXPECT_EQ(502, message.GetExtension(Traits::repeated_int64_extension, 1));
  EXPECT_EQ(503, message.GetExtension(Traits::repeated_uint32_extension, 1));
  EXPECT_EQ(504, message.GetExtension(Traits::repeated_uint64_extension, 1));
  EXPECT_EQ(505, message.GetExtension(Traits::repeated_sint32_extension, 1));
  EXPECT_EQ(506, message.GetExtension(Traits::repeated_sint64_extension, 1));
  EXPECT_EQ(507, message.GetExtension(Traits::repeated_fixed32_extension, 1));
  EXPECT_EQ(508, message.GetExtension(Traits::repeated_fixed64_extension, 1));
  EXPECT_EQ(509, message.GetExtension(Traits::repeated_sfixed32_extension, 1));
  EXPECT_EQ(510, message.GetExtension(Traits::repeated_sfixed64_extension, 1));
  EXPECT_EQ(511, message.GetExtension(Traits::repeated_float_extension, 1));
  EXPECT_EQ(512, message.GetExtension(Traits::repeated_double_extension, 1));
  EXPECT_TRUE(message.GetExtension(Traits::repeated_bool_extension, 1));
  EXPECT_EQ("515", message.GetExtension(Traits::repeated_string_extension, 1));
  EXPECT_EQ("516", message.GetExtension(Traits::repeated_bytes_extension, 1));

  EXPECT_EQ(517, message.GetExtension(Traits::repeatedgroup_extension, 1).a());
  EXPECT_EQ(
      518,
      message.GetExtension(Traits::repeated_nested_message_extension, 1).bb());
  EXPECT_EQ(
      519,
      message.GetExtension(Traits::repeated_foreign_message_extension, 1).c());
  EXPECT_EQ(
      520,
      message.GetExtension(Traits::repeated_import_message_extension, 1).d());
  EXPECT_EQ(
      527,
      message.GetExtension(Traits::repeated_lazy_message_extension, 1).bb());

  EXPECT_EQ(Traits::TestAllTypes::FOO,
            message.GetExtension(Traits::repeated_nested_enum_extension, 1));
  EXPECT_EQ(Traits::FOREIGN_FOO,
            message.GetExtension(Traits::repeated_foreign_enum_extension, 1));
  EXPECT_EQ(Traits::IMPORT_FOO,
            message.GetExtension(Traits::repeated_import_enum_extension, 1));

  EXPECT_EQ("524",
            message.GetExtension(Traits::repeated_string_piece_extension, 1));
  EXPECT_EQ("525", message.GetExtension(Traits::repeated_cord_extension, 1));
}

template <typename TestAllExtensions>
void ExpectLastRepeatedExtensionsRemoved(const TestAllExtensions& message) {
  using Traits = TestUtilTraits<TestAllExtensions>;
  // Test that one element was removed.
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_int32_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_int64_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_uint32_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_uint64_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_sint32_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_sint64_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_fixed32_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_fixed64_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_sfixed32_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_sfixed64_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_float_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_double_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_bool_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_string_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_bytes_extension));

  ASSERT_EQ(1, message.ExtensionSize(Traits::repeatedgroup_extension));
  ASSERT_EQ(1,
            message.ExtensionSize(Traits::repeated_nested_message_extension));
  ASSERT_EQ(1,
            message.ExtensionSize(Traits::repeated_foreign_message_extension));
  ASSERT_EQ(1,
            message.ExtensionSize(Traits::repeated_import_message_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_lazy_message_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_nested_enum_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_foreign_enum_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_import_enum_extension));

  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_string_piece_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_cord_extension));

  // Test that the remaining element is the correct one.
  EXPECT_EQ(201, message.GetExtension(Traits::repeated_int32_extension, 0));
  EXPECT_EQ(202, message.GetExtension(Traits::repeated_int64_extension, 0));
  EXPECT_EQ(203, message.GetExtension(Traits::repeated_uint32_extension, 0));
  EXPECT_EQ(204, message.GetExtension(Traits::repeated_uint64_extension, 0));
  EXPECT_EQ(205, message.GetExtension(Traits::repeated_sint32_extension, 0));
  EXPECT_EQ(206, message.GetExtension(Traits::repeated_sint64_extension, 0));
  EXPECT_EQ(207, message.GetExtension(Traits::repeated_fixed32_extension, 0));
  EXPECT_EQ(208, message.GetExtension(Traits::repeated_fixed64_extension, 0));
  EXPECT_EQ(209, message.GetExtension(Traits::repeated_sfixed32_extension, 0));
  EXPECT_EQ(210, message.GetExtension(Traits::repeated_sfixed64_extension, 0));
  EXPECT_EQ(211, message.GetExtension(Traits::repeated_float_extension, 0));
  EXPECT_EQ(212, message.GetExtension(Traits::repeated_double_extension, 0));
  EXPECT_TRUE(message.GetExtension(Traits::repeated_bool_extension, 0));
  EXPECT_EQ("215", message.GetExtension(Traits::repeated_string_extension, 0));
  EXPECT_EQ("216", message.GetExtension(Traits::repeated_bytes_extension, 0));

  EXPECT_EQ(217, message.GetExtension(Traits::repeatedgroup_extension, 0).a());
  EXPECT_EQ(
      218,
      message.GetExtension(Traits::repeated_nested_message_extension, 0).bb());
  EXPECT_EQ(
      219,
      message.GetExtension(Traits::repeated_foreign_message_extension, 0).c());
  EXPECT_EQ(
      220,
      message.GetExtension(Traits::repeated_import_message_extension, 0).d());
  EXPECT_EQ(
      227,
      message.GetExtension(Traits::repeated_lazy_message_extension, 0).bb());

  EXPECT_EQ(Traits::TestAllTypes::BAR,
            message.GetExtension(Traits::repeated_nested_enum_extension, 0));
  EXPECT_EQ(Traits::FOREIGN_BAR,
            message.GetExtension(Traits::repeated_foreign_enum_extension, 0));
  EXPECT_EQ(Traits::IMPORT_BAR,
            message.GetExtension(Traits::repeated_import_enum_extension, 0));

  EXPECT_EQ("224",
            message.GetExtension(Traits::repeated_string_piece_extension, 0));
  EXPECT_EQ("225", message.GetExtension(Traits::repeated_cord_extension, 0));
}

template <typename TestAllExtensions>
void ExpectRepeatedExtensionsSwapped(const TestAllExtensions& message) {
  using Traits = TestUtilTraits<TestAllExtensions>;
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_int32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_int64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_uint32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_uint64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_sint32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_sint64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_fixed32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_fixed64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_sfixed32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_sfixed64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_float_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_double_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_bool_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_string_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_bytes_extension));

  ASSERT_EQ(2, message.ExtensionSize(Traits::repeatedgroup_extension));
  ASSERT_EQ(2,
            message.ExtensionSize(Traits::repeated_nested_message_extension));
  ASSERT_EQ(2,
            message.ExtensionSize(Traits::repeated_foreign_message_extension));
  ASSERT_EQ(2,
            message.ExtensionSize(Traits::repeated_import_message_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_lazy_message_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_nested_enum_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_foreign_enum_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_import_enum_extension));

  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_string_piece_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::repeated_cord_extension));

  EXPECT_EQ(201, message.GetExtension(Traits::repeated_int32_extension, 1));
  EXPECT_EQ(202, message.GetExtension(Traits::repeated_int64_extension, 1));
  EXPECT_EQ(203, message.GetExtension(Traits::repeated_uint32_extension, 1));
  EXPECT_EQ(204, message.GetExtension(Traits::repeated_uint64_extension, 1));
  EXPECT_EQ(205, message.GetExtension(Traits::repeated_sint32_extension, 1));
  EXPECT_EQ(206, message.GetExtension(Traits::repeated_sint64_extension, 1));
  EXPECT_EQ(207, message.GetExtension(Traits::repeated_fixed32_extension, 1));
  EXPECT_EQ(208, message.GetExtension(Traits::repeated_fixed64_extension, 1));
  EXPECT_EQ(209, message.GetExtension(Traits::repeated_sfixed32_extension, 1));
  EXPECT_EQ(210, message.GetExtension(Traits::repeated_sfixed64_extension, 1));
  EXPECT_EQ(211, message.GetExtension(Traits::repeated_float_extension, 1));
  EXPECT_EQ(212, message.GetExtension(Traits::repeated_double_extension, 1));
  EXPECT_TRUE(message.GetExtension(Traits::repeated_bool_extension, 1));
  EXPECT_EQ("215", message.GetExtension(Traits::repeated_string_extension, 1));
  EXPECT_EQ("216", message.GetExtension(Traits::repeated_bytes_extension, 1));

  EXPECT_EQ(217, message.GetExtension(Traits::repeatedgroup_extension, 1).a());
  EXPECT_EQ(
      218,
      message.GetExtension(Traits::repeated_nested_message_extension, 1).bb());
  EXPECT_EQ(
      219,
      message.GetExtension(Traits::repeated_foreign_message_extension, 1).c());
  EXPECT_EQ(
      220,
      message.GetExtension(Traits::repeated_import_message_extension, 1).d());
  EXPECT_EQ(
      227,
      message.GetExtension(Traits::repeated_lazy_message_extension, 1).bb());

  EXPECT_EQ(Traits::TestAllTypes::BAR,
            message.GetExtension(Traits::repeated_nested_enum_extension, 1));
  EXPECT_EQ(Traits::FOREIGN_BAR,
            message.GetExtension(Traits::repeated_foreign_enum_extension, 1));
  EXPECT_EQ(Traits::IMPORT_BAR,
            message.GetExtension(Traits::repeated_import_enum_extension, 1));

  EXPECT_EQ("224",
            message.GetExtension(Traits::repeated_string_piece_extension, 1));
  EXPECT_EQ("225", message.GetExtension(Traits::repeated_cord_extension, 1));

  EXPECT_EQ(301, message.GetExtension(Traits::repeated_int32_extension, 0));
  EXPECT_EQ(302, message.GetExtension(Traits::repeated_int64_extension, 0));
  EXPECT_EQ(303, message.GetExtension(Traits::repeated_uint32_extension, 0));
  EXPECT_EQ(304, message.GetExtension(Traits::repeated_uint64_extension, 0));
  EXPECT_EQ(305, message.GetExtension(Traits::repeated_sint32_extension, 0));
  EXPECT_EQ(306, message.GetExtension(Traits::repeated_sint64_extension, 0));
  EXPECT_EQ(307, message.GetExtension(Traits::repeated_fixed32_extension, 0));
  EXPECT_EQ(308, message.GetExtension(Traits::repeated_fixed64_extension, 0));
  EXPECT_EQ(309, message.GetExtension(Traits::repeated_sfixed32_extension, 0));
  EXPECT_EQ(310, message.GetExtension(Traits::repeated_sfixed64_extension, 0));
  EXPECT_EQ(311, message.GetExtension(Traits::repeated_float_extension, 0));
  EXPECT_EQ(312, message.GetExtension(Traits::repeated_double_extension, 0));
  EXPECT_FALSE(message.GetExtension(Traits::repeated_bool_extension, 0));
  EXPECT_EQ("315", message.GetExtension(Traits::repeated_string_extension, 0));
  EXPECT_EQ("316", message.GetExtension(Traits::repeated_bytes_extension, 0));

  EXPECT_EQ(317, message.GetExtension(Traits::repeatedgroup_extension, 0).a());
  EXPECT_EQ(
      318,
      message.GetExtension(Traits::repeated_nested_message_extension, 0).bb());
  EXPECT_EQ(
      319,
      message.GetExtension(Traits::repeated_foreign_message_extension, 0).c());
  EXPECT_EQ(
      320,
      message.GetExtension(Traits::repeated_import_message_extension, 0).d());
  EXPECT_EQ(
      327,
      message.GetExtension(Traits::repeated_lazy_message_extension, 0).bb());

  EXPECT_EQ(Traits::TestAllTypes::BAZ,
            message.GetExtension(Traits::repeated_nested_enum_extension, 0));
  EXPECT_EQ(Traits::FOREIGN_BAZ,
            message.GetExtension(Traits::repeated_foreign_enum_extension, 0));
  EXPECT_EQ(Traits::IMPORT_BAZ,
            message.GetExtension(Traits::repeated_import_enum_extension, 0));

  EXPECT_EQ("324",
            message.GetExtension(Traits::repeated_string_piece_extension, 0));
  EXPECT_EQ("325", message.GetExtension(Traits::repeated_cord_extension, 0));
}

template <typename TestFieldOrderings>
void SetAllFieldsAndExtensions(TestFieldOrderings* message) {
  using Traits = TestUtilTraits<TestFieldOrderings>;
  message->set_my_int(1);
  message->set_my_string("foo");
  message->set_my_float(1.0);
  message->SetExtension(Traits::my_extension_int, 23);
  message->SetExtension(Traits::my_extension_string, "bar");
}

template <typename TestPackedExtensions>
void SetPackedExtensions(TestPackedExtensions* message) {
  using Traits = TestUtilTraits<TestPackedExtensions>;
  message->AddExtension(Traits::packed_int32_extension, 601);
  message->AddExtension(Traits::packed_int64_extension, 602);
  message->AddExtension(Traits::packed_uint32_extension, 603);
  message->AddExtension(Traits::packed_uint64_extension, 604);
  message->AddExtension(Traits::packed_sint32_extension, 605);
  message->AddExtension(Traits::packed_sint64_extension, 606);
  message->AddExtension(Traits::packed_fixed32_extension, 607);
  message->AddExtension(Traits::packed_fixed64_extension, 608);
  message->AddExtension(Traits::packed_sfixed32_extension, 609);
  message->AddExtension(Traits::packed_sfixed64_extension, 610);
  message->AddExtension(Traits::packed_float_extension, 611);
  message->AddExtension(Traits::packed_double_extension, 612);
  message->AddExtension(Traits::packed_bool_extension, true);
  message->AddExtension(Traits::packed_enum_extension, Traits::FOREIGN_BAR);
  // add a second one of each field
  message->AddExtension(Traits::packed_int32_extension, 701);
  message->AddExtension(Traits::packed_int64_extension, 702);
  message->AddExtension(Traits::packed_uint32_extension, 703);
  message->AddExtension(Traits::packed_uint64_extension, 704);
  message->AddExtension(Traits::packed_sint32_extension, 705);
  message->AddExtension(Traits::packed_sint64_extension, 706);
  message->AddExtension(Traits::packed_fixed32_extension, 707);
  message->AddExtension(Traits::packed_fixed64_extension, 708);
  message->AddExtension(Traits::packed_sfixed32_extension, 709);
  message->AddExtension(Traits::packed_sfixed64_extension, 710);
  message->AddExtension(Traits::packed_float_extension, 711);
  message->AddExtension(Traits::packed_double_extension, 712);
  message->AddExtension(Traits::packed_bool_extension, false);
  message->AddExtension(Traits::packed_enum_extension, Traits::FOREIGN_BAZ);
}

template <typename TestPackedExtensions>
void ExpectPackedExtensionsSet(const TestPackedExtensions& message) {
  using Traits = TestUtilTraits<TestPackedExtensions>;
  ASSERT_EQ(2, message.ExtensionSize(Traits::packed_int32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::packed_int64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::packed_uint32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::packed_uint64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::packed_sint32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::packed_sint64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::packed_fixed32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::packed_fixed64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::packed_sfixed32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::packed_sfixed64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::packed_float_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::packed_double_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::packed_bool_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::packed_enum_extension));

  EXPECT_EQ(601, message.GetExtension(Traits::packed_int32_extension, 0));
  EXPECT_EQ(602, message.GetExtension(Traits::packed_int64_extension, 0));
  EXPECT_EQ(603, message.GetExtension(Traits::packed_uint32_extension, 0));
  EXPECT_EQ(604, message.GetExtension(Traits::packed_uint64_extension, 0));
  EXPECT_EQ(605, message.GetExtension(Traits::packed_sint32_extension, 0));
  EXPECT_EQ(606, message.GetExtension(Traits::packed_sint64_extension, 0));
  EXPECT_EQ(607, message.GetExtension(Traits::packed_fixed32_extension, 0));
  EXPECT_EQ(608, message.GetExtension(Traits::packed_fixed64_extension, 0));
  EXPECT_EQ(609, message.GetExtension(Traits::packed_sfixed32_extension, 0));
  EXPECT_EQ(610, message.GetExtension(Traits::packed_sfixed64_extension, 0));
  EXPECT_EQ(611, message.GetExtension(Traits::packed_float_extension, 0));
  EXPECT_EQ(612, message.GetExtension(Traits::packed_double_extension, 0));
  EXPECT_TRUE(message.GetExtension(Traits::packed_bool_extension, 0));
  EXPECT_EQ(Traits::FOREIGN_BAR,
            message.GetExtension(Traits::packed_enum_extension, 0));
  EXPECT_EQ(701, message.GetExtension(Traits::packed_int32_extension, 1));
  EXPECT_EQ(702, message.GetExtension(Traits::packed_int64_extension, 1));
  EXPECT_EQ(703, message.GetExtension(Traits::packed_uint32_extension, 1));
  EXPECT_EQ(704, message.GetExtension(Traits::packed_uint64_extension, 1));
  EXPECT_EQ(705, message.GetExtension(Traits::packed_sint32_extension, 1));
  EXPECT_EQ(706, message.GetExtension(Traits::packed_sint64_extension, 1));
  EXPECT_EQ(707, message.GetExtension(Traits::packed_fixed32_extension, 1));
  EXPECT_EQ(708, message.GetExtension(Traits::packed_fixed64_extension, 1));
  EXPECT_EQ(709, message.GetExtension(Traits::packed_sfixed32_extension, 1));
  EXPECT_EQ(710, message.GetExtension(Traits::packed_sfixed64_extension, 1));
  EXPECT_EQ(711, message.GetExtension(Traits::packed_float_extension, 1));
  EXPECT_EQ(712, message.GetExtension(Traits::packed_double_extension, 1));
  EXPECT_FALSE(message.GetExtension(Traits::packed_bool_extension, 1));
  EXPECT_EQ(Traits::FOREIGN_BAZ,
            message.GetExtension(Traits::packed_enum_extension, 1));
}

template <typename TestUnpackedExtensions>
void ExpectUnpackedExtensionsSet(const TestUnpackedExtensions& message) {
  using Traits = TestUtilTraits<TestUnpackedExtensions>;
  ASSERT_EQ(2, message.ExtensionSize(Traits::unpacked_int32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::unpacked_int64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::unpacked_uint32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::unpacked_uint64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::unpacked_sint32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::unpacked_sint64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::unpacked_fixed32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::unpacked_fixed64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::unpacked_sfixed32_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::unpacked_sfixed64_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::unpacked_float_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::unpacked_double_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::unpacked_bool_extension));
  ASSERT_EQ(2, message.ExtensionSize(Traits::unpacked_enum_extension));

  EXPECT_EQ(601, message.GetExtension(Traits::unpacked_int32_extension, 0));
  EXPECT_EQ(602, message.GetExtension(Traits::unpacked_int64_extension, 0));
  EXPECT_EQ(603, message.GetExtension(Traits::unpacked_uint32_extension, 0));
  EXPECT_EQ(604, message.GetExtension(Traits::unpacked_uint64_extension, 0));
  EXPECT_EQ(605, message.GetExtension(Traits::unpacked_sint32_extension, 0));
  EXPECT_EQ(606, message.GetExtension(Traits::unpacked_sint64_extension, 0));
  EXPECT_EQ(607, message.GetExtension(Traits::unpacked_fixed32_extension, 0));
  EXPECT_EQ(608, message.GetExtension(Traits::unpacked_fixed64_extension, 0));
  EXPECT_EQ(609, message.GetExtension(Traits::unpacked_sfixed32_extension, 0));
  EXPECT_EQ(610, message.GetExtension(Traits::unpacked_sfixed64_extension, 0));
  EXPECT_EQ(611, message.GetExtension(Traits::unpacked_float_extension, 0));
  EXPECT_EQ(612, message.GetExtension(Traits::unpacked_double_extension, 0));
  EXPECT_EQ(true, message.GetExtension(Traits::unpacked_bool_extension, 0));
  EXPECT_EQ(Traits::FOREIGN_BAR,
            message.GetExtension(Traits::unpacked_enum_extension, 0));
  EXPECT_EQ(701, message.GetExtension(Traits::unpacked_int32_extension, 1));
  EXPECT_EQ(702, message.GetExtension(Traits::unpacked_int64_extension, 1));
  EXPECT_EQ(703, message.GetExtension(Traits::unpacked_uint32_extension, 1));
  EXPECT_EQ(704, message.GetExtension(Traits::unpacked_uint64_extension, 1));
  EXPECT_EQ(705, message.GetExtension(Traits::unpacked_sint32_extension, 1));
  EXPECT_EQ(706, message.GetExtension(Traits::unpacked_sint64_extension, 1));
  EXPECT_EQ(707, message.GetExtension(Traits::unpacked_fixed32_extension, 1));
  EXPECT_EQ(708, message.GetExtension(Traits::unpacked_fixed64_extension, 1));
  EXPECT_EQ(709, message.GetExtension(Traits::unpacked_sfixed32_extension, 1));
  EXPECT_EQ(710, message.GetExtension(Traits::unpacked_sfixed64_extension, 1));
  EXPECT_EQ(711, message.GetExtension(Traits::unpacked_float_extension, 1));
  EXPECT_EQ(712, message.GetExtension(Traits::unpacked_double_extension, 1));
  EXPECT_FALSE(message.GetExtension(Traits::unpacked_bool_extension, 1));
  EXPECT_EQ(Traits::FOREIGN_BAZ,
            message.GetExtension(Traits::unpacked_enum_extension, 1));
}

template <typename TestPackedExtensions>
void ExpectLastRepeatedExtensionsRemovedPacked(
    const TestPackedExtensions& message) {
  using Traits = TestUtilTraits<TestPackedExtensions>;
  // Test that one element was removed.
  ASSERT_EQ(1, message.ExtensionSize(Traits::packed_int32_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::packed_int64_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::packed_uint32_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::packed_uint64_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::packed_sint32_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::packed_sint64_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::packed_fixed32_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::packed_fixed64_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::packed_sfixed32_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::packed_sfixed64_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::packed_float_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::packed_double_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::packed_bool_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::packed_enum_extension));

  // Test that the remaining element is the correct one.
  EXPECT_EQ(601, message.GetExtension(Traits::packed_int32_extension, 0));
  EXPECT_EQ(602, message.GetExtension(Traits::packed_int64_extension, 0));
  EXPECT_EQ(603, message.GetExtension(Traits::packed_uint32_extension, 0));
  EXPECT_EQ(604, message.GetExtension(Traits::packed_uint64_extension, 0));
  EXPECT_EQ(605, message.GetExtension(Traits::packed_sint32_extension, 0));
  EXPECT_EQ(606, message.GetExtension(Traits::packed_sint64_extension, 0));
  EXPECT_EQ(607, message.GetExtension(Traits::packed_fixed32_extension, 0));
  EXPECT_EQ(608, message.GetExtension(Traits::packed_fixed64_extension, 0));
  EXPECT_EQ(609, message.GetExtension(Traits::packed_sfixed32_extension, 0));
  EXPECT_EQ(610, message.GetExtension(Traits::packed_sfixed64_extension, 0));
  EXPECT_EQ(611, message.GetExtension(Traits::packed_float_extension, 0));
  EXPECT_EQ(612, message.GetExtension(Traits::packed_double_extension, 0));
  EXPECT_TRUE(message.GetExtension(Traits::packed_bool_extension, 0));
  EXPECT_EQ(Traits::FOREIGN_BAR,
            message.GetExtension(Traits::packed_enum_extension, 0));
}

template <typename TestAllTypes>
void ExpectLastRepeatedsReleased(const TestAllTypes& message) {
  ASSERT_EQ(1, message.repeatedgroup_size());
  ASSERT_EQ(1, message.repeated_nested_message_size());
  ASSERT_EQ(1, message.repeated_foreign_message_size());
  ASSERT_EQ(1, message.repeated_import_message_size());
  ASSERT_EQ(1, message.repeated_import_message_size());

  EXPECT_EQ(217, message.repeatedgroup(0).a());
  EXPECT_EQ(218, message.repeated_nested_message(0).bb());
  EXPECT_EQ(219, message.repeated_foreign_message(0).c());
  EXPECT_EQ(220, message.repeated_import_message(0).d());
  EXPECT_EQ(220, message.repeated_import_message(0).d());
}

template <typename TestAllExtensions>
void ExpectLastRepeatedExtensionsReleased(const TestAllExtensions& message) {
  using Traits = TestUtilTraits<TestAllExtensions>;
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeatedgroup_extension));
  ASSERT_EQ(1,
            message.ExtensionSize(Traits::repeated_nested_message_extension));
  ASSERT_EQ(1,
            message.ExtensionSize(Traits::repeated_foreign_message_extension));
  ASSERT_EQ(1,
            message.ExtensionSize(Traits::repeated_import_message_extension));
  ASSERT_EQ(1, message.ExtensionSize(Traits::repeated_lazy_message_extension));

  EXPECT_EQ(217, message.GetExtension(Traits::repeatedgroup_extension, 0).a());
  EXPECT_EQ(
      218,
      message.GetExtension(Traits::repeated_nested_message_extension, 0).bb());
  EXPECT_EQ(
      219,
      message.GetExtension(Traits::repeated_foreign_message_extension, 0).c());
  EXPECT_EQ(
      220,
      message.GetExtension(Traits::repeated_import_message_extension, 0).d());
  EXPECT_EQ(
      227,
      message.GetExtension(Traits::repeated_lazy_message_extension, 0).bb());
}

}  // namespace TestUtil
}  // namespace protobuf
}  // namespace google

// Declare the default trait so that most users don't need to do it.
PROTOBUF_TEST_UTIL_DECLARE_INPUT();

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_TEST_UTIL_H__
