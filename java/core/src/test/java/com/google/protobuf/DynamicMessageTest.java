// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestEmptyMessage;
import protobuf_unittest.UnittestProto.TestPackedTypes;

import junit.framework.TestCase;

import java.util.Arrays;

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
  TestUtil.ReflectionTester packedReflectionTester =
    new TestUtil.ReflectionTester(TestPackedTypes.getDescriptor(), null);

  public void testDynamicMessageAccessors() throws Exception {
    Message.Builder builder =
      DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.setAllFieldsViaReflection(builder);
    Message message = builder.build();
    reflectionTester.assertAllFieldsSetViaReflection(message);
  }

  public void testSettersAfterBuild() throws Exception {
    Message.Builder builder =
      DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    Message firstMessage = builder.build();
    // double build()
    builder.build();
    // clear() after build()
    builder.clear();
    // setters after build()
    reflectionTester.setAllFieldsViaReflection(builder);
    Message message = builder.build();
    reflectionTester.assertAllFieldsSetViaReflection(message);
    // repeated setters after build()
    reflectionTester.modifyRepeatedFieldsViaReflection(builder);
    message = builder.build();
    reflectionTester.assertRepeatedFieldsModifiedViaReflection(message);
    // firstMessage shouldn't have been modified.
    reflectionTester.assertClearViaReflection(firstMessage);
  }

  public void testUnknownFields() throws Exception {
    Message.Builder builder =
        DynamicMessage.newBuilder(TestEmptyMessage.getDescriptor());
    builder.setUnknownFields(UnknownFieldSet.newBuilder()
        .addField(1, UnknownFieldSet.Field.newBuilder().addVarint(1).build())
        .addField(2, UnknownFieldSet.Field.newBuilder().addFixed32(1).build())
        .build());
    Message message = builder.build();
    assertEquals(2, message.getUnknownFields().asMap().size());
    // clone() with unknown fields
    Message.Builder newBuilder = builder.clone();
    assertEquals(2, newBuilder.getUnknownFields().asMap().size());
    // clear() with unknown fields
    newBuilder.clear();
    assertTrue(newBuilder.getUnknownFields().asMap().isEmpty());
    // serialize/parse with unknown fields
    newBuilder.mergeFrom(message.toByteString());
    assertEquals(2, newBuilder.getUnknownFields().asMap().size());
  }

  public void testDynamicMessageSettersRejectNull() throws Exception {
    Message.Builder builder =
      DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.assertReflectionSettersRejectNull(builder);
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

  public void testDynamicMessageExtensionSettersRejectNull() throws Exception {
    Message.Builder builder =
      DynamicMessage.newBuilder(TestAllExtensions.getDescriptor());
    extensionsReflectionTester.assertReflectionSettersRejectNull(builder);
  }

  public void testDynamicMessageRepeatedSetters() throws Exception {
    Message.Builder builder =
      DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.setAllFieldsViaReflection(builder);
    reflectionTester.modifyRepeatedFieldsViaReflection(builder);
    Message message = builder.build();
    reflectionTester.assertRepeatedFieldsModifiedViaReflection(message);
  }

  public void testDynamicMessageRepeatedSettersRejectNull() throws Exception {
    Message.Builder builder =
      DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.assertReflectionRepeatedSettersRejectNull(builder);
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

    // Test Parser interface.
    Message message3 = message2.getParserForType().parseFrom(rawBytes);
    reflectionTester.assertAllFieldsSetViaReflection(message3);
  }

  public void testDynamicMessageExtensionParsing() throws Exception {
    ByteString rawBytes = TestUtil.getAllExtensionsSet().toByteString();
    Message message = DynamicMessage.parseFrom(
        TestAllExtensions.getDescriptor(), rawBytes,
        TestUtil.getExtensionRegistry());
    extensionsReflectionTester.assertAllFieldsSetViaReflection(message);

    // Test Parser interface.
    Message message2 = message.getParserForType().parseFrom(
        rawBytes, TestUtil.getExtensionRegistry());
    extensionsReflectionTester.assertAllFieldsSetViaReflection(message2);
  }

  public void testDynamicMessagePackedSerialization() throws Exception {
    Message.Builder builder =
        DynamicMessage.newBuilder(TestPackedTypes.getDescriptor());
    packedReflectionTester.setPackedFieldsViaReflection(builder);
    Message message = builder.build();

    ByteString rawBytes = message.toByteString();
    TestPackedTypes message2 = TestPackedTypes.parseFrom(rawBytes);

    TestUtil.assertPackedFieldsSet(message2);

    // In fact, the serialized forms should be exactly the same, byte-for-byte.
    assertEquals(TestUtil.getPackedSet().toByteString(), rawBytes);
  }

  public void testDynamicMessagePackedParsing() throws Exception {
    TestPackedTypes.Builder builder = TestPackedTypes.newBuilder();
    TestUtil.setPackedFields(builder);
    TestPackedTypes message = builder.build();

    ByteString rawBytes = message.toByteString();

    Message message2 =
      DynamicMessage.parseFrom(TestPackedTypes.getDescriptor(), rawBytes);
    packedReflectionTester.assertPackedFieldsSetViaReflection(message2);

    // Test Parser interface.
    Message message3 = message2.getParserForType().parseFrom(rawBytes);
    packedReflectionTester.assertPackedFieldsSetViaReflection(message3);
  }

  public void testDynamicMessageCopy() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();

    DynamicMessage copy = DynamicMessage.newBuilder(message).build();
    reflectionTester.assertAllFieldsSetViaReflection(copy);

    // Test oneof behavior
    FieldDescriptor bytesField =
        TestAllTypes.getDescriptor().findFieldByName("oneof_bytes");
    FieldDescriptor uint32Field =
        TestAllTypes.getDescriptor().findFieldByName("oneof_uint32");
    assertTrue(copy.hasField(bytesField));
    assertFalse(copy.hasField(uint32Field));
    DynamicMessage copy2 =
        DynamicMessage.newBuilder(message).setField(uint32Field, 123).build();
    assertFalse(copy2.hasField(bytesField));
    assertTrue(copy2.hasField(uint32Field));
    assertEquals(123, copy2.getField(uint32Field));
  }

  public void testToBuilder() throws Exception {
    DynamicMessage.Builder builder =
        DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.setAllFieldsViaReflection(builder);
    int unknownFieldNum = 9;
    long unknownFieldVal = 90;
    builder.setUnknownFields(UnknownFieldSet.newBuilder()
        .addField(unknownFieldNum,
            UnknownFieldSet.Field.newBuilder()
                .addVarint(unknownFieldVal).build())
        .build());
    DynamicMessage message = builder.build();

    DynamicMessage derived = message.toBuilder().build();
    reflectionTester.assertAllFieldsSetViaReflection(derived);
    assertEquals(Arrays.asList(unknownFieldVal),
        derived.getUnknownFields().getField(unknownFieldNum).getVarintList());
  }

  public void testDynamicOneofMessage() throws Exception {
    DynamicMessage.Builder builder =
        DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    OneofDescriptor oneof = TestAllTypes.getDescriptor().getOneofs().get(0);
    assertFalse(builder.hasOneof(oneof));
    assertSame(null, builder.getOneofFieldDescriptor(oneof));

    reflectionTester.setAllFieldsViaReflection(builder);
    assertTrue(builder.hasOneof(oneof));
    FieldDescriptor field = oneof.getField(3);
    assertSame(field, builder.getOneofFieldDescriptor(oneof));

    DynamicMessage message = builder.buildPartial();
    assertTrue(message.hasOneof(oneof));

    DynamicMessage.Builder mergedBuilder =
        DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    FieldDescriptor mergedField = oneof.getField(0);
    mergedBuilder.setField(mergedField, 123);
    assertTrue(mergedBuilder.hasField(mergedField));
    mergedBuilder.mergeFrom(message);
    assertTrue(mergedBuilder.hasField(field));
    assertFalse(mergedBuilder.hasField(mergedField));

    builder.clearOneof(oneof);
    assertSame(null, builder.getOneofFieldDescriptor(oneof));
    message = builder.build();
    assertSame(null, message.getOneofFieldDescriptor(oneof));
  }

  // Regression test for a bug that makes setField() not work for repeated
  // enum fields.
  public void testSettersForRepeatedEnumField() throws Exception {
    DynamicMessage.Builder builder =
        DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    FieldDescriptor repeatedEnumField =
        TestAllTypes.getDescriptor().findFieldByName(
            "repeated_nested_enum");
    EnumDescriptor enumDescriptor = TestAllTypes.NestedEnum.getDescriptor();
    builder.setField(repeatedEnumField, enumDescriptor.getValues());
    DynamicMessage message = builder.build();
    assertEquals(
        enumDescriptor.getValues(), message.getField(repeatedEnumField));
  }
}
