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

import junit.framework.TestCase;
import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestFieldOrderings;
import protobuf_unittest.UnittestMset.TestMessageSet;
import protobuf_unittest.UnittestMset.RawMessageSet;
import protobuf_unittest.UnittestMset.TestMessageSetExtension1;
import protobuf_unittest.UnittestMset.TestMessageSetExtension2;

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

  public void testSerializeExtensions() throws Exception {
    // TestAllTypes and TestAllExtensions should have compatible wire formats,
    // so if we serealize a TestAllExtensions then parse it as TestAllTypes
    // it should work.

    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    ByteString rawBytes = message.toByteString();
    assertEquals(rawBytes.size(), message.getSerializedSize());

    TestAllTypes message2 = TestAllTypes.parseFrom(rawBytes);

    TestUtil.assertAllFieldsSet(message2);
  }

  public void testParseExtensions() throws Exception {
    // TestAllTypes and TestAllExtensions should have compatible wire formats,
    // so if we serealize a TestAllTypes then parse it as TestAllExtensions
    // it should work.

    TestAllTypes message = TestUtil.getAllSet();
    ByteString rawBytes = message.toByteString();

    ExtensionRegistry registry = ExtensionRegistry.newInstance();
    TestUtil.registerAllExtensions(registry);
    registry = registry.getUnmodifiable();

    TestAllExtensions message2 =
      TestAllExtensions.parseFrom(rawBytes, registry);

    TestUtil.assertAllExtensionsSet(message2);
  }

  public void testExtensionsSerializedSize() throws Exception {
    assertEquals(TestUtil.getAllSet().getSerializedSize(),
                 TestUtil.getAllExtensionsSet().getSerializedSize());
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

  private static final int UNKNOWN_TYPE_ID = 1550055;
  private static final int TYPE_ID_1 =
    TestMessageSetExtension1.getDescriptor().getExtensions().get(0).getNumber();
  private static final int TYPE_ID_2 =
    TestMessageSetExtension2.getDescriptor().getExtensions().get(0).getNumber();

  public void testSerializeMessageSet() throws Exception {
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

  public void testParseMessageSet() throws Exception {
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
}

