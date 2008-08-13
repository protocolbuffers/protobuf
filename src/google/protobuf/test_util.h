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

#ifndef GOOGLE_PROTOBUF_TEST_UTIL_H__
#define GOOGLE_PROTOBUF_TEST_UTIL_H__

#include <stack>
#include <string>
#include <google/protobuf/message.h>
#include <google/protobuf/unittest.pb.h>

namespace google {
namespace protobuf {

namespace unittest = protobuf_unittest;
namespace unittest_import = protobuf_unittest_import;

class TestUtil {
 public:
  // Set every field in the message to a unique value.
  static void SetAllFields(unittest::TestAllTypes* message);
  static void SetAllExtensions(unittest::TestAllExtensions* message);
  static void SetAllFieldsAndExtensions(unittest::TestFieldOrderings* message);

  // Use the repeated versions of the set_*() accessors to modify all the
  // repeated fields of the messsage (which should already have been
  // initialized with SetAllFields()).  SetAllFields() itself only tests
  // the add_*() accessors.
  static void ModifyRepeatedFields(unittest::TestAllTypes* message);
  static void ModifyRepeatedExtensions(unittest::TestAllExtensions* message);

  // Check that all fields have the values that they should have after
  // SetAllFields() is called.
  static void ExpectAllFieldsSet(const unittest::TestAllTypes& message);
  static void ExpectAllExtensionsSet(
      const unittest::TestAllExtensions& message);

  // Expect that the message is modified as would be expected from
  // ModifyRepeatedFields().
  static void ExpectRepeatedFieldsModified(
      const unittest::TestAllTypes& message);
  static void ExpectRepeatedExtensionsModified(
      const unittest::TestAllExtensions& message);

  // Check that all fields have their default values.
  static void ExpectClear(const unittest::TestAllTypes& message);
  static void ExpectExtensionsClear(const unittest::TestAllExtensions& message);

  // Check that the passed-in serialization is the canonical serialization we
  // expect for a TestFieldOrderings message filled in by
  // SetAllFieldsAndExtensions().
  static void ExpectAllFieldsAndExtensionsInOrder(const string& serialized);

  // Like above, but use the reflection interface.
  class ReflectionTester {
   public:
    // base_descriptor must be a descriptor for TestAllTypes or
    // TestAllExtensions.  In the former case, ReflectionTester fetches from
    // it the FieldDescriptors needed to use the reflection interface.  In
    // the latter case, ReflectionTester searches for extension fields in
    // its file.
    explicit ReflectionTester(const Descriptor* base_descriptor);

    void SetAllFieldsViaReflection(Message* message);
    void ModifyRepeatedFieldsViaReflection(Message* message);
    void ExpectAllFieldsSetViaReflection(const Message& message);
    void ExpectClearViaReflection(const Message& message);

   private:
    const FieldDescriptor* F(const string& name);

    const Descriptor* base_descriptor_;

    const FieldDescriptor* group_a_;
    const FieldDescriptor* repeated_group_a_;
    const FieldDescriptor* nested_b_;
    const FieldDescriptor* foreign_c_;
    const FieldDescriptor* import_d_;

    const EnumValueDescriptor* nested_foo_;
    const EnumValueDescriptor* nested_bar_;
    const EnumValueDescriptor* nested_baz_;
    const EnumValueDescriptor* foreign_foo_;
    const EnumValueDescriptor* foreign_bar_;
    const EnumValueDescriptor* foreign_baz_;
    const EnumValueDescriptor* import_foo_;
    const EnumValueDescriptor* import_bar_;
    const EnumValueDescriptor* import_baz_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ReflectionTester);
  };

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(TestUtil);
};

}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_TEST_UTIL_H__
