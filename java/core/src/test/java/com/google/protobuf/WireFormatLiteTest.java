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

import static com.google.protobuf.UnittestLite.optionalForeignEnumExtensionLite;

import com.google.protobuf.UnittestLite.ForeignEnumLite;
import com.google.protobuf.UnittestLite.TestAllExtensionsLite;
import com.google.protobuf.UnittestLite.TestPackedExtensionsLite;
import map_test.MapForProto2TestProto;
import map_test.MapTestProto.TestMap;
import protobuf_unittest.UnittestMset.RawMessageSet;
import protobuf_unittest.UnittestMset.TestMessageSetExtension1;
import protobuf_unittest.UnittestMset.TestMessageSetExtension2;
import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestExtensionInsideTable;
import protobuf_unittest.UnittestProto.TestFieldOrderings;
import protobuf_unittest.UnittestProto.TestOneof2;
import protobuf_unittest.UnittestProto.TestOneofBackwardsCompatible;
import protobuf_unittest.UnittestProto.TestPackedExtensions;
import protobuf_unittest.UnittestProto.TestPackedTypes;
import proto2_wireformat_unittest.UnittestMsetWireFormat.TestMessageSet;
import proto3_unittest.UnittestProto3;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.List;
import junit.framework.TestCase;

public class WireFormatLiteTest extends TestCase {
  public void testSerializeExtensionsLite() throws Exception {
    // TestAllTypes and TestAllExtensions should have compatible wire formats,
    // so if we serialize a TestAllExtensions then parse it as TestAllTypes
    // it should work.

    TestAllExtensionsLite message = TestUtilLite.getAllLiteExtensionsSet();
    ByteString rawBytes = message.toByteString();
    assertEquals(rawBytes.size(), message.getSerializedSize());

    TestAllTypes message2 = TestAllTypes.parseFrom(rawBytes);

    TestUtil.assertAllFieldsSet(message2);
  }

  public void testSerializePackedExtensionsLite() throws Exception {
    // TestPackedTypes and TestPackedExtensions should have compatible wire
    // formats; check that they serialize to the same string.
    TestPackedExtensionsLite message = TestUtilLite.getLitePackedExtensionsSet();
    ByteString rawBytes = message.toByteString();

    TestPackedTypes message2 = TestUtil.getPackedSet();
    ByteString rawBytes2 = message2.toByteString();

    assertEquals(rawBytes, rawBytes2);
  }

  public void testParseExtensionsLite() throws Exception {
    // TestAllTypes and TestAllExtensions should have compatible wire formats,
    // so if we serialize a TestAllTypes then parse it as TestAllExtensions
    // it should work.

    TestAllTypes message = TestUtil.getAllSet();
    ByteString rawBytes = message.toByteString();

    ExtensionRegistryLite registryLite = TestUtilLite.getExtensionRegistryLite();

    TestAllExtensionsLite message2 = TestAllExtensionsLite.parseFrom(rawBytes, registryLite);
    TestUtil.assertAllExtensionsSet(message2);
    message2 = TestAllExtensionsLite.parseFrom(message.toByteArray(), registryLite);
    TestUtil.assertAllExtensionsSet(message2);
  }

  public void testParsePackedExtensionsLite() throws Exception {
    // Ensure that packed extensions can be properly parsed.
    TestPackedExtensionsLite message = TestUtilLite.getLitePackedExtensionsSet();
    ByteString rawBytes = message.toByteString();

    ExtensionRegistryLite registry = TestUtilLite.getExtensionRegistryLite();

    TestPackedExtensionsLite message2 = TestPackedExtensionsLite.parseFrom(rawBytes, registry);
    TestUtil.assertPackedExtensionsSet(message2);
    message2 = TestPackedExtensionsLite.parseFrom(message.toByteArray(), registry);
    TestUtil.assertPackedExtensionsSet(message2);
  }

  public void testSerialization() throws Exception {
    TestAllTypes message = TestUtil.getAllSet();

    ByteString rawBytes = message.toByteString();
    assertEquals(rawBytes.size(), message.getSerializedSize());

    TestAllTypes message2 = TestAllTypes.parseFrom(rawBytes);

    TestUtil.assertAllFieldsSet(message2);
  }

  public void testSerializationPacked() throws Exception {
    TestPackedTypes message = TestUtil.getPackedSet();

    ByteString rawBytes = message.toByteString();
    assertEquals(rawBytes.size(), message.getSerializedSize());

    TestPackedTypes message2 = TestPackedTypes.parseFrom(rawBytes);

    TestUtil.assertPackedFieldsSet(message2);
  }

  public void testSerializeExtensions() throws Exception {
    // TestAllTypes and TestAllExtensions should have compatible wire formats,
    // so if we serialize a TestAllExtensions then parse it as TestAllTypes
    // it should work.

    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    ByteString rawBytes = message.toByteString();
    assertEquals(rawBytes.size(), message.getSerializedSize());

    TestAllTypes message2 = TestAllTypes.parseFrom(rawBytes);

    TestUtil.assertAllFieldsSet(message2);
  }

  public void testSerializePackedExtensions() throws Exception {
    // TestPackedTypes and TestPackedExtensions should have compatible wire
    // formats; check that they serialize to the same string.
    TestPackedExtensions message = TestUtil.getPackedExtensionsSet();
    ByteString rawBytes = message.toByteString();

    TestPackedTypes message2 = TestUtil.getPackedSet();
    ByteString rawBytes2 = message2.toByteString();

    assertEquals(rawBytes, rawBytes2);
  }

