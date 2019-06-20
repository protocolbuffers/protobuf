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
import com.google.protobuf.Proto2UnknownEnumValuesTestProto.Proto2EnumMessage;
import com.google.protobuf.Proto2UnknownEnumValuesTestProto.Proto2EnumMessageWithEnumSubset;
import com.google.protobuf.Proto2UnknownEnumValuesTestProto.Proto2TestEnum;
import com.google.protobuf.Proto2UnknownEnumValuesTestProto.Proto2TestEnumSubset;
import com.google.protobuf.TextFormat.ParseException;
import junit.framework.TestCase;

/**
 * Unit tests for protos that keep unknown enum values rather than discard them as unknown fields.
 */
public class UnknownEnumValueTest extends TestCase {

  public void testUnknownEnumValues() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.setOptionalNestedEnumValue(4321);
    builder.addRepeatedNestedEnumValue(5432);
    builder.addPackedNestedEnumValue(6543);
    TestAllTypes message = builder.build();
    assertEquals(4321, message.getOptionalNestedEnumValue());
    assertEquals(5432, message.getRepeatedNestedEnumValue(0));
    assertEquals(5432, message.getRepeatedNestedEnumValueList().get(0).intValue());
    assertEquals(6543, message.getPackedNestedEnumValue(0));
    // Returns UNRECOGNIZED if an enum type is requested.
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, message.getOptionalNestedEnum());
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, message.getRepeatedNestedEnum(0));
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, message.getRepeatedNestedEnumList().get(0));
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, message.getPackedNestedEnum(0));

    // Test serialization and parsing.
    ByteString data = message.toByteString();
    message = TestAllTypes.parseFrom(data);
    assertEquals(4321, message.getOptionalNestedEnumValue());
    assertEquals(5432, message.getRepeatedNestedEnumValue(0));
    assertEquals(5432, message.getRepeatedNestedEnumValueList().get(0).intValue());
    assertEquals(6543, message.getPackedNestedEnumValue(0));
    // Returns UNRECOGNIZED if an enum type is requested.
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, message.getOptionalNestedEnum());
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, message.getRepeatedNestedEnum(0));
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, message.getRepeatedNestedEnumList().get(0));
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, message.getPackedNestedEnum(0));

    // Test toBuilder().
    builder = message.toBuilder();
    assertEquals(4321, builder.getOptionalNestedEnumValue());
    assertEquals(5432, builder.getRepeatedNestedEnumValue(0));
    assertEquals(5432, builder.getRepeatedNestedEnumValueList().get(0).intValue());
    assertEquals(6543, builder.getPackedNestedEnumValue(0));
    // Returns UNRECOGNIZED if an enum type is requested.
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, builder.getOptionalNestedEnum());
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, builder.getRepeatedNestedEnum(0));
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, builder.getRepeatedNestedEnumList().get(0));
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, builder.getPackedNestedEnum(0));

    // Test mergeFrom().
    builder = TestAllTypes.newBuilder().mergeFrom(message);
    assertEquals(4321, builder.getOptionalNestedEnumValue());
    assertEquals(5432, builder.getRepeatedNestedEnumValue(0));
    assertEquals(5432, builder.getRepeatedNestedEnumValueList().get(0).intValue());
    assertEquals(6543, builder.getPackedNestedEnumValue(0));
    // Returns UNRECOGNIZED if an enum type is requested.
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, builder.getOptionalNestedEnum());
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, builder.getRepeatedNestedEnum(0));
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, builder.getRepeatedNestedEnumList().get(0));
    assertEquals(TestAllTypes.NestedEnum.UNRECOGNIZED, builder.getPackedNestedEnum(0));

    // Test equals() and hashCode()
    TestAllTypes sameMessage = builder.build();
    assertEquals(message, sameMessage);
    assertEquals(message.hashCode(), sameMessage.hashCode());

    // Getting the numeric value of UNRECOGNIZED will throw an exception.
    try {
      TestAllTypes.NestedEnum.UNRECOGNIZED.getNumber();
      fail("Exception is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    // Setting an enum field to an UNRECOGNIZED value will throw an exception.
    try {
      builder.setOptionalNestedEnum(builder.getOptionalNestedEnum());
      fail("Exception is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }
    try {
      builder.addRepeatedNestedEnum(builder.getOptionalNestedEnum());
      fail("Exception is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }
  }

  public void testUnknownEnumValueInReflectionApi() throws Exception {
    Descriptor descriptor = TestAllTypes.getDescriptor();
    FieldDescriptor optionalNestedEnumField = descriptor.findFieldByName("optional_nested_enum");
    FieldDescriptor repeatedNestedEnumField = descriptor.findFieldByName("repeated_nested_enum");
    FieldDescriptor packedNestedEnumField = descriptor.findFieldByName("packed_nested_enum");
    EnumDescriptor enumType = TestAllTypes.NestedEnum.getDescriptor();

    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.setField(optionalNestedEnumField, enumType.findValueByNumberCreatingIfUnknown(4321));
    builder.addRepeatedField(
        repeatedNestedEnumField, enumType.findValueByNumberCreatingIfUnknown(5432));
    builder.addRepeatedField(
        packedNestedEnumField, enumType.findValueByNumberCreatingIfUnknown(6543));
    TestAllTypes message = builder.build();

    // Getters will return unknown enum values as EnumValueDescriptor.
    EnumValueDescriptor unknown4321 =
        (EnumValueDescriptor) message.getField(optionalNestedEnumField);
    EnumValueDescriptor unknown5432 =
        (EnumValueDescriptor) message.getRepeatedField(repeatedNestedEnumField, 0);
    EnumValueDescriptor unknown6543 =
        (EnumValueDescriptor) message.getRepeatedField(packedNestedEnumField, 0);
    assertEquals(4321, unknown4321.getNumber());
    assertEquals(5432, unknown5432.getNumber());
    assertEquals(6543, unknown6543.getNumber());

    // Unknown EnumValueDescriptor will map to UNRECOGNIZED.
    assertEquals(
        TestAllTypes.NestedEnum.UNRECOGNIZED, TestAllTypes.NestedEnum.valueOf(unknown4321));
    assertEquals(
        TestAllTypes.NestedEnum.UNRECOGNIZED, TestAllTypes.NestedEnum.valueOf(unknown5432));
    assertEquals(
        TestAllTypes.NestedEnum.UNRECOGNIZED, TestAllTypes.NestedEnum.valueOf(unknown6543));

    // Setters also accept unknown EnumValueDescriptor.
    builder.setField(optionalNestedEnumField, unknown6543);
    builder.setRepeatedField(repeatedNestedEnumField, 0, unknown4321);
    builder.setRepeatedField(packedNestedEnumField, 0, unknown5432);
    message = builder.build();
    // Like other descriptors, unknown EnumValueDescriptor can be compared by
    // object identity.
    assertSame(message.getField(optionalNestedEnumField), unknown6543);
    assertSame(message.getRepeatedField(repeatedNestedEnumField, 0), unknown4321);
    assertSame(message.getRepeatedField(packedNestedEnumField, 0), unknown5432);
  }

  public void testUnknownEnumValueWithDynamicMessage() throws Exception {
    Descriptor descriptor = TestAllTypes.getDescriptor();
    FieldDescriptor optionalNestedEnumField = descriptor.findFieldByName("optional_nested_enum");
    FieldDescriptor repeatedNestedEnumField = descriptor.findFieldByName("repeated_nested_enum");
    FieldDescriptor packedNestedEnumField = descriptor.findFieldByName("packed_nested_enum");
    EnumDescriptor enumType = TestAllTypes.NestedEnum.getDescriptor();

    Message dynamicMessageDefaultInstance = DynamicMessage.getDefaultInstance(descriptor);

    Message.Builder builder = dynamicMessageDefaultInstance.newBuilderForType();
    builder.setField(optionalNestedEnumField, enumType.findValueByNumberCreatingIfUnknown(4321));
    builder.addRepeatedField(
        repeatedNestedEnumField, enumType.findValueByNumberCreatingIfUnknown(5432));
    builder.addRepeatedField(
        packedNestedEnumField, enumType.findValueByNumberCreatingIfUnknown(6543));
    Message message = builder.build();
    assertEquals(
        4321, ((EnumValueDescriptor) message.getField(optionalNestedEnumField)).getNumber());
    assertEquals(
        5432,
        ((EnumValueDescriptor) message.getRepeatedField(repeatedNestedEnumField, 0)).getNumber());
    assertEquals(
        6543,
        ((EnumValueDescriptor) message.getRepeatedField(packedNestedEnumField, 0)).getNumber());

    // Test reflection based serialization/parsing implementation.
    ByteString data = message.toByteString();
    message = dynamicMessageDefaultInstance.newBuilderForType().mergeFrom(data).build();
    assertEquals(
        4321, ((EnumValueDescriptor) message.getField(optionalNestedEnumField)).getNumber());
    assertEquals(
        5432,
        ((EnumValueDescriptor) message.getRepeatedField(repeatedNestedEnumField, 0)).getNumber());
    assertEquals(
        6543,
        ((EnumValueDescriptor) message.getRepeatedField(packedNestedEnumField, 0)).getNumber());

    // Test reflection based equals()/hashCode().
    builder = dynamicMessageDefaultInstance.newBuilderForType();
    builder.setField(optionalNestedEnumField, enumType.findValueByNumberCreatingIfUnknown(4321));
    builder.addRepeatedField(
        repeatedNestedEnumField, enumType.findValueByNumberCreatingIfUnknown(5432));
    builder.addRepeatedField(
        packedNestedEnumField, enumType.findValueByNumberCreatingIfUnknown(6543));
    Message sameMessage = builder.build();
    assertEquals(message, sameMessage);
    assertEquals(message.hashCode(), sameMessage.hashCode());
    builder.setField(optionalNestedEnumField, enumType.findValueByNumberCreatingIfUnknown(0));
    Message differentMessage = builder.build();
    assertFalse(message.equals(differentMessage));
  }

  public void testUnknownEnumValuesInTextFormat() {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.setOptionalNestedEnumValue(4321);
    builder.addRepeatedNestedEnumValue(5432);
    builder.addPackedNestedEnumValue(6543);
    TestAllTypes message = builder.build();

    // We can print a message with unknown enum values.
    String textData = TextFormat.printer().printToString(message);
    assertEquals(
        "optional_nested_enum: UNKNOWN_ENUM_VALUE_NestedEnum_4321\n"
            + "repeated_nested_enum: UNKNOWN_ENUM_VALUE_NestedEnum_5432\n"
            + "packed_nested_enum: UNKNOWN_ENUM_VALUE_NestedEnum_6543\n",
        textData);

    // Parsing unknown enum values will fail just like parsing other kinds of
    // unknown fields.
    try {
      TextFormat.merge(textData, builder);
      fail();
    } catch (ParseException e) {
      // expected.
    }
  }

  public void testUnknownEnumValuesInProto2() throws Exception {
    Proto2EnumMessage.Builder sourceMessage = Proto2EnumMessage.newBuilder();
    sourceMessage
        .addRepeatedPackedEnum(Proto2TestEnum.ZERO)
        .addRepeatedPackedEnum(Proto2TestEnum.TWO) // Unknown in parsed proto
        .addRepeatedPackedEnum(Proto2TestEnum.ONE);

    Proto2EnumMessageWithEnumSubset destMessage =
        Proto2EnumMessageWithEnumSubset.parseFrom(sourceMessage.build().toByteArray());

    // Known enum values should be preserved.
    assertEquals(2, destMessage.getRepeatedPackedEnumCount());
    assertEquals(Proto2TestEnumSubset.TESTENUM_SUBSET_ZERO, destMessage.getRepeatedPackedEnum(0));
    assertEquals(Proto2TestEnumSubset.TESTENUM_SUBSET_ONE, destMessage.getRepeatedPackedEnum(1));

    // Unknown enum values should be found in UnknownFieldSet.
    UnknownFieldSet unknown = destMessage.getUnknownFields();
    assertEquals(
        Proto2TestEnum.TWO_VALUE,
        unknown
            .getField(Proto2EnumMessageWithEnumSubset.REPEATED_PACKED_ENUM_FIELD_NUMBER)
            .getVarintList()
            .get(0)
            .longValue());
  }

  public void testUnknownEnumValuesInProto2WithDynamicMessage() throws Exception {
    Descriptor descriptor = Proto2EnumMessageWithEnumSubset.getDescriptor();
    FieldDescriptor repeatedPackedField = descriptor.findFieldByName("repeated_packed_enum");

    Proto2EnumMessage.Builder sourceMessage = Proto2EnumMessage.newBuilder();
    sourceMessage
        .addRepeatedPackedEnum(Proto2TestEnum.ZERO)
        .addRepeatedPackedEnum(Proto2TestEnum.TWO) // Unknown in parsed proto
        .addRepeatedPackedEnum(Proto2TestEnum.ONE);

    DynamicMessage message =
        DynamicMessage.parseFrom(
            Proto2EnumMessageWithEnumSubset.getDescriptor(), sourceMessage.build().toByteArray());

    // Known enum values should be preserved.
    assertEquals(2, message.getRepeatedFieldCount(repeatedPackedField));
    EnumValueDescriptor enumValue0 =
        (EnumValueDescriptor) message.getRepeatedField(repeatedPackedField, 0);
    EnumValueDescriptor enumValue1 =
        (EnumValueDescriptor) message.getRepeatedField(repeatedPackedField, 1);

    assertEquals(Proto2TestEnumSubset.TESTENUM_SUBSET_ZERO_VALUE, enumValue0.getNumber());
    assertEquals(Proto2TestEnumSubset.TESTENUM_SUBSET_ONE_VALUE, enumValue1.getNumber());

    // Unknown enum values should be found in UnknownFieldSet.
    UnknownFieldSet unknown = message.getUnknownFields();
    assertEquals(
        Proto2TestEnum.TWO_VALUE,
        unknown.getField(repeatedPackedField.getNumber()).getVarintList().get(0).longValue());
  }
}
