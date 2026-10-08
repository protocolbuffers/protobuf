// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import com.google.protobuf.testing.Proto2TestingLite;
import com.google.protobuf.testing.Proto2TestingLite.Proto2MessageLite;
import com.google.protobuf.testing.Proto2TestingLite.Proto2MessageLite.TestEnum;
import com.google.protobuf.testing.Proto2TestingLite.Proto2MessageLiteWithExtensions;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class Proto2ExtensionLiteLookupSchemaTest {
  private byte[] data;
  private ExtensionRegistryLite extensionRegistry;

  @Before
  public void setup() {
    data = new Proto2MessageLiteFactory(10, 20, 1, 1).newMessage().toByteArray();
    extensionRegistry = ExtensionRegistryLite.newInstance();
    Proto2TestingLite.registerAllExtensions(extensionRegistry);
  }

  @Test
  public void testExtensions() throws Exception {
    Proto2MessageLiteWithExtensions base =
        Proto2MessageLiteWithExtensions.parseFrom(data, extensionRegistry);

    Proto2MessageLiteWithExtensions message =
        ExperimentalSerializationUtil.fromByteArray(
            data, Proto2MessageLiteWithExtensions.class, extensionRegistry);
    assertThat(message).isEqualTo(base);

    Proto2MessageLiteWithExtensions roundtripMessage =
        ExperimentalSerializationUtil.fromByteArray(
            message.toByteArray(), Proto2MessageLiteWithExtensions.class, extensionRegistry);
    assertThat(roundtripMessage).isEqualTo(base);
  }

  @Test
  public void testUnknownEnum() throws Exception {
    // Use unknown fields to hold invalid enum values.
    UnknownFieldSetLite unknowns = UnknownFieldSetLite.newInstance();
    final int outOfRange = 1000;
    assertThat(TestEnum.forNumber(outOfRange)).isNull();
    unknowns.storeField(
        WireFormat.makeTag(
            Proto2MessageLite.FIELD_ENUM_13_FIELD_NUMBER, WireFormat.WIRETYPE_VARINT),
        (long) outOfRange);
    unknowns.storeField(
        WireFormat.makeTag(
            Proto2MessageLite.FIELD_ENUM_LIST_30_FIELD_NUMBER, WireFormat.WIRETYPE_VARINT),
        (long) TestEnum.ONE_VALUE);
    unknowns.storeField(
        WireFormat.makeTag(
            Proto2MessageLite.FIELD_ENUM_LIST_30_FIELD_NUMBER, WireFormat.WIRETYPE_VARINT),
        (long) outOfRange);
    unknowns.storeField(
        WireFormat.makeTag(
            Proto2MessageLite.FIELD_ENUM_LIST_30_FIELD_NUMBER, WireFormat.WIRETYPE_VARINT),
        (long) TestEnum.TWO_VALUE);

    {
      // Construct a packed enum list.
      int packedSize =
          CodedOutputStream.computeUInt32SizeNoTag(TestEnum.ONE_VALUE)
              + CodedOutputStream.computeUInt32SizeNoTag(outOfRange)
              + CodedOutputStream.computeUInt32SizeNoTag(TestEnum.ONE_VALUE);
      ByteString.CodedBuilder packedBuilder = ByteString.newCodedBuilder(packedSize);
      CodedOutputStream packedOut = packedBuilder.getCodedOutput();
      packedOut.writeEnumNoTag(TestEnum.ONE_VALUE);
      packedOut.writeEnumNoTag(outOfRange);
      packedOut.writeEnumNoTag(TestEnum.TWO_VALUE);
      unknowns.storeField(
          WireFormat.makeTag(
              Proto2MessageLite.FIELD_ENUM_LIST_PACKED_44_FIELD_NUMBER,
              WireFormat.WIRETYPE_LENGTH_DELIMITED),
          packedBuilder.build());
    }
    int size = unknowns.getSerializedSize();
    byte[] output = new byte[size];
    CodedOutputStream codedOutput = CodedOutputStream.newInstance(output);
    unknowns.writeTo(codedOutput);
    codedOutput.flush();

    Proto2MessageLiteWithExtensions parsed =
        ExperimentalSerializationUtil.fromByteArray(
            output, Proto2MessageLiteWithExtensions.class, extensionRegistry);
    assertWithMessage("out-of-range singular enum should not be in message")
        .that(parsed.hasExtension(Proto2TestingLite.fieldEnum13))
        .isFalse();
    assertWithMessage("out-of-range repeated enum should not be in message")
        .that(parsed.getExtension(Proto2TestingLite.fieldEnumList30).size())
        .isEqualTo(2);
    assertThat(parsed.getExtension(Proto2TestingLite.fieldEnumList30, 0)).isEqualTo(TestEnum.ONE);
    assertThat(parsed.getExtension(Proto2TestingLite.fieldEnumList30, 1)).isEqualTo(TestEnum.TWO);
    assertWithMessage("out-of-range packed repeated enum should not be in message")
        .that(parsed.getExtension(Proto2TestingLite.fieldEnumListPacked44).size())
        .isEqualTo(2);
    assertThat(parsed.getExtension(Proto2TestingLite.fieldEnumListPacked44, 0))
        .isEqualTo(TestEnum.ONE);
    assertThat(parsed.getExtension(Proto2TestingLite.fieldEnumListPacked44, 1))
        .isEqualTo(TestEnum.TWO);
  }
}
