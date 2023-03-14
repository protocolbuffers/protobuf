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

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;
import dynamicmessagetest.DynamicMessageTestProto.EmptyMessage;
import dynamicmessagetest.DynamicMessageTestProto.MessageWithMapFields;
import protobuf_unittest.UnittestMset.TestMessageSetExtension2;
import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestAllTypes.NestedMessage;
import protobuf_unittest.UnittestProto.TestEmptyMessage;
import protobuf_unittest.UnittestProto.TestPackedTypes;
import proto2_wireformat_unittest.UnittestMsetWireFormat.TestMessageSet;
import java.util.ArrayList;
import org.junit.Test;
import org.junit.function.ThrowingRunnable;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Unit test for {@link DynamicMessage}. See also {@link MessageTest}, which tests some {@link
 * DynamicMessage} functionality.
 */
@RunWith(JUnit4.class)
public class DynamicMessageTest {
  TestUtil.ReflectionTester reflectionTester =
      new TestUtil.ReflectionTester(TestAllTypes.getDescriptor(), null);

  TestUtil.ReflectionTester extensionsReflectionTester =
      new TestUtil.ReflectionTester(
          TestAllExtensions.getDescriptor(), TestUtil.getFullExtensionRegistry());
  TestUtil.ReflectionTester packedReflectionTester =
      new TestUtil.ReflectionTester(TestPackedTypes.getDescriptor(), null);

  @Test
  public void testDynamicMessageAccessors() throws Exception {
    Message.Builder builder = DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.setAllFieldsViaReflection(builder);
    Message message = builder.build();
    reflectionTester.assertAllFieldsSetViaReflection(message);
  }

  @Test
  public void testSettersAfterBuild() throws Exception {
    Message.Builder builder = DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    Message firstMessage = builder.build();
    // double build()
    Message unused = builder.build();
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

  @Test
  public void testUnknownFields() throws Exception {
    Message.Builder builder = DynamicMessage.newBuilder(TestEmptyMessage.getDescriptor());
    builder.setUnknownFields(
        UnknownFieldSet.newBuilder()
            .addField(1, UnknownFieldSet.Field.newBuilder().addVarint(1).build())
            .addField(2, UnknownFieldSet.Field.newBuilder().addFixed32(1).build())
            .build());
    Message message = builder.build();
    assertThat(builder.getUnknownFields().asMap()).hasSize(2);
    // clone() with unknown fields
    Message.Builder newBuilder = builder.clone();
    assertThat(newBuilder.getUnknownFields().asMap()).hasSize(2);
    // clear() with unknown fields
    newBuilder.clear();
    assertThat(newBuilder.getUnknownFields().asMap()).isEmpty();
    // serialize/parse with unknown fields
    newBuilder.mergeFrom(message.toByteString());
    assertThat(newBuilder.getUnknownFields().asMap()).hasSize(2);
  }

  @Test
  public void testDynamicMessageSettersRejectNull() throws Exception {
    Message.Builder builder = DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.assertReflectionSettersRejectNull(builder);
  }

  @Test
  public void testDynamicMessageExtensionAccessors() throws Exception {
    // We don't need to extensively test DynamicMessage's handling of
    // extensions because, frankly, it doesn't do anything special with them.
    // It treats them just like any other fields.
    Message.Builder builder = DynamicMessage.newBuilder(TestAllExtensions.getDescriptor());
    extensionsReflectionTester.setAllFieldsViaReflection(builder);
    Message message = builder.build();
    extensionsReflectionTester.assertAllFieldsSetViaReflection(message);
  }

  @Test
  public void testDynamicMessageExtensionSettersRejectNull() throws Exception {
    Message.Builder builder = DynamicMessage.newBuilder(TestAllExtensions.getDescriptor());
    extensionsReflectionTester.assertReflectionSettersRejectNull(builder);
  }

  @Test
  public void testDynamicMessageRepeatedSetters() throws Exception {
    Message.Builder builder = DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.setAllFieldsViaReflection(builder);
    reflectionTester.modifyRepeatedFieldsViaReflection(builder);
    Message message = builder.build();
    reflectionTester.assertRepeatedFieldsModifiedViaReflection(message);
  }

  @Test
  public void testDynamicMessageRepeatedSettersRejectNull() throws Exception {
    Message.Builder builder = DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.assertReflectionRepeatedSettersRejectNull(builder);
  }

  @Test
  public void testDynamicMessageDefaults() throws Exception {
    reflectionTester.assertClearViaReflection(
        DynamicMessage.getDefaultInstance(TestAllTypes.getDescriptor()));
    reflectionTester.assertClearViaReflection(
        DynamicMessage.newBuilder(TestAllTypes.getDescriptor()).build());
  }

  @Test
  public void testDynamicMessageSerializedSize() throws Exception {
    TestAllTypes message = TestUtil.getAllSet();

    Message.Builder dynamicBuilder = DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.setAllFieldsViaReflection(dynamicBuilder);
    Message dynamicMessage = dynamicBuilder.build();

    assertThat(message.getSerializedSize()).isEqualTo(dynamicMessage.getSerializedSize());
  }

  @Test
  public void testDynamicMessageSerialization() throws Exception {
    Message.Builder builder = DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.setAllFieldsViaReflection(builder);
    Message message = builder.build();

    ByteString rawBytes = message.toByteString();
    TestAllTypes message2 = TestAllTypes.parseFrom(rawBytes);

    TestUtil.assertAllFieldsSet(message2);

    // In fact, the serialized forms should be exactly the same, byte-for-byte.
    assertThat(rawBytes).isEqualTo(TestUtil.getAllSet().toByteString());
  }

  @Test
  public void testDynamicMessageParsing() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();

    ByteString rawBytes = message.toByteString();

    Message message2 = DynamicMessage.parseFrom(TestAllTypes.getDescriptor(), rawBytes);
    reflectionTester.assertAllFieldsSetViaReflection(message2);

    // Test Parser interface.
    Message message3 = message2.getParserForType().parseFrom(rawBytes);
    reflectionTester.assertAllFieldsSetViaReflection(message3);
  }

