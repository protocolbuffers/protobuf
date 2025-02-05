// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;
import static com.google.common.truth.TruthJUnit.assume;
import static org.junit.Assert.assertThrows;

import com.google.protobuf.CodedOutputStream.OutOfSpaceException;
import proto2_unittest.UnittestProto.SparseEnumMessage;
import proto2_unittest.UnittestProto.TestAllTypes;
import proto2_unittest.UnittestProto.TestSparseEnum;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

/** Unit test for {@link CodedOutputStream}. */
@RunWith(Parameterized.class)
public class CodedOutputStreamTest {
  @Parameters(name = "OutputType={0}")
  public static List<OutputType> data() {
    return Arrays.asList(OutputType.values());
  }

  private final OutputType outputType;

  public CodedOutputStreamTest(OutputType outputType) {
    this.outputType = outputType;
  }

  private interface Coder {
    CodedOutputStream stream();

    byte[] toByteArray();
  }

  // Like ByteArrayOutputStream, but doesn't dynamically grow the backing byte[]. Instead, it
  // throws OutOfSpaceException if we overflow the backing byte[].
  private static final class FixedSizeByteArrayOutputStream extends OutputStream {
    private final byte[] buf;
    private int size = 0;

    FixedSizeByteArrayOutputStream(int size) {
      this.buf = new byte[size];
    }

    @Override
    public void write(int b) throws IOException {
      try {
        buf[size] = (byte) b;
      } catch (IndexOutOfBoundsException e) {
        // Real OutputStreams probably won't be so kind as to throw the exact OutOfSpaceException
        // that we want in our tests. Throwing this makes our tests simpler, and OutputStream
        // doesn't really have a good protocol for signalling running out of buffer space.
        throw new OutOfSpaceException(size, buf.length, 1, e);
      }
      size++;
    }

    public byte[] toByteArray() {
      return Arrays.copyOf(buf, size);
    }
  }

  private static final class OutputStreamCoder implements Coder {
    private final CodedOutputStream stream;
    private final FixedSizeByteArrayOutputStream output;

    OutputStreamCoder(int size, int blockSize) {
      output = new FixedSizeByteArrayOutputStream(size);
      stream = CodedOutputStream.newInstance(output, blockSize);
    }

    @Override
    public CodedOutputStream stream() {
      return stream;
    }

    @Override
    public byte[] toByteArray() {
      return output.toByteArray();
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
  }

  private static final class NioDirectCoder implements Coder {
    private final int initialPosition;
    private final CodedOutputStream stream;
    private final ByteBuffer buffer;

    NioDirectCoder(int size, boolean unsafe) {
      this(size, 0, unsafe);
    }

    NioDirectCoder(int size, int initialPosition, boolean unsafe) {
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
  }

  private static final class ByteOutputWrappingArrayCoder implements Coder {
    private final CodedOutputStream stream;
    private final byte[] bytes;

    ByteOutputWrappingArrayCoder(int size) {
      bytes = new byte[size];
      // Any ByteOutput subclass would do. All CodedInputStreams implement ByteOutput, so it
      // seemed most convenient to this this with a CodedInputStream.newInstance(byte[]).
      ByteOutput byteOutput = CodedOutputStream.newInstance(bytes);
      stream = CodedOutputStream.newInstance(byteOutput, size);
    }

    @Override
    public CodedOutputStream stream() {
      return stream;
    }

    @Override
    public byte[] toByteArray() {
      return Arrays.copyOf(bytes, stream.getTotalBytesWritten());
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
    NIO_HEAP_WITH_INITIAL_OFFSET() {
      @Override
      Coder newCoder(int size) {
        int offset = 2;
        return new NioHeapCoder(size + offset, /* initialPosition= */ offset);
      }
    },
    NIO_DIRECT_SAFE() {
      @Override
      Coder newCoder(int size) {
        return new NioDirectCoder(size, /* unsafe= */ false);
      }
    },
    NIO_DIRECT_SAFE_WITH_INITIAL_OFFSET() {
      @Override
      Coder newCoder(int size) {
        int offset = 2;
        return new NioDirectCoder(size + offset, offset, /* unsafe= */ false);
      }
    },
    NIO_DIRECT_UNSAFE() {
      @Override
      Coder newCoder(int size) {
        return new NioDirectCoder(size, /* unsafe= */ true);
      }
    },
    NIO_DIRECT_UNSAFE_WITH_INITIAL_OFFSET() {
      @Override
      Coder newCoder(int size) {
        int offset = 2;
        return new NioDirectCoder(size + offset, offset, /* unsafe= */ true);
      }
    },
    STREAM() {
      @Override
      Coder newCoder(int size) {
        return new OutputStreamCoder(size, /* blockSize= */ size);
      }
    },
    STREAM_MINIMUM_BUFFER_SIZE() {
      @Override
      Coder newCoder(int size) {
        // Block Size 0 gets rounded up to minimum block size, see AbstractBufferedEncoder.
        return new OutputStreamCoder(size, /* blockSize= */ 0);
      }
    },
    BYTE_OUTPUT_WRAPPING_ARRAY() {
      @Override
      Coder newCoder(int size) {
        return new ByteOutputWrappingArrayCoder(size);
      }
    };

    abstract Coder newCoder(int size);

    /** Whether we can call CodedOutputStream.spaceLeft(). */
    boolean supportsSpaceLeft() {
      // Buffered encoders don't know how much space is left.
      switch (this) {
        case STREAM:
        case STREAM_MINIMUM_BUFFER_SIZE:
        case BYTE_OUTPUT_WRAPPING_ARRAY:
          return false;
        default:
          return true;
      }
    }
  }

