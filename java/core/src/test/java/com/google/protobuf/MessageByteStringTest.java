// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;
import static java.nio.charset.StandardCharsets.UTF_8;

import com.google.protobuf.wrapperstest.WrappersTestProto.TopLevelMessage;
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

/** Tests for {@link MessageByteString}. */
@RunWith(JUnit4.class)
public class MessageByteStringTest {
  private static final MessageLite EMPTY_MSG = TopLevelMessage.getDefaultInstance();
  private static final ByteString EMPTY = ByteString.wrap(TopLevelMessage.getDefaultInstance());
  private static final String CLASSNAME = "ByteString$MessageByteString";
  private static final MessageLite MSG =
      TopLevelMessage.newBuilder()
          .setFieldBytes(
              BytesValue.of(ByteString.copyFrom(ByteStringTest.getTestBytes(1234, 11337766L))))
          .build();
  private static final int EXPECTED_HASH = MSG.toByteString().hashCode();
  private static final byte[] BYTES = MSG.toByteArray();

  private final ByteString.MessageByteString testByteString =
      (ByteString.MessageByteString) ByteString.wrap(MSG);

  @Test
  public void testExpectedType() {
    String actualClassName = getActualClassName(testByteString);
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
      stillEqual = (BYTES[i] == testByteString.byteAt(i));
    }
    assertWithMessage("%s must capture the right bytes", CLASSNAME).that(stillEqual).isTrue();
  }

  @Test
  public void testByteIterator() {
    boolean stillEqual = true;
    ByteString.ByteIterator iter = testByteString.iterator();
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
    for (byte quantum : testByteString) {
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
        .hasLength(testByteString.size());
    assertThat(testByteString.allocated).isNull();
  }

  @Test
  public void testGetTreeDepth() {
    assertWithMessage("%s must have depth 0", CLASSNAME)
        .that(testByteString.getTreeDepth())
        .isEqualTo(0);
    assertThat(testByteString.allocated).isNull();
  }

  @Test
  public void testIsBalanced() {
    assertWithMessage("%s is technically balanced", CLASSNAME)
        .that(testByteString.isBalanced())
        .isTrue();
    assertThat(testByteString.allocated).isNull();
  }

  @Test
  public void testCopyTo_ByteArrayOffsetLength_AllBytes() {
    int destinationOffset = 50;
    int length = BYTES.length;
    byte[] destination = new byte[destinationOffset + length];
    int sourceOffset = 0;
    testByteString.copyTo(destination, sourceOffset, destinationOffset, length);
    boolean stillEqual = true;
    for (int i = 0; stillEqual && i < length; ++i) {
      stillEqual = BYTES[i + sourceOffset] == destination[i + destinationOffset];
    }
    assertWithMessage("%s.copyTo(4 arg) must give the expected bytes", CLASSNAME)
        .that(stillEqual)
        .isTrue();
    assertThat(testByteString.allocated).isNull();
  }

  @Test
  public void testCopyTo_ByteArrayOffsetLength_SomeBytes() {
    int destinationOffset = 50;
    int length = 100;
    byte[] destination = new byte[destinationOffset + length];
    int sourceOffset = 213;
    testByteString.copyTo(destination, sourceOffset, destinationOffset, length);
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
      testByteString.copyTo(
          destination, testByteString.size() + 1 - length, destinationOffset, length);
      assertWithMessage(
              "Should have thrown an exception when copying too many bytes of a %s", CLASSNAME)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative sourceOffset
      testByteString.copyTo(destination, -1, destinationOffset, length);
      assertWithMessage(
              "Should have thrown an exception when given a negative sourceOffset in %s ",
              CLASSNAME)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative destinationOffset
      testByteString.copyTo(destination, 0, -1, length);
      assertWithMessage(
              "Should have thrown an exception when given a negative destinationOffset in %s",
              CLASSNAME)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative size
      testByteString.copyTo(destination, 0, 0, -1);
      assertWithMessage(
              "Should have thrown an exception when given a negative size in %s", CLASSNAME)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal too-large sourceOffset
      testByteString.copyTo(destination, 2 * testByteString.size(), 0, length);
      assertWithMessage(
              "Should have thrown an exception when the destinationOffset is too large in %s",
              CLASSNAME)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal too-large destinationOffset
      testByteString.copyTo(destination, 0, 2 * destination.length, length);
      assertWithMessage(
              "Should have thrown an exception when the destinationOffset is too large in %s",
              CLASSNAME)
          .fail();
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    assertThat(testByteString.allocated).isNull();
  }

  @Test
  public void testCopyTo_ByteBuffer() {
    ByteBuffer backingBuffer = ByteBuffer.wrap(BYTES);

    // Same length.
    ByteBuffer myBuffer = ByteBuffer.allocate(BYTES.length);
    testByteString.copyTo(myBuffer);
    myBuffer.flip();
    assertWithMessage("%s.copyTo(ByteBuffer) must give back the same bytes", CLASSNAME)
        .that(backingBuffer)
        .isEqualTo(myBuffer);

    // Target buffer bigger than required.
    myBuffer = ByteBuffer.allocate(testByteString.size() + 1);
    testByteString.copyTo(myBuffer);
    myBuffer.flip();
    assertThat(backingBuffer).isEqualTo(myBuffer);

    // Target buffer has no space.
    myBuffer = ByteBuffer.allocate(0);
    try {
      testByteString.copyTo(myBuffer);
      assertWithMessage(
              "Should have thrown an exception when target ByteBuffer has insufficient capacity")
          .fail();
    } catch (BufferOverflowException e) {
      // Expected.
    }

    // Target buffer too small.
    myBuffer = ByteBuffer.allocate(1);
    try {
      testByteString.copyTo(myBuffer);
      assertWithMessage(
              "Should have thrown an exception when target ByteBuffer has insufficient capacity")
          .fail();
    } catch (BufferOverflowException e) {
      // Expected.
    }

    assertThat(testByteString.allocated).isNull();
  }

  @Test
  public void testMarkSupported() {
    InputStream stream = testByteString.newInput();
    assertWithMessage("%s.newInput() must support marking", CLASSNAME)
        .that(stream.markSupported())
        .isTrue();
  }

  @Test
  public void testMarkAndReset() throws IOException {
    int fraction = testByteString.size() / 3;

    InputStream stream = testByteString.newInput();
    stream.mark(testByteString.size()); // First, mark() the end.

    skipFully(stream, fraction); // Skip a large fraction, but not all.
    assertWithMessage("%s: after skipping to the 'middle', half the bytes are available", CLASSNAME)
        .that((testByteString.size() - fraction))
        .isEqualTo(stream.available());
    stream.reset();
    assertWithMessage("%s: after resetting, all bytes are available", CLASSNAME)
        .that(testByteString.size())
        .isEqualTo(stream.available());

    skipFully(stream, testByteString.size()); // Skip to the end.
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
    ByteBuffer byteBuffer = testByteString.asReadOnlyByteBuffer();
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
    List<ByteBuffer> byteBuffers = testByteString.asReadOnlyByteBufferList();
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
    byte[] roundTripBytes = testByteString.toByteArray();
    assertWithMessage("%s.toByteArray() must give back the same bytes", CLASSNAME)
        .that(Arrays.equals(BYTES, roundTripBytes))
        .isTrue();
    assertThat(testByteString.allocated).isNull();
  }

  @Test
  public void testWriteTo() throws IOException {
    ByteArrayOutputStream bos = new ByteArrayOutputStream();
    testByteString.writeTo(bos);
    byte[] roundTripBytes = bos.toByteArray();
    assertWithMessage("%s.writeTo() must give back the same bytes", CLASSNAME)
        .that(Arrays.equals(BYTES, roundTripBytes))
        .isTrue();
    assertThat(testByteString.allocated).isNull();
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
    testByteString.writeTo(os);
    assertWithMessage("%s.writeTo() must NOT grant access to underlying buffer", CLASSNAME)
        .that(Arrays.equals(original, BYTES))
        .isTrue();
    assertThat(testByteString.allocated).isNull();
  }

  @Test
  public void testNewOutput() throws IOException {
    ByteArrayOutputStream bos = new ByteArrayOutputStream();
    ByteString.Output output = ByteString.newOutput();
    testByteString.writeTo(output);
    assertWithMessage("Output Size returns correct result")
        .that(output.size())
        .isEqualTo(testByteString.size());
    output.writeTo(bos);
    assertWithMessage("Output.writeTo() must give back the same bytes")
        .that(Arrays.equals(BYTES, bos.toByteArray()))
        .isTrue();

    // write the output stream to itself! This should cause it to double
    output.writeTo(output);
    assertWithMessage("Writing an output stream to itself is successful")
        // Avoid testing directly on the concat'd stream to avoid the test triggering allocation.
        // .that(testByteString.concat(testByteString))
        .that(ByteString.copyFrom(testByteString.concat(testByteString).toByteArray()))
        .isEqualTo(output.toByteString());

    output.reset();
    assertWithMessage("Output.reset() resets the output").that(output.size()).isEqualTo(0);
    assertWithMessage("Output.reset() resets the output")
        .that(output.toByteString())
        .isEqualTo(EMPTY);
    assertThat(testByteString.allocated).isNull();
  }

  @Test
  public void testToString_returnsCanonicalEmptyString() {
    assertWithMessage("%s must be the same string references", CLASSNAME)
        .that(EMPTY.toString(UTF_8))
        .isSameInstanceAs(ByteString.wrap(EMPTY_MSG).toString(UTF_8));
    assertThat(testByteString.allocated).isNull();
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
      testByteString.toString("invalid");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (UnsupportedEncodingException expected) {
      // This is success
    }
    assertThat(testByteString.allocated).isNull();
  }

  @Test
  @SuppressWarnings("TruthSelfEquals")
  public void testEquals() {
    assertWithMessage("%s must not equal null", CLASSNAME).that(testByteString).isNotEqualTo(null);
    assertWithMessage("%s must equal self", CLASSNAME)
        .that(testByteString)
        .isEqualTo(testByteString);
    assertWithMessage("%s must not equal the empty string", CLASSNAME)
        .that(testByteString)
        .isNotEqualTo(EMPTY);
    assertWithMessage("%s empty strings must be equal", CLASSNAME)
        .that(EMPTY)
        .isEqualTo(testByteString.substring(55, 55));
    assertWithMessage("%s must equal another string with the same value", CLASSNAME)
        .that(testByteString)
        .isEqualTo(ByteString.wrap(MSG));
    assertWithMessage("%s must equal another string with the same value", CLASSNAME)
        .that(testByteString)
        .isEqualTo(MSG.toByteString());
    assertWithMessage("%s must equal another string with the same value", CLASSNAME)
        .that(testByteString)
        .isEqualTo(ByteString.wrap(MSG.toByteArray()));

    byte[] mungedBytes = mungedBytes();
    assertWithMessage("%s must not equal every string with the same length", CLASSNAME)
        .that(testByteString.equals(ByteString.wrap(mungedBytes)))
        .isFalse();
  }

  @Test
  public void testEqualsLiteralByteString() {
    ByteString literal = ByteString.copyFrom(BYTES);
    assertWithMessage("%s must equal LiteralByteString with same value", CLASSNAME)
        .that(literal)
        .isEqualTo(testByteString);
    assertWithMessage("%s must equal LiteralByteString with same value", CLASSNAME)
        .that(testByteString)
        .isEqualTo(literal);
    assertWithMessage("%s must not equal the empty string", CLASSNAME)
        .that(testByteString)
        .isNotEqualTo(ByteString.EMPTY);
    assertWithMessage("%s empty strings must be equal", CLASSNAME)
        .that(ByteString.EMPTY)
        .isEqualTo(testByteString.substring(55, 55));

    literal = ByteString.copyFrom(mungedBytes());
    assertWithMessage("%s must not equal every LiteralByteString with the same length", CLASSNAME)
        .that(testByteString)
        .isNotEqualTo(literal);
    assertWithMessage("%s must not equal every LiteralByteString with the same length", CLASSNAME)
        .that(literal)
        .isNotEqualTo(testByteString);
  }

  @Test
  public void testEqualsRopeByteString() {
    ByteString p1 = ByteString.copyFrom(BYTES, 0, 5);
    ByteString p2 = ByteString.copyFrom(BYTES, 5, BYTES.length - 5);
    ByteString rope = p1.concat(p2);

    assertWithMessage("%s must equal RopeByteString with same value", CLASSNAME)
        .that(rope)
        .isEqualTo(testByteString);
    assertWithMessage("%s must equal RopeByteString with same value", CLASSNAME)
        .that(testByteString)
        .isEqualTo(rope);
    assertWithMessage("%s must not equal the empty string", CLASSNAME)
        .that(testByteString)
        .isNotEqualTo(ByteString.EMPTY.concat(ByteString.EMPTY));
    assertWithMessage("%s empty strings must be equal", CLASSNAME)
        .that(ByteString.EMPTY.concat(ByteString.EMPTY))
        .isEqualTo(testByteString.substring(55, 55));

    byte[] mungedBytes = mungedBytes();
    p1 = ByteString.copyFrom(mungedBytes, 0, 5);
    p2 = ByteString.copyFrom(mungedBytes, 5, mungedBytes.length - 5);
    rope = p1.concat(p2);
    assertWithMessage("%s must not equal every RopeByteString with the same length", CLASSNAME)
        .that(testByteString)
        .isNotEqualTo(rope);

    assertWithMessage("%s must not equal every RopeByteString with the same length", CLASSNAME)
        .that(rope)
        .isNotEqualTo(testByteString);

    // Exact roped copies should be equal
    ByteString a = testByteString.concat(testByteString);
    ByteString b = testByteString.concat(testByteString);
    assertThat(a).isEqualTo(b);

    // Exact roped copies that aren't delegating to MessageByteString should _also_ be equal.
    a = ByteString.copyFrom(BYTES).concat(ByteString.copyFrom(BYTES));
    assertThat(a).isEqualTo(b);
  }

  private byte[] mungedBytes() {
    byte[] mungedBytes = new byte[BYTES.length];
    System.arraycopy(BYTES, 0, mungedBytes, 0, BYTES.length);
    mungedBytes[mungedBytes.length - 5] = (byte) (mungedBytes[mungedBytes.length - 5] ^ 0xFF);
    return mungedBytes;
  }

  @Test
  public void testHashCode() {
    int hash = testByteString.hashCode();
    assertWithMessage("%s must have expected hashCode", CLASSNAME)
        .that(hash)
        .isEqualTo(EXPECTED_HASH);
  }

  @Test
  public void testPeekCachedHashCode() {
    ByteString newString = ByteString.wrap(MSG);
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
    int hash = testByteString.partialHash(testByteString.size(), 0, testByteString.size());
    assertWithMessage("%s.partialHash() must yield expected hashCode", CLASSNAME)
        .that(hash)
        .isEqualTo(EXPECTED_HASH);
  }

  @Test
  public void testNewInput() throws IOException {
    InputStream input = testByteString.newInput();
    assertWithMessage("InputStream.available() returns correct value")
        .that(testByteString.size())
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
    InputStream input = testByteString.newInput();
    int stringSize = testByteString.size();
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
        .isEqualTo(testByteString.byteAt(nearEndIndex) & 0xFF);
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
        .isEqualTo(testByteString.byteAt(nearEndIndex) & 0xFF);
  }

  @Test
  public void testNewCodedInput() throws IOException {
    CodedInputStream cis = testByteString.newCodedInput();
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
        .that(testByteString.concat(EMPTY))
        .isSameInstanceAs(testByteString);
    assertWithMessage("empty concatenated with %s must give %s", CLASSNAME, CLASSNAME)
        .that(EMPTY.concat(testByteString))
        .isSameInstanceAs(testByteString);
    assertThat(testByteString.allocated).isNull();
  }

  @Test
  public void testJavaSerialization() throws Exception {
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    ObjectOutputStream oos = new ObjectOutputStream(out);
    oos.writeObject(testByteString);
    oos.close();
    byte[] pickled = out.toByteArray();
    InputStream in = new ByteArrayInputStream(pickled);
    ObjectInputStream ois = new ObjectInputStream(in);
    Object o = ois.readObject();
    assertWithMessage("Didn't get a ByteString back").that(o).isInstanceOf(ByteString.class);
    assertWithMessage("Should get an equal ByteString back").that(o).isEqualTo(testByteString);
  }

  @Test
  public void testUsedAsBytesField() throws Exception {
    TopLevelMessage msgFromWrapper =
        TopLevelMessage.newBuilder().setFieldBytes(BytesValue.of(testByteString)).build();
    TopLevelMessage msgFromBytes =
        TopLevelMessage.newBuilder().setFieldBytes(BytesValue.of(MSG.toByteString())).build();
    assertThat(msgFromWrapper.toByteString()).isEqualTo(msgFromBytes.toByteString());
    assertThat(testByteString.allocated).isNull();

    // These currently do allocations, so are after the above assertions.
    assertThat(msgFromWrapper).isEqualTo(msgFromBytes);
    assertThat(msgFromWrapper.hashCode()).isEqualTo(msgFromBytes.hashCode());
  }

  @Test
  public void testEqualWrappedBytesFields() throws Exception {
    TopLevelMessage wrapper1 =
        TopLevelMessage.newBuilder().setFieldBytes(BytesValue.of(testByteString)).build();
    ByteString.MessageByteString newTestByteString =
        (ByteString.MessageByteString)
            ByteString.wrap(
                TopLevelMessage.parseFrom(BYTES, ExtensionRegistryLite.getEmptyRegistry()));
    TopLevelMessage wrapper2 =
        TopLevelMessage.newBuilder().setFieldBytes(BytesValue.of(newTestByteString)).build();

    assertThat(wrapper1).isEqualTo(wrapper2);
    assertThat(testByteString.allocated).isNotNull();
    assertThat(newTestByteString.allocated).isNotNull();
  }

  @Test
  public void testNotEqualWrappedBytesFields() throws Exception {
    TopLevelMessage wrapper1 =
        TopLevelMessage.newBuilder().setFieldBytes(BytesValue.of(testByteString)).build();
    TopLevelMessage message =
        TopLevelMessage.newBuilder()
            .setFieldBytes(
                BytesValue.of(ByteString.copyFrom(ByteStringTest.getTestBytes(1234, 11337767L))))
            .build();
    ByteString.MessageByteString newTestByteString =
        (ByteString.MessageByteString) ByteString.wrap(message);
    TopLevelMessage wrapper2 =
        TopLevelMessage.newBuilder().setFieldBytes(BytesValue.of(newTestByteString)).build();

    assertThat(wrapper1).isNotEqualTo(wrapper2);
    assertThat(testByteString.allocated).isNotNull();
    assertThat(newTestByteString.allocated).isNotNull();
  }

  @Test
  public void testEqualAndSameMsgInstanceWrappedBytesFields() throws Exception {
    TopLevelMessage wrapper1 =
        TopLevelMessage.newBuilder().setFieldBytes(BytesValue.of(testByteString)).build();
    ByteString.MessageByteString newTestByteString =
        (ByteString.MessageByteString) ByteString.wrap(MSG);
    TopLevelMessage wrapper2 =
        TopLevelMessage.newBuilder().setFieldBytes(BytesValue.of(newTestByteString)).build();

    assertThat(wrapper1).isEqualTo(wrapper2);
    assertThat(testByteString.allocated).isNull();
    assertThat(newTestByteString.allocated).isNull();
  }

  @Test
  public void testDifferentSizedWrappedBytesFields() throws Exception {
    TopLevelMessage wrapper =
        TopLevelMessage.newBuilder().setFieldBytes(BytesValue.of(testByteString)).build();
    TopLevelMessage differentMsg =
        TopLevelMessage.newBuilder()
            .setFieldBytes(BytesValue.of(ByteString.copyFrom(ByteStringTest.getTestBytes(10, 15L))))
            .build();
    ByteString.MessageByteString differenttestByteString =
        (ByteString.MessageByteString) ByteString.wrap(differentMsg);
    TopLevelMessage differentWrapper =
        TopLevelMessage.newBuilder().setFieldBytes(BytesValue.of(differenttestByteString)).build();

    assertThat(testByteString.size()).isNotEqualTo(differenttestByteString.size());
    assertThat(wrapper).isNotEqualTo(differentWrapper);
    assertThat(testByteString.allocated).isNull();
    assertThat(differenttestByteString.allocated).isNull();
  }
}
