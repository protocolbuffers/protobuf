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

import com.google.protobuf.ByteString.Output;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Field;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.Random;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Test methods with implementations in {@link ByteString}, plus do some top-level "integration"
 * tests.
 */
@RunWith(JUnit4.class)
public class ByteStringTest {

  private static final Charset UTF_16 = Charset.forName("UTF-16");

  static byte[] getTestBytes(int size, long seed) {
    Random random = new Random(seed);
    byte[] result = new byte[size];
    random.nextBytes(result);
    return result;
  }

  private static byte[] getTestBytes(int size) {
    return getTestBytes(size, 445566L);
  }

  private static byte[] getTestBytes() {
    return getTestBytes(1000);
  }

  // Compare the entire left array with a subset of the right array.
  private static boolean isArrayRange(byte[] left, byte[] right, int rightOffset, int length) {
    boolean stillEqual = (left.length == length);
    for (int i = 0; (stillEqual && i < length); ++i) {
      stillEqual = (left[i] == right[rightOffset + i]);
    }
    return stillEqual;
  }

  // Returns true only if the given two arrays have identical contents.
  private static boolean isArray(byte[] left, byte[] right) {
    return left.length == right.length && isArrayRange(left, right, 0, left.length);
  }

  @Test
  public void testCompare_equalByteStrings_compareEqual() throws Exception {
    byte[] referenceBytes = getTestBytes();
    ByteString string1 = ByteString.copyFrom(referenceBytes);
    ByteString string2 = ByteString.copyFrom(referenceBytes);

    assertWithMessage("ByteString instances containing the same data must compare equal.")
        .that(ByteString.unsignedLexicographicalComparator().compare(string1, string2))
        .isEqualTo(0);
  }

  @Test
  public void testCompare_byteStringsSortLexicographically() throws Exception {
    ByteString app = ByteString.copyFromUtf8("app");
    ByteString apple = ByteString.copyFromUtf8("apple");
    ByteString banana = ByteString.copyFromUtf8("banana");

    Comparator<ByteString> comparator = ByteString.unsignedLexicographicalComparator();

    assertWithMessage("ByteString(app) < ByteString(apple)")
        .that(comparator.compare(app, apple) < 0)
        .isTrue();
    assertWithMessage("ByteString(app) < ByteString(banana)")
        .that(comparator.compare(app, banana) < 0)
        .isTrue();
    assertWithMessage("ByteString(apple) < ByteString(banana)")
        .that(comparator.compare(apple, banana) < 0)
        .isTrue();
  }

  @Test
  public void testCompare_interpretsByteValuesAsUnsigned() throws Exception {
    // Two's compliment of `-1` == 0b11111111 == 255
    ByteString twoHundredFiftyFive = ByteString.copyFrom(new byte[] {-1});
    // 0b00000001 == 1
    ByteString one = ByteString.copyFrom(new byte[] {1});

    assertWithMessage("ByteString comparison treats bytes as unsigned values")
        .that(ByteString.unsignedLexicographicalComparator().compare(one, twoHundredFiftyFive) < 0)
        .isTrue();
  }

  @Test
  public void testSubstring_beginIndex() {
    byte[] bytes = getTestBytes();
    ByteString substring = ByteString.copyFrom(bytes).substring(500);
    assertWithMessage("substring must contain the tail of the string")
        .that(isArrayRange(substring.toByteArray(), bytes, 500, bytes.length - 500))
        .isTrue();
  }

  @Test
  public void testEmpty_isEmpty() {
    ByteString byteString = ByteString.empty();
    assertThat(byteString.isEmpty()).isTrue();
    assertWithMessage("ByteString.empty() must return empty byte array")
        .that(isArray(byteString.toByteArray(), new byte[] {}))
        .isTrue();
  }

  @Test
  public void testEmpty_referenceEquality() {
    assertThat(ByteString.empty()).isSameInstanceAs(ByteString.EMPTY);
    assertThat(ByteString.empty()).isSameInstanceAs(ByteString.empty());
  }

  @Test
  public void testFromHex_hexString() {
    ByteString byteString;
    byteString = ByteString.fromHex("0a0b0c");
    assertWithMessage("fromHex must contain the expected bytes")
        .that(isArray(byteString.toByteArray(), new byte[] {0x0a, 0x0b, 0x0c}))
        .isTrue();

    byteString = ByteString.fromHex("0A0B0C");
    assertWithMessage("fromHex must contain the expected bytes")
        .that(isArray(byteString.toByteArray(), new byte[] {0x0a, 0x0b, 0x0c}))
        .isTrue();

    byteString = ByteString.fromHex("0a0b0c0d0e0f");
    assertWithMessage("fromHex must contain the expected bytes")
        .that(isArray(byteString.toByteArray(), new byte[] {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f}))
        .isTrue();
  }