  public void testSerializationPackedWithoutGetSerializedSize() throws Exception {
    // Write directly to an OutputStream, without invoking getSerializedSize()
    // This used to be a bug where the size of a packed field was incorrect,
    // since getSerializedSize() was never invoked.
    TestPackedTypes message = TestUtil.getPackedSet();

    // Directly construct a CodedOutputStream around the actual OutputStream,
    // in case writeTo(OutputStream output) invokes getSerializedSize();
    ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
    CodedOutputStream codedOutput = CodedOutputStream.newInstance(outputStream);

    message.writeTo(codedOutput);

    codedOutput.flush();

    TestPackedTypes message2 = TestPackedTypes.parseFrom(outputStream.toByteArray());

    TestUtil.assertPackedFieldsSet(message2);
  }

  public void testParseExtensions() throws Exception {
    // TestAllTypes and TestAllExtensions should have compatible wire formats,
    // so if we serialize a TestAllTypes then parse it as TestAllExtensions
    // it should work.

    TestAllTypes message = TestUtil.getAllSet();
    ByteString rawBytes = message.toByteString();

    ExtensionRegistryLite registry = TestUtil.getExtensionRegistry();

    TestAllExtensions message2 = TestAllExtensions.parseFrom(rawBytes, registry);

    TestUtil.assertAllExtensionsSet(message2);
  }

  public void testParsePackedExtensions() throws Exception {
    // Ensure that packed extensions can be properly parsed.
    TestPackedExtensions message = TestUtil.getPackedExtensionsSet();
    ByteString rawBytes = message.toByteString();

    ExtensionRegistryLite registry = TestUtil.getExtensionRegistry();

    TestPackedExtensions message2 = TestPackedExtensions.parseFrom(rawBytes, registry);

    TestUtil.assertPackedExtensionsSet(message2);
  }

  public void testSerializeDelimited() throws Exception {
    ByteArrayOutputStream output = new ByteArrayOutputStream();
    TestUtil.getAllSet().writeDelimitedTo(output);
    output.write(12);
    TestUtil.getPackedSet().writeDelimitedTo(output);
    output.write(34);

    ByteArrayInputStream input = new ByteArrayInputStream(output.toByteArray());

    TestUtil.assertAllFieldsSet(TestAllTypes.parseDelimitedFrom(input));
    assertEquals(12, input.read());
    TestUtil.assertPackedFieldsSet(TestPackedTypes.parseDelimitedFrom(input));
    assertEquals(34, input.read());
    assertEquals(-1, input.read());

    // We're at EOF, so parsing again should return null.
    assertNull(TestAllTypes.parseDelimitedFrom(input));
  }

  private ExtensionRegistryLite getTestFieldOrderingsRegistry() {
    ExtensionRegistryLite result = ExtensionRegistryLite.newInstance();
    result.add(UnittestProto.myExtensionInt);
    result.add(UnittestProto.myExtensionString);
    return result;
  }

  public void testParseMultipleExtensionRanges() throws Exception {
    // Make sure we can parse a message that contains multiple extensions
    // ranges.
    TestFieldOrderings source =
        TestFieldOrderings.newBuilder()
            .setMyInt(1)
            .setMyString("foo")
            .setMyFloat(1.0F)
            .setExtension(UnittestProto.myExtensionInt, 23)
            .setExtension(UnittestProto.myExtensionString, "bar")
            .build();
    TestFieldOrderings dest =
        TestFieldOrderings.parseFrom(source.toByteString(), getTestFieldOrderingsRegistry());
    assertEquals(source, dest);
  }

  private static ExtensionRegistryLite getTestExtensionInsideTableRegistry() {
    ExtensionRegistryLite result = ExtensionRegistryLite.newInstance();
    result.add(UnittestProto.testExtensionInsideTableExtension);
    return result;
  }

  public void testExtensionInsideTable() throws Exception {
    // Make sure the extension within the range of table is parsed correctly in experimental
    // runtime.
    TestExtensionInsideTable source =
        TestExtensionInsideTable.newBuilder()
            .setField1(1)
            .setExtension(UnittestProto.testExtensionInsideTableExtension, 23)
            .build();
    TestExtensionInsideTable dest =
        TestExtensionInsideTable.parseFrom(
            source.toByteString(), getTestExtensionInsideTableRegistry());
    assertEquals(source, dest);
  }

  private static final int UNKNOWN_TYPE_ID = 1550055;
  private static final int TYPE_ID_1 = 1545008;
  private static final int TYPE_ID_2 = 1547769;

  public void testSerializeMessageSetEagerly() throws Exception {
    testSerializeMessageSetWithFlag(true);
  }

  public void testSerializeMessageSetNotEagerly() throws Exception {
    testSerializeMessageSetWithFlag(false);
  }

  private void testSerializeMessageSetWithFlag(boolean eagerParsing) throws Exception {
    ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
    CodedOutputStream output = CodedOutputStream.newInstance(byteArrayOutputStream);
    output.writeRawMessageSetExtension(UNKNOWN_TYPE_ID, ByteString.copyFromUtf8("bar"));
    output.flush();
    byte[] messageSetBytes = byteArrayOutputStream.toByteArray();

    ExtensionRegistryLite.setEagerlyParseMessageSets(eagerParsing);
    // Set up a TestMessageSet with two known messages and an unknown one.
    TestMessageSet messageSet =
        TestMessageSet.newBuilder()
            .setExtension(
                TestMessageSetExtension1.messageSetExtension,
                TestMessageSetExtension1.newBuilder().setI(123).build())
            .setExtension(
                TestMessageSetExtension2.messageSetExtension,
                TestMessageSetExtension2.newBuilder().setStr("foo").build())
            .mergeFrom(messageSetBytes)
            .build();

    ByteString data = messageSet.toByteString();

    // Parse back using RawMessageSet and check the contents.
    RawMessageSet raw = RawMessageSet.parseFrom(data);

    assertEquals(3, raw.getItemCount());
    assertEquals(TYPE_ID_1, raw.getItem(0).getTypeId());
    assertEquals(TYPE_ID_2, raw.getItem(1).getTypeId());
    assertEquals(UNKNOWN_TYPE_ID, raw.getItem(2).getTypeId());

    TestMessageSetExtension1 message1 =
        TestMessageSetExtension1.parseFrom(raw.getItem(0).getMessage());
    assertEquals(123, message1.getI());

    TestMessageSetExtension2 message2 =
        TestMessageSetExtension2.parseFrom(raw.getItem(1).getMessage());
    assertEquals("foo", message2.getStr());

    assertEquals("bar", raw.getItem(2).getMessage().toStringUtf8());
  }

