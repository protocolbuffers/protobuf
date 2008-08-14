// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.protobuf;

import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestEmptyMessage;
import protobuf_unittest.UnittestProto.
  TestEmptyMessageWithExtensions;

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
}