  @Test
  @SuppressWarnings("AlwaysThrows") // Verifying that indeed these calls do throw.
  public void testFromHex_invalidHexString() {
    try {
      ByteString.fromHex("a0b0c");
      assertWithMessage("Should throw").fail();
    } catch (NumberFormatException expected) {
      assertThat(expected).hasMessageThat().contains("even");
    }

    try {
      ByteString.fromHex("0x0y0z");
      assertWithMessage("Should throw").fail();
    } catch (NumberFormatException expected) {
      assertThat(expected).hasMessageThat().contains("[0-9a-fA-F]");
    }

    try {
      ByteString.fromHex("0૫");
      assertWithMessage("Should throw").fail();
    } catch (NumberFormatException expected) {
      assertThat(expected).hasMessageThat().contains("[0-9a-fA-F]");
    }
  }

  @Test
  public void testCopyFrom_bytesOffsetSize() {
    byte[] bytes = getTestBytes();
    ByteString byteString = ByteString.copyFrom(bytes, 500, 200);
    assertWithMessage("copyFrom sub-range must contain the expected bytes")
        .that(isArrayRange(byteString.toByteArray(), bytes, 500, 200))
        .isTrue();
  }

  @Test
  public void testCopyFrom_bytes() {
    byte[] bytes = getTestBytes();
    ByteString byteString = ByteString.copyFrom(bytes);
    assertWithMessage("copyFrom must contain the expected bytes")
        .that(isArray(byteString.toByteArray(), bytes))
        .isTrue();
  }

  @Test
  public void testCopyFrom_byteBufferSize() {
    byte[] bytes = getTestBytes();
    ByteBuffer byteBuffer = ByteBuffer.allocate(bytes.length);
    byteBuffer.put(bytes);
    byteBuffer.position(500);
    ByteString byteString = ByteString.copyFrom(byteBuffer, 200);
    assertWithMessage("copyFrom byteBuffer sub-range must contain the expected bytes")
        .that(isArrayRange(byteString.toByteArray(), bytes, 500, 200))
        .isTrue();
  }

  @Test
  public void testCopyFrom_byteBuffer() {
    byte[] bytes = getTestBytes();
    ByteBuffer byteBuffer = ByteBuffer.allocate(bytes.length);
    byteBuffer.put(bytes);
    byteBuffer.position(500);
    ByteString byteString = ByteString.copyFrom(byteBuffer);
    assertWithMessage("copyFrom byteBuffer sub-range must contain the expected bytes")
        .that(isArrayRange(byteString.toByteArray(), bytes, 500, bytes.length - 500))
        .isTrue();
  }

  @Test
  public void testCopyFrom_stringEncoding() {
    String testString = "I love unicode \u1234\u5678 characters";
    ByteString byteString = ByteString.copyFrom(testString, UTF_16);
    byte[] testBytes = testString.getBytes(UTF_16);
    assertWithMessage("copyFrom string must respect the charset")
        .that(isArrayRange(byteString.toByteArray(), testBytes, 0, testBytes.length))
        .isTrue();
  }

  @Test
  public void testCopyFrom_utf8() {
    String testString = "I love unicode \u1234\u5678 characters";
    ByteString byteString = ByteString.copyFromUtf8(testString);
    byte[] testBytes = testString.getBytes(Internal.UTF_8);
    assertWithMessage("copyFromUtf8 string must respect the charset")
        .that(isArrayRange(byteString.toByteArray(), testBytes, 0, testBytes.length))
        .isTrue();
  }

  @Test
  public void testCopyFrom_iterable() {
    byte[] testBytes = getTestBytes(77777, 113344L);
    final List<ByteString> pieces = makeConcretePieces(testBytes);
    // Call copyFrom() on a Collection
    ByteString byteString = ByteString.copyFrom(pieces);
    assertWithMessage("copyFrom a List must contain the expected bytes")
        .that(isArrayRange(byteString.toByteArray(), testBytes, 0, testBytes.length))
        .isTrue();
    // Call copyFrom on an iteration that's not a collection
    ByteString byteStringAlt =
        ByteString.copyFrom(
            new Iterable<ByteString>() {
              @Override
              public Iterator<ByteString> iterator() {
                return pieces.iterator();
              }
            });
    assertWithMessage("copyFrom from an Iteration must contain the expected bytes")
        .that(byteString)
        .isEqualTo(byteStringAlt);
  }

