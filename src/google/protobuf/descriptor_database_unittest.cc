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
//
// This file makes extensive use of RFC 3092.  :)

#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/stubs/strutil.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace {

static bool AddToPool(DescriptorPool* pool, const char* file_text) {
  FileDescriptorProto file_proto;
  if (!TextFormat::ParseFromString(file_text, &file_proto)) return false;
  if (pool->BuildFile(file_proto) == NULL) return false;
  return true;
}

static void AddToDatabase(SimpleDescriptorDatabase* database,
                          const char* file_text) {
  FileDescriptorProto file_proto;
  EXPECT_TRUE(TextFormat::ParseFromString(file_text, &file_proto));
  database->Add(file_proto);
}

static void ExpectContainsType(const FileDescriptorProto& proto,
                               const string& type_name) {
  for (int i = 0; i < proto.message_type_size(); i++) {
    if (proto.message_type(i).name() == type_name) return;
  }
  ADD_FAILURE() << "\"" << proto.name()
                << "\" did not contain expected type \""
                << type_name << "\".";
}

// ===================================================================

TEST(SimpleDescriptorDatabaseTest, FindFileByName) {
  SimpleDescriptorDatabase database;
  AddToDatabase(&database,
    "name: \"foo.proto\" "
    "message_type { name:\"Foo\" }");
  AddToDatabase(&database,
    "name: \"bar.proto\" "
    "message_type { name:\"Bar\" }");

  {
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileByName("foo.proto", &file));
    EXPECT_EQ("foo.proto", file.name());
    ExpectContainsType(file, "Foo");
  }

  {
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileByName("bar.proto", &file));
    EXPECT_EQ("bar.proto", file.name());
    ExpectContainsType(file, "Bar");
  }

  {
    // Fails to find undefined files.
    FileDescriptorProto file;
    EXPECT_FALSE(database.FindFileByName("baz.proto", &file));
  }
}

TEST(SimpleDescriptorDatabaseTest, FindFileContainingSymbol) {
  SimpleDescriptorDatabase database;
  AddToDatabase(&database,
    "name: \"foo.proto\" "
    "message_type { "
    "  name: \"Foo\" "
    "  field { name:\"qux\" }"
    "  nested_type { name: \"Grault\" } "
    "  enum_type { name: \"Garply\" } "
    "} "
    "enum_type { "
    "  name: \"Waldo\" "
    "  value { name:\"FRED\" } "
    "} "
    "extension { name: \"plugh\" } "
    "service { "
    "  name: \"Xyzzy\" "
    "  method { name: \"Thud\" } "
    "}"
    );
  AddToDatabase(&database,
    "name: \"bar.proto\" "
    "package: \"corge\" "
    "message_type { name: \"Bar\" }");

  {
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingSymbol("Foo", &file));
    EXPECT_EQ("foo.proto", file.name());
  }

  {
    // Can find fields.
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingSymbol("Foo.qux", &file));
    EXPECT_EQ("foo.proto", file.name());
  }

  {
    // Can find nested types.
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingSymbol("Foo.Grault", &file));
    EXPECT_EQ("foo.proto", file.name());
  }

  {
    // Can find nested enums.
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingSymbol("Foo.Garply", &file));
    EXPECT_EQ("foo.proto", file.name());
  }

  {
    // Can find enum types.
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingSymbol("Waldo", &file));
    EXPECT_EQ("foo.proto", file.name());
  }

  {
    // Can find enum values.
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingSymbol("Waldo.FRED", &file));
    EXPECT_EQ("foo.proto", file.name());
  }

  {
    // Can find extensions.
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingSymbol("plugh", &file));
    EXPECT_EQ("foo.proto", file.name());
  }

  {
    // Can find services.
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingSymbol("Xyzzy", &file));
    EXPECT_EQ("foo.proto", file.name());
  }

  {
    // Can find methods.
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingSymbol("Xyzzy.Thud", &file));
    EXPECT_EQ("foo.proto", file.name());
  }

  {
    // Can find things in packages.
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingSymbol("corge.Bar", &file));
    EXPECT_EQ("bar.proto", file.name());
  }

  {
    // Fails to find undefined symbols.
    FileDescriptorProto file;
    EXPECT_FALSE(database.FindFileContainingSymbol("Baz", &file));
  }

  {
    // Names must be fully-qualified.
    FileDescriptorProto file;
    EXPECT_FALSE(database.FindFileContainingSymbol("Bar", &file));
  }
}

