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
import protobuf_unittest.UnittestProto.TestPackedTypes;

import junit.framework.TestCase;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Unit test for {@link CodedOutputStream}.
 *
 * @author kenton@google.com Kenton Varda
 */
public class CodedOutputStreamTest extends TestCase {
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

  /** Arrays.asList() does not work with arrays of primitives.  :( */
  private List<Byte> toList(byte[] bytes) {
    List<Byte> result = new ArrayList<Byte>();
    for (byte b : bytes) {
      result.add(b);
    }
    return result;
  }

  private void assertEqualBytes(byte[] a, byte[] b) {
    assertEquals(toList(a), toList(b));
  }

  /**
   * Writes the given value using writeRawVarint32() and writeRawVarint64() and
   * checks that the result matches the given bytes.
   */
  private void assertWriteVarint(byte[] data, long value) throws Exception {
    // Only do 32-bit write if the value fits in 32 bits.
    if ((value >>> 32) == 0) {
      ByteArrayOutputStream rawOutput = new ByteArrayOutputStream();
      CodedOutputStream output = CodedOutputStream.newInstance(rawOutput);
      output.writeRawVarint32((int) value);
      output.flush();
      assertEqualBytes(data, rawOutput.toByteArray());

      // Also try computing size.
      assertEquals(data.length,
                   CodedOutputStream.computeRawVarint32Size((int) value));
    }

    {
      ByteArrayOutputStream rawOutput = new ByteArrayOutputStream();
      CodedOutputStream output = CodedOutputStream.newInstance(rawOutput);
      output.writeRawVarint64(value);
      output.flush();
      assertEqualBytes(data, rawOutput.toByteArray());

      // Also try computing size.
      assertEquals(data.length,
                   CodedOutputStream.computeRawVarint64Size(value));
    }

    // Try different block sizes.
    for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
      // Only do 32-bit write if the value fits in 32 bits.
      if ((value >>> 32) == 0) {
        ByteArrayOutputStream rawOutput = new ByteArrayOutputStream();
        CodedOutputStream output =
          CodedOutputStream.newInstance(rawOutput, blockSize);
        output.writeRawVarint32((int) value);
        output.flush();
        assertEqualBytes(data, rawOutput.toByteArray());
      }

      {
        ByteArrayOutputStream rawOutput = new ByteArrayOutputStream();
        CodedOutputStream output =
          CodedOutputStream.newInstance(rawOutput, blockSize);
        output.writeRawVarint64(value);
        output.flush();
        assertEqualBytes(data, rawOutput.toByteArray());
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
    assertWriteVarint(bytes(0xbe, 0xf7, 0x92, 0x84, 0x0b),
      (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) |
      (0x0bL << 28));

    // 64-bit
    // 7256456126
    assertWriteVarint(bytes(0xbe, 0xf7, 0x92, 0x84, 0x1b),
      (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) |
      (0x1bL << 28));
    // 41256202580718336
    assertWriteVarint(
      bytes(0x80, 0xe6, 0xeb, 0x9c, 0xc3, 0xc9, 0xa4, 0x49),
      (0x00 << 0) | (0x66 << 7) | (0x6b << 14) | (0x1c << 21) |
      (0x43L << 28) | (0x49L << 35) | (0x24L << 42) | (0x49L << 49));
    // 11964378330978735131
    assertWriteVarint(
      bytes(0x9b, 0xa8, 0xf9, 0xc2, 0xbb, 0xd6, 0x80, 0x85, 0xa6, 0x01),
      (0x1b << 0) | (0x28 << 7) | (0x79 << 14) | (0x42 << 21) |
      (0x3bL << 28) | (0x56L << 35) | (0x00L << 42) |
      (0x05L << 49) | (0x26L << 56) | (0x01L << 63));
  }

  /**
   * Parses the given bytes using writeRawLittleEndian32() and checks
   * that the result matches the given value.
   */
  private void assertWriteLittleEndian32(byte[] data, int value)
                                         throws Exception {
    ByteArrayOutputStream rawOutput = new ByteArrayOutputStream();
    CodedOutputStream output = CodedOutputStream.newInstance(rawOutput);
    output.writeRawLittleEndian32(value);
    output.flush();
    assertEqualBytes(data, rawOutput.toByteArray());

    // Try different block sizes.
    for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
      rawOutput = new ByteArrayOutputStream();
      output = CodedOutputStream.newInstance(rawOutput, blockSize);
      output.writeRawLittleEndian32(value);
      output.flush();
      assertEqualBytes(data, rawOutput.toByteArray());
    }
  }

