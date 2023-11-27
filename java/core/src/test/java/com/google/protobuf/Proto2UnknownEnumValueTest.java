// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.Descriptors.FieldDescriptor;
import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for proto2 that treats unknown enum values as unknown fields. */
@RunWith(JUnit4.class)
public class Proto2UnknownEnumValueTest {
  FieldDescriptor singularField =
      TestAllTypes.getDescriptor().findFieldByName("optional_nested_enum");
  FieldDescriptor repeatedField =
      TestAllTypes.getDescriptor().findFieldByName("repeated_nested_enum");
  byte[] payload = buildPayloadWithUnknownEnumValues();

  private byte[] buildPayloadWithUnknownEnumValues() {
    // Builds a payload with unknown enum values.
    UnknownFieldSet.Builder builder = UnknownFieldSet.newBuilder();
    builder.addField(
        singularField.getNumber(),
        UnknownFieldSet.Field.newBuilder()
            .addVarint(TestAllTypes.NestedEnum.BAR.getNumber())
            .addVarint(1901 /* unknown enum value */)
            .build());
    builder.addField(
        repeatedField.getNumber(),
        UnknownFieldSet.Field.newBuilder()
            .addVarint(TestAllTypes.NestedEnum.FOO.getNumber())
            .addVarint(1902 /* unknown enum value */)
            .addVarint(TestAllTypes.NestedEnum.BAZ.getNumber())
            .addVarint(1903 /* unknown enum value */)
            .build());
    return builder.build().toByteArray();
  }

  @Test
  public void testUnknownEnumValues() throws Exception {
    TestAllTypes message = TestAllTypes.parseFrom(payload);

    // Known enum values should be preserved.
    assertThat(message.getOptionalNestedEnum()).isEqualTo(TestAllTypes.NestedEnum.BAR);
    assertThat(message.getRepeatedNestedEnumList().size()).isEqualTo(2);
    assertThat(message.getRepeatedNestedEnum(0)).isEqualTo(TestAllTypes.NestedEnum.FOO);
    assertThat(message.getRepeatedNestedEnum(1)).isEqualTo(TestAllTypes.NestedEnum.BAZ);

    // Unknown enum values should be found in UnknownFieldSet.
    UnknownFieldSet unknown = message.getUnknownFields();
    assertThat(unknown.getField(singularField.getNumber()).getVarintList().get(0).longValue())
        .isEqualTo(1901);
    assertThat(unknown.getField(repeatedField.getNumber()).getVarintList().get(0).longValue())
        .isEqualTo(1902);
    assertThat(unknown.getField(repeatedField.getNumber()).getVarintList().get(1).longValue())
        .isEqualTo(1903);
  }

  @Test
  public void testExtensionUnknownEnumValues() throws Exception {
    ExtensionRegistry registry = ExtensionRegistry.newInstance();
    UnittestProto.registerAllExtensions(registry);
    TestAllExtensions message = TestAllExtensions.parseFrom(payload, registry);

    assertThat(message.getExtension(UnittestProto.optionalNestedEnumExtension))
        .isEqualTo(TestAllTypes.NestedEnum.BAR);
    assertThat(message.getExtension(UnittestProto.repeatedNestedEnumExtension).size()).isEqualTo(2);
    assertThat(message.getExtension(UnittestProto.repeatedNestedEnumExtension, 0))
        .isEqualTo(TestAllTypes.NestedEnum.FOO);
    assertThat(message.getExtension(UnittestProto.repeatedNestedEnumExtension, 1))
        .isEqualTo(TestAllTypes.NestedEnum.BAZ);

    // Unknown enum values should be found in UnknownFieldSet.
    UnknownFieldSet unknown = message.getUnknownFields();
    assertThat(unknown.getField(singularField.getNumber()).getVarintList().get(0).longValue())
        .isEqualTo(1901);
    assertThat(unknown.getField(repeatedField.getNumber()).getVarintList().get(0).longValue())
        .isEqualTo(1902);
    assertThat(unknown.getField(repeatedField.getNumber()).getVarintList().get(1).longValue())
        .isEqualTo(1903);
  }
}
