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

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.nio.BufferOverflowException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;
import java.util.NoSuchElementException;
import junit.framework.TestCase;

/** Tests for {@link NioByteString}. */
public class NioByteStringTest extends TestCase {
  private static final ByteString EMPTY = new NioByteString(ByteBuffer.wrap(new byte[0]));
  private static final String CLASSNAME = NioByteString.class.getSimpleName();
  private static final byte[] BYTES = ByteStringTest.getTestBytes(1234, 11337766L);
  private static final int EXPECTED_HASH = ByteString.wrap(BYTES).hashCode();

  private final ByteBuffer backingBuffer = ByteBuffer.wrap(BYTES.clone());
  private final ByteString testString = new NioByteString(backingBuffer);

  public void testExpectedType() {
    String actualClassName = getActualClassName(testString);
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
      stillEqual = (BYTES[i] == testString.byteAt(i));
    }
    assertTrue(CLASSNAME + " must capture the right bytes", stillEqual);
  }

  public void testByteIterator() {
    boolean stillEqual = true;
    ByteString.ByteIterator iter = testString.iterator();
    for (int i = 0; stillEqual && i < BYTES.length; ++i) {
      stillEqual = (iter.hasNext() && BYTES[i] == iter.nextByte());
    }
    assertTrue(CLASSNAME + " must capture the right bytes", stillEqual);
    assertFalse(CLASSNAME + " must have exhausted the iterator", iter.hasNext());

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
    for (byte quantum : testString) {
      stillEqual = (BYTES[j] == quantum);
      ++j;
    }
    assertTrue(CLASSNAME + " must capture the right bytes as Bytes", stillEqual);
    assertEquals(CLASSNAME + " iterable character count", BYTES.length, j);
  }

  public void testSize() {
    assertEquals(CLASSNAME + " must have the expected size", BYTES.length, testString.size());
  }

  public void testGetTreeDepth() {
    assertEquals(CLASSNAME + " must have depth 0", 0, testString.getTreeDepth());
  }

  public void testIsBalanced() {
    assertTrue(CLASSNAME + " is technically balanced", testString.isBalanced());
  }

  public void testCopyTo_ByteArrayOffsetLength() {
    int destinationOffset = 50;
    int length = 100;
    byte[] destination = new byte[destinationOffset + length];
    int sourceOffset = 213;
    testString.copyTo(destination, sourceOffset, destinationOffset, length);
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
      testString.copyTo(destination, testString.size() + 1 - length, destinationOffset, length);
      fail("Should have thrown an exception when copying too many bytes of a " + CLASSNAME);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative sourceOffset
      testString.copyTo(destination, -1, destinationOffset, length);
      fail("Should have thrown an exception when given a negative sourceOffset in " + CLASSNAME);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative destinationOffset
      testString.copyTo(destination, 0, -1, length);
      fail(
          "Should have thrown an exception when given a negative destinationOffset in "
              + CLASSNAME);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative size
      testString.copyTo(destination, 0, 0, -1);
      fail("Should have thrown an exception when given a negative size in " + CLASSNAME);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal too-large sourceOffset
      testString.copyTo(destination, 2 * testString.size(), 0, length);
      fail(
          "Should have thrown an exception when the destinationOffset is too large in "
              + CLASSNAME);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal too-large destinationOffset
      testString.copyTo(destination, 0, 2 * destination.length, length);
      fail(
          "Should have thrown an exception when the destinationOffset is too large in "
              + CLASSNAME);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }
  }

  public void testCopyTo_ByteBuffer() {
    // Same length.
    ByteBuffer myBuffer = ByteBuffer.allocate(BYTES.length);
    testString.copyTo(myBuffer);
    myBuffer.flip();
    assertEquals(
        CLASSNAME + ".copyTo(ByteBuffer) must give back the same bytes", backingBuffer, myBuffer);

    // Target buffer bigger than required.
    myBuffer = ByteBuffer.allocate(testString.size() + 1);
    testString.copyTo(myBuffer);
    myBuffer.flip();
    assertEquals(backingBuffer, myBuffer);

    // Target buffer has no space.
    myBuffer = ByteBuffer.allocate(0);
    try {
      testString.copyTo(myBuffer);
      fail("Should have thrown an exception when target ByteBuffer has insufficient capacity");
    } catch (BufferOverflowException e) {
      // Expected.
    }

    // Target buffer too small.
    myBuffer = ByteBuffer.allocate(1);
    try {
      testString.copyTo(myBuffer);
      fail("Should have thrown an exception when target ByteBuffer has insufficient capacity");
    } catch (BufferOverflowException e) {
      // Expected.
    }
  }

  public void testMarkSupported() {
    InputStream stream = testString.newInput();
    assertTrue(CLASSNAME + ".newInput() must support marking", stream.markSupported());
  }

  public void testMarkAndReset() throws IOException {
    int fraction = testString.size() / 3;

    InputStream stream = testString.newInput();
    stream.mark(testString.size()); // First, mark() the end.

    skipFully(stream, fraction); // Skip a large fraction, but not all.
    assertEquals(
        CLASSNAME + ": after skipping to the 'middle', half the bytes are available",
        (testString.size() - fraction),
        stream.available());
    stream.reset();
    assertEquals(
        CLASSNAME + ": after resetting, all bytes are available",
        testString.size(),
        stream.available());

    skipFully(stream, testString.size()); // Skip to the end.
    assertEquals(
        CLASSNAME + ": after skipping to the end, no more bytes are available",
        0,
        stream.available());
  }

  /**
   * Discards {@code n} bytes of data from the input stream. This method will block until the full
   * amount has been skipped. Does not close the stream.
   *
   * <p>Copied from com.google.common.io.ByteStreams to avoid adding dependency.
   *
   * @param in the input stream to read from
   * @param n the number of bytes to skip
   * @throws EOFException if this stream reaches the end before skipping all the bytes
   * @throws IOException if an I/O error occurs, or the stream does not support skipping
   */
  static void skipFully(InputStream in, long n) throws IOException {
    long toSkip = n;
    while (n > 0) {
      long amt = in.skip(n);
      if (amt == 0) {
        // Force a blocking read to avoid infinite loop
        if (in.read() == -1) {
          long skipped = toSkip - n;
          throw new EOFException(
              "reached end of stream after skipping "
                  + skipped
                  + " bytes; "
                  + toSkip
                  + " bytes expected");
        }
        n--;
      } else {
        n -= amt;
      }
    }
  }

  public void testAsReadOnlyByteBuffer() {
    ByteBuffer byteBuffer = testString.asReadOnlyByteBuffer();
    byte[] roundTripBytes = new byte[BYTES.length];
    assertTrue(byteBuffer.remaining() == BYTES.length);
    assertTrue(byteBuffer.isReadOnly());
    byteBuffer.get(roundTripBytes);
    assertTrue(
        CLASSNAME + ".asReadOnlyByteBuffer() must give back the same bytes",
        Arrays.equals(BYTES, roundTripBytes));
  }

  public void testAsReadOnlyByteBufferList() {
    List<ByteBuffer> byteBuffers = testString.asReadOnlyByteBufferList();
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
    assertTrue(
        CLASSNAME + ".asReadOnlyByteBufferTest() must give back the same bytes",
        Arrays.equals(BYTES, roundTripBytes));
  }

  public void testToByteArray() {
    byte[] roundTripBytes = testString.toByteArray();
    assertTrue(
        CLASSNAME + ".toByteArray() must give back the same bytes",
        Arrays.equals(BYTES, roundTripBytes));
  }

  public void testWriteTo() throws IOException {
    ByteArrayOutputStream bos = new ByteArrayOutputStream();
    testString.writeTo(bos);
    byte[] roundTripBytes = bos.toByteArray();
    assertTrue(
        CLASSNAME + ".writeTo() must give back the same bytes",
        Arrays.equals(BYTES, roundTripBytes));
  }

  public void testWriteToShouldNotExposeInternalBufferToOutputStream() throws IOException {
    OutputStream os =
        new OutputStream() {
          @Override
          public void write(byte[] b, int off, int len) {
            Arrays.fill(b, off, off + len, (byte) 0);
          }

          @Override
          public void write(int b) {
            throw new UnsupportedOperationException();
          }
        };

    byte[] original = Arrays.copyOf(BYTES, BYTES.length);
    testString.writeTo(os);
    assertTrue(
        CLASSNAME + ".writeTo() must NOT grant access to underlying buffer",
        Arrays.equals(original, BYTES));
  }

  public void testWriteToInternalShouldExposeInternalBufferToOutputStream() throws IOException {
    OutputStream os =
        new OutputStream() {
          @Override
          public void write(byte[] b, int off, int len) {
            Arrays.fill(b, off, off + len, (byte) 0);
          }

          @Override
          public void write(int b) {
            throw new UnsupportedOperationException();
          }
        };

    testString.writeToInternal(os, 0, testString.size());
    byte[] allZeros = new byte[testString.size()];
    assertTrue(
        CLASSNAME + ".writeToInternal() must grant access to underlying buffer",
        Arrays.equals(allZeros, backingBuffer.array()));
  }

  public void testWriteToShouldExposeInternalBufferToByteOutput() throws IOException {
    ByteOutput out =
        new ByteOutput() {
          @Override
          public void write(byte value) throws IOException {
            throw new UnsupportedOperationException();
          }

          @Override
          public void write(byte[] value, int offset, int length) throws IOException {
            throw new UnsupportedOperationException();
          }

          @Override
          public void write(ByteBuffer value) throws IOException {
            throw new UnsupportedOperationException();
          }

          @Override
          public void writeLazy(byte[] value, int offset, int length) throws IOException {
            throw new UnsupportedOperationException();
          }

          @Override
          public void writeLazy(ByteBuffer value) throws IOException {
            Arrays.fill(
                value.array(), value.arrayOffset(), value.arrayOffset() + value.limit(), (byte) 0);
          }
        };

    testString.writeTo(out);
    byte[] allZeros = new byte[testString.size()];
    assertTrue(
        CLASSNAME + ".writeTo() must grant access to underlying buffer",
        Arrays.equals(allZeros, backingBuffer.array()));
  }

  public void testNewOutput() throws IOException {
    ByteArrayOutputStream bos = new ByteArrayOutputStream();
    ByteString.Output output = ByteString.newOutput();
    testString.writeTo(output);
    assertEquals("Output Size returns correct result", output.size(), testString.size());
    output.writeTo(bos);
    assertTrue(
        "Output.writeTo() must give back the same bytes", Arrays.equals(BYTES, bos.toByteArray()));

    // write the output stream to itself! This should cause it to double
    output.writeTo(output);
    assertEquals(
        "Writing an output stream to itself is successful",
        testString.concat(testString),
        output.toByteString());

    output.reset();
    assertEquals("Output.reset() resets the output", 0, output.size());
    assertEquals("Output.reset() resets the output", EMPTY, output.toByteString());
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
    assertSame(
        CLASSNAME + " must be the same string references",
        EMPTY.toString(UTF_8),
        new NioByteString(ByteBuffer.wrap(new byte[0])).toString(UTF_8));
  }

  public void testToString_raisesException() {
    try {
      EMPTY.toString("invalid");
      fail("Should have thrown an exception.");
    } catch (UnsupportedEncodingException expected) {
      // This is success
    }

    try {
      testString.toString("invalid");
      fail("Should have thrown an exception.");
    } catch (UnsupportedEncodingException expected) {
      // This is success
    }
  }

  public void testEquals() {
    assertEquals(CLASSNAME + " must not equal null", false, testString.equals(null));
    assertEquals(CLASSNAME + " must equal self", testString, testString);
    assertFalse(CLASSNAME + " must not equal the empty string", testString.equals(EMPTY));
    assertEquals(CLASSNAME + " empty strings must be equal", EMPTY, testString.substring(55, 55));
    assertEquals(
        CLASSNAME + " must equal another string with the same value",
        testString,
        new NioByteString(backingBuffer));

    byte[] mungedBytes = mungedBytes();
    assertFalse(
        CLASSNAME + " must not equal every string with the same length",
        testString.equals(new NioByteString(ByteBuffer.wrap(mungedBytes))));
  }

  public void testEqualsLiteralByteString() {
    ByteString literal = ByteString.copyFrom(BYTES);
    assertEquals(CLASSNAME + " must equal LiteralByteString with same value", literal, testString);
    assertEquals(CLASSNAME + " must equal LiteralByteString with same value", testString, literal);
    assertFalse(
        CLASSNAME + " must not equal the empty string", testString.equals(ByteString.EMPTY));
    assertEquals(
        CLASSNAME + " empty strings must be equal", ByteString.EMPTY, testString.substring(55, 55));

    literal = ByteString.copyFrom(mungedBytes());
    assertFalse(
        CLASSNAME + " must not equal every LiteralByteString with the same length",
        testString.equals(literal));
    assertFalse(
        CLASSNAME + " must not equal every LiteralByteString with the same length",
        literal.equals(testString));
  }

  public void testEqualsRopeByteString() {
    ByteString p1 = ByteString.copyFrom(BYTES, 0, 5);
    ByteString p2 = ByteString.copyFrom(BYTES, 5, BYTES.length - 5);
    ByteString rope = p1.concat(p2);

    assertEquals(CLASSNAME + " must equal RopeByteString with same value", rope, testString);
    assertEquals(CLASSNAME + " must equal RopeByteString with same value", testString, rope);
    assertFalse(
        CLASSNAME + " must not equal the empty string",
        testString.equals(ByteString.EMPTY.concat(ByteString.EMPTY)));
    assertEquals(
        CLASSNAME + " empty strings must be equal",
        ByteString.EMPTY.concat(ByteString.EMPTY),
        testString.substring(55, 55));

    byte[] mungedBytes = mungedBytes();
    p1 = ByteString.copyFrom(mungedBytes, 0, 5);
    p2 = ByteString.copyFrom(mungedBytes, 5, mungedBytes.length - 5);
    rope = p1.concat(p2);
    assertFalse(
        CLASSNAME + " must not equal every RopeByteString with the same length",
        testString.equals(rope));
    assertFalse(
        CLASSNAME + " must not equal every RopeByteString with the same length",
        rope.equals(testString));
  }

  private byte[] mungedBytes() {
    byte[] mungedBytes = new byte[BYTES.length];
    System.arraycopy(BYTES, 0, mungedBytes, 0, BYTES.length);
    mungedBytes[mungedBytes.length - 5] = (byte) (mungedBytes[mungedBytes.length - 5] ^ 0xFF);
    return mungedBytes;
  }

  public void testHashCode() {
    int hash = testString.hashCode();
    assertEquals(CLASSNAME + " must have expected hashCode", EXPECTED_HASH, hash);
  }

  public void testPeekCachedHashCode() {
    ByteString newString = new NioByteString(backingBuffer);
    assertEquals(
        CLASSNAME + ".peekCachedHashCode() should return zero at first",
        0,
        newString.peekCachedHashCode());
    newString.hashCode();
    assertEquals(
        CLASSNAME + ".peekCachedHashCode should return zero at first",
        EXPECTED_HASH,
        newString.peekCachedHashCode());
  }

  public void testPartialHash() {
    // partialHash() is more strenuously tested elsewhere by testing hashes of substrings.
    // This test would fail if the expected hash were 1.  It's not.
    int hash = testString.partialHash(testString.size(), 0, testString.size());
    assertEquals(CLASSNAME + ".partialHash() must yield expected hashCode", EXPECTED_HASH, hash);
  }

  public void testNewInput() throws IOException {
    InputStream input = testString.newInput();
    assertEquals(
        "InputStream.available() returns correct value", testString.size(), input.available());
    boolean stillEqual = true;
    for (byte referenceByte : BYTES) {
      int expectedInt = (referenceByte & 0xFF);
      stillEqual = (expectedInt == input.read());
    }
    assertEquals("InputStream.available() returns correct value", 0, input.available());
    assertTrue(CLASSNAME + " must give the same bytes from the InputStream", stillEqual);
    assertEquals(CLASSNAME + " InputStream must now be exhausted", -1, input.read());
  }

  public void testNewInput_skip() throws IOException {
    InputStream input = testString.newInput();
    int stringSize = testString.size();
    int nearEndIndex = stringSize * 2 / 3;
    long skipped1 = input.skip(nearEndIndex);
    assertEquals("InputStream.skip()", skipped1, nearEndIndex);
    assertEquals("InputStream.available()", stringSize - skipped1, input.available());
    assertTrue("InputStream.mark() is available", input.markSupported());
    input.mark(0);
    assertEquals(
        "InputStream.skip(), read()", testString.byteAt(nearEndIndex) & 0xFF, input.read());
    assertEquals("InputStream.available()", stringSize - skipped1 - 1, input.available());
    long skipped2 = input.skip(stringSize);
    assertEquals("InputStream.skip() incomplete", skipped2, stringSize - skipped1 - 1);
    assertEquals("InputStream.skip(), no more input", 0, input.available());
    assertEquals("InputStream.skip(), no more input", -1, input.read());
    input.reset();
    assertEquals("InputStream.reset() succeeded", stringSize - skipped1, input.available());
    assertEquals(
        "InputStream.reset(), read()", testString.byteAt(nearEndIndex) & 0xFF, input.read());
  }

  public void testNewCodedInput() throws IOException {
    CodedInputStream cis = testString.newCodedInput();
    byte[] roundTripBytes = cis.readRawBytes(BYTES.length);
    assertTrue(
        CLASSNAME + " must give the same bytes back from the CodedInputStream",
        Arrays.equals(BYTES, roundTripBytes));
    assertTrue(CLASSNAME + " CodedInputStream must now be exhausted", cis.isAtEnd());
  }

  /**
   * Make sure we keep things simple when concatenating with empty. See also {@link
   * ByteStringTest#testConcat_empty()}.
   */
  public void testConcat_empty() {
    assertSame(
        CLASSNAME + " concatenated with empty must give " + CLASSNAME,
        testString.concat(EMPTY),
        testString);
    assertSame(
        "empty concatenated with " + CLASSNAME + " must give " + CLASSNAME,
        EMPTY.concat(testString),
        testString);
  }

  public void testJavaSerialization() throws Exception {
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    ObjectOutputStream oos = new ObjectOutputStream(out);
    oos.writeObject(testString);
    oos.close();
    byte[] pickled = out.toByteArray();
    InputStream in = new ByteArrayInputStream(pickled);
    ObjectInputStream ois = new ObjectInputStream(in);
    Object o = ois.readObject();
    assertTrue("Didn't get a ByteString back", o instanceof ByteString);
    assertEquals("Should get an equal ByteString back", testString, o);
  }

  private static ByteString forString(String str) {
    return new NioByteString(ByteBuffer.wrap(str.getBytes(UTF_8)));
  }
}
