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

import com.google.protobuf.testing.Proto3TestingLite.Proto3EmptyLite;
import com.google.protobuf.testing.Proto3TestingLite.Proto3MessageLite;
import com.google.protobuf.testing.Proto3TestingLite.Proto3MessageLiteWithMaps;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;

/** Base class for tests using {@link Proto3MessageLite}. */
public abstract class AbstractProto3LiteSchemaTest extends AbstractSchemaTest<Proto3MessageLite> {
  @Override
  protected Proto3MessageLiteFactory messageFactory() {
    return new Proto3MessageLiteFactory(10, 20, 2, 2);
  }

  @Override
  protected List<ByteBuffer> serializedBytesWithInvalidUtf8() throws IOException {
    List<ByteBuffer> invalidBytes = new ArrayList<>();
    byte[] invalid = new byte[] {(byte) 0x80};
    {
      ByteBuffer buffer = ByteBuffer.allocate(100);
      CodedOutputStream codedOutput = CodedOutputStream.newInstance(buffer);
      codedOutput.writeByteArray(Proto3MessageLite.FIELD_STRING_9_FIELD_NUMBER, invalid);
      codedOutput.flush();
      buffer.flip();
      invalidBytes.add(buffer);
    }
    {
      ByteBuffer buffer = ByteBuffer.allocate(100);
      CodedOutputStream codedOutput = CodedOutputStream.newInstance(buffer);
      codedOutput.writeByteArray(Proto3MessageLite.FIELD_STRING_LIST_26_FIELD_NUMBER, invalid);
      codedOutput.flush();
      buffer.flip();
      invalidBytes.add(buffer);
    }
    return invalidBytes;
  }

  @Test
  public void mergeOptionalMessageFields() throws Exception {
    Proto3MessageLite message1 =
        newBuilder()
            .setFieldMessage10(newBuilder().setFieldInt643(123).clearFieldInt325().build())
            .build();
    Proto3MessageLite message2 =
        newBuilder()
            .setFieldMessage10(newBuilder().clearFieldInt643().setFieldInt325(456).build())
            .build();
    Proto3MessageLite message3 =
        newBuilder()
            .setFieldMessage10(newBuilder().setFieldInt643(789).clearFieldInt325().build())
            .build();
    ByteArrayOutputStream output = new ByteArrayOutputStream();
    message1.writeTo(output);
    message2.writeTo(output);
    message3.writeTo(output);
    byte[] data = output.toByteArray();

    Proto3MessageLite merged =
        ExperimentalSerializationUtil.fromByteArray(data, Proto3MessageLite.class);
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
  }

  @Test
  public void retainUnknownFields() {
    // Unknown fields are retained in lite runtime.
    Proto3MessageLite expectedMessage = messageFactory().newMessage();
    Proto3EmptyLite empty =
        ExperimentalSerializationUtil.fromByteArray(
            expectedMessage.toByteArray(), Proto3EmptyLite.class);
    assertThat(empty.getSerializedSize()).isEqualTo(expectedMessage.getSerializedSize());
  }

  @Test
  public void mapsShouldRoundtrip() throws IOException {
    roundtrip(
        "Proto3MessageLiteWithMaps",
        new Proto3MessageLiteFactory(2, 10, 2, 2).newMessageWithMaps(),
        Protobuf.getInstance().schemaFor(Proto3MessageLiteWithMaps.class));
  }

  private static Proto3MessageLite.Builder newBuilder() {
    return Proto3MessageLite.newBuilder();
  }
}
