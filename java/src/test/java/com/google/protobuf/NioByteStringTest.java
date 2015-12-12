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

import static com.google.protobuf.Internal.UTF_8;

import junit.framework.TestCase;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.UnsupportedEncodingException;
import java.nio.BufferOverflowException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;
import java.util.NoSuchElementException;

/**
 * Tests for {@link NioByteString}.
 */
public class NioByteStringTest extends TestCase {
  private static final ByteString EMPTY = UnsafeByteStrings.unsafeWrap(
      ByteBuffer.wrap(new byte[0]));
  private static final String CLASSNAME = NioByteString.class.getSimpleName();
  private static final byte[] BYTES = ByteStringTest.getTestBytes(1234, 11337766L);
  private static final int EXPECTED_HASH = new LiteralByteString(BYTES).hashCode();
  private static final ByteBuffer BUFFER = ByteBuffer.wrap(BYTES.clone());
  private static final ByteString TEST_STRING = UnsafeByteStrings.unsafeWrap(BUFFER);

  public void testExpectedType() {
    String actualClassName = getActualClassName(TEST_STRING);
    assertEquals(CLASSNAME + " should match type exactly", CLASSNAME, actualClassName);
  }

  protected String getActualClassName(Object object) {
    String actualClassName = object.getClass().getName();
    actualClassName = actualClassName.substring(actualClassName.lastIndexOf('.') + 1);
    return actualClassName;
  }

  public void testByteAt() {
    boolean stillEqual = true;
    for (int i = 0; stillEqual && i < BYTES.length; ++i) {
      stillEqual = (BYTES[i] == TEST_STRING.byteAt(i));
    }
    assertTrue(CLASSNAME + " must capture the right bytes", stillEqual);
  }

  public void testByteIterator() {
    boolean stillEqual = true;
    ByteString.ByteIterator iter = TEST_STRING.iterator();
    for (int i = 0; stillEqual && i < BYTES.length; ++i) {
      stillEqual = (iter.hasNext() && BYTES[i] == iter.nextByte());
    }
    assertTrue(CLASSNAME + " must capture the right bytes", stillEqual);
    assertFalse(CLASSNAME + " must have exhausted the itertor", iter.hasNext());

    try {
      iter.nextByte();
      fail("Should have thrown an exception.");
    } catch (NoSuchElementException e) {
      // This is success
    }
  }

  public void testByteIterable() {
    boolean stillEqual = true;
    int j = 0;
    for (byte quantum : TEST_STRING) {
      stillEqual = (BYTES[j] == quantum);
      ++j;
    }
    assertTrue(CLASSNAME + " must capture the right bytes as Bytes", stillEqual);
    assertEquals(CLASSNAME + " iterable character count", BYTES.length, j);
  }

  public void testSize() {
    assertEquals(CLASSNAME + " must have the expected size", BYTES.length,
        TEST_STRING.size());
  }

  public void testGetTreeDepth() {
    assertEquals(CLASSNAME + " must have depth 0", 0, TEST_STRING.getTreeDepth());
  }

  public void testIsBalanced() {
    assertTrue(CLASSNAME + " is technically balanced", TEST_STRING.isBalanced());
  }

  public void testCopyTo_ByteArrayOffsetLength() {
    int destinationOffset = 50;
    int length = 100;
    byte[] destination = new byte[destinationOffset + length];
    int sourceOffset = 213;
    TEST_STRING.copyTo(destination, sourceOffset, destinationOffset, length);
    boolean stillEqual = true;
    for (int i = 0; stillEqual && i < length; ++i) {
      stillEqual = BYTES[i + sourceOffset] == destination[i + destinationOffset];
    }
    assertTrue(CLASSNAME + ".copyTo(4 arg) must give the expected bytes", stillEqual);
  }