  @Test
  public void testDynamicMessageExtensionParsing() throws Exception {
    ByteString rawBytes = TestUtil.getAllExtensionsSet().toByteString();
    Message message =
        DynamicMessage.parseFrom(
            TestAllExtensions.getDescriptor(), rawBytes, TestUtil.getFullExtensionRegistry());
    extensionsReflectionTester.assertAllFieldsSetViaReflection(message);

    // Test Parser interface.
    Message message2 =
        message.getParserForType().parseFrom(rawBytes, TestUtil.getExtensionRegistry());
    extensionsReflectionTester.assertAllFieldsSetViaReflection(message2);
  }

  @Test
  public void testDynamicMessagePackedSerialization() throws Exception {
    Message.Builder builder = DynamicMessage.newBuilder(TestPackedTypes.getDescriptor());
    packedReflectionTester.setPackedFieldsViaReflection(builder);
    Message message = builder.build();

    ByteString rawBytes = message.toByteString();
    TestPackedTypes message2 = TestPackedTypes.parseFrom(rawBytes);

    TestUtil.assertPackedFieldsSet(message2);

    // In fact, the serialized forms should be exactly the same, byte-for-byte.
    assertThat(rawBytes).isEqualTo(TestUtil.getPackedSet().toByteString());
  }

  @Test
  public void testDynamicMessagePackedEmptySerialization() throws Exception {
    Message message =
        DynamicMessage.newBuilder(TestPackedTypes.getDescriptor())
            .setField(
                TestPackedTypes.getDescriptor()
                    .findFieldByNumber(TestPackedTypes.PACKED_INT64_FIELD_NUMBER),
                new ArrayList<Long>())
            .build();

    assertThat(message.toByteString()).isEqualTo(ByteString.EMPTY);
  }

  @Test
  public void testDynamicMessagePackedParsing() throws Exception {
    TestPackedTypes.Builder builder = TestPackedTypes.newBuilder();
    TestUtil.setPackedFields(builder);
    TestPackedTypes message = builder.build();

    ByteString rawBytes = message.toByteString();

    Message message2 = DynamicMessage.parseFrom(TestPackedTypes.getDescriptor(), rawBytes);
    packedReflectionTester.assertPackedFieldsSetViaReflection(message2);

    // Test Parser interface.
    Message message3 = message2.getParserForType().parseFrom(rawBytes);
    packedReflectionTester.assertPackedFieldsSetViaReflection(message3);
  }

