// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <memory>

#include "google/protobuf/any.pb.h"
#include <gtest/gtest.h>
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/compiler/csharp/csharp_helpers.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {
namespace {

TEST(CSharpEnumValue, PascalCasedPrefixStripping) {
  EXPECT_EQ("Bar", GetEnumValueName("Foo", "BAR"));
  EXPECT_EQ("BarBaz", GetEnumValueName("Foo", "BAR_BAZ"));
  EXPECT_EQ("Bar", GetEnumValueName("Foo", "FOO_BAR"));
  EXPECT_EQ("Bar", GetEnumValueName("Foo", "FOO__BAR"));
  EXPECT_EQ("BarBaz", GetEnumValueName("Foo", "FOO_BAR_BAZ"));
  EXPECT_EQ("BarBaz", GetEnumValueName("Foo", "Foo_BarBaz"));
  EXPECT_EQ("Bar", GetEnumValueName("FO_O", "FOO_BAR"));
  EXPECT_EQ("Bar", GetEnumValueName("FOO", "F_O_O_BAR"));
  EXPECT_EQ("Bar", GetEnumValueName("Foo", "BAR"));
  EXPECT_EQ("BarBaz", GetEnumValueName("Foo", "BAR_BAZ"));
  EXPECT_EQ("Foo", GetEnumValueName("Foo", "FOO"));
  EXPECT_EQ("Foo", GetEnumValueName("Foo", "FOO___"));
  // Identifiers can't start with digits
  EXPECT_EQ("_2Bar", GetEnumValueName("Foo", "FOO_2_BAR"));
  EXPECT_EQ("_2", GetEnumValueName("Foo", "FOO___2"));
}

TEST(DescriptorProtoHelpers, IsDescriptorProto) {
  EXPECT_TRUE(IsDescriptorProto(DescriptorProto::descriptor()->file()));
  EXPECT_FALSE(IsDescriptorProto(google::protobuf::Any::descriptor()->file()));
}

TEST(DescriptorProtoHelpers, IsDescriptorOptionMessage) {
  EXPECT_TRUE(IsDescriptorOptionMessage(FileOptions::descriptor()));
  EXPECT_FALSE(IsDescriptorOptionMessage(google::protobuf::Any::descriptor()));
  EXPECT_FALSE(IsDescriptorOptionMessage(DescriptorProto::descriptor()));
}

TEST(CSharpIdentifiers, UnderscoresToCamelCase) {
	EXPECT_EQ("FooBar", UnderscoresToCamelCase("Foo_Bar", true));
	EXPECT_EQ("fooBar", UnderscoresToCamelCase("FooBar", false));
	EXPECT_EQ("foo123", UnderscoresToCamelCase("foo_123", false));
	// remove leading underscores
	EXPECT_EQ("Foo123", UnderscoresToCamelCase("_Foo_123", true));
	// this one has slight unexpected output as it capitalises the first
	// letter after consuming the underscores, but this was the existing
	// behaviour so I have not changed it
	EXPECT_EQ("FooBar", UnderscoresToCamelCase("___fooBar", false));
	// leave a leading underscore for identifiers that would otherwise
	// be invalid because they would start with a digit
	EXPECT_EQ("_123Foo", UnderscoresToCamelCase("_123_foo", true));
	EXPECT_EQ("_123Foo", UnderscoresToCamelCase("___123_foo", true));
}

}  // namespace
}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
