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

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;
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
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for {@link NioByteString}. */
@RunWith(JUnit4.class)
public class NioByteStringTest {
  private static final ByteString EMPTY = new NioByteString(ByteBuffer.wrap(new byte[0]));
  private static final String CLASSNAME = NioByteString.class.getSimpleName();
  private static final byte[] BYTES = ByteStringTest.getTestBytes(1234, 11337766L);
  private static final int EXPECTED_HASH = ByteString.wrap(BYTES).hashCode();

  private final ByteBuffer backingBuffer = ByteBuffer.wrap(BYTES.clone());
  private final ByteString testString = new NioByteString(backingBuffer);

  @Test
  public void testExpectedType() {
    String actualClassName = getActualClassName(testString);
    assertWithMessage("%s should match type exactly", CLASSNAME)
        .that(CLASSNAME)
        .isEqualTo(actualClassName);
  }

  protected String getActualClassName(Object object) {
    String actualClassName = object.getClass().getName();
    actualClassName = actualClassName.substring(actualClassName.lastIndexOf('.') + 1);
    return actualClassName;
  }

  @Test
  public void testByteAt() {
    boolean stillEqual = true;
    for (int i = 0; stillEqual && i < BYTES.length; ++i) {
      stillEqual = (BYTES[i] == testString.byteAt(i));
    }
    assertWithMessage("%s must capture the right bytes", CLASSNAME).that(stillEqual).isTrue();
  }

  @Test
  public void testByteIterator() {
    boolean stillEqual = true;
    ByteString.ByteIterator iter = testString.iterator();
    for (int i = 0; stillEqual && i < BYTES.length; ++i) {
      stillEqual = (iter.hasNext() && BYTES[i] == iter.nextByte());
    }
    assertWithMessage("%s must capture the right bytes", CLASSNAME).that(stillEqual).isTrue();
    assertWithMessage("%s must have exhausted the iterator", CLASSNAME)
        .that(iter.hasNext())
        .isFalse();

    try {
      iter.nextByte();
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (NoSuchElementException e) {
      // This is success
    }
  }

  @Test
  public void testByteIterable() {
    boolean stillEqual = true;
    int j = 0;
    for (byte quantum : testString) {
      stillEqual = (BYTES[j] == quantum);
      ++j;
    }
    assertWithMessage("%s must capture the right bytes as Bytes", CLASSNAME)
        .that(stillEqual)
        .isTrue();
    assertWithMessage("%s iterable character count", CLASSNAME).that(BYTES).hasLength(j);
  }

  @Test
  public void testSize() {
    assertWithMessage("%s must have the expected size", CLASSNAME)
        .that(BYTES)
        .hasLength(testString.size());
  }

  @Test
  public void testGetTreeDepth() {
    assertWithMessage("%s must have depth 0", CLASSNAME)
        .that(testString.getTreeDepth())
        .isEqualTo(0);
  }

  @Test
  public void testIsBalanced() {
    assertWithMessage("%s is technically balanced", CLASSNAME)
        .that(testString.isBalanced())
        .isTrue();
  }

  @Test
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
    assertWithMessage("%s.copyTo(4 arg) must give the expected bytes", CLASSNAME)
        .that(stillEqual)
        .isTrue();
  }

  @Test
  public void testCopyTo_ByteArrayOffsetLengthErrors() {
    int destinationOffset = 50;
    int length = 100;
    byte[] destination = new byte[destinationOffset + length];

    try {
      // Copy one too many bytes
      testString.copyTo(destination, testString.size() + 1 - length, destinationOffset, length);
      assertWithMessage(
              "Should have thrown an exception when copying too many bytes of a %s", CLASSNAME)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative sourceOffset
      testString.copyTo(destination, -1, destinationOffset, length);
      assertWithMessage(
              "Should have thrown an exception when given a negative sourceOffset in %s ",
              CLASSNAME)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative destinationOffset
      testString.copyTo(destination, 0, -1, length);
      assertWithMessage(
              "Should have thrown an exception when given a negative destinationOffset in %s",
              CLASSNAME)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative size
      testString.copyTo(destination, 0, 0, -1);
      assertWithMessage(
              "Should have thrown an exception when given a negative size in %s", CLASSNAME)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal too-large sourceOffset
      testString.copyTo(destination, 2 * testString.size(), 0, length);
      assertWithMessage(
              "Should have thrown an exception when the destinationOffset is too large in %s",
              CLASSNAME)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal too-large destinationOffset
      testString.copyTo(destination, 0, 2 * destination.length, length);
      assertWithMessage(
              "Should have thrown an exception when the destinationOffset is too large in %s",
              CLASSNAME)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }
  }