  /** Checks that invariants are maintained for varint round trip input and output. */
  @Test
  public void testVarintRoundTrips() throws Exception {
    assertVarintRoundTrip(0L);
    for (int bits = 0; bits < 64; bits++) {
      long value = 1L << bits;
      assertVarintRoundTrip(value);
      assertVarintRoundTrip(value + 1);
      assertVarintRoundTrip(value - 1);
      assertVarintRoundTrip(-value);
    }
  }

  /** Tests writeRawVarint32() and writeRawVarint64(). */
  @Test
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
        (0x00 << 0)
            | (0x66 << 7)
            | (0x6b << 14)
            | (0x1c << 21)
            | (0x43L << 28)
            | (0x49L << 35)
            | (0x24L << 42)
            | (0x49L << 49));
    // 11964378330978735131
    assertWriteVarint(
        bytes(0x9b, 0xa8, 0xf9, 0xc2, 0xbb, 0xd6, 0x80, 0x85, 0xa6, 0x01),
        (0x1b << 0)
            | (0x28 << 7)
            | (0x79 << 14)
            | (0x42 << 21)
            | (0x3bL << 28)
            | (0x56L << 35)
            | (0x00L << 42)
            | (0x05L << 49)
            | (0x26L << 56)
            | (0x01L << 63));
  }

  @Test
  public void testWriteFixed32NoTag() throws Exception {
    assertWriteFixed32(bytes(0x78, 0x56, 0x34, 0x12), 0x12345678);
    assertWriteFixed32(bytes(0xf0, 0xde, 0xbc, 0x9a), 0x9abcdef0);
  }

  @Test
  public void testWriteFixed64NoTag() throws Exception {
    assertWriteFixed64(bytes(0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12), 0x123456789abcdef0L);
    assertWriteFixed64(bytes(0x78, 0x56, 0x34, 0x12, 0xf0, 0xde, 0xbc, 0x9a), 0x9abcdef012345678L);
  }

  @Test
  public void testWriteFixed32NoTag_outOfBounds_throws() throws Exception {
    for (int i = 0; i < 4; i++) {
      Coder coder = outputType.newCoder(i);
      // Some coders throw immediately on write, some throw on flush.
      @SuppressWarnings("AssertThrowsMultipleStatements")
      OutOfSpaceException e =
          assertThrows(
              OutOfSpaceException.class,
              () -> {
                coder.stream().writeFixed32NoTag(1);
                coder.stream().flush();
              });
      // STREAM writes one byte at a time.
      if (outputType != OutputType.STREAM && outputType != OutputType.STREAM_MINIMUM_BUFFER_SIZE) {
        assertThat(e).hasMessageThat().contains("len: 4");
      }
      if (outputType.supportsSpaceLeft()) {
        assertThat(coder.stream().spaceLeft()).isEqualTo(i);
      }
    }
  }

  @Test
  public void testWriteFixed64NoTag_outOfBounds_throws() throws Exception {
    for (int i = 0; i < 8; i++) {
      Coder coder = outputType.newCoder(i);
      // Some coders throw immediately on write, some throw on flush.
      @SuppressWarnings("AssertThrowsMultipleStatements")
      OutOfSpaceException e =
          assertThrows(
              OutOfSpaceException.class,
              () -> {
                coder.stream().writeFixed64NoTag(1);
                coder.stream().flush();
              });
      if (outputType != OutputType.STREAM && outputType != OutputType.STREAM_MINIMUM_BUFFER_SIZE) {
        assertThat(e).hasMessageThat().contains("len: 8");
      }
      if (outputType.supportsSpaceLeft()) {
        assertThat(coder.stream().spaceLeft()).isEqualTo(i);
      }
    }
  }