  public void testCopyTo_ByteArrayOffsetLengthErrors() {
    int destinationOffset = 50;
    int length = 100;
    byte[] destination = new byte[destinationOffset + length];

    try {
      // Copy one too many bytes
      TEST_STRING.copyTo(destination, TEST_STRING.size() + 1 - length,
          destinationOffset, length);
      fail("Should have thrown an exception when copying too many bytes of a "
          + CLASSNAME);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative sourceOffset
      TEST_STRING.copyTo(destination, -1, destinationOffset, length);
      fail("Should have thrown an exception when given a negative sourceOffset in "
          + CLASSNAME);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative destinationOffset
      TEST_STRING.copyTo(destination, 0, -1, length);
      fail("Should have thrown an exception when given a negative destinationOffset in "
          + CLASSNAME);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative size
      TEST_STRING.copyTo(destination, 0, 0, -1);
      fail("Should have thrown an exception when given a negative size in "
          + CLASSNAME);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal too-large sourceOffset
      TEST_STRING.copyTo(destination, 2 * TEST_STRING.size(), 0, length);
      fail("Should have thrown an exception when the destinationOffset is too large in "
          + CLASSNAME);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal too-large destinationOffset
      TEST_STRING.copyTo(destination, 0, 2 * destination.length, length);
      fail("Should have thrown an exception when the destinationOffset is too large in "
          + CLASSNAME);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }
  }

  public void testCopyTo_ByteBuffer() {
    // Same length.
    ByteBuffer myBuffer = ByteBuffer.allocate(BYTES.length);
    TEST_STRING.copyTo(myBuffer);
    myBuffer.flip();
    assertEquals(CLASSNAME + ".copyTo(ByteBuffer) must give back the same bytes",
        BUFFER, myBuffer);

    // Target buffer bigger than required.
    myBuffer = ByteBuffer.allocate(TEST_STRING.size() + 1);
    TEST_STRING.copyTo(myBuffer);
    myBuffer.flip();
    assertEquals(BUFFER, myBuffer);

    // Target buffer has no space.
    myBuffer = ByteBuffer.allocate(0);
    try {
      TEST_STRING.copyTo(myBuffer);
      fail("Should have thrown an exception when target ByteBuffer has insufficient capacity");
    } catch (BufferOverflowException e) {
      // Expected.
    }

    // Target buffer too small.
    myBuffer = ByteBuffer.allocate(1);
    try {
      TEST_STRING.copyTo(myBuffer);
      fail("Should have thrown an exception when target ByteBuffer has insufficient capacity");
    } catch (BufferOverflowException e) {
      // Expected.
    }
  }

  public void testMarkSupported() {
    InputStream stream = TEST_STRING.newInput();
    assertTrue(CLASSNAME + ".newInput() must support marking", stream.markSupported());
  }

  public void testMarkAndReset() throws IOException {
    int fraction = TEST_STRING.size() / 3;

    InputStream stream = TEST_STRING.newInput();
    stream.mark(TEST_STRING.size()); // First, mark() the end.

    skipFully(stream, fraction); // Skip a large fraction, but not all.
    assertEquals(
        CLASSNAME + ": after skipping to the 'middle', half the bytes are available",
        (TEST_STRING.size() - fraction), stream.available());
    stream.reset();
    assertEquals(
        CLASSNAME + ": after resetting, all bytes are available",
        TEST_STRING.size(), stream.available());

    skipFully(stream, TEST_STRING.size()); // Skip to the end.
    assertEquals(
        CLASSNAME + ": after skipping to the end, no more bytes are available",
        0, stream.available());
  }

  /**
   * Discards {@code n} bytes of data from the input stream. This method
   * will block until the full amount has been skipped. Does not close the
   * stream.
   * <p>Copied from com.google.common.io.ByteStreams to avoid adding dependency.
   *
   * @param in the input stream to read from
   * @param n the number of bytes to skip
   * @throws EOFException if this stream reaches the end before skipping all
   *     the bytes
   * @throws IOException if an I/O error occurs, or the stream does not
   *     support skipping
   */
  static void skipFully(InputStream in, long n) throws IOException {
    long toSkip = n;
    while (n > 0) {
      long amt = in.skip(n);
      if (amt == 0) {
        // Force a blocking read to avoid infinite loop
        if (in.read() == -1) {
          long skipped = toSkip - n;
          throw new EOFException("reached end of stream after skipping "
              + skipped + " bytes; " + toSkip + " bytes expected");
        }
        n--;
      } else {
        n -= amt;
      }
    }
  }

