// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;
import java.util.NoSuchElementException;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Test {@code LiteralByteString} by setting up a reference string in {@link #setUp()}. This class
 * is designed to be extended for testing extensions of {@code LiteralByteString} such as {@code
 * BoundedByteString}, see {@link BoundedByteStringTest}.
 */
@RunWith(JUnit4.class)
public class LiteralByteStringTest {
  protected static final String UTF_8 = "UTF-8";

  protected String classUnderTest;
  protected byte[] referenceBytes;
  protected ByteString stringUnderTest;
  protected int expectedHashCode;

  @Before
  public void setUp() throws Exception {
    classUnderTest = "LiteralByteString";
    referenceBytes = ByteStringTest.getTestBytes(1234, 11337766L);
    stringUnderTest = ByteString.copyFrom(referenceBytes);
    expectedHashCode = 331161852;
  }

  @Test
  public void testExpectedType() {
    String actualClassName = getActualClassName(stringUnderTest);
    assertWithMessage("%s should match type exactly", classUnderTest)
        .that(classUnderTest)
        .isEqualTo(actualClassName);
  }

  protected String getActualClassName(Object object) {
    return object.getClass().getSimpleName();
  }

  @Test
  public void testByteAt() {
    boolean stillEqual = true;
    for (int i = 0; stillEqual && i < referenceBytes.length; ++i) {
      stillEqual = (referenceBytes[i] == stringUnderTest.byteAt(i));
    }
    assertWithMessage("%s must capture the right bytes", classUnderTest).that(stillEqual).isTrue();
  }

  @Test
  public void testByteIterator() {
    boolean stillEqual = true;
    ByteString.ByteIterator iter = stringUnderTest.iterator();
    for (int i = 0; stillEqual && i < referenceBytes.length; ++i) {
      stillEqual = (iter.hasNext() && referenceBytes[i] == iter.nextByte());
    }
    assertWithMessage("%s must capture the right bytes", classUnderTest).that(stillEqual).isTrue();
    assertWithMessage("%s must have exhausted the iterator", classUnderTest)
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
    for (byte quantum : stringUnderTest) {
      stillEqual = (referenceBytes[j] == quantum);
      ++j;
    }
    assertWithMessage("%s must capture the right bytes as Bytes", classUnderTest)
        .that(stillEqual)
        .isTrue();
    assertWithMessage("%s iterable character count", classUnderTest)
        .that(referenceBytes)
        .hasLength(j);
  }

  @Test
  public void testSize() {
    assertWithMessage("%s must have the expected size", classUnderTest)
        .that(referenceBytes.length)
        .isEqualTo(stringUnderTest.size());
  }

  @Test
  public void testGetTreeDepth() {
    assertWithMessage("%s must have depth 0", classUnderTest)
        .that(stringUnderTest.getTreeDepth())
        .isEqualTo(0);
  }

  @Test
  public void testIsBalanced() {
    assertWithMessage("%s is technically balanced", classUnderTest)
        .that(stringUnderTest.isBalanced())
        .isTrue();
  }

  @Test
  public void testCopyTo_ByteArrayOffsetLength() {
    int destinationOffset = 50;
    int length = 100;
    byte[] destination = new byte[destinationOffset + length];
    int sourceOffset = 213;
    stringUnderTest.copyTo(destination, sourceOffset, destinationOffset, length);
    boolean stillEqual = true;
    for (int i = 0; stillEqual && i < length; ++i) {
      stillEqual = referenceBytes[i + sourceOffset] == destination[i + destinationOffset];
    }
    assertWithMessage("%s.copyTo(4 arg) must give the expected bytes", classUnderTest)
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
      stringUnderTest.copyTo(
          destination, stringUnderTest.size() + 1 - length, destinationOffset, length);
      assertWithMessage(
              "Should have thrown an exception when copying too many bytes of a %s", classUnderTest)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative sourceOffset
      stringUnderTest.copyTo(destination, -1, destinationOffset, length);
      assertWithMessage(
              "Should have thrown an exception when given a negative sourceOffset in %s",
              classUnderTest)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative destinationOffset
      stringUnderTest.copyTo(destination, 0, -1, length);
      assertWithMessage(
              "Should have thrown an exception when given a negative destinationOffset in %s",
              classUnderTest)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative size
      stringUnderTest.copyTo(destination, 0, 0, -1);
      assertWithMessage(
              "Should have thrown an exception when given a negative size in %s", classUnderTest)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal too-large sourceOffset
      stringUnderTest.copyTo(destination, 2 * stringUnderTest.size(), 0, length);
      assertWithMessage(
              "Should have thrown an exception when the destinationOffset is too large in %s",
              classUnderTest)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal too-large destinationOffset
      stringUnderTest.copyTo(destination, 0, 2 * destination.length, length);
      assertWithMessage(
              "Should have thrown an exception when the destinationOffset is too large in %s",
              classUnderTest)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }
  }

  @Test
  public void testCopyTo_ByteBuffer() {
    ByteBuffer myBuffer = ByteBuffer.allocate(referenceBytes.length);
    stringUnderTest.copyTo(myBuffer);
    assertWithMessage("%s.copyTo(ByteBuffer) must give back the same bytes", classUnderTest)
        .that(Arrays.equals(referenceBytes, myBuffer.array()))
        .isTrue();
  }

  @Test
  public void testMarkSupported() {
    InputStream stream = stringUnderTest.newInput();
    assertWithMessage("%s.newInput() must support marking", classUnderTest)
        .that(stream.markSupported())
        .isTrue();
  }

  @Test
  public void testMarkAndReset() throws IOException {
    int fraction = stringUnderTest.size() / 3;

    InputStream stream = stringUnderTest.newInput();
    stream.mark(stringUnderTest.size()); // First, mark() the end.

    skipFully(stream, fraction); // Skip a large fraction, but not all.
    int available = stream.available();
    assertWithMessage(
            "%s: after skipping to the 'middle', half the bytes are available", classUnderTest)
        .that((stringUnderTest.size() - fraction) == available)
        .isTrue();
    stream.reset();

    skipFully(stream, stringUnderTest.size()); // Skip to the end.
    available = stream.available();
    assertWithMessage("%s: after skipping to the end, no more bytes are available", classUnderTest)
        .that(0 == available)
        .isTrue();
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
    ByteBuffer byteBuffer = stringUnderTest.asReadOnlyByteBuffer();
    byte[] roundTripBytes = new byte[referenceBytes.length];
    assertThat(byteBuffer.remaining() == referenceBytes.length).isTrue();
    assertThat(byteBuffer.isReadOnly()).isTrue();
    byteBuffer.get(roundTripBytes);
    assertWithMessage("%s.asReadOnlyByteBuffer() must give back the same bytes", classUnderTest)
        .that(Arrays.equals(referenceBytes, roundTripBytes))
        .isTrue();
  }

  @Test
  public void testAsReadOnlyByteBufferList() {
    List<ByteBuffer> byteBuffers = stringUnderTest.asReadOnlyByteBufferList();
    int bytesSeen = 0;
    byte[] roundTripBytes = new byte[referenceBytes.length];
    for (ByteBuffer byteBuffer : byteBuffers) {
      int thisLength = byteBuffer.remaining();
      assertThat(byteBuffer.isReadOnly()).isTrue();
      assertThat(bytesSeen + thisLength <= referenceBytes.length).isTrue();
      byteBuffer.get(roundTripBytes, bytesSeen, thisLength);
      bytesSeen += thisLength;
    }
    assertThat(bytesSeen == referenceBytes.length).isTrue();
    assertWithMessage("%s.asReadOnlyByteBufferTest() must give back the same bytes", classUnderTest)
        .that(Arrays.equals(referenceBytes, roundTripBytes))
        .isTrue();
  }

  @Test
  public void testToByteArray() {
    byte[] roundTripBytes = stringUnderTest.toByteArray();
    assertWithMessage("%s.toByteArray() must give back the same bytes", classUnderTest)
        .that(Arrays.equals(referenceBytes, roundTripBytes))
        .isTrue();
  }

  @Test
  public void testWriteTo() throws IOException {
    ByteArrayOutputStream bos = new ByteArrayOutputStream();
    stringUnderTest.writeTo(bos);
    byte[] roundTripBytes = bos.toByteArray();
    assertWithMessage("%s.writeTo() must give back the same bytes", classUnderTest)
        .that(Arrays.equals(referenceBytes, roundTripBytes))
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

    stringUnderTest.writeTo(os);
    assertWithMessage("%s.writeTo() must not grant access to underlying array", classUnderTest)
        .that(Arrays.equals(referenceBytes, stringUnderTest.toByteArray()))
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

    stringUnderTest.writeToInternal(os, 0, stringUnderTest.size());
    byte[] allZeros = new byte[stringUnderTest.size()];
    assertWithMessage("%s.writeToInternal() must grant access to underlying array", classUnderTest)
        .that(Arrays.equals(allZeros, stringUnderTest.toByteArray()))
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
            Arrays.fill(value, offset, offset + length, (byte) 0);
          }

          @Override
          public void writeLazy(ByteBuffer value) throws IOException {
            throw new UnsupportedOperationException();
          }
        };

    stringUnderTest.writeTo(out);
    byte[] allZeros = new byte[stringUnderTest.size()];
    assertWithMessage("%s.writeToInternal() must grant access to underlying array", classUnderTest)
        .that(Arrays.equals(allZeros, stringUnderTest.toByteArray()))
        .isTrue();
  }

  @Test
  public void testNewOutput() throws IOException {
    ByteArrayOutputStream bos = new ByteArrayOutputStream();
    ByteString.Output output = ByteString.newOutput();
    stringUnderTest.writeTo(output);
    assertWithMessage("Output Size returns correct result")
        .that(output.size())
        .isEqualTo(stringUnderTest.size());
    output.writeTo(bos);
    assertWithMessage("Output.writeTo() must give back the same bytes")
        .that(Arrays.equals(referenceBytes, bos.toByteArray()))
        .isTrue();

    // write the output stream to itself! This should cause it to double
    output.writeTo(output);
    assertWithMessage("Writing an output stream to itself is successful")
        .that(stringUnderTest.concat(stringUnderTest))
        .isEqualTo(output.toByteString());

    output.reset();
    assertWithMessage("Output.reset() resets the output").that(output.size()).isEqualTo(0);
    assertWithMessage("Output.reset() resets the output")
        .that(output.toByteString())
        .isEqualTo(ByteString.EMPTY);
  }

  @Test
  public void testToString() throws UnsupportedEncodingException {
    String testString = "I love unicode \u1234\u5678 characters";
    ByteString unicode = ByteString.wrap(testString.getBytes(Internal.UTF_8));
    String roundTripString = unicode.toString(UTF_8);
    assertWithMessage("%s unicode must match", classUnderTest)
        .that(testString)
        .isEqualTo(roundTripString);
  }

  @Test
  public void testCharsetToString() {
    String testString = "I love unicode \u1234\u5678 characters";
    ByteString unicode = ByteString.wrap(testString.getBytes(Internal.UTF_8));
    String roundTripString = unicode.toString(Internal.UTF_8);
    assertWithMessage("%s unicode must match", classUnderTest)
        .that(testString)
        .isEqualTo(roundTripString);
  }

  @Test
  public void testToString_returnsCanonicalEmptyString() {
    assertWithMessage("%s must be the same string references", classUnderTest)
        .that(ByteString.EMPTY.toString(Internal.UTF_8))
        .isSameInstanceAs(ByteString.wrap(new byte[] {}).toString(Internal.UTF_8));
  }

  @Test
  public void testToString_raisesException() {
    try {
      ByteString.EMPTY.toString("invalid");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (UnsupportedEncodingException expected) {
      // This is success
    }

    try {
      ByteString.wrap(referenceBytes).toString("invalid");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (UnsupportedEncodingException expected) {
      // This is success
    }
  }

  @Test
  @SuppressWarnings("SelfEquals")
  public void testEquals() {
    assertWithMessage("%s must not equal null", classUnderTest)
        .that(stringUnderTest)
        .isNotEqualTo(null);
    assertWithMessage("%s must equal self", classUnderTest)
        .that(stringUnderTest.equals(stringUnderTest))
        .isTrue();
    assertWithMessage("%s must not equal the empty string", classUnderTest)
        .that(stringUnderTest)
        .isNotEqualTo(ByteString.EMPTY);
    assertWithMessage("%s empty strings must be equal", classUnderTest)
        .that(ByteString.wrap(new byte[] {}))
        .isEqualTo(stringUnderTest.substring(55, 55));
    assertWithMessage("%s must equal another string with the same value", classUnderTest)
        .that(stringUnderTest)
        .isEqualTo(ByteString.wrap(referenceBytes));

    byte[] mungedBytes = new byte[referenceBytes.length];
    System.arraycopy(referenceBytes, 0, mungedBytes, 0, referenceBytes.length);
    mungedBytes[mungedBytes.length - 5] = (byte) (mungedBytes[mungedBytes.length - 5] ^ 0xFF);
    assertWithMessage("%s must not equal every string with the same length", classUnderTest)
        .that(stringUnderTest)
        .isNotEqualTo(ByteString.wrap(mungedBytes));
  }

  @Test
  public void testHashCode() {
    int hash = stringUnderTest.hashCode();
    assertWithMessage("%s must have expected hashCode", classUnderTest)
        .that(hash)
        .isEqualTo(expectedHashCode);
  }

  @Test
  public void testPeekCachedHashCode() {
    assertWithMessage("%s.peekCachedHashCode() should return zero at first", classUnderTest)
        .that(stringUnderTest.peekCachedHashCode())
        .isEqualTo(0);
    int unused = stringUnderTest.hashCode();
    assertWithMessage("%s.peekCachedHashCode should return zero at first", classUnderTest)
        .that(stringUnderTest.peekCachedHashCode())
        .isEqualTo(expectedHashCode);
  }

  @Test
  public void testPartialHash() {
    // partialHash() is more strenuously tested elsewhere by testing hashes of substrings.
    // This test would fail if the expected hash were 1.  It's not.
    int hash = stringUnderTest.partialHash(stringUnderTest.size(), 0, stringUnderTest.size());
    assertWithMessage("%s.partialHash() must yield expected hashCode", classUnderTest)
        .that(hash)
        .isEqualTo(expectedHashCode);
  }

  @Test
  public void testNewInput() throws IOException {
    InputStream input = stringUnderTest.newInput();
    assertWithMessage("InputStream.available() returns correct value")
        .that(stringUnderTest.size())
        .isEqualTo(input.available());
    boolean stillEqual = true;
    for (byte referenceByte : referenceBytes) {
      int expectedInt = (referenceByte & 0xFF);
      stillEqual = (expectedInt == input.read());
    }
    assertWithMessage("InputStream.available() returns correct value")
        .that(input.available())
        .isEqualTo(0);
    assertWithMessage("%s must give the same bytes from the InputStream", classUnderTest)
        .that(stillEqual)
        .isTrue();
    assertWithMessage("%s InputStream must now be exhausted", classUnderTest)
        .that(input.read())
        .isEqualTo(-1);
  }

  @Test
  public void testNewInput_readZeroBytes() throws IOException {
    InputStream input = stringUnderTest.newInput();
    assertWithMessage(
            "%s InputStream.read() returns 0 when told to read 0 bytes and not at EOF",
            classUnderTest)
        .that(input.read(new byte[0]))
        .isEqualTo(0);

    input.skip(input.available());
    assertWithMessage(
            "%s InputStream.read() returns -1 when told to read 0 bytes at EOF", classUnderTest)
        .that(input.read(new byte[0]))
        .isEqualTo(-1);
  }

  @Test
  public void testNewInput_skip() throws IOException {
    InputStream input = stringUnderTest.newInput();
    int stringSize = stringUnderTest.size();
    int nearEndIndex = stringSize * 2 / 3;

    long skipped1 = input.skip(nearEndIndex);
    assertWithMessage("InputStream.skip()").that(skipped1).isEqualTo(nearEndIndex);
    assertWithMessage("InputStream.available()")
        .that(input.available())
        .isEqualTo(stringSize - skipped1);
    assertWithMessage("InputStream.mark() is available").that(input.markSupported()).isTrue();
    input.mark(0);
    assertWithMessage("InputStream.skip(), read()")
        .that(stringUnderTest.byteAt(nearEndIndex) & 0xFF)
        .isEqualTo(input.read());
    assertWithMessage("InputStream.available()")
        .that(input.available())
        .isEqualTo(stringSize - skipped1 - 1);

    long skipped2 = input.skip(stringSize);
    assertWithMessage("InputStream.skip() incomplete")
        .that(skipped2)
        .isEqualTo(stringSize - skipped1 - 1);
    assertWithMessage("InputStream.skip(), no more input").that(input.available()).isEqualTo(0);
    assertWithMessage("InputStream.skip(), no more input").that(input.read()).isEqualTo(-1);
    assertThat(input.skip(1)).isEqualTo(0);
    assertThat(input.read(new byte[1], /* off= */ 0, /*len=*/ 0)).isEqualTo(-1);

    input.reset();
    assertWithMessage("InputStream.reset() succeeded")
        .that(input.available())
        .isEqualTo(stringSize - skipped1);
    assertWithMessage("InputStream.reset(), read()")
        .that(input.read())
        .isEqualTo(stringUnderTest.byteAt(nearEndIndex) & 0xFF);
  }

  @Test
  public void testNewCodedInput() throws IOException {
    CodedInputStream cis = stringUnderTest.newCodedInput();
    byte[] roundTripBytes = cis.readRawBytes(referenceBytes.length);
    assertWithMessage("%s must give the same bytes back from the CodedInputStream", classUnderTest)
        .that(Arrays.equals(referenceBytes, roundTripBytes))
        .isTrue();
    assertWithMessage(" %s CodedInputStream must now be exhausted", classUnderTest)
        .that(cis.isAtEnd())
        .isTrue();
  }

  /**
   * Make sure we keep things simple when concatenating with empty. See also {@link
   * ByteStringTest#testConcat_empty()}.
   */
  @Test
  public void testConcat_empty() {
    assertWithMessage("%s concatenated with empty must give %s ", classUnderTest, classUnderTest)
        .that(stringUnderTest.concat(ByteString.EMPTY))
        .isSameInstanceAs(stringUnderTest);
    assertWithMessage("empty concatenated with %s must give %s", classUnderTest, classUnderTest)
        .that(ByteString.EMPTY.concat(stringUnderTest))
        .isSameInstanceAs(stringUnderTest);
  }

  @Test
  public void testJavaSerialization() throws Exception {
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    ObjectOutputStream oos = new ObjectOutputStream(out);
    oos.writeObject(stringUnderTest);
    oos.close();
    byte[] pickled = out.toByteArray();
    InputStream in = new ByteArrayInputStream(pickled);
    ObjectInputStream ois = new ObjectInputStream(in);
    Object o = ois.readObject();
    assertWithMessage("Didn't get a ByteString back").that(o).isInstanceOf(ByteString.class);
    assertWithMessage("Should get an equal ByteString back").that(o).isEqualTo(stringUnderTest);
  }
}
