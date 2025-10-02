// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.testing.Proto2Testing.Proto2Message;
import com.google.protobuf.testing.Proto3Testing.Proto3Message;
import java.io.IOException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class CodedAdapterTest {

  @Test
  public void proto3Roundtrip() throws Exception {
    Proto3Message expected = new Proto3MessageFactory(5, 10, 2, 2).newMessage();
    byte[] expectedBytes = expected.toByteArray();

    // Deserialize with BinaryReader and verify that the message matches the original.
    Proto3Message result = fromByteArray(expectedBytes, Proto3Message.class);
    assertThat(result).isEqualTo(expected);

    // Now write it back out using BinaryWriter and verify the output length.
    byte[] actualBytes = toByteArray(result, expectedBytes.length);

    // Read back in the bytes and verify that it matches the original message.
    Proto3Message actual = Proto3Message.parseFrom(actualBytes);
    assertThat(actual).isEqualTo(expected);
  }

  @Test
  public void proto2Roundtrip() throws Exception {
    Proto2Message expected = new Proto2MessageFactory(5, 10, 2, 2).newMessage();
    byte[] expectedBytes = expected.toByteArray();

    // Deserialize with BinaryReader and verify that the message matches the original.
    Proto2Message result = fromByteArray(expectedBytes, Proto2Message.class);
    assertThat(result).isEqualTo(expected);

    // Now write it back out using BinaryWriter and verify the output length.
    byte[] actualBytes = toByteArray(result, expectedBytes.length);

    // Read back in the bytes and verify that it matches the original message.
    Proto2Message actual = Proto2Message.parseFrom(actualBytes);
    assertThat(actual).isEqualTo(expected);
  }

  public static <T> byte[] toByteArray(T msg, int size) throws Exception {
    Schema<T> schema = Protobuf.getInstance().schemaFor(msg);
    byte[] out = new byte[size];
    CodedOutputStreamWriter writer =
        CodedOutputStreamWriter.forCodedOutput(CodedOutputStream.newInstance(out));
    schema.writeTo(msg, writer);
    assertThat(writer.getTotalBytesWritten()).isEqualTo(out.length);
    return out;
  }

  public static <T> T fromByteArray(byte[] data, Class<T> messageType) {
    Schema<T> schema = Protobuf.getInstance().schemaFor(messageType);
    try {
      T msg = schema.newInstance();
      schema.mergeFrom(
          msg,
          CodedInputStreamReader.forCodedInput(CodedInputStream.newInstance(data)),
          ExtensionRegistryLite.EMPTY_REGISTRY_LITE);
      return msg;
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
  }
}