  public void testParseMessageSetEagerly() throws Exception {
    testParseMessageSetWithFlag(true);
  }

  public void testParseMessageSetNotEagerly() throws Exception {
    testParseMessageSetWithFlag(false);
  }

  private void testParseMessageSetWithFlag(boolean eagerParsing) throws Exception {
    ExtensionRegistryLite.setEagerlyParseMessageSets(eagerParsing);
    ExtensionRegistryLite extensionRegistry = ExtensionRegistryLite.newInstance();
    extensionRegistry.add(TestMessageSetExtension1.messageSetExtension);
    extensionRegistry.add(TestMessageSetExtension2.messageSetExtension);

    // Set up a RawMessageSet with two known messages and an unknown one.
    RawMessageSet raw =
        RawMessageSet.newBuilder()
            .addItem(
                RawMessageSet.Item.newBuilder()
                    .setTypeId(TYPE_ID_1)
                    .setMessage(
                        TestMessageSetExtension1.newBuilder().setI(123).build().toByteString())
                    .build())
            .addItem(
                RawMessageSet.Item.newBuilder()
                    .setTypeId(TYPE_ID_2)
                    .setMessage(
                        TestMessageSetExtension2.newBuilder().setStr("foo").build().toByteString())
                    .build())
            .addItem(
                RawMessageSet.Item.newBuilder()
                    .setTypeId(UNKNOWN_TYPE_ID)
                    .setMessage(ByteString.copyFromUtf8("bar"))
                    .build())
            .build();

    ByteString data = raw.toByteString();

    // Parse as a TestMessageSet and check the contents.
    TestMessageSet messageSet = TestMessageSet.parseFrom(data, extensionRegistry);

    assertEquals(123, messageSet.getExtension(TestMessageSetExtension1.messageSetExtension).getI());
    assertEquals(
        "foo", messageSet.getExtension(TestMessageSetExtension2.messageSetExtension).getStr());
  }

  public void testParseMessageSetExtensionEagerly() throws Exception {
    testParseMessageSetExtensionWithFlag(true);
  }

  public void testParseMessageSetExtensionNotEagerly() throws Exception {
    testParseMessageSetExtensionWithFlag(false);
  }

  private void testParseMessageSetExtensionWithFlag(boolean eagerParsing) throws Exception {
    ExtensionRegistryLite.setEagerlyParseMessageSets(eagerParsing);
    ExtensionRegistryLite extensionRegistry = ExtensionRegistryLite.newInstance();
    extensionRegistry.add(TestMessageSetExtension1.messageSetExtension);

    // Set up a RawMessageSet with a known messages.
    int typeId1 = 1545008;
    RawMessageSet raw =
        RawMessageSet.newBuilder()
            .addItem(
                RawMessageSet.Item.newBuilder()
                    .setTypeId(typeId1)
                    .setMessage(
                        TestMessageSetExtension1.newBuilder().setI(123).build().toByteString())
                    .build())
            .build();

    ByteString data = raw.toByteString();

    // Parse as a TestMessageSet and check the contents.
    TestMessageSet messageSet = TestMessageSet.parseFrom(data, extensionRegistry);
    assertEquals(123, messageSet.getExtension(TestMessageSetExtension1.messageSetExtension).getI());
  }

  public void testMergeLazyMessageSetExtensionEagerly() throws Exception {
    testMergeLazyMessageSetExtensionWithFlag(true);
  }

  public void testMergeLazyMessageSetExtensionNotEagerly() throws Exception {
    testMergeLazyMessageSetExtensionWithFlag(false);
  }

  private void testMergeLazyMessageSetExtensionWithFlag(boolean eagerParsing) throws Exception {
    ExtensionRegistryLite.setEagerlyParseMessageSets(eagerParsing);
    ExtensionRegistryLite extensionRegistry = ExtensionRegistryLite.newInstance();
    extensionRegistry.add(TestMessageSetExtension1.messageSetExtension);

    // Set up a RawMessageSet with a known messages.
    int typeId1 = 1545008;
    RawMessageSet raw =
        RawMessageSet.newBuilder()
            .addItem(
                RawMessageSet.Item.newBuilder()
                    .setTypeId(typeId1)
                    .setMessage(
                        TestMessageSetExtension1.newBuilder().setI(123).build().toByteString())
                    .build())
            .build();

    ByteString data = raw.toByteString();

    // Parse as a TestMessageSet and store value into lazy field
    TestMessageSet messageSet = TestMessageSet.parseFrom(data, extensionRegistry);
    // Merge lazy field check the contents.
    messageSet = messageSet.toBuilder().mergeFrom(data, extensionRegistry).build();
    assertEquals(123, messageSet.getExtension(TestMessageSetExtension1.messageSetExtension).getI());
  }

  public void testMergeMessageSetExtensionEagerly() throws Exception {
    testMergeMessageSetExtensionWithFlag(true);
  }

