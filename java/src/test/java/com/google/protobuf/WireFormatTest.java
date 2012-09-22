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

import junit.framework.TestCase;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.util.List;

import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestFieldOrderings;
import protobuf_unittest.UnittestProto.TestPackedExtensions;
import protobuf_unittest.UnittestProto.TestPackedTypes;
import protobuf_unittest.UnittestMset.TestMessageSet;
import protobuf_unittest.UnittestMset.RawMessageSet;
import protobuf_unittest.UnittestMset.TestMessageSetExtension1;
import protobuf_unittest.UnittestMset.TestMessageSetExtension2;
import com.google.protobuf.UnittestLite.TestAllExtensionsLite;
import com.google.protobuf.UnittestLite.TestPackedExtensionsLite;

/**
 * Tests related to parsing and serialization.
 *
 * @author kenton@google.com (Kenton Varda)
 */
public class WireFormatTest extends TestCase {
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

  public void testSerializationPackedWithoutGetSerializedSize()
      throws Exception {
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

    TestPackedTypes message2 = TestPackedTypes.parseFrom(
        outputStream.toByteArray());

    TestUtil.assertPackedFieldsSet(message2);
  }

  public void testSerializeExtensionsLite() throws Exception {
    // TestAllTypes and TestAllExtensions should have compatible wire formats,
    // so if we serialize a TestAllExtensions then parse it as TestAllTypes
    // it should work.

    TestAllExtensionsLite message = TestUtil.getAllLiteExtensionsSet();
    ByteString rawBytes = message.toByteString();
    assertEquals(rawBytes.size(), message.getSerializedSize());

    TestAllTypes message2 = TestAllTypes.parseFrom(rawBytes);

    TestUtil.assertAllFieldsSet(message2);
  }

  public void testSerializePackedExtensionsLite() throws Exception {
    // TestPackedTypes and TestPackedExtensions should have compatible wire
    // formats; check that they serialize to the same string.
    TestPackedExtensionsLite message = TestUtil.getLitePackedExtensionsSet();
    ByteString rawBytes = message.toByteString();

    TestPackedTypes message2 = TestUtil.getPackedSet();
    ByteString rawBytes2 = message2.toByteString();

    assertEquals(rawBytes, rawBytes2);
  }

  public void testParseExtensions() throws Exception {
    // TestAllTypes and TestAllExtensions should have compatible wire formats,
    // so if we serialize a TestAllTypes then parse it as TestAllExtensions
    // it should work.

    TestAllTypes message = TestUtil.getAllSet();
    ByteString rawBytes = message.toByteString();

    ExtensionRegistry registry = TestUtil.getExtensionRegistry();

    TestAllExtensions message2 =
      TestAllExtensions.parseFrom(rawBytes, registry);

    TestUtil.assertAllExtensionsSet(message2);
  }

  public void testParsePackedExtensions() throws Exception {
    // Ensure that packed extensions can be properly parsed.
    TestPackedExtensions message = TestUtil.getPackedExtensionsSet();
    ByteString rawBytes = message.toByteString();

    ExtensionRegistry registry = TestUtil.getExtensionRegistry();

    TestPackedExtensions message2 =
        TestPackedExtensions.parseFrom(rawBytes, registry);

    TestUtil.assertPackedExtensionsSet(message2);
  }

  public void testParseExtensionsLite() throws Exception {
    // TestAllTypes and TestAllExtensions should have compatible wire formats,
    // so if we serialize a TestAllTypes then parse it as TestAllExtensions
    // it should work.

    TestAllTypes message = TestUtil.getAllSet();
    ByteString rawBytes = message.toByteString();

    ExtensionRegistryLite registry_lite = TestUtil.getExtensionRegistryLite();

    TestAllExtensionsLite message2 =
      TestAllExtensionsLite.parseFrom(rawBytes, registry_lite);

    TestUtil.assertAllExtensionsSet(message2);

    // Try again using a full extension registry.
    ExtensionRegistry registry = TestUtil.getExtensionRegistry();

    TestAllExtensionsLite message3 =
      TestAllExtensionsLite.parseFrom(rawBytes, registry);

    TestUtil.assertAllExtensionsSet(message3);
  }