  public void testAsReadOnlyByteBuffer() {
    ByteBuffer byteBuffer = TEST_STRING.asReadOnlyByteBuffer();
    byte[] roundTripBytes = new byte[BYTES.length];
    assertTrue(byteBuffer.remaining() == BYTES.length);
    assertTrue(byteBuffer.isReadOnly());
    byteBuffer.get(roundTripBytes);
    assertTrue(CLASSNAME + ".asReadOnlyByteBuffer() must give back the same bytes",
        Arrays.equals(BYTES, roundTripBytes));
  }

  public void testAsReadOnlyByteBufferList() {
    List<ByteBuffer> byteBuffers = TEST_STRING.asReadOnlyByteBufferList();
    int bytesSeen = 0;
    byte[] roundTripBytes = new byte[BYTES.length];
    for (ByteBuffer byteBuffer : byteBuffers) {
      int thisLength = byteBuffer.remaining();
      assertTrue(byteBuffer.isReadOnly());
      assertTrue(bytesSeen + thisLength <= BYTES.length);
      byteBuffer.get(roundTripBytes, bytesSeen, thisLength);
      bytesSeen += thisLength;
    }
    assertTrue(bytesSeen == BYTES.length);
    assertTrue(CLASSNAME + ".asReadOnlyByteBufferTest() must give back the same bytes",
        Arrays.equals(BYTES, roundTripBytes));
  }

  public void testToByteArray() {
    byte[] roundTripBytes = TEST_STRING.toByteArray();
    assertTrue(CLASSNAME + ".toByteArray() must give back the same bytes",
        Arrays.equals(BYTES, roundTripBytes));
  }

  public void testWriteTo() throws IOException {
    ByteArrayOutputStream bos = new ByteArrayOutputStream();
    TEST_STRING.writeTo(bos);
    byte[] roundTripBytes = bos.toByteArray();
    assertTrue(CLASSNAME + ".writeTo() must give back the same bytes",
        Arrays.equals(BYTES, roundTripBytes));
  }

  public void testNewOutput() throws IOException {
    ByteArrayOutputStream bos = new ByteArrayOutputStream();
    ByteString.Output output = ByteString.newOutput();
    TEST_STRING.writeTo(output);
    assertEquals("Output Size returns correct result",
        output.size(), TEST_STRING.size());
    output.writeTo(bos);
    assertTrue("Output.writeTo() must give back the same bytes",
        Arrays.equals(BYTES, bos.toByteArray()));

    // write the output stream to itself! This should cause it to double
    output.writeTo(output);
    assertEquals("Writing an output stream to itself is successful",
        TEST_STRING.concat(TEST_STRING), output.toByteString());

    output.reset();
    assertEquals("Output.reset() resets the output", 0, output.size());
    assertEquals("Output.reset() resets the output",
        EMPTY, output.toByteString());
  }

  public void testToString() {
    String testString = "I love unicode \u1234\u5678 characters";
    ByteString unicode = forString(testString);
    String roundTripString = unicode.toString(UTF_8);
    assertEquals(CLASSNAME + " unicode must match", testString, roundTripString);
  }

  public void testCharsetToString() {
    String testString = "I love unicode \u1234\u5678 characters";
    ByteString unicode = forString(testString);
    String roundTripString = unicode.toString(UTF_8);
    assertEquals(CLASSNAME + " unicode must match", testString, roundTripString);
  }

  public void testToString_returnsCanonicalEmptyString() {
    assertSame(CLASSNAME + " must be the same string references",
        EMPTY.toString(UTF_8),
        UnsafeByteStrings.unsafeWrap(ByteBuffer.wrap(new byte[0])).toString(UTF_8));
  }

  public void testToString_raisesException() {
    try {
      EMPTY.toString("invalid");
      fail("Should have thrown an exception.");
    } catch (UnsupportedEncodingException expected) {
      // This is success
    }

    try {
      TEST_STRING.toString("invalid");
      fail("Should have thrown an exception.");
    } catch (UnsupportedEncodingException expected) {
      // This is success
    }
  }