  public void testMergeMessageSetExtensionNotEagerly() throws Exception {
    testMergeMessageSetExtensionWithFlag(false);
  }

  private void testMergeMessageSetExtensionWithFlag(boolean eagerParsing) throws Exception {
    ExtensionRegistryLite.setEagerlyParseMessageSets(eagerParsing);
    ExtensionRegistryLite extensionRegistry = ExtensionRegistryLite.newInstance();
    extensionRegistry.add(TestMessageSetExtension1.messageSetExtension);

    // Set up a RawMessageSet with a known messages.
    int typeId1 = 1545008;
    RawMessageSet raw =
        RawMessageSet.newBuilder()
            .addItem(
                RawMessageSet.Item.newBuilder()
                    .setTypeId(typeId1)
                    .setMessage(
                        TestMessageSetExtension1.newBuilder().setI(123).build().toByteString())
                    .build())
            .build();

    // Serialize RawMessageSet unnormally (message value before type id)
    ByteString.CodedBuilder out = ByteString.newCodedBuilder(raw.getSerializedSize());
    CodedOutputStream output = out.getCodedOutput();
    List<RawMessageSet.Item> items = raw.getItemList();
    for (RawMessageSet.Item item : items) {
      output.writeTag(1, WireFormat.WIRETYPE_START_GROUP);
      output.writeBytes(3, item.getMessage());
      output.writeInt32(2, item.getTypeId());
      output.writeTag(1, WireFormat.WIRETYPE_END_GROUP);
    }
    ByteString data = out.build();

    // Merge bytes into TestMessageSet and check the contents.
    TestMessageSet messageSet =
        TestMessageSet.newBuilder().mergeFrom(data, extensionRegistry).build();
    assertEquals(123, messageSet.getExtension(TestMessageSetExtension1.messageSetExtension).getI());
  }

  // ================================================================
  // oneof
  public void testOneofWireFormat() throws Exception {
    TestOneof2.Builder builder = TestOneof2.newBuilder();
    TestUtil.setOneof(builder);
    TestOneof2 message = builder.build();
    ByteString rawBytes = message.toByteString();

    assertEquals(rawBytes.size(), message.getSerializedSize());

    TestOneof2 message2 = TestOneof2.parseFrom(rawBytes);
    TestUtil.assertOneofSet(message2);
  }

  public void testOneofOnlyLastSet() throws Exception {
    TestOneofBackwardsCompatible source =
        TestOneofBackwardsCompatible.newBuilder().setFooInt(100).setFooString("101").build();

    ByteString rawBytes = source.toByteString();
    TestOneof2 message = TestOneof2.parseFrom(rawBytes);
    assertFalse(message.hasFooInt());
    assertTrue(message.hasFooString());
  }

  private void assertInvalidWireFormat(
      MessageLite defaultInstance, byte[] data, int offset, int length) {
    // Test all combinations: (builder vs parser) x (byte[] vs. InputStream).
    try {
      defaultInstance.newBuilderForType().mergeFrom(data, offset, length);
      fail("Expected exception");
    } catch (InvalidProtocolBufferException e) {
      // Pass.
    }
    try {
      defaultInstance.getParserForType().parseFrom(data, offset, length);
      fail("Expected exception");
    } catch (InvalidProtocolBufferException e) {
      // Pass.
    }
    try {
      InputStream input = new ByteArrayInputStream(data, offset, length);
      defaultInstance.newBuilderForType().mergeFrom(input);
      fail("Expected exception");
    } catch (IOException e) {
      // Pass.
    }
    try {
      InputStream input = new ByteArrayInputStream(data, offset, length);
      defaultInstance.getParserForType().parseFrom(input);
      fail("Expected exception");
    } catch (IOException e) {
      // Pass.
    }
  }

  private void assertInvalidWireFormat(MessageLite defaultInstance, byte[] data) {
    assertInvalidWireFormat(defaultInstance, data, 0, data.length);
  }

  private void assertInvalidWireFormat(byte[] data) {
    assertInvalidWireFormat(TestAllTypes.getDefaultInstance(), data);
    assertInvalidWireFormat(UnittestProto3.TestAllTypes.getDefaultInstance(), data);
  }

  public void testParserRejectInvalidTag() throws Exception {
    byte[] invalidTags =
        new byte[] {
          // Zero tag is not allowed.
          0,
          // Invalid wire types.
          (byte) WireFormat.makeTag(1, 6),
          (byte) WireFormat.makeTag(1, 7),
          // Field number 0 is not allowed.
          (byte) WireFormat.makeTag(0, WireFormat.WIRETYPE_VARINT),
        };
    for (byte invalidTag : invalidTags) {
      // Add a trailing 0 to make sure the parsing actually fails on the tag.
      byte[] data = new byte[] {invalidTag, 0};
      assertInvalidWireFormat(data);

      // Invalid tag in an unknown group field.
      data =
          new byte[] {
            (byte) WireFormat.makeTag(1, WireFormat.WIRETYPE_START_GROUP),
            invalidTag,
            0,
            (byte) WireFormat.makeTag(1, WireFormat.WIRETYPE_END_GROUP),
          };
      assertInvalidWireFormat(data);

      // Invalid tag in a MessageSet item.
      data =
          new byte[] {
            (byte) WireFormat.MESSAGE_SET_ITEM_TAG,
            (byte) WireFormat.MESSAGE_SET_TYPE_ID_TAG,
            100, // TYPE_ID = 100
            (byte) WireFormat.MESSAGE_SET_MESSAGE_TAG,
            0, // empty payload
            invalidTag,
            0,
            (byte) WireFormat.MESSAGE_SET_ITEM_END_TAG,
          };
      assertInvalidWireFormat(TestMessageSet.getDefaultInstance(), data);

      // Invalid tag inside a MessageSet item's unknown group.
      data =
          new byte[] {
            (byte) WireFormat.MESSAGE_SET_ITEM_TAG,
            (byte) WireFormat.MESSAGE_SET_TYPE_ID_TAG,
            100, // TYPE_ID = 100
            (byte) WireFormat.MESSAGE_SET_MESSAGE_TAG,
            0, // empty payload
            (byte) WireFormat.makeTag(4, WireFormat.WIRETYPE_START_GROUP),
            invalidTag,
            0,
            (byte) WireFormat.makeTag(4, WireFormat.WIRETYPE_END_GROUP),
            (byte) WireFormat.MESSAGE_SET_ITEM_END_TAG,
          };
      assertInvalidWireFormat(TestMessageSet.getDefaultInstance(), data);

      // Invalid tag inside a map field.
      data =
          new byte[] {
            (byte) WireFormat.makeTag(1, WireFormat.WIRETYPE_LENGTH_DELIMITED), 2, invalidTag, 0,
          };
      assertInvalidWireFormat(TestMap.getDefaultInstance(), data);
    }
  }

