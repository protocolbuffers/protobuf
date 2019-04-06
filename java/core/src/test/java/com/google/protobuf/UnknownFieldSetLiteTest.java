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

import static junit.framework.TestCase.assertEquals;

import com.google.protobuf.UnittestLite.TestAllExtensionsLite;
import com.google.protobuf.UnittestLite.TestAllTypesLite;
import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.ForeignEnum;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestEmptyMessage;
import protobuf_unittest.UnittestProto.TestPackedExtensions;
import protobuf_unittest.UnittestProto.TestPackedTypes;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.Bar;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.Foo;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.Map;
import junit.framework.TestCase;

/**
 * Tests for {@link UnknownFieldSetLite}.
 *
 * @author dweis@google.com (Daniel Weis)
 */
public class UnknownFieldSetLiteTest extends TestCase {
  @Override
  public void setUp() throws Exception {
    allFields = TestUtil.getAllSet();
    allFieldsData = allFields.toByteString();
    emptyMessage = TestEmptyMessage.parseFrom(allFieldsData);
    unknownFields = emptyMessage.getUnknownFields();
  }

  TestAllTypes allFields;
  ByteString allFieldsData;

  // Constructs a protocol buffer which contains fields with all the same
  // numbers as allFieldsData except that each field is some other wire
  // type.
  private ByteString getBizarroData() throws Exception {
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

  // An empty message that has been parsed from allFieldsData.  So, it has
  // unknown fields of every type.
  TestEmptyMessage emptyMessage;
  UnknownFieldSet unknownFields;

  public void testDefaultInstance() {
    UnknownFieldSetLite unknownFields = UnknownFieldSetLite.getDefaultInstance();

    assertEquals(0, unknownFields.getSerializedSize());
    assertEquals(ByteString.EMPTY, toByteString(unknownFields));
  }

  public void testEmptyInstance() {
    UnknownFieldSetLite instance = UnknownFieldSetLite.newInstance();

    assertEquals(0, instance.getSerializedSize());
    assertEquals(ByteString.EMPTY, toByteString(instance));
    assertEquals(UnknownFieldSetLite.getDefaultInstance(), instance);
  }

  public void testMergeFieldFrom() throws IOException {
    Foo foo = Foo.newBuilder().setValue(2).build();

    CodedInputStream input = CodedInputStream.newInstance(foo.toByteArray());

    UnknownFieldSetLite instance = UnknownFieldSetLite.newInstance();
    instance.mergeFieldFrom(input.readTag(), input);

    assertEquals(foo.toByteString(), toByteString(instance));
  }

  public void testSerializedSize() throws IOException {
    Foo foo = Foo.newBuilder().setValue(2).build();

    CodedInputStream input = CodedInputStream.newInstance(foo.toByteArray());

    UnknownFieldSetLite instance = UnknownFieldSetLite.newInstance();
    instance.mergeFieldFrom(input.readTag(), input);

    assertEquals(foo.toByteString().size(), instance.getSerializedSize());
  }

  public void testHashCodeAfterDeserialization() throws IOException {
    Foo foo = Foo.newBuilder().setValue(2).build();

    Foo fooDeserialized = Foo.parseFrom(foo.toByteArray());

    assertEquals(fooDeserialized, foo);
    assertEquals(foo.hashCode(), fooDeserialized.hashCode());
  }

  public void testNewInstanceHashCode() {
    UnknownFieldSetLite emptyFieldSet = UnknownFieldSetLite.getDefaultInstance();
    UnknownFieldSetLite paddedFieldSet = UnknownFieldSetLite.newInstance();

    assertEquals(emptyFieldSet, paddedFieldSet);
    assertEquals(emptyFieldSet.hashCode(), paddedFieldSet.hashCode());
  }

  public void testMergeVarintField() throws IOException {
    UnknownFieldSetLite unknownFields = UnknownFieldSetLite.newInstance();
    unknownFields.mergeVarintField(10, 2);

    CodedInputStream input =
        CodedInputStream.newInstance(toByteString(unknownFields).toByteArray());

    int tag = input.readTag();
    assertEquals(10, WireFormat.getTagFieldNumber(tag));
    assertEquals(WireFormat.WIRETYPE_VARINT, WireFormat.getTagWireType(tag));
    assertEquals(2, input.readUInt64());
    assertTrue(input.isAtEnd());
  }

  public void testMergeVarintField_negative() throws IOException {
    UnknownFieldSetLite builder = UnknownFieldSetLite.newInstance();
    builder.mergeVarintField(10, -6);

    CodedInputStream input = CodedInputStream.newInstance(toByteString(builder).toByteArray());

    int tag = input.readTag();
    assertEquals(10, WireFormat.getTagFieldNumber(tag));
    assertEquals(WireFormat.WIRETYPE_VARINT, WireFormat.getTagWireType(tag));
    assertEquals(-6, input.readUInt64());
    assertTrue(input.isAtEnd());
  }

  public void testEqualsAndHashCode() {
    UnknownFieldSetLite unknownFields1 = UnknownFieldSetLite.newInstance();
    unknownFields1.mergeVarintField(10, 2);

    UnknownFieldSetLite unknownFields2 = UnknownFieldSetLite.newInstance();
    unknownFields2.mergeVarintField(10, 2);

    assertEquals(unknownFields1, unknownFields2);
    assertEquals(unknownFields1.hashCode(), unknownFields2.hashCode());
    assertFalse(unknownFields1.equals(UnknownFieldSetLite.getDefaultInstance()));
    assertFalse(unknownFields1.hashCode() == UnknownFieldSetLite.getDefaultInstance().hashCode());
  }

  public void testMutableCopyOf() throws IOException {
    UnknownFieldSetLite unknownFields = UnknownFieldSetLite.newInstance();
    unknownFields.mergeVarintField(10, 2);
    unknownFields = UnknownFieldSetLite.mutableCopyOf(unknownFields, unknownFields);
    unknownFields.checkMutable();

    CodedInputStream input =
        CodedInputStream.newInstance(toByteString(unknownFields).toByteArray());

    int tag = input.readTag();
    assertEquals(10, WireFormat.getTagFieldNumber(tag));
    assertEquals(WireFormat.WIRETYPE_VARINT, WireFormat.getTagWireType(tag));
    assertEquals(2, input.readUInt64());
    assertFalse(input.isAtEnd());
    input.readTag();
    assertEquals(10, WireFormat.getTagFieldNumber(tag));
    assertEquals(WireFormat.WIRETYPE_VARINT, WireFormat.getTagWireType(tag));
    assertEquals(2, input.readUInt64());
    assertTrue(input.isAtEnd());
  }

  public void testMutableCopyOf_empty() {
    UnknownFieldSetLite unknownFields =
        UnknownFieldSetLite.mutableCopyOf(
            UnknownFieldSetLite.getDefaultInstance(), UnknownFieldSetLite.getDefaultInstance());
    unknownFields.checkMutable();

    assertEquals(0, unknownFields.getSerializedSize());
    assertEquals(ByteString.EMPTY, toByteString(unknownFields));
  }

  public void testRoundTrips() throws InvalidProtocolBufferException {
    Foo foo =
        Foo.newBuilder()
            .setValue(1)
            .setExtension(Bar.fooExt, Bar.newBuilder().setName("name").build())
            .setExtension(LiteEqualsAndHash.varint, 22)
            .setExtension(LiteEqualsAndHash.fixed32, 44)
            .setExtension(LiteEqualsAndHash.fixed64, 66L)
            .setExtension(
                LiteEqualsAndHash.myGroup,
                LiteEqualsAndHash.MyGroup.newBuilder().setGroupValue("value").build())
            .build();

    Foo copy = Foo.parseFrom(foo.toByteArray());

    assertEquals(foo.getSerializedSize(), copy.getSerializedSize());
    assertFalse(foo.equals(copy));

    Foo secondCopy = Foo.parseFrom(foo.toByteArray());
    assertEquals(copy, secondCopy);

    ExtensionRegistryLite extensionRegistry = ExtensionRegistryLite.newInstance();
    LiteEqualsAndHash.registerAllExtensions(extensionRegistry);
    Foo copyOfCopy = Foo.parseFrom(copy.toByteArray(), extensionRegistry);

    assertEquals(foo, copyOfCopy);
  }

  public void testMalformedBytes() throws Exception {
    try {
      Foo.parseFrom("this is a malformed protocol buffer".getBytes(Internal.UTF_8));
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  public void testMissingStartGroupTag() throws IOException {
    ByteString.Output byteStringOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(byteStringOutput);
    output.writeGroupNoTag(Foo.newBuilder().setValue(11).build());
    output.writeTag(100, WireFormat.WIRETYPE_END_GROUP);
    output.flush();

    try {
      Foo.parseFrom(byteStringOutput.toByteString());
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  public void testMissingEndGroupTag() throws IOException {
    ByteString.Output byteStringOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(byteStringOutput);
    output.writeTag(100, WireFormat.WIRETYPE_START_GROUP);
    output.writeGroupNoTag(Foo.newBuilder().setValue(11).build());
    output.flush();

    try {
      Foo.parseFrom(byteStringOutput.toByteString());
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  public void testMismatchingGroupTags() throws IOException {
    ByteString.Output byteStringOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(byteStringOutput);
    output.writeTag(100, WireFormat.WIRETYPE_START_GROUP);
    output.writeGroupNoTag(Foo.newBuilder().setValue(11).build());
    output.writeTag(101, WireFormat.WIRETYPE_END_GROUP);
    output.flush();

    try {
      Foo.parseFrom(byteStringOutput.toByteString());
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  public void testTruncatedInput() {
    Foo foo =
        Foo.newBuilder()
            .setValue(1)
            .setExtension(Bar.fooExt, Bar.newBuilder().setName("name").build())
            .setExtension(LiteEqualsAndHash.varint, 22)
            .setExtension(
                LiteEqualsAndHash.myGroup,
                LiteEqualsAndHash.MyGroup.newBuilder().setGroupValue("value").build())
            .build();

    try {
      Foo.parseFrom(foo.toByteString().substring(0, foo.toByteString().size() - 10));
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  public void testMakeImmutable() throws Exception {
    UnknownFieldSetLite unknownFields = UnknownFieldSetLite.newInstance();
    unknownFields.makeImmutable();

    try {
      unknownFields.mergeVarintField(1, 1);
      fail();
    } catch (UnsupportedOperationException expected) {
    }

    try {
      unknownFields.mergeLengthDelimitedField(2, ByteString.copyFromUtf8("hello"));
      fail();
    } catch (UnsupportedOperationException expected) {
    }

    try {
      unknownFields.mergeFieldFrom(1, CodedInputStream.newInstance(new byte[0]));
      fail();
    } catch (UnsupportedOperationException expected) {
    }
  }

  public void testEndToEnd() throws Exception {
    TestAllTypesLite testAllTypes = TestAllTypesLite.getDefaultInstance();
    try {
      testAllTypes.unknownFields.checkMutable();
      fail();
    } catch (UnsupportedOperationException expected) {
    }

    testAllTypes = TestAllTypesLite.parseFrom(new byte[0]);
    try {
      testAllTypes.unknownFields.checkMutable();
      fail();
    } catch (UnsupportedOperationException expected) {
    }

    testAllTypes = TestAllTypesLite.newBuilder().build();
    try {
      testAllTypes.unknownFields.checkMutable();
      fail();
    } catch (UnsupportedOperationException expected) {
    }

    testAllTypes = TestAllTypesLite.newBuilder().setDefaultBool(true).build();
    try {
      testAllTypes.unknownFields.checkMutable();
      fail();
    } catch (UnsupportedOperationException expected) {
    }

    TestAllExtensionsLite testAllExtensions =
        TestAllExtensionsLite.newBuilder()
            .mergeFrom(
                TestAllExtensionsLite.newBuilder()
                    .setExtension(UnittestLite.optionalInt32ExtensionLite, 2)
                    .build()
                    .toByteArray())
            .build();
    try {
      testAllExtensions.unknownFields.checkMutable();
      fail();
    } catch (UnsupportedOperationException expected) {
    }
  }

  private ByteString toByteString(UnknownFieldSetLite unknownFields) {
    ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
    CodedOutputStream output = CodedOutputStream.newInstance(byteArrayOutputStream);
    try {
      unknownFields.writeTo(output);
      output.flush();
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
    return ByteString.copyFrom(byteArrayOutputStream.toByteArray());
  }

  public void testSerializeLite() throws Exception {
    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parseFrom(allFieldsData);
    assertEquals(allFieldsData.size(), emptyMessageLite.getSerializedSize());
    ByteString data = emptyMessageLite.toByteString();
    TestAllTypes message = TestAllTypes.parseFrom(data);
    TestUtil.assertAllFieldsSet(message);
    assertEquals(allFieldsData, data);
  }

  public void testAllExtensionsLite() throws Exception {
    TestAllExtensions allExtensions = TestUtil.getAllExtensionsSet();
    ByteString allExtensionsData = allExtensions.toByteString();
    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parser().parseFrom(allExtensionsData);
    ByteString data = emptyMessageLite.toByteString();
    TestAllExtensions message = TestAllExtensions.parseFrom(data, TestUtil.getExtensionRegistry());
    TestUtil.assertAllExtensionsSet(message);
    assertEquals(allExtensionsData, data);
  }

  public void testAllPackedFieldsLite() throws Exception {
    TestPackedTypes allPackedFields = TestUtil.getPackedSet();
    ByteString allPackedData = allPackedFields.toByteString();
    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parseFrom(allPackedData);
    ByteString data = emptyMessageLite.toByteString();
    TestPackedTypes message = TestPackedTypes.parseFrom(data, TestUtil.getExtensionRegistry());
    TestUtil.assertPackedFieldsSet(message);
    assertEquals(allPackedData, data);
  }

  public void testAllPackedExtensionsLite() throws Exception {
    TestPackedExtensions allPackedExtensions = TestUtil.getPackedExtensionsSet();
    ByteString allPackedExtensionsData = allPackedExtensions.toByteString();
    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parseFrom(allPackedExtensionsData);
    ByteString data = emptyMessageLite.toByteString();
    TestPackedExtensions message =
        TestPackedExtensions.parseFrom(data, TestUtil.getExtensionRegistry());
    TestUtil.assertPackedExtensionsSet(message);
    assertEquals(allPackedExtensionsData, data);
  }

  public void testCopyFromLite() throws Exception {
    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parseFrom(allFieldsData);
    UnittestLite.TestEmptyMessageLite emptyMessageLite2 =
        UnittestLite.TestEmptyMessageLite.newBuilder().mergeFrom(emptyMessageLite).build();
    assertEquals(emptyMessageLite.toByteString(), emptyMessageLite2.toByteString());
  }

  public void testMergeFromLite() throws Exception {
    TestAllTypes message1 =
        TestAllTypes.newBuilder()
            .setOptionalInt32(1)
            .setOptionalString("foo")
            .addRepeatedString("bar")
            .setOptionalNestedEnum(TestAllTypes.NestedEnum.BAZ)
            .build();

    TestAllTypes message2 =
        TestAllTypes.newBuilder()
            .setOptionalInt64(2)
            .setOptionalString("baz")
            .addRepeatedString("qux")
            .setOptionalForeignEnum(ForeignEnum.FOREIGN_BAZ)
            .build();

    ByteString data1 = message1.toByteString();
    UnittestLite.TestEmptyMessageLite emptyMessageLite1 =
        UnittestLite.TestEmptyMessageLite.parseFrom(data1);
    ByteString data2 = message2.toByteString();
    UnittestLite.TestEmptyMessageLite emptyMessageLite2 =
        UnittestLite.TestEmptyMessageLite.parseFrom(data2);

    message1 = TestAllTypes.newBuilder(message1).mergeFrom(message2).build();
    emptyMessageLite1 =
        UnittestLite.TestEmptyMessageLite.newBuilder(emptyMessageLite1)
            .mergeFrom(emptyMessageLite2)
            .build();

    data1 = emptyMessageLite1.toByteString();
    message2 = TestAllTypes.parseFrom(data1);

    assertEquals(message1, message2);
  }

  public void testWrongTypeTreatedAsUnknownLite() throws Exception {
    // Test that fields of the wrong wire type are treated like unknown fields
    // when parsing.

    ByteString bizarroData = getBizarroData();
    TestAllTypes allTypesMessage = TestAllTypes.parseFrom(bizarroData);
    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parseFrom(bizarroData);
    ByteString data = emptyMessageLite.toByteString();
    TestAllTypes allTypesMessage2 = TestAllTypes.parseFrom(data);

    assertEquals(allTypesMessage.toString(), allTypesMessage2.toString());
  }

  public void testUnknownExtensionsLite() throws Exception {
    // Make sure fields are properly parsed to the UnknownFieldSet even when
    // they are declared as extension numbers.

    UnittestLite.TestEmptyMessageWithExtensionsLite message =
        UnittestLite.TestEmptyMessageWithExtensionsLite.parseFrom(allFieldsData);

    assertEquals(allFieldsData, message.toByteString());
  }

  public void testWrongExtensionTypeTreatedAsUnknownLite() throws Exception {
    // Test that fields of the wrong wire type are treated like unknown fields
    // when parsing extensions.

    ByteString bizarroData = getBizarroData();
    TestAllExtensions allExtensionsMessage = TestAllExtensions.parseFrom(bizarroData);
    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parseFrom(bizarroData);

    // All fields should have been interpreted as unknown, so the byte strings
    // should be the same.
    assertEquals(emptyMessageLite.toByteString(), allExtensionsMessage.toByteString());
  }

  public void testParseUnknownEnumValueLite() throws Exception {
    Descriptors.FieldDescriptor singularField =
        TestAllTypes.getDescriptor().findFieldByName("optional_nested_enum");
    Descriptors.FieldDescriptor repeatedField =
        TestAllTypes.getDescriptor().findFieldByName("repeated_nested_enum");
    assertNotNull(singularField);
    assertNotNull(repeatedField);

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

    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parseFrom(data);
    data = emptyMessageLite.toByteString();

    {
      TestAllTypes message = TestAllTypes.parseFrom(data);
      assertEquals(TestAllTypes.NestedEnum.BAR, message.getOptionalNestedEnum());
      assertEquals(
          Arrays.asList(TestAllTypes.NestedEnum.FOO, TestAllTypes.NestedEnum.BAZ),
          message.getRepeatedNestedEnumList());
      assertEquals(
          Arrays.asList(5L),
          message.getUnknownFields().getField(singularField.getNumber()).getVarintList());
      assertEquals(
          Arrays.asList(4L, 6L),
          message.getUnknownFields().getField(repeatedField.getNumber()).getVarintList());
    }

    {
      TestAllExtensions message =
          TestAllExtensions.parseFrom(data, TestUtil.getExtensionRegistry());
      assertEquals(
          TestAllTypes.NestedEnum.BAR,
          message.getExtension(UnittestProto.optionalNestedEnumExtension));
      assertEquals(
          Arrays.asList(TestAllTypes.NestedEnum.FOO, TestAllTypes.NestedEnum.BAZ),
          message.getExtension(UnittestProto.repeatedNestedEnumExtension));
      assertEquals(
          Arrays.asList(5L),
          message.getUnknownFields().getField(singularField.getNumber()).getVarintList());
      assertEquals(
          Arrays.asList(4L, 6L),
          message.getUnknownFields().getField(repeatedField.getNumber()).getVarintList());
    }
  }

  public void testClearLite() throws Exception {
    UnittestLite.TestEmptyMessageLite emptyMessageLite1 =
        UnittestLite.TestEmptyMessageLite.parseFrom(allFieldsData);
    UnittestLite.TestEmptyMessageLite emptyMessageLite2 =
        UnittestLite.TestEmptyMessageLite.newBuilder().mergeFrom(emptyMessageLite1).clear().build();
    assertEquals(0, emptyMessageLite2.getSerializedSize());
    ByteString data = emptyMessageLite2.toByteString();
    assertEquals(0, data.size());
  }
}
