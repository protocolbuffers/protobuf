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

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.FieldPresenceTestProto.TestAllTypes;
import com.google.protobuf.FieldPresenceTestProto.TestOptionalFieldsOnly;
import com.google.protobuf.FieldPresenceTestProto.TestRepeatedFieldsOnly;
import protobuf_unittest.UnittestProto;

import junit.framework.TestCase;

/**
 * Unit tests for protos that doesn't support field presence test for optional
 * non-message fields.
 */
public class FieldPresenceTest extends TestCase {
  private static boolean hasMethod(Class<?> clazz, String name) {
    try {
      if (clazz.getMethod(name) != null) {
        return true;
      } else {
        return false;
      }
    } catch (NoSuchMethodException e) {
      return false;
    }
  }

  private static void assertHasMethodRemoved(
      Class<?> classWithFieldPresence,
      Class<?> classWithoutFieldPresence,
      String camelName) {
    assertTrue(hasMethod(classWithFieldPresence, "get" + camelName));
    assertTrue(hasMethod(classWithFieldPresence, "has" + camelName));
    assertTrue(hasMethod(classWithoutFieldPresence, "get" + camelName));
    assertFalse(hasMethod(classWithoutFieldPresence, "has" + camelName));
  }

  public void testHasMethod() {
    // Optional non-message fields don't have a hasFoo() method generated.
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.class,
        TestAllTypes.class,
        "OptionalInt32");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.class,
        TestAllTypes.class,
        "OptionalString");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.class,
        TestAllTypes.class,
        "OptionalBytes");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.class,
        TestAllTypes.class,
        "OptionalNestedEnum");

    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.Builder.class,
        TestAllTypes.Builder.class,
        "OptionalInt32");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.Builder.class,
        TestAllTypes.Builder.class,
        "OptionalString");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.Builder.class,
        TestAllTypes.Builder.class,
        "OptionalBytes");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.Builder.class,
        TestAllTypes.Builder.class,
        "OptionalNestedEnum");

    // message fields still have the hasFoo() method generated.
    assertFalse(TestAllTypes.newBuilder().build().hasOptionalNestedMessage());
    assertFalse(TestAllTypes.newBuilder().hasOptionalNestedMessage());

    // oneof fields don't have hasFoo() methods (even for message types).
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.class,
        TestAllTypes.class,
        "OneofUint32");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.class,
        TestAllTypes.class,
        "OneofString");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.class,
        TestAllTypes.class,
        "OneofBytes");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.class,
        TestAllTypes.class,
        "OneofNestedMessage");

    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.Builder.class,
        TestAllTypes.Builder.class,
        "OneofUint32");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.Builder.class,
        TestAllTypes.Builder.class,
        "OneofString");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.Builder.class,
        TestAllTypes.Builder.class,
        "OneofBytes");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.Builder.class,
        TestAllTypes.Builder.class,
        "OneofNestedMessage");
  }

  public void testOneofEquals() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestAllTypes message1 = builder.build();
    // Set message2's oneof_uint32 field to defalut value. The two
    // messages should be different when check with oneof case.
    builder.setOneofUint32(0);
    TestAllTypes message2 = builder.build();
    assertFalse(message1.equals(message2));
  }

  public void testLazyField() throws Exception {
    // Test default constructed message.
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestAllTypes message = builder.build();
    assertFalse(message.hasOptionalLazyMessage());
    assertEquals(0, message.getSerializedSize());
    assertEquals(ByteString.EMPTY, message.toByteString());

    // Set default instance to the field.
    builder.setOptionalLazyMessage(TestAllTypes.NestedMessage.getDefaultInstance());
    message = builder.build();
    assertTrue(message.hasOptionalLazyMessage());
    assertEquals(2, message.getSerializedSize());

    // Test parse zero-length from wire sets the presence.
    TestAllTypes parsed = TestAllTypes.parseFrom(message.toByteString());
    assertTrue(parsed.hasOptionalLazyMessage());
    assertEquals(message.getOptionalLazyMessage(), parsed.getOptionalLazyMessage());
  }

  public void testFieldPresence() {
    // Optional non-message fields set to their default value are treated the
    // same way as not set.

    // Serialization will ignore such fields.
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.setOptionalInt32(0);
    builder.setOptionalString("");
    builder.setOptionalBytes(ByteString.EMPTY);
    builder.setOptionalNestedEnum(TestAllTypes.NestedEnum.FOO);
    TestAllTypes message = builder.build();
    assertEquals(0, message.getSerializedSize());

    // mergeFrom() will ignore such fields.
    TestAllTypes.Builder a = TestAllTypes.newBuilder();
    a.setOptionalInt32(1);
    a.setOptionalString("x");
    a.setOptionalBytes(ByteString.copyFromUtf8("y"));
    a.setOptionalNestedEnum(TestAllTypes.NestedEnum.BAR);
    TestAllTypes.Builder b = TestAllTypes.newBuilder();
    b.setOptionalInt32(0);
    b.setOptionalString("");
    b.setOptionalBytes(ByteString.EMPTY);
    b.setOptionalNestedEnum(TestAllTypes.NestedEnum.FOO);
    a.mergeFrom(b.build());
    message = a.build();
    assertEquals(1, message.getOptionalInt32());
    assertEquals("x", message.getOptionalString());
    assertEquals(ByteString.copyFromUtf8("y"), message.getOptionalBytes());
    assertEquals(TestAllTypes.NestedEnum.BAR, message.getOptionalNestedEnum());

    // equals()/hashCode() should produce the same results.
    TestAllTypes empty = TestAllTypes.newBuilder().build();
    message = builder.build();
    assertTrue(empty.equals(message));
    assertTrue(message.equals(empty));
    assertEquals(empty.hashCode(), message.hashCode());
  }

  public void testFieldPresenceByReflection() {
    Descriptor descriptor = TestAllTypes.getDescriptor();
    FieldDescriptor optionalInt32Field = descriptor.findFieldByName("optional_int32");
    FieldDescriptor optionalStringField = descriptor.findFieldByName("optional_string");
    FieldDescriptor optionalBytesField = descriptor.findFieldByName("optional_bytes");
    FieldDescriptor optionalNestedEnumField = descriptor.findFieldByName("optional_nested_enum");

    // Field not present.
    TestAllTypes message = TestAllTypes.newBuilder().build();
    assertFalse(message.hasField(optionalInt32Field));
    assertFalse(message.hasField(optionalStringField));
    assertFalse(message.hasField(optionalBytesField));
    assertFalse(message.hasField(optionalNestedEnumField));
    assertEquals(0, message.getAllFields().size());

    // Field set to default value is seen as not present.
    message = TestAllTypes.newBuilder()
        .setOptionalInt32(0)
        .setOptionalString("")
        .setOptionalBytes(ByteString.EMPTY)
        .setOptionalNestedEnum(TestAllTypes.NestedEnum.FOO)
        .build();
    assertFalse(message.hasField(optionalInt32Field));
    assertFalse(message.hasField(optionalStringField));
    assertFalse(message.hasField(optionalBytesField));
    assertFalse(message.hasField(optionalNestedEnumField));
    assertEquals(0, message.getAllFields().size());

    // Field set to non-default value is seen as present.
    message = TestAllTypes.newBuilder()
        .setOptionalInt32(1)
        .setOptionalString("x")
        .setOptionalBytes(ByteString.copyFromUtf8("y"))
        .setOptionalNestedEnum(TestAllTypes.NestedEnum.BAR)
        .build();
    assertTrue(message.hasField(optionalInt32Field));
    assertTrue(message.hasField(optionalStringField));
    assertTrue(message.hasField(optionalBytesField));
    assertTrue(message.hasField(optionalNestedEnumField));
    assertEquals(4, message.getAllFields().size());
  }

  public void testFieldPresenceDynamicMessage() {
    Descriptor descriptor = TestAllTypes.getDescriptor();
    FieldDescriptor optionalInt32Field = descriptor.findFieldByName("optional_int32");
    FieldDescriptor optionalStringField = descriptor.findFieldByName("optional_string");
    FieldDescriptor optionalBytesField = descriptor.findFieldByName("optional_bytes");
    FieldDescriptor optionalNestedEnumField = descriptor.findFieldByName("optional_nested_enum");
    EnumDescriptor enumDescriptor = optionalNestedEnumField.getEnumType();
    EnumValueDescriptor defaultEnumValueDescriptor = enumDescriptor.getValues().get(0);
    EnumValueDescriptor nonDefaultEnumValueDescriptor = enumDescriptor.getValues().get(1);

    DynamicMessage defaultInstance = DynamicMessage.getDefaultInstance(descriptor);
    // Field not present.
    DynamicMessage message = defaultInstance.newBuilderForType().build();
    assertFalse(message.hasField(optionalInt32Field));
    assertFalse(message.hasField(optionalStringField));
    assertFalse(message.hasField(optionalBytesField));
    assertFalse(message.hasField(optionalNestedEnumField));
    assertEquals(0, message.getAllFields().size());

    // Field set to non-default value is seen as present.
    message =
        defaultInstance
            .newBuilderForType()
            .setField(optionalInt32Field, 1)
            .setField(optionalStringField, "x")
            .setField(optionalBytesField, ByteString.copyFromUtf8("y"))
            .setField(optionalNestedEnumField, nonDefaultEnumValueDescriptor)
            .build();
    assertTrue(message.hasField(optionalInt32Field));
    assertTrue(message.hasField(optionalStringField));
    assertTrue(message.hasField(optionalBytesField));
    assertTrue(message.hasField(optionalNestedEnumField));
    assertEquals(4, message.getAllFields().size());

    // Field set to default value is seen as not present.
    message = message.toBuilder()
            .setField(optionalInt32Field, 0)
            .setField(optionalStringField, "")
            .setField(optionalBytesField, ByteString.EMPTY)
            .setField(optionalNestedEnumField, defaultEnumValueDescriptor)
            .build();
    assertFalse(message.hasField(optionalInt32Field));
    assertFalse(message.hasField(optionalStringField));
    assertFalse(message.hasField(optionalBytesField));
    assertFalse(message.hasField(optionalNestedEnumField));
    assertEquals(0, message.getAllFields().size());
  }

  public void testMessageField() {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    assertFalse(builder.hasOptionalNestedMessage());
    assertFalse(builder.build().hasOptionalNestedMessage());

    TestAllTypes.NestedMessage.Builder nestedBuilder =
        builder.getOptionalNestedMessageBuilder();
    assertTrue(builder.hasOptionalNestedMessage());
    assertTrue(builder.build().hasOptionalNestedMessage());

    nestedBuilder.setValue(1);
    assertEquals(1, builder.build().getOptionalNestedMessage().getValue());

    builder.clearOptionalNestedMessage();
    assertFalse(builder.hasOptionalNestedMessage());
    assertFalse(builder.build().hasOptionalNestedMessage());

    // Unlike non-message fields, if we set a message field to its default value (i.e.,
    // default instance), the field should be seen as present.
    builder.setOptionalNestedMessage(TestAllTypes.NestedMessage.getDefaultInstance());
    assertTrue(builder.hasOptionalNestedMessage());
    assertTrue(builder.build().hasOptionalNestedMessage());
  }

  public void testSerializeAndParse() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.setOptionalInt32(1234);
    builder.setOptionalString("hello");
    builder.setOptionalNestedMessage(TestAllTypes.NestedMessage.getDefaultInstance());
    // Set an oneof field to its default value and expect it to be serialized (i.e.,
    // an oneof field set to the default value should be treated as present).
    builder.setOneofInt32(0);
    ByteString data = builder.build().toByteString();

    TestAllTypes message = TestAllTypes.parseFrom(data);
    assertEquals(1234, message.getOptionalInt32());
    assertEquals("hello", message.getOptionalString());
    // Fields not set will have the default value.
    assertEquals(ByteString.EMPTY, message.getOptionalBytes());
    assertEquals(TestAllTypes.NestedEnum.FOO, message.getOptionalNestedEnum());
    // The message field is set despite that it's set with a default instance.
    assertTrue(message.hasOptionalNestedMessage());
    assertEquals(0, message.getOptionalNestedMessage().getValue());
    // The oneof field set to its default value is also present.
    assertEquals(
        TestAllTypes.OneofFieldCase.ONEOF_INT32, message.getOneofFieldCase());
  }

  // Regression test for b/16173397
  // Make sure we haven't screwed up the code generation for repeated fields.
  public void testRepeatedFields() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.setOptionalInt32(1234);
    builder.setOptionalString("hello");
    builder.setOptionalNestedMessage(TestAllTypes.NestedMessage.getDefaultInstance());
    builder.addRepeatedInt32(4321);
    builder.addRepeatedString("world");
    builder.addRepeatedNestedMessage(TestAllTypes.NestedMessage.getDefaultInstance());
    ByteString data = builder.build().toByteString();

    TestOptionalFieldsOnly optionalOnlyMessage = TestOptionalFieldsOnly.parseFrom(data);
    assertEquals(1234, optionalOnlyMessage.getOptionalInt32());
    assertEquals("hello", optionalOnlyMessage.getOptionalString());
    assertTrue(optionalOnlyMessage.hasOptionalNestedMessage());
    assertEquals(0, optionalOnlyMessage.getOptionalNestedMessage().getValue());

    TestRepeatedFieldsOnly repeatedOnlyMessage = TestRepeatedFieldsOnly.parseFrom(data);
    assertEquals(1, repeatedOnlyMessage.getRepeatedInt32Count());
    assertEquals(4321, repeatedOnlyMessage.getRepeatedInt32(0));
    assertEquals(1, repeatedOnlyMessage.getRepeatedStringCount());
    assertEquals("world", repeatedOnlyMessage.getRepeatedString(0));
    assertEquals(1, repeatedOnlyMessage.getRepeatedNestedMessageCount());
    assertEquals(0, repeatedOnlyMessage.getRepeatedNestedMessage(0).getValue());
  }

  public void testIsInitialized() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();

    // Test optional proto2 message fields.
    UnittestProto.TestRequired.Builder proto2Builder =
        builder.getOptionalProto2MessageBuilder();
    assertFalse(builder.isInitialized());
    assertFalse(builder.buildPartial().isInitialized());

    proto2Builder.setA(1).setB(2).setC(3);
    assertTrue(builder.isInitialized());
    assertTrue(builder.buildPartial().isInitialized());

    // Test oneof proto2 message fields.
    proto2Builder = builder.getOneofProto2MessageBuilder();
    assertFalse(builder.isInitialized());
    assertFalse(builder.buildPartial().isInitialized());

    proto2Builder.setA(1).setB(2).setC(3);
    assertTrue(builder.isInitialized());
    assertTrue(builder.buildPartial().isInitialized());

    // Test repeated proto2 message fields.
    proto2Builder = builder.addRepeatedProto2MessageBuilder();
    assertFalse(builder.isInitialized());
    assertFalse(builder.buildPartial().isInitialized());

    proto2Builder.setA(1).setB(2).setC(3);
    assertTrue(builder.isInitialized());
    assertTrue(builder.buildPartial().isInitialized());
  }


  // Test that unknown fields are dropped.
  public void testUnknownFields() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.setOptionalInt32(1234);
    builder.addRepeatedInt32(5678);
    TestAllTypes message = builder.build();
    ByteString data = message.toByteString();

    TestOptionalFieldsOnly optionalOnlyMessage =
        TestOptionalFieldsOnly.parseFrom(data);
    // UnknownFieldSet should be empty.
    assertEquals(
        0, optionalOnlyMessage.getUnknownFields().toByteString().size());
    assertEquals(1234, optionalOnlyMessage.getOptionalInt32());
    message = TestAllTypes.parseFrom(optionalOnlyMessage.toByteString());
    assertEquals(1234, message.getOptionalInt32());
    // The repeated field is discarded because it's unknown to the optional-only
    // message.
    assertEquals(0, message.getRepeatedInt32Count());

    DynamicMessage dynamicOptionalOnlyMessage =
        DynamicMessage.getDefaultInstance(
            TestOptionalFieldsOnly.getDescriptor())
        .getParserForType().parseFrom(data);
    assertEquals(
        0, dynamicOptionalOnlyMessage.getUnknownFields().toByteString().size());
    assertEquals(optionalOnlyMessage.toByteString(),
        dynamicOptionalOnlyMessage.toByteString());
  }
}