  public void testUnmatchedGroupTag() throws Exception {
    int startTag = WireFormat.makeTag(16, WireFormat.WIRETYPE_START_GROUP);
    byte[] data =
        new byte[] {
          (byte) ((startTag & 0x7F) | 0x80), (byte) ((startTag >>> 7) & 0x7F),
        };
    assertInvalidWireFormat(data);

    // Unmatched group tags inside a MessageSet item.
    data =
        new byte[] {
          (byte) WireFormat.MESSAGE_SET_ITEM_TAG,
          (byte) WireFormat.MESSAGE_SET_TYPE_ID_TAG,
          100, // TYPE_ID = 100
          (byte) WireFormat.MESSAGE_SET_MESSAGE_TAG,
          0, // empty payload
          (byte) WireFormat.makeTag(4, WireFormat.WIRETYPE_START_GROUP),
        };
    assertInvalidWireFormat(TestMessageSet.getDefaultInstance(), data);
  }

  private void assertAccepted(MessageLite defaultInstance, byte[] data) throws Exception {
    MessageLite message1 = defaultInstance.newBuilderForType().mergeFrom(data).build();
    MessageLite message2 = defaultInstance.getParserForType().parseFrom(data);
    MessageLite message3 =
        defaultInstance.newBuilderForType().mergeFrom(new ByteArrayInputStream(data)).build();
    MessageLite message4 =
        defaultInstance.getParserForType().parseFrom(new ByteArrayInputStream(data));
    assertEquals(message1, message2);
    assertEquals(message2, message3);
    assertEquals(message3, message4);
  }

  public void testUnmatchedWireType() throws Exception {
    // Build a payload with all fields from 1 to 128 being varints. Parsing it into TestAllTypes
    // or other message types should succeed even though the wire type doesn't match for some
    // fields.
    ByteArrayOutputStream output = new ByteArrayOutputStream();
    CodedOutputStream codedOutput = CodedOutputStream.newInstance(output);
    for (int i = 1; i <= 128; i++) {
      codedOutput.writeInt32(i, 0);
    }
    codedOutput.flush();
    byte[] data = output.toByteArray();
    // It can be parsed into any message type that doesn't have required fields.
    assertAccepted(TestAllTypes.getDefaultInstance(), data);
    assertAccepted(UnittestProto3.TestAllTypes.getDefaultInstance(), data);
    assertAccepted(TestMap.getDefaultInstance(), data);
    assertAccepted(MapForProto2TestProto.TestMap.getDefaultInstance(), data);
  }

  public void testParseTruncatedPackedFields() throws Exception {
    TestPackedTypes all = TestUtil.getPackedSet();
    TestPackedTypes[] messages =
        new TestPackedTypes[] {
          TestPackedTypes.newBuilder().addAllPackedInt32(all.getPackedInt32List()).build(),
          TestPackedTypes.newBuilder().addAllPackedInt64(all.getPackedInt64List()).build(),
          TestPackedTypes.newBuilder().addAllPackedUint32(all.getPackedUint32List()).build(),
          TestPackedTypes.newBuilder().addAllPackedUint64(all.getPackedUint64List()).build(),
          TestPackedTypes.newBuilder().addAllPackedSint32(all.getPackedSint32List()).build(),
          TestPackedTypes.newBuilder().addAllPackedSint64(all.getPackedSint64List()).build(),
          TestPackedTypes.newBuilder().addAllPackedFixed32(all.getPackedFixed32List()).build(),
          TestPackedTypes.newBuilder().addAllPackedFixed64(all.getPackedFixed64List()).build(),
          TestPackedTypes.newBuilder().addAllPackedSfixed32(all.getPackedSfixed32List()).build(),
          TestPackedTypes.newBuilder().addAllPackedSfixed64(all.getPackedSfixed64List()).build(),
          TestPackedTypes.newBuilder().addAllPackedFloat(all.getPackedFloatList()).build(),
          TestPackedTypes.newBuilder().addAllPackedDouble(all.getPackedDoubleList()).build(),
          TestPackedTypes.newBuilder().addAllPackedEnum(all.getPackedEnumList()).build(),
        };
    for (TestPackedTypes message : messages) {
      byte[] data = message.toByteArray();
      // Parsing truncated payload should fail.
      for (int i = 1; i < data.length; i++) {
        assertInvalidWireFormat(TestPackedTypes.getDefaultInstance(), data, 0, i);
      }
    }
  }