  @Test
  // Some coders throw immediately on write, some throw on flush.
  @SuppressWarnings("AssertThrowsMultipleStatements")
  public void testWriteUInt32NoTag_outOfBounds_throws() throws Exception {
    for (int i = 0; i < 5; i++) {
      Coder coder = outputType.newCoder(i);
      assertThrows(
          OutOfSpaceException.class,
          () -> {
            coder.stream().writeUInt32NoTag(Integer.MAX_VALUE);
            coder.stream().flush();
          });

      // Space left should not go negative.
      if (outputType.supportsSpaceLeft()) {
        assertWithMessage("i=%s", i).that(coder.stream().spaceLeft()).isAtLeast(0);
      }
    }
  }

  @Test
  // Some coders throw immediately on write, some throw on flush.
  @SuppressWarnings("AssertThrowsMultipleStatements")
  public void testWriteUInt64NoTag_outOfBounds_throws() throws Exception {
    for (int i = 0; i < 9; i++) {
      Coder coder = outputType.newCoder(i);
      assertThrows(
          OutOfSpaceException.class,
          () -> {
            coder.stream().writeUInt64NoTag(Long.MAX_VALUE);
            coder.stream().flush();
          });

      // Space left should not go negative.
      if (outputType.supportsSpaceLeft()) {
        assertWithMessage("i=%s", i).that(coder.stream().spaceLeft()).isAtLeast(0);
      }
    }
  }