  @Test
  public void testCopyFrom_lengthTooBig() {
    byte[] testBytes = getTestBytes(100);
    try {
      ByteString.copyFrom(testBytes, 0, 200);
      assertWithMessage("Should throw").fail();
    } catch (IndexOutOfBoundsException expected) {
    }

    try {
      ByteString.copyFrom(testBytes, 99, 2);
      assertWithMessage("Should throw").fail();
    } catch (IndexOutOfBoundsException expected) {
    }

    ByteBuffer buf = ByteBuffer.wrap(testBytes);
    try {
      ByteString.copyFrom(buf, 101);
      assertWithMessage("Should throw").fail();
    } catch (IndexOutOfBoundsException expected) {
    }

    try {
      ByteString.copyFrom(testBytes, -1, 10);
      assertWithMessage("Should throw").fail();
    } catch (IndexOutOfBoundsException expected) {
    }
  }

  @Test
  public void testCopyTo_targetOffset() {
    byte[] bytes = getTestBytes();
    ByteString byteString = ByteString.copyFrom(bytes);
    byte[] target = new byte[bytes.length + 1000];
    byteString.copyTo(target, 400);
    assertWithMessage("copyFrom byteBuffer sub-range must contain the expected bytes")
        .that(isArrayRange(bytes, target, 400, bytes.length))
        .isTrue();
  }

  @Test
  public void testReadFrom_emptyStream() throws IOException {
    ByteString byteString = ByteString.readFrom(new ByteArrayInputStream(new byte[0]));
    assertWithMessage("reading an empty stream must result in the EMPTY constant byte string")
        .that(ByteString.EMPTY)
        .isSameInstanceAs(byteString);
  }

  @Test
  public void testReadFrom_smallStream() throws IOException {
    assertReadFrom(getTestBytes(10));
  }

  @Test
  public void testReadFrom_mutating() throws IOException {
    EvilInputStream eis = new EvilInputStream();
    ByteString byteString = ByteString.readFrom(eis);
    byte[] capturedArray = eis.capturedArray;

    byte[] originalValue = byteString.toByteArray();
    for (int x = 0; x < capturedArray.length; ++x) {
      capturedArray[x] = (byte) 0;
    }

    byte[] newValue = byteString.toByteArray();
    assertWithMessage("copyFrom byteBuffer must not grant access to underlying array")
        .that(Arrays.equals(originalValue, newValue))
        .isTrue();
  }

  // Tests sizes that are near the rope copy-out threshold.
  @Test
  public void testReadFrom_mediumStream() throws IOException {
    assertReadFrom(getTestBytes(ByteString.CONCATENATE_BY_COPY_SIZE - 1));
    assertReadFrom(getTestBytes(ByteString.CONCATENATE_BY_COPY_SIZE));
    assertReadFrom(getTestBytes(ByteString.CONCATENATE_BY_COPY_SIZE + 1));
    assertReadFrom(getTestBytes(200));
  }

  // Tests sizes that are over multi-segment rope threshold.
  @Test
  public void testReadFrom_largeStream() throws IOException {
    assertReadFrom(getTestBytes(0x100));
    assertReadFrom(getTestBytes(0x101));
    assertReadFrom(getTestBytes(0x110));
    assertReadFrom(getTestBytes(0x1000));
    assertReadFrom(getTestBytes(0x1001));
    assertReadFrom(getTestBytes(0x1010));
    assertReadFrom(getTestBytes(0x10000));
    assertReadFrom(getTestBytes(0x10001));
    assertReadFrom(getTestBytes(0x10010));
  }

  // Tests sizes that are near the read buffer size.
  @Test
  public void testReadFrom_byteBoundaries() throws IOException {
    final int min = ByteString.MIN_READ_FROM_CHUNK_SIZE;
    final int max = ByteString.MAX_READ_FROM_CHUNK_SIZE;

    assertReadFrom(getTestBytes(min - 1));
    assertReadFrom(getTestBytes(min));
    assertReadFrom(getTestBytes(min + 1));

    assertReadFrom(getTestBytes(min * 2 - 1));
    assertReadFrom(getTestBytes(min * 2));
    assertReadFrom(getTestBytes(min * 2 + 1));

    assertReadFrom(getTestBytes(min * 4 - 1));
    assertReadFrom(getTestBytes(min * 4));
    assertReadFrom(getTestBytes(min * 4 + 1));

    assertReadFrom(getTestBytes(min * 8 - 1));
    assertReadFrom(getTestBytes(min * 8));
    assertReadFrom(getTestBytes(min * 8 + 1));

    assertReadFrom(getTestBytes(max - 1));
    assertReadFrom(getTestBytes(max));
    assertReadFrom(getTestBytes(max + 1));

    assertReadFrom(getTestBytes(max * 2 - 1));
    assertReadFrom(getTestBytes(max * 2));
    assertReadFrom(getTestBytes(max * 2 + 1));
  }