TEST(SimpleDescriptorDatabaseTest, FindFileContainingExtension) {
  SimpleDescriptorDatabase database;
  AddToDatabase(&database,
    "name: \"foo.proto\" "
    "message_type { "
    "  name: \"Foo\" "
    "  extension_range { start: 1 end: 1000 } "
    "  extension { name:\"qux\" label:LABEL_OPTIONAL type:TYPE_INT32 number:5 "
    "              extendee: \".Foo\" }"
    "}");
  AddToDatabase(&database,
    "name: \"bar.proto\" "
    "package: \"corge\" "
    "dependency: \"foo.proto\" "
    "message_type { "
    "  name: \"Bar\" "
    "  extension_range { start: 1 end: 1000 } "
    "} "
    "extension { name:\"grault\" extendee: \".Foo\"       number:32 } "
    "extension { name:\"garply\" extendee: \".corge.Bar\" number:70 } "
    "extension { name:\"waldo\"  extendee: \"Bar\"        number:56 } ");

  {
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingExtension("Foo", 5, &file));
    EXPECT_EQ("foo.proto", file.name());
  }

  {
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingExtension("Foo", 32, &file));
    EXPECT_EQ("bar.proto", file.name());
  }

  {
    // Can find extensions for qualified type names.
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingExtension("corge.Bar", 70, &file));
    EXPECT_EQ("bar.proto", file.name());
  }

  {
    // Can't find extensions whose extendee was not fully-qualified in the
    // FileDescriptorProto.
    FileDescriptorProto file;
    EXPECT_FALSE(database.FindFileContainingExtension("Bar", 56, &file));
    EXPECT_FALSE(database.FindFileContainingExtension("corge.Bar", 56, &file));
  }

  {
    // Can't find non-existent extension numbers.
    FileDescriptorProto file;
    EXPECT_FALSE(database.FindFileContainingExtension("Foo", 12, &file));
  }

  {
    // Can't find extensions for non-existent types.
    FileDescriptorProto file;
    EXPECT_FALSE(database.FindFileContainingExtension("NoSuchType", 5, &file));
  }

  {
    // Can't find extensions for unqualified type names.
    FileDescriptorProto file;
    EXPECT_FALSE(database.FindFileContainingExtension("Bar", 70, &file));
  }
}

// ===================================================================

TEST(DescriptorPoolDatabaseTest, FindFileByName) {
  DescriptorPool pool;
  ASSERT_TRUE(AddToPool(&pool,
    "name: \"foo.proto\" "
    "message_type { name:\"Foo\" }"));
  ASSERT_TRUE(AddToPool(&pool,
    "name: \"bar.proto\" "
    "message_type { name:\"Bar\" }"));

  DescriptorPoolDatabase database(pool);

  {
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileByName("foo.proto", &file));
    EXPECT_EQ("foo.proto", file.name());
    ExpectContainsType(file, "Foo");
  }

  {
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileByName("bar.proto", &file));
    EXPECT_EQ("bar.proto", file.name());
    ExpectContainsType(file, "Bar");
  }

  {
    // Fails to find undefined files.
    FileDescriptorProto file;
    EXPECT_FALSE(database.FindFileByName("baz.proto", &file));
  }
}

TEST(DescriptorPoolDatabaseTest, FindFileContainingSymbol) {
  DescriptorPool pool;
  ASSERT_TRUE(AddToPool(&pool,
    "name: \"foo.proto\" "
    "message_type { "
    "  name: \"Foo\" "
    "  field { name:\"qux\" label:LABEL_OPTIONAL type:TYPE_INT32 number:1 }"
    "}"));
  ASSERT_TRUE(AddToPool(&pool,
    "name: \"bar.proto\" "
    "package: \"corge\" "
    "message_type { name: \"Bar\" }"));

  DescriptorPoolDatabase database(pool);

  {
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingSymbol("Foo", &file));
    EXPECT_EQ("foo.proto", file.name());
  }

  {
    // Can find fields.
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingSymbol("Foo.qux", &file));
    EXPECT_EQ("foo.proto", file.name());
  }

  {
    // Can find things in packages.
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingSymbol("corge.Bar", &file));
    EXPECT_EQ("bar.proto", file.name());
  }

  {
    // Fails to find undefined symbols.
    FileDescriptorProto file;
    EXPECT_FALSE(database.FindFileContainingSymbol("Baz", &file));
  }

  {
    // Names must be fully-qualified.
    FileDescriptorProto file;
    EXPECT_FALSE(database.FindFileContainingSymbol("Bar", &file));
  }
}

