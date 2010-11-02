// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestRecursiveMessage;

import junit.framework.TestCase;

import java.io.ByteArrayInputStream;
import java.io.FilterInputStream;
import java.io.InputStream;
import java.io.IOException;

/**
 * Unit test for {@link CodedInputStream}.
 *
 * @author kenton@google.com Kenton Varda
 */
public class CodedInputStreamTest extends TestCase {
  /**
   * Helper to construct a byte array from a bunch of bytes.  The inputs are
   * actually ints so that I can use hex notation and not get stupid errors
   * about precision.
   */
  private byte[] bytes(int... bytesAsInts) {
    byte[] bytes = new byte[bytesAsInts.length];
    for (int i = 0; i < bytesAsInts.length; i++) {
      bytes[i] = (byte) bytesAsInts[i];
    }
    return bytes;
  }

  /**
   * An InputStream which limits the number of bytes it reads at a time.
   * We use this to make sure that CodedInputStream doesn't screw up when
   * reading in small blocks.
   */
  private static final class SmallBlockInputStream extends FilterInputStream {
    private final int blockSize;

    public SmallBlockInputStream(byte[] data, int blockSize) {
      this(new ByteArrayInputStream(data), blockSize);
    }

    public SmallBlockInputStream(InputStream in, int blockSize) {
      super(in);
      this.blockSize = blockSize;
    }

    public int read(byte[] b) throws IOException {
      return super.read(b, 0, Math.min(b.length, blockSize));
    }

    public int read(byte[] b, int off, int len) throws IOException {
      return super.read(b, off, Math.min(len, blockSize));
    }
  }

  /**
   * Parses the given bytes using readRawVarint32() and readRawVarint64() and
   * checks that the result matches the given value.
   */
  private void assertReadVarint(byte[] data, long value) throws Exception {
    CodedInputStream input = CodedInputStream.newInstance(data);
    assertEquals((int)value, input.readRawVarint32());

    input = CodedInputStream.newInstance(data);
    assertEquals(value, input.readRawVarint64());
    assertTrue(input.isAtEnd());

    // Try different block sizes.
    for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
      input = CodedInputStream.newInstance(
        new SmallBlockInputStream(data, blockSize));
      assertEquals((int)value, input.readRawVarint32());

      input = CodedInputStream.newInstance(
        new SmallBlockInputStream(data, blockSize));
      assertEquals(value, input.readRawVarint64());
      assertTrue(input.isAtEnd());
    }