  // Tests that IOExceptions propagate through ByteString.readFrom().
  @Test
  public void testReadFrom_iOExceptions() {
    try {
      ByteString.readFrom(new FailStream());
      assertWithMessage("readFrom must throw the underlying IOException").fail();

    } catch (IOException e) {
      assertWithMessage("readFrom must throw the expected exception")
          .that(e)
          .hasMessageThat()
          .isEqualTo("synthetic failure");
    }
  }

  // Tests that ByteString.readFrom works with streams that don't
  // always fill their buffers.
  @Test
  public void testReadFrom_reluctantStream() throws IOException {
    final byte[] data = getTestBytes(0x1000);

    ByteString byteString = ByteString.readFrom(new ReluctantStream(data));
    assertWithMessage("readFrom byte stream must contain the expected bytes")
        .that(isArray(byteString.toByteArray(), data))
        .isTrue();

    // Same test as above, but with some specific chunk sizes.
    assertReadFromReluctantStream(data, 100);
    assertReadFromReluctantStream(data, 248);
    assertReadFromReluctantStream(data, 249);
    assertReadFromReluctantStream(data, 250);
    assertReadFromReluctantStream(data, 251);
    assertReadFromReluctantStream(data, 0x1000);
    assertReadFromReluctantStream(data, 0x1001);
  }

  // Fails unless ByteString.readFrom reads the bytes correctly from a
  // reluctant stream with the given chunkSize parameter.
  private void assertReadFromReluctantStream(byte[] bytes, int chunkSize) throws IOException {
    ByteString b = ByteString.readFrom(new ReluctantStream(bytes), chunkSize);
    assertWithMessage("readFrom byte stream must contain the expected bytes")
        .that(isArray(b.toByteArray(), bytes))
        .isTrue();
  }

  // Tests that ByteString.readFrom works with streams that implement
  // available().
  @Test
  public void testReadFrom_available() throws IOException {
    final byte[] data = getTestBytes(0x1001);

    ByteString byteString = ByteString.readFrom(new AvailableStream(data));
    assertWithMessage("readFrom byte stream must contain the expected bytes")
        .that(isArray(byteString.toByteArray(), data))
        .isTrue();
  }

  // Fails unless ByteString.readFrom reads the bytes correctly.
  private void assertReadFrom(byte[] bytes) throws IOException {
    ByteString byteString = ByteString.readFrom(new ByteArrayInputStream(bytes));
    assertWithMessage("readFrom byte stream must contain the expected bytes")
        .that(isArray(byteString.toByteArray(), bytes))
        .isTrue();
  }

  // A stream that fails when read.
  private static final class FailStream extends InputStream {
    @Override
    public int read() throws IOException {
      throw new IOException("synthetic failure");
    }
  }

  // A stream that simulates blocking by only producing 250 characters
  // per call to read(byte[]).
  private static class ReluctantStream extends InputStream {
    protected final byte[] data;
    protected int pos = 0;

    public ReluctantStream(byte[] data) {
      this.data = data;
    }

    @Override
    public int read() {
      if (pos == data.length) {
        return -1;
      } else {
        return data[pos++];
      }
    }

    @Override
    public int read(byte[] buf) {
      return read(buf, 0, buf.length);
    }

    @Override
    public int read(byte[] buf, int offset, int size) {
      if (pos == data.length) {
        return -1;
      }
      int count = Math.min(Math.min(size, data.length - pos), 250);
      System.arraycopy(data, pos, buf, offset, count);
      pos += count;
      return count;
    }
  }

  // Same as above, but also implements available().
  private static final class AvailableStream extends ReluctantStream {
    public AvailableStream(byte[] data) {
      super(data);
    }

    @Override
    public int available() {
      return Math.min(250, data.length - pos);
    }
  }

  // A stream which exposes the byte array passed into read(byte[], int, int).
  private static class EvilInputStream extends InputStream {
    public byte[] capturedArray = null;

    @Override
    public int read(byte[] buf, int off, int len) {
      if (capturedArray != null) {
        return -1;
      } else {
        capturedArray = buf;
        for (int x = 0; x < len; ++x) {
          buf[x] = (byte) x;
        }
        return len;
      }
    }

    @Override
    public int read() {
      // Purposefully do nothing.
      return -1;
    }
  }

  // A stream which exposes the byte array passed into write(byte[], int, int).
  private static class EvilOutputStream extends OutputStream {
    public byte[] capturedArray = null;

