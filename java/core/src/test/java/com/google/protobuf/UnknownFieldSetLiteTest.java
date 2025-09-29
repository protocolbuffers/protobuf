// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;
import static org.junit.Assert.assertThrows;

import com.google.protobuf.UnittestLite.TestAllExtensionsLite;
import com.google.protobuf.UnittestLite.TestAllTypesLite;
import proto2_unittest.UnittestProto;
import proto2_unittest.UnittestProto.ForeignEnum;
import proto2_unittest.UnittestProto.TestAllExtensions;
import proto2_unittest.UnittestProto.TestAllTypes;
import proto2_unittest.UnittestProto.TestEmptyMessage;
import proto2_unittest.UnittestProto.TestPackedExtensions;
import proto2_unittest.UnittestProto.TestPackedTypes;
import proto2_unittest.lite_equals_and_hash.LiteEqualsAndHash;
import proto2_unittest.lite_equals_and_hash.LiteEqualsAndHash.Bar;
import proto2_unittest.lite_equals_and_hash.LiteEqualsAndHash.Foo;
import proto3_unittest.UnittestProto3;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for {@link UnknownFieldSetLite}. */
@RunWith(JUnit4.class)
public class UnknownFieldSetLiteTest {

  @Before
  public void setUp() throws Exception {
    allFields = TestUtil.getAllSet();
    allFieldsData = allFields.toByteString();
    emptyMessage = TestEmptyMessage.parseFrom(allFieldsData);
  }

  TestAllTypes allFields;
  ByteString allFieldsData;

  // Constructs a protocol buffer which contains multiple conflicting definitions for the same field
  // number.
  private ByteString getBizarroData() throws Exception {
    UnknownFieldSetLite unknownFields = UnknownFieldSetLite.newInstance();
    unknownFields.storeField(WireFormat.makeTag(1, WireFormat.WIRETYPE_VARINT), 1337L);
    unknownFields.storeField(WireFormat.makeTag(1, WireFormat.WIRETYPE_FIXED32), 42);
    unknownFields.storeField(WireFormat.makeTag(1, WireFormat.WIRETYPE_FIXED64), 0xDEADBEEFL);
    return toByteString(unknownFields);
  }

  // An empty message that has been parsed from allFieldsData.  So, it has
  // unknown fields of every type.
  TestEmptyMessage emptyMessage;

  @Test
  public void testDefaultInstance() {
    UnknownFieldSetLite unknownFields = UnknownFieldSetLite.getDefaultInstance();

    assertThat(unknownFields.getSerializedSize()).isEqualTo(0);
    assertThat(toByteString(unknownFields)).isEqualTo(ByteString.EMPTY);
  }

  @Test
  public void testEmptyInstance() {
    UnknownFieldSetLite instance = UnknownFieldSetLite.newInstance();

    assertThat(instance.getSerializedSize()).isEqualTo(0);
    assertThat(toByteString(instance)).isEqualTo(ByteString.EMPTY);
    assertThat(instance).isEqualTo(UnknownFieldSetLite.getDefaultInstance());
  }

  @Test
  public void testMergeFieldFrom() throws IOException {
    Foo foo = Foo.newBuilder().setValue(2).build();

    CodedInputStream input = CodedInputStream.newInstance(foo.toByteArray());

    UnknownFieldSetLite instance = UnknownFieldSetLite.newInstance();
    instance.mergeFieldFrom(input.readTag(), input);

    assertThat(toByteString(instance)).isEqualTo(foo.toByteString());
  }

  @Test
  public void testMergeFieldFromInvalidEndGroup() throws IOException {
    byte[] data = new byte[] {(byte) WireFormat.makeTag(1, WireFormat.WIRETYPE_END_GROUP)};
    CodedInputStream input = CodedInputStream.newInstance(data);

    UnknownFieldSetLite instance = UnknownFieldSetLite.newInstance();
    assertThrows(
        InvalidProtocolBufferException.class,
        () -> instance.mergeFieldFrom(input.readTag(), input));
  }

  @Test
  public void testSerializedSize() throws IOException {
    Foo foo = Foo.newBuilder().setValue(2).build();

    CodedInputStream input = CodedInputStream.newInstance(foo.toByteArray());

    UnknownFieldSetLite instance = UnknownFieldSetLite.newInstance();
    instance.mergeFieldFrom(input.readTag(), input);

    assertThat(foo.toByteString().size()).isEqualTo(instance.getSerializedSize());
  }