  public void testParsePackedFieldsWithIncorrectLength() throws Exception {
    // Set the length-prefix to 1 with a 4-bytes payload to test what happens when reading a packed
    // element moves the reading position past the given length limit. It should result in an
    // InvalidProtocolBufferException but an implementation may forget to check it especially for
    // packed varint fields.
    byte[] data =
        new byte[] {
          0,
          0, // first two bytes is reserved for the tag.
          1, // length is 1
          (byte) 0x80,
          (byte) 0x80,
          (byte) 0x80,
          (byte) 0x01, // a 4-bytes varint
        };
    // All fields that can read a 4-bytes varint (all varint fields and fixed 32-bit fields).
    int[] fieldNumbers =
        new int[] {
          TestPackedTypes.PACKED_INT32_FIELD_NUMBER,
          TestPackedTypes.PACKED_INT64_FIELD_NUMBER,
          TestPackedTypes.PACKED_UINT32_FIELD_NUMBER,
          TestPackedTypes.PACKED_UINT64_FIELD_NUMBER,
          TestPackedTypes.PACKED_SINT32_FIELD_NUMBER,
          TestPackedTypes.PACKED_SINT64_FIELD_NUMBER,
          TestPackedTypes.PACKED_FIXED32_FIELD_NUMBER,
          TestPackedTypes.PACKED_SFIXED32_FIELD_NUMBER,
          TestPackedTypes.PACKED_FLOAT_FIELD_NUMBER,
          TestPackedTypes.PACKED_BOOL_FIELD_NUMBER,
          TestPackedTypes.PACKED_ENUM_FIELD_NUMBER,
        };
    for (int number : fieldNumbers) {
      // Set the tag.
      data[0] =
          (byte) ((WireFormat.makeTag(number, WireFormat.WIRETYPE_LENGTH_DELIMITED) & 0x7F) | 0x80);
      data[1] =
          (byte) ((WireFormat.makeTag(number, WireFormat.WIRETYPE_LENGTH_DELIMITED) >>> 7) & 0x7F);
      assertInvalidWireFormat(TestPackedTypes.getDefaultInstance(), data);
    }

    // Data with 8-bytes payload to test some fixed 64-bit fields.
    byte[] data8Bytes =
        new byte[] {
          0,
          0, // first two bytes is reserved for the tag.
          1, // length is 1
          (byte) 0x80,
          (byte) 0x80,
          (byte) 0x80,
          (byte) 0x80,
          (byte) 0x80,
          (byte) 0x80,
          (byte) 0x80,
          (byte) 0x01, // a 8-bytes varint
        };
    // All fields that can only read 8-bytes data.
    int[] fieldNumbers8Bytes =
        new int[] {
          TestPackedTypes.PACKED_FIXED64_FIELD_NUMBER,
          TestPackedTypes.PACKED_SFIXED64_FIELD_NUMBER,
          TestPackedTypes.PACKED_DOUBLE_FIELD_NUMBER,
        };
    for (int number : fieldNumbers8Bytes) {
      // Set the tag.
      data8Bytes[0] =
          (byte) ((WireFormat.makeTag(number, WireFormat.WIRETYPE_LENGTH_DELIMITED) & 0x7F) | 0x80);
      data8Bytes[1] =
          (byte) ((WireFormat.makeTag(number, WireFormat.WIRETYPE_LENGTH_DELIMITED) >>> 7) & 0x7F);
      assertInvalidWireFormat(TestPackedTypes.getDefaultInstance(), data8Bytes);
    }
  }

  public void testParseVarintMinMax() throws Exception {
    TestAllTypes message =
        TestAllTypes.newBuilder()
            .setOptionalInt32(Integer.MIN_VALUE)
            .addRepeatedInt32(Integer.MAX_VALUE)
            .setOptionalInt64(Long.MIN_VALUE)
            .addRepeatedInt64(Long.MAX_VALUE)
            .build();
    TestAllTypes parsed = TestAllTypes.parseFrom(message.toByteArray());
    assertEquals(Integer.MIN_VALUE, parsed.getOptionalInt32());
    assertEquals(Integer.MAX_VALUE, parsed.getRepeatedInt32(0));
    assertEquals(Long.MIN_VALUE, parsed.getOptionalInt64());
    assertEquals(Long.MAX_VALUE, parsed.getRepeatedInt64(0));
  }

  public void testParseAllVarintBits() throws Exception {
    for (int i = 0; i < 32; i++) {
      final int value = 1 << i;
      TestAllTypes message = TestAllTypes.newBuilder().setOptionalInt32(value).build();
      TestAllTypes parsed = TestAllTypes.parseFrom(message.toByteArray());
      assertEquals(value, parsed.getOptionalInt32());
    }
    for (int i = 0; i < 64; i++) {
      final long value = 1L << i;
      TestAllTypes message = TestAllTypes.newBuilder().setOptionalInt64(value).build();
      TestAllTypes parsed = TestAllTypes.parseFrom(message.toByteArray());
      assertEquals(value, parsed.getOptionalInt64());
    }
  }

  public void testParseEmptyUnknownLengthDelimitedField() throws Exception {
    byte[] data =
        new byte[] {(byte) WireFormat.makeTag(1, WireFormat.WIRETYPE_LENGTH_DELIMITED), 0};
    TestAllTypes parsed = TestAllTypes.parseFrom(data);
    assertTrue(Arrays.equals(data, parsed.toByteArray()));
  }

  public void testParseEmptyString() throws Exception {
    TestAllTypes message = TestAllTypes.newBuilder().setOptionalString("").build();
    TestAllTypes parsed = TestAllTypes.parseFrom(message.toByteArray());
    assertEquals("", parsed.getOptionalString());
  }

