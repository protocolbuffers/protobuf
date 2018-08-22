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

import com.google.protobuf.CodedOutputStream.OutOfSpaceException;
import protobuf_unittest.UnittestProto.SparseEnumMessage;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestPackedTypes;
import protobuf_unittest.UnittestProto.TestSparseEnum;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import junit.framework.TestCase;

/**
 * Unit test for {@link CodedOutputStream}.
 *
 * @author kenton@google.com Kenton Varda
 */
public class CodedOutputStreamTest extends TestCase {
  private interface Coder {
    CodedOutputStream stream();

    byte[] toByteArray();

    OutputType getOutputType();
  }

  private static final class OutputStreamCoder implements Coder {
    private final CodedOutputStream stream;
    private final ByteArrayOutputStream output;

    OutputStreamCoder(int size) {
      output = new ByteArrayOutputStream();
      stream = CodedOutputStream.newInstance(output, size);
    }

    @Override
    public CodedOutputStream stream() {
      return stream;
    }

    @Override
    public byte[] toByteArray() {
      return output.toByteArray();
    }

    @Override
    public OutputType getOutputType() {
      return OutputType.STREAM;
    }
  }

  private static final class ArrayCoder implements Coder {
    private final CodedOutputStream stream;
    private final byte[] bytes;

    ArrayCoder(int size) {
      bytes = new byte[size];
      stream = CodedOutputStream.newInstance(bytes);
    }

    @Override
    public CodedOutputStream stream() {
      return stream;
    }

    @Override
    public byte[] toByteArray() {
      return Arrays.copyOf(bytes, stream.getTotalBytesWritten());
    }

    @Override
    public OutputType getOutputType() {
      return OutputType.ARRAY;
    }
  }

  private static final class NioHeapCoder implements Coder {
    private final CodedOutputStream stream;
    private final ByteBuffer buffer;
    private final int initialPosition;

    NioHeapCoder(int size) {
      this(size, 0);
    }

    NioHeapCoder(int size, int initialPosition) {
      this.initialPosition = initialPosition;
      buffer = ByteBuffer.allocate(size);
      buffer.position(initialPosition);
      stream = CodedOutputStream.newInstance(buffer);
    }

    @Override
    public CodedOutputStream stream() {
      return stream;
    }

    @Override
    public byte[] toByteArray() {
      ByteBuffer dup = buffer.duplicate();
      dup.position(initialPosition);
      dup.limit(buffer.position());

      byte[] bytes = new byte[dup.remaining()];
      dup.get(bytes);
      return bytes;
    }

    @Override
    public OutputType getOutputType() {
      return OutputType.NIO_HEAP;
    }
  }

  private static final class NioDirectCoder implements Coder {
    private final int initialPosition;
    private final CodedOutputStream stream;
    private final ByteBuffer buffer;
    private final boolean unsafe;

    NioDirectCoder(int size, boolean unsafe) {
      this(size, 0, unsafe);
    }

    NioDirectCoder(int size, int initialPosition, boolean unsafe) {
      this.unsafe = unsafe;
      this.initialPosition = initialPosition;
      buffer = ByteBuffer.allocateDirect(size);
      buffer.position(initialPosition);
      stream =
          unsafe
              ? CodedOutputStream.newUnsafeInstance(buffer)
              : CodedOutputStream.newSafeInstance(buffer);
    }

    @Override
    public CodedOutputStream stream() {
      return stream;
    }

    @Override
    public byte[] toByteArray() {
      ByteBuffer dup = buffer.duplicate();
      dup.position(initialPosition);
      dup.limit(buffer.position());

      byte[] bytes = new byte[dup.remaining()];
      dup.get(bytes);
      return bytes;
    }

    @Override
    public OutputType getOutputType() {
      return unsafe ? OutputType.NIO_DIRECT_SAFE : OutputType.NIO_DIRECT_UNSAFE;
    }
  }

  private enum OutputType {
    ARRAY() {
      @Override
      Coder newCoder(int size) {
        return new ArrayCoder(size);
      }
    },
    NIO_HEAP() {
      @Override
      Coder newCoder(int size) {
        return new NioHeapCoder(size);
      }
    },
    NIO_DIRECT_SAFE() {
      @Override
      Coder newCoder(int size) {
        return new NioDirectCoder(size, false);
      }
    },
    NIO_DIRECT_UNSAFE() {
      @Override
      Coder newCoder(int size) {
        return new NioDirectCoder(size, true);
      }
    },
    STREAM() {
      @Override
      Coder newCoder(int size) {
        return new OutputStreamCoder(size);
      }
    };

