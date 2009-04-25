// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestEmptyMessage;
import protobuf_unittest.UnittestProto.TestEmptyMessageWithExtensions;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.Map;

/**
 * Tests related to unknown field handling.
 *
 * @author kenton@google.com (Kenton Varda)
 */
public class UnknownFieldSetTest extends TestCase {
  public void setUp() throws Exception {
    descriptor = TestAllTypes.getDescriptor();
    allFields = TestUtil.getAllSet();
    allFieldsData = allFields.toByteString();
    emptyMessage = TestEmptyMessage.parseFrom(allFieldsData);
    unknownFields = emptyMessage.getUnknownFields();
  }

  UnknownFieldSet.Field getField(String name) {
    Descriptors.FieldDescriptor field = descriptor.findFieldByName(name);
    assertNotNull(field);
    return unknownFields.getField(field.getNumber());
  }

  // Constructs a protocol buffer which contains fields with all the same
  // numbers as allFieldsData except that each field is some other wire
  // type.
  ByteString getBizarroData() throws Exception {
    UnknownFieldSet.Builder bizarroFields = UnknownFieldSet.newBuilder();

    UnknownFieldSet.Field varintField =
      UnknownFieldSet.Field.newBuilder().addVarint(1).build();
    UnknownFieldSet.Field fixed32Field =
      UnknownFieldSet.Field.newBuilder().addFixed32(1).build();

    for (Map.Entry<Integer, UnknownFieldSet.Field> entry :
         unknownFields.asMap().entrySet()) {
      if (entry.getValue().getVarintList().isEmpty()) {
        // Original field is not a varint, so use a varint.
        bizarroFields.addField(entry.getKey(), varintField);
      } else {
        // Original field *is* a varint, so use something else.
        bizarroFields.addField(entry.getKey(), fixed32Field);
      }
    }

    return bizarroFields.build().toByteString();
  }

  Descriptors.Descriptor descriptor;
  TestAllTypes allFields;
  ByteString allFieldsData;

  // An empty message that has been parsed from allFieldsData.  So, it has
  // unknown fields of every type.
  TestEmptyMessage emptyMessage;
  UnknownFieldSet unknownFields;

  // =================================================================

  public void testVarint() throws Exception {
    UnknownFieldSet.Field field = getField("optional_int32");
    assertEquals(1, field.getVarintList().size());
    assertEquals(allFields.getOptionalInt32(),
                 (long) field.getVarintList().get(0));
  }

  public void testFixed32() throws Exception {
    UnknownFieldSet.Field field = getField("optional_fixed32");
    assertEquals(1, field.getFixed32List().size());
    assertEquals(allFields.getOptionalFixed32(),
                 (int) field.getFixed32List().get(0));
  }

  public void testFixed64() throws Exception {
    UnknownFieldSet.Field field = getField("optional_fixed64");
    assertEquals(1, field.getFixed64List().size());
    assertEquals(allFields.getOptionalFixed64(),
                 (long) field.getFixed64List().get(0));
  }

  public void testLengthDelimited() throws Exception {
    UnknownFieldSet.Field field = getField("optional_bytes");
    assertEquals(1, field.getLengthDelimitedList().size());
    assertEquals(allFields.getOptionalBytes(),
                 field.getLengthDelimitedList().get(0));
  }

  public void testGroup() throws Exception {
    Descriptors.FieldDescriptor nestedFieldDescriptor =
      TestAllTypes.OptionalGroup.getDescriptor().findFieldByName("a");
    assertNotNull(nestedFieldDescriptor);

    UnknownFieldSet.Field field = getField("optionalgroup");
    assertEquals(1, field.getGroupList().size());

    UnknownFieldSet group = field.getGroupList().get(0);
    assertEquals(1, group.asMap().size());
    assertTrue(group.hasField(nestedFieldDescriptor.getNumber()));

    UnknownFieldSet.Field nestedField =
      group.getField(nestedFieldDescriptor.getNumber());
    assertEquals(1, nestedField.getVarintList().size());
    assertEquals(allFields.getOptionalGroup().getA(),
                 (long) nestedField.getVarintList().get(0));
  }

  public void testSerialize() throws Exception {
    // Check that serializing the UnknownFieldSet produces the original data
    // again.
    ByteString data = emptyMessage.toByteString();
    assertEquals(allFieldsData, data);
  }

  public void testCopyFrom() throws Exception {
    TestEmptyMessage message =
      TestEmptyMessage.newBuilder().mergeFrom(emptyMessage).build();

    assertEquals(emptyMessage.toString(), message.toString());
  }