  public void testParseEmptyStringProto3() throws Exception {
    TestAllTypes message = TestAllTypes.newBuilder().setOptionalString("").build();
    // Note that we are parsing from a proto2 proto to a proto3 proto because empty string field is
    // not serialized in proto3.
    UnittestProto3.TestAllTypes parsed =
        UnittestProto3.TestAllTypes.parseFrom(message.toByteArray());
    assertEquals("", parsed.getOptionalString());
  }

  public void testParseEmptyBytes() throws Exception {
    TestAllTypes message = TestAllTypes.newBuilder().setOptionalBytes(ByteString.EMPTY).build();
    TestAllTypes parsed = TestAllTypes.parseFrom(message.toByteArray());
    assertEquals(ByteString.EMPTY, parsed.getOptionalBytes());
  }

  public void testParseEmptyRepeatedStringField() throws Exception {
    TestAllTypes message =
        TestAllTypes.newBuilder()
            .addRepeatedString("")
            .addRepeatedString("")
            .addRepeatedString("0")
            .build();
    TestAllTypes parsed = TestAllTypes.parseFrom(message.toByteArray());
    assertEquals(3, parsed.getRepeatedStringCount());
    assertEquals("", parsed.getRepeatedString(0));
    assertEquals("", parsed.getRepeatedString(1));
    assertEquals("0", parsed.getRepeatedString(2));
  }

  public void testParseEmptyRepeatedStringFieldProto3() throws Exception {
    TestAllTypes message =
        TestAllTypes.newBuilder()
            .addRepeatedString("")
            .addRepeatedString("")
            .addRepeatedString("0")
            .addRepeatedBytes(ByteString.EMPTY)
            .build();
    UnittestProto3.TestAllTypes parsed =
        UnittestProto3.TestAllTypes.parseFrom(message.toByteArray());
    assertEquals(3, parsed.getRepeatedStringCount());
    assertEquals("", parsed.getRepeatedString(0));
    assertEquals("", parsed.getRepeatedString(1));
    assertEquals("0", parsed.getRepeatedString(2));
  }

  public void testParseEmptyRepeatedBytesField() throws Exception {
    ByteString oneByte = ByteString.copyFrom(new byte[] {1});
    TestAllTypes message =
        TestAllTypes.newBuilder()
            .addRepeatedBytes(ByteString.EMPTY)
            .addRepeatedBytes(ByteString.EMPTY)
            .addRepeatedBytes(oneByte)
            .build();
    TestAllTypes parsed = TestAllTypes.parseFrom(message.toByteArray());
    assertEquals(3, parsed.getRepeatedBytesCount());
    assertEquals(ByteString.EMPTY, parsed.getRepeatedBytes(0));
    assertEquals(ByteString.EMPTY, parsed.getRepeatedBytes(1));
    assertEquals(oneByte, parsed.getRepeatedBytes(2));
  }

  public void testSkipUnknownFieldInMessageSetItem() throws Exception {
    ByteArrayOutputStream output = new ByteArrayOutputStream();
    // MessageSet item's start tag.
    output.write((byte) WireFormat.MESSAGE_SET_ITEM_TAG);
    // Put all field types into the item.
    TestUtil.getAllSet().writeTo(output);
    // Closing the item with the real payload and closing tag.
    output.write(
        new byte[] {
          (byte) WireFormat.MESSAGE_SET_TYPE_ID_TAG,
          100, // TYPE_ID = 100
          (byte) WireFormat.MESSAGE_SET_MESSAGE_TAG,
          0, // empty payload
          (byte) WireFormat.MESSAGE_SET_ITEM_END_TAG,
        });
    byte[] data = output.toByteArray();
    TestMessageSet parsed = TestMessageSet.parseFrom(data);

    // Convert to RawMessageSet for inspection.
    RawMessageSet raw = RawMessageSet.parseFrom(parsed.toByteArray());
    assertEquals(1, raw.getItemCount());
    assertEquals(100, raw.getItem(0).getTypeId());
    assertEquals(0, raw.getItem(0).getMessage().size());
  }

  public void testProto2UnknownEnumValuesInOptionalField() throws Exception {
    // Proto2 doesn't allow setting unknown enum values so we use proto3 to build a message with
    // unknown enum values
    UnittestProto3.TestAllTypes message =
        UnittestProto3.TestAllTypes.newBuilder().setOptionalNestedEnumValue(4321).build();
    TestAllTypes parsed = TestAllTypes.parseFrom(message.toByteArray());
    assertFalse(parsed.hasOptionalNestedEnum());
    // Make sure unknown enum values are preserved.
    UnittestProto3.TestAllTypes actual =
        UnittestProto3.TestAllTypes.parseFrom(parsed.toByteArray());
    assertEquals(4321, actual.getOptionalNestedEnumValue());
  }

  public void testProto2UnknownEnumValuesInRepeatedField() throws Exception {
    // Proto2 doesn't allow setting unknown enum values so we use proto3 to build a message with
    // unknown enum values
    UnittestProto3.TestAllTypes message =
        UnittestProto3.TestAllTypes.newBuilder().addRepeatedNestedEnumValue(5432).build();
    TestAllTypes parsed = TestAllTypes.parseFrom(message.toByteArray());
    assertEquals(0, parsed.getRepeatedNestedEnumCount());
    // Make sure unknown enum values are preserved.
    UnittestProto3.TestAllTypes actual =
        UnittestProto3.TestAllTypes.parseFrom(parsed.toByteArray());
    assertEquals(1, actual.getRepeatedNestedEnumCount());
    assertEquals(5432, actual.getRepeatedNestedEnumValue(0));
  }