    abstract Coder newCoder(int size);
  }

  /** Checks that invariants are maintained for varint round trip input and output. */
  public void testVarintRoundTrips() throws Exception {
    for (OutputType outputType : OutputType.values()) {
      assertVarintRoundTrip(outputType, 0L);
      for (int bits = 0; bits < 64; bits++) {
        long value = 1L << bits;
        assertVarintRoundTrip(outputType, value);
        assertVarintRoundTrip(outputType, value + 1);
        assertVarintRoundTrip(outputType, value - 1);
        assertVarintRoundTrip(outputType, -value);
      }
    }
  }

  /** Tests writeRawVarint32() and writeRawVarint64(). */
  public void testWriteVarint() throws Exception {
    assertWriteVarint(bytes(0x00), 0);
    assertWriteVarint(bytes(0x01), 1);
    assertWriteVarint(bytes(0x7f), 127);
    // 14882
    assertWriteVarint(bytes(0xa2, 0x74), (0x22 << 0) | (0x74 << 7));
    // 2961488830
    assertWriteVarint(
        bytes(0xbe, 0xf7, 0x92, 0x84, 0x0b),
        (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) | (0x0bL << 28));

    // 64-bit
    // 7256456126
    assertWriteVarint(
        bytes(0xbe, 0xf7, 0x92, 0x84, 0x1b),
        (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) | (0x1bL << 28));
    // 41256202580718336
    assertWriteVarint(
        bytes(0x80, 0xe6, 0xeb, 0x9c, 0xc3, 0xc9, 0xa4, 0x49),
        (0x00 << 0) | (0x66 << 7) | (0x6b << 14) | (0x1c << 21) | (0x43L << 28) | (0x49L << 35)
        | (0x24L << 42) | (0x49L << 49));
    // 11964378330978735131
    assertWriteVarint(
        bytes(0x9b, 0xa8, 0xf9, 0xc2, 0xbb, 0xd6, 0x80, 0x85, 0xa6, 0x01),
        (0x1b << 0) | (0x28 << 7) | (0x79 << 14) | (0x42 << 21) | (0x3bL << 28) | (0x56L << 35)
        | (0x00L << 42) | (0x05L << 49) | (0x26L << 56) | (0x01L << 63));
  }

  /** Tests writeRawLittleEndian32() and writeRawLittleEndian64(). */
  public void testWriteLittleEndian() throws Exception {
    assertWriteLittleEndian32(bytes(0x78, 0x56, 0x34, 0x12), 0x12345678);
    assertWriteLittleEndian32(bytes(0xf0, 0xde, 0xbc, 0x9a), 0x9abcdef0);

    assertWriteLittleEndian64(
        bytes(0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12), 0x123456789abcdef0L);
    assertWriteLittleEndian64(
        bytes(0x78, 0x56, 0x34, 0x12, 0xf0, 0xde, 0xbc, 0x9a), 0x9abcdef012345678L);
  }

