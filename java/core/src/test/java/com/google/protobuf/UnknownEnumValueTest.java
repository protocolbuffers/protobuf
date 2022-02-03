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
import static com.google.common.truth.Truth.assertWithMessage;

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
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Unit tests for protos that keep unknown enum values rather than discard them as unknown fields.
 */
@RunWith(JUnit4.class)
public class UnknownEnumValueTest {

  @Test
  public void testUnknownEnumValues() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.setOptionalNestedEnumValue(4321);
    builder.addRepeatedNestedEnumValue(5432);
    builder.addPackedNestedEnumValue(6543);
    TestAllTypes message = builder.build();
    assertThat(message.getOptionalNestedEnumValue()).isEqualTo(4321);
    assertThat(message.getRepeatedNestedEnumValue(0)).isEqualTo(5432);
    assertThat(message.getRepeatedNestedEnumValueList().get(0).intValue()).isEqualTo(5432);
    assertThat(message.getPackedNestedEnumValue(0)).isEqualTo(6543);
    // Returns UNRECOGNIZED if an enum type is requested.
    assertThat(message.getOptionalNestedEnum()).isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);
    assertThat(message.getRepeatedNestedEnum(0)).isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);
    assertThat(message.getRepeatedNestedEnumList().get(0))
        .isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);
    assertThat(message.getPackedNestedEnum(0)).isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);

    // Test serialization and parsing.
    ByteString data = message.toByteString();
    message = TestAllTypes.parseFrom(data);
    assertThat(message.getOptionalNestedEnumValue()).isEqualTo(4321);
    assertThat(message.getRepeatedNestedEnumValue(0)).isEqualTo(5432);
    assertThat(message.getRepeatedNestedEnumValueList().get(0).intValue()).isEqualTo(5432);
    assertThat(message.getPackedNestedEnumValue(0)).isEqualTo(6543);
    // Returns UNRECOGNIZED if an enum type is requested.
    assertThat(message.getOptionalNestedEnum()).isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);
    assertThat(message.getRepeatedNestedEnum(0)).isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);
    assertThat(message.getRepeatedNestedEnumList().get(0))
        .isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);
    assertThat(message.getPackedNestedEnum(0)).isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);

    // Test toBuilder().
    builder = message.toBuilder();
    assertThat(builder.getOptionalNestedEnumValue()).isEqualTo(4321);
    assertThat(builder.getRepeatedNestedEnumValue(0)).isEqualTo(5432);
    assertThat(builder.getRepeatedNestedEnumValueList().get(0).intValue()).isEqualTo(5432);
    assertThat(builder.getPackedNestedEnumValue(0)).isEqualTo(6543);
    // Returns UNRECOGNIZED if an enum type is requested.
    assertThat(builder.getOptionalNestedEnum()).isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);
    assertThat(builder.getRepeatedNestedEnum(0)).isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);
    assertThat(builder.getRepeatedNestedEnumList().get(0))
        .isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);
    assertThat(builder.getPackedNestedEnum(0)).isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);

    // Test mergeFrom().
    builder = TestAllTypes.newBuilder().mergeFrom(message);
    assertThat(builder.getOptionalNestedEnumValue()).isEqualTo(4321);
    assertThat(builder.getRepeatedNestedEnumValue(0)).isEqualTo(5432);
    assertThat(builder.getRepeatedNestedEnumValueList().get(0).intValue()).isEqualTo(5432);
    assertThat(builder.getPackedNestedEnumValue(0)).isEqualTo(6543);
    // Returns UNRECOGNIZED if an enum type is requested.
    assertThat(builder.getOptionalNestedEnum()).isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);
    assertThat(builder.getRepeatedNestedEnum(0)).isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);
    assertThat(builder.getRepeatedNestedEnumList().get(0))
        .isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);
    assertThat(builder.getPackedNestedEnum(0)).isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);

    // Test equals() and hashCode()
    TestAllTypes sameMessage = builder.build();
    assertThat(sameMessage).isEqualTo(message);
    assertThat(sameMessage.hashCode()).isEqualTo(message.hashCode());

    // Getting the numeric value of UNRECOGNIZED will throw an exception.
    try {
      TestAllTypes.NestedEnum.UNRECOGNIZED.getNumber();
      assertWithMessage("Exception is expected.").fail();
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    // Setting an enum field to an UNRECOGNIZED value will throw an exception.
    try {
      builder.setOptionalNestedEnum(builder.getOptionalNestedEnum());
      assertWithMessage("Exception is expected.").fail();
    } catch (IllegalArgumentException e) {
      // Expected.
    }
    try {
      builder.addRepeatedNestedEnum(builder.getOptionalNestedEnum());
      assertWithMessage("Exception is expected.").fail();
    } catch (IllegalArgumentException e) {
      // Expected.
    }
  }

  @Test
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
    assertThat(unknown4321.getNumber()).isEqualTo(4321);
    assertThat(unknown5432.getNumber()).isEqualTo(5432);
    assertThat(unknown6543.getNumber()).isEqualTo(6543);

    // Unknown EnumValueDescriptor will map to UNRECOGNIZED.
    assertThat(TestAllTypes.NestedEnum.valueOf(unknown4321))
        .isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);
    assertThat(TestAllTypes.NestedEnum.valueOf(unknown5432))
        .isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);
    assertThat(TestAllTypes.NestedEnum.valueOf(unknown6543))
        .isEqualTo(TestAllTypes.NestedEnum.UNRECOGNIZED);

    // Setters also accept unknown EnumValueDescriptor.
    builder.setField(optionalNestedEnumField, unknown6543);
    builder.setRepeatedField(repeatedNestedEnumField, 0, unknown4321);
    builder.setRepeatedField(packedNestedEnumField, 0, unknown5432);
    message = builder.build();
    // Like other descriptors, unknown EnumValueDescriptor can be compared by
    // object identity.
    assertThat(unknown6543).isSameInstanceAs(message.getField(optionalNestedEnumField));
    assertThat(unknown4321).isSameInstanceAs(message.getRepeatedField(repeatedNestedEnumField, 0));
    assertThat(unknown5432).isSameInstanceAs(message.getRepeatedField(packedNestedEnumField, 0));
  }

  @Test
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
    assertThat(((EnumValueDescriptor) message.getField(optionalNestedEnumField)).getNumber())
        .isEqualTo(4321);
    assertThat(
            ((EnumValueDescriptor) message.getRepeatedField(repeatedNestedEnumField, 0))
                .getNumber())
        .isEqualTo(5432);
    assertThat(
            ((EnumValueDescriptor) message.getRepeatedField(packedNestedEnumField, 0)).getNumber())
        .isEqualTo(6543);

    // Test reflection based serialization/parsing implementation.
    ByteString data = message.toByteString();
    message = dynamicMessageDefaultInstance.newBuilderForType().mergeFrom(data).build();
    assertThat(((EnumValueDescriptor) message.getField(optionalNestedEnumField)).getNumber())
        .isEqualTo(4321);
    assertThat(
            ((EnumValueDescriptor) message.getRepeatedField(repeatedNestedEnumField, 0))
                .getNumber())
        .isEqualTo(5432);
    assertThat(
            ((EnumValueDescriptor) message.getRepeatedField(packedNestedEnumField, 0)).getNumber())
        .isEqualTo(6543);

    // Test reflection based equals()/hashCode().
    builder = dynamicMessageDefaultInstance.newBuilderForType();
    builder.setField(optionalNestedEnumField, enumType.findValueByNumberCreatingIfUnknown(4321));
    builder.addRepeatedField(
        repeatedNestedEnumField, enumType.findValueByNumberCreatingIfUnknown(5432));
    builder.addRepeatedField(
        packedNestedEnumField, enumType.findValueByNumberCreatingIfUnknown(6543));
    Message sameMessage = builder.build();
    assertThat(sameMessage).isEqualTo(message);
    assertThat(sameMessage.hashCode()).isEqualTo(message.hashCode());
    builder.setField(optionalNestedEnumField, enumType.findValueByNumberCreatingIfUnknown(0));
    Message differentMessage = builder.build();
    assertThat(message.equals(differentMessage)).isFalse();
  }

  @Test
  public void testUnknownEnumValuesInTextFormat() {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.setOptionalNestedEnumValue(4321);
    builder.addRepeatedNestedEnumValue(5432);
    builder.addPackedNestedEnumValue(6543);
    TestAllTypes message = builder.build();

    // We can print a message with unknown enum values.
    String textData = TextFormat.printer().printToString(message);
    assertThat(textData)
        .isEqualTo(
            "optional_nested_enum: UNKNOWN_ENUM_VALUE_NestedEnum_4321\n"
                + "repeated_nested_enum: UNKNOWN_ENUM_VALUE_NestedEnum_5432\n"
                + "packed_nested_enum: UNKNOWN_ENUM_VALUE_NestedEnum_6543\n");

    // Parsing unknown enum values will fail just like parsing other kinds of
    // unknown fields.
    try {
      TextFormat.merge(textData, builder);
      assertWithMessage("Expected exception").fail();
    } catch (ParseException e) {
      // expected.
    }
  }

  @Test
  public void testUnknownEnumValuesInProto2() throws Exception {
    Proto2EnumMessage.Builder sourceMessage = Proto2EnumMessage.newBuilder();
    sourceMessage
        .addRepeatedPackedEnum(Proto2TestEnum.ZERO)
        .addRepeatedPackedEnum(Proto2TestEnum.TWO) // Unknown in parsed proto
        .addRepeatedPackedEnum(Proto2TestEnum.ONE);

    Proto2EnumMessageWithEnumSubset destMessage =
        Proto2EnumMessageWithEnumSubset.parseFrom(sourceMessage.build().toByteArray());

    // Known enum values should be preserved.
    assertThat(destMessage.getRepeatedPackedEnumCount()).isEqualTo(2);
    assertThat(destMessage.getRepeatedPackedEnum(0))
        .isEqualTo(Proto2TestEnumSubset.TESTENUM_SUBSET_ZERO);
    assertThat(destMessage.getRepeatedPackedEnum(1))
        .isEqualTo(Proto2TestEnumSubset.TESTENUM_SUBSET_ONE);

    // Unknown enum values should be found in UnknownFieldSet.
    UnknownFieldSet unknown = destMessage.getUnknownFields();
    assertThat(
            unknown
                .getField(Proto2EnumMessageWithEnumSubset.REPEATED_PACKED_ENUM_FIELD_NUMBER)
                .getVarintList()
                .get(0)
                .longValue())
        .isEqualTo(Proto2TestEnum.TWO_VALUE);
  }

  @Test
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
    assertThat(message.getRepeatedFieldCount(repeatedPackedField)).isEqualTo(2);
    EnumValueDescriptor enumValue0 =
        (EnumValueDescriptor) message.getRepeatedField(repeatedPackedField, 0);
    EnumValueDescriptor enumValue1 =
        (EnumValueDescriptor) message.getRepeatedField(repeatedPackedField, 1);

    assertThat(enumValue0.getNumber()).isEqualTo(Proto2TestEnumSubset.TESTENUM_SUBSET_ZERO_VALUE);
    assertThat(enumValue1.getNumber()).isEqualTo(Proto2TestEnumSubset.TESTENUM_SUBSET_ONE_VALUE);

    // Unknown enum values should be found in UnknownFieldSet.
    UnknownFieldSet unknown = message.getUnknownFields();
    assertThat(unknown.getField(repeatedPackedField.getNumber()).getVarintList().get(0).longValue())
        .isEqualTo(Proto2TestEnum.TWO_VALUE);
  }
}