  public void testParsePackedExtensionsLite() throws Exception {
    // Ensure that packed extensions can be properly parsed.
    TestPackedExtensionsLite message = TestUtil.getLitePackedExtensionsSet();
    ByteString rawBytes = message.toByteString();

    ExtensionRegistryLite registry = TestUtil.getExtensionRegistryLite();

    TestPackedExtensionsLite message2 =
        TestPackedExtensionsLite.parseFrom(rawBytes, registry);

    TestUtil.assertPackedExtensionsSet(message2);
  }

  public void testExtensionsSerializedSize() throws Exception {
    assertEquals(TestUtil.getAllSet().getSerializedSize(),
                 TestUtil.getAllExtensionsSet().getSerializedSize());
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
    assertTrue(TestAllTypes.parseDelimitedFrom(input) == null);
  }

  private void assertFieldsInOrder(ByteString data) throws Exception {
    CodedInputStream input = data.newCodedInput();
    int previousTag = 0;

    while (true) {
      int tag = input.readTag();
      if (tag == 0) {
        break;
      }

      assertTrue(tag > previousTag);
      previousTag = tag;
      input.skipField(tag);
    }
  }

  public void testInterleavedFieldsAndExtensions() throws Exception {
    // Tests that fields are written in order even when extension ranges
    // are interleaved with field numbers.
    ByteString data =
      TestFieldOrderings.newBuilder()
        .setMyInt(1)
        .setMyString("foo")
        .setMyFloat(1.0F)
        .setExtension(UnittestProto.myExtensionInt, 23)
        .setExtension(UnittestProto.myExtensionString, "bar")
        .build().toByteString();
    assertFieldsInOrder(data);

    Descriptors.Descriptor descriptor = TestFieldOrderings.getDescriptor();
    ByteString dynamic_data =
      DynamicMessage.newBuilder(TestFieldOrderings.getDescriptor())
        .setField(descriptor.findFieldByName("my_int"), 1L)
        .setField(descriptor.findFieldByName("my_string"), "foo")
        .setField(descriptor.findFieldByName("my_float"), 1.0F)
        .setField(UnittestProto.myExtensionInt.getDescriptor(), 23)
        .setField(UnittestProto.myExtensionString.getDescriptor(), "bar")
        .build().toByteString();
    assertFieldsInOrder(dynamic_data);
  }

  private ExtensionRegistry getTestFieldOrderingsRegistry() {
    ExtensionRegistry result = ExtensionRegistry.newInstance();
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
      TestFieldOrderings.parseFrom(source.toByteString(),
                                   getTestFieldOrderingsRegistry());
    assertEquals(source, dest);
  }

  public void testParseMultipleExtensionRangesDynamic() throws Exception {
    // Same as above except with DynamicMessage.
    Descriptors.Descriptor descriptor = TestFieldOrderings.getDescriptor();
    DynamicMessage source =
      DynamicMessage.newBuilder(TestFieldOrderings.getDescriptor())
        .setField(descriptor.findFieldByName("my_int"), 1L)
        .setField(descriptor.findFieldByName("my_string"), "foo")
        .setField(descriptor.findFieldByName("my_float"), 1.0F)
        .setField(UnittestProto.myExtensionInt.getDescriptor(), 23)
        .setField(UnittestProto.myExtensionString.getDescriptor(), "bar")
        .build();
    DynamicMessage dest =
      DynamicMessage.parseFrom(descriptor, source.toByteString(),
                               getTestFieldOrderingsRegistry());
    assertEquals(source, dest);
  }

  private static final int UNKNOWN_TYPE_ID = 1550055;
  private static final int TYPE_ID_1 =
    TestMessageSetExtension1.getDescriptor().getExtensions().get(0).getNumber();
  private static final int TYPE_ID_2 =
    TestMessageSetExtension2.getDescriptor().getExtensions().get(0).getNumber();

  public void testSerializeMessageSetEagerly() throws Exception {
    testSerializeMessageSetWithFlag(true);
  }

  public void testSerializeMessageSetNotEagerly() throws Exception {
    testSerializeMessageSetWithFlag(false);
  }

