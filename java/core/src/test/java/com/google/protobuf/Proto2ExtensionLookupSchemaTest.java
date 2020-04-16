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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;

import com.google.protobuf.testing.Proto2Testing;
import com.google.protobuf.testing.Proto2Testing.Proto2Message;
import com.google.protobuf.testing.Proto2Testing.Proto2Message.TestEnum;
import com.google.protobuf.testing.Proto2Testing.Proto2MessageWithExtensions;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class Proto2ExtensionLookupSchemaTest {
  private byte[] data;
  private ExtensionRegistry extensionRegistry;

  @Before
  public void setup() {
    TestSchemas.registerGenericProto2Schemas();

    Protobuf.getInstance().schemaFor(Proto2MessageWithExtensions.class);
    data = new Proto2MessageFactory(10, 20, 1, 1).newMessage().toByteArray();
    extensionRegistry = ExtensionRegistry.newInstance();
    Proto2Testing.registerAllExtensions(extensionRegistry);
  }

  @Test
  public void testExtensions() throws Exception {
    Proto2MessageWithExtensions base =
        Proto2MessageWithExtensions.parseFrom(data, extensionRegistry);

    Proto2MessageWithExtensions message =
        ExperimentalSerializationUtil.fromByteArray(
            data, Proto2MessageWithExtensions.class, extensionRegistry);
    assertEquals(base, message);

    Proto2MessageWithExtensions roundtripMessage =
        ExperimentalSerializationUtil.fromByteArray(
            ExperimentalSerializationUtil.toByteArray(message),
            Proto2MessageWithExtensions.class,
            extensionRegistry);
    assertEquals(base, roundtripMessage);
  }

  @Test
  public void testUnknownEnum() throws Exception {
    // Use unknown fields to hold invalid enum values.
    UnknownFieldSetLite unknowns = UnknownFieldSetLite.newInstance();
    final int outOfRange = 1000;
    assertNull(TestEnum.forNumber(outOfRange));
    unknowns.storeField(
        WireFormat.makeTag(Proto2Message.FIELD_ENUM_13_FIELD_NUMBER, WireFormat.WIRETYPE_VARINT),
        (long) outOfRange);
    unknowns.storeField(
        WireFormat.makeTag(
            Proto2Message.FIELD_ENUM_LIST_30_FIELD_NUMBER, WireFormat.WIRETYPE_VARINT),
        (long) TestEnum.ONE_VALUE);
    unknowns.storeField(
        WireFormat.makeTag(
            Proto2Message.FIELD_ENUM_LIST_30_FIELD_NUMBER, WireFormat.WIRETYPE_VARINT),
        (long) outOfRange);
    unknowns.storeField(
        WireFormat.makeTag(
            Proto2Message.FIELD_ENUM_LIST_30_FIELD_NUMBER, WireFormat.WIRETYPE_VARINT),
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
              Proto2Message.FIELD_ENUM_LIST_PACKED_44_FIELD_NUMBER,
              WireFormat.WIRETYPE_LENGTH_DELIMITED),
          packedBuilder.build());
    }
    int size = unknowns.getSerializedSize();
    byte[] output = new byte[size];
    CodedOutputStream codedOutput = CodedOutputStream.newInstance(output);
    unknowns.writeTo(codedOutput);
    codedOutput.flush();

    Proto2MessageWithExtensions parsed =
        ExperimentalSerializationUtil.fromByteArray(
            output, Proto2MessageWithExtensions.class, extensionRegistry);
    assertFalse(
        "out-of-range singular enum should not be in message",
        parsed.hasExtension(Proto2Testing.fieldEnum13));
    {
      List<Long> singularEnum =
          parsed
              .getUnknownFields()
              .getField(Proto2Message.FIELD_ENUM_13_FIELD_NUMBER)
              .getVarintList();
      assertEquals(1, singularEnum.size());
      assertEquals((Long) (long) outOfRange, singularEnum.get(0));
    }
    {
      List<Long> repeatedEnum =
          parsed
              .getUnknownFields()
              .getField(Proto2Message.FIELD_ENUM_LIST_30_FIELD_NUMBER)
              .getVarintList();
      assertEquals(1, repeatedEnum.size());
      assertEquals((Long) (long) outOfRange, repeatedEnum.get(0));
    }
    {
      List<Long> packedRepeatedEnum =
          parsed
              .getUnknownFields()
              .getField(Proto2Message.FIELD_ENUM_LIST_PACKED_44_FIELD_NUMBER)
              .getVarintList();
      assertEquals(1, packedRepeatedEnum.size());
      assertEquals((Long) (long) outOfRange, packedRepeatedEnum.get(0));
    }
    assertEquals(
        "out-of-range repeated enum should not be in message",
        2,
        parsed.getExtension(Proto2Testing.fieldEnumList30).size());
    assertEquals(TestEnum.ONE, parsed.getExtension(Proto2Testing.fieldEnumList30, 0));
    assertEquals(TestEnum.TWO, parsed.getExtension(Proto2Testing.fieldEnumList30, 1));
    assertEquals(
        "out-of-range packed repeated enum should not be in message",
        2,
        parsed.getExtension(Proto2Testing.fieldEnumListPacked44).size());
    assertEquals(TestEnum.ONE, parsed.getExtension(Proto2Testing.fieldEnumListPacked44, 0));
    assertEquals(TestEnum.TWO, parsed.getExtension(Proto2Testing.fieldEnumListPacked44, 1));
  }
}
