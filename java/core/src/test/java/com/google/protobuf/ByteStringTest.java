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

import com.google.protobuf.ByteString.Output;

import junit.framework.TestCase;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.Random;

/**
 * Test methods with implementations in {@link ByteString}, plus do some top-level "integration"
 * tests.
 *
 * @author carlanton@google.com (Carl Haverl)
 */
public class ByteStringTest extends TestCase {

  private static final Charset UTF_16 = Charset.forName("UTF-16");

  static byte[] getTestBytes(int size, long seed) {
    Random random = new Random(seed);
    byte[] result = new byte[size];
    random.nextBytes(result);
    return result;
  }

  private byte[] getTestBytes(int size) {
    return getTestBytes(size, 445566L);
  }

  private byte[] getTestBytes() {
    return getTestBytes(1000);
  }

  // Compare the entire left array with a subset of the right array.
  private boolean isArrayRange(byte[] left, byte[] right, int rightOffset, int length) {
    boolean stillEqual = (left.length == length);
    for (int i = 0; (stillEqual && i < length); ++i) {
      stillEqual = (left[i] == right[rightOffset + i]);
    }
    return stillEqual;
  }

  // Returns true only if the given two arrays have identical contents.
  private boolean isArray(byte[] left, byte[] right) {
    return left.length == right.length && isArrayRange(left, right, 0, left.length);
  }

  public void testSubstring_BeginIndex() {
    byte[] bytes = getTestBytes();
    ByteString substring = ByteString.copyFrom(bytes).substring(500);
    assertTrue("substring must contain the tail of the string",
        isArrayRange(substring.toByteArray(), bytes, 500, bytes.length - 500));
  }

  public void testCopyFrom_BytesOffsetSize() {
    byte[] bytes = getTestBytes();
    ByteString byteString = ByteString.copyFrom(bytes, 500, 200);
    assertTrue("copyFrom sub-range must contain the expected bytes",
        isArrayRange(byteString.toByteArray(), bytes, 500, 200));
  }

  public void testCopyFrom_Bytes() {
    byte[] bytes = getTestBytes();
    ByteString byteString = ByteString.copyFrom(bytes);
    assertTrue("copyFrom must contain the expected bytes",
        isArray(byteString.toByteArray(), bytes));
  }

  public void testCopyFrom_ByteBufferSize() {
    byte[] bytes = getTestBytes();
    ByteBuffer byteBuffer = ByteBuffer.allocate(bytes.length);
    byteBuffer.put(bytes);
    byteBuffer.position(500);
    ByteString byteString = ByteString.copyFrom(byteBuffer, 200);
    assertTrue("copyFrom byteBuffer sub-range must contain the expected bytes",
        isArrayRange(byteString.toByteArray(), bytes, 500, 200));
  }

  public void testCopyFrom_ByteBuffer() {
    byte[] bytes = getTestBytes();
    ByteBuffer byteBuffer = ByteBuffer.allocate(bytes.length);
    byteBuffer.put(bytes);
    byteBuffer.position(500);
    ByteString byteString = ByteString.copyFrom(byteBuffer);
    assertTrue("copyFrom byteBuffer sub-range must contain the expected bytes",
        isArrayRange(byteString.toByteArray(), bytes, 500, bytes.length - 500));
  }

  public void testCopyFrom_StringEncoding() {
    String testString = "I love unicode \u1234\u5678 characters";
    ByteString byteString = ByteString.copyFrom(testString, UTF_16);
    byte[] testBytes = testString.getBytes(UTF_16);
    assertTrue("copyFrom string must respect the charset",
        isArrayRange(byteString.toByteArray(), testBytes, 0, testBytes.length));
  }

  public void testCopyFrom_Utf8() {
    String testString = "I love unicode \u1234\u5678 characters";
    ByteString byteString = ByteString.copyFromUtf8(testString);
    byte[] testBytes = testString.getBytes(Internal.UTF_8);
    assertTrue("copyFromUtf8 string must respect the charset",
        isArrayRange(byteString.toByteArray(), testBytes, 0, testBytes.length));
  }

  public void testCopyFrom_Iterable() {
    byte[] testBytes = getTestBytes(77777, 113344L);
    final List<ByteString> pieces = makeConcretePieces(testBytes);
    // Call copyFrom() on a Collection
    ByteString byteString = ByteString.copyFrom(pieces);
    assertTrue("copyFrom a List must contain the expected bytes",
        isArrayRange(byteString.toByteArray(), testBytes, 0, testBytes.length));
    // Call copyFrom on an iteration that's not a collection
    ByteString byteStringAlt = ByteString.copyFrom(new Iterable<ByteString>() {
      @Override
      public Iterator<ByteString> iterator() {
        return pieces.iterator();
      }
    });
    assertEquals("copyFrom from an Iteration must contain the expected bytes",
        byteString, byteStringAlt);
  }

