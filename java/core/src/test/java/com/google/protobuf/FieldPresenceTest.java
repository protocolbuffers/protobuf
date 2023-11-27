// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;
import com.google.protobuf.FieldPresenceTestProto.TestAllTypes;
import com.google.protobuf.FieldPresenceTestProto.TestOptionalFieldsOnly;
import com.google.protobuf.FieldPresenceTestProto.TestRepeatedFieldsOnly;
import com.google.protobuf.testing.proto.TestProto3Optional;
import protobuf_unittest.UnittestProto;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Unit tests for protos that doesn't support field presence test for optional non-message fields.
 */
@RunWith(JUnit4.class)
public class FieldPresenceTest {
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
      Class<?> classWithFieldPresence, Class<?> classWithoutFieldPresence, String camelName) {
    assertThat(hasMethod(classWithFieldPresence, "get" + camelName)).isTrue();
    assertThat(hasMethod(classWithFieldPresence, "has" + camelName)).isTrue();
    assertThat(hasMethod(classWithoutFieldPresence, "get" + camelName)).isTrue();
    assertThat(hasMethod(classWithoutFieldPresence, "has" + camelName)).isFalse();
  }

  private static void assertHasMethodExisting(Class<?> clazz, String camelName) {
    assertThat(hasMethod(clazz, "get" + camelName)).isTrue();
    assertThat(hasMethod(clazz, "has" + camelName)).isTrue();
  }

  @Test
  public void testHasMethod() {
    // Optional non-message fields don't have a hasFoo() method generated.
    assertHasMethodRemoved(UnittestProto.TestAllTypes.class, TestAllTypes.class, "OptionalInt32");
    assertHasMethodRemoved(UnittestProto.TestAllTypes.class, TestAllTypes.class, "OptionalString");
    assertHasMethodRemoved(UnittestProto.TestAllTypes.class, TestAllTypes.class, "OptionalBytes");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.class, TestAllTypes.class, "OptionalNestedEnum");

    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.Builder.class, TestAllTypes.Builder.class, "OptionalInt32");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.Builder.class, TestAllTypes.Builder.class, "OptionalString");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.Builder.class, TestAllTypes.Builder.class, "OptionalBytes");
    assertHasMethodRemoved(
        UnittestProto.TestAllTypes.Builder.class, TestAllTypes.Builder.class, "OptionalNestedEnum");

    // message fields still have the hasFoo() method generated.
    assertThat(TestAllTypes.getDefaultInstance().hasOptionalNestedMessage()).isFalse();
    assertThat(TestAllTypes.newBuilder().hasOptionalNestedMessage()).isFalse();

    // oneof fields support hasFoo() methods for non-message types.
    assertHasMethodExisting(TestAllTypes.class, "OneofUint32");
    assertHasMethodExisting(TestAllTypes.class, "OneofString");
    assertHasMethodExisting(TestAllTypes.class, "OneofBytes");
    assertThat(TestAllTypes.getDefaultInstance().hasOneofNestedMessage()).isFalse();
    assertThat(TestAllTypes.newBuilder().hasOneofNestedMessage()).isFalse();

    assertHasMethodExisting(TestAllTypes.Builder.class, "OneofUint32");
    assertHasMethodExisting(TestAllTypes.Builder.class, "OneofString");
    assertHasMethodExisting(TestAllTypes.Builder.class, "OneofBytes");
  }

  @Test
  public void testHasMethodForProto3Optional() throws Exception {
    assertThat(TestProto3Optional.getDefaultInstance().hasOptionalInt32()).isFalse();
    assertThat(TestProto3Optional.getDefaultInstance().hasOptionalInt64()).isFalse();
    assertThat(TestProto3Optional.getDefaultInstance().hasOptionalUint32()).isFalse();
    assertThat(TestProto3Optional.getDefaultInstance().hasOptionalUint64()).isFalse();
    assertThat(TestProto3Optional.getDefaultInstance().hasOptionalSint32()).isFalse();
    assertThat(TestProto3Optional.getDefaultInstance().hasOptionalSint64()).isFalse();
    assertThat(TestProto3Optional.getDefaultInstance().hasOptionalFixed32()).isFalse();
    assertThat(TestProto3Optional.getDefaultInstance().hasOptionalFixed64()).isFalse();
    assertThat(TestProto3Optional.getDefaultInstance().hasOptionalFloat()).isFalse();
    assertThat(TestProto3Optional.getDefaultInstance().hasOptionalDouble()).isFalse();
    assertThat(TestProto3Optional.getDefaultInstance().hasOptionalBool()).isFalse();
    assertThat(TestProto3Optional.getDefaultInstance().hasOptionalString()).isFalse();
    assertThat(TestProto3Optional.getDefaultInstance().hasOptionalBytes()).isFalse();

    TestProto3Optional.Builder builder = TestProto3Optional.newBuilder().setOptionalInt32(0);
    assertThat(builder.hasOptionalInt32()).isTrue();
    assertThat(builder.build().hasOptionalInt32()).isTrue();

    TestProto3Optional.Builder otherBuilder = TestProto3Optional.newBuilder().setOptionalInt32(1);
    otherBuilder.mergeFrom(builder.build());
    assertThat(otherBuilder.hasOptionalInt32()).isTrue();
    assertThat(otherBuilder.getOptionalInt32()).isEqualTo(0);

    TestProto3Optional.Builder builder3 =
        TestProto3Optional.newBuilder().setOptionalNestedEnumValue(5);
    assertThat(builder3.hasOptionalNestedEnum()).isTrue();

    TestProto3Optional.Builder builder4 =
        TestProto3Optional.newBuilder().setOptionalNestedEnum(TestProto3Optional.NestedEnum.FOO);
    assertThat(builder4.hasOptionalNestedEnum()).isTrue();

    TestProto3Optional proto = TestProto3Optional.parseFrom(builder.build().toByteArray());
    assertThat(proto.hasOptionalInt32()).isTrue();
    assertThat(proto.toBuilder().hasOptionalInt32()).isTrue();
  }

  private static void assertProto3OptionalReflection(String name) throws Exception {
    FieldDescriptor fieldDescriptor = TestProto3Optional.getDescriptor().findFieldByName(name);
    OneofDescriptor oneofDescriptor = fieldDescriptor.getContainingOneof();
    assertThat(fieldDescriptor.getContainingOneof()).isNotNull();
    assertThat(fieldDescriptor.hasOptionalKeyword()).isTrue();
    assertThat(fieldDescriptor.hasPresence()).isTrue();

    assertThat(TestProto3Optional.getDefaultInstance().hasOneof(oneofDescriptor)).isFalse();
    assertThat(TestProto3Optional.getDefaultInstance().getOneofFieldDescriptor(oneofDescriptor))
        .isNull();

    TestProto3Optional.Builder builder = TestProto3Optional.newBuilder();
    builder.setField(fieldDescriptor, fieldDescriptor.getDefaultValue());
    assertThat(builder.hasField(fieldDescriptor)).isTrue();
    assertThat(builder.getField(fieldDescriptor)).isEqualTo(fieldDescriptor.getDefaultValue());
    assertThat(builder.build().hasField(fieldDescriptor)).isTrue();
    assertThat(builder.build().getField(fieldDescriptor))
        .isEqualTo(fieldDescriptor.getDefaultValue());
    assertThat(builder.hasOneof(oneofDescriptor)).isTrue();
    assertThat(builder.getOneofFieldDescriptor(oneofDescriptor)).isEqualTo(fieldDescriptor);
    assertThat(builder.build().hasOneof(oneofDescriptor)).isTrue();
    assertThat(builder.build().getOneofFieldDescriptor(oneofDescriptor)).isEqualTo(fieldDescriptor);

    TestProto3Optional.Builder otherBuilder = TestProto3Optional.newBuilder();
    otherBuilder.mergeFrom(builder.build());
    assertThat(otherBuilder.hasField(fieldDescriptor)).isTrue();
    assertThat(otherBuilder.getField(fieldDescriptor)).isEqualTo(fieldDescriptor.getDefaultValue());

    TestProto3Optional proto = TestProto3Optional.parseFrom(builder.build().toByteArray());
    assertThat(proto.hasField(fieldDescriptor)).isTrue();
    assertThat(proto.toBuilder().hasField(fieldDescriptor)).isTrue();

    DynamicMessage.Builder dynamicBuilder =
        DynamicMessage.newBuilder(TestProto3Optional.getDescriptor());
    dynamicBuilder.setField(fieldDescriptor, fieldDescriptor.getDefaultValue());
    assertThat(dynamicBuilder.hasField(fieldDescriptor)).isTrue();
    assertThat(dynamicBuilder.getField(fieldDescriptor))
        .isEqualTo(fieldDescriptor.getDefaultValue());
    assertThat(dynamicBuilder.build().hasField(fieldDescriptor)).isTrue();
    assertThat(dynamicBuilder.build().getField(fieldDescriptor))
        .isEqualTo(fieldDescriptor.getDefaultValue());
    assertThat(dynamicBuilder.hasOneof(oneofDescriptor)).isTrue();
    assertThat(dynamicBuilder.getOneofFieldDescriptor(oneofDescriptor)).isEqualTo(fieldDescriptor);
    assertThat(dynamicBuilder.build().hasOneof(oneofDescriptor)).isTrue();
    assertThat(dynamicBuilder.build().getOneofFieldDescriptor(oneofDescriptor))
        .isEqualTo(fieldDescriptor);

    DynamicMessage.Builder otherDynamicBuilder =
        DynamicMessage.newBuilder(TestProto3Optional.getDescriptor());
    otherDynamicBuilder.mergeFrom(dynamicBuilder.build());
    assertThat(otherDynamicBuilder.hasField(fieldDescriptor)).isTrue();
    assertThat(otherDynamicBuilder.getField(fieldDescriptor))
        .isEqualTo(fieldDescriptor.getDefaultValue());

    DynamicMessage dynamicProto =
        DynamicMessage.parseFrom(TestProto3Optional.getDescriptor(), builder.build().toByteArray());
    assertThat(dynamicProto.hasField(fieldDescriptor)).isTrue();
    assertThat(dynamicProto.toBuilder().hasField(fieldDescriptor)).isTrue();
  }

  @Test
  public void testProto3Optional_reflection() throws Exception {
    assertProto3OptionalReflection("optional_int32");
    assertProto3OptionalReflection("optional_int64");
    assertProto3OptionalReflection("optional_uint32");
    assertProto3OptionalReflection("optional_uint64");
    assertProto3OptionalReflection("optional_sint32");
    assertProto3OptionalReflection("optional_sint64");
    assertProto3OptionalReflection("optional_fixed32");
    assertProto3OptionalReflection("optional_fixed64");
    assertProto3OptionalReflection("optional_float");
    assertProto3OptionalReflection("optional_double");
    assertProto3OptionalReflection("optional_bool");
    assertProto3OptionalReflection("optional_string");
    assertProto3OptionalReflection("optional_bytes");
  }

  @Test
  public void testOneofEquals() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestAllTypes message1 = builder.build();
    // Set message2's oneof_uint32 field to default value. The two
    // messages should be different when check with oneof case.
    builder.setOneofUint32(0);
    TestAllTypes message2 = builder.build();
    assertThat(message1.equals(message2)).isFalse();
  }

  @Test
  public void testLazyField() throws Exception {
    // Test default constructed message.
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestAllTypes message = builder.build();
    assertThat(message.hasOptionalLazyMessage()).isFalse();
    assertThat(message.getSerializedSize()).isEqualTo(0);
    assertThat(message.toByteString()).isEqualTo(ByteString.EMPTY);

    // Set default instance to the field.
    builder.setOptionalLazyMessage(TestAllTypes.NestedMessage.getDefaultInstance());
    message = builder.build();
    assertThat(message.hasOptionalLazyMessage()).isTrue();
    assertThat(message.getSerializedSize()).isEqualTo(2);

    // Test parse zero-length from wire sets the presence.
    TestAllTypes parsed = TestAllTypes.parseFrom(message.toByteString());
    assertThat(parsed.hasOptionalLazyMessage()).isTrue();
    assertThat(parsed.getOptionalLazyMessage()).isEqualTo(message.getOptionalLazyMessage());
  }

  @Test
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
    assertThat(message.getSerializedSize()).isEqualTo(0);

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
    assertThat(message.getOptionalInt32()).isEqualTo(1);
    assertThat(message.getOptionalString()).isEqualTo("x");
    assertThat(message.getOptionalBytes()).isEqualTo(ByteString.copyFromUtf8("y"));
    assertThat(message.getOptionalNestedEnum()).isEqualTo(TestAllTypes.NestedEnum.BAR);

    // equals()/hashCode() should produce the same results.
    TestAllTypes empty = TestAllTypes.getDefaultInstance();
    message = builder.build();
    assertThat(empty).isEqualTo(message);
    assertThat(message).isEqualTo(empty);
    assertThat(message.hashCode()).isEqualTo(empty.hashCode());
  }

  @Test
  public void testFieldPresenceByReflection() {
    Descriptor descriptor = TestAllTypes.getDescriptor();
    FieldDescriptor optionalInt32Field = descriptor.findFieldByName("optional_int32");
    FieldDescriptor optionalStringField = descriptor.findFieldByName("optional_string");
    FieldDescriptor optionalBytesField = descriptor.findFieldByName("optional_bytes");
    FieldDescriptor optionalNestedEnumField = descriptor.findFieldByName("optional_nested_enum");

    // Field not present.
    TestAllTypes message = TestAllTypes.getDefaultInstance();
    assertThat(message.hasField(optionalInt32Field)).isFalse();
    assertThat(message.hasField(optionalStringField)).isFalse();
    assertThat(message.hasField(optionalBytesField)).isFalse();
    assertThat(message.hasField(optionalNestedEnumField)).isFalse();
    assertThat(message.getAllFields()).isEmpty();

    // Field set to default value is seen as not present.
    message =
        TestAllTypes.newBuilder()
            .setOptionalInt32(0)
            .setOptionalString("")
            .setOptionalBytes(ByteString.EMPTY)
            .setOptionalNestedEnum(TestAllTypes.NestedEnum.FOO)
            .build();
    assertThat(message.hasField(optionalInt32Field)).isFalse();
    assertThat(message.hasField(optionalStringField)).isFalse();
    assertThat(message.hasField(optionalBytesField)).isFalse();
    assertThat(message.hasField(optionalNestedEnumField)).isFalse();
    assertThat(message.getAllFields()).isEmpty();

    // Field set to non-default value is seen as present.
    message =
        TestAllTypes.newBuilder()
            .setOptionalInt32(1)
            .setOptionalString("x")
            .setOptionalBytes(ByteString.copyFromUtf8("y"))
            .setOptionalNestedEnum(TestAllTypes.NestedEnum.BAR)
            .build();
    assertThat(message.hasField(optionalInt32Field)).isTrue();
    assertThat(message.hasField(optionalStringField)).isTrue();
    assertThat(message.hasField(optionalBytesField)).isTrue();
    assertThat(message.hasField(optionalNestedEnumField)).isTrue();
    assertThat(message.getAllFields()).hasSize(4);
  }

  @Test
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
    assertThat(message.hasField(optionalInt32Field)).isFalse();
    assertThat(message.hasField(optionalStringField)).isFalse();
    assertThat(message.hasField(optionalBytesField)).isFalse();
    assertThat(message.hasField(optionalNestedEnumField)).isFalse();
    assertThat(message.getAllFields()).isEmpty();

    // Field set to non-default value is seen as present.
    message =
        defaultInstance
            .newBuilderForType()
            .setField(optionalInt32Field, 1)
            .setField(optionalStringField, "x")
            .setField(optionalBytesField, ByteString.copyFromUtf8("y"))
            .setField(optionalNestedEnumField, nonDefaultEnumValueDescriptor)
            .build();
    assertThat(message.hasField(optionalInt32Field)).isTrue();
    assertThat(message.hasField(optionalStringField)).isTrue();
    assertThat(message.hasField(optionalBytesField)).isTrue();
    assertThat(message.hasField(optionalNestedEnumField)).isTrue();
    assertThat(message.getAllFields()).hasSize(4);

    // Field set to default value is seen as not present.
    message =
        message
            .toBuilder()
            .setField(optionalInt32Field, 0)
            .setField(optionalStringField, "")
            .setField(optionalBytesField, ByteString.EMPTY)
            .setField(optionalNestedEnumField, defaultEnumValueDescriptor)
            .build();
    assertThat(message.hasField(optionalInt32Field)).isFalse();
    assertThat(message.hasField(optionalStringField)).isFalse();
    assertThat(message.hasField(optionalBytesField)).isFalse();
    assertThat(message.hasField(optionalNestedEnumField)).isFalse();
    assertThat(message.getAllFields()).isEmpty();
  }

  @Test
  public void testMessageField() {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    assertThat(builder.hasOptionalNestedMessage()).isFalse();
    assertThat(builder.build().hasOptionalNestedMessage()).isFalse();

    TestAllTypes.NestedMessage.Builder nestedBuilder = builder.getOptionalNestedMessageBuilder();
    assertThat(builder.hasOptionalNestedMessage()).isTrue();
    assertThat(builder.build().hasOptionalNestedMessage()).isTrue();

    nestedBuilder.setValue(1);
    assertThat(builder.build().getOptionalNestedMessage().getValue()).isEqualTo(1);

    builder.clearOptionalNestedMessage();
    assertThat(builder.hasOptionalNestedMessage()).isFalse();
    assertThat(builder.build().hasOptionalNestedMessage()).isFalse();

    // Unlike non-message fields, if we set a message field to its default value (i.e.,
    // default instance), the field should be seen as present.
    builder.setOptionalNestedMessage(TestAllTypes.NestedMessage.getDefaultInstance());
    assertThat(builder.hasOptionalNestedMessage()).isTrue();
    assertThat(builder.build().hasOptionalNestedMessage()).isTrue();
  }

  @Test
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
    assertThat(message.getOptionalInt32()).isEqualTo(1234);
    assertThat(message.getOptionalString()).isEqualTo("hello");
    // Fields not set will have the default value.
    assertThat(message.getOptionalBytes()).isEqualTo(ByteString.EMPTY);
    assertThat(message.getOptionalNestedEnum()).isEqualTo(TestAllTypes.NestedEnum.FOO);
    // The message field is set despite that it's set with a default instance.
    assertThat(message.hasOptionalNestedMessage()).isTrue();
    assertThat(message.getOptionalNestedMessage().getValue()).isEqualTo(0);
    // The oneof field set to its default value is also present.
    assertThat(message.getOneofFieldCase()).isEqualTo(TestAllTypes.OneofFieldCase.ONEOF_INT32);
  }

  // Regression test for b/16173397
  // Make sure we haven't screwed up the code generation for repeated fields.
  @Test
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
    assertThat(optionalOnlyMessage.getOptionalInt32()).isEqualTo(1234);
    assertThat(optionalOnlyMessage.getOptionalString()).isEqualTo("hello");
    assertThat(optionalOnlyMessage.hasOptionalNestedMessage()).isTrue();
    assertThat(optionalOnlyMessage.getOptionalNestedMessage().getValue()).isEqualTo(0);

    TestRepeatedFieldsOnly repeatedOnlyMessage = TestRepeatedFieldsOnly.parseFrom(data);
    assertThat(repeatedOnlyMessage.getRepeatedInt32Count()).isEqualTo(1);
    assertThat(repeatedOnlyMessage.getRepeatedInt32(0)).isEqualTo(4321);
    assertThat(repeatedOnlyMessage.getRepeatedStringCount()).isEqualTo(1);
    assertThat(repeatedOnlyMessage.getRepeatedString(0)).isEqualTo("world");
    assertThat(repeatedOnlyMessage.getRepeatedNestedMessageCount()).isEqualTo(1);
    assertThat(repeatedOnlyMessage.getRepeatedNestedMessage(0).getValue()).isEqualTo(0);
  }

  @Test
  public void testIsInitialized() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();

    // Test optional proto2 message fields.
    UnittestProto.TestRequired.Builder proto2Builder = builder.getOptionalProto2MessageBuilder();
    assertThat(builder.isInitialized()).isFalse();
    assertThat(builder.buildPartial().isInitialized()).isFalse();

    proto2Builder.setA(1).setB(2).setC(3);
    assertThat(builder.isInitialized()).isTrue();
    assertThat(builder.buildPartial().isInitialized()).isTrue();

    // Test oneof proto2 message fields.
    proto2Builder = builder.getOneofProto2MessageBuilder();
    assertThat(builder.isInitialized()).isFalse();
    assertThat(builder.buildPartial().isInitialized()).isFalse();

    proto2Builder.setA(1).setB(2).setC(3);
    assertThat(builder.isInitialized()).isTrue();
    assertThat(builder.buildPartial().isInitialized()).isTrue();

    // Test repeated proto2 message fields.
    proto2Builder = builder.addRepeatedProto2MessageBuilder();
    assertThat(builder.isInitialized()).isFalse();
    assertThat(builder.buildPartial().isInitialized()).isFalse();

    proto2Builder.setA(1).setB(2).setC(3);
    assertThat(builder.isInitialized()).isTrue();
    assertThat(builder.buildPartial().isInitialized()).isTrue();
  }
}
