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

import com.google.protobuf.testing.Proto2Testing.Proto2Empty;
import com.google.protobuf.testing.Proto2Testing.Proto2Message;
import com.google.protobuf.testing.Proto2Testing.Proto2Message.TestEnum;
import com.google.protobuf.testing.Proto2Testing.Proto2MessageWithMaps;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.List;
import org.junit.Test;

/** Base class for tests using {@link Proto2Message}. */
public abstract class AbstractProto2SchemaTest extends AbstractSchemaTest<Proto2Message> {

  @Override
  protected Proto2MessageFactory messageFactory() {
    return new Proto2MessageFactory(10, 20, 2, 2);
  }

  @Test
  public void mergeOptionalMessageFields() throws Exception {
    Proto2Message message1 =
        newBuilder()
            .setFieldMessage10(newBuilder().setFieldInt643(123).clearFieldInt325().build())
            .build();
    Proto2Message message2 =
        newBuilder()
            .setFieldMessage10(newBuilder().clearFieldInt643().setFieldInt325(456).build())
            .build();
    Proto2Message message3 =
        newBuilder()
            .setFieldMessage10(newBuilder().setFieldInt643(789).clearFieldInt325().build())
            .build();
    ByteArrayOutputStream output = new ByteArrayOutputStream();
    message1.writeTo(output);
    message2.writeTo(output);
    message3.writeTo(output);
    byte[] data = output.toByteArray();

    Proto2Message merged = ExperimentalSerializationUtil.fromByteArray(data, Proto2Message.class);
    assertThat(merged.getFieldMessage10().getFieldInt643()).isEqualTo(789);
    assertThat(merged.getFieldMessage10().getFieldInt325()).isEqualTo(456);
  }

  @Test
  public void oneofFieldsShouldRoundtrip() throws IOException {
    roundtrip("Field 53", newBuilder().setFieldDouble53(100).build());
    roundtrip("Field 54", newBuilder().setFieldFloat54(100).build());
    roundtrip("Field 55", newBuilder().setFieldInt6455(100).build());
    roundtrip("Field 56", newBuilder().setFieldUint6456(100L).build());
    roundtrip("Field 57", newBuilder().setFieldInt3257(100).build());
    roundtrip("Field 58", newBuilder().setFieldFixed6458(100).build());
    roundtrip("Field 59", newBuilder().setFieldFixed3259(100).build());
    roundtrip("Field 60", newBuilder().setFieldBool60(true).build());
    roundtrip("Field 61", newBuilder().setFieldString61(data().getString()).build());
    roundtrip(
        "Field 62", newBuilder().setFieldMessage62(newBuilder().setFieldDouble1(100)).build());
    roundtrip("Field 63", newBuilder().setFieldBytes63(data().getBytes()).build());
    roundtrip("Field 64", newBuilder().setFieldUint3264(100).build());
    roundtrip("Field 65", newBuilder().setFieldSfixed3265(100).build());
    roundtrip("Field 66", newBuilder().setFieldSfixed6466(100).build());
    roundtrip("Field 67", newBuilder().setFieldSint3267(100).build());
    roundtrip("Field 68", newBuilder().setFieldSint6468(100).build());
    roundtrip(
        "Field 69",
        newBuilder()
            .setFieldGroup69(
                Proto2Message.FieldGroup69.newBuilder().setFieldInt3270(data().getInt()))
            .build());
  }

  private Proto2Message.Builder newBuilder() {
    return messageFactory().newMessage().toBuilder();
  }

  @Test
  public void mapsShouldRoundtrip() throws IOException {
    roundtrip(
        "Proto2MessageWithMaps",
        new Proto2MessageFactory(2, 10, 2, 2).newMessageWithMaps(),
        Protobuf.getInstance().schemaFor(Proto2MessageWithMaps.class));
  }

  @Test
  public void unknownFieldsUnrecognized() throws Exception {
    Proto2Message expectedMessage = messageFactory().newMessage();
    byte[] serializedBytes = expectedMessage.toByteArray();
    Proto2Empty empty =
        ExperimentalSerializationUtil.fromByteArray(serializedBytes, Proto2Empty.class);

    // Merge serialized bytes into an empty message, then reserialize and merge it to a new
    // Proto2Message. Make sure the two messages equal.
    byte[] roundtripBytes = ExperimentalSerializationUtil.toByteArray(empty);
    assertThat(serializedBytes).hasLength(roundtripBytes.length);
    Proto2Message roundtripMessage =
        ExperimentalSerializationUtil.fromByteArray(roundtripBytes, Proto2Message.class);
    assertThat(roundtripMessage).isEqualTo(expectedMessage);
  }

  @Test
  public void unknownEnum() throws IOException {
    // Use unknown fields to hold invalid enum values.
    UnknownFieldSetLite unknowns = UnknownFieldSetLite.newInstance();
    final int outOfRange = 1000;
    assertThat(TestEnum.forNumber(outOfRange)).isNull();
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

    Proto2Message parsed = ExperimentalSerializationUtil.fromByteArray(output, Proto2Message.class);
    assertWithMessage("out-of-range singular enum should not be in message")
        .that(parsed.hasFieldEnum13())
        .isFalse();
    {
      List<Long> singularEnum =
          parsed
              .getUnknownFields()
              .getField(Proto2Message.FIELD_ENUM_13_FIELD_NUMBER)
              .getVarintList();
      assertThat(singularEnum).hasSize(1);
      assertThat((Long) (long) outOfRange).isEqualTo(singularEnum.get(0));
    }
    {
      List<Long> repeatedEnum =
          parsed
              .getUnknownFields()
              .getField(Proto2Message.FIELD_ENUM_LIST_30_FIELD_NUMBER)
              .getVarintList();
      assertThat(repeatedEnum).hasSize(1);
      assertThat((Long) (long) outOfRange).isEqualTo(repeatedEnum.get(0));
    }
    {
      List<Long> packedRepeatedEnum =
          parsed
              .getUnknownFields()
              .getField(Proto2Message.FIELD_ENUM_LIST_PACKED_44_FIELD_NUMBER)
              .getVarintList();
      assertThat(packedRepeatedEnum).hasSize(1);
      assertThat((Long) (long) outOfRange).isEqualTo(packedRepeatedEnum.get(0));
    }
    assertWithMessage("out-of-range repeated enum should not be in message")
        .that(parsed.getFieldEnumList30Count())
        .isEqualTo(2);
    assertThat(parsed.getFieldEnumList30(0)).isEqualTo(TestEnum.ONE);
    assertThat(parsed.getFieldEnumList30(1)).isEqualTo(TestEnum.TWO);
    assertWithMessage("out-of-range packed repeated enum should not be in message")
        .that(parsed.getFieldEnumListPacked44Count())
        .isEqualTo(2);
    assertThat(parsed.getFieldEnumListPacked44(0)).isEqualTo(TestEnum.ONE);
    assertThat(parsed.getFieldEnumListPacked44(1)).isEqualTo(TestEnum.TWO);
  }

  @Override
  protected List<Proto2Message> newMessagesMissingRequiredFields() {
    return messageFactory().newMessagesMissingRequiredFields();
  }
}
