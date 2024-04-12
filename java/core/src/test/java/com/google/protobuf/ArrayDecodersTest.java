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

import static com.google.common.truth.Truth.assertWithMessage;

import com.google.protobuf.ArrayDecoders.Registers;
import java.io.IOException;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class ArrayDecodersTest {

  private static final int TAG = WireFormat.makeTag(1, WireFormat.WIRETYPE_LENGTH_DELIMITED);
  private static final ByteString NEGATIVE_SIZE_0 = generateNegativeLength(0);
  private static final ByteString NEGATIVE_SIZE_1 = generateNegativeLength(1);

  private Registers registers;

  @Before
  public void setUp() {
    registers = new Registers();
    registers.int1 = TAG;
  }

  @Test
  public void testException_decodeString() {
    try {
      ArrayDecoders.decodeString(NEGATIVE_SIZE_0.toByteArray(), 0, registers);
      assertWithMessage("should throw exception").fail();
    } catch (InvalidProtocolBufferException expected) {
    }
  }

  @Test
  public void testException_decodeStringRequireUtf8() {
    try {
      ArrayDecoders.decodeStringRequireUtf8(NEGATIVE_SIZE_0.toByteArray(), 0, registers);
      assertWithMessage("should throw an exception").fail();
    } catch (InvalidProtocolBufferException expected) {
    }
  }

  @Test
  public void testException_decodeBytes() {
    try {
      ArrayDecoders.decodeBytes(NEGATIVE_SIZE_0.toByteArray(), 0, registers);
      assertWithMessage("should throw an exception").fail();
    } catch (InvalidProtocolBufferException expected) {
    }
  }

  @Test
  public void testException_decodeStringList_first() {
    try {
      ArrayDecoders.decodeStringList(
          TAG,
          NEGATIVE_SIZE_0.toByteArray(),
          0,
          NEGATIVE_SIZE_0.size(),
          new ProtobufArrayList<Object>(),
          registers);
      assertWithMessage("should throw an exception").fail();
    } catch (InvalidProtocolBufferException expected) {
    }
  }

  @Test
  public void testException_decodeStringList_second() {
    try {
      ArrayDecoders.decodeStringList(
          TAG,
          NEGATIVE_SIZE_1.toByteArray(),
          0,
          NEGATIVE_SIZE_1.size(),
          new ProtobufArrayList<Object>(),
          registers);
      assertWithMessage("should throw an exception").fail();
    } catch (InvalidProtocolBufferException expected) {
    }
  }

  @Test
  public void testException_decodeStringListRequireUtf8_first() {
    try {
      ArrayDecoders.decodeStringListRequireUtf8(
          TAG,
          NEGATIVE_SIZE_0.toByteArray(),
          0,
          NEGATIVE_SIZE_0.size(),
          new ProtobufArrayList<Object>(),
          registers);
      assertWithMessage("should throw an exception").fail();
    } catch (InvalidProtocolBufferException expected) {
    }
  }

  @Test
  public void testException_decodeStringListRequireUtf8_second() {
    try {
      ArrayDecoders.decodeStringListRequireUtf8(
          TAG,
          NEGATIVE_SIZE_1.toByteArray(),
          0,
          NEGATIVE_SIZE_1.size(),
          new ProtobufArrayList<Object>(),
          registers);
      assertWithMessage("should throw an exception").fail();
    } catch (InvalidProtocolBufferException expected) {
    }
  }

  @Test
  public void testException_decodeBytesList_first() {
    try {
      ArrayDecoders.decodeBytesList(
          TAG,
          NEGATIVE_SIZE_0.toByteArray(),
          0,
          NEGATIVE_SIZE_0.size(),
          new ProtobufArrayList<Object>(),
          registers);
      assertWithMessage("should throw an exception").fail();
    } catch (InvalidProtocolBufferException expected) {
    }
  }

  @Test
  public void testException_decodeBytesList_second() {
    try {
      ArrayDecoders.decodeBytesList(
          TAG,
          NEGATIVE_SIZE_1.toByteArray(),
          0,
          NEGATIVE_SIZE_1.size(),
          new ProtobufArrayList<Object>(),
          registers);
      assertWithMessage("should throw an exception").fail();
    } catch (InvalidProtocolBufferException expected) {
    }
  }

  @Test
  public void testException_decodeUnknownField() {
    try {
      ArrayDecoders.decodeUnknownField(
          TAG,
          NEGATIVE_SIZE_0.toByteArray(),
          0,
          NEGATIVE_SIZE_0.size(),
          UnknownFieldSetLite.newInstance(),
          registers);
      assertWithMessage("should throw an exception").fail();
    } catch (InvalidProtocolBufferException expected) {
    }
  }

  @Test
  public void testException_decodeHugeField() {
    byte[] badBytes =
        new byte[] {
          (byte) 0x80, (byte) 0xFF, (byte) 0xFF, (byte) 0xEF, 0x73, 0x74, 0x69, 0x6E, 0x67
        };
    try {
      ArrayDecoders.decodeUnknownField(
          TAG, badBytes, 0, badBytes.length, UnknownFieldSetLite.newInstance(), registers);
      assertWithMessage("should throw an exception").fail();
    } catch (InvalidProtocolBufferException expected) {
    }

    try {
      ArrayDecoders.decodeBytes(badBytes, 0, registers);
      assertWithMessage("should throw an exception").fail();
    } catch (InvalidProtocolBufferException expected) {
    }

    byte[] badBytesList =
        new byte[] {
          0x01,
          0x77,
          0x0A,
          (byte) 0x80,
          (byte) 0xFF,
          (byte) 0xFF,
          (byte) 0xEF,
          0x73,
          0x74,
          0x69,
          0x6E,
          0x67
        };
    try {
      ArrayDecoders.decodeBytesList(
          TAG, badBytesList, 0, badBytes.length, new ProtobufArrayList<>(), registers);
      assertWithMessage("should throw an exception").fail();
    } catch (InvalidProtocolBufferException expected) {
    }
  }

  private static ByteString generateNegativeLength(int count) {
    try {
      ByteString.Output byteStringOutput = ByteString.newOutput();
      CodedOutputStream codedOutput = CodedOutputStream.newInstance(byteStringOutput);

      // Write out count - 1 valid 0 length fields; we only write out tags after the field since
      // ArrayDecoders expects the first tag to already have been parsed.
      for (int i = 0; i < count; i++) {
        codedOutput.writeInt32NoTag(0);
        codedOutput.writeInt32NoTag(TAG);
      }

      // Write out a negative length
      codedOutput.writeInt32NoTag(-1);

      codedOutput.flush();

      return byteStringOutput.toByteString();
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
  }
}
