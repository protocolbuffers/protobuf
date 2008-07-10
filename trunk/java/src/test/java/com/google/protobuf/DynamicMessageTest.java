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

package com.google.protobuf;

import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestAllExtensions;

import junit.framework.TestCase;

/**
 * Unit test for {@link DynamicMessage}.  See also {@link MessageTest}, which
 * tests some {@link DynamicMessage} functionality.
 *
 * @author kenton@google.com Kenton Varda
 */
public class DynamicMessageTest extends TestCase {
  TestUtil.ReflectionTester reflectionTester =
    new TestUtil.ReflectionTester(TestAllTypes.getDescriptor(), null);

  TestUtil.ReflectionTester extensionsReflectionTester =
    new TestUtil.ReflectionTester(TestAllExtensions.getDescriptor(),
                                  TestUtil.getExtensionRegistry());

  public void testDynamicMessageAccessors() throws Exception {
    Message.Builder builder =
      DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.setAllFieldsViaReflection(builder);
    Message message = builder.build();
    reflectionTester.assertAllFieldsSetViaReflection(message);
  }

  public void testDynamicMessageExtensionAccessors() throws Exception {
    // We don't need to extensively test DynamicMessage's handling of
    // extensions because, frankly, it doesn't do anything special with them.
    // It treats them just like any other fields.
    Message.Builder builder =
      DynamicMessage.newBuilder(TestAllExtensions.getDescriptor());
    extensionsReflectionTester.setAllFieldsViaReflection(builder);
    Message message = builder.build();
    extensionsReflectionTester.assertAllFieldsSetViaReflection(message);
  }

  public void testDynamicMessageRepeatedSetters() throws Exception {
    Message.Builder builder =
      DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.setAllFieldsViaReflection(builder);
    reflectionTester.modifyRepeatedFieldsViaReflection(builder);
    Message message = builder.build();
    reflectionTester.assertRepeatedFieldsModifiedViaReflection(message);
  }

  public void testDynamicMessageDefaults() throws Exception {
    reflectionTester.assertClearViaReflection(
      DynamicMessage.getDefaultInstance(TestAllTypes.getDescriptor()));
    reflectionTester.assertClearViaReflection(
      DynamicMessage.newBuilder(TestAllTypes.getDescriptor()).build());
  }

  public void testDynamicMessageSerializedSize() throws Exception {
    TestAllTypes message = TestUtil.getAllSet();

    Message.Builder dynamicBuilder =
      DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.setAllFieldsViaReflection(dynamicBuilder);
    Message dynamicMessage = dynamicBuilder.build();

    assertEquals(message.getSerializedSize(),
                 dynamicMessage.getSerializedSize());
  }

  public void testDynamicMessageSerialization() throws Exception {
    Message.Builder builder =
      DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.setAllFieldsViaReflection(builder);
    Message message = builder.build();

    ByteString rawBytes = message.toByteString();
    TestAllTypes message2 = TestAllTypes.parseFrom(rawBytes);

    TestUtil.assertAllFieldsSet(message2);

    // In fact, the serialized forms should be exactly the same, byte-for-byte.
    assertEquals(TestUtil.getAllSet().toByteString(), rawBytes);
  }

  public void testDynamicMessageParsing() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();

    ByteString rawBytes = message.toByteString();

    Message message2 =
      DynamicMessage.parseFrom(TestAllTypes.getDescriptor(), rawBytes);
    reflectionTester.assertAllFieldsSetViaReflection(message2);
  }

  public void testDynamicMessageCopy() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();

    DynamicMessage copy = DynamicMessage.newBuilder(message).build();
    reflectionTester.assertAllFieldsSetViaReflection(copy);
  }
}