  public void testMergeFrom() throws Exception {
    TestEmptyMessage source =
      TestEmptyMessage.newBuilder()
        .setUnknownFields(
          UnknownFieldSet.newBuilder()
            .addField(2,
              UnknownFieldSet.Field.newBuilder()
                .addVarint(2).build())
            .addField(3,
              UnknownFieldSet.Field.newBuilder()
                .addVarint(4).build())
            .build())
        .build();
    TestEmptyMessage destination =
      TestEmptyMessage.newBuilder()
        .setUnknownFields(
          UnknownFieldSet.newBuilder()
            .addField(1,
              UnknownFieldSet.Field.newBuilder()
                .addVarint(1).build())
            .addField(3,
              UnknownFieldSet.Field.newBuilder()
                .addVarint(3).build())
            .build())
        .mergeFrom(source)
        .build();

    assertEquals(
      "1: 1\n" +
      "2: 2\n" +
      "3: 3\n" +
      "3: 4\n",
      destination.toString());
  }

  public void testClear() throws Exception {
    UnknownFieldSet fields =
      UnknownFieldSet.newBuilder().mergeFrom(unknownFields).clear().build();
    assertTrue(fields.asMap().isEmpty());
  }

  public void testClearMessage() throws Exception {
    TestEmptyMessage message =
      TestEmptyMessage.newBuilder().mergeFrom(emptyMessage).clear().build();
    assertEquals(0, message.getSerializedSize());
  }

  public void testParseKnownAndUnknown() throws Exception {
    // Test mixing known and unknown fields when parsing.

    UnknownFieldSet fields =
      UnknownFieldSet.newBuilder(unknownFields)
        .addField(123456,
          UnknownFieldSet.Field.newBuilder().addVarint(654321).build())
        .build();

    ByteString data = fields.toByteString();
    TestAllTypes destination = TestAllTypes.parseFrom(data);

    TestUtil.assertAllFieldsSet(destination);
    assertEquals(1, destination.getUnknownFields().asMap().size());

    UnknownFieldSet.Field field =
      destination.getUnknownFields().getField(123456);
    assertEquals(1, field.getVarintList().size());
    assertEquals(654321, (long) field.getVarintList().get(0));
  }

  public void testWrongTypeTreatedAsUnknown() throws Exception {
    // Test that fields of the wrong wire type are treated like unknown fields
    // when parsing.

    ByteString bizarroData = getBizarroData();
    TestAllTypes allTypesMessage = TestAllTypes.parseFrom(bizarroData);
    TestEmptyMessage emptyMessage = TestEmptyMessage.parseFrom(bizarroData);

    // All fields should have been interpreted as unknown, so the debug strings
    // should be the same.
    assertEquals(emptyMessage.toString(), allTypesMessage.toString());
  }

  public void testUnknownExtensions() throws Exception {
    // Make sure fields are properly parsed to the UnknownFieldSet even when
    // they are declared as extension numbers.

    TestEmptyMessageWithExtensions message =
      TestEmptyMessageWithExtensions.parseFrom(allFieldsData);

    assertEquals(unknownFields.asMap().size(),
                 message.getUnknownFields().asMap().size());
    assertEquals(allFieldsData, message.toByteString());
  }

  public void testWrongExtensionTypeTreatedAsUnknown() throws Exception {
    // Test that fields of the wrong wire type are treated like unknown fields
    // when parsing extensions.

    ByteString bizarroData = getBizarroData();
    TestAllExtensions allExtensionsMessage =
      TestAllExtensions.parseFrom(bizarroData);
    TestEmptyMessage emptyMessage = TestEmptyMessage.parseFrom(bizarroData);

    // All fields should have been interpreted as unknown, so the debug strings
    // should be the same.
    assertEquals(emptyMessage.toString(),
                 allExtensionsMessage.toString());
  }

  public void testParseUnknownEnumValue() throws Exception {
    Descriptors.FieldDescriptor singularField =
      TestAllTypes.getDescriptor().findFieldByName("optional_nested_enum");
    Descriptors.FieldDescriptor repeatedField =
      TestAllTypes.getDescriptor().findFieldByName("repeated_nested_enum");
    assertNotNull(singularField);
    assertNotNull(repeatedField);

    ByteString data =
      UnknownFieldSet.newBuilder()
        .addField(singularField.getNumber(),
          UnknownFieldSet.Field.newBuilder()
            .addVarint(TestAllTypes.NestedEnum.BAR.getNumber())
            .addVarint(5)   // not valid
            .build())
        .addField(repeatedField.getNumber(),
          UnknownFieldSet.Field.newBuilder()
            .addVarint(TestAllTypes.NestedEnum.FOO.getNumber())
            .addVarint(4)   // not valid
            .addVarint(TestAllTypes.NestedEnum.BAZ.getNumber())
            .addVarint(6)   // not valid
            .build())
        .build()
        .toByteString();

    {
      TestAllTypes message = TestAllTypes.parseFrom(data);
      assertEquals(TestAllTypes.NestedEnum.BAR,
                   message.getOptionalNestedEnum());
      assertEquals(
        Arrays.asList(TestAllTypes.NestedEnum.FOO, TestAllTypes.NestedEnum.BAZ),
        message.getRepeatedNestedEnumList());
      assertEquals(Arrays.asList(5L),
                   message.getUnknownFields()
                          .getField(singularField.getNumber())
                          .getVarintList());
      assertEquals(Arrays.asList(4L, 6L),
                   message.getUnknownFields()
                          .getField(repeatedField.getNumber())
                          .getVarintList());
    }

    {
      TestAllExtensions message =
        TestAllExtensions.parseFrom(data, TestUtil.getExtensionRegistry());
      assertEquals(TestAllTypes.NestedEnum.BAR,
        message.getExtension(UnittestProto.optionalNestedEnumExtension));
      assertEquals(
        Arrays.asList(TestAllTypes.NestedEnum.FOO, TestAllTypes.NestedEnum.BAZ),
        message.getExtension(UnittestProto.repeatedNestedEnumExtension));
      assertEquals(Arrays.asList(5L),
                   message.getUnknownFields()
                          .getField(singularField.getNumber())
                          .getVarintList());
      assertEquals(Arrays.asList(4L, 6L),
                   message.getUnknownFields()
                          .getField(repeatedField.getNumber())
                          .getVarintList());
    }
  }