  @Test
  public void testCopyTo_ByteBuffer() {
    // Same length.
    ByteBuffer myBuffer = ByteBuffer.allocate(BYTES.length);
    testString.copyTo(myBuffer);
    myBuffer.flip();
    assertWithMessage("%s.copyTo(ByteBuffer) must give back the same bytes", CLASSNAME)
        .that(backingBuffer)
        .isEqualTo(myBuffer);

    // Target buffer bigger than required.
    myBuffer = ByteBuffer.allocate(testString.size() + 1);
    testString.copyTo(myBuffer);
    myBuffer.flip();
    assertThat(backingBuffer).isEqualTo(myBuffer);

    // Target buffer has no space.
    myBuffer = ByteBuffer.allocate(0);
    try {
      testString.copyTo(myBuffer);
      assertWithMessage(
              "Should have thrown an exception when target ByteBuffer has insufficient capacity")
          .fail();
    } catch (BufferOverflowException e) {
      // Expected.
    }

    // Target buffer too small.
    myBuffer = ByteBuffer.allocate(1);
    try {
      testString.copyTo(myBuffer);
      assertWithMessage(
              "Should have thrown an exception when target ByteBuffer has insufficient capacity")
          .fail();
    } catch (BufferOverflowException e) {
      // Expected.
    }
  }

  @Test
  public void testMarkSupported() {
    InputStream stream = testString.newInput();
    assertWithMessage("%s.newInput() must support marking", CLASSNAME)
        .that(stream.markSupported())
        .isTrue();
  }

  @Test
  public void testMarkAndReset() throws IOException {
    int fraction = testString.size() / 3;

    InputStream stream = testString.newInput();
    stream.mark(testString.size()); // First, mark() the end.

    skipFully(stream, fraction); // Skip a large fraction, but not all.
    assertWithMessage("%s: after skipping to the 'middle', half the bytes are available", CLASSNAME)
        .that((testString.size() - fraction))
        .isEqualTo(stream.available());
    stream.reset();
    assertWithMessage("%s: after resetting, all bytes are available", CLASSNAME)
        .that(testString.size())
        .isEqualTo(stream.available());

    skipFully(stream, testString.size()); // Skip to the end.
    assertWithMessage("%s: after skipping to the end, no more bytes are available", CLASSNAME)
        .that(stream.available())
        .isEqualTo(0);
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

  @Test
  public void testAsReadOnlyByteBuffer() {
    ByteBuffer byteBuffer = testString.asReadOnlyByteBuffer();
    byte[] roundTripBytes = new byte[BYTES.length];
    assertThat(byteBuffer.remaining() == BYTES.length).isTrue();
    assertThat(byteBuffer.isReadOnly()).isTrue();
    byteBuffer.get(roundTripBytes);
    assertWithMessage("%s.asReadOnlyByteBuffer() must give back the same bytes", CLASSNAME)
        .that(Arrays.equals(BYTES, roundTripBytes))
        .isTrue();
  }

  @Test
  public void testAsReadOnlyByteBufferList() {
    List<ByteBuffer> byteBuffers = testString.asReadOnlyByteBufferList();
    int bytesSeen = 0;
    byte[] roundTripBytes = new byte[BYTES.length];
    for (ByteBuffer byteBuffer : byteBuffers) {
      int thisLength = byteBuffer.remaining();
      assertThat(byteBuffer.isReadOnly()).isTrue();
      assertThat(bytesSeen + thisLength <= BYTES.length).isTrue();
      byteBuffer.get(roundTripBytes, bytesSeen, thisLength);
      bytesSeen += thisLength;
    }
    assertThat(BYTES).hasLength(bytesSeen);
    assertWithMessage("%s.asReadOnlyByteBufferTest() must give back the same bytes", CLASSNAME)
        .that(Arrays.equals(BYTES, roundTripBytes))
        .isTrue();
  }

  @Test
  public void testToByteArray() {
    byte[] roundTripBytes = testString.toByteArray();
    assertWithMessage("%s.toByteArray() must give back the same bytes", CLASSNAME)
        .that(Arrays.equals(BYTES, roundTripBytes))
        .isTrue();
  }

  @Test
  public void testWriteTo() throws IOException {
    ByteArrayOutputStream bos = new ByteArrayOutputStream();
    testString.writeTo(bos);
    byte[] roundTripBytes = bos.toByteArray();
    assertWithMessage("%s.writeTo() must give back the same bytes", CLASSNAME)
        .that(Arrays.equals(BYTES, roundTripBytes))
        .isTrue();
  }

  @Test
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
    assertWithMessage("%s.writeTo() must NOT grant access to underlying buffer", CLASSNAME)
        .that(Arrays.equals(original, BYTES))
        .isTrue();
  }