  /** Test encodeZigZag32() and encodeZigZag64(). */
  public void testEncodeZigZag() throws Exception {
    assertEquals(0, CodedOutputStream.encodeZigZag32(0));
    assertEquals(1, CodedOutputStream.encodeZigZag32(-1));
    assertEquals(2, CodedOutputStream.encodeZigZag32(1));
    assertEquals(3, CodedOutputStream.encodeZigZag32(-2));
    assertEquals(0x7FFFFFFE, CodedOutputStream.encodeZigZag32(0x3FFFFFFF));
    assertEquals(0x7FFFFFFF, CodedOutputStream.encodeZigZag32(0xC0000000));
    assertEquals(0xFFFFFFFE, CodedOutputStream.encodeZigZag32(0x7FFFFFFF));
    assertEquals(0xFFFFFFFF, CodedOutputStream.encodeZigZag32(0x80000000));

    assertEquals(0, CodedOutputStream.encodeZigZag64(0));
    assertEquals(1, CodedOutputStream.encodeZigZag64(-1));
    assertEquals(2, CodedOutputStream.encodeZigZag64(1));
    assertEquals(3, CodedOutputStream.encodeZigZag64(-2));
    assertEquals(0x000000007FFFFFFEL, CodedOutputStream.encodeZigZag64(0x000000003FFFFFFFL));
    assertEquals(0x000000007FFFFFFFL, CodedOutputStream.encodeZigZag64(0xFFFFFFFFC0000000L));
    assertEquals(0x00000000FFFFFFFEL, CodedOutputStream.encodeZigZag64(0x000000007FFFFFFFL));
    assertEquals(0x00000000FFFFFFFFL, CodedOutputStream.encodeZigZag64(0xFFFFFFFF80000000L));
    assertEquals(0xFFFFFFFFFFFFFFFEL, CodedOutputStream.encodeZigZag64(0x7FFFFFFFFFFFFFFFL));
    assertEquals(0xFFFFFFFFFFFFFFFFL, CodedOutputStream.encodeZigZag64(0x8000000000000000L));

    // Some easier-to-verify round-trip tests.  The inputs (other than 0, 1, -1)
    // were chosen semi-randomly via keyboard bashing.
    assertEquals(0, CodedOutputStream.encodeZigZag32(CodedInputStream.decodeZigZag32(0)));
    assertEquals(1, CodedOutputStream.encodeZigZag32(CodedInputStream.decodeZigZag32(1)));
    assertEquals(-1, CodedOutputStream.encodeZigZag32(CodedInputStream.decodeZigZag32(-1)));
    assertEquals(14927, CodedOutputStream.encodeZigZag32(CodedInputStream.decodeZigZag32(14927)));
    assertEquals(-3612, CodedOutputStream.encodeZigZag32(CodedInputStream.decodeZigZag32(-3612)));

    assertEquals(0, CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(0)));
    assertEquals(1, CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(1)));
    assertEquals(-1, CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(-1)));
    assertEquals(14927, CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(14927)));
    assertEquals(-3612, CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(-3612)));

    assertEquals(
        856912304801416L,
        CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(856912304801416L)));
    assertEquals(
        -75123905439571256L,
        CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(-75123905439571256L)));
  }

  /** Tests writing a whole message with every field type. */
  public void testWriteWholeMessage() throws Exception {
    final byte[] expectedBytes = TestUtil.getGoldenMessage().toByteArray();
    TestAllTypes message = TestUtil.getAllSet();

    for (OutputType outputType : OutputType.values()) {
      Coder coder = outputType.newCoder(message.getSerializedSize());
      message.writeTo(coder.stream());
      coder.stream().flush();
      byte[] rawBytes = coder.toByteArray();
      assertEqualBytes(outputType, expectedBytes, rawBytes);
    }

    // Try different block sizes.
    for (int blockSize = 1; blockSize < 256; blockSize *= 2) {
      Coder coder = OutputType.STREAM.newCoder(blockSize);
      message.writeTo(coder.stream());
      coder.stream().flush();
      assertEqualBytes(OutputType.STREAM, expectedBytes, coder.toByteArray());
    }
  }

  /**
   * Tests writing a whole message with every packed field type. Ensures the
   * wire format of packed fields is compatible with C++.
   */
  public void testWriteWholePackedFieldsMessage() throws Exception {
    byte[] expectedBytes = TestUtil.getGoldenPackedFieldsMessage().toByteArray();
    TestPackedTypes message = TestUtil.getPackedSet();

    for (OutputType outputType : OutputType.values()) {
      Coder coder = outputType.newCoder(message.getSerializedSize());
      message.writeTo(coder.stream());
      coder.stream().flush();
      byte[] rawBytes = coder.toByteArray();
      assertEqualBytes(outputType, expectedBytes, rawBytes);
    }
  }

  /**
   * Test writing a message containing a negative enum value. This used to
   * fail because the size was not properly computed as a sign-extended varint.
   */
  public void testWriteMessageWithNegativeEnumValue() throws Exception {
    SparseEnumMessage message =
        SparseEnumMessage.newBuilder().setSparseEnum(TestSparseEnum.SPARSE_E).build();
    assertTrue(message.getSparseEnum().getNumber() < 0);
    for (OutputType outputType : OutputType.values()) {
      Coder coder = outputType.newCoder(message.getSerializedSize());
      message.writeTo(coder.stream());
      coder.stream().flush();
      byte[] rawBytes = coder.toByteArray();
      SparseEnumMessage message2 = SparseEnumMessage.parseFrom(rawBytes);
      assertEquals(TestSparseEnum.SPARSE_E, message2.getSparseEnum());
    }
  }

  /** Test getTotalBytesWritten() */
  public void testGetTotalBytesWritten() throws Exception {
    Coder coder = OutputType.STREAM.newCoder(4 * 1024);

    // Write some some bytes (more than the buffer can hold) and verify that totalWritten
    // is correct.
    byte[] value = "abcde".getBytes(Internal.UTF_8);
    for (int i = 0; i < 1024; ++i) {
      coder.stream().writeRawBytes(value, 0, value.length);
    }
    assertEquals(value.length * 1024, coder.stream().getTotalBytesWritten());

    // Now write an encoded string.
    String string =
        "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";
    // Ensure we take the slower fast path.
    assertTrue(CodedOutputStream.computeUInt32SizeNoTag(string.length())
        != CodedOutputStream.computeUInt32SizeNoTag(string.length() * Utf8.MAX_BYTES_PER_CHAR));

    coder.stream().writeStringNoTag(string);
    coder.stream().flush();
    int stringSize = CodedOutputStream.computeStringSizeNoTag(string);

    // Verify that the total bytes written is correct
    assertEquals((value.length * 1024) + stringSize, coder.stream().getTotalBytesWritten());
  }

  // TODO(dweis): Write a comprehensive test suite for CodedOutputStream that covers more than just
  //    this case.
  public void testWriteStringNoTag_fastpath() throws Exception {
    int bufferSize = 153;
    String threeBytesPer = "\u0981";
    String string = threeBytesPer;
    for (int i = 0; i < 50; i++) {
      string += threeBytesPer;
    }
    // These checks ensure we will tickle the slower fast path.
    assertEquals(1, CodedOutputStream.computeUInt32SizeNoTag(string.length()));
    assertEquals(
        2, CodedOutputStream.computeUInt32SizeNoTag(string.length() * Utf8.MAX_BYTES_PER_CHAR));
    assertEquals(bufferSize, string.length() * Utf8.MAX_BYTES_PER_CHAR);

    for (OutputType outputType : OutputType.values()) {
      Coder coder = outputType.newCoder(bufferSize + 2);
      coder.stream().writeStringNoTag(string);
      coder.stream().flush();
    }
  }

  public void testWriteToByteBuffer() throws Exception {
    final int bufferSize = 16 * 1024;
    ByteBuffer buffer = ByteBuffer.allocate(bufferSize);
    CodedOutputStream codedStream = CodedOutputStream.newInstance(buffer);
    // Write raw bytes into the ByteBuffer.
    final int length1 = 5000;
    for (int i = 0; i < length1; i++) {
      codedStream.writeRawByte((byte) 1);
    }
    final int length2 = 8 * 1024;
    byte[] data = new byte[length2];
    for (int i = 0; i < length2; i++) {
      data[i] = (byte) 2;
    }
    codedStream.writeRawBytes(data);
    final int length3 = bufferSize - length1 - length2;
    for (int i = 0; i < length3; i++) {
      codedStream.writeRawByte((byte) 3);
    }
    codedStream.flush();

    // Check that data is correctly written to the ByteBuffer.
    assertEquals(0, buffer.remaining());
    buffer.flip();
    for (int i = 0; i < length1; i++) {
      assertEquals((byte) 1, buffer.get());
    }
    for (int i = 0; i < length2; i++) {
      assertEquals((byte) 2, buffer.get());
    }
    for (int i = 0; i < length3; i++) {
      assertEquals((byte) 3, buffer.get());
    }
  }

  public void testWriteByteBuffer() throws Exception {
    byte[] value = "abcde".getBytes(Internal.UTF_8);
    ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
    CodedOutputStream codedStream = CodedOutputStream.newInstance(outputStream);
    ByteBuffer byteBuffer = ByteBuffer.wrap(value, 0, 1);
    // This will actually write 5 bytes into the CodedOutputStream as the
    // ByteBuffer's capacity() is 5.
    codedStream.writeRawBytes(byteBuffer);
    // The above call shouldn't affect the ByteBuffer's state.
    assertEquals(0, byteBuffer.position());
    assertEquals(1, byteBuffer.limit());

    // The correct way to write part of an array using ByteBuffer.
    codedStream.writeRawBytes(ByteBuffer.wrap(value, 2, 1).slice());

    codedStream.flush();
    byte[] result = outputStream.toByteArray();
    assertEquals(6, result.length);
    for (int i = 0; i < 5; i++) {
      assertEquals(value[i], result[i]);
    }
    assertEquals(value[2], result[5]);
  }

  public void testWriteByteArrayWithOffsets() throws Exception {
    byte[] fullArray = bytes(0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88);
    for (OutputType type : new OutputType[] {OutputType.ARRAY}) {
      Coder coder = type.newCoder(4);
      coder.stream().writeByteArrayNoTag(fullArray, 2, 2);
      assertEqualBytes(type, bytes(0x02, 0x33, 0x44), coder.toByteArray());
      assertEquals(3, coder.stream().getTotalBytesWritten());
    }
  }

  public void testSerializeUtf8_MultipleSmallWrites() throws Exception {
    final String source = "abcdefghijklmnopqrstuvwxyz";

    // Generate the expected output if the source string is written 2 bytes at a time.
    ByteArrayOutputStream expectedBytesStream = new ByteArrayOutputStream();
    for (int pos = 0; pos < source.length(); pos += 2) {
      String substr = source.substring(pos, pos + 2);
      expectedBytesStream.write(2);
      expectedBytesStream.write(substr.getBytes(Internal.UTF_8));
    }
    final byte[] expectedBytes = expectedBytesStream.toByteArray();

    // For each output type, write the source string 2 bytes at a time and verify the output.
    for (OutputType outputType : OutputType.values()) {
      Coder coder = outputType.newCoder(expectedBytes.length);
      for (int pos = 0; pos < source.length(); pos += 2) {
        String substr = source.substring(pos, pos + 2);
        coder.stream().writeStringNoTag(substr);
      }
      coder.stream().flush();
      assertEqualBytes(outputType, expectedBytes, coder.toByteArray());
    }
  }

  public void testSerializeInvalidUtf8() throws Exception {
    String[] invalidStrings = new String[] {newString(Character.MIN_HIGH_SURROGATE),
        "foobar" + newString(Character.MIN_HIGH_SURROGATE), newString(Character.MIN_LOW_SURROGATE),
        "foobar" + newString(Character.MIN_LOW_SURROGATE),
        newString(Character.MIN_HIGH_SURROGATE, Character.MIN_HIGH_SURROGATE)};

    CodedOutputStream outputWithStream = CodedOutputStream.newInstance(new ByteArrayOutputStream());
    CodedOutputStream outputWithArray = CodedOutputStream.newInstance(new byte[10000]);
    CodedOutputStream outputWithByteBuffer =
        CodedOutputStream.newInstance(ByteBuffer.allocate(10000));
    for (String s : invalidStrings) {
      // TODO(dweis): These should all fail; instead they are corrupting data.
      CodedOutputStream.computeStringSizeNoTag(s);
      outputWithStream.writeStringNoTag(s);
      outputWithArray.writeStringNoTag(s);
      outputWithByteBuffer.writeStringNoTag(s);
    }
  }

  // TODO(nathanmittler): This test can be deleted once we properly throw IOException while
  // encoding invalid UTF-8 strings.
  public void testSerializeInvalidUtf8FollowedByOutOfSpace() throws Exception {
    final int notEnoughBytes = 4;
    CodedOutputStream outputWithArray = CodedOutputStream.newInstance(new byte[notEnoughBytes]);
    CodedOutputStream outputWithByteBuffer =
        CodedOutputStream.newInstance(ByteBuffer.allocate(notEnoughBytes));

    String invalidString = newString(Character.MIN_HIGH_SURROGATE, 'f', 'o', 'o', 'b', 'a', 'r');
    try {
      outputWithArray.writeStringNoTag(invalidString);
      fail("Expected OutOfSpaceException");
    } catch (OutOfSpaceException e) {
      assertTrue(e.getCause() instanceof IndexOutOfBoundsException);
    }
    try {
      outputWithByteBuffer.writeStringNoTag(invalidString);
      fail("Expected OutOfSpaceException");
    } catch (OutOfSpaceException e) {
      assertTrue(e.getCause() instanceof IndexOutOfBoundsException);
    }
  }

  /** Regression test for https://github.com/protocolbuffers/protobuf/issues/292 */
  public void testCorrectExceptionThrowWhenEncodingStringsWithoutEnoughSpace() throws Exception {
    String testCase = "Foooooooo";
    assertEquals(
        CodedOutputStream.computeUInt32SizeNoTag(testCase.length()),
        CodedOutputStream.computeUInt32SizeNoTag(testCase.length() * 3));
    assertEquals(11, CodedOutputStream.computeStringSize(1, testCase));
    // Tag is one byte, varint describing string length is 1 byte, string length is 9 bytes.
    // An array of size 1 will cause a failure when trying to write the varint.
    for (OutputType outputType :
        new OutputType[] {
          OutputType.ARRAY,
          OutputType.NIO_HEAP,
          OutputType.NIO_DIRECT_SAFE,
          OutputType.NIO_DIRECT_UNSAFE
        }) {
      for (int i = 0; i < 11; i++) {
        Coder coder = outputType.newCoder(i);
        try {
          coder.stream().writeString(1, testCase);
          fail("Should have thrown an out of space exception");
        } catch (CodedOutputStream.OutOfSpaceException expected) {
        }
      }
    }
  }

  public void testDifferentStringLengths() throws Exception {
    // Test string serialization roundtrip using strings of the following lengths,
    // with ASCII and Unicode characters requiring different UTF-8 byte counts per
    // char, hence causing the length delimiter varint to sometimes require more
    // bytes for the Unicode strings than the ASCII string of the same length.
    int[] lengths = new int[] {
        0,
        1,
        (1 << 4) - 1, // 1 byte for ASCII and Unicode
        (1 << 7) - 1, // 1 byte for ASCII, 2 bytes for Unicode
        (1 << 11) - 1, // 2 bytes for ASCII and Unicode
        (1 << 14) - 1, // 2 bytes for ASCII, 3 bytes for Unicode
        (1 << 17) - 1,
        // 3 bytes for ASCII and Unicode
    };
    for (OutputType outputType : OutputType.values()) {
      for (int i : lengths) {
        testEncodingOfString(outputType, 'q', i); // 1 byte per char
        testEncodingOfString(outputType, '\u07FF', i); // 2 bytes per char
        testEncodingOfString(outputType, '\u0981', i); // 3 bytes per char
      }
    }
  }

  public void testNioEncodersWithInitialOffsets() throws Exception {
    String value = "abc";
    for (Coder coder :
        new Coder[] {
          new NioHeapCoder(10, 2), new NioDirectCoder(10, 2, false), new NioDirectCoder(10, 2, true)
        }) {
      coder.stream().writeStringNoTag(value);
      coder.stream().flush();
      assertEqualBytes(coder.getOutputType(), new byte[] {3, 'a', 'b', 'c'}, coder.toByteArray());
    }
  }

  /**
   * Parses the given bytes using writeRawLittleEndian32() and checks
   * that the result matches the given value.
   */
  private static void assertWriteLittleEndian32(byte[] data, int value) throws Exception {
    for (OutputType outputType : OutputType.values()) {
      Coder coder = outputType.newCoder(data.length);
      coder.stream().writeFixed32NoTag(value);
      coder.stream().flush();
      assertEqualBytes(outputType, data, coder.toByteArray());
    }

    // Try different block sizes.
    for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
      Coder coder = OutputType.STREAM.newCoder(blockSize);
      coder.stream().writeFixed32NoTag(value);
      coder.stream().flush();
      assertEqualBytes(OutputType.STREAM, data, coder.toByteArray());
    }
  }

  /**
   * Parses the given bytes using writeRawLittleEndian64() and checks
   * that the result matches the given value.
   */
  private static void assertWriteLittleEndian64(byte[] data, long value) throws Exception {
    for (OutputType outputType : OutputType.values()) {
      Coder coder = outputType.newCoder(data.length);
      coder.stream().writeFixed64NoTag(value);
      coder.stream().flush();
      assertEqualBytes(outputType, data, coder.toByteArray());
    }

    // Try different block sizes.
    for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
      Coder coder = OutputType.STREAM.newCoder(blockSize);
      coder.stream().writeFixed64NoTag(value);
      coder.stream().flush();
      assertEqualBytes(OutputType.STREAM, data, coder.toByteArray());
    }
  }

  private static String newString(char... chars) {
    return new String(chars);
  }

  private static void testEncodingOfString(OutputType outputType, char c, int length)
      throws Exception {
    String fullString = fullString(c, length);
    TestAllTypes testAllTypes = TestAllTypes.newBuilder().setOptionalString(fullString).build();
    Coder coder = outputType.newCoder(testAllTypes.getSerializedSize());
    testAllTypes.writeTo(coder.stream());
    coder.stream().flush();
    assertEquals(
        "OuputType: " + outputType,
        fullString,
        TestAllTypes.parseFrom(coder.toByteArray()).getOptionalString());
  }

  private static String fullString(char c, int length) {
    char[] result = new char[length];
    Arrays.fill(result, c);
    return new String(result);
  }

  /**
   * Helper to construct a byte array from a bunch of bytes.  The inputs are
   * actually ints so that I can use hex notation and not get stupid errors
   * about precision.
   */
  private static byte[] bytes(int... bytesAsInts) {
    byte[] bytes = new byte[bytesAsInts.length];
    for (int i = 0; i < bytesAsInts.length; i++) {
      bytes[i] = (byte) bytesAsInts[i];
    }
    return bytes;
  }

  /** Arrays.asList() does not work with arrays of primitives.  :( */
  private static List<Byte> toList(byte[] bytes) {
    List<Byte> result = new ArrayList<Byte>();
    for (byte b : bytes) {
      result.add(b);
    }
    return result;
  }

  private static void assertEqualBytes(OutputType outputType, byte[] a, byte[] b) {
    assertEquals(outputType.name(), toList(a), toList(b));
  }

  /**
   * Writes the given value using writeRawVarint32() and writeRawVarint64() and
   * checks that the result matches the given bytes.
   */
  private static void assertWriteVarint(byte[] data, long value) throws Exception {
    for (OutputType outputType : OutputType.values()) {
      // Only test 32-bit write if the value fits into an int.
      if (value == (int) value) {
        Coder coder = outputType.newCoder(10);
        coder.stream().writeUInt32NoTag((int) value);
        coder.stream().flush();
        assertEqualBytes(outputType, data, coder.toByteArray());

        // Also try computing size.
        assertEquals(data.length, CodedOutputStream.computeUInt32SizeNoTag((int) value));
      }

      {
        Coder coder = outputType.newCoder(10);
        coder.stream().writeUInt64NoTag(value);
        coder.stream().flush();
        assertEqualBytes(outputType, data, coder.toByteArray());

        // Also try computing size.
        assertEquals(data.length, CodedOutputStream.computeUInt64SizeNoTag(value));
      }
    }

    // Try different block sizes.
    for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
      // Only test 32-bit write if the value fits into an int.
      if (value == (int) value) {
        Coder coder = OutputType.STREAM.newCoder(blockSize);
        coder.stream().writeUInt64NoTag((int) value);
        coder.stream().flush();
        assertEqualBytes(OutputType.STREAM, data, coder.toByteArray());

        ByteArrayOutputStream rawOutput = new ByteArrayOutputStream();
        CodedOutputStream output = CodedOutputStream.newInstance(rawOutput, blockSize);
        output.writeUInt32NoTag((int) value);
        output.flush();
        assertEqualBytes(OutputType.STREAM, data, rawOutput.toByteArray());
      }

      {
        Coder coder = OutputType.STREAM.newCoder(blockSize);
        coder.stream().writeUInt64NoTag(value);
        coder.stream().flush();
        assertEqualBytes(OutputType.STREAM, data, coder.toByteArray());
      }
    }
  }

  private static void assertVarintRoundTrip(OutputType outputType, long value) throws Exception {
    {
      Coder coder = outputType.newCoder(10);
      coder.stream().writeUInt64NoTag(value);
      coder.stream().flush();
      byte[] bytes = coder.toByteArray();
      assertEquals(
          outputType.name(), bytes.length, CodedOutputStream.computeUInt64SizeNoTag(value));
      CodedInputStream input = CodedInputStream.newInstance(new ByteArrayInputStream(bytes));
      assertEquals(outputType.name(), value, input.readRawVarint64());
    }

    if (value == (int) value) {
      Coder coder = outputType.newCoder(10);
      coder.stream().writeUInt32NoTag((int) value);
      coder.stream().flush();
      byte[] bytes = coder.toByteArray();
      assertEquals(
          outputType.name(), bytes.length, CodedOutputStream.computeUInt32SizeNoTag((int) value));
      CodedInputStream input = CodedInputStream.newInstance(new ByteArrayInputStream(bytes));
      assertEquals(outputType.name(), value, input.readRawVarint32());
    }
  }
}