    @Override
    public void write(byte[] buf, int off, int len) {
      if (capturedArray == null) {
        capturedArray = buf;
      }
    }

    @Override
    public void write(int ignored) {
      // Purposefully do nothing.
    }
  }

  @Test
  public void testToStringUtf8() {
    String testString = "I love unicode \u1234\u5678 characters";
    byte[] testBytes = testString.getBytes(Internal.UTF_8);
    ByteString byteString = ByteString.copyFrom(testBytes);
    assertWithMessage("copyToStringUtf8 must respect the charset")
        .that(testString)
        .isEqualTo(byteString.toStringUtf8());
  }

  @Test
  public void testToString() {
    String toString =
        ByteString.copyFrom("Here are some bytes: \t\u00a1".getBytes(Internal.UTF_8)).toString();
    assertWithMessage(toString).that(toString.contains("size=24")).isTrue();
    assertWithMessage(toString)
        .that(toString.contains("contents=\"Here are some bytes: \\t\\302\\241\""))
        .isTrue();
  }

  @Test
  public void testToString_long() {
    String toString =
        ByteString.copyFrom(
                "123456789012345678901234567890123456789012345678901234567890"
                    .getBytes(Internal.UTF_8))
            .toString();
    assertWithMessage(toString).that(toString.contains("size=60")).isTrue();
    assertWithMessage(toString)
        .that(toString.contains("contents=\"12345678901234567890123456789012345678901234567...\""))
        .isTrue();
  }

  @Test
  public void testNewOutput_initialCapacity() throws IOException {
    byte[] bytes = getTestBytes();
    ByteString.Output output = ByteString.newOutput(bytes.length + 100);
    output.write(bytes);
    ByteString byteString = output.toByteString();
    assertWithMessage("String built from newOutput(int) must contain the expected bytes")
        .that(isArrayRange(bytes, byteString.toByteArray(), 0, bytes.length))
        .isTrue();
  }

  // Test newOutput() using a variety of buffer sizes and a variety of (fixed)
  // write sizes
  @Test
  public void testNewOutput_arrayWrite() {
    byte[] bytes = getTestBytes();
    int length = bytes.length;
    int[] bufferSizes = {
      128, 256, length / 2, length - 1, length, length + 1, 2 * length, 3 * length
    };
    int[] writeSizes = {1, 4, 5, 7, 23, bytes.length};

    for (int bufferSize : bufferSizes) {
      for (int writeSize : writeSizes) {
        // Test writing the entire output writeSize bytes at a time.
        ByteString.Output output = ByteString.newOutput(bufferSize);
        for (int i = 0; i < length; i += writeSize) {
          output.write(bytes, i, Math.min(writeSize, length - i));
        }
        ByteString byteString = output.toByteString();
        assertWithMessage("String built from newOutput() must contain the expected bytes")
            .that(isArrayRange(bytes, byteString.toByteArray(), 0, bytes.length))
            .isTrue();
      }
    }
  }

  // Test newOutput() using a variety of buffer sizes, but writing all the
  // characters using write(byte);
  @Test
  public void testNewOutput_writeChar() {
    byte[] bytes = getTestBytes();
    int length = bytes.length;
    int[] bufferSizes = {
      0, 1, 128, 256, length / 2, length - 1, length, length + 1, 2 * length, 3 * length
    };
    for (int bufferSize : bufferSizes) {
      ByteString.Output output = ByteString.newOutput(bufferSize);
      for (byte byteValue : bytes) {
        output.write(byteValue);
      }
      ByteString byteString = output.toByteString();
      assertWithMessage("String built from newOutput() must contain the expected bytes")
          .that(isArrayRange(bytes, byteString.toByteArray(), 0, bytes.length))
          .isTrue();
    }
  }

  // Test newOutput() in which we write the bytes using a variety of methods
  // and sizes, and in which we repeatedly call toByteString() in the middle.
  @Test
  public void testNewOutput_mixed() {
    Random rng = new Random(1);
    byte[] bytes = getTestBytes();
    int length = bytes.length;
    int[] bufferSizes = {
      0, 1, 128, 256, length / 2, length - 1, length, length + 1, 2 * length, 3 * length
    };

    for (int bufferSize : bufferSizes) {
      // Test writing the entire output using a mixture of write sizes and
      // methods;
      ByteString.Output output = ByteString.newOutput(bufferSize);
      int position = 0;
      while (position < bytes.length) {
        if (rng.nextBoolean()) {
          int count = 1 + rng.nextInt(bytes.length - position);
          output.write(bytes, position, count);
          position += count;
        } else {
          output.write(bytes[position]);
          position++;
        }
        assertWithMessage("size() returns the right value").that(position).isEqualTo(output.size());
        assertWithMessage("newOutput() substring must have correct bytes")
            .that(isArrayRange(output.toByteString().toByteArray(), bytes, 0, position))
            .isTrue();
      }
      ByteString byteString = output.toByteString();
      assertWithMessage("String built from newOutput() must contain the expected bytes")
          .that(isArrayRange(bytes, byteString.toByteArray(), 0, bytes.length))
          .isTrue();
    }
  }