  public void testCopyTo_TargetOffset() {
    byte[] bytes = getTestBytes();
    ByteString byteString = ByteString.copyFrom(bytes);
    byte[] target = new byte[bytes.length + 1000];
    byteString.copyTo(target, 400);
    assertTrue("copyFrom byteBuffer sub-range must contain the expected bytes",
        isArrayRange(bytes, target, 400, bytes.length));
  }

  public void testReadFrom_emptyStream() throws IOException {
    ByteString byteString =
        ByteString.readFrom(new ByteArrayInputStream(new byte[0]));
    assertSame("reading an empty stream must result in the EMPTY constant "
        + "byte string", ByteString.EMPTY, byteString);
  }

  public void testReadFrom_smallStream() throws IOException {
    assertReadFrom(getTestBytes(10));
  }

  public void testReadFrom_mutating() throws IOException {
    byte[] capturedArray = null;
    EvilInputStream eis = new EvilInputStream();
    ByteString byteString = ByteString.readFrom(eis);

    capturedArray = eis.capturedArray;
    byte[] originalValue = byteString.toByteArray();
    for (int x = 0; x < capturedArray.length; ++x) {
      capturedArray[x] = (byte) 0;
    }

    byte[] newValue = byteString.toByteArray();
    assertTrue("copyFrom byteBuffer must not grant access to underlying array",
        Arrays.equals(originalValue, newValue));
  }

  // Tests sizes that are near the rope copy-out threshold.
  public void testReadFrom_mediumStream() throws IOException {
    assertReadFrom(getTestBytes(ByteString.CONCATENATE_BY_COPY_SIZE - 1));
    assertReadFrom(getTestBytes(ByteString.CONCATENATE_BY_COPY_SIZE));
    assertReadFrom(getTestBytes(ByteString.CONCATENATE_BY_COPY_SIZE + 1));
    assertReadFrom(getTestBytes(200));
  }

  // Tests sizes that are over multi-segment rope threshold.
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
  public void testReadFrom_IOExceptions() {
    try {
      ByteString.readFrom(new FailStream());
      fail("readFrom must throw the underlying IOException");

    } catch (IOException e) {
      assertEquals("readFrom must throw the expected exception",
                   "synthetic failure", e.getMessage());
    }
  }

