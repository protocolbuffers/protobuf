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
// Since the reflection interface for DynamicMessage is implemented by
// GenericMessageReflection, the only thing we really have to test is
// that DynamicMessage correctly sets up the information that
// GenericMessageReflection needs to use.  So, we focus on that in this
// test.  Other tests, such as generic_message_reflection_unittest and
// reflection_ops_unittest, cover the rest of the functionality used by
// DynamicMessage.

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/test_util.h>
#include <google/protobuf/unittest.pb.h>

#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {

class DynamicMessageTest : public testing::Test {
 protected:
  DescriptorPool pool_;
  DynamicMessageFactory factory_;
  const Descriptor* descriptor_;
  const Message* prototype_;
  const Descriptor* extensions_descriptor_;
  const Message* extensions_prototype_;

  DynamicMessageTest(): factory_(&pool_) {}

  virtual void SetUp() {
    // We want to make sure that DynamicMessage works (particularly with
    // extensions) even if we use descriptors that are *not* from compiled-in
    // types, so we make copies of the descriptors for unittest.proto and
    // unittest_import.proto.
    FileDescriptorProto unittest_file;
    FileDescriptorProto unittest_import_file;

    unittest::TestAllTypes::descriptor()->file()->CopyTo(&unittest_file);
    unittest_import::ImportMessage::descriptor()->file()->CopyTo(
      &unittest_import_file);

    ASSERT_TRUE(pool_.BuildFile(unittest_import_file) != NULL);
    ASSERT_TRUE(pool_.BuildFile(unittest_file) != NULL);

    descriptor_ = pool_.FindMessageTypeByName("protobuf_unittest.TestAllTypes");
    ASSERT_TRUE(descriptor_ != NULL);
    prototype_ = factory_.GetPrototype(descriptor_);

    extensions_descriptor_ =
      pool_.FindMessageTypeByName("protobuf_unittest.TestAllExtensions");
    ASSERT_TRUE(extensions_descriptor_ != NULL);
    extensions_prototype_ = factory_.GetPrototype(extensions_descriptor_);
  }
};

TEST_F(DynamicMessageTest, Descriptor) {
  // Check that the descriptor on the DynamicMessage matches the descriptor
  // passed to GetPrototype().
  EXPECT_EQ(prototype_->GetDescriptor(), descriptor_);
}

TEST_F(DynamicMessageTest, OnePrototype) {
  // Check that requesting the same prototype twice produces the same object.
  EXPECT_EQ(prototype_, factory_.GetPrototype(descriptor_));
}

TEST_F(DynamicMessageTest, Defaults) {
  // Check that all default values are set correctly in the initial message.
  TestUtil::ReflectionTester reflection_tester(descriptor_);
  reflection_tester.ExpectClearViaReflection(*prototype_->GetReflection());
}

TEST_F(DynamicMessageTest, IndependentOffsets) {
  // Check that all fields have independent offsets by setting each
  // one to a unique value then checking that they all still have those
  // unique values (i.e. they don't stomp each other).
  scoped_ptr<Message> message(prototype_->New());
  TestUtil::ReflectionTester reflection_tester(descriptor_);

  reflection_tester.SetAllFieldsViaReflection(message->GetReflection());
  reflection_tester.ExpectAllFieldsSetViaReflection(*message->GetReflection());
}

TEST_F(DynamicMessageTest, Extensions) {
  // Check that extensions work.
  scoped_ptr<Message> message(extensions_prototype_->New());
  TestUtil::ReflectionTester reflection_tester(extensions_descriptor_);

  reflection_tester.SetAllFieldsViaReflection(message->GetReflection());
  reflection_tester.ExpectAllFieldsSetViaReflection(*message->GetReflection());
}

}  // namespace protobuf
}  // namespace google