  @Test
  public void testNewOutputEmpty() {
    // Make sure newOutput() correctly builds empty byte strings
    ByteString byteString = ByteString.newOutput().toByteString();
    assertThat(ByteString.EMPTY).isEqualTo(byteString);
  }

  @Test
  public void testNewOutput_mutating() throws IOException {
    Output os = ByteString.newOutput(5);
    os.write(new byte[] {1, 2, 3, 4, 5});
    EvilOutputStream eos = new EvilOutputStream();
    os.writeTo(eos);
    byte[] capturedArray = eos.capturedArray;
    ByteString byteString = os.toByteString();
    byte[] oldValue = byteString.toByteArray();
    Arrays.fill(capturedArray, (byte) 0);
    byte[] newValue = byteString.toByteArray();
    assertWithMessage("Output must not provide access to the underlying byte array")
        .that(Arrays.equals(oldValue, newValue))
        .isTrue();
  }

  @Test
  public void testNewCodedBuilder() throws IOException {
    byte[] bytes = getTestBytes();
    ByteString.CodedBuilder builder = ByteString.newCodedBuilder(bytes.length);
    builder.getCodedOutput().writeRawBytes(bytes);
    ByteString byteString = builder.build();
    assertWithMessage("String built from newCodedBuilder() must contain the expected bytes")
        .that(isArrayRange(bytes, byteString.toByteArray(), 0, bytes.length))
        .isTrue();
  }

  @Test
  public void testSubstringParity() {
    byte[] bigBytes = getTestBytes(2048 * 1024, 113344L);
    int start = 512 * 1024 - 3333;
    int end = 512 * 1024 + 7777;
    ByteString concreteSubstring = ByteString.copyFrom(bigBytes).substring(start, end);
    boolean ok = true;
    for (int i = start; ok && i < end; ++i) {
      ok = (bigBytes[i] == concreteSubstring.byteAt(i - start));
    }
    assertWithMessage("Concrete substring didn't capture the right bytes").that(ok).isTrue();

    ByteString literalString = ByteString.copyFrom(bigBytes, start, end - start);
    assertWithMessage("Substring must be equal to literal string")
        .that(literalString)
        .isEqualTo(concreteSubstring);
    assertWithMessage("Substring must have same hashcode as literal string")
        .that(literalString.hashCode())
        .isEqualTo(concreteSubstring.hashCode());
  }

  @Test
  public void testCompositeSubstring() {
    byte[] referenceBytes = getTestBytes(77748, 113344L);

    List<ByteString> pieces = makeConcretePieces(referenceBytes);
    ByteString listString = ByteString.copyFrom(pieces);

    int from = 1000;
    int to = 40000;
    ByteString compositeSubstring = listString.substring(from, to);
    byte[] substringBytes = compositeSubstring.toByteArray();
    boolean stillEqual = true;
    for (int i = 0; stillEqual && i < to - from; ++i) {
      stillEqual = referenceBytes[from + i] == substringBytes[i];
    }
    assertWithMessage("Substring must return correct bytes").that(stillEqual).isTrue();

    stillEqual = true;
    for (int i = 0; stillEqual && i < to - from; ++i) {
      stillEqual = referenceBytes[from + i] == compositeSubstring.byteAt(i);
    }
    assertWithMessage("Substring must support byteAt() correctly").that(stillEqual).isTrue();

    ByteString literalSubstring = ByteString.copyFrom(referenceBytes, from, to - from);
    assertWithMessage("Composite substring must equal a literal substring over the same bytes")
        .that(literalSubstring)
        .isEqualTo(compositeSubstring);
    assertWithMessage("Literal substring must equal a composite substring over the same bytes")
        .that(compositeSubstring)
        .isEqualTo(literalSubstring);

    assertWithMessage("We must get the same hashcodes for composite and literal substrings")
        .that(literalSubstring.hashCode())
        .isEqualTo(compositeSubstring.hashCode());

    assertWithMessage("We can't be equal to a proper substring")
        .that(compositeSubstring.equals(literalSubstring.substring(0, literalSubstring.size() - 1)))
        .isFalse();
  }