  // Tests that ByteString.readFrom works with streams that don't
  // always fill their buffers.
  public void testReadFrom_reluctantStream() throws IOException {
    final byte[] data = getTestBytes(0x1000);

    ByteString byteString = ByteString.readFrom(new ReluctantStream(data));
    assertTrue("readFrom byte stream must contain the expected bytes",
        isArray(byteString.toByteArray(), data));

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
  private void assertReadFromReluctantStream(byte[] bytes, int chunkSize)
      throws IOException {
    ByteString b = ByteString.readFrom(new ReluctantStream(bytes), chunkSize);
    assertTrue("readFrom byte stream must contain the expected bytes",
        isArray(b.toByteArray(), bytes));
  }

  // Tests that ByteString.readFrom works with streams that implement
  // available().
  public void testReadFrom_available() throws IOException {
    final byte[] data = getTestBytes(0x1001);

    ByteString byteString = ByteString.readFrom(new AvailableStream(data));
    assertTrue("readFrom byte stream must contain the expected bytes",
        isArray(byteString.toByteArray(), data));
  }

  // Fails unless ByteString.readFrom reads the bytes correctly.
  private void assertReadFrom(byte[] bytes) throws IOException {
    ByteString byteString =
        ByteString.readFrom(new ByteArrayInputStream(bytes));
    assertTrue("readFrom byte stream must contain the expected bytes",
        isArray(byteString.toByteArray(), bytes));
  }

  // A stream that fails when read.
  private static final class FailStream extends InputStream {
    @Override public int read() throws IOException {
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

    @Override public int read() {
      if (pos == data.length) {
        return -1;
      } else {
        return data[pos++];
      }
    }

    @Override public int read(byte[] buf) {
      return read(buf, 0, buf.length);
    }

    @Override public int read(byte[] buf, int offset, int size) {
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

    @Override public int available() {
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

  public void testToStringUtf8() {
    String testString = "I love unicode \u1234\u5678 characters";
    byte[] testBytes = testString.getBytes(Internal.UTF_8);
    ByteString byteString = ByteString.copyFrom(testBytes);
    assertEquals("copyToStringUtf8 must respect the charset",
        testString, byteString.toStringUtf8());
  }

  public void testNewOutput_InitialCapacity() throws IOException {
    byte[] bytes = getTestBytes();
    ByteString.Output output = ByteString.newOutput(bytes.length + 100);
    output.write(bytes);
    ByteString byteString = output.toByteString();
    assertTrue(
        "String built from newOutput(int) must contain the expected bytes",
        isArrayRange(bytes, byteString.toByteArray(), 0, bytes.length));
  }

  // Test newOutput() using a variety of buffer sizes and a variety of (fixed)
  // write sizes
  public void testNewOutput_ArrayWrite() {
    byte[] bytes = getTestBytes();
    int length = bytes.length;
    int[] bufferSizes = {128, 256, length / 2, length - 1, length, length + 1,
                         2 * length, 3 * length};
    int[] writeSizes = {1, 4, 5, 7, 23, bytes.length};

    for (int bufferSize : bufferSizes) {
      for (int writeSize : writeSizes) {
        // Test writing the entire output writeSize bytes at a time.
        ByteString.Output output = ByteString.newOutput(bufferSize);
        for (int i = 0; i < length; i += writeSize) {
          output.write(bytes, i, Math.min(writeSize, length - i));
        }
        ByteString byteString = output.toByteString();
        assertTrue("String built from newOutput() must contain the expected bytes",
            isArrayRange(bytes, byteString.toByteArray(), 0, bytes.length));
      }
    }
  }

  // Test newOutput() using a variety of buffer sizes, but writing all the
  // characters using write(byte);
  public void testNewOutput_WriteChar() {
    byte[] bytes = getTestBytes();
    int length = bytes.length;
    int[] bufferSizes = {0, 1, 128, 256, length / 2,
                         length - 1, length, length + 1,
                         2 * length, 3 * length};
    for (int bufferSize : bufferSizes) {
      ByteString.Output output = ByteString.newOutput(bufferSize);
      for (byte byteValue : bytes) {
        output.write(byteValue);
      }
      ByteString byteString = output.toByteString();
      assertTrue("String built from newOutput() must contain the expected bytes",
          isArrayRange(bytes, byteString.toByteArray(), 0, bytes.length));
    }
  }

  // Test newOutput() in which we write the bytes using a variety of methods
  // and sizes, and in which we repeatedly call toByteString() in the middle.
  public void testNewOutput_Mixed() {
    Random rng = new Random(1);
    byte[] bytes = getTestBytes();
    int length = bytes.length;
    int[] bufferSizes = {0, 1, 128, 256, length / 2,
                         length - 1, length, length + 1,
                         2 * length, 3 * length};

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
        assertEquals("size() returns the right value", position, output.size());
        assertTrue("newOutput() substring must have correct bytes",
            isArrayRange(output.toByteString().toByteArray(),
                bytes, 0, position));
      }
      ByteString byteString = output.toByteString();
      assertTrue("String built from newOutput() must contain the expected bytes",
          isArrayRange(bytes, byteString.toByteArray(), 0, bytes.length));
    }
  }

  public void testNewOutputEmpty() {
    // Make sure newOutput() correctly builds empty byte strings
    ByteString byteString = ByteString.newOutput().toByteString();
    assertEquals(ByteString.EMPTY, byteString);
  }

  public void testNewOutput_Mutating() throws IOException {
    Output os = ByteString.newOutput(5);
    os.write(new byte[] {1, 2, 3, 4, 5});
    EvilOutputStream eos = new EvilOutputStream();
    os.writeTo(eos);
    byte[] capturedArray = eos.capturedArray;
    ByteString byteString = os.toByteString();
    byte[] oldValue = byteString.toByteArray();
    Arrays.fill(capturedArray, (byte) 0);
    byte[] newValue = byteString.toByteArray();
    assertTrue("Output must not provide access to the underlying byte array",
        Arrays.equals(oldValue, newValue));
  }

  public void testNewCodedBuilder() throws IOException {
    byte[] bytes = getTestBytes();
    ByteString.CodedBuilder builder = ByteString.newCodedBuilder(bytes.length);
    builder.getCodedOutput().writeRawBytes(bytes);
    ByteString byteString = builder.build();
    assertTrue("String built from newCodedBuilder() must contain the expected bytes",
        isArrayRange(bytes, byteString.toByteArray(), 0, bytes.length));
  }

  public void testSubstringParity() {
    byte[] bigBytes = getTestBytes(2048 * 1024, 113344L);
    int start = 512 * 1024 - 3333;
    int end   = 512 * 1024 + 7777;
    ByteString concreteSubstring = ByteString.copyFrom(bigBytes).substring(start, end);
    boolean ok = true;
    for (int i = start; ok && i < end; ++i) {
      ok = (bigBytes[i] == concreteSubstring.byteAt(i - start));
    }
    assertTrue("Concrete substring didn't capture the right bytes", ok);

    ByteString literalString = ByteString.copyFrom(bigBytes, start, end - start);
    assertTrue("Substring must be equal to literal string",
        concreteSubstring.equals(literalString));
    assertEquals("Substring must have same hashcode as literal string",
        literalString.hashCode(), concreteSubstring.hashCode());
  }

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
    assertTrue("Substring must return correct bytes", stillEqual);

    stillEqual = true;
    for (int i = 0; stillEqual && i < to - from; ++i) {
      stillEqual = referenceBytes[from + i] == compositeSubstring.byteAt(i);
    }
    assertTrue("Substring must support byteAt() correctly", stillEqual);

    ByteString literalSubstring = ByteString.copyFrom(referenceBytes, from, to - from);
    assertTrue("Composite substring must equal a literal substring over the same bytes",
        compositeSubstring.equals(literalSubstring));
    assertTrue("Literal substring must equal a composite substring over the same bytes",
        literalSubstring.equals(compositeSubstring));

    assertEquals("We must get the same hashcodes for composite and literal substrings",
        literalSubstring.hashCode(), compositeSubstring.hashCode());

    assertFalse("We can't be equal to a proper substring",
        compositeSubstring.equals(literalSubstring.substring(0, literalSubstring.size() - 1)));
  }

  public void testCopyFromList() {
    byte[] referenceBytes = getTestBytes(77748, 113344L);
    ByteString literalString = ByteString.copyFrom(referenceBytes);

    List<ByteString> pieces = makeConcretePieces(referenceBytes);
    ByteString listString = ByteString.copyFrom(pieces);

    assertTrue("Composite string must be equal to literal string",
        listString.equals(literalString));
    assertEquals("Composite string must have same hashcode as literal string",
        literalString.hashCode(), listString.hashCode());
  }

  public void testConcat() {
    byte[] referenceBytes = getTestBytes(77748, 113344L);
    ByteString literalString = ByteString.copyFrom(referenceBytes);

    List<ByteString> pieces = makeConcretePieces(referenceBytes);

    Iterator<ByteString> iter = pieces.iterator();
    ByteString concatenatedString = iter.next();
    while (iter.hasNext()) {
      concatenatedString = concatenatedString.concat(iter.next());
    }

    assertTrue("Concatenated string must be equal to literal string",
        concatenatedString.equals(literalString));
    assertEquals("Concatenated string must have same hashcode as literal string",
        literalString.hashCode(), concatenatedString.hashCode());
  }

  /**
   * Test the Rope implementation can deal with Empty nodes, even though we
   * guard against them. See also {@link LiteralByteStringTest#testConcat_empty()}.
   */
  public void testConcat_empty() {
    byte[] referenceBytes = getTestBytes(7748, 113344L);
    ByteString literalString = ByteString.copyFrom(referenceBytes);

    ByteString duo = RopeByteString.newInstanceForTest(literalString, literalString);
    ByteString temp = RopeByteString.newInstanceForTest(
        RopeByteString.newInstanceForTest(literalString, ByteString.EMPTY),
        RopeByteString.newInstanceForTest(ByteString.EMPTY, literalString));
    ByteString quintet = RopeByteString.newInstanceForTest(temp, ByteString.EMPTY);

    assertTrue("String with concatenated nulls must equal simple concatenate",
        duo.equals(quintet));
    assertEquals("String with concatenated nulls have same hashcode as simple concatenate",
        duo.hashCode(), quintet.hashCode());

    ByteString.ByteIterator duoIter = duo.iterator();
    ByteString.ByteIterator quintetIter = quintet.iterator();
    boolean stillEqual = true;
    while (stillEqual && quintetIter.hasNext()) {
      stillEqual = (duoIter.nextByte() == quintetIter.nextByte());
    }
    assertTrue("We must get the same characters by iterating", stillEqual);
    assertFalse("Iterator must be exhausted", duoIter.hasNext());
    try {
      duoIter.nextByte();
      fail("Should have thrown an exception.");
    } catch (NoSuchElementException e) {
      // This is success
    }
    try {
      quintetIter.nextByte();
      fail("Should have thrown an exception.");
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
      assertTrue("Substrings of size() < 2 must not be RopeByteStrings",
          duo.substring(i - 1, i) instanceof ByteString.LeafByteString);
    }
    for (int i = 1; i < quintet.size(); ++i) {
      assertTrue("Substrings of size() < 2 must not be RopeByteStrings",
          quintet.substring(i - 1, i) instanceof ByteString.LeafByteString);
    }
  }

  public void testStartsWith() {
    byte[] bytes = getTestBytes(1000, 1234L);
    ByteString string = ByteString.copyFrom(bytes);
    ByteString prefix = ByteString.copyFrom(bytes, 0, 500);
    ByteString suffix = ByteString.copyFrom(bytes, 400, 600);
    assertTrue(string.startsWith(ByteString.EMPTY));
    assertTrue(string.startsWith(string));
    assertTrue(string.startsWith(prefix));
    assertFalse(string.startsWith(suffix));
    assertFalse(prefix.startsWith(suffix));
    assertFalse(suffix.startsWith(prefix));
    assertFalse(ByteString.EMPTY.startsWith(prefix));
    assertTrue(ByteString.EMPTY.startsWith(ByteString.EMPTY));
  }

  public void testEndsWith() {
    byte[] bytes = getTestBytes(1000, 1234L);
    ByteString string = ByteString.copyFrom(bytes);
    ByteString prefix = ByteString.copyFrom(bytes, 0, 500);
    ByteString suffix = ByteString.copyFrom(bytes, 400, 600);
    assertTrue(string.endsWith(ByteString.EMPTY));
    assertTrue(string.endsWith(string));
    assertTrue(string.endsWith(suffix));
    assertFalse(string.endsWith(prefix));
    assertFalse(suffix.endsWith(prefix));
    assertFalse(prefix.endsWith(suffix));
    assertFalse(ByteString.EMPTY.endsWith(suffix));
    assertTrue(ByteString.EMPTY.endsWith(ByteString.EMPTY));
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

  private byte[] substringUsingWriteTo(
      ByteString data, int offset, int length) throws IOException {
    ByteArrayOutputStream output = new ByteArrayOutputStream();
    data.writeTo(output, offset, length);
    return output.toByteArray();
  }

  public void testWriteToOutputStream() throws Exception {
    // Choose a size large enough so when two ByteStrings are concatenated they
    // won't be merged into one byte array due to some optimizations.
    final int dataSize = ByteString.CONCATENATE_BY_COPY_SIZE + 1;
    byte[] data1 = new byte[dataSize];
    for (int i = 0; i < data1.length; i++) {
      data1[i] = (byte) 1;
    }
    data1[1] = (byte) 11;
    // Test LiteralByteString.writeTo(OutputStream,int,int)
    ByteString left = ByteString.wrap(data1);
    byte[] result = substringUsingWriteTo(left, 1, 1);
    assertEquals(1, result.length);
    assertEquals((byte) 11, result[0]);

    byte[] data2 = new byte[dataSize];
    for (int i = 0; i < data1.length; i++) {
      data2[i] = (byte) 2;
    }
    ByteString right = ByteString.wrap(data2);
    // Concatenate two ByteStrings to create a RopeByteString.
    ByteString root = left.concat(right);
    // Make sure we are actually testing a RopeByteString with a simple tree
    // structure.
    assertEquals(1, root.getTreeDepth());
    // Write parts of the left node.
    result = substringUsingWriteTo(root, 0, dataSize);
    assertEquals(dataSize, result.length);
    assertEquals((byte) 1, result[0]);
    assertEquals((byte) 1, result[dataSize - 1]);
    // Write parts of the right node.
    result = substringUsingWriteTo(root, dataSize, dataSize);
    assertEquals(dataSize, result.length);
    assertEquals((byte) 2, result[0]);
    assertEquals((byte) 2, result[dataSize - 1]);
    // Write a segment of bytes that runs across both nodes.
    result = substringUsingWriteTo(root, dataSize / 2, dataSize);
    assertEquals(dataSize, result.length);
    assertEquals((byte) 1, result[0]);
    assertEquals((byte) 1, result[dataSize - dataSize / 2 - 1]);
    assertEquals((byte) 2, result[dataSize - dataSize / 2]);
    assertEquals((byte) 2, result[dataSize - 1]);
  }
}