  public void testEquals() {
    assertEquals(CLASSNAME + " must not equal null", false, TEST_STRING.equals(null));
    assertEquals(CLASSNAME + " must equal self", TEST_STRING, TEST_STRING);
    assertFalse(CLASSNAME + " must not equal the empty string",
        TEST_STRING.equals(EMPTY));
    assertEquals(CLASSNAME + " empty strings must be equal",
        EMPTY, TEST_STRING.substring(55, 55));
    assertEquals(CLASSNAME + " must equal another string with the same value",
        TEST_STRING, UnsafeByteStrings.unsafeWrap(BUFFER));

    byte[] mungedBytes = mungedBytes();
    assertFalse(CLASSNAME + " must not equal every string with the same length",
        TEST_STRING.equals(UnsafeByteStrings.unsafeWrap(ByteBuffer.wrap(mungedBytes))));
  }

  public void testEqualsLiteralByteString() {
    ByteString literal = ByteString.copyFrom(BYTES);
    assertEquals(CLASSNAME + " must equal LiteralByteString with same value", literal,
        TEST_STRING);
    assertEquals(CLASSNAME + " must equal LiteralByteString with same value", TEST_STRING,
        literal);
    assertFalse(CLASSNAME + " must not equal the empty string",
        TEST_STRING.equals(ByteString.EMPTY));
    assertEquals(CLASSNAME + " empty strings must be equal",
        ByteString.EMPTY, TEST_STRING.substring(55, 55));

    literal = ByteString.copyFrom(mungedBytes());
    assertFalse(CLASSNAME + " must not equal every LiteralByteString with the same length",
        TEST_STRING.equals(literal));
    assertFalse(CLASSNAME + " must not equal every LiteralByteString with the same length",
        literal.equals(TEST_STRING));
  }

  public void testEqualsRopeByteString() {
    ByteString p1 = ByteString.copyFrom(BYTES, 0, 5);
    ByteString p2 = ByteString.copyFrom(BYTES, 5, BYTES.length - 5);
    ByteString rope = p1.concat(p2);

    assertEquals(CLASSNAME + " must equal RopeByteString with same value", rope,
        TEST_STRING);
    assertEquals(CLASSNAME + " must equal RopeByteString with same value", TEST_STRING,
        rope);
    assertFalse(CLASSNAME + " must not equal the empty string",
        TEST_STRING.equals(ByteString.EMPTY.concat(ByteString.EMPTY)));
    assertEquals(CLASSNAME + " empty strings must be equal",
        ByteString.EMPTY.concat(ByteString.EMPTY), TEST_STRING.substring(55, 55));

    byte[] mungedBytes = mungedBytes();
    p1 = ByteString.copyFrom(mungedBytes, 0, 5);
    p2 = ByteString.copyFrom(mungedBytes, 5, mungedBytes.length - 5);
    rope = p1.concat(p2);
    assertFalse(CLASSNAME + " must not equal every RopeByteString with the same length",
        TEST_STRING.equals(rope));
    assertFalse(CLASSNAME + " must not equal every RopeByteString with the same length",
        rope.equals(TEST_STRING));
  }

  private byte[] mungedBytes() {
    byte[] mungedBytes = new byte[BYTES.length];
    System.arraycopy(BYTES, 0, mungedBytes, 0, BYTES.length);
    mungedBytes[mungedBytes.length - 5] = (byte) (mungedBytes[mungedBytes.length - 5] ^ 0xFF);
    return mungedBytes;
  }

  public void testHashCode() {
    int hash = TEST_STRING.hashCode();
    assertEquals(CLASSNAME + " must have expected hashCode", EXPECTED_HASH, hash);
  }

  public void testPeekCachedHashCode() {
    ByteString newString = UnsafeByteStrings.unsafeWrap(BUFFER);
    assertEquals(CLASSNAME + ".peekCachedHashCode() should return zero at first", 0,
        newString.peekCachedHashCode());
    newString.hashCode();
    assertEquals(CLASSNAME + ".peekCachedHashCode should return zero at first",
        EXPECTED_HASH, newString.peekCachedHashCode());
  }

