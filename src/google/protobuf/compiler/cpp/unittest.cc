// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// To test the code generator, we actually use it to generate code for
// google/protobuf/unittest.proto, then test that.  This means that we
// are actually testing the parser and other parts of the system at the same
// time, and that problems in the generator may show up as compile-time errors
// rather than unittest failures, which may be surprising.  However, testing
// the output of the C++ generator directly would be very hard.  We can't very
// well just check it against golden files since those files would have to be
// updated for any small change; such a test would be very brittle and probably
// not very helpful.  What we really want to test is that the code compiles
// correctly and produces the interfaces we expect, which is why this test
// is written this way.

#include "google/protobuf/compiler/cpp/unittest.h"

#include "google/protobuf/test_util.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_embed_optimize_for.pb.h"
#include "google/protobuf/unittest_import.pb.h"
#include "google/protobuf/unittest_optimize_for.pb.h"


#define MESSAGE_TEST_NAME MessageTest
#define GENERATED_DESCRIPTOR_TEST_NAME GeneratedDescriptorTest
#define GENERATED_MESSAGE_TEST_NAME GeneratedMessageTest
#define GENERATED_ENUM_TEST_NAME GeneratedEnumTest
#define GENERATED_SERVICE_TEST_NAME GeneratedServiceTest
#define HELPERS_TEST_NAME HelpersTest
#define DESCRIPTOR_INIT_TEST_NAME DescriptorInitializationTest

#define UNITTEST_PROTO_PATH "google/protobuf/unittest.proto"
#define UNITTEST ::proto2_unittest
#define UNITTEST_IMPORT ::proto2_unittest_import

// Must include after the above macros.
#include "google/protobuf/compiler/cpp/unittest.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

// Can't use an anonymous namespace here due to brokenness of Tru64 compiler.
namespace cpp_unittest {

namespace proto2_unittest = ::proto2_unittest;

TEST(GENERATED_MESSAGE_TEST_NAME, TestConflictingSymbolNames) {
  // test_bad_identifiers.proto successfully compiled, then it works.  The
  // following is just a token usage to insure that the code is, in fact,
  // being compiled and linked.

  proto2_unittest::TestConflictingSymbolNames message;
  message.set_uint32(1);
  EXPECT_EQ(3, message.ByteSizeLong());

  message.set_friend_(5);
  EXPECT_EQ(5, message.friend_());

  message.set_class_(6);
  EXPECT_EQ(6, message.class_());

  // Instantiate extension template functions to test conflicting template
  // parameter names.
  typedef proto2_unittest::TestConflictingSymbolNamesExtension ExtensionMessage;
  message.AddExtension(ExtensionMessage::repeated_int32_ext, 123);
  EXPECT_EQ(123, message.GetExtension(ExtensionMessage::repeated_int32_ext, 0));
}


TEST(GENERATED_MESSAGE_TEST_NAME, TestSwapNameIsNotMangledForFields) {
  // For backwards compatibility we do not mangle `swap`. It works thanks to
  // overload resolution.
  int v [[maybe_unused]] =
      proto2_unittest::TestConflictingSymbolNames::BadKnownNamesFields().swap();

  // But we do mangle `swap` for extensions because there is no overloading
  // there.
  v = proto2_unittest::TestConflictingSymbolNames::BadKnownNamesValues()
          .GetExtension(proto2_unittest::TestConflictingSymbolNames::
                            BadKnownNamesValues::swap_);
}

TEST(GENERATED_MESSAGE_TEST_NAME, TestNoStandardDescriptorOption) {
  // When no_standard_descriptor_accessor = true, we should not mangle fields
  // named `descriptor`.
  int v [[maybe_unused]] =
      proto2_unittest::TestConflictingSymbolNames::BadKnownNamesFields()
          .descriptor_();
  v = proto2_unittest::TestConflictingSymbolNames::
          BadKnownNamesFieldsNoStandardDescriptor()
              .descriptor();
}

TEST(GENERATED_MESSAGE_TEST_NAME, TestFileVsMessageScope) {
  // Special names at message scope are mangled,
  int v [[maybe_unused]] =
      proto2_unittest::TestConflictingSymbolNames::BadKnownNamesValues()
          .GetExtension(proto2_unittest::TestConflictingSymbolNames::
                            BadKnownNamesValues::unknown_fields_);
  // But not at file scope.
  v = proto2_unittest::TestConflictingSymbolNames::BadKnownNamesValues()
          .GetExtension(proto2_unittest::unknown_fields);
}

TEST(GENERATED_MESSAGE_TEST_NAME, TestConflictingEnumNames) {
  proto2_unittest::TestConflictingEnumNames message;
  message.set_conflicting_enum(
      proto2_unittest::TestConflictingEnumNames_while_and_);
  EXPECT_EQ(1, message.conflicting_enum());
  message.set_conflicting_enum(
      proto2_unittest::TestConflictingEnumNames_while_XOR);
  EXPECT_EQ(5, message.conflicting_enum());

  proto2_unittest::bool_ conflicting_enum;
  conflicting_enum = proto2_unittest::NOT_EQ;
  EXPECT_EQ(1, conflicting_enum);
  conflicting_enum = proto2_unittest::return_;
  EXPECT_EQ(3, conflicting_enum);
}

TEST(GENERATED_MESSAGE_TEST_NAME, TestConflictingMessageNames) {
  proto2_unittest::NULL_ message;
  message.set_int_(123);
  EXPECT_EQ(message.int_(), 123);
}

TEST(GENERATED_MESSAGE_TEST_NAME, TestConflictingExtension) {
  proto2_unittest::TestConflictingSymbolNames message;
  message.SetExtension(proto2_unittest::void_, 123);
  EXPECT_EQ(123, message.GetExtension(proto2_unittest::void_));
}

}  // namespace cpp_unittest
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