  private void testSerializeMessageSetWithFlag(boolean eagerParsing)
      throws Exception {
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
        .setUnknownFields(
          UnknownFieldSet.newBuilder()
            .addField(UNKNOWN_TYPE_ID,
              UnknownFieldSet.Field.newBuilder()
                .addLengthDelimited(ByteString.copyFromUtf8("bar"))
                .build())
            .build())
        .build();

    ByteString data = messageSet.toByteString();

    // Parse back using RawMessageSet and check the contents.
    RawMessageSet raw = RawMessageSet.parseFrom(data);

    assertTrue(raw.getUnknownFields().asMap().isEmpty());

    assertEquals(3, raw.getItemCount());
    assertEquals(TYPE_ID_1, raw.getItem(0).getTypeId());
    assertEquals(TYPE_ID_2, raw.getItem(1).getTypeId());
    assertEquals(UNKNOWN_TYPE_ID, raw.getItem(2).getTypeId());

    TestMessageSetExtension1 message1 =
      TestMessageSetExtension1.parseFrom(
        raw.getItem(0).getMessage().toByteArray());
    assertEquals(123, message1.getI());

    TestMessageSetExtension2 message2 =
      TestMessageSetExtension2.parseFrom(
        raw.getItem(1).getMessage().toByteArray());
    assertEquals("foo", message2.getStr());

    assertEquals("bar", raw.getItem(2).getMessage().toStringUtf8());
  }

  public void testParseMessageSetEagerly() throws Exception {
    testParseMessageSetWithFlag(true);
  }

  public void testParseMessageSetNotEagerly()throws Exception {
    testParseMessageSetWithFlag(false);
  }

  private void testParseMessageSetWithFlag(boolean eagerParsing)
      throws Exception {
    ExtensionRegistryLite.setEagerlyParseMessageSets(eagerParsing);
    ExtensionRegistry extensionRegistry = ExtensionRegistry.newInstance();
    extensionRegistry.add(TestMessageSetExtension1.messageSetExtension);
    extensionRegistry.add(TestMessageSetExtension2.messageSetExtension);

    // Set up a RawMessageSet with two known messages and an unknown one.
    RawMessageSet raw =
      RawMessageSet.newBuilder()
        .addItem(
          RawMessageSet.Item.newBuilder()
            .setTypeId(TYPE_ID_1)
            .setMessage(
              TestMessageSetExtension1.newBuilder()
                .setI(123)
                .build().toByteString())
            .build())
        .addItem(
          RawMessageSet.Item.newBuilder()
            .setTypeId(TYPE_ID_2)
            .setMessage(
              TestMessageSetExtension2.newBuilder()
                .setStr("foo")
                .build().toByteString())
            .build())
        .addItem(
          RawMessageSet.Item.newBuilder()
            .setTypeId(UNKNOWN_TYPE_ID)
            .setMessage(ByteString.copyFromUtf8("bar"))
            .build())
        .build();

    ByteString data = raw.toByteString();

    // Parse as a TestMessageSet and check the contents.
    TestMessageSet messageSet =
      TestMessageSet.parseFrom(data, extensionRegistry);

    assertEquals(123, messageSet.getExtension(
      TestMessageSetExtension1.messageSetExtension).getI());
    assertEquals("foo", messageSet.getExtension(
      TestMessageSetExtension2.messageSetExtension).getStr());

    // Check for unknown field with type LENGTH_DELIMITED,
    //   number UNKNOWN_TYPE_ID, and contents "bar".
    UnknownFieldSet unknownFields = messageSet.getUnknownFields();
    assertEquals(1, unknownFields.asMap().size());
    assertTrue(unknownFields.hasField(UNKNOWN_TYPE_ID));

    UnknownFieldSet.Field field = unknownFields.getField(UNKNOWN_TYPE_ID);
    assertEquals(1, field.getLengthDelimitedList().size());
    assertEquals("bar", field.getLengthDelimitedList().get(0).toStringUtf8());
  }

  public void testParseMessageSetExtensionEagerly() throws Exception {
    testParseMessageSetExtensionWithFlag(true);
  }

  public void testParseMessageSetExtensionNotEagerly() throws Exception {
    testParseMessageSetExtensionWithFlag(false);
  }