  public void testPartialHash() {
    // partialHash() is more strenuously tested elsewhere by testing hashes of substrings.
    // This test would fail if the expected hash were 1.  It's not.
    int hash = TEST_STRING.partialHash(TEST_STRING.size(), 0, TEST_STRING.size());
    assertEquals(CLASSNAME + ".partialHash() must yield expected hashCode",
        EXPECTED_HASH, hash);
  }

  public void testNewInput() throws IOException {
    InputStream input = TEST_STRING.newInput();
    assertEquals("InputStream.available() returns correct value",
        TEST_STRING.size(), input.available());
    boolean stillEqual = true;
    for (byte referenceByte : BYTES) {
      int expectedInt = (referenceByte & 0xFF);
      stillEqual = (expectedInt == input.read());
    }
    assertEquals("InputStream.available() returns correct value",
        0, input.available());
    assertTrue(CLASSNAME + " must give the same bytes from the InputStream", stillEqual);
    assertEquals(CLASSNAME + " InputStream must now be exhausted", -1, input.read());
  }

  public void testNewInput_skip() throws IOException {
    InputStream input = TEST_STRING.newInput();
    int stringSize = TEST_STRING.size();
    int nearEndIndex = stringSize * 2 / 3;
    long skipped1 = input.skip(nearEndIndex);
    assertEquals("InputStream.skip()", skipped1, nearEndIndex);
    assertEquals("InputStream.available()",
        stringSize - skipped1, input.available());
    assertTrue("InputStream.mark() is available", input.markSupported());
    input.mark(0);
    assertEquals("InputStream.skip(), read()",
        TEST_STRING.byteAt(nearEndIndex) & 0xFF, input.read());
    assertEquals("InputStream.available()",
        stringSize - skipped1 - 1, input.available());
    long skipped2 = input.skip(stringSize);
    assertEquals("InputStream.skip() incomplete",
        skipped2, stringSize - skipped1 - 1);
    assertEquals("InputStream.skip(), no more input", 0, input.available());
    assertEquals("InputStream.skip(), no more input", -1, input.read());
    input.reset();
    assertEquals("InputStream.reset() succeded",
        stringSize - skipped1, input.available());
    assertEquals("InputStream.reset(), read()",
        TEST_STRING.byteAt(nearEndIndex) & 0xFF, input.read());
  }

  public void testNewCodedInput() throws IOException {
    CodedInputStream cis = TEST_STRING.newCodedInput();
    byte[] roundTripBytes = cis.readRawBytes(BYTES.length);
    assertTrue(CLASSNAME + " must give the same bytes back from the CodedInputStream",
        Arrays.equals(BYTES, roundTripBytes));
    assertTrue(CLASSNAME + " CodedInputStream must now be exhausted", cis.isAtEnd());
  }

  /**
   * Make sure we keep things simple when concatenating with empty. See also
   * {@link ByteStringTest#testConcat_empty()}.
   */
  public void testConcat_empty() {
    assertSame(CLASSNAME + " concatenated with empty must give " + CLASSNAME,
        TEST_STRING.concat(EMPTY), TEST_STRING);
    assertSame("empty concatenated with " + CLASSNAME + " must give " + CLASSNAME,
        EMPTY.concat(TEST_STRING), TEST_STRING);
  }

  public void testJavaSerialization() throws Exception {
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    ObjectOutputStream oos = new ObjectOutputStream(out);
    oos.writeObject(TEST_STRING);
    oos.close();
    byte[] pickled = out.toByteArray();
    InputStream in = new ByteArrayInputStream(pickled);
    ObjectInputStream ois = new ObjectInputStream(in);
    Object o = ois.readObject();
    assertTrue("Didn't get a ByteString back", o instanceof ByteString);
    assertEquals("Should get an equal ByteString back", TEST_STRING, o);
  }

  private static ByteString forString(String str) {
    return UnsafeByteStrings.unsafeWrap(ByteBuffer.wrap(str.getBytes(UTF_8)));
  }
}