  @Test
  public void testCopyFromList() {
    byte[] referenceBytes = getTestBytes(77748, 113344L);
    ByteString literalString = ByteString.copyFrom(referenceBytes);

    List<ByteString> pieces = makeConcretePieces(referenceBytes);
    ByteString listString = ByteString.copyFrom(pieces);

    assertWithMessage("Composite string must be equal to literal string")
        .that(literalString)
        .isEqualTo(listString);
    assertWithMessage("Composite string must have same hashcode as literal string")
        .that(literalString.hashCode())
        .isEqualTo(listString.hashCode());
  }

  @Test
  public void testConcat() {
    byte[] referenceBytes = getTestBytes(77748, 113344L);
    ByteString literalString = ByteString.copyFrom(referenceBytes);

    List<ByteString> pieces = makeConcretePieces(referenceBytes);

    Iterator<ByteString> iter = pieces.iterator();
    ByteString concatenatedString = iter.next();
    while (iter.hasNext()) {
      concatenatedString = concatenatedString.concat(iter.next());
    }

    assertWithMessage("Concatenated string must be equal to literal string")
        .that(literalString)
        .isEqualTo(concatenatedString);
    assertWithMessage("Concatenated string must have same hashcode as literal string")
        .that(literalString.hashCode())
        .isEqualTo(concatenatedString.hashCode());
  }