  @Test
  public void testHashCodeAfterDeserialization() throws IOException {
    Foo foo = Foo.newBuilder().setValue(2).build();

    Foo fooDeserialized = Foo.parseFrom(foo.toByteArray());

    assertThat(foo).isEqualTo(fooDeserialized);
    assertThat(foo.hashCode()).isEqualTo(fooDeserialized.hashCode());
  }

  @Test
  public void testNewInstanceHashCode() {
    UnknownFieldSetLite emptyFieldSet = UnknownFieldSetLite.getDefaultInstance();
    UnknownFieldSetLite paddedFieldSet = UnknownFieldSetLite.newInstance();

    assertThat(paddedFieldSet).isEqualTo(emptyFieldSet);
    assertThat(paddedFieldSet.hashCode()).isEqualTo(emptyFieldSet.hashCode());
  }

  @Test
  public void testMergeVarintField() throws IOException {
    UnknownFieldSetLite unknownFields = UnknownFieldSetLite.newInstance();
    unknownFields.mergeVarintField(10, 2);

    CodedInputStream input =
        CodedInputStream.newInstance(toByteString(unknownFields).toByteArray());

    int tag = input.readTag();
    assertThat(WireFormat.getTagFieldNumber(tag)).isEqualTo(10);
    assertThat(WireFormat.getTagWireType(tag)).isEqualTo(WireFormat.WIRETYPE_VARINT);
    assertThat(input.readUInt64()).isEqualTo(2);
    assertThat(input.isAtEnd()).isTrue();
  }

  @Test
  public void testMergeVarintField_negative() throws IOException {
    UnknownFieldSetLite builder = UnknownFieldSetLite.newInstance();
    builder.mergeVarintField(10, -6);

    CodedInputStream input = CodedInputStream.newInstance(toByteString(builder).toByteArray());

    int tag = input.readTag();
    assertThat(WireFormat.getTagFieldNumber(tag)).isEqualTo(10);
    assertThat(WireFormat.getTagWireType(tag)).isEqualTo(WireFormat.WIRETYPE_VARINT);
    assertThat(input.readUInt64()).isEqualTo(-6);
    assertThat(input.isAtEnd()).isTrue();
  }

  @Test
  public void testEqualsAndHashCode() {
    UnknownFieldSetLite unknownFields1 = UnknownFieldSetLite.newInstance();
    unknownFields1.mergeVarintField(10, 2);

    UnknownFieldSetLite unknownFields2 = UnknownFieldSetLite.newInstance();
    unknownFields2.mergeVarintField(10, 2);

    assertThat(unknownFields2).isEqualTo(unknownFields1);
    assertThat(unknownFields1.hashCode()).isEqualTo(unknownFields2.hashCode());
    assertThat(unknownFields1.equals(UnknownFieldSetLite.getDefaultInstance())).isFalse();
    assertThat(unknownFields1.hashCode())
        .isNotEqualTo(UnknownFieldSetLite.getDefaultInstance().hashCode());
  }

  @Test
  public void testMutableCopyOf() throws IOException {
    UnknownFieldSetLite unknownFields = UnknownFieldSetLite.newInstance();
    unknownFields.mergeVarintField(10, 2);
    unknownFields = UnknownFieldSetLite.mutableCopyOf(unknownFields, unknownFields);
    unknownFields.checkMutable();

    CodedInputStream input =
        CodedInputStream.newInstance(toByteString(unknownFields).toByteArray());

    int tag = input.readTag();
    assertThat(WireFormat.getTagFieldNumber(tag)).isEqualTo(10);
    assertThat(WireFormat.getTagWireType(tag)).isEqualTo(WireFormat.WIRETYPE_VARINT);
    assertThat(input.readUInt64()).isEqualTo(2);
    assertThat(input.isAtEnd()).isFalse();
    input.readTag();
    assertThat(WireFormat.getTagFieldNumber(tag)).isEqualTo(10);
    assertThat(WireFormat.getTagWireType(tag)).isEqualTo(WireFormat.WIRETYPE_VARINT);
    assertThat(input.readUInt64()).isEqualTo(2);
    assertThat(input.isAtEnd()).isTrue();
  }

