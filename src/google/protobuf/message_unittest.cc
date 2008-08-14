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

#include <google/protobuf/message.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sstream>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/unittest.pb.h>
#include <google/protobuf/test_util.h>

#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {

#ifndef O_BINARY
#ifdef _O_BINARY
#define O_BINARY _O_BINARY
#else
#define O_BINARY 0     // If this isn't defined, the platform doesn't need it.
#endif
#endif

TEST(MessageTest, SerializeHelpers) {
  // TODO(kenton):  Test more helpers?  They're all two-liners so it seems
  //   like a waste of time.

  protobuf_unittest::TestAllTypes message;
  TestUtil::SetAllFields(&message);
  stringstream stream;

  string str1("foo");
  string str2("bar");

  message.SerializeToString(&str1);
  message.AppendToString(&str2);
  message.SerializeToOstream(&stream);

  EXPECT_EQ(str1.size() + 3, str2.size());
  EXPECT_EQ("bar", str2.substr(0, 3));
  // Don't use EXPECT_EQ because we don't want to dump raw binary data to
  // stdout.
  EXPECT_TRUE(str2.substr(3) == str1);

  // GCC gives some sort of error if we try to just do stream.str() == str1.
  string temp = stream.str();
  EXPECT_TRUE(temp == str1);

}

TEST(MessageTest, ParseFromFileDescriptor) {
  string filename = TestSourceDir() +
                    "/google/protobuf/testdata/golden_message";
  int file = open(filename.c_str(), O_RDONLY | O_BINARY);

  unittest::TestAllTypes message;
  EXPECT_TRUE(message.ParseFromFileDescriptor(file));
  TestUtil::ExpectAllFieldsSet(message);

  EXPECT_GE(close(file), 0);
}

TEST(MessageTest, ParseHelpers) {
  // TODO(kenton):  Test more helpers?  They're all two-liners so it seems
  //   like a waste of time.
  string data;

  {
    // Set up.
    protobuf_unittest::TestAllTypes message;
    TestUtil::SetAllFields(&message);
    message.SerializeToString(&data);
  }

  {
    // Test ParseFromString.
    protobuf_unittest::TestAllTypes message;
    EXPECT_TRUE(message.ParseFromString(data));
    TestUtil::ExpectAllFieldsSet(message);
  }

  {
    // Test ParseFromIstream.
    protobuf_unittest::TestAllTypes message;
    stringstream stream(data);
    EXPECT_TRUE(message.ParseFromIstream(&stream));
    EXPECT_TRUE(stream.eof());
    TestUtil::ExpectAllFieldsSet(message);
  }
}

TEST(MessageTest, ParseFailsIfNotInitialized) {
  unittest::TestRequired message;
  vector<string> errors;

  {
    ScopedMemoryLog log;
    EXPECT_FALSE(message.ParseFromString(""));
    errors = log.GetMessages(ERROR);
  }

  ASSERT_EQ(1, errors.size());
  EXPECT_EQ("Can't parse message of type \"protobuf_unittest.TestRequired\" "
            "because it is missing required fields: a, b, c",
            errors[0]);
}

TEST(MessageTest, BypassInitializationCheckOnParse) {
  unittest::TestRequired message;
  io::ArrayInputStream raw_input(NULL, 0);
  io::CodedInputStream input(&raw_input);
  EXPECT_TRUE(message.MergePartialFromCodedStream(&input));
}

TEST(MessageTest, InitializationErrorString) {
  unittest::TestRequired message;
  EXPECT_EQ("a, b, c", message.InitializationErrorString());
}

#ifdef GTEST_HAS_DEATH_TEST  // death tests do not work on Windows yet.

TEST(MessageTest, SerializeFailsIfNotInitialized) {
  unittest::TestRequired message;
  string data;
  EXPECT_DEBUG_DEATH(EXPECT_TRUE(message.SerializeToString(&data)),
    "Can't serialize message of type \"protobuf_unittest.TestRequired\" because "
    "it is missing required fields: a, b, c");
}

TEST(MessageTest, CheckInitialized) {
  unittest::TestRequired message;
  EXPECT_DEATH(message.CheckInitialized(),
    "Message of type \"protobuf_unittest.TestRequired\" is missing required "
    "fields: a, b, c");
}

#endif  // GTEST_HAS_DEATH_TEST

TEST(MessageTest, BypassInitializationCheckOnSerialize) {
  unittest::TestRequired message;
  io::ArrayOutputStream raw_output(NULL, 0);
  io::CodedOutputStream output(&raw_output);
  EXPECT_TRUE(message.SerializePartialToCodedStream(&output));
}

TEST(MessageTest, FindInitializationErrors) {
  unittest::TestRequired message;
  vector<string> errors;
  message.FindInitializationErrors(&errors);
  ASSERT_EQ(3, errors.size());
  EXPECT_EQ("a", errors[0]);
  EXPECT_EQ("b", errors[1]);
  EXPECT_EQ("c", errors[2]);
}

TEST(MessageTest, ParseFailsOnInvalidMessageEnd) {
  unittest::TestAllTypes message;

  // Control case.
  EXPECT_TRUE(message.ParseFromArray("", 0));

  // The byte is a valid varint, but not a valid tag (zero).
  EXPECT_FALSE(message.ParseFromArray("\0", 1));

  // The byte is a malformed varint.
  EXPECT_FALSE(message.ParseFromArray("\200", 1));

  // The byte is an endgroup tag, but we aren't parsing a group.
  EXPECT_FALSE(message.ParseFromArray("\014", 1));
}

TEST(MessageFactoryTest, GeneratedFactoryLookup) {
  EXPECT_EQ(
    MessageFactory::generated_factory()->GetPrototype(
      protobuf_unittest::TestAllTypes::descriptor()),
    &protobuf_unittest::TestAllTypes::default_instance());
}

TEST(MessageFactoryTest, GeneratedFactoryUnknownType) {
  // Construct a new descriptor.
  DescriptorPool pool;
  FileDescriptorProto file;
  file.set_name("foo.proto");
  file.add_message_type()->set_name("Foo");
  const Descriptor* descriptor = pool.BuildFile(file)->message_type(0);

  // Trying to construct it should return NULL.
  EXPECT_TRUE(
    MessageFactory::generated_factory()->GetPrototype(descriptor) == NULL);
}

}  // namespace protobuf
}  // namespace google