  /** Test encodeZigZag32() and encodeZigZag64(). */
  @Test
  public void testEncodeZigZag() throws Exception {
    // We only need to run this test once, they don't depend on outputType.
    // Arbitrarily run them just for ARRAY.
    assume().that(outputType).isEqualTo(OutputType.ARRAY);

    assertThat(CodedOutputStream.encodeZigZag32(0)).isEqualTo(0);
    assertThat(CodedOutputStream.encodeZigZag32(-1)).isEqualTo(1);
    assertThat(CodedOutputStream.encodeZigZag32(1)).isEqualTo(2);
    assertThat(CodedOutputStream.encodeZigZag32(-2)).isEqualTo(3);
    assertThat(CodedOutputStream.encodeZigZag32(0x3FFFFFFF)).isEqualTo(0x7FFFFFFE);
    assertThat(CodedOutputStream.encodeZigZag32(0xC0000000)).isEqualTo(0x7FFFFFFF);
    assertThat(CodedOutputStream.encodeZigZag32(0x7FFFFFFF)).isEqualTo(0xFFFFFFFE);
    assertThat(CodedOutputStream.encodeZigZag32(0x80000000)).isEqualTo(0xFFFFFFFF);

    assertThat(CodedOutputStream.encodeZigZag64(0)).isEqualTo(0);
    assertThat(CodedOutputStream.encodeZigZag64(-1)).isEqualTo(1);
    assertThat(CodedOutputStream.encodeZigZag64(1)).isEqualTo(2);
    assertThat(CodedOutputStream.encodeZigZag64(-2)).isEqualTo(3);
    assertThat(CodedOutputStream.encodeZigZag64(0x000000003FFFFFFFL))
        .isEqualTo(0x000000007FFFFFFEL);
    assertThat(CodedOutputStream.encodeZigZag64(0xFFFFFFFFC0000000L))
        .isEqualTo(0x000000007FFFFFFFL);
    assertThat(CodedOutputStream.encodeZigZag64(0x000000007FFFFFFFL))
        .isEqualTo(0x00000000FFFFFFFEL);
    assertThat(CodedOutputStream.encodeZigZag64(0xFFFFFFFF80000000L))
        .isEqualTo(0x00000000FFFFFFFFL);
    assertThat(CodedOutputStream.encodeZigZag64(0x7FFFFFFFFFFFFFFFL))
        .isEqualTo(0xFFFFFFFFFFFFFFFEL);
    assertThat(CodedOutputStream.encodeZigZag64(0x8000000000000000L))
        .isEqualTo(0xFFFFFFFFFFFFFFFFL);

    // Some easier-to-verify round-trip tests.  The inputs (other than 0, 1, -1)
    // were chosen semi-randomly via keyboard bashing.
    assertThat(CodedOutputStream.encodeZigZag32(CodedInputStream.decodeZigZag32(0))).isEqualTo(0);
    assertThat(CodedOutputStream.encodeZigZag32(CodedInputStream.decodeZigZag32(1))).isEqualTo(1);
    assertThat(CodedOutputStream.encodeZigZag32(CodedInputStream.decodeZigZag32(-1))).isEqualTo(-1);
    assertThat(CodedOutputStream.encodeZigZag32(CodedInputStream.decodeZigZag32(14927)))
        .isEqualTo(14927);
    assertThat(CodedOutputStream.encodeZigZag32(CodedInputStream.decodeZigZag32(-3612)))
        .isEqualTo(-3612);

    assertThat(CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(0))).isEqualTo(0);
    assertThat(CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(1))).isEqualTo(1);
    assertThat(CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(-1))).isEqualTo(-1);
    assertThat(CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(14927)))
        .isEqualTo(14927);
    assertThat(CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(-3612)))
        .isEqualTo(-3612);

    assertThat(CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(856912304801416L)))
        .isEqualTo(856912304801416L);
    assertThat(
            CodedOutputStream.encodeZigZag64(CodedInputStream.decodeZigZag64(-75123905439571256L)))
        .isEqualTo(-75123905439571256L);
  }

  @Test
  public void computeIntSize() {
    // We only need to run this test once, they don't depend on outputType.
    // Arbitrarily run them just for ARRAY.
    assume().that(outputType).isEqualTo(OutputType.ARRAY);

    assertThat(CodedOutputStream.computeUInt32SizeNoTag(0)).isEqualTo(1);
    assertThat(CodedOutputStream.computeUInt64SizeNoTag(0)).isEqualTo(1);
    int i;
    for (i = 0; i < 7; i++) {
      assertThat(CodedOutputStream.computeInt32SizeNoTag(1 << i)).isEqualTo(1);
      assertThat(CodedOutputStream.computeUInt32SizeNoTag(1 << i)).isEqualTo(1);
      assertThat(CodedOutputStream.computeUInt64SizeNoTag(1L << i)).isEqualTo(1);
    }
    for (; i < 14; i++) {
      assertThat(CodedOutputStream.computeInt32SizeNoTag(1 << i)).isEqualTo(2);
      assertThat(CodedOutputStream.computeUInt32SizeNoTag(1 << i)).isEqualTo(2);
      assertThat(CodedOutputStream.computeUInt64SizeNoTag(1L << i)).isEqualTo(2);
    }
    for (; i < 21; i++) {
      assertThat(CodedOutputStream.computeInt32SizeNoTag(1 << i)).isEqualTo(3);
      assertThat(CodedOutputStream.computeUInt32SizeNoTag(1 << i)).isEqualTo(3);
      assertThat(CodedOutputStream.computeUInt64SizeNoTag(1L << i)).isEqualTo(3);
    }
    for (; i < 28; i++) {
      assertThat(CodedOutputStream.computeInt32SizeNoTag(1 << i)).isEqualTo(4);
      assertThat(CodedOutputStream.computeUInt32SizeNoTag(1 << i)).isEqualTo(4);
      assertThat(CodedOutputStream.computeUInt64SizeNoTag(1L << i)).isEqualTo(4);
    }
    for (; i < 31; i++) {
      assertThat(CodedOutputStream.computeInt32SizeNoTag(1 << i)).isEqualTo(5);
      assertThat(CodedOutputStream.computeUInt32SizeNoTag(1 << i)).isEqualTo(5);
      assertThat(CodedOutputStream.computeUInt64SizeNoTag(1L << i)).isEqualTo(5);
    }
    for (; i < 32; i++) {
      assertThat(CodedOutputStream.computeInt32SizeNoTag(1 << i)).isEqualTo(10);
      assertThat(CodedOutputStream.computeUInt32SizeNoTag(1 << i)).isEqualTo(5);
      assertThat(CodedOutputStream.computeUInt64SizeNoTag(1L << i)).isEqualTo(5);
    }
    for (; i < 35; i++) {
      assertThat(CodedOutputStream.computeUInt64SizeNoTag(1L << i)).isEqualTo(5);
    }
    for (; i < 42; i++) {
      assertThat(CodedOutputStream.computeUInt64SizeNoTag(1L << i)).isEqualTo(6);
    }
    for (; i < 49; i++) {
      assertThat(CodedOutputStream.computeUInt64SizeNoTag(1L << i)).isEqualTo(7);
    }
    for (; i < 56; i++) {
      assertThat(CodedOutputStream.computeUInt64SizeNoTag(1L << i)).isEqualTo(8);
    }
    for (; i < 63; i++) {
      assertThat(CodedOutputStream.computeUInt64SizeNoTag(1L << i)).isEqualTo(9);
    }
  }

  @Test
  public void computeTagSize() {
    // We only need to run this test once, they don't depend on outputType.
    // Arbitrarily run them just for ARRAY.
    assume().that(outputType).isEqualTo(OutputType.ARRAY);

    assertThat(CodedOutputStream.computeTagSize(0)).isEqualTo(1);
    int i;
    for (i = 0; i < 4; i++) {
      assertThat(CodedOutputStream.computeTagSize(1 << i)).isEqualTo(1);
    }
    for (; i < 11; i++) {
      assertThat(CodedOutputStream.computeTagSize(1 << i)).isEqualTo(2);
    }
    for (; i < 18; i++) {
      assertThat(CodedOutputStream.computeTagSize(1 << i)).isEqualTo(3);
    }
    for (; i < 25; i++) {
      assertThat(CodedOutputStream.computeTagSize(1 << i)).isEqualTo(4);
    }
    for (; i < 29; i++) {
      assertThat(CodedOutputStream.computeTagSize(1 << i)).isEqualTo(5);
    }
    // Invalid tags
    assertThat(CodedOutputStream.computeTagSize((1 << 30) + 1)).isEqualTo(1);
  }

  /**
   * Test writing a message containing a negative enum value. This used to fail because the size was
   * not properly computed as a sign-extended varint.
   */
  @Test
  public void testWriteMessageWithNegativeEnumValue() throws Exception {
    SparseEnumMessage message =
        SparseEnumMessage.newBuilder().setSparseEnum(TestSparseEnum.SPARSE_E).build();
    assertThat(message.getSparseEnum().getNumber()).isLessThan(0);
    Coder coder = outputType.newCoder(message.getSerializedSize());
    message.writeTo(coder.stream());
    coder.stream().flush();
    byte[] rawBytes = coder.toByteArray();
    SparseEnumMessage message2 = SparseEnumMessage.parseFrom(rawBytes);
    assertThat(message2.getSparseEnum()).isEqualTo(TestSparseEnum.SPARSE_E);
  }

  /** Test getTotalBytesWritten() */
  @Test
  public void testGetTotalBytesWritten() throws Exception {
    assume().that(outputType).isEqualTo(OutputType.STREAM);

    Coder coder = outputType.newCoder(/* size= */ 16 * 1024);

    // Write some some bytes (more than the buffer can hold) and verify that totalWritten
    // is correct.
    byte[] value = "abcde".getBytes(Internal.UTF_8);
    for (int i = 0; i < 1024; ++i) {
      coder.stream().writeRawBytes(value, 0, value.length);
    }
    assertThat(coder.stream().getTotalBytesWritten()).isEqualTo(value.length * 1024);

    // Now write an encoded string.
    String string =
        "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";
    // Ensure we take the slower fast path.
    assertThat(CodedOutputStream.computeUInt32SizeNoTag(string.length()))
        .isNotEqualTo(
            CodedOutputStream.computeUInt32SizeNoTag(string.length() * Utf8.MAX_BYTES_PER_CHAR));

    coder.stream().writeStringNoTag(string);
    coder.stream().flush();
    int stringSize = CodedOutputStream.computeStringSizeNoTag(string);

    // Verify that the total bytes written is correct
    assertThat(coder.stream().getTotalBytesWritten()).isEqualTo((value.length * 1024) + stringSize);
  }

  // TODO: Write a comprehensive test suite for CodedOutputStream that covers more than just
  //    this case.
  @Test
  public void testWriteStringNoTag_fastpath() throws Exception {
    int bufferSize = 153;
    String threeBytesPer = "\u0981";
    String string = threeBytesPer;
    for (int i = 0; i < 50; i++) {
      string += threeBytesPer;
    }
    // These checks ensure we will tickle the slower fast path.
    assertThat(CodedOutputStream.computeUInt32SizeNoTag(string.length())).isEqualTo(1);
    assertThat(CodedOutputStream.computeUInt32SizeNoTag(string.length() * Utf8.MAX_BYTES_PER_CHAR))
        .isEqualTo(2);
    assertThat(bufferSize).isEqualTo(string.length() * Utf8.MAX_BYTES_PER_CHAR);

    Coder coder = outputType.newCoder(bufferSize + 2);
    coder.stream().writeStringNoTag(string);
    coder.stream().flush();
  }

  @Test
  public void testWriteToByteBuffer() throws Exception {
    assume().that(outputType).isEqualTo(OutputType.NIO_HEAP);

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
    Arrays.fill(data, 0, length2, (byte) 2);
    codedStream.writeRawBytes(data);
    final int length3 = bufferSize - length1 - length2;
    for (int i = 0; i < length3; i++) {
      codedStream.writeRawByte((byte) 3);
    }
    codedStream.flush();

    // Check that data is correctly written to the ByteBuffer.
    assertThat(buffer.remaining()).isEqualTo(0);
    buffer.flip();
    for (int i = 0; i < length1; i++) {
      assertThat(buffer.get()).isEqualTo((byte) 1);
    }
    for (int i = 0; i < length2; i++) {
      assertThat(buffer.get()).isEqualTo((byte) 2);
    }
    for (int i = 0; i < length3; i++) {
      assertThat(buffer.get()).isEqualTo((byte) 3);
    }
  }

  @Test
  public void testWriteByte() throws Exception {
    Coder coder = outputType.newCoder(5);
    // Write 5 bytes
    coder.stream().write((byte) 1);
    coder.stream().write((byte) 1);
    coder.stream().write((byte) 1);
    coder.stream().write((byte) 1);
    coder.stream().write((byte) 1);
    coder.stream().flush();
    byte[] rawBytes = coder.toByteArray();
    assertThat(rawBytes).isEqualTo(new byte[] {1, 1, 1, 1, 1});
    if (outputType.supportsSpaceLeft()) {
      assertThat(coder.stream().spaceLeft()).isEqualTo(0);
    }

    // Some coders throw immediately on write, some throw on flush.
    @SuppressWarnings("AssertThrowsMultipleStatements")
    OutOfSpaceException e =
        assertThrows(
            OutOfSpaceException.class,
            () -> {
              coder.stream().write((byte) 1);
              coder.stream().flush();
            });
    assertThat(e).hasMessageThat().contains("len: 1");
    if (outputType.supportsSpaceLeft()) {
      assertThat(coder.stream().spaceLeft()).isEqualTo(0);
    }
  }

  @Test
  public void testWriteRawBytes_byteBuffer() throws Exception {
    byte[] value = "abcde".getBytes(Internal.UTF_8);
    Coder coder = outputType.newCoder(100);
    CodedOutputStream codedStream = coder.stream();
    ByteBuffer byteBuffer = ByteBuffer.wrap(value, /* offset= */ 0, /* length= */ 1);
    assertThat(byteBuffer.capacity()).isEqualTo(5);
    // This will actually write 5 bytes into the CodedOutputStream as the
    // ByteBuffer's capacity() is 5.
    codedStream.writeRawBytes(byteBuffer);
    assertThat(codedStream.getTotalBytesWritten()).isEqualTo(5);

    // writeRawBytes shouldn't affect the ByteBuffer's state.
    assertThat(byteBuffer.position()).isEqualTo(0);
    assertThat(byteBuffer.limit()).isEqualTo(1);

    // The correct way to write part of an array using ByteBuffer.
    codedStream.writeRawBytes(ByteBuffer.wrap(value, /* offset= */ 2, /* length= */ 1).slice());

    codedStream.flush();
    byte[] result = coder.toByteArray();
    assertThat(result).hasLength(6);
    for (int i = 0; i < 5; i++) {
      assertThat(value[i]).isEqualTo(result[i]);
    }
    assertThat(value[2]).isEqualTo(result[5]);
  }

  @Test
  public void testWrite_byteBuffer() throws Exception {
    byte[] bytes = new byte[] {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    Coder coder = outputType.newCoder(100);
    CodedOutputStream codedStream = coder.stream();
    ByteBuffer byteBuffer = ByteBuffer.wrap(bytes);
    assertThat(byteBuffer.capacity()).isEqualTo(10);
    assertThat(byteBuffer.position()).isEqualTo(0);
    assertThat(byteBuffer.limit()).isEqualTo(10);

    codedStream.write(byteBuffer);
    codedStream.flush();
    assertThat(codedStream.getTotalBytesWritten()).isEqualTo(10);

    // write should update the ByteBuffer's state.
    assertThat(byteBuffer.position()).isEqualTo(10);
    assertThat(byteBuffer.limit()).isEqualTo(10);

    assertThat(coder.toByteArray()).isEqualTo(bytes);
  }

  @Test
  // Some coders throw immediately on write, some throw on flush.
  @SuppressWarnings("AssertThrowsMultipleStatements")
  public void testWrite_byteBuffer_outOfSpace() throws Exception {
    byte[] bytes = new byte[10];

    for (int i = 0; i < 10; i++) {
      ByteBuffer byteBuffer = ByteBuffer.wrap(bytes);
      Coder coder = outputType.newCoder(i);
      CodedOutputStream codedStream = coder.stream();
      assertThrows("i=" + i, OutOfSpaceException.class, () -> {
        codedStream.write(byteBuffer);
        codedStream.flush();
      });
    }
  }

  @Test
  public void testWrite_byteArray() throws Exception {
    byte[] bytes = new byte[] {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    Coder coder = outputType.newCoder(100);
    CodedOutputStream codedStream = coder.stream();

    codedStream.write(bytes, 0, bytes.length);
    codedStream.flush();
    assertThat(codedStream.getTotalBytesWritten()).isEqualTo(10);

    assertThat(coder.toByteArray()).isEqualTo(bytes);
  }

  @Test
  // Some coders throw immediately on write, some throw on flush.
  @SuppressWarnings("AssertThrowsMultipleStatements")
  public void testWrite_byteArray_outOfSpace() throws Exception {
    byte[] bytes = new byte[10];

    for (int i = 0; i < 10; i++) {
      Coder coder = outputType.newCoder(i);
      CodedOutputStream codedStream = coder.stream();
      assertThrows("i=" + i, OutOfSpaceException.class, () -> {
        codedStream.write(bytes, 0, bytes.length);
        codedStream.flush();
      });
    }
  }

  @Test
  public void testWriteByteArrayNoTag_withOffsets() throws Exception {
    assume().that(outputType).isEqualTo(OutputType.ARRAY);

    byte[] fullArray = bytes(0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88);
    Coder coder = outputType.newCoder(4);
    coder.stream().writeByteArrayNoTag(fullArray, 2, 2);
    assertThat(coder.toByteArray()).isEqualTo(bytes(0x02, 0x33, 0x44));
    assertThat(coder.stream().getTotalBytesWritten()).isEqualTo(3);
  }

  @Test
  public void testSerializeUtf8_multipleSmallWrites() throws Exception {
    final String source = "abcdefghijklmnopqrstuvwxyz";

    // Generate the expected output if the source string is written 2 bytes at a time.
    ByteArrayOutputStream expectedBytesStream = new ByteArrayOutputStream();
    for (int pos = 0; pos < source.length(); pos += 2) {
      String substr = source.substring(pos, pos + 2);
      expectedBytesStream.write(2);
      expectedBytesStream.write(substr.getBytes(Internal.UTF_8));
    }
    final byte[] expectedBytes = expectedBytesStream.toByteArray();

    // Write the source string 2 bytes at a time and verify the output.
    Coder coder = outputType.newCoder(expectedBytes.length);
    for (int pos = 0; pos < source.length(); pos += 2) {
      String substr = source.substring(pos, pos + 2);
      coder.stream().writeStringNoTag(substr);
    }
    coder.stream().flush();
    assertThat(coder.toByteArray()).isEqualTo(expectedBytes);
  }

  @Test
  public void testSerializeInvalidUtf8() throws Exception {
    String[] invalidStrings =
        new String[] {
          newString(Character.MIN_HIGH_SURROGATE),
          "foobar" + newString(Character.MIN_HIGH_SURROGATE),
          newString(Character.MIN_LOW_SURROGATE),
          "foobar" + newString(Character.MIN_LOW_SURROGATE),
          newString(Character.MIN_HIGH_SURROGATE, Character.MIN_HIGH_SURROGATE)
        };

    Coder coder = outputType.newCoder(10000);
    for (String s : invalidStrings) {
      // TODO: These should all fail; instead they are corrupting data.
      CodedOutputStream.computeStringSizeNoTag(s);
      coder.stream().writeStringNoTag(s);
    }
  }

  // TODO: This test can be deleted once we properly throw IOException while
  // encoding invalid UTF-8 strings.
  @Test
  public void testSerializeInvalidUtf8FollowedByOutOfSpace() throws Exception {
    final int notEnoughBytes = 4;
    // This test fails for BYTE_OUTPUT_WRAPPING_ARRAY
    assume().that(outputType).isNotEqualTo(OutputType.BYTE_OUTPUT_WRAPPING_ARRAY);

    Coder coder = outputType.newCoder(notEnoughBytes);

    String invalidString = newString(Character.MIN_HIGH_SURROGATE, 'f', 'o', 'o', 'b', 'a', 'r');
    // Some coders throw immediately on write, some throw on flush.
    @SuppressWarnings("AssertThrowsMultipleStatements")
    OutOfSpaceException e =
        assertThrows(
            OutOfSpaceException.class,
            () -> {
              coder.stream().writeStringNoTag(invalidString);
              coder.stream().flush();
            });
    assertThat(e).hasCauseThat().isInstanceOf(IndexOutOfBoundsException.class);
  }

  /** Regression test for https://github.com/protocolbuffers/protobuf/issues/292 */
  @Test
  // Some coders throw immediately on write, some throw on flush.
  @SuppressWarnings("AssertThrowsMultipleStatements")
  public void testCorrectExceptionThrowWhenEncodingStringsWithoutEnoughSpace() throws Exception {
    String testCase = "Foooooooo";
    assertThat(CodedOutputStream.computeUInt32SizeNoTag(testCase.length()))
        .isEqualTo(CodedOutputStream.computeUInt32SizeNoTag(testCase.length() * 3));
    assertThat(CodedOutputStream.computeStringSize(1, testCase)).isEqualTo(11);

    // Tag is one byte, varint describing string length is 1 byte, string length is 9 bytes.
    // An array of size 1 will cause a failure when trying to write the varint.

    // Stream's buffering means we don't throw.
    assume().that(outputType).isNotEqualTo(OutputType.STREAM);

    for (int i = 0; i < 11; i++) {
      Coder coder = outputType.newCoder(i);
      assertThrows(OutOfSpaceException.class, () -> {
        coder.stream().writeString(1, testCase);
        coder.stream().flush();
      });
    }
  }

  @Test
  public void testDifferentStringLengths() throws Exception {
    // Test string serialization roundtrip using strings of the following lengths,
    // with ASCII and Unicode characters requiring different UTF-8 byte counts per
    // char, hence causing the length delimiter varint to sometimes require more
    // bytes for the Unicode strings than the ASCII string of the same length.
    int[] lengths =
        new int[] {
          0,
          1,
          (1 << 4) - 1, // 1 byte for ASCII and Unicode
          (1 << 7) - 1, // 1 byte for ASCII, 2 bytes for Unicode
          (1 << 11) - 1, // 2 bytes for ASCII and Unicode
          (1 << 14) - 1, // 2 bytes for ASCII, 3 bytes for Unicode
          (1 << 17) - 1,
          // 3 bytes for ASCII and Unicode
        };
    for (int i : lengths) {
      testEncodingOfString('q', i); // 1 byte per char
      testEncodingOfString('\u07FF', i); // 2 bytes per char
      testEncodingOfString('\u0981', i); // 3 bytes per char
    }
  }

  @Test
  public void testWriteSmallString() throws Exception {
    Coder coder = outputType.newCoder(10);
    coder.stream().writeStringNoTag("abc");
    coder.stream().flush();
    assertThat(coder.toByteArray()).isEqualTo(new byte[] {3, 'a', 'b', 'c'});
  }

  /**
   * Parses the given bytes using writeFixed32NoTag() and checks that the result matches the given
   * value.
   */
  private void assertWriteFixed32(byte[] data, int value) throws Exception {
    Coder coder = outputType.newCoder(data.length);
    coder.stream().writeFixed32NoTag(value);
    coder.stream().flush();
    assertThat(coder.toByteArray()).isEqualTo(data);
  }

  /**
   * Parses the given bytes using writeFixed64NoTag() and checks that the result matches the given
   * value.
   */
  private void assertWriteFixed64(byte[] data, long value) throws Exception {
    Coder coder = outputType.newCoder(data.length);
    coder.stream().writeFixed64NoTag(value);
    coder.stream().flush();
    assertThat(coder.toByteArray()).isEqualTo(data);
  }

  private static String newString(char... chars) {
    return new String(chars);
  }

  private void testEncodingOfString(char c, int length) throws Exception {
    String fullString = fullString(c, length);
    TestAllTypes testAllTypes = TestAllTypes.newBuilder().setOptionalString(fullString).build();
    Coder coder = outputType.newCoder(testAllTypes.getSerializedSize());
    testAllTypes.writeTo(coder.stream());
    coder.stream().flush();
    assertThat(fullString)
        .isEqualTo(TestAllTypes.parseFrom(coder.toByteArray()).getOptionalString());
  }

  private static String fullString(char c, int length) {
    char[] result = new char[length];
    Arrays.fill(result, c);
    return new String(result);
  }

  /**
   * Helper to construct a byte array from a bunch of bytes. The inputs are actually ints so that I
   * can use hex notation and not get stupid errors about precision.
   */
  private static byte[] bytes(int... bytesAsInts) {
    byte[] bytes = new byte[bytesAsInts.length];
    for (int i = 0; i < bytesAsInts.length; i++) {
      bytes[i] = (byte) bytesAsInts[i];
    }
    return bytes;
  }

  /**
   * Writes the given value using writeRawVarint32() and writeRawVarint64() and checks that the
   * result matches the given bytes.
   */
  @SuppressWarnings("UnnecessaryLongToIntConversion") // Intentionally tests 32-bit int values.
  private void assertWriteVarint(byte[] data, long value) throws Exception {
    // Only test 32-bit write if the value fits into an int.
    if (value == (int) value) {
      Coder coder = outputType.newCoder(10);
      coder.stream().writeUInt32NoTag((int) value);
      coder.stream().flush();
      assertThat(coder.toByteArray()).isEqualTo(data);

      // Also try computing size.
      assertThat(data).hasLength(CodedOutputStream.computeUInt32SizeNoTag((int) value));
    }

    {
      Coder coder = outputType.newCoder(10);
      coder.stream().writeUInt64NoTag(value);
      coder.stream().flush();
      assertThat(coder.toByteArray()).isEqualTo(data);

      // Also try computing size.
      assertThat(data).hasLength(CodedOutputStream.computeUInt64SizeNoTag(value));
    }
  }

  private void assertVarintRoundTrip(long value) throws Exception {
    {
      Coder coder = outputType.newCoder(10);
      coder.stream().writeUInt64NoTag(value);
      coder.stream().flush();
      byte[] bytes = coder.toByteArray();
      assertThat(bytes).hasLength(CodedOutputStream.computeUInt64SizeNoTag(value));
      CodedInputStream input = CodedInputStream.newInstance(new ByteArrayInputStream(bytes));
      assertThat(input.readRawVarint64()).isEqualTo(value);
    }

    if (value == (int) value) {
      Coder coder = outputType.newCoder(10);
      coder.stream().writeUInt32NoTag((int) value);
      coder.stream().flush();
      byte[] bytes = coder.toByteArray();
      assertThat(bytes).hasLength(CodedOutputStream.computeUInt32SizeNoTag((int) value));
      CodedInputStream input = CodedInputStream.newInstance(new ByteArrayInputStream(bytes));
      assertThat(input.readRawVarint32()).isEqualTo(value);
    }
  }
}