    // Try reading direct from an InputStream.  We want to verify that it
    // doesn't read past the end of the input, so we copy to a new, bigger
    // array first.
    byte[] longerData = new byte[data.length + 1];
    System.arraycopy(data, 0, longerData, 0, data.length);
    InputStream rawInput = new ByteArrayInputStream(longerData);
    assertEquals((int)value, CodedInputStream.readRawVarint32(rawInput));
    assertEquals(1, rawInput.available());
  }

  /**
   * Parses the given bytes using readRawVarint32() and readRawVarint64() and
   * expects them to fail with an InvalidProtocolBufferException whose
   * description matches the given one.
   */
  private void assertReadVarintFailure(
      InvalidProtocolBufferException expected, byte[] data)
      throws Exception {
    CodedInputStream input = CodedInputStream.newInstance(data);
    try {
      input.readRawVarint32();
      fail("Should have thrown an exception.");
    } catch (InvalidProtocolBufferException e) {
      assertEquals(expected.getMessage(), e.getMessage());
    }

    input = CodedInputStream.newInstance(data);
    try {
      input.readRawVarint64();
      fail("Should have thrown an exception.");
    } catch (InvalidProtocolBufferException e) {
      assertEquals(expected.getMessage(), e.getMessage());
    }

    // Make sure we get the same error when reading direct from an InputStream.
    try {
      CodedInputStream.readRawVarint32(new ByteArrayInputStream(data));
      fail("Should have thrown an exception.");
    } catch (InvalidProtocolBufferException e) {
      assertEquals(expected.getMessage(), e.getMessage());
    }
  }

  /** Tests readRawVarint32() and readRawVarint64(). */
  public void testReadVarint() throws Exception {
    assertReadVarint(bytes(0x00), 0);
    assertReadVarint(bytes(0x01), 1);
    assertReadVarint(bytes(0x7f), 127);
    // 14882
    assertReadVarint(bytes(0xa2, 0x74), (0x22 << 0) | (0x74 << 7));
    // 2961488830
    assertReadVarint(bytes(0xbe, 0xf7, 0x92, 0x84, 0x0b),
      (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) |
      (0x0bL << 28));

    // 64-bit
    // 7256456126
    assertReadVarint(bytes(0xbe, 0xf7, 0x92, 0x84, 0x1b),
      (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) |
      (0x1bL << 28));
    // 41256202580718336
    assertReadVarint(
      bytes(0x80, 0xe6, 0xeb, 0x9c, 0xc3, 0xc9, 0xa4, 0x49),
      (0x00 << 0) | (0x66 << 7) | (0x6b << 14) | (0x1c << 21) |
      (0x43L << 28) | (0x49L << 35) | (0x24L << 42) | (0x49L << 49));
    // 11964378330978735131
    assertReadVarint(
      bytes(0x9b, 0xa8, 0xf9, 0xc2, 0xbb, 0xd6, 0x80, 0x85, 0xa6, 0x01),
      (0x1b << 0) | (0x28 << 7) | (0x79 << 14) | (0x42 << 21) |
      (0x3bL << 28) | (0x56L << 35) | (0x00L << 42) |
      (0x05L << 49) | (0x26L << 56) | (0x01L << 63));

    // Failures
    assertReadVarintFailure(
      InvalidProtocolBufferException.malformedVarint(),
      bytes(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
            0x00));
    assertReadVarintFailure(
      InvalidProtocolBufferException.truncatedMessage(),
      bytes(0x80));
  }

  /**
   * Parses the given bytes using readRawLittleEndian32() and checks
   * that the result matches the given value.
   */
  private void assertReadLittleEndian32(byte[] data, int value)
                                        throws Exception {
    CodedInputStream input = CodedInputStream.newInstance(data);
    assertEquals(value, input.readRawLittleEndian32());
    assertTrue(input.isAtEnd());

    // Try different block sizes.
    for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
      input = CodedInputStream.newInstance(
        new SmallBlockInputStream(data, blockSize));
      assertEquals(value, input.readRawLittleEndian32());
      assertTrue(input.isAtEnd());
    }
  }

  /**
   * Parses the given bytes using readRawLittleEndian64() and checks
   * that the result matches the given value.
   */
  private void assertReadLittleEndian64(byte[] data, long value)
                                        throws Exception {
    CodedInputStream input = CodedInputStream.newInstance(data);
    assertEquals(value, input.readRawLittleEndian64());
    assertTrue(input.isAtEnd());

    // Try different block sizes.
    for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
      input = CodedInputStream.newInstance(
        new SmallBlockInputStream(data, blockSize));
      assertEquals(value, input.readRawLittleEndian64());
      assertTrue(input.isAtEnd());
    }
  }

  /** Tests readRawLittleEndian32() and readRawLittleEndian64(). */
  public void testReadLittleEndian() throws Exception {
    assertReadLittleEndian32(bytes(0x78, 0x56, 0x34, 0x12), 0x12345678);
    assertReadLittleEndian32(bytes(0xf0, 0xde, 0xbc, 0x9a), 0x9abcdef0);

    assertReadLittleEndian64(
      bytes(0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12),
      0x123456789abcdef0L);
    assertReadLittleEndian64(
      bytes(0x78, 0x56, 0x34, 0x12, 0xf0, 0xde, 0xbc, 0x9a),
      0x9abcdef012345678L);
  }

  /** Test decodeZigZag32() and decodeZigZag64(). */
  public void testDecodeZigZag() throws Exception {
    assertEquals( 0, CodedInputStream.decodeZigZag32(0));
    assertEquals(-1, CodedInputStream.decodeZigZag32(1));
    assertEquals( 1, CodedInputStream.decodeZigZag32(2));
    assertEquals(-2, CodedInputStream.decodeZigZag32(3));
    assertEquals(0x3FFFFFFF, CodedInputStream.decodeZigZag32(0x7FFFFFFE));
    assertEquals(0xC0000000, CodedInputStream.decodeZigZag32(0x7FFFFFFF));
    assertEquals(0x7FFFFFFF, CodedInputStream.decodeZigZag32(0xFFFFFFFE));
    assertEquals(0x80000000, CodedInputStream.decodeZigZag32(0xFFFFFFFF));

    assertEquals( 0, CodedInputStream.decodeZigZag64(0));
    assertEquals(-1, CodedInputStream.decodeZigZag64(1));
    assertEquals( 1, CodedInputStream.decodeZigZag64(2));
    assertEquals(-2, CodedInputStream.decodeZigZag64(3));
    assertEquals(0x000000003FFFFFFFL,
                 CodedInputStream.decodeZigZag64(0x000000007FFFFFFEL));
    assertEquals(0xFFFFFFFFC0000000L,
                 CodedInputStream.decodeZigZag64(0x000000007FFFFFFFL));
    assertEquals(0x000000007FFFFFFFL,
                 CodedInputStream.decodeZigZag64(0x00000000FFFFFFFEL));
    assertEquals(0xFFFFFFFF80000000L,
                 CodedInputStream.decodeZigZag64(0x00000000FFFFFFFFL));
    assertEquals(0x7FFFFFFFFFFFFFFFL,
                 CodedInputStream.decodeZigZag64(0xFFFFFFFFFFFFFFFEL));
    assertEquals(0x8000000000000000L,
                 CodedInputStream.decodeZigZag64(0xFFFFFFFFFFFFFFFFL));
  }

  /** Tests reading and parsing a whole message with every field type. */
  public void testReadWholeMessage() throws Exception {
    TestAllTypes message = TestUtil.getAllSet();

    byte[] rawBytes = message.toByteArray();
    assertEquals(rawBytes.length, message.getSerializedSize());

    TestAllTypes message2 = TestAllTypes.parseFrom(rawBytes);
    TestUtil.assertAllFieldsSet(message2);

    // Try different block sizes.
    for (int blockSize = 1; blockSize < 256; blockSize *= 2) {
      message2 = TestAllTypes.parseFrom(
        new SmallBlockInputStream(rawBytes, blockSize));
      TestUtil.assertAllFieldsSet(message2);
    }
  }

  /** Tests skipField(). */
  public void testSkipWholeMessage() throws Exception {
    TestAllTypes message = TestUtil.getAllSet();
    byte[] rawBytes = message.toByteArray();

    // Create two parallel inputs.  Parse one as unknown fields while using
    // skipField() to skip each field on the other.  Expect the same tags.
    CodedInputStream input1 = CodedInputStream.newInstance(rawBytes);
    CodedInputStream input2 = CodedInputStream.newInstance(rawBytes);
    UnknownFieldSet.Builder unknownFields = UnknownFieldSet.newBuilder();

    while (true) {
      int tag = input1.readTag();
      assertEquals(tag, input2.readTag());
      if (tag == 0) {
        break;
      }
      unknownFields.mergeFieldFrom(tag, input1);
      input2.skipField(tag);
    }
  }

  /**
   * Test that a bug in skipRawBytes() has been fixed:  if the skip skips
   * exactly up to a limit, this should not break things.
   */
  public void testSkipRawBytesBug() throws Exception {
    byte[] rawBytes = new byte[] { 1, 2 };
    CodedInputStream input = CodedInputStream.newInstance(rawBytes);

    int limit = input.pushLimit(1);
    input.skipRawBytes(1);
    input.popLimit(limit);
    assertEquals(2, input.readRawByte());
  }

  /**
   * Test that a bug in skipRawBytes() has been fixed:  if the skip skips
   * past the end of a buffer with a limit that has been set past the end of
   * that buffer, this should not break things.
   */
  public void testSkipRawBytesPastEndOfBufferWithLimit() throws Exception {
    byte[] rawBytes = new byte[] { 1, 2, 3, 4, 5 };
    CodedInputStream input = CodedInputStream.newInstance(
        new SmallBlockInputStream(rawBytes, 3));

    int limit = input.pushLimit(4);
    // In order to expose the bug we need to read at least one byte to prime the
    // buffer inside the CodedInputStream.
    assertEquals(1, input.readRawByte());
    // Skip to the end of the limit.
    input.skipRawBytes(3);
    assertTrue(input.isAtEnd());
    input.popLimit(limit);
    assertEquals(5, input.readRawByte());
  }

  public void testReadHugeBlob() throws Exception {
    // Allocate and initialize a 1MB blob.
    byte[] blob = new byte[1 << 20];
    for (int i = 0; i < blob.length; i++) {
      blob[i] = (byte)i;
    }

    // Make a message containing it.
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    builder.setOptionalBytes(ByteString.copyFrom(blob));
    TestAllTypes message = builder.build();

    // Serialize and parse it.  Make sure to parse from an InputStream, not
    // directly from a ByteString, so that CodedInputStream uses buffered
    // reading.
    TestAllTypes message2 =
      TestAllTypes.parseFrom(message.toByteString().newInput());

    assertEquals(message.getOptionalBytes(), message2.getOptionalBytes());

    // Make sure all the other fields were parsed correctly.
    TestAllTypes message3 = TestAllTypes.newBuilder(message2)
      .setOptionalBytes(TestUtil.getAllSet().getOptionalBytes())
      .build();
    TestUtil.assertAllFieldsSet(message3);
  }

  public void testReadMaliciouslyLargeBlob() throws Exception {
    ByteString.Output rawOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(rawOutput);

    int tag = WireFormat.makeTag(1, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    output.writeRawVarint32(tag);
    output.writeRawVarint32(0x7FFFFFFF);
    output.writeRawBytes(new byte[32]);  // Pad with a few random bytes.
    output.flush();

    CodedInputStream input = rawOutput.toByteString().newCodedInput();
    assertEquals(tag, input.readTag());

    try {
      input.readBytes();
      fail("Should have thrown an exception!");
    } catch (InvalidProtocolBufferException e) {
      // success.
    }
  }

  private TestRecursiveMessage makeRecursiveMessage(int depth) {
    if (depth == 0) {
      return TestRecursiveMessage.newBuilder().setI(5).build();
    } else {
      return TestRecursiveMessage.newBuilder()
        .setA(makeRecursiveMessage(depth - 1)).build();
    }
  }

  private void assertMessageDepth(TestRecursiveMessage message, int depth) {
    if (depth == 0) {
      assertFalse(message.hasA());
      assertEquals(5, message.getI());
    } else {
      assertTrue(message.hasA());
      assertMessageDepth(message.getA(), depth - 1);
    }
  }

  public void testMaliciousRecursion() throws Exception {
    ByteString data64 = makeRecursiveMessage(64).toByteString();
    ByteString data65 = makeRecursiveMessage(65).toByteString();

    assertMessageDepth(TestRecursiveMessage.parseFrom(data64), 64);

    try {
      TestRecursiveMessage.parseFrom(data65);
      fail("Should have thrown an exception!");
    } catch (InvalidProtocolBufferException e) {
      // success.
    }

    CodedInputStream input = data64.newCodedInput();
    input.setRecursionLimit(8);
    try {
      TestRecursiveMessage.parseFrom(input);
      fail("Should have thrown an exception!");
    } catch (InvalidProtocolBufferException e) {
      // success.
    }
  }

  public void testSizeLimit() throws Exception {
    CodedInputStream input = CodedInputStream.newInstance(
      TestUtil.getAllSet().toByteString().newInput());
    input.setSizeLimit(16);

    try {
      TestAllTypes.parseFrom(input);
      fail("Should have thrown an exception!");
    } catch (InvalidProtocolBufferException e) {
      // success.
    }
  }

  public void testResetSizeCounter() throws Exception {
    CodedInputStream input = CodedInputStream.newInstance(
        new SmallBlockInputStream(new byte[256], 8));
    input.setSizeLimit(16);
    input.readRawBytes(16);
    assertEquals(16, input.getTotalBytesRead());

    try {
      input.readRawByte();
      fail("Should have thrown an exception!");
    } catch (InvalidProtocolBufferException e) {
      // success.
    }

    input.resetSizeCounter();
    assertEquals(0, input.getTotalBytesRead());
    input.readRawByte();  // No exception thrown.
    input.resetSizeCounter();
    assertEquals(0, input.getTotalBytesRead());

    try {
      input.readRawBytes(16);  // Hits limit again.
      fail("Should have thrown an exception!");
    } catch (InvalidProtocolBufferException e) {
      // success.
    }
  }

  /**
   * Tests that if we read an string that contains invalid UTF-8, no exception
   * is thrown.  Instead, the invalid bytes are replaced with the Unicode
   * "replacement character" U+FFFD.
   */
  public void testReadInvalidUtf8() throws Exception {
    ByteString.Output rawOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(rawOutput);

    int tag = WireFormat.makeTag(1, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    output.writeRawVarint32(tag);
    output.writeRawVarint32(1);
    output.writeRawBytes(new byte[] { (byte)0x80 });
    output.flush();

    CodedInputStream input = rawOutput.toByteString().newCodedInput();
    assertEquals(tag, input.readTag());
    String text = input.readString();
    assertEquals(0xfffd, text.charAt(0));
  }

  public void testReadFromSlice() throws Exception {
    byte[] bytes = bytes(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    CodedInputStream in = CodedInputStream.newInstance(bytes, 3, 5);
    assertEquals(0, in.getTotalBytesRead());
    for (int i = 3; i < 8; i++) {
      assertEquals(i, in.readRawByte());
      assertEquals(i-2, in.getTotalBytesRead());
    }
    // eof
    assertEquals(0, in.readTag());
    assertEquals(5, in.getTotalBytesRead());
  }

  public void testInvalidTag() throws Exception {
    // Any tag number which corresponds to field number zero is invalid and
    // should throw InvalidProtocolBufferException.
    for (int i = 0; i < 8; i++) {
      try {
        CodedInputStream.newInstance(bytes(i)).readTag();
        fail("Should have thrown an exception.");
      } catch (InvalidProtocolBufferException e) {
        assertEquals(InvalidProtocolBufferException.invalidTag().getMessage(),
                     e.getMessage());
      }
    }
  }
}