  private void testParseMessageSetExtensionWithFlag(boolean eagerParsing)
      throws Exception {
    ExtensionRegistryLite.setEagerlyParseMessageSets(eagerParsing);
    ExtensionRegistry extensionRegistry = ExtensionRegistry.newInstance();
    extensionRegistry.add(TestMessageSetExtension1.messageSetExtension);

    // Set up a RawMessageSet with a known messages.
    int TYPE_ID_1 =
        TestMessageSetExtension1
            .getDescriptor().getExtensions().get(0).getNumber();
    RawMessageSet raw =
      RawMessageSet.newBuilder()
        .addItem(
          RawMessageSet.Item.newBuilder()
            .setTypeId(TYPE_ID_1)
            .setMessage(
              TestMessageSetExtension1.newBuilder()
                .setI(123)
                .build().toByteString())
            .build())
        .build();

    ByteString data = raw.toByteString();

    // Parse as a TestMessageSet and check the contents.
    TestMessageSet messageSet =
        TestMessageSet.parseFrom(data, extensionRegistry);
    assertEquals(123, messageSet.getExtension(
        TestMessageSetExtension1.messageSetExtension).getI());
  }

  public void testMergeLazyMessageSetExtensionEagerly() throws Exception {
    testMergeLazyMessageSetExtensionWithFlag(true);
  }

  public void testMergeLazyMessageSetExtensionNotEagerly() throws Exception {
    testMergeLazyMessageSetExtensionWithFlag(false);
  }

  private void testMergeLazyMessageSetExtensionWithFlag(boolean eagerParsing)
      throws Exception {
    ExtensionRegistryLite.setEagerlyParseMessageSets(eagerParsing);
    ExtensionRegistry extensionRegistry = ExtensionRegistry.newInstance();
    extensionRegistry.add(TestMessageSetExtension1.messageSetExtension);

    // Set up a RawMessageSet with a known messages.
    int TYPE_ID_1 =
        TestMessageSetExtension1
            .getDescriptor().getExtensions().get(0).getNumber();
    RawMessageSet raw =
      RawMessageSet.newBuilder()
        .addItem(
          RawMessageSet.Item.newBuilder()
            .setTypeId(TYPE_ID_1)
            .setMessage(
              TestMessageSetExtension1.newBuilder()
                .setI(123)
                .build().toByteString())
            .build())
        .build();

    ByteString data = raw.toByteString();

    // Parse as a TestMessageSet and store value into lazy field
    TestMessageSet messageSet =
        TestMessageSet.parseFrom(data, extensionRegistry);
    // Merge lazy field check the contents.
    messageSet =
        messageSet.toBuilder().mergeFrom(data, extensionRegistry).build();
    assertEquals(123, messageSet.getExtension(
        TestMessageSetExtension1.messageSetExtension).getI());
  }

  public void testMergeMessageSetExtensionEagerly() throws Exception {
    testMergeMessageSetExtensionWithFlag(true);
  }

  public void testMergeMessageSetExtensionNotEagerly() throws Exception {
    testMergeMessageSetExtensionWithFlag(false);
  }

  private void testMergeMessageSetExtensionWithFlag(boolean eagerParsing)
      throws Exception {
    ExtensionRegistryLite.setEagerlyParseMessageSets(eagerParsing);
    ExtensionRegistry extensionRegistry = ExtensionRegistry.newInstance();
    extensionRegistry.add(TestMessageSetExtension1.messageSetExtension);

    // Set up a RawMessageSet with a known messages.
    int TYPE_ID_1 =
        TestMessageSetExtension1
            .getDescriptor().getExtensions().get(0).getNumber();
    RawMessageSet raw =
      RawMessageSet.newBuilder()
        .addItem(
          RawMessageSet.Item.newBuilder()
            .setTypeId(TYPE_ID_1)
            .setMessage(
              TestMessageSetExtension1.newBuilder()
                .setI(123)
                .build().toByteString())
            .build())
        .build();

    // Serialize RawMessageSet unnormally (message value before type id)
    ByteString.CodedBuilder out = ByteString.newCodedBuilder(
        raw.getSerializedSize());
    CodedOutputStream output = out.getCodedOutput();
    List<RawMessageSet.Item> items = raw.getItemList();
    for (int i = 0; i < items.size(); i++) {
      RawMessageSet.Item item = items.get(i);
      output.writeTag(1, WireFormat.WIRETYPE_START_GROUP);
      output.writeBytes(3, item.getMessage());
      output.writeInt32(2, item.getTypeId());
      output.writeTag(1, WireFormat.WIRETYPE_END_GROUP);
    }
    ByteString data = out.build();

    // Merge bytes into TestMessageSet and check the contents.
    TestMessageSet messageSet =
        TestMessageSet.newBuilder().mergeFrom(data, extensionRegistry).build();
    assertEquals(123, messageSet.getExtension(
        TestMessageSetExtension1.messageSetExtension).getI());
  }
}