  @Test
  public void testGetBuilderForExtensionField() {
    DynamicMessage.Builder builder = DynamicMessage.newBuilder(TestAllExtensions.getDescriptor());
    Message.Builder fieldBuilder =
        builder.newBuilderForField(UnittestProto.optionalNestedMessageExtension.getDescriptor());
    final int expected = 7432;
    FieldDescriptor field =
        NestedMessage.getDescriptor().findFieldByNumber(NestedMessage.BB_FIELD_NUMBER);
    fieldBuilder.setField(field, expected);
    assertThat(fieldBuilder.build().getField(field)).isEqualTo(expected);
  }

  @Test
  public void testDynamicMessageCopy() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();

    DynamicMessage copy = DynamicMessage.newBuilder(message).build();
    reflectionTester.assertAllFieldsSetViaReflection(copy);

    // Test oneof behavior
    FieldDescriptor bytesField = TestAllTypes.getDescriptor().findFieldByName("oneof_bytes");
    FieldDescriptor uint32Field = TestAllTypes.getDescriptor().findFieldByName("oneof_uint32");
    assertThat(copy.hasField(bytesField)).isTrue();
    assertThat(copy.hasField(uint32Field)).isFalse();
    DynamicMessage copy2 = DynamicMessage.newBuilder(message).setField(uint32Field, 123).build();
    assertThat(copy2.hasField(bytesField)).isFalse();
    assertThat(copy2.hasField(uint32Field)).isTrue();
    assertThat(copy2.getField(uint32Field)).isEqualTo(123);
  }

  @Test
  public void testToBuilder() throws Exception {
    DynamicMessage.Builder builder = DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    reflectionTester.setAllFieldsViaReflection(builder);
    int unknownFieldNum = 9;
    long unknownFieldVal = 90;
    builder.setUnknownFields(
        UnknownFieldSet.newBuilder()
            .addField(
                unknownFieldNum,
                UnknownFieldSet.Field.newBuilder().addVarint(unknownFieldVal).build())
            .build());
    DynamicMessage message = builder.build();

    DynamicMessage derived = message.toBuilder().build();
    reflectionTester.assertAllFieldsSetViaReflection(derived);
    assertThat(derived.getUnknownFields().getField(unknownFieldNum).getVarintList())
        .containsExactly(unknownFieldVal);
  }

  @Test
  public void testDynamicOneofMessage() throws Exception {
    DynamicMessage.Builder builder = DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    OneofDescriptor oneof = TestAllTypes.getDescriptor().getOneofs().get(0);
    assertThat(builder.hasOneof(oneof)).isFalse();
    assertThat(builder.getOneofFieldDescriptor(oneof)).isNull();

    reflectionTester.setAllFieldsViaReflection(builder);
    assertThat(builder.hasOneof(oneof)).isTrue();
    FieldDescriptor field = oneof.getField(3);
    assertThat(builder.getOneofFieldDescriptor(oneof)).isSameInstanceAs(field);

    DynamicMessage message = builder.buildPartial();
    assertThat(message.hasOneof(oneof)).isTrue();

    DynamicMessage.Builder mergedBuilder = DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    FieldDescriptor mergedField = oneof.getField(0);
    mergedBuilder.setField(mergedField, 123);
    assertThat(mergedBuilder.hasField(mergedField)).isTrue();
    mergedBuilder.mergeFrom(message);
    assertThat(mergedBuilder.hasField(field)).isTrue();
    assertThat(mergedBuilder.hasField(mergedField)).isFalse();

    builder.clearOneof(oneof);
    assertThat(builder.getOneofFieldDescriptor(oneof)).isNull();
    message = builder.build();
    assertThat(message.getOneofFieldDescriptor(oneof)).isNull();
  }

  // Regression test for a bug that makes setField() not work for repeated
  // enum fields.
  @Test
  public void testSettersForRepeatedEnumField() throws Exception {
    DynamicMessage.Builder builder = DynamicMessage.newBuilder(TestAllTypes.getDescriptor());
    FieldDescriptor repeatedEnumField =
        TestAllTypes.getDescriptor().findFieldByName("repeated_nested_enum");
    EnumDescriptor enumDescriptor = TestAllTypes.NestedEnum.getDescriptor();
    builder.setField(repeatedEnumField, enumDescriptor.getValues());
    DynamicMessage message = builder.build();
    assertThat(message.getField(repeatedEnumField)).isEqualTo(enumDescriptor.getValues());
  }

  @Test
  public void testBuilderGetFieldBuilder_mapField_throwsUnsupportedOperationException() {
    final DynamicMessage.Builder builder =
        DynamicMessage.newBuilder(MessageWithMapFields.getDescriptor());
    final FieldDescriptor mapField =
        MessageWithMapFields.getDescriptor().findFieldByName("string_message_map");

    Message.Builder entryBuilder = builder.newBuilderForField(mapField);
    entryBuilder.setField(entryBuilder.getDescriptorForType().findFieldByNumber(1), "foo");
    entryBuilder.setField(
        entryBuilder.getDescriptorForType().findFieldByNumber(2),
        EmptyMessage.getDefaultInstance());
    builder.addRepeatedField(mapField, entryBuilder.build());

    assertThrows(
        UnsupportedOperationException.class,
        new ThrowingRunnable() {
          @Override
          public void run() throws Throwable {
            builder.getFieldBuilder(mapField);
          }
        });
  }

  @Test
  public void testBuilderGetRepeatedFieldBuilder_mapField_throwsUnsupportedOperationException() {
    final DynamicMessage.Builder builder =
        DynamicMessage.newBuilder(MessageWithMapFields.getDescriptor());
    final FieldDescriptor mapField =
        MessageWithMapFields.getDescriptor().findFieldByName("string_message_map");

    Message.Builder entryBuilder = builder.newBuilderForField(mapField);
    entryBuilder.setField(entryBuilder.getDescriptorForType().findFieldByNumber(1), "foo");
    entryBuilder.setField(
        entryBuilder.getDescriptorForType().findFieldByNumber(2),
        EmptyMessage.getDefaultInstance());
    builder.addRepeatedField(mapField, entryBuilder.build());

    assertThrows(
        UnsupportedOperationException.class,
        new ThrowingRunnable() {
          @Override
          public void run() throws Throwable {
            builder.getFieldBuilder(mapField);
          }
        });
  }

  @Test
  public void serialize_lazyFieldInMessageSet() throws Exception {
    ExtensionRegistry extensionRegistry = ExtensionRegistry.newInstance();
    extensionRegistry.add(TestMessageSetExtension2.messageSetExtension);
    TestMessageSetExtension2 messageSetExtension =
        TestMessageSetExtension2.newBuilder().setStr("foo").build();
    // This is a valid serialization of the above message.
    ByteString suboptimallySerializedMessageSetExtension =
        messageSetExtension.toByteString().concat(messageSetExtension.toByteString());
    DynamicMessage expectedMessage =
        DynamicMessage.newBuilder(TestMessageSet.getDescriptor())
            .setField(
                TestMessageSetExtension2.messageSetExtension.getDescriptor(), messageSetExtension)
            .build();
    // Constructed with a LazyField, for whom roundtripping the serialized form will shorten the
    // encoded form.
    // In particular, this checks matching between lazy field encoding size.
    DynamicMessage complicatedlyBuiltMessage =
        DynamicMessage.newBuilder(TestMessageSet.getDescriptor())
            .setField(
                TestMessageSetExtension2.messageSetExtension.getDescriptor(),
                new LazyField(
                    DynamicMessage.getDefaultInstance(TestMessageSetExtension2.getDescriptor()),
                    extensionRegistry,
                    suboptimallySerializedMessageSetExtension))
            .build();

    DynamicMessage roundtrippedMessage =
        DynamicMessage.newBuilder(TestMessageSet.getDescriptor())
            .mergeFrom(complicatedlyBuiltMessage.toByteString(), extensionRegistry)
            .build();

    assertThat(complicatedlyBuiltMessage).isEqualTo(expectedMessage);
    assertThat(roundtrippedMessage).isEqualTo(expectedMessage);
  }
}
