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

import com.google.protobuf.testing.Proto2Testing.Proto2Message;
import com.google.protobuf.testing.Proto3Testing.Proto3Message;
import java.io.IOException;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class CodedAdapterTest {
  @Before
  public void setup() {
    TestSchemas.registerGenericProto2Schemas();

    Protobuf.getInstance()
        .registerSchemaOverride(Proto3Message.class, TestSchemas.genericProto3Schema);
  }

  @Test
  public void proto3Roundtrip() throws Exception {
    Proto3Message expected = new Proto3MessageFactory(5, 10, 2, 2).newMessage();
    byte[] expectedBytes = expected.toByteArray();

    // Deserialize with BinaryReader and verify that the message matches the original.
    Proto3Message result = fromByteArray(expectedBytes, Proto3Message.class);
    assertEquals(expected, result);

    // Now write it back out using BinaryWriter and verify the output length.
    byte[] actualBytes = toByteArray(result, expectedBytes.length);

    // Read back in the bytes and verify that it matches the original message.
    Proto3Message actual = Proto3Message.parseFrom(actualBytes);
    assertEquals(expected, actual);
  }

  @Test
  public void proto2Roundtrip() throws Exception {
    Proto2Message expected = new Proto2MessageFactory(5, 10, 2, 2).newMessage();
    byte[] expectedBytes = expected.toByteArray();

    // Deserialize with BinaryReader and verify that the message matches the original.
    Proto2Message result = fromByteArray(expectedBytes, Proto2Message.class);
    assertEquals(expected, result);

    // Now write it back out using BinaryWriter and verify the output length.
    byte[] actualBytes = toByteArray(result, expectedBytes.length);

    // Read back in the bytes and verify that it matches the original message.
    Proto2Message actual = Proto2Message.parseFrom(actualBytes);
    assertEquals(expected, actual);
  }

  public static <T> byte[] toByteArray(T msg, int size) throws Exception {
    Schema<T> schema = Protobuf.getInstance().schemaFor(msg);
    byte[] out = new byte[size];
    CodedOutputStreamWriter writer =
        CodedOutputStreamWriter.forCodedOutput(CodedOutputStream.newInstance(out));
    schema.writeTo(msg, writer);
    assertEquals(out.length, writer.getTotalBytesWritten());
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