TEST(DescriptorPoolDatabaseTest, FindFileContainingExtension) {
  DescriptorPool pool;
  ASSERT_TRUE(AddToPool(&pool,
    "name: \"foo.proto\" "
    "message_type { "
    "  name: \"Foo\" "
    "  extension_range { start: 1 end: 1000 } "
    "  extension { name:\"qux\" label:LABEL_OPTIONAL type:TYPE_INT32 number:5 "
    "              extendee: \"Foo\" }"
    "}"));
  ASSERT_TRUE(AddToPool(&pool,
    "name: \"bar.proto\" "
    "package: \"corge\" "
    "dependency: \"foo.proto\" "
    "message_type { "
    "  name: \"Bar\" "
    "  extension_range { start: 1 end: 1000 } "
    "} "
    "extension { name:\"grault\" label:LABEL_OPTIONAL type:TYPE_BOOL number:32 "
    "            extendee: \"Foo\" } "
    "extension { name:\"garply\" label:LABEL_OPTIONAL type:TYPE_BOOL number:70 "
    "            extendee: \"Bar\" } "));

  DescriptorPoolDatabase database(pool);

  {
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingExtension("Foo", 5, &file));
    EXPECT_EQ("foo.proto", file.name());
  }

  {
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingExtension("Foo", 32, &file));
    EXPECT_EQ("bar.proto", file.name());
  }

  {
    // Can find extensions for qualified type names..
    FileDescriptorProto file;
    EXPECT_TRUE(database.FindFileContainingExtension("corge.Bar", 70, &file));
    EXPECT_EQ("bar.proto", file.name());
  }

  {
    // Can't find non-existent extension numbers.
    FileDescriptorProto file;
    EXPECT_FALSE(database.FindFileContainingExtension("Foo", 12, &file));
  }

  {
    // Can't find extensions for non-existent types.
    FileDescriptorProto file;
    EXPECT_FALSE(database.FindFileContainingExtension("NoSuchType", 5, &file));
  }

  {
    // Can't find extensions for unqualified type names.
    FileDescriptorProto file;
    EXPECT_FALSE(database.FindFileContainingExtension("Bar", 70, &file));
  }
}

// ===================================================================

class MergedDescriptorDatabaseTest : public testing::Test {
 protected:
  MergedDescriptorDatabaseTest()
    : forward_merged_(&database1_, &database2_),
      reverse_merged_(&database2_, &database1_) {}

  virtual void SetUp() {
    AddToDatabase(&database1_,
      "name: \"foo.proto\" "
      "message_type { name:\"Foo\" extension_range { start: 1 end: 100 } } "
      "extension { name:\"foo_ext\" extendee: \".Foo\" number:3 "
      "            label:LABEL_OPTIONAL type:TYPE_INT32 } ");
    AddToDatabase(&database2_,
      "name: \"bar.proto\" "
      "message_type { name:\"Bar\" extension_range { start: 1 end: 100 } } "
      "extension { name:\"bar_ext\" extendee: \".Bar\" number:5 "
      "            label:LABEL_OPTIONAL type:TYPE_INT32 } ");

    // baz.proto exists in both pools, with different definitions.
    AddToDatabase(&database1_,
      "name: \"baz.proto\" "
      "message_type { name:\"Baz\" extension_range { start: 1 end: 100 } } "
      "message_type { name:\"FromPool1\" } "
      "extension { name:\"baz_ext\" extendee: \".Baz\" number:12 "
      "            label:LABEL_OPTIONAL type:TYPE_INT32 } "
      "extension { name:\"database1_only_ext\" extendee: \".Baz\" number:13 "
      "            label:LABEL_OPTIONAL type:TYPE_INT32 } ");
    AddToDatabase(&database2_,
      "name: \"baz.proto\" "
      "message_type { name:\"Baz\" extension_range { start: 1 end: 100 } } "
      "message_type { name:\"FromPool2\" } "
      "extension { name:\"baz_ext\" extendee: \".Baz\" number:12 "
      "            label:LABEL_OPTIONAL type:TYPE_INT32 } ");
  }

  SimpleDescriptorDatabase database1_;
  SimpleDescriptorDatabase database2_;

  MergedDescriptorDatabase forward_merged_;
  MergedDescriptorDatabase reverse_merged_;
};

