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

import com.google.protobuf.Descriptors.FieldDescriptor;
import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import junit.framework.TestCase;

/** Unit tests for proto2 that treats unknown enum values as unknown fields. */
public class Proto2UnknownEnumValueTest extends TestCase {
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

  public void testUnknownEnumValues() throws Exception {
    TestAllTypes message = TestAllTypes.parseFrom(payload);

    // Known enum values should be preserved.
    assertEquals(TestAllTypes.NestedEnum.BAR, message.getOptionalNestedEnum());
    assertEquals(2, message.getRepeatedNestedEnumList().size());
    assertEquals(TestAllTypes.NestedEnum.FOO, message.getRepeatedNestedEnum(0));
    assertEquals(TestAllTypes.NestedEnum.BAZ, message.getRepeatedNestedEnum(1));

    // Unknown enum values should be found in UnknownFieldSet.
    UnknownFieldSet unknown = message.getUnknownFields();
    assertEquals(
        1901, unknown.getField(singularField.getNumber()).getVarintList().get(0).longValue());
    assertEquals(
        1902, unknown.getField(repeatedField.getNumber()).getVarintList().get(0).longValue());
    assertEquals(
        1903, unknown.getField(repeatedField.getNumber()).getVarintList().get(1).longValue());
  }

  public void testExtensionUnknownEnumValues() throws Exception {
    ExtensionRegistry registry = ExtensionRegistry.newInstance();
    UnittestProto.registerAllExtensions(registry);
    TestAllExtensions message = TestAllExtensions.parseFrom(payload, registry);

    assertEquals(
        TestAllTypes.NestedEnum.BAR,
        message.getExtension(UnittestProto.optionalNestedEnumExtension));
    assertEquals(2, message.getExtension(UnittestProto.repeatedNestedEnumExtension).size());
    assertEquals(
        TestAllTypes.NestedEnum.FOO,
        message.getExtension(UnittestProto.repeatedNestedEnumExtension, 0));
    assertEquals(
        TestAllTypes.NestedEnum.BAZ,
        message.getExtension(UnittestProto.repeatedNestedEnumExtension, 1));

    // Unknown enum values should be found in UnknownFieldSet.
    UnknownFieldSet unknown = message.getUnknownFields();
    assertEquals(
        1901, unknown.getField(singularField.getNumber()).getVarintList().get(0).longValue());
    assertEquals(
        1902, unknown.getField(repeatedField.getNumber()).getVarintList().get(0).longValue());
    assertEquals(
        1903, unknown.getField(repeatedField.getNumber()).getVarintList().get(1).longValue());
  }
}