  /**
   * Parses the given bytes using writeRawLittleEndian64() and checks
   * that the result matches the given value.
   */
  private void assertWriteLittleEndian64(byte[] data, long value)
                                         throws Exception {
    ByteArrayOutputStream rawOutput = new ByteArrayOutputStream();
    CodedOutputStream output = CodedOutputStream.newInstance(rawOutput);
    output.writeRawLittleEndian64(value);
    output.flush();
    assertEqualBytes(data, rawOutput.toByteArray());

    // Try different block sizes.
    for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
      rawOutput = new ByteArrayOutputStream();
      output = CodedOutputStream.newInstance(rawOutput, blockSize);
      output.writeRawLittleEndian64(value);
      output.flush();
      assertEqualBytes(data, rawOutput.toByteArray());
    }
  }

  /** Tests writeRawLittleEndian32() and writeRawLittleEndian64(). */
  public void testWriteLittleEndian() throws Exception {
    assertWriteLittleEndian32(bytes(0x78, 0x56, 0x34, 0x12), 0x12345678);
    assertWriteLittleEndian32(bytes(0xf0, 0xde, 0xbc, 0x9a), 0x9abcdef0);

    assertWriteLittleEndian64(
      bytes(0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12),
      0x123456789abcdef0L);
    assertWriteLittleEndian64(
      bytes(0x78, 0x56, 0x34, 0x12, 0xf0, 0xde, 0xbc, 0x9a),
      0x9abcdef012345678L);
  }

  /** Test writing cached strings. */
  public void testWriteStringCached() throws IOException {
    final ByteArrayOutputStream output = new ByteArrayOutputStream();
    final CodedOutputStream stream = CodedOutputStream.newInstance(output);

    // Test writing a string that is not cached
    stream.writeStringCached(5, "hello", null);
    stream.flush();
    CodedInputStream in = CodedInputStream.newInstance(output.toByteArray());
    assertEquals(WireFormat.makeTag(5, WireFormat.WIRETYPE_LENGTH_DELIMITED),
                 in.readTag());
    assertEquals("hello", in.readString());

    // Write a cached string: the real string is ignored
    output.reset();
    stream.writeStringCached(5, "ignored", ByteString.copyFromUtf8("hello"));
    stream.flush();
    in = CodedInputStream.newInstance(output.toByteArray());
    assertEquals(WireFormat.makeTag(5, WireFormat.WIRETYPE_LENGTH_DELIMITED),
                 in.readTag());
    assertEquals("hello", in.readString());
  }

  /** Test encodeZigZag32() and encodeZigZag64(). */
  public void testEncodeZigZag() throws Exception {
    assertEquals(0, CodedOutputStream.encodeZigZag32( 0));
    assertEquals(1, CodedOutputStream.encodeZigZag32(-1));
    assertEquals(2, CodedOutputStream.encodeZigZag32( 1));
    assertEquals(3, CodedOutputStream.encodeZigZag32(-2));
    assertEquals(0x7FFFFFFE, CodedOutputStream.encodeZigZag32(0x3FFFFFFF));
    assertEquals(0x7FFFFFFF, CodedOutputStream.encodeZigZag32(0xC0000000));
    assertEquals(0xFFFFFFFE, CodedOutputStream.encodeZigZag32(0x7FFFFFFF));
    assertEquals(0xFFFFFFFF, CodedOutputStream.encodeZigZag32(0x80000000));

    assertEquals(0, CodedOutputStream.encodeZigZag64( 0));
    assertEquals(1, CodedOutputStream.encodeZigZag64(-1));
    assertEquals(2, CodedOutputStream.encodeZigZag64( 1));
    assertEquals(3, CodedOutputStream.encodeZigZag64(-2));
    assertEquals(0x000000007FFFFFFEL,
                 CodedOutputStream.encodeZigZag64(0x000000003FFFFFFFL));
    assertEquals(0x000000007FFFFFFFL,
                 CodedOutputStream.encodeZigZag64(0xFFFFFFFFC0000000L));
    assertEquals(0x00000000FFFFFFFEL,
                 CodedOutputStream.encodeZigZag64(0x000000007FFFFFFFL));
    assertEquals(0x00000000FFFFFFFFL,
                 CodedOutputStream.encodeZigZag64(0xFFFFFFFF80000000L));
    assertEquals(0xFFFFFFFFFFFFFFFEL,
                 CodedOutputStream.encodeZigZag64(0x7FFFFFFFFFFFFFFFL));
    assertEquals(0xFFFFFFFFFFFFFFFFL,
                 CodedOutputStream.encodeZigZag64(0x8000000000000000L));

    // Some easier-to-verify round-trip tests.  The inputs (other than 0, 1, -1)
    // were chosen semi-randomly via keyboard bashing.
    assertEquals(0,
      CodedOutputStream.encodeZigZag32(CodedInputStream.decodeZigZag32(0)));
    assertEquals(1,
      CodedOutputStream.encodeZigZag32(CodedInputStream.decodeZigZag32(1)));
    assertEquals(-1,
      CodedOutputStream.encodeZigZag32(CodedInputStream.decodeZigZag32(-1)));
    assertEquals(14927,
      CodedOutputStream.encodeZigZag32(CodedInputStream.decodeZigZag32(14927)));
    assertEquals(-3612,
      CodedOutputStream.encodeZigZag32(CodedInputStream.decodeZigZag32(-3612)));

    assertEquals(0,
      CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(0)));
    assertEquals(1,
      CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(1)));
    assertEquals(-1,
      CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(-1)));
    assertEquals(14927,
      CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(14927)));
    assertEquals(-3612,
      CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(-3612)));

    assertEquals(856912304801416L,
      CodedOutputStream.encodeZigZag64(
        CodedInputStream.decodeZigZag64(
          856912304801416L)));
    assertEquals(-75123905439571256L,
      CodedOutputStream.encodeZigZag64(
        CodedInputStream.decodeZigZag64(
          -75123905439571256L)));
  }

  /** Tests writing a whole message with every field type. */
  public void testWriteWholeMessage() throws Exception {
    TestAllTypes message = TestUtil.getAllSet();

    byte[] rawBytes = message.toByteArray();
    assertEqualBytes(TestUtil.getGoldenMessage().toByteArray(), rawBytes);

    // Try different block sizes.
    for (int blockSize = 1; blockSize < 256; blockSize *= 2) {
      ByteArrayOutputStream rawOutput = new ByteArrayOutputStream();
      CodedOutputStream output =
        CodedOutputStream.newInstance(rawOutput, blockSize);
      message.writeTo(output);
      output.flush();
      assertEqualBytes(rawBytes, rawOutput.toByteArray());
    }
  }

  /** Tests writing a whole message with every packed field type. Ensures the
   * wire format of packed fields is compatible with C++. */
  public void testWriteWholePackedFieldsMessage() throws Exception {
    TestPackedTypes message = TestUtil.getPackedSet();

    byte[] rawBytes = message.toByteArray();
    assertEqualBytes(TestUtil.getGoldenPackedFieldsMessage().toByteArray(),
                     rawBytes);
  }
}