TEST_F(MergedDescriptorDatabaseTest, FindFileByName) {
  {
    // Can find file that is only in database1_.
    FileDescriptorProto file;
    EXPECT_TRUE(forward_merged_.FindFileByName("foo.proto", &file));
    EXPECT_EQ("foo.proto", file.name());
    ExpectContainsType(file, "Foo");
  }

  {
    // Can find file that is only in database2_.
    FileDescriptorProto file;
    EXPECT_TRUE(forward_merged_.FindFileByName("bar.proto", &file));
    EXPECT_EQ("bar.proto", file.name());
    ExpectContainsType(file, "Bar");
  }

  {
    // In forward_merged_, database1_'s baz.proto takes precedence.
    FileDescriptorProto file;
    EXPECT_TRUE(forward_merged_.FindFileByName("baz.proto", &file));
    EXPECT_EQ("baz.proto", file.name());
    ExpectContainsType(file, "FromPool1");
  }

  {
    // In reverse_merged_, database2_'s baz.proto takes precedence.
    FileDescriptorProto file;
    EXPECT_TRUE(reverse_merged_.FindFileByName("baz.proto", &file));
    EXPECT_EQ("baz.proto", file.name());
    ExpectContainsType(file, "FromPool2");
  }

  {
    // Can't find non-existent file.
    FileDescriptorProto file;
    EXPECT_FALSE(forward_merged_.FindFileByName("no_such.proto", &file));
  }
}

TEST_F(MergedDescriptorDatabaseTest, FindFileContainingSymbol) {
  {
    // Can find file that is only in database1_.
    FileDescriptorProto file;
    EXPECT_TRUE(forward_merged_.FindFileContainingSymbol("Foo", &file));
    EXPECT_EQ("foo.proto", file.name());
    ExpectContainsType(file, "Foo");
  }

  {
    // Can find file that is only in database2_.
    FileDescriptorProto file;
    EXPECT_TRUE(forward_merged_.FindFileContainingSymbol("Bar", &file));
    EXPECT_EQ("bar.proto", file.name());
    ExpectContainsType(file, "Bar");
  }

  {
    // In forward_merged_, database1_'s baz.proto takes precedence.
    FileDescriptorProto file;
    EXPECT_TRUE(forward_merged_.FindFileContainingSymbol("Baz", &file));
    EXPECT_EQ("baz.proto", file.name());
    ExpectContainsType(file, "FromPool1");
  }

  {
    // In reverse_merged_, database2_'s baz.proto takes precedence.
    FileDescriptorProto file;
    EXPECT_TRUE(reverse_merged_.FindFileContainingSymbol("Baz", &file));
    EXPECT_EQ("baz.proto", file.name());
    ExpectContainsType(file, "FromPool2");
  }

  {
    // FromPool1 only shows up in forward_merged_ because it is masked by
    // database2_'s baz.proto in reverse_merged_.
    FileDescriptorProto file;
    EXPECT_TRUE(forward_merged_.FindFileContainingSymbol("FromPool1", &file));
    EXPECT_FALSE(reverse_merged_.FindFileContainingSymbol("FromPool1", &file));
  }

  {
    // Can't find non-existent symbol.
    FileDescriptorProto file;
    EXPECT_FALSE(
      forward_merged_.FindFileContainingSymbol("NoSuchType", &file));
  }
}

TEST_F(MergedDescriptorDatabaseTest, FindFileContainingExtension) {
  {
    // Can find file that is only in database1_.
    FileDescriptorProto file;
    EXPECT_TRUE(
      forward_merged_.FindFileContainingExtension("Foo", 3, &file));
    EXPECT_EQ("foo.proto", file.name());
    ExpectContainsType(file, "Foo");
  }

  {
    // Can find file that is only in database2_.
    FileDescriptorProto file;
    EXPECT_TRUE(
      forward_merged_.FindFileContainingExtension("Bar", 5, &file));
    EXPECT_EQ("bar.proto", file.name());
    ExpectContainsType(file, "Bar");
  }

  {
    // In forward_merged_, database1_'s baz.proto takes precedence.
    FileDescriptorProto file;
    EXPECT_TRUE(
      forward_merged_.FindFileContainingExtension("Baz", 12, &file));
    EXPECT_EQ("baz.proto", file.name());
    ExpectContainsType(file, "FromPool1");
  }

  {
    // In reverse_merged_, database2_'s baz.proto takes precedence.
    FileDescriptorProto file;
    EXPECT_TRUE(
      reverse_merged_.FindFileContainingExtension("Baz", 12, &file));
    EXPECT_EQ("baz.proto", file.name());
    ExpectContainsType(file, "FromPool2");
  }

  {
    // Baz's extension 13 only shows up in forward_merged_ because it is
    // masked by database2_'s baz.proto in reverse_merged_.
    FileDescriptorProto file;
    EXPECT_TRUE(forward_merged_.FindFileContainingExtension("Baz", 13, &file));
    EXPECT_FALSE(reverse_merged_.FindFileContainingExtension("Baz", 13, &file));
  }

  {
    // Can't find non-existent extension.
    FileDescriptorProto file;
    EXPECT_FALSE(
      forward_merged_.FindFileContainingExtension("Foo", 6, &file));
  }
}

}  // anonymous namespace
}  // namespace protobuf
}  // namespace google