  /**
   * Test the Rope implementation can deal with Empty nodes, even though we guard against them. See
   * also {@link LiteralByteStringTest#testConcat_empty()}.
   */
  @Test
  public void testConcat_empty() {
    byte[] referenceBytes = getTestBytes(7748, 113344L);
    ByteString literalString = ByteString.copyFrom(referenceBytes);

    ByteString duo = RopeByteString.newInstanceForTest(literalString, literalString);
    ByteString temp =
        RopeByteString.newInstanceForTest(
            RopeByteString.newInstanceForTest(literalString, ByteString.EMPTY),
            RopeByteString.newInstanceForTest(ByteString.EMPTY, literalString));
    ByteString quintet = RopeByteString.newInstanceForTest(temp, ByteString.EMPTY);

    assertWithMessage("String with concatenated nulls must equal simple concatenate")
        .that(quintet)
        .isEqualTo(duo);
    assertWithMessage("String with concatenated nulls have same hashcode as simple concatenate")
        .that(duo.hashCode())
        .isEqualTo(quintet.hashCode());

    ByteString.ByteIterator duoIter = duo.iterator();
    ByteString.ByteIterator quintetIter = quintet.iterator();
    boolean stillEqual = true;
    while (stillEqual && quintetIter.hasNext()) {
      stillEqual = (duoIter.nextByte() == quintetIter.nextByte());
    }
    assertWithMessage("We must get the same characters by iterating").that(stillEqual).isTrue();
    assertWithMessage("Iterator must be exhausted").that(duoIter.hasNext()).isFalse();
    try {
      duoIter.nextByte();
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (NoSuchElementException e) {
      // This is success
    }
    try {
      quintetIter.nextByte();
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (NoSuchElementException e) {
      // This is success
    }

    // Test that even if we force empty strings in as rope leaves in this
    // configuration, we always get a (possibly Bounded) LiteralByteString
    // for a length 1 substring.
    //
    // It is possible, using the testing factory method to create deeply nested
    // trees of empty leaves, to make a string that will fail this test.
    for (int i = 1; i < duo.size(); ++i) {
      assertWithMessage("Substrings of size() < 2 must not be RopeByteStrings")
          .that(duo.substring(i - 1, i) instanceof ByteString.LeafByteString)
          .isTrue();
    }
    for (int i = 1; i < quintet.size(); ++i) {
      assertWithMessage("Substrings of size() < 2 must not be RopeByteStrings")
          .that(quintet.substring(i - 1, i) instanceof ByteString.LeafByteString)
          .isTrue();
    }
  }

  @Test
  public void testStartsWith() {
    byte[] bytes = getTestBytes(1000, 1234L);
    ByteString string = ByteString.copyFrom(bytes);
    ByteString prefix = ByteString.copyFrom(bytes, 0, 500);
    ByteString suffix = ByteString.copyFrom(bytes, 400, 600);
    assertThat(string.startsWith(ByteString.EMPTY)).isTrue();
    assertThat(string.startsWith(string)).isTrue();
    assertThat(string.startsWith(prefix)).isTrue();
    assertThat(string.startsWith(suffix)).isFalse();
    assertThat(prefix.startsWith(suffix)).isFalse();
    assertThat(suffix.startsWith(prefix)).isFalse();
    assertThat(ByteString.EMPTY.startsWith(prefix)).isFalse();
    assertThat(ByteString.EMPTY.startsWith(ByteString.EMPTY)).isTrue();
  }

  @Test
  public void testEndsWith() {
    byte[] bytes = getTestBytes(1000, 1234L);
    ByteString string = ByteString.copyFrom(bytes);
    ByteString prefix = ByteString.copyFrom(bytes, 0, 500);
    ByteString suffix = ByteString.copyFrom(bytes, 400, 600);
    assertThat(string.endsWith(ByteString.EMPTY)).isTrue();
    assertThat(string.endsWith(string)).isTrue();
    assertThat(string.endsWith(suffix)).isTrue();
    assertThat(string.endsWith(prefix)).isFalse();
    assertThat(suffix.endsWith(prefix)).isFalse();
    assertThat(prefix.endsWith(suffix)).isFalse();
    assertThat(ByteString.EMPTY.endsWith(suffix)).isFalse();
    assertThat(ByteString.EMPTY.endsWith(ByteString.EMPTY)).isTrue();
  }

  static List<ByteString> makeConcretePieces(byte[] referenceBytes) {
    List<ByteString> pieces = new ArrayList<ByteString>();
    // Starting length should be small enough that we'll do some concatenating by
    // copying if we just concatenate all these pieces together.
    for (int start = 0, length = 16; start < referenceBytes.length; start += length) {
      length = (length << 1) - 1;
      if (start + length > referenceBytes.length) {
        length = referenceBytes.length - start;
      }
      pieces.add(ByteString.copyFrom(referenceBytes, start, length));
    }
    return pieces;
  }

  private byte[] substringUsingWriteTo(ByteString data, int offset, int length) throws IOException {
    ByteArrayOutputStream output = new ByteArrayOutputStream();
    data.writeTo(output, offset, length);
    return output.toByteArray();
  }

  @Test
  public void testWriteToOutputStream() throws Exception {
    // Choose a size large enough so when two ByteStrings are concatenated they
    // won't be merged into one byte array due to some optimizations.
    final int dataSize = ByteString.CONCATENATE_BY_COPY_SIZE + 1;
    byte[] data1 = new byte[dataSize];
    Arrays.fill(data1, (byte) 1);
    data1[1] = (byte) 11;
    // Test LiteralByteString.writeTo(OutputStream,int,int)
    ByteString left = ByteString.wrap(data1);
    byte[] result = substringUsingWriteTo(left, 1, 1);
    assertThat(result).hasLength(1);
    assertThat(result[0]).isEqualTo((byte) 11);

    byte[] data2 = new byte[dataSize];
    Arrays.fill(data2, 0, data1.length, (byte) 2);
    ByteString right = ByteString.wrap(data2);
    // Concatenate two ByteStrings to create a RopeByteString.
    ByteString root = left.concat(right);
    // Make sure we are actually testing a RopeByteString with a simple tree
    // structure.
    assertThat(root.getTreeDepth()).isEqualTo(1);
    // Write parts of the left node.
    result = substringUsingWriteTo(root, 0, dataSize);
    assertThat(result).hasLength(dataSize);
    assertThat(result[0]).isEqualTo((byte) 1);
    assertThat(result[dataSize - 1]).isEqualTo((byte) 1);
    // Write parts of the right node.
    result = substringUsingWriteTo(root, dataSize, dataSize);
    assertThat(result).hasLength(dataSize);
    assertThat(result[0]).isEqualTo((byte) 2);
    assertThat(result[dataSize - 1]).isEqualTo((byte) 2);
    // Write a segment of bytes that runs across both nodes.
    result = substringUsingWriteTo(root, dataSize / 2, dataSize);
    assertThat(result).hasLength(dataSize);
    assertThat(result[0]).isEqualTo((byte) 1);
    assertThat(result[dataSize - dataSize / 2 - 1]).isEqualTo((byte) 1);
    assertThat(result[dataSize - dataSize / 2]).isEqualTo((byte) 2);
    assertThat(result[dataSize - 1]).isEqualTo((byte) 2);
  }

  /** Tests ByteString uses Arrays based byte copier when running under Hotstop VM. */
  @Test
  public void testByteArrayCopier() throws Exception {
    if (Android.isOnAndroidDevice()) {
      return;
    }
    Field field = ByteString.class.getDeclaredField("byteArrayCopier");
    field.setAccessible(true);
    Object byteArrayCopier = field.get(null);
    assertThat(byteArrayCopier).isNotNull();
    assertWithMessage(byteArrayCopier.toString())
        .that(byteArrayCopier.getClass().getSimpleName().endsWith("ArraysByteArrayCopier"))
        .isTrue();
  }
}