  @Test
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
    assertWithMessage("%s.writeToInternal() must grant access to underlying buffer", CLASSNAME)
        .that(Arrays.equals(allZeros, backingBuffer.array()))
        .isTrue();
  }

  @Test
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
    assertWithMessage("%s.writeTo() must grant access to underlying buffer", CLASSNAME)
        .that(Arrays.equals(allZeros, backingBuffer.array()))
        .isTrue();
  }

  @Test
  public void testNewOutput() throws IOException {
    ByteArrayOutputStream bos = new ByteArrayOutputStream();
    ByteString.Output output = ByteString.newOutput();
    testString.writeTo(output);
    assertWithMessage("Output Size returns correct result")
        .that(output.size())
        .isEqualTo(testString.size());
    output.writeTo(bos);
    assertWithMessage("Output.writeTo() must give back the same bytes")
        .that(Arrays.equals(BYTES, bos.toByteArray()))
        .isTrue();

    // write the output stream to itself! This should cause it to double
    output.writeTo(output);
    assertWithMessage("Writing an output stream to itself is successful")
        .that(testString.concat(testString))
        .isEqualTo(output.toByteString());

    output.reset();
    assertWithMessage("Output.reset() resets the output").that(output.size()).isEqualTo(0);
    assertWithMessage("Output.reset() resets the output")
        .that(output.toByteString())
        .isEqualTo(EMPTY);
  }

  @Test
  public void testToString() {
    String testString = "I love unicode \u1234\u5678 characters";
    ByteString unicode = forString(testString);
    String roundTripString = unicode.toString(UTF_8);
    assertWithMessage("%s unicode must match", CLASSNAME)
        .that(testString)
        .isEqualTo(roundTripString);
  }

  @Test
  public void testCharsetToString() {
    String testString = "I love unicode \u1234\u5678 characters";
    ByteString unicode = forString(testString);
    String roundTripString = unicode.toString(UTF_8);
    assertWithMessage("%s unicode must match", CLASSNAME)
        .that(testString)
        .isEqualTo(roundTripString);
  }

  @Test
  public void testToString_returnsCanonicalEmptyString() {
    assertWithMessage("%s must be the same string references", CLASSNAME)
        .that(EMPTY.toString(UTF_8))
        .isSameInstanceAs(new NioByteString(ByteBuffer.wrap(new byte[0])).toString(UTF_8));
  }

  @Test
  public void testToString_raisesException() {
    try {
      EMPTY.toString("invalid");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (UnsupportedEncodingException expected) {
      // This is success
    }

    try {
      testString.toString("invalid");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (UnsupportedEncodingException expected) {
      // This is success
    }
  }

  @Test
  @SuppressWarnings("TruthSelfEquals")
  public void testEquals() {
    assertWithMessage("%s must not equal null", CLASSNAME).that(testString).isNotEqualTo(null);
    assertWithMessage("%s must equal self", CLASSNAME).that(testString).isEqualTo(testString);
    assertWithMessage("%s must not equal the empty string", CLASSNAME)
        .that(testString)
        .isNotEqualTo(EMPTY);
    assertWithMessage("%s empty strings must be equal", CLASSNAME)
        .that(EMPTY)
        .isEqualTo(testString.substring(55, 55));
    assertWithMessage("%s must equal another string with the same value", CLASSNAME)
        .that(testString)
        .isEqualTo(new NioByteString(backingBuffer));

    byte[] mungedBytes = mungedBytes();
    assertWithMessage("%s must not equal every string with the same length", CLASSNAME)
        .that(testString.equals(new NioByteString(ByteBuffer.wrap(mungedBytes))))
        .isFalse();
  }

  @Test
  public void testEqualsLiteralByteString() {
    ByteString literal = ByteString.copyFrom(BYTES);
    assertWithMessage("%s must equal LiteralByteString with same value", CLASSNAME)
        .that(literal)
        .isEqualTo(testString);
    assertWithMessage("%s must equal LiteralByteString with same value", CLASSNAME)
        .that(testString)
        .isEqualTo(literal);
    assertWithMessage("%s must not equal the empty string", CLASSNAME)
        .that(testString)
        .isNotEqualTo(ByteString.EMPTY);
    assertWithMessage("%s empty strings must be equal", CLASSNAME)
        .that(ByteString.EMPTY)
        .isEqualTo(testString.substring(55, 55));

    literal = ByteString.copyFrom(mungedBytes());
    assertWithMessage("%s must not equal every LiteralByteString with the same length", CLASSNAME)
        .that(testString)
        .isNotEqualTo(literal);
    assertWithMessage("%s must not equal every LiteralByteString with the same length", CLASSNAME)
        .that(literal)
        .isNotEqualTo(testString);
  }

  @Test
  public void testEqualsRopeByteString() {
    ByteString p1 = ByteString.copyFrom(BYTES, 0, 5);
    ByteString p2 = ByteString.copyFrom(BYTES, 5, BYTES.length - 5);
    ByteString rope = p1.concat(p2);

    assertWithMessage("%s must equal RopeByteString with same value", CLASSNAME)
        .that(rope)
        .isEqualTo(testString);
    assertWithMessage("%s must equal RopeByteString with same value", CLASSNAME)
        .that(testString)
        .isEqualTo(rope);
    assertWithMessage("%s must not equal the empty string", CLASSNAME)
        .that(testString)
        .isNotEqualTo(ByteString.EMPTY.concat(ByteString.EMPTY));
    assertWithMessage("%s empty strings must be equal", CLASSNAME)
        .that(ByteString.EMPTY.concat(ByteString.EMPTY))
        .isEqualTo(testString.substring(55, 55));

    byte[] mungedBytes = mungedBytes();
    p1 = ByteString.copyFrom(mungedBytes, 0, 5);
    p2 = ByteString.copyFrom(mungedBytes, 5, mungedBytes.length - 5);
    rope = p1.concat(p2);
    assertWithMessage("%s must not equal every RopeByteString with the same length", CLASSNAME)
        .that(testString)
        .isNotEqualTo(rope);

    assertWithMessage("%s must not equal every RopeByteString with the same length", CLASSNAME)
        .that(rope)
        .isNotEqualTo(testString);
  }

  private byte[] mungedBytes() {
    byte[] mungedBytes = new byte[BYTES.length];
    System.arraycopy(BYTES, 0, mungedBytes, 0, BYTES.length);
    mungedBytes[mungedBytes.length - 5] = (byte) (mungedBytes[mungedBytes.length - 5] ^ 0xFF);
    return mungedBytes;
  }

  @Test
  public void testHashCode() {
    int hash = testString.hashCode();
    assertWithMessage("%s must have expected hashCode", CLASSNAME)
        .that(hash)
        .isEqualTo(EXPECTED_HASH);
  }

  @Test
  public void testPeekCachedHashCode() {
    ByteString newString = new NioByteString(backingBuffer);
    assertWithMessage("%s.peekCachedHashCode() should return zero at first", CLASSNAME)
        .that(newString.peekCachedHashCode())
        .isEqualTo(0);
    int unused = newString.hashCode();
    assertWithMessage("%s.peekCachedHashCode should return zero at first", CLASSNAME)
        .that(newString.peekCachedHashCode())
        .isEqualTo(EXPECTED_HASH);
  }

  @Test
  public void testPartialHash() {
    // partialHash() is more strenuously tested elsewhere by testing hashes of substrings.
    // This test would fail if the expected hash were 1.  It's not.
    int hash = testString.partialHash(testString.size(), 0, testString.size());
    assertWithMessage("%s.partialHash() must yield expected hashCode", CLASSNAME)
        .that(hash)
        .isEqualTo(EXPECTED_HASH);
  }

  @Test
  public void testNewInput() throws IOException {
    InputStream input = testString.newInput();
    assertWithMessage("InputStream.available() returns correct value")
        .that(testString.size())
        .isEqualTo(input.available());
    boolean stillEqual = true;
    for (byte referenceByte : BYTES) {
      int expectedInt = (referenceByte & 0xFF);
      stillEqual = (expectedInt == input.read());
    }
    assertWithMessage("InputStream.available() returns correct value")
        .that(input.available())
        .isEqualTo(0);
    assertWithMessage("%s must give the same bytes from the InputStream", CLASSNAME)
        .that(stillEqual)
        .isTrue();
    assertWithMessage("%s InputStream must now be exhausted", CLASSNAME)
        .that(input.read())
        .isEqualTo(-1);
  }

  @Test
  public void testNewInput_skip() throws IOException {
    InputStream input = testString.newInput();
    int stringSize = testString.size();
    int nearEndIndex = stringSize * 2 / 3;
    long skipped1 = input.skip(nearEndIndex);
    assertWithMessage("InputStream.skip()").that(skipped1).isEqualTo(nearEndIndex);
    assertWithMessage("InputStream.available()")
        .that(input.available())
        .isEqualTo(stringSize - skipped1);
    assertWithMessage("InputStream.mark() is available").that(input.markSupported()).isTrue();
    input.mark(0);
    assertWithMessage("InputStream.skip(), read()")
        .that(input.read())
        .isEqualTo(testString.byteAt(nearEndIndex) & 0xFF);
    assertWithMessage("InputStream.available()")
        .that(input.available())
        .isEqualTo(stringSize - skipped1 - 1);
    long skipped2 = input.skip(stringSize);
    assertWithMessage("InputStream.skip() incomplete")
        .that(skipped2)
        .isEqualTo(stringSize - skipped1 - 1);
    assertWithMessage("InputStream.skip(), no more input").that(input.available()).isEqualTo(0);
    assertWithMessage("InputStream.skip(), no more input").that(input.read()).isEqualTo(-1);
    input.reset();
    assertWithMessage("InputStream.reset() succeeded")
        .that(input.available())
        .isEqualTo(stringSize - skipped1);
    assertWithMessage("InputStream.reset(), read()")
        .that(input.read())
        .isEqualTo(testString.byteAt(nearEndIndex) & 0xFF);
  }

  @Test
  public void testNewCodedInput() throws IOException {
    CodedInputStream cis = testString.newCodedInput();
    byte[] roundTripBytes = cis.readRawBytes(BYTES.length);
    assertWithMessage("%s must give the same bytes back from the CodedInputStream", CLASSNAME)
        .that(Arrays.equals(BYTES, roundTripBytes))
        .isTrue();
    assertWithMessage("%s CodedInputStream must now be exhausted", CLASSNAME)
        .that(cis.isAtEnd())
        .isTrue();
  }

  /**
   * Make sure we keep things simple when concatenating with empty. See also {@link
   * ByteStringTest#testConcat_empty()}.
   */
  @Test
  public void testConcat_empty() {
    assertWithMessage("%s concatenated with empty must give %s", CLASSNAME, CLASSNAME)
        .that(testString.concat(EMPTY))
        .isSameInstanceAs(testString);
    assertWithMessage("empty concatenated with %s must give %s", CLASSNAME, CLASSNAME)
        .that(EMPTY.concat(testString))
        .isSameInstanceAs(testString);
  }

  @Test
  public void testJavaSerialization() throws Exception {
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    ObjectOutputStream oos = new ObjectOutputStream(out);
    oos.writeObject(testString);
    oos.close();
    byte[] pickled = out.toByteArray();
    InputStream in = new ByteArrayInputStream(pickled);
    ObjectInputStream ois = new ObjectInputStream(in);
    Object o = ois.readObject();
    assertWithMessage("Didn't get a ByteString back").that(o).isInstanceOf(ByteString.class);
    assertWithMessage("Should get an equal ByteString back").that(o).isEqualTo(testString);
  }

  private static ByteString forString(String str) {
    return new NioByteString(ByteBuffer.wrap(str.getBytes(UTF_8)));
  }
}