  public void testProto2UnknownEnumValuesInMapField() throws Exception {
    // Proto2 doesn't allow setting unknown enum values so we use proto3 to build a message with
    // unknown enum values
    TestMap message = TestMap.newBuilder().putInt32ToEnumFieldValue(1, 4321).build();
    MapForProto2TestProto.TestMap parsed =
        MapForProto2TestProto.TestMap.parseFrom(message.toByteArray());
    assertEquals(0, parsed.getInt32ToEnumFieldMap().size());
    // Make sure unknown enum values are preserved.
    TestMap actual = TestMap.parseFrom(parsed.toByteArray());
    assertEquals(1, actual.getInt32ToEnumFieldMap().size());
    assertEquals(4321, actual.getInt32ToEnumFieldValueOrThrow(1));
  }

  public void testProto2UnknownEnumValuesInOneof() throws Exception {
    // Proto2 doesn't allow setting unknown enum values so we use proto3 to build a message with
    // unknown enum values
    UnittestProto3.TestOneof2 message =
        UnittestProto3.TestOneof2.newBuilder().setFooEnumValue(1234).build();
    TestOneof2 parsed = TestOneof2.parseFrom(message.toByteArray());
    assertFalse(parsed.hasFooEnum());
    // Make sure unknown enum values are preserved.
    UnittestProto3.TestOneof2 actual = UnittestProto3.TestOneof2.parseFrom(parsed.toByteArray());
    assertEquals(1234, actual.getFooEnumValue());
  }

  public void testProto2UnknownEnumValuesInExtension() throws Exception {
    ExtensionRegistryLite extensionRegistry = TestUtilLite.getExtensionRegistryLite();
    // Raw bytes for "[.optional_foreign_enum_extension_lite]: 10"
    final byte[] rawBytes = new byte[]{-80, 1, 10};
    TestAllExtensionsLite testAllExtensionsLite =
        TestAllExtensionsLite.parseFrom(rawBytes, extensionRegistry);
    assertEquals(ForeignEnumLite.FOREIGN_LITE_FOO,
        testAllExtensionsLite.getExtension(optionalForeignEnumExtensionLite));
    final byte[] resultRawBytes = testAllExtensionsLite.toByteArray();
    assertEquals(rawBytes.length, resultRawBytes.length);
    for (int i = 0; i < rawBytes.length; i++) {
      assertEquals(rawBytes[i], resultRawBytes[i]);
    }
  }

  public void testProto3UnknownEnumValuesInOptionalField() throws Exception {
    UnittestProto3.TestAllTypes message =
        UnittestProto3.TestAllTypes.newBuilder().setOptionalNestedEnumValue(4321).build();
    UnittestProto3.TestAllTypes parsed =
        UnittestProto3.TestAllTypes.parseFrom(message.toByteArray());
    assertEquals(4321, parsed.getOptionalNestedEnumValue());
  }

  public void testProto3UnknownEnumValuesInRepeatedField() throws Exception {
    UnittestProto3.TestAllTypes message =
        UnittestProto3.TestAllTypes.newBuilder().addRepeatedNestedEnumValue(5432).build();
    UnittestProto3.TestAllTypes parsed =
        UnittestProto3.TestAllTypes.parseFrom(message.toByteArray());
    assertEquals(1, parsed.getRepeatedNestedEnumCount());
    assertEquals(5432, parsed.getRepeatedNestedEnumValue(0));
  }

  public void testProto3UnknownEnumValuesInMapField() throws Exception {
    TestMap message = TestMap.newBuilder().putInt32ToEnumFieldValue(1, 4321).build();
    TestMap parsed = TestMap.parseFrom(message.toByteArray());
    assertEquals(1, parsed.getInt32ToEnumFieldMap().size());
    assertEquals(4321, parsed.getInt32ToEnumFieldValueOrThrow(1));
  }

  public void testProto3UnknownEnumValuesInOneof() throws Exception {
    UnittestProto3.TestOneof2 message =
        UnittestProto3.TestOneof2.newBuilder().setFooEnumValue(1234).build();
    UnittestProto3.TestOneof2 parsed = UnittestProto3.TestOneof2.parseFrom(message.toByteArray());
    assertEquals(1234, parsed.getFooEnumValue());
  }

  public void testProto3MessageFieldMergeBehavior() throws Exception {
    UnittestProto3.NestedTestAllTypes message1 =
        UnittestProto3.NestedTestAllTypes.newBuilder()
            .setPayload(
                UnittestProto3.TestAllTypes.newBuilder()
                    .setOptionalInt32(1234)
                    .setOptionalInt64(5678))
            .build();
    UnittestProto3.NestedTestAllTypes message2 =
        UnittestProto3.NestedTestAllTypes.newBuilder()
            .setPayload(
                UnittestProto3.TestAllTypes.newBuilder()
                    .setOptionalInt32(4321)
                    .setOptionalUint32(8765))
            .build();

    UnittestProto3.NestedTestAllTypes merged =
        UnittestProto3.NestedTestAllTypes.newBuilder()
            .mergeFrom(message1.toByteArray())
            .mergeFrom(message2.toByteArray())
            .build();
    // Field values coming later in the stream override earlier values.
    assertEquals(4321, merged.getPayload().getOptionalInt32());
    // Field values present in either message should be present in the merged result.
    assertEquals(5678, merged.getPayload().getOptionalInt64());
    assertEquals(8765, merged.getPayload().getOptionalUint32());
  }

  public void testMergeFromPartialByteArray() throws Exception {
    byte[] data = TestUtil.getAllSet().toByteArray();
    byte[] dataWithPaddings = new byte[data.length + 2];
    System.arraycopy(data, 0, dataWithPaddings, 1, data.length);
    // Parsing will fail if the builder (or parser) interprets offset or length incorrectly.
    TestAllTypes.newBuilder().mergeFrom(dataWithPaddings, 1, data.length);
    TestAllTypes.parser().parseFrom(dataWithPaddings, 1, data.length);
  }
}