  @Test
  public void testMutableCopyOf_empty() {
    UnknownFieldSetLite unknownFields =
        UnknownFieldSetLite.mutableCopyOf(
            UnknownFieldSetLite.getDefaultInstance(), UnknownFieldSetLite.getDefaultInstance());
    unknownFields.checkMutable();

    assertThat(unknownFields.getSerializedSize()).isEqualTo(0);
    assertThat(toByteString(unknownFields)).isEqualTo(ByteString.EMPTY);
  }

  @Test
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

    assertThat(foo.getSerializedSize()).isEqualTo(copy.getSerializedSize());
    assertThat(foo.equals(copy)).isFalse();

    Foo secondCopy = Foo.parseFrom(foo.toByteArray());
    assertThat(secondCopy).isEqualTo(copy);

    ExtensionRegistryLite extensionRegistry = ExtensionRegistryLite.newInstance();
    LiteEqualsAndHash.registerAllExtensions(extensionRegistry);
    Foo copyOfCopy = Foo.parseFrom(copy.toByteArray(), extensionRegistry);

    assertThat(copyOfCopy).isEqualTo(foo);
  }

  @Test
  public void testMalformedBytes() throws Exception {
    try {
      Foo.parseFrom("this is a malformed protocol buffer".getBytes(Internal.UTF_8));
      assertWithMessage("expected exception").fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  @Test
  public void testMissingStartGroupTag() throws IOException {
    ByteString.Output byteStringOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(byteStringOutput);
    output.writeGroupNoTag(Foo.newBuilder().setValue(11).build());
    output.writeTag(100, WireFormat.WIRETYPE_END_GROUP);
    output.flush();

    try {
      Foo.parseFrom(byteStringOutput.toByteString());
      assertWithMessage("expected exception").fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  @Test
  public void testMissingEndGroupTag() throws IOException {
    ByteString.Output byteStringOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(byteStringOutput);
    output.writeTag(100, WireFormat.WIRETYPE_START_GROUP);
    output.writeGroupNoTag(Foo.newBuilder().setValue(11).build());
    output.flush();

    try {
      Foo.parseFrom(byteStringOutput.toByteString());
      assertWithMessage("expected exception").fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  @Test
  public void testMismatchingGroupTags() throws IOException {
    ByteString.Output byteStringOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(byteStringOutput);
    output.writeTag(100, WireFormat.WIRETYPE_START_GROUP);
    output.writeGroupNoTag(Foo.newBuilder().setValue(11).build());
    output.writeTag(101, WireFormat.WIRETYPE_END_GROUP);
    output.flush();

    try {
      Foo.parseFrom(byteStringOutput.toByteString());
      assertWithMessage("expected exception").fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  @Test
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
      assertWithMessage("expected exception").fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  @Test
  public void testMakeImmutable() throws Exception {
    UnknownFieldSetLite unknownFields = UnknownFieldSetLite.newInstance();
    unknownFields.makeImmutable();

    try {
      unknownFields.mergeVarintField(1, 1);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException expected) {
    }

    try {
      unknownFields.mergeLengthDelimitedField(2, ByteString.copyFromUtf8("hello"));
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException expected) {
    }

    try {
      unknownFields.mergeFieldFrom(1, CodedInputStream.newInstance(new byte[0]));
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException expected) {
    }
  }

  @Test
  public void testEndToEnd() throws Exception {
    TestAllTypesLite testAllTypes = TestAllTypesLite.getDefaultInstance();
    try {
      testAllTypes.unknownFields.checkMutable();
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException expected) {
    }

    testAllTypes = TestAllTypesLite.parseFrom(new byte[0]);
    try {
      testAllTypes.unknownFields.checkMutable();
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException expected) {
    }

    testAllTypes = TestAllTypesLite.newBuilder().build();
    try {
      testAllTypes.unknownFields.checkMutable();
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException expected) {
    }

    testAllTypes = TestAllTypesLite.newBuilder().setDefaultBool(true).build();
    try {
      testAllTypes.unknownFields.checkMutable();
      assertWithMessage("expected exception").fail();
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
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException expected) {
    }
  }

  private static ByteString toByteString(UnknownFieldSetLite unknownFields) {
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

  @Test
  public void testSerializeLite() throws Exception {
    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parseFrom(allFieldsData);
    assertThat(emptyMessageLite.getSerializedSize()).isEqualTo(allFieldsData.size());
    ByteString data = emptyMessageLite.toByteString();
    TestAllTypes message = TestAllTypes.parseFrom(data);
    TestUtil.assertAllFieldsSet(message);
    assertThat(data).isEqualTo(allFieldsData);
  }

  @Test
  public void testAllExtensionsLite() throws Exception {
    TestAllExtensions allExtensions = TestUtil.getAllExtensionsSet();
    ByteString allExtensionsData = allExtensions.toByteString();
    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parseFrom(allExtensionsData);
    ByteString data = emptyMessageLite.toByteString();
    TestAllExtensions message = TestAllExtensions.parseFrom(data, TestUtil.getExtensionRegistry());
    TestUtil.assertAllExtensionsSet(message);
    assertThat(data).isEqualTo(allExtensionsData);
  }

  @Test
  public void testAllExtensions_areImmutable() throws Exception {
    // Empty message
    TestUtil.assertRepeatedExtensionsImmutable(TestAllExtensions.getDefaultInstance());

    // Message populated with all extensions
    TestUtil.assertRepeatedExtensionsImmutable(TestUtil.getAllExtensionsSet());
  }

  @Test
  public void testAllPackedFieldsLite() throws Exception {
    TestPackedTypes allPackedFields = TestUtil.getPackedSet();
    ByteString allPackedData = allPackedFields.toByteString();
    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parseFrom(allPackedData);
    ByteString data = emptyMessageLite.toByteString();
    TestPackedTypes message = TestPackedTypes.parseFrom(data, TestUtil.getExtensionRegistry());
    TestUtil.assertPackedFieldsSet(message);
    assertThat(data).isEqualTo(allPackedData);
  }

  @Test
  public void testAllPackedExtensionsLite() throws Exception {
    TestPackedExtensions allPackedExtensions = TestUtil.getPackedExtensionsSet();
    ByteString allPackedExtensionsData = allPackedExtensions.toByteString();
    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parseFrom(allPackedExtensionsData);
    ByteString data = emptyMessageLite.toByteString();
    TestPackedExtensions message =
        TestPackedExtensions.parseFrom(data, TestUtil.getExtensionRegistry());
    TestUtil.assertPackedExtensionsSet(message);
    assertThat(data).isEqualTo(allPackedExtensionsData);
  }

  @Test
  @SuppressWarnings("ProtoNewBuilderMergeFrom")
  public void testCopyFromLite() throws Exception {
    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parseFrom(allFieldsData);
    UnittestLite.TestEmptyMessageLite emptyMessageLite2 =
        UnittestLite.TestEmptyMessageLite.newBuilder().mergeFrom(emptyMessageLite).build();
    assertThat(emptyMessageLite2.toByteString()).isEqualTo(emptyMessageLite.toByteString());
  }

  @Test
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

    assertThat(message2).isEqualTo(message1);
  }

  @Test
  public void testWrongTypeTreatedAsUnknownLite() throws Exception {
    // Test that fields of the wrong wire type are treated like unknown fields
    // when parsing.

    ByteString bizarroData = getBizarroData();
    TestAllTypes allTypesMessage = TestAllTypes.parseFrom(bizarroData);
    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parseFrom(bizarroData);
    ByteString data = emptyMessageLite.toByteString();
    TestAllTypes allTypesMessage2 = TestAllTypes.parseFrom(data);

    assertThat(allTypesMessage2.toString()).isEqualTo(allTypesMessage.toString());
  }

  @Test
  public void testUnknownExtensionsLite() throws Exception {
    // Make sure fields are properly parsed to the UnknownFieldSet even when
    // they are declared as extension numbers.

    UnittestLite.TestEmptyMessageWithExtensionsLite message =
        UnittestLite.TestEmptyMessageWithExtensionsLite.parseFrom(allFieldsData);

    assertThat(message.toByteString()).isEqualTo(allFieldsData);
  }

  @Test
  public void testWrongExtensionTypeTreatedAsUnknownLite() throws Exception {
    // Test that fields of the wrong wire type are treated like unknown fields
    // when parsing extensions.

    ByteString bizarroData = getBizarroData();
    TestAllExtensions allExtensionsMessage = TestAllExtensions.parseFrom(bizarroData);
    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parseFrom(bizarroData);

    // All fields should have been interpreted as unknown, so the byte strings
    // should be the same.
    assertThat(allExtensionsMessage.toByteString()).isEqualTo(emptyMessageLite.toByteString());
  }

  @Test
  public void testParseUnknownEnumValueLite() throws Exception {
    ByteString data =
        toByteString(
            UnknownFieldSetLite.newInstance()
                .mergeVarintField(
                    TestAllTypes.OPTIONAL_NESTED_ENUM_FIELD_NUMBER,
                    TestAllTypes.NestedEnum.BAR_VALUE)
                .mergeVarintField(
                    TestAllTypes.REPEATED_NESTED_ENUM_FIELD_NUMBER,
                    TestAllTypes.NestedEnum.FOO_VALUE)
                .mergeVarintField(
                    TestAllTypes.REPEATED_NESTED_ENUM_FIELD_NUMBER,
                    TestAllTypes.NestedEnum.BAZ_VALUE)
                // Invalid values.  Since the internals of UnknownFieldSetLite are private, the only
                // way to verify that unknown values are preserved is to do a round-trip test, and
                // unknown values are always serialized last.
                .mergeVarintField(TestAllTypes.OPTIONAL_NESTED_ENUM_FIELD_NUMBER, 5)
                .mergeVarintField(TestAllTypes.REPEATED_NESTED_ENUM_FIELD_NUMBER, 4)
                .mergeVarintField(TestAllTypes.REPEATED_NESTED_ENUM_FIELD_NUMBER, 6));

    UnittestLite.TestEmptyMessageLite emptyMessageLite =
        UnittestLite.TestEmptyMessageLite.parseFrom(data);
    assertThat(emptyMessageLite.toByteString()).isEqualTo(data);

    {
      TestAllTypes message = TestAllTypes.parseFrom(data);
      assertThat(message.getOptionalNestedEnum()).isEqualTo(TestAllTypes.NestedEnum.BAR);
      assertThat(message.getRepeatedNestedEnumList())
          .containsExactly(TestAllTypes.NestedEnum.FOO, TestAllTypes.NestedEnum.BAZ)
          .inOrder();
      assertThat(message.toByteString()).isEqualTo(data);
    }

    {
      TestAllExtensions message =
          TestAllExtensions.parseFrom(data, TestUtil.getExtensionRegistry());
      assertThat(message.getExtension(UnittestProto.optionalNestedEnumExtension))
          .isEqualTo(TestAllTypes.NestedEnum.BAR);
      assertThat(message.getExtension(UnittestProto.repeatedNestedEnumExtension))
          .containsExactly(TestAllTypes.NestedEnum.FOO, TestAllTypes.NestedEnum.BAZ)
          .inOrder();
      assertThat(message.toByteString()).isEqualTo(data);
    }
  }

  @Test
  @SuppressWarnings("ProtoNewBuilderMergeFrom")
  public void testClearLite() throws Exception {
    UnittestLite.TestEmptyMessageLite emptyMessageLite1 =
        UnittestLite.TestEmptyMessageLite.parseFrom(allFieldsData);
    UnittestLite.TestEmptyMessageLite emptyMessageLite2 =
        UnittestLite.TestEmptyMessageLite.newBuilder().mergeFrom(emptyMessageLite1).clear().build();
    assertThat(emptyMessageLite2.getSerializedSize()).isEqualTo(0);
    ByteString data = emptyMessageLite2.toByteString();
    assertThat(data.size()).isEqualTo(0);
  }

  @Test
  @SuppressWarnings("ProtoNewBuilderMergeFrom")
  public void testProto3RoundTrip() throws Exception {
    ByteString data = getBizarroData();

    UnittestProto3.TestEmptyMessage message =
        UnittestProto3.TestEmptyMessage.parseFrom(data, ExtensionRegistryLite.getEmptyRegistry());
    assertThat(message.toByteString()).isEqualTo(data);

    message = UnittestProto3.TestEmptyMessage.newBuilder().mergeFrom(message).build();
    assertThat(message.toByteString()).isEqualTo(data);

    // regression test for b/142710815
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
