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

import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.ForeignEnum;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestEmptyMessage;
import protobuf_unittest.UnittestProto.TestEmptyMessageWithExtensions;
import protobuf_unittest.UnittestProto.TestPackedExtensions;
import protobuf_unittest.UnittestProto.TestPackedTypes;
import proto3_unittest.UnittestProto3;
import java.util.List;
import java.util.Map;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests related to unknown field handling. */
@RunWith(JUnit4.class)
public class UnknownFieldSetTest {

  @Before
  public void setUp() throws Exception {
    descriptor = TestAllTypes.getDescriptor();
    allFields = TestUtil.getAllSet();
    allFieldsData = allFields.toByteString();
    emptyMessage = TestEmptyMessage.parseFrom(allFieldsData);
    unknownFields = emptyMessage.getUnknownFields();
  }

  private UnknownFieldSet.Field getField(String name) {
    Descriptors.FieldDescriptor field = descriptor.findFieldByName(name);
    assertThat(field).isNotNull();
    return unknownFields.getField(field.getNumber());
  }

  // Constructs a protocol buffer which contains fields with all the same
  // numbers as allFieldsData except that each field is some other wire
  // type.
  ByteString getBizarroData() throws Exception {
    UnknownFieldSet.Builder bizarroFields = UnknownFieldSet.newBuilder();

    UnknownFieldSet.Field varintField = UnknownFieldSet.Field.newBuilder().addVarint(1).build();
    UnknownFieldSet.Field fixed32Field = UnknownFieldSet.Field.newBuilder().addFixed32(1).build();

    for (Map.Entry<Integer, UnknownFieldSet.Field> entry : unknownFields.asMap().entrySet()) {
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

  @Test
  public void testFieldBuildersAreReusable() {
    UnknownFieldSet.Field.Builder fieldBuilder = UnknownFieldSet.Field.newBuilder();
    fieldBuilder.addFixed32(10);
    UnknownFieldSet.Field first = fieldBuilder.build();
    UnknownFieldSet.Field second = fieldBuilder.build();
    fieldBuilder.addFixed32(11);
    UnknownFieldSet.Field third = fieldBuilder.build();

    assertThat(first).isEqualTo(second);
    assertThat(first).isNotEqualTo(third);
  }

  @Test
  public void testClone() {
    UnknownFieldSet.Builder unknownSetBuilder = UnknownFieldSet.newBuilder();
    UnknownFieldSet.Field.Builder fieldBuilder = UnknownFieldSet.Field.newBuilder();
    fieldBuilder.addFixed32(10);
    unknownSetBuilder.addField(8, fieldBuilder.build());
    // necessary to call clone twice to expose the bug
    UnknownFieldSet.Builder clone1 = unknownSetBuilder.clone();
    UnknownFieldSet.Builder clone2 = unknownSetBuilder.clone(); // failure is a NullPointerException
    assertThat(clone1).isNotSameInstanceAs(clone2);
  }

  @Test
  public void testClone_lengthDelimited() {
    UnknownFieldSet.Builder destUnknownFieldSet =
        UnknownFieldSet.newBuilder()
            .addField(997, UnknownFieldSet.Field.newBuilder().addVarint(99).build())
            .addField(
                999,
                UnknownFieldSet.Field.newBuilder()
                    .addLengthDelimited(ByteString.copyFromUtf8("some data"))
                    .addLengthDelimited(ByteString.copyFromUtf8("some more data"))
                    .build());
    UnknownFieldSet clone = destUnknownFieldSet.clone().build();
    assertThat(clone.getField(997)).isNotNull();
    UnknownFieldSet.Field field999 = clone.getField(999);
    List<ByteString> lengthDelimited = field999.getLengthDelimitedList();
    assertThat(lengthDelimited.get(0).toStringUtf8()).isEqualTo("some data");
    assertThat(lengthDelimited.get(1).toStringUtf8()).isEqualTo("some more data");

    UnknownFieldSet clone2 = destUnknownFieldSet.clone().build();
    assertThat(clone2.getField(997)).isNotNull();
    UnknownFieldSet.Field secondField = clone2.getField(999);
    List<ByteString> lengthDelimited2 = secondField.getLengthDelimitedList();
    assertThat(lengthDelimited2.get(0).toStringUtf8()).isEqualTo("some data");
    assertThat(lengthDelimited2.get(1).toStringUtf8()).isEqualTo("some more data");
  }

  @Test
  public void testReuse() {
    UnknownFieldSet.Builder builder =
        UnknownFieldSet.newBuilder()
            .addField(997, UnknownFieldSet.Field.newBuilder().addVarint(99).build())
            .addField(
                999,
                UnknownFieldSet.Field.newBuilder()
                    .addLengthDelimited(ByteString.copyFromUtf8("some data"))
                    .addLengthDelimited(ByteString.copyFromUtf8("some more data"))
                    .build());

    UnknownFieldSet fieldSet1 = builder.build();
    UnknownFieldSet fieldSet2 = builder.build();
    builder.addField(1000, UnknownFieldSet.Field.newBuilder().addVarint(-90).build());
    UnknownFieldSet fieldSet3 = builder.build();

    assertThat(fieldSet1).isEqualTo(fieldSet2);
    assertThat(fieldSet1).isNotEqualTo(fieldSet3);
  }

  @Test
  @SuppressWarnings("ModifiedButNotUsed")
  public void testAddField_zero() {
    UnknownFieldSet.Field field = getField("optional_int32");
    try {
      UnknownFieldSet.newBuilder().addField(0, field);
      Assert.fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("0 is not a valid field number.");
    }
  }

  @Test
  @SuppressWarnings("ModifiedButNotUsed")
  public void testAddField_negative() {
    UnknownFieldSet.Field field = getField("optional_int32");
    try {
      UnknownFieldSet.newBuilder().addField(-2, field);
      Assert.fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("-2 is not a valid field number.");
    }
  }

  @Test
  @SuppressWarnings("ModifiedButNotUsed")
  public void testClearField_negative() {
    try {
      UnknownFieldSet.newBuilder().clearField(-28);
      Assert.fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("-28 is not a valid field number.");
    }
  }

  @Test
  @SuppressWarnings("ModifiedButNotUsed")
  public void testMergeField_negative() {
    UnknownFieldSet.Field field = getField("optional_int32");
    try {
      UnknownFieldSet.newBuilder().mergeField(-2, field);
      Assert.fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("-2 is not a valid field number.");
    }
  }

  @Test
  @SuppressWarnings("ModifiedButNotUsed")
  public void testMergeVarintField_negative() {
    try {
      UnknownFieldSet.newBuilder().mergeVarintField(-2, 78);
      Assert.fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("-2 is not a valid field number.");
    }
  }

  @Test
  @SuppressWarnings("ModifiedButNotUsed")
  public void testHasField_negative() {
    assertThat(UnknownFieldSet.newBuilder().hasField(-2)).isFalse();
  }

  @Test
  @SuppressWarnings("ModifiedButNotUsed")
  public void testMergeLengthDelimitedField_negative() {
    ByteString byteString = ByteString.copyFromUtf8("some data");
    try {
      UnknownFieldSet.newBuilder().mergeLengthDelimitedField(-2, byteString);
      Assert.fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("-2 is not a valid field number.");
    }
  }

  @Test
  public void testAddField() {
    UnknownFieldSet.Field field = getField("optional_int32");
    UnknownFieldSet fieldSet = UnknownFieldSet.newBuilder().addField(1, field).build();
    assertThat(fieldSet.getField(1)).isEqualTo(field);
  }

  @Test
  public void testAddField_withReplacement() {
    UnknownFieldSet.Field first = UnknownFieldSet.Field.newBuilder().addFixed32(56).build();
    UnknownFieldSet.Field second = UnknownFieldSet.Field.newBuilder().addFixed32(25).build();
    UnknownFieldSet fieldSet = UnknownFieldSet.newBuilder()
        .addField(1, first)
        .addField(1, second)
        .build();
    List<Integer> list = fieldSet.getField(1).getFixed32List();
    assertThat(list).hasSize(1);
    assertThat(list.get(0)).isEqualTo(25);
  }

  @Test
  public void testVarint() throws Exception {
    UnknownFieldSet.Field field = getField("optional_int32");
    assertThat(field.getVarintList()).hasSize(1);
    assertThat((long) field.getVarintList().get(0)).isEqualTo(allFields.getOptionalInt32());
  }

  @Test
  public void testFixed32() throws Exception {
    UnknownFieldSet.Field field = getField("optional_fixed32");
    assertThat(field.getFixed32List()).hasSize(1);
    assertThat((int) field.getFixed32List().get(0)).isEqualTo(allFields.getOptionalFixed32());
  }

  @Test
  public void testFixed64() throws Exception {
    UnknownFieldSet.Field field = getField("optional_fixed64");
    assertThat(field.getFixed64List()).hasSize(1);
    assertThat((long) field.getFixed64List().get(0)).isEqualTo(allFields.getOptionalFixed64());
  }

  @Test
  public void testLengthDelimited() throws Exception {
    UnknownFieldSet.Field field = getField("optional_bytes");
    assertThat(field.getLengthDelimitedList()).hasSize(1);
    assertThat(field.getLengthDelimitedList().get(0)).isEqualTo(allFields.getOptionalBytes());
  }

  @Test
  public void testGroup() throws Exception {
    Descriptors.FieldDescriptor nestedFieldDescriptor =
        TestAllTypes.OptionalGroup.getDescriptor().findFieldByName("a");
    assertThat(nestedFieldDescriptor).isNotNull();

    UnknownFieldSet.Field field = getField("optionalgroup");
    assertThat(field.getGroupList()).hasSize(1);

    UnknownFieldSet group = field.getGroupList().get(0);
    assertThat(group.asMap()).hasSize(1);
    assertThat(group.hasField(nestedFieldDescriptor.getNumber())).isTrue();

    UnknownFieldSet.Field nestedField = group.getField(nestedFieldDescriptor.getNumber());
    assertThat(nestedField.getVarintList()).hasSize(1);
    assertThat((long) nestedField.getVarintList().get(0))
        .isEqualTo(allFields.getOptionalGroup().getA());
  }

  @Test
  public void testSerialize() throws Exception {
    // Check that serializing the UnknownFieldSet produces the original data
    // again.
    ByteString data = emptyMessage.toByteString();
    assertThat(data).isEqualTo(allFieldsData);
  }

  @Test
  public void testCopyFrom() throws Exception {
    TestEmptyMessage message = TestEmptyMessage.newBuilder().mergeFrom(emptyMessage).build();

    assertThat(message.toString()).isEqualTo(emptyMessage.toString());
  }

  @Test
  public void testMergeFrom() throws Exception {
    TestEmptyMessage source =
        TestEmptyMessage.newBuilder()
            .setUnknownFields(
                UnknownFieldSet.newBuilder()
                    .addField(2, UnknownFieldSet.Field.newBuilder().addVarint(2).build())
                    .addField(3, UnknownFieldSet.Field.newBuilder().addVarint(4).build())
                    .build())
            .build();
    TestEmptyMessage destination =
        TestEmptyMessage.newBuilder()
            .setUnknownFields(
                UnknownFieldSet.newBuilder()
                    .addField(1, UnknownFieldSet.Field.newBuilder().addVarint(1).build())
                    .addField(3, UnknownFieldSet.Field.newBuilder().addVarint(3).build())
                    .build())
            .mergeFrom(source)
            .build();

    assertThat(destination.toString()).isEqualTo("1: 1\n2: 2\n3: 3\n3: 4\n");
  }

  @Test
  public void testAsMap() throws Exception {
    UnknownFieldSet.Builder builder = UnknownFieldSet.newBuilder().mergeFrom(unknownFields);
    Map<Integer, UnknownFieldSet.Field> mapFromBuilder = builder.asMap();
    assertThat(mapFromBuilder).isNotEmpty();
    UnknownFieldSet fields = builder.build();
    Map<Integer, UnknownFieldSet.Field> mapFromFieldSet = fields.asMap();
    assertThat(mapFromFieldSet).containsExactlyEntriesIn(mapFromBuilder);
  }

  @Test
  public void testClear() throws Exception {
    UnknownFieldSet fields = UnknownFieldSet.newBuilder().mergeFrom(unknownFields).clear().build();
    assertThat(fields.asMap()).isEmpty();
  }

  @Test
  public void testClearMessage() throws Exception {
    TestEmptyMessage message =
        TestEmptyMessage.newBuilder().mergeFrom(emptyMessage).clear().build();
    assertThat(message.getSerializedSize()).isEqualTo(0);
  }

  @Test
  public void testClearField() throws Exception {
    int fieldNumber = unknownFields.asMap().keySet().iterator().next();
    UnknownFieldSet fields =
        UnknownFieldSet.newBuilder().mergeFrom(unknownFields).clearField(fieldNumber).build();
    assertThat(fields.hasField(fieldNumber)).isFalse();
  }

  @Test
  public void testParseKnownAndUnknown() throws Exception {
    // Test mixing known and unknown fields when parsing.

    UnknownFieldSet fields =
        UnknownFieldSet.newBuilder(unknownFields)
            .addField(123456, UnknownFieldSet.Field.newBuilder().addVarint(654321).build())
            .build();

    ByteString data = fields.toByteString();
    TestAllTypes destination = TestAllTypes.parseFrom(data);

    TestUtil.assertAllFieldsSet(destination);
    assertThat(destination.getUnknownFields().asMap()).hasSize(1);

    UnknownFieldSet.Field field = destination.getUnknownFields().getField(123456);
    assertThat(field.getVarintList()).hasSize(1);
    assertThat((long) field.getVarintList().get(0)).isEqualTo(654321);
  }

  @Test
  public void testWrongTypeTreatedAsUnknown() throws Exception {
    // Test that fields of the wrong wire type are treated like unknown fields
    // when parsing.

    ByteString bizarroData = getBizarroData();
    TestAllTypes allTypesMessage = TestAllTypes.parseFrom(bizarroData);
    TestEmptyMessage emptyMessage = TestEmptyMessage.parseFrom(bizarroData);

    // All fields should have been interpreted as unknown, so the debug strings
    // should be the same.
    assertThat(emptyMessage.toString()).isEqualTo(allTypesMessage.toString());
  }

  @Test
  public void testUnknownExtensions() throws Exception {
    // Make sure fields are properly parsed to the UnknownFieldSet even when
    // they are declared as extension numbers.

    TestEmptyMessageWithExtensions message =
        TestEmptyMessageWithExtensions.parseFrom(allFieldsData);

    assertThat(unknownFields.asMap()).hasSize(message.getUnknownFields().asMap().size());
    assertThat(allFieldsData).isEqualTo(message.toByteString());
  }

  @Test
  public void testWrongExtensionTypeTreatedAsUnknown() throws Exception {
    // Test that fields of the wrong wire type are treated like unknown fields
    // when parsing extensions.

    ByteString bizarroData = getBizarroData();
    TestAllExtensions allExtensionsMessage = TestAllExtensions.parseFrom(bizarroData);
    TestEmptyMessage emptyMessage = TestEmptyMessage.parseFrom(bizarroData);

    // All fields should have been interpreted as unknown, so the debug strings
    // should be the same.
    assertThat(emptyMessage.toString()).isEqualTo(allExtensionsMessage.toString());
  }

  @Test
  public void testParseUnknownEnumValue() throws Exception {
    Descriptors.FieldDescriptor singularField =
        TestAllTypes.getDescriptor().findFieldByName("optional_nested_enum");
    Descriptors.FieldDescriptor repeatedField =
        TestAllTypes.getDescriptor().findFieldByName("repeated_nested_enum");
    assertThat(singularField).isNotNull();
    assertThat(repeatedField).isNotNull();

    ByteString data =
        UnknownFieldSet.newBuilder()
            .addField(
                singularField.getNumber(),
                UnknownFieldSet.Field.newBuilder()
                    .addVarint(TestAllTypes.NestedEnum.BAR.getNumber())
                    .addVarint(5) // not valid
                    .build())
            .addField(
                repeatedField.getNumber(),
                UnknownFieldSet.Field.newBuilder()
                    .addVarint(TestAllTypes.NestedEnum.FOO.getNumber())
                    .addVarint(4) // not valid
                    .addVarint(TestAllTypes.NestedEnum.BAZ.getNumber())
                    .addVarint(6) // not valid
                    .build())
            .build()
            .toByteString();

    {
      TestAllTypes message = TestAllTypes.parseFrom(data);
      assertThat(message.getOptionalNestedEnum()).isEqualTo(TestAllTypes.NestedEnum.BAR);
      assertThat(message.getRepeatedNestedEnumList())
          .containsExactly(TestAllTypes.NestedEnum.FOO, TestAllTypes.NestedEnum.BAZ)
          .inOrder();
      assertThat(message.getUnknownFields().getField(singularField.getNumber()).getVarintList())
          .containsExactly(5L);
      assertThat(message.getUnknownFields().getField(repeatedField.getNumber()).getVarintList())
          .containsExactly(4L, 6L)
          .inOrder();
    }

    {
      TestAllExtensions message =
          TestAllExtensions.parseFrom(data, TestUtil.getExtensionRegistry());
      assertThat(message.getExtension(UnittestProto.optionalNestedEnumExtension))
          .isEqualTo(TestAllTypes.NestedEnum.BAR);
      assertThat(message.getExtension(UnittestProto.repeatedNestedEnumExtension))
          .containsExactly(TestAllTypes.NestedEnum.FOO, TestAllTypes.NestedEnum.BAZ)
          .inOrder();
      assertThat(message.getUnknownFields().getField(singularField.getNumber()).getVarintList())
          .containsExactly(5L);
      assertThat(message.getUnknownFields().getField(repeatedField.getNumber()).getVarintList())
          .containsExactly(4L, 6L)
          .inOrder();
    }
  }

  @Test
  public void testLargeVarint() throws Exception {
    ByteString data =
        UnknownFieldSet.newBuilder()
            .addField(1, UnknownFieldSet.Field.newBuilder().addVarint(0x7FFFFFFFFFFFFFFFL).build())
            .build()
            .toByteString();
    UnknownFieldSet parsed = UnknownFieldSet.parseFrom(data);
    UnknownFieldSet.Field field = parsed.getField(1);
    assertThat(field.getVarintList()).hasSize(1);
    assertThat((long) field.getVarintList().get(0)).isEqualTo(0x7FFFFFFFFFFFFFFFL);
  }

  @Test
  public void testEqualsAndHashCode() {
    UnknownFieldSet.Field fixed32Field = UnknownFieldSet.Field.newBuilder().addFixed32(1).build();
    UnknownFieldSet.Field fixed64Field = UnknownFieldSet.Field.newBuilder().addFixed64(1).build();
    UnknownFieldSet.Field varIntField = UnknownFieldSet.Field.newBuilder().addVarint(1).build();
    UnknownFieldSet.Field lengthDelimitedField =
        UnknownFieldSet.Field.newBuilder().addLengthDelimited(ByteString.EMPTY).build();
    UnknownFieldSet.Field groupField =
        UnknownFieldSet.Field.newBuilder().addGroup(unknownFields).build();

    UnknownFieldSet a = UnknownFieldSet.newBuilder().addField(1, fixed32Field).build();
    UnknownFieldSet b = UnknownFieldSet.newBuilder().addField(1, fixed64Field).build();
    UnknownFieldSet c = UnknownFieldSet.newBuilder().addField(1, varIntField).build();
    UnknownFieldSet d = UnknownFieldSet.newBuilder().addField(1, lengthDelimitedField).build();
    UnknownFieldSet e = UnknownFieldSet.newBuilder().addField(1, groupField).build();

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
   * Asserts that the given field sets are not equal and have different hash codes.
   *
   * <p><b>Note:</b> It's valid for non-equal objects to have the same hash code, so this test is
   * stricter than it needs to be. However, this should happen relatively rarely.
   */
  private void checkNotEqual(UnknownFieldSet s1, UnknownFieldSet s2) {
    String equalsError = String.format("%s should not be equal to %s", s1, s2);
    assertWithMessage(equalsError).that(s1).isNotEqualTo(s2);
    assertWithMessage(equalsError).that(s2).isNotEqualTo(s1);

    assertWithMessage("%s should have a different hash code from %s", s1, s2)
        .that(s1.hashCode())
        .isNotEqualTo(s2.hashCode());
  }

  /** Asserts that the given field sets are equal and have identical hash codes. */
  private void checkEqualsIsConsistent(UnknownFieldSet set) {
    // Object should be equal to itself.
    assertThat(set.equals(set)).isTrue();

    // Object should be equal to a copy of itself.
    UnknownFieldSet copy = UnknownFieldSet.newBuilder(set).build();
    assertThat(copy).isEqualTo(set);
    assertThat(set).isEqualTo(copy);
    assertThat(set.hashCode()).isEqualTo(copy.hashCode());
  }

  // =================================================================

  @Test
  public void testProto3RoundTrip() throws Exception {
    ByteString data = getBizarroData();

    UnittestProto3.TestEmptyMessage message =
        UnittestProto3.TestEmptyMessage.parseFrom(data, ExtensionRegistryLite.getEmptyRegistry());
    assertThat(message.toByteString()).isEqualTo(data);

    message = UnittestProto3.TestEmptyMessage.newBuilder().mergeFrom(message).build();
    assertThat(message.toByteString()).isEqualTo(data);

    assertThat(data)
        .isEqualTo(
            UnittestProto3.TestMessageWithDummy.parseFrom(
                    data, ExtensionRegistryLite.getEmptyRegistry())
                .toBuilder()
                // force copy-on-write
                .setDummy(true)
                .build()
                .toBuilder()
                .clearDummy()
                .build()
                .toByteString());
  }
}
