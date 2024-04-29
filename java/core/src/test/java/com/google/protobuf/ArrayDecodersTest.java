// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
