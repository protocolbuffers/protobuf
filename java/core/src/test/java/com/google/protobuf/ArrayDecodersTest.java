// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static org.junit.Assert.assertThrows;

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
    assertThrows(
        InvalidProtocolBufferException.class,
        () -> ArrayDecoders.decodeString(NEGATIVE_SIZE_0.toByteArray(), 0, registers));
  }

  @Test
  public void testException_decodeStringRequireUtf8() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () -> ArrayDecoders.decodeStringRequireUtf8(NEGATIVE_SIZE_0.toByteArray(), 0, registers));
  }

  @Test
  public void testException_decodeBytes() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () -> ArrayDecoders.decodeBytes(NEGATIVE_SIZE_0.toByteArray(), 0, registers));
  }

  @Test
  public void testException_decodeStringList_first() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodeStringList(
                TAG,
                NEGATIVE_SIZE_0.toByteArray(),
                0,
                NEGATIVE_SIZE_0.size(),
                new ProtobufArrayList<Object>(),
                registers));
  }

  @Test
  public void testException_decodeStringList_second() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodeStringList(
                TAG,
                NEGATIVE_SIZE_1.toByteArray(),
                0,
                NEGATIVE_SIZE_1.size(),
                new ProtobufArrayList<Object>(),
                registers));
  }

  @Test
  public void testException_decodeStringListRequireUtf8_first() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodeStringListRequireUtf8(
                TAG,
                NEGATIVE_SIZE_0.toByteArray(),
                0,
                NEGATIVE_SIZE_0.size(),
                new ProtobufArrayList<Object>(),
                registers));
  }

  @Test
  public void testException_decodeStringListRequireUtf8_second() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodeStringListRequireUtf8(
                TAG,
                NEGATIVE_SIZE_1.toByteArray(),
                0,
                NEGATIVE_SIZE_1.size(),
                new ProtobufArrayList<Object>(),
                registers));
  }

  @Test
  public void testException_decodeBytesList_first() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodeBytesList(
                TAG,
                NEGATIVE_SIZE_0.toByteArray(),
                0,
                NEGATIVE_SIZE_0.size(),
                new ProtobufArrayList<Object>(),
                registers));
  }

  @Test
  public void testException_decodeBytesList_second() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodeBytesList(
                TAG,
                NEGATIVE_SIZE_1.toByteArray(),
                0,
                NEGATIVE_SIZE_1.size(),
                new ProtobufArrayList<Object>(),
                registers));
  }

  @Test
  public void testException_decodeUnknownField() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodeUnknownField(
                TAG,
                NEGATIVE_SIZE_0.toByteArray(),
                0,
                NEGATIVE_SIZE_0.size(),
                UnknownFieldSetLite.newInstance(),
                registers));
  }

  @Test
  public void testDecodePackedFixed32List_negativeSize() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodePackedFixed32List(
                packedSizeBytesNoTag(-1), 0, new IntArrayList(), registers));
  }

  @Test
  public void testDecodePackedFixed32List_2gb_beyondEndOfArray() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodePackedFixed32List(
                packedSizeBytesNoTag(2_000_000_000), 0, new IntArrayList(), registers));
  }

  @Test
  public void testDecodePackedFixed64List_2gb_beyondEndOfArray() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodePackedFixed64List(
                packedSizeBytesNoTag(2_000_000_000), 0, new LongArrayList(), registers));
  }

  @Test
  public void testDecodePackedFloatList_2gb_beyondEndOfArray() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodePackedFloatList(
                packedSizeBytesNoTag(2_000_000_000), 0, new FloatArrayList(), registers));
  }

  @Test
  public void testDecodePackedDoubleList_2gb_beyondEndOfArray() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodePackedDoubleList(
                packedSizeBytesNoTag(2_000_000_000), 0, new DoubleArrayList(), registers));
  }

  @Test
  public void testDecodePackedFixed64List_negativeSize() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodePackedFixed64List(
                packedSizeBytesNoTag(-1), 0, new LongArrayList(), registers));
  }

  @Test
  public void testDecodePackedFloatList_negativeSize() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodePackedFloatList(
                packedSizeBytesNoTag(-1), 0, new FloatArrayList(), registers));
  }

  @Test
  public void testDecodePackedDoubleList_negativeSize() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodePackedDoubleList(
                packedSizeBytesNoTag(-1), 0, new DoubleArrayList(), registers));
  }

  @Test
  public void testDecodePackedBoolList_negativeSize() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodePackedBoolList(
                packedSizeBytesNoTag(-1), 0, new BooleanArrayList(), registers));
  }

  @Test
  public void testDecodePackedSInt32List_negativeSize() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodePackedSInt32List(
                packedSizeBytesNoTag(-1), 0, new IntArrayList(), registers));
  }

  @Test
  public void testDecodePackedSInt64List_negativeSize() {
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodePackedSInt64List(
                packedSizeBytesNoTag(-1), 0, new LongArrayList(), registers));
  }

  @Test
  public void testException_decodeHugeField() {
    byte[] badBytes =
        new byte[] {
          (byte) 0x80, (byte) 0xFF, (byte) 0xFF, (byte) 0xEF, 0x73, 0x74, 0x69, 0x6E, 0x67
        };
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodeUnknownField(
                TAG, badBytes, 0, badBytes.length, UnknownFieldSetLite.newInstance(), registers));

    assertThrows(
        InvalidProtocolBufferException.class,
        () -> ArrayDecoders.decodeBytes(badBytes, 0, registers));

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
    assertThrows(
        InvalidProtocolBufferException.class,
        () ->
            ArrayDecoders.decodeBytesList(
                TAG, badBytesList, 0, badBytes.length, new ProtobufArrayList<>(), registers));
  }

  // Encodes a single varint without a tag prefix.
  // For use when testing decoding of packed primitive lists.
  // e.g. size = -1 is not a proper byte size for a list.
  private static byte[] packedSizeBytesNoTag(int size) {
    try {
      ByteString.Output byteStringOutput = ByteString.newOutput();
      CodedOutputStream codedOutput = CodedOutputStream.newInstance(byteStringOutput);
      codedOutput.writeInt32NoTag(size);
      codedOutput.flush();
      return byteStringOutput.toByteString().toByteArray();
    } catch (IOException e) {
      throw new RuntimeException(e);
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