  public void testLargeVarint() throws Exception {
    ByteString data =
      UnknownFieldSet.newBuilder()
        .addField(1,
          UnknownFieldSet.Field.newBuilder()
            .addVarint(0x7FFFFFFFFFFFFFFFL)
            .build())
        .build()
        .toByteString();
    UnknownFieldSet parsed = UnknownFieldSet.parseFrom(data);
    UnknownFieldSet.Field field = parsed.getField(1);
    assertEquals(1, field.getVarintList().size());
    assertEquals(0x7FFFFFFFFFFFFFFFL, (long)field.getVarintList().get(0));
  }

  public void testEqualsAndHashCode() {
    UnknownFieldSet.Field fixed32Field =
        UnknownFieldSet.Field.newBuilder()
            .addFixed32(1)
            .build();
    UnknownFieldSet.Field fixed64Field =
        UnknownFieldSet.Field.newBuilder()
            .addFixed64(1)
            .build();
    UnknownFieldSet.Field varIntField =
        UnknownFieldSet.Field.newBuilder()
            .addVarint(1)
            .build();
    UnknownFieldSet.Field lengthDelimitedField =
        UnknownFieldSet.Field.newBuilder()
            .addLengthDelimited(ByteString.EMPTY)
            .build();
    UnknownFieldSet.Field groupField =
        UnknownFieldSet.Field.newBuilder()
            .addGroup(unknownFields)
            .build();

    UnknownFieldSet a =
        UnknownFieldSet.newBuilder()
            .addField(1, fixed32Field)
            .build();
    UnknownFieldSet b =
        UnknownFieldSet.newBuilder()
            .addField(1, fixed64Field)
            .build();
    UnknownFieldSet c =
        UnknownFieldSet.newBuilder()
            .addField(1, varIntField)
            .build();
    UnknownFieldSet d =
        UnknownFieldSet.newBuilder()
            .addField(1, lengthDelimitedField)
            .build();
    UnknownFieldSet e =
        UnknownFieldSet.newBuilder()
            .addField(1, groupField)
            .build();

    checkEqualsIsConsistent(a);
    checkEqualsIsConsistent(b);
    checkEqualsIsConsistent(c);
    checkEqualsIsConsistent(d);
    checkEqualsIsConsistent(e);

    checkNotEqual(a, b);
    checkNotEqual(a, c);
    checkNotEqual(a, d);
    checkNotEqual(a, e);
    checkNotEqual(b, c);
    checkNotEqual(b, d);
    checkNotEqual(b, e);
    checkNotEqual(c, d);
    checkNotEqual(c, e);
    checkNotEqual(d, e);
  }

  /**
   * Asserts that the given field sets are not equal and have different
   * hash codes.
   *
   * @warning It's valid for non-equal objects to have the same hash code, so
   *   this test is stricter than it needs to be. However, this should happen
   *   relatively rarely.
   */
  private void checkNotEqual(UnknownFieldSet s1, UnknownFieldSet s2) {
    String equalsError = String.format("%s should not be equal to %s", s1, s2);
    assertFalse(equalsError, s1.equals(s2));
    assertFalse(equalsError, s2.equals(s1));

    assertFalse(
        String.format("%s should have a different hash code from %s", s1, s2),
        s1.hashCode() == s2.hashCode());
  }

  /**
   * Asserts that the given field sets are equal and have identical hash codes.
   */
  private void checkEqualsIsConsistent(UnknownFieldSet set) {
    // Object should be equal to itself.
    assertEquals(set, set);

    // Object should be equal to a copy of itself.
    UnknownFieldSet copy = UnknownFieldSet.newBuilder(set).build();
    assertEquals(set, copy);
    assertEquals(copy, set);
    assertEquals(set.hashCode(), copy.hashCode());
  }
}
