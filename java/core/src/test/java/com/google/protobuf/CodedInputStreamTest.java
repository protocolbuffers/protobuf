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
import static org.junit.Assert.assertArrayEquals;
import protobuf_unittest.UnittestProto.BoolMessage;
import protobuf_unittest.UnittestProto.Int32Message;
import protobuf_unittest.UnittestProto.Int64Message;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestRecursiveMessage;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.FilterInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit test for {@link CodedInputStream}. */
@RunWith(JUnit4.class)
public class CodedInputStreamTest {

  private static final int DEFAULT_BLOCK_SIZE = 4096;

  private enum InputType {
    ARRAY {
      @Override
      CodedInputStream newDecoder(byte[] data, int blockSize) {
        return CodedInputStream.newInstance(data);
      }
    },
    NIO_HEAP {
      @Override
      CodedInputStream newDecoder(byte[] data, int blockSize) {
        return CodedInputStream.newInstance(ByteBuffer.wrap(data));
      }
    },
    NIO_DIRECT {
      @Override
      CodedInputStream newDecoder(byte[] data, int blockSize) {
        ByteBuffer buffer = ByteBuffer.allocateDirect(data.length);
        buffer.put(data);
        buffer.flip();
        return CodedInputStream.newInstance(buffer);
      }
    },
    STREAM {
      @Override
      CodedInputStream newDecoder(byte[] data, int blockSize) {
        return CodedInputStream.newInstance(new SmallBlockInputStream(data, blockSize));
      }
    },
    ITER_DIRECT {
      @Override
      CodedInputStream newDecoder(byte[] data, int blockSize) {
        if (blockSize > DEFAULT_BLOCK_SIZE) {
          blockSize = DEFAULT_BLOCK_SIZE;
        }
        ArrayList<ByteBuffer> input = new ArrayList<ByteBuffer>();
        for (int i = 0; i < data.length; i += blockSize) {
          int rl = Math.min(blockSize, data.length - i);
          ByteBuffer rb = ByteBuffer.allocateDirect(rl);
          rb.put(data, i, rl);
          rb.flip();
          input.add(rb);
        }
        return CodedInputStream.newInstance(input);
      }
    },
    STREAM_ITER_DIRECT {
      @Override
      CodedInputStream newDecoder(byte[] data, int blockSize) {
        if (blockSize > DEFAULT_BLOCK_SIZE) {
          blockSize = DEFAULT_BLOCK_SIZE;
        }
        ArrayList<ByteBuffer> input = new ArrayList<ByteBuffer>();
        for (int i = 0; i < data.length; i += blockSize) {
          int rl = Math.min(blockSize, data.length - i);
          ByteBuffer rb = ByteBuffer.allocateDirect(rl);
          rb.put(data, i, rl);
          rb.flip();
          input.add(rb);
        }
        return CodedInputStream.newInstance(new IterableByteBufferInputStream(input));
      }
    };

    CodedInputStream newDecoder(byte[] data) {
      return newDecoder(data, data.length);
    }

    abstract CodedInputStream newDecoder(byte[] data, int blockSize);
  }

  /**
   * Helper to construct a byte array from a bunch of bytes. The inputs are actually ints so that I
   * can use hex notation and not get stupid errors about precision.
   */
  private byte[] bytes(int... bytesAsInts) {
    byte[] bytes = new byte[bytesAsInts.length];
    for (int i = 0; i < bytesAsInts.length; i++) {
      bytes[i] = (byte) bytesAsInts[i];
    }
    return bytes;
  }

  /**
   * An InputStream which limits the number of bytes it reads at a time. We use this to make sure
   * that CodedInputStream doesn't screw up when reading in small blocks.
   */
  private static final class SmallBlockInputStream extends FilterInputStream {
    private final int blockSize;
    private int skipCalls;
    private int readCalls;

    public SmallBlockInputStream(byte[] data, int blockSize) {
      super(new ByteArrayInputStream(data));
      this.blockSize = blockSize;
    }

    @Override
    public int read() throws IOException {
      readCalls++;
      return super.read();
    }

    @Override
    public int read(byte[] b) throws IOException {
      readCalls++;
      return super.read(b, 0, Math.min(b.length, blockSize));
    }

    @Override
    public int read(byte[] b, int off, int len) throws IOException {
      readCalls++;
      return super.read(b, off, Math.min(len, blockSize));
    }

    @Override
    public long skip(long len) throws IOException {
      skipCalls++;
      return super.skip(Math.min(len, blockSize));
    }
  }

  private void assertDataConsumed(String msg, byte[] data, CodedInputStream input)
      throws IOException {
    assertWithMessage(msg).that(data).hasLength(input.getTotalBytesRead());
    assertWithMessage(msg).that(input.isAtEnd()).isTrue();
  }

  /**
   * Parses the given bytes using readRawVarint32() and readRawVarint64() and checks that the result
   * matches the given value.
   */
  private void assertReadVarint(byte[] data, long value) throws Exception {
    for (InputType inputType : InputType.values()) {
      // Try different block sizes.
      for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
        CodedInputStream input = inputType.newDecoder(data, blockSize);
        assertWithMessage(inputType.name()).that(input.readRawVarint32()).isEqualTo((int) value);
        assertDataConsumed(inputType.name(), data, input);

        input = inputType.newDecoder(data, blockSize);
        assertWithMessage(inputType.name()).that(input.readRawVarint64()).isEqualTo(value);
        assertDataConsumed(inputType.name(), data, input);

        input = inputType.newDecoder(data, blockSize);
        assertWithMessage(inputType.name()).that(input.readRawVarint64SlowPath()).isEqualTo(value);
        assertDataConsumed(inputType.name(), data, input);

        input = inputType.newDecoder(data, blockSize);
        assertWithMessage(inputType.name())
            .that(input.skipField(WireFormat.WIRETYPE_VARINT))
            .isTrue();
        assertDataConsumed(inputType.name(), data, input);
      }
    }

    // Try reading direct from an InputStream.  We want to verify that it
    // doesn't read past the end of the input, so we copy to a new, bigger
    // array first.
    byte[] longerData = new byte[data.length + 1];
    System.arraycopy(data, 0, longerData, 0, data.length);
    InputStream rawInput = new ByteArrayInputStream(longerData);
    assertThat(CodedInputStream.readRawVarint32(rawInput)).isEqualTo((int) value);
    assertThat(rawInput.available()).isEqualTo(1);
  }

  /**
   * Parses the given bytes using readRawVarint32() and readRawVarint64() and expects them to fail
   * with an InvalidProtocolBufferException whose description matches the given one.
   */
  private void assertReadVarintFailure(InvalidProtocolBufferException expected, byte[] data)
      throws Exception {
    for (InputType inputType : InputType.values()) {
      try {
        CodedInputStream input = inputType.newDecoder(data);
        input.readRawVarint32();
        assertWithMessage("%s: Should have thrown an exception.", inputType.name()).fail();
      } catch (InvalidProtocolBufferException e) {
        assertWithMessage(inputType.name())
            .that(e)
            .hasMessageThat()
            .isEqualTo(expected.getMessage());
      }
      try {
        CodedInputStream input = inputType.newDecoder(data);
        input.readRawVarint64();
        assertWithMessage("%s: Should have thrown an exception.", inputType.name()).fail();
      } catch (InvalidProtocolBufferException e) {
        assertWithMessage(inputType.name())
            .that(e)
            .hasMessageThat()
            .isEqualTo(expected.getMessage());
      }
    }

    // Make sure we get the same error when reading direct from an InputStream.
    try {
      CodedInputStream.readRawVarint32(new ByteArrayInputStream(data));
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (InvalidProtocolBufferException e) {
      assertThat(e).hasMessageThat().isEqualTo(expected.getMessage());
    }
  }

  /** Tests readRawVarint32() and readRawVarint64(). */
  @Test
  public void testReadVarint() throws Exception {
    assertReadVarint(bytes(0x00), 0);
    assertReadVarint(bytes(0x01), 1);
    assertReadVarint(bytes(0x7f), 127);
    // 14882
    assertReadVarint(bytes(0xa2, 0x74), (0x22 << 0) | (0x74 << 7));
    // 2961488830
    assertReadVarint(
        bytes(0xbe, 0xf7, 0x92, 0x84, 0x0b),
        (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) | (0x0bL << 28));

    // 64-bit
    // 7256456126
    assertReadVarint(
        bytes(0xbe, 0xf7, 0x92, 0x84, 0x1b),
        (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) | (0x1bL << 28));
    // 41256202580718336
    assertReadVarint(
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
    assertReadVarint(
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

    // Failures
    assertReadVarintFailure(
        InvalidProtocolBufferException.malformedVarint(),
        bytes(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));
    assertReadVarintFailure(InvalidProtocolBufferException.truncatedMessage(), bytes(0x80));
  }

  /**
   * Parses the given bytes using readRawLittleEndian32() and checks that the result matches the
   * given value.
   */
  private void assertReadLittleEndian32(byte[] data, int value) throws Exception {
    for (InputType inputType : InputType.values()) {
      // Try different block sizes.
      for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
        CodedInputStream input = inputType.newDecoder(data, blockSize);
        assertWithMessage(inputType.name()).that(input.readRawLittleEndian32()).isEqualTo(value);
        assertWithMessage(inputType.name()).that(input.isAtEnd()).isTrue();
      }
    }
  }

  /**
   * Parses the given bytes using readRawLittleEndian64() and checks that the result matches the
   * given value.
   */
  private void assertReadLittleEndian64(byte[] data, long value) throws Exception {
    for (InputType inputType : InputType.values()) {
      // Try different block sizes.
      for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
        CodedInputStream input = inputType.newDecoder(data, blockSize);
        assertWithMessage(inputType.name()).that(input.readRawLittleEndian64()).isEqualTo(value);
        assertWithMessage(inputType.name()).that(input.isAtEnd()).isTrue();
      }
    }
  }

  /** Tests readRawLittleEndian32() and readRawLittleEndian64(). */
  @Test
  public void testReadLittleEndian() throws Exception {
    assertReadLittleEndian32(bytes(0x78, 0x56, 0x34, 0x12), 0x12345678);
    assertReadLittleEndian32(bytes(0xf0, 0xde, 0xbc, 0x9a), 0x9abcdef0);

    assertReadLittleEndian64(
        bytes(0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12), 0x123456789abcdef0L);
    assertReadLittleEndian64(
        bytes(0x78, 0x56, 0x34, 0x12, 0xf0, 0xde, 0xbc, 0x9a), 0x9abcdef012345678L);
  }

  /** Test decodeZigZag32() and decodeZigZag64(). */
  @Test
  public void testDecodeZigZag() throws Exception {
    assertThat(CodedInputStream.decodeZigZag32(0)).isEqualTo(0);
    assertThat(CodedInputStream.decodeZigZag32(1)).isEqualTo(-1);
    assertThat(CodedInputStream.decodeZigZag32(2)).isEqualTo(1);
    assertThat(CodedInputStream.decodeZigZag32(3)).isEqualTo(-2);
    assertThat(CodedInputStream.decodeZigZag32(0x7FFFFFFE)).isEqualTo(0x3FFFFFFF);
    assertThat(CodedInputStream.decodeZigZag32(0x7FFFFFFF)).isEqualTo(0xC0000000);
    assertThat(CodedInputStream.decodeZigZag32(0xFFFFFFFE)).isEqualTo(0x7FFFFFFF);
    assertThat(CodedInputStream.decodeZigZag32(0xFFFFFFFF)).isEqualTo(0x80000000);

    assertThat(CodedInputStream.decodeZigZag64(0)).isEqualTo(0);
    assertThat(CodedInputStream.decodeZigZag64(1)).isEqualTo(-1);
    assertThat(CodedInputStream.decodeZigZag64(2)).isEqualTo(1);
    assertThat(CodedInputStream.decodeZigZag64(3)).isEqualTo(-2);
    assertThat(CodedInputStream.decodeZigZag64(0x000000007FFFFFFEL)).isEqualTo(0x000000003FFFFFFFL);
    assertThat(CodedInputStream.decodeZigZag64(0x000000007FFFFFFFL)).isEqualTo(0xFFFFFFFFC0000000L);
    assertThat(CodedInputStream.decodeZigZag64(0x00000000FFFFFFFEL)).isEqualTo(0x000000007FFFFFFFL);
    assertThat(CodedInputStream.decodeZigZag64(0x00000000FFFFFFFFL)).isEqualTo(0xFFFFFFFF80000000L);
    assertThat(CodedInputStream.decodeZigZag64(0xFFFFFFFFFFFFFFFEL)).isEqualTo(0x7FFFFFFFFFFFFFFFL);
    assertThat(CodedInputStream.decodeZigZag64(0xFFFFFFFFFFFFFFFFL)).isEqualTo(0x8000000000000000L);
  }

  /** Tests reading and parsing a whole message with every field type. */
  @Test
  public void testReadWholeMessage() throws Exception {
    TestAllTypes message = TestUtil.getAllSet();

    byte[] rawBytes = message.toByteArray();
    assertThat(rawBytes).hasLength(message.getSerializedSize());

    for (InputType inputType : InputType.values()) {
      // Try different block sizes.
      for (int blockSize = 1; blockSize < 256; blockSize *= 2) {
        TestAllTypes message2 = TestAllTypes.parseFrom(inputType.newDecoder(rawBytes, blockSize));
        TestUtil.assertAllFieldsSet(message2);
      }
    }
  }

  /** Tests skipField(). */
  @Test
  public void testSkipWholeMessage() throws Exception {
    TestAllTypes message = TestUtil.getAllSet();
    byte[] rawBytes = message.toByteArray();

    InputType[] inputTypes = InputType.values();
    CodedInputStream[] inputs = new CodedInputStream[inputTypes.length];
    for (int i = 0; i < inputs.length; ++i) {
      inputs[i] = inputTypes[i].newDecoder(rawBytes);
    }
    UnknownFieldSet.Builder unknownFields = UnknownFieldSet.newBuilder();

    while (true) {
      CodedInputStream input1 = inputs[0];
      int tag = input1.readTag();
      // Ensure that the rest match.
      for (int i = 1; i < inputs.length; ++i) {
        assertWithMessage(inputTypes[i].name()).that(inputs[i].readTag()).isEqualTo(tag);
      }
      if (tag == 0) {
        break;
      }
      unknownFields.mergeFieldFrom(tag, input1);
      // Skip the field for the rest of the inputs.
      for (int i = 1; i < inputs.length; ++i) {
        inputs[i].skipField(tag);
      }
    }
  }


  /**
   * Test that a bug in skipRawBytes() has been fixed: if the skip skips exactly up to a limit, this
   * should not break things.
   */
  @Test
  public void testSkipRawBytesBug() throws Exception {
    byte[] rawBytes = new byte[] {1, 2};
    for (InputType inputType : InputType.values()) {
      CodedInputStream input = inputType.newDecoder(rawBytes);
      int limit = input.pushLimit(1);
      input.skipRawBytes(1);
      input.popLimit(limit);
      assertWithMessage(inputType.name()).that(input.readRawByte()).isEqualTo(2);
    }
  }

  /**
   * Test that a bug in skipRawBytes() has been fixed: if the skip skips past the end of a buffer
   * with a limit that has been set past the end of that buffer, this should not break things.
   */
  @Test
  public void testSkipRawBytesPastEndOfBufferWithLimit() throws Exception {
    byte[] rawBytes = new byte[] {1, 2, 3, 4, 5};
    for (InputType inputType : InputType.values()) {
      CodedInputStream input = inputType.newDecoder(rawBytes);
      int limit = input.pushLimit(4);
      // In order to expose the bug we need to read at least one byte to prime the
      // buffer inside the CodedInputStream.
      assertWithMessage(inputType.name()).that(input.readRawByte()).isEqualTo(1);
      // Skip to the end of the limit.
      input.skipRawBytes(3);
      assertWithMessage(inputType.name()).that(input.isAtEnd()).isTrue();
      input.popLimit(limit);
      assertWithMessage(inputType.name()).that(input.readRawByte()).isEqualTo(5);
    }
  }

  /**
   * Test that calling skipRawBytes (when not merging a message) actually skips from the underlying
   * inputstream, regardless of the buffer size used.
   */
  @Test
  public void testSkipRawBytesActuallySkips() throws Exception {
    SmallBlockInputStream bytes = new SmallBlockInputStream(new byte[] {1, 2, 3, 4, 5}, 3);
    CodedInputStream input = CodedInputStream.newInstance(bytes, 1); // Tiny buffer
    input.skipRawBytes(3);
    input.skipRawBytes(2);
    assertThat(bytes.skipCalls).isEqualTo(2);
    assertThat(bytes.readCalls).isEqualTo(0);
  }

  @Test
  public void testSkipHugeBlob() throws Exception {
    // Allocate and initialize a 1MB blob.
    int blobSize = 1 << 20;
    byte[] blob = new byte[blobSize];
    for (int i = 0; i < blob.length; i++) {
      blob[i] = (byte) i;
    }

    for (InputType inputType : InputType.values()) {
      CodedInputStream decoder = inputType.newDecoder(blob);
      decoder.skipRawBytes(blobSize - 10);
      byte[] remaining = decoder.readRawBytes(10);
      assertArrayEquals(Arrays.copyOfRange(blob, blobSize - 10, blobSize), remaining);
    }
  }

  /** Skipping a huge blob should not allocate excessive memory, so there should be no limit */
  @Test
  public void testSkipMaliciouslyHugeBlob() throws Exception {
    InputStream is = new RepeatingInputStream(new byte[]{1}, Integer.MAX_VALUE);
    CodedInputStream.newInstance(is).skipRawBytes(Integer.MAX_VALUE);
  }

  @Test
  public void testReadHugeBlob() throws Exception {
    // Allocate and initialize a 1MB blob.
    byte[] blob = new byte[1 << 20];
    for (int i = 0; i < blob.length; i++) {
      blob[i] = (byte) i;
    }

    // Make a message containing it.
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    builder.setOptionalBytes(ByteString.copyFrom(blob));
    TestAllTypes message = builder.build();

    byte[] data = message.toByteArray();
    for (InputType inputType : InputType.values()) {
      // Serialize and parse it.  Make sure to parse from an InputStream, not
      // directly from a ByteString, so that CodedInputStream uses buffered
      // reading.
      TestAllTypes message2 = TestAllTypes.parseFrom(inputType.newDecoder(data));

      assertWithMessage(inputType.name())
          .that(message.getOptionalBytes())
          .isEqualTo(message2.getOptionalBytes());

      // Make sure all the other fields were parsed correctly.
      TestAllTypes message3 =
          TestAllTypes.newBuilder(message2)
              .setOptionalBytes(TestUtil.getAllSet().getOptionalBytes())
              .build();
      TestUtil.assertAllFieldsSet(message3);
    }
  }

  @Test
  public void testReadMaliciouslyLargeBlob() throws Exception {
    ByteString.Output rawOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(rawOutput);

    int tag = WireFormat.makeTag(1, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    output.writeRawVarint32(tag);
    output.writeRawVarint32(0x7FFFFFFF);
    output.writeRawBytes(new byte[32]); // Pad with a few random bytes.
    output.flush();

    byte[] data = rawOutput.toByteString().toByteArray();
    for (InputType inputType : InputType.values()) {
      CodedInputStream input = inputType.newDecoder(data);
      assertThat(input.readTag()).isEqualTo(tag);
      try {
        input.readBytes();
        assertWithMessage("%s: Should have thrown an exception!", inputType.name()).fail();
      } catch (InvalidProtocolBufferException e) {
        // success.
      }
    }
  }

  /**
   * Test we can do messages that are up to CodedInputStream#DEFAULT_SIZE_LIMIT in size (2G or
   * Integer#MAX_SIZE).
   *
   * @throws IOException
   */
  @Test
  public void testParseMessagesCloseTo2G() throws IOException {
    byte[] serializedMessage = getBigSerializedMessage();
    // How many of these big messages do we need to take us near our 2G limit?
    int count = Integer.MAX_VALUE / serializedMessage.length;
    // Now make an inputstream that will fake a near 2G message of messages
    // returning our big serialized message 'count' times.
    InputStream is = new RepeatingInputStream(serializedMessage, count);
    // Parse should succeed!
    TestAllTypes.parseFrom(is);
  }

  /**
   * Test there is an exception if a message exceeds CodedInputStream#DEFAULT_SIZE_LIMIT in size (2G
   * or Integer#MAX_SIZE).
   *
   * @throws IOException
   */
  @Test
  public void testParseMessagesOver2G() throws IOException {
    byte[] serializedMessage = getBigSerializedMessage();
    // How many of these big messages do we need to take us near our 2G limit?
    int count = Integer.MAX_VALUE / serializedMessage.length;
    // Now add one to take us over the limit
    count++;
    // Now make an inputstream that will fake a near 2G message of messages
    // returning our big serialized message 'count' times.
    InputStream is = new RepeatingInputStream(serializedMessage, count);
    try {
      TestAllTypes.parseFrom(is);
      assertWithMessage("Should have thrown an exception!").fail();
    } catch (InvalidProtocolBufferException e) {
      assertThat(e).hasMessageThat().contains("too large");
    }
  }

  /*
   * @return A serialized big message.
   */
  private static byte[] getBigSerializedMessage() {
    byte[] value = new byte[16 * 1024 * 1024];
    ByteString bsValue = ByteString.wrap(value);
    return TestAllTypes.newBuilder().setOptionalBytes(bsValue).build().toByteArray();
  }

  /*
   * An input stream that repeats a byte arrays' content a number of times.
   * Simulates really large input without consuming loads of memory. Used above
   * to test the parsing behavior when the input size exceeds 2G or close to it.
   */
  private static class RepeatingInputStream extends InputStream {
    private final byte[] serializedMessage;
    private final int count;
    private int index = 0;
    private int offset = 0;

    RepeatingInputStream(byte[] serializedMessage, int count) {
      this.serializedMessage = serializedMessage;
      this.count = count;
    }

    @Override
    public int read() throws IOException {
      if (this.offset == this.serializedMessage.length) {
        this.index++;
        this.offset = 0;
      }
      if (this.index == this.count) {
        return -1;
      }
      return this.serializedMessage[offset++];
    }
  }

  private TestRecursiveMessage makeRecursiveMessage(int depth) {
    if (depth == 0) {
      return TestRecursiveMessage.newBuilder().setI(5).build();
    } else {
      return TestRecursiveMessage.newBuilder().setA(makeRecursiveMessage(depth - 1)).build();
    }
  }

  private void assertMessageDepth(String msg, TestRecursiveMessage message, int depth) {
    if (depth == 0) {
      assertWithMessage(msg).that(message.hasA()).isFalse();
      assertWithMessage(msg).that(message.getI()).isEqualTo(5);
    } else {
      assertWithMessage(msg).that(message.hasA()).isTrue();
      assertMessageDepth(msg, message.getA(), depth - 1);
    }
  }

  @Test
  public void testMaliciousRecursion() throws Exception {
    byte[] data100 = makeRecursiveMessage(100).toByteArray();
    byte[] data101 = makeRecursiveMessage(101).toByteArray();

    for (InputType inputType : InputType.values()) {
      assertMessageDepth(
          inputType.name(), TestRecursiveMessage.parseFrom(inputType.newDecoder(data100)), 100);

      try {
        TestRecursiveMessage.parseFrom(inputType.newDecoder(data101));
        assertWithMessage("Should have thrown an exception!").fail();
      } catch (InvalidProtocolBufferException e) {
        // success.
      }


      CodedInputStream input = inputType.newDecoder(data100);
      input.setRecursionLimit(8);
      try {
        TestRecursiveMessage.parseFrom(input);
        assertWithMessage("%s: Should have thrown an exception!", inputType.name()).fail();
      } catch (InvalidProtocolBufferException e) {
        // success.
      }
    }
  }

  private void checkSizeLimitExceeded(InvalidProtocolBufferException e) {
    assertThat(e)
        .hasMessageThat()
        .isEqualTo(InvalidProtocolBufferException.sizeLimitExceeded().getMessage());
  }

  @Test
  public void testSizeLimit() throws Exception {
    // NOTE: Size limit only applies to the stream-backed CIS.
    CodedInputStream input =
        CodedInputStream.newInstance(
            new SmallBlockInputStream(TestUtil.getAllSet().toByteArray(), 16));
    input.setSizeLimit(16);

    try {
      TestAllTypes.parseFrom(input);
      assertWithMessage("Should have thrown an exception!").fail();
    } catch (InvalidProtocolBufferException expected) {
      checkSizeLimitExceeded(expected);
    }
  }

  @Test
  public void testResetSizeCounter() throws Exception {
    // NOTE: Size limit only applies to the stream-backed CIS.
    CodedInputStream input =
        CodedInputStream.newInstance(new SmallBlockInputStream(new byte[256], 8));
    input.setSizeLimit(16);
    input.readRawBytes(16);
    assertThat(input.getTotalBytesRead()).isEqualTo(16);

    try {
      input.readRawByte();
      assertWithMessage("Should have thrown an exception!").fail();
    } catch (InvalidProtocolBufferException expected) {
      checkSizeLimitExceeded(expected);
    }

    input.resetSizeCounter();
    assertThat(input.getTotalBytesRead()).isEqualTo(0);
    input.readRawByte(); // No exception thrown.
    input.resetSizeCounter();
    assertThat(input.getTotalBytesRead()).isEqualTo(0);
    input.readRawBytes(16);
    assertThat(input.getTotalBytesRead()).isEqualTo(16);
    input.resetSizeCounter();

    try {
      input.readRawBytes(17); // Hits limit again.
      assertWithMessage("Should have thrown an exception!").fail();
    } catch (InvalidProtocolBufferException expected) {
      checkSizeLimitExceeded(expected);
    }
  }

  @Test
  public void testRefillBufferWithCorrectSize() throws Exception {
    // NOTE: refillBuffer only applies to the stream-backed CIS.
    byte[] bytes = "123456789".getBytes("UTF-8");
    ByteArrayOutputStream rawOutput = new ByteArrayOutputStream();
    CodedOutputStream output = CodedOutputStream.newInstance(rawOutput, bytes.length);

    int tag = WireFormat.makeTag(1, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    output.writeRawVarint32(tag);
    output.writeRawVarint32(bytes.length);
    output.writeRawBytes(bytes);
    output.writeRawVarint32(tag);
    output.writeRawVarint32(bytes.length);
    output.writeRawBytes(bytes);
    output.writeRawByte(4);
    output.flush();

    // Input is two string with length 9 and one raw byte.
    byte[] rawInput = rawOutput.toByteArray();
    for (int inputStreamBufferLength = 8;
        inputStreamBufferLength <= rawInput.length + 1;
        inputStreamBufferLength++) {
      CodedInputStream input =
          CodedInputStream.newInstance(new ByteArrayInputStream(rawInput), inputStreamBufferLength);
      input.setSizeLimit(rawInput.length - 1);
      input.readString();
      input.readString();
      try {
        input.readRawByte(); // Hits limit.
        assertWithMessage("Should have thrown an exception!").fail();
      } catch (InvalidProtocolBufferException expected) {
        checkSizeLimitExceeded(expected);
      }
    }
  }

  @Test
  public void testIsAtEnd() throws Exception {
    CodedInputStream input = CodedInputStream.newInstance(new ByteArrayInputStream(new byte[5]));
    try {
      for (int i = 0; i < 5; i++) {
        assertThat(input.isAtEnd()).isFalse();
        input.readRawByte();
      }
      assertThat(input.isAtEnd()).isTrue();
    } catch (Exception e) {
      throw new AssertionError("Catch exception in the testIsAtEnd", e);
    }
  }

  @Test
  public void testCurrentLimitExceeded() throws Exception {
    byte[] bytes = "123456789".getBytes("UTF-8");
    ByteArrayOutputStream rawOutput = new ByteArrayOutputStream();
    CodedOutputStream output = CodedOutputStream.newInstance(rawOutput, bytes.length);

    int tag = WireFormat.makeTag(1, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    output.writeRawVarint32(tag);
    output.writeRawVarint32(bytes.length);
    output.writeRawBytes(bytes);
    output.flush();

    byte[] rawInput = rawOutput.toByteArray();
    CodedInputStream input = CodedInputStream.newInstance(new ByteArrayInputStream(rawInput));
    // The length of the whole rawInput
    input.setSizeLimit(11);
    // Some number that is smaller than the rawInput's length
    // but larger than 2
    input.pushLimit(5);
    try {
      input.readString();
      assertWithMessage("Should have thrown an exception").fail();
    } catch (InvalidProtocolBufferException expected) {
      assertThat(expected)
          .hasMessageThat()
          .isEqualTo(InvalidProtocolBufferException.truncatedMessage().getMessage());
    }
  }

  @Test
  public void testSizeLimitMultipleMessages() throws Exception {
    // NOTE: Size limit only applies to the stream-backed CIS.
    byte[] bytes = new byte[256];
    for (int i = 0; i < bytes.length; i++) {
      bytes[i] = (byte) i;
    }
    CodedInputStream input = CodedInputStream.newInstance(new SmallBlockInputStream(bytes, 7));
    input.setSizeLimit(16);
    for (int i = 0; i < 256 / 16; i++) {
      byte[] message = input.readRawBytes(16);
      for (int j = 0; j < message.length; j++) {
        assertThat(message[j] & 0xff).isEqualTo(i * 16 + j);
      }
      assertThat(input.getTotalBytesRead()).isEqualTo(16);
      input.resetSizeCounter();
      assertThat(input.getTotalBytesRead()).isEqualTo(0);
    }
  }

  @Test
  public void testReadString() throws Exception {
    String lorem = "Lorem ipsum dolor sit amet ";
    StringBuilder builder = new StringBuilder();
    for (int i = 0; i < 4096; i += lorem.length()) {
      builder.append(lorem);
    }
    lorem = builder.toString().substring(0, 4096);
    byte[] bytes = lorem.getBytes("UTF-8");
    ByteString.Output rawOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(rawOutput, bytes.length);

    int tag = WireFormat.makeTag(1, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    output.writeRawVarint32(tag);
    output.writeRawVarint32(bytes.length);
    output.writeRawBytes(bytes);
    output.flush();

    byte[] rawInput = rawOutput.toByteString().toByteArray();
    for (InputType inputType : InputType.values()) {
      CodedInputStream input = inputType.newDecoder(rawInput);
      assertWithMessage(inputType.name()).that(tag).isEqualTo(input.readTag());
      String text = input.readString();
      assertWithMessage(inputType.name()).that(lorem).isEqualTo(text);
    }
  }

  @Test
  public void testReadStringRequireUtf8() throws Exception {
    String lorem = "Lorem ipsum dolor sit amet ";
    StringBuilder builder = new StringBuilder();
    for (int i = 0; i < 4096; i += lorem.length()) {
      builder.append(lorem);
    }
    lorem = builder.toString().substring(0, 4096);
    byte[] bytes = lorem.getBytes("UTF-8");
    ByteString.Output rawOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(rawOutput, bytes.length);

    int tag = WireFormat.makeTag(1, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    output.writeRawVarint32(tag);
    output.writeRawVarint32(bytes.length);
    output.writeRawBytes(bytes);
    output.flush();

    byte[] rawInput = rawOutput.toByteString().toByteArray();
    for (InputType inputType : InputType.values()) {
      CodedInputStream input = inputType.newDecoder(rawInput);
      assertWithMessage(inputType.name()).that(tag).isEqualTo(input.readTag());
      String text = input.readStringRequireUtf8();
      assertWithMessage(inputType.name()).that(lorem).isEqualTo(text);
    }
  }

  /**
   * Tests that if we readString invalid UTF-8 bytes, no exception is thrown. Instead, the invalid
   * bytes are replaced with the Unicode "replacement character" U+FFFD.
   */
  @Test
  public void testReadStringInvalidUtf8() throws Exception {
    ByteString.Output rawOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(rawOutput);

    int tag = WireFormat.makeTag(1, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    output.writeRawVarint32(tag);
    output.writeRawVarint32(1);
    output.writeRawBytes(new byte[] {(byte) 0x80});
    output.flush();

    byte[] rawInput = rawOutput.toByteString().toByteArray();
    for (InputType inputType : InputType.values()) {
      CodedInputStream input = inputType.newDecoder(rawInput);
      assertWithMessage(inputType.name()).that(input.readTag()).isEqualTo(tag);
      String text = input.readString();
      assertWithMessage(inputType.name()).that(text.charAt(0)).isEqualTo(0xfffd);
    }
  }

  /**
   * Tests that if we readStringRequireUtf8 invalid UTF-8 bytes, an InvalidProtocolBufferException
   * is thrown.
   */
  @Test
  public void testReadStringRequireUtf8InvalidUtf8() throws Exception {
    ByteString.Output rawOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(rawOutput);

    int tag = WireFormat.makeTag(1, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    output.writeRawVarint32(tag);
    output.writeRawVarint32(1);
    output.writeRawBytes(new byte[] {(byte) 0x80});
    output.flush();

    byte[] rawInput = rawOutput.toByteString().toByteArray();
    for (InputType inputType : InputType.values()) {
      CodedInputStream input = inputType.newDecoder(rawInput);
      assertThat(input.readTag()).isEqualTo(tag);
      try {
        input.readStringRequireUtf8();
        assertWithMessage("%s: Expected invalid UTF-8 exception.", inputType.name()).fail();
      } catch (InvalidProtocolBufferException exception) {
        assertWithMessage(inputType.name())
            .that(exception)
            .hasMessageThat()
            .isEqualTo("Protocol message had invalid UTF-8.");
      }
    }
  }

  @Test
  public void testReadFromSlice() throws Exception {
    byte[] bytes = bytes(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    CodedInputStream in = CodedInputStream.newInstance(bytes, 3, 5);
    assertThat(in.getTotalBytesRead()).isEqualTo(0);
    for (int i = 3; i < 8; i++) {
      assertThat(in.readRawByte()).isEqualTo(i);
      assertThat(in.getTotalBytesRead()).isEqualTo(i - 2);
    }
    // eof
    assertThat(in.readTag()).isEqualTo(0);
    assertThat(in.getTotalBytesRead()).isEqualTo(5);
  }

  @Test
  public void testInvalidTag() throws Exception {
    // Any tag number which corresponds to field number zero is invalid and
    // should throw InvalidProtocolBufferException.
    for (InputType inputType : InputType.values()) {
      for (int i = 0; i < 8; i++) {
        try {
          inputType.newDecoder(bytes(i)).readTag();
          assertWithMessage("%s: Should have thrown an exception.", inputType.name()).fail();
        } catch (InvalidProtocolBufferException e) {
          assertWithMessage(inputType.name())
              .that(e)
              .hasMessageThat()
              .isEqualTo(InvalidProtocolBufferException.invalidTag().getMessage());
        }
      }
    }
  }

  @Test
  public void testReadByteArray() throws Exception {
    ByteString.Output rawOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(rawOutput);
    // Zero-sized bytes field.
    output.writeRawVarint32(0);
    // One one-byte bytes field
    output.writeRawVarint32(1);
    output.writeRawBytes(new byte[] {(byte) 23});
    // Another one-byte bytes field
    output.writeRawVarint32(1);
    output.writeRawBytes(new byte[] {(byte) 45});
    // A bytes field large enough that won't fit into the 4K buffer.
    final int bytesLength = 16 * 1024;
    byte[] bytes = new byte[bytesLength];
    bytes[0] = (byte) 67;
    bytes[bytesLength - 1] = (byte) 89;
    output.writeRawVarint32(bytesLength);
    output.writeRawBytes(bytes);

    output.flush();

    byte[] rawInput = rawOutput.toByteString().toByteArray();
    for (InputType inputType : InputType.values()) {
      CodedInputStream inputStream = inputType.newDecoder(rawInput);

      byte[] result = inputStream.readByteArray();
      assertWithMessage(inputType.name()).that(result).isEmpty();
      result = inputStream.readByteArray();
      assertWithMessage(inputType.name()).that(result).hasLength(1);
      assertWithMessage(inputType.name()).that(result[0]).isEqualTo((byte) 23);
      result = inputStream.readByteArray();
      assertWithMessage(inputType.name()).that(result).hasLength(1);
      assertWithMessage(inputType.name()).that(result[0]).isEqualTo((byte) 45);
      result = inputStream.readByteArray();
      assertWithMessage(inputType.name()).that(result).hasLength(bytesLength);
      assertWithMessage(inputType.name()).that(result[0]).isEqualTo((byte) 67);
      assertWithMessage(inputType.name()).that(result[bytesLength - 1]).isEqualTo((byte) 89);
    }
  }

  @Test
  public void testReadLargeByteStringFromInputStream() throws Exception {
    byte[] bytes = new byte[1024 * 1024];
    for (int i = 0; i < bytes.length; i++) {
      bytes[i] = (byte) (i & 0xFF);
    }
    ByteString.Output rawOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(rawOutput);
    output.writeRawVarint32(bytes.length);
    output.writeRawBytes(bytes);
    output.flush();
    byte[] data = rawOutput.toByteString().toByteArray();

    CodedInputStream input =
        CodedInputStream.newInstance(
            new ByteArrayInputStream(data) {
              @Override
              public synchronized int available() {
                return 0;
              }
            });
    ByteString result = input.readBytes();
    assertThat(ByteString.copyFrom(bytes)).isEqualTo(result);
  }

  @Test
  public void testReadLargeByteArrayFromInputStream() throws Exception {
    byte[] bytes = new byte[1024 * 1024];
    for (int i = 0; i < bytes.length; i++) {
      bytes[i] = (byte) (i & 0xFF);
    }
    ByteString.Output rawOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(rawOutput);
    output.writeRawVarint32(bytes.length);
    output.writeRawBytes(bytes);
    output.flush();
    byte[] data = rawOutput.toByteString().toByteArray();

    CodedInputStream input =
        CodedInputStream.newInstance(
            new ByteArrayInputStream(data) {
              @Override
              public synchronized int available() {
                return 0;
              }
            });
    byte[] result = input.readByteArray();
    assertThat(Arrays.equals(bytes, result)).isTrue();
  }

  @Test
  public void testReadByteBuffer() throws Exception {
    ByteString.Output rawOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(rawOutput);
    // Zero-sized bytes field.
    output.writeRawVarint32(0);
    // One one-byte bytes field
    output.writeRawVarint32(1);
    output.writeRawBytes(new byte[] {(byte) 23});
    // Another one-byte bytes field
    output.writeRawVarint32(1);
    output.writeRawBytes(new byte[] {(byte) 45});
    // A bytes field large enough that won't fit into the 4K buffer.
    final int bytesLength = 16 * 1024;
    byte[] bytes = new byte[bytesLength];
    bytes[0] = (byte) 67;
    bytes[bytesLength - 1] = (byte) 89;
    output.writeRawVarint32(bytesLength);
    output.writeRawBytes(bytes);

    output.flush();

    byte[] rawInput = rawOutput.toByteString().toByteArray();
    for (InputType inputType : InputType.values()) {
      CodedInputStream inputStream = inputType.newDecoder(rawInput);

      ByteBuffer result = inputStream.readByteBuffer();
      assertWithMessage(inputType.name()).that(result.capacity()).isEqualTo(0);
      result = inputStream.readByteBuffer();
      assertWithMessage(inputType.name()).that(result.capacity()).isEqualTo(1);
      assertWithMessage(inputType.name()).that(result.get()).isEqualTo((byte) 23);
      result = inputStream.readByteBuffer();
      assertWithMessage(inputType.name()).that(result.capacity()).isEqualTo(1);
      assertWithMessage(inputType.name()).that(result.get()).isEqualTo((byte) 45);
      result = inputStream.readByteBuffer();
      assertWithMessage(inputType.name()).that(result.capacity()).isEqualTo(bytesLength);
      assertWithMessage(inputType.name()).that(result.get()).isEqualTo((byte) 67);
      result.position(bytesLength - 1);
      assertWithMessage(inputType.name()).that(result.get()).isEqualTo((byte) 89);
    }
  }

  @Test
  public void testReadByteBufferAliasing() throws Exception {
    ByteArrayOutputStream byteArrayStream = new ByteArrayOutputStream();
    CodedOutputStream output = CodedOutputStream.newInstance(byteArrayStream);
    // Zero-sized bytes field.
    output.writeRawVarint32(0);
    // One one-byte bytes field
    output.writeRawVarint32(1);
    output.writeRawBytes(new byte[] {(byte) 23});
    // Another one-byte bytes field
    output.writeRawVarint32(1);
    output.writeRawBytes(new byte[] {(byte) 45});
    // A bytes field large enough that won't fit into the 4K buffer.
    final int bytesLength = 16 * 1024;
    byte[] bytes = new byte[bytesLength];
    bytes[0] = (byte) 67;
    bytes[bytesLength - 1] = (byte) 89;
    output.writeRawVarint32(bytesLength);
    output.writeRawBytes(bytes);
    output.flush();

    byte[] data = byteArrayStream.toByteArray();

    for (InputType inputType : InputType.values()) {
      if (inputType == InputType.STREAM
          || inputType == InputType.STREAM_ITER_DIRECT
          || inputType == InputType.ITER_DIRECT) {
        // Aliasing doesn't apply to stream-backed CIS.
        continue;
      }

      // Without aliasing
      CodedInputStream inputStream = inputType.newDecoder(data);
      ByteBuffer result = inputStream.readByteBuffer();
      assertWithMessage(inputType.name()).that(result.capacity()).isEqualTo(0);
      result = inputStream.readByteBuffer();
      assertWithMessage(inputType.name()).that(result.array() != data).isTrue();
      assertWithMessage(inputType.name()).that(result.capacity()).isEqualTo(1);
      assertWithMessage(inputType.name()).that(result.get()).isEqualTo((byte) 23);
      result = inputStream.readByteBuffer();
      assertWithMessage(inputType.name()).that(result.array() != data).isTrue();
      assertWithMessage(inputType.name()).that(result.capacity()).isEqualTo(1);
      assertWithMessage(inputType.name()).that(result.get()).isEqualTo((byte) 45);
      result = inputStream.readByteBuffer();
      assertWithMessage(inputType.name()).that(result.array() != data).isTrue();
      assertWithMessage(inputType.name()).that(result.capacity()).isEqualTo(bytesLength);
      assertWithMessage(inputType.name()).that(result.get()).isEqualTo((byte) 67);
      result.position(bytesLength - 1);
      assertWithMessage(inputType.name()).that(result.get()).isEqualTo((byte) 89);

      // Enable aliasing
      inputStream = inputType.newDecoder(data, data.length);
      inputStream.enableAliasing(true);
      result = inputStream.readByteBuffer();
      assertWithMessage(inputType.name()).that(result.capacity()).isEqualTo(0);
      result = inputStream.readByteBuffer();
      if (result.hasArray()) {
        assertWithMessage(inputType.name()).that(result.array() == data).isTrue();
      }
      assertWithMessage(inputType.name()).that(result.capacity()).isEqualTo(1);
      assertWithMessage(inputType.name()).that(result.get()).isEqualTo((byte) 23);
      result = inputStream.readByteBuffer();
      if (result.hasArray()) {
        assertWithMessage(inputType.name()).that(result.array() == data).isTrue();
      }
      assertWithMessage(inputType.name()).that(result.capacity()).isEqualTo(1);
      assertWithMessage(inputType.name()).that(result.get()).isEqualTo((byte) 45);
      result = inputStream.readByteBuffer();
      if (result.hasArray()) {
        assertWithMessage(inputType.name()).that(result.array() == data).isTrue();
      }
      assertWithMessage(inputType.name()).that(result.capacity()).isEqualTo(bytesLength);
      assertWithMessage(inputType.name()).that(result.get()).isEqualTo((byte) 67);
      result.position(bytesLength - 1);
      assertWithMessage(inputType.name()).that(result.get()).isEqualTo((byte) 89);
    }
  }

  @Test
  public void testIterableByteBufferInputStreamReadBytesWithAlias() throws Exception {
    ByteArrayOutputStream byteArrayStream = new ByteArrayOutputStream();
    CodedOutputStream output = CodedOutputStream.newInstance(byteArrayStream);
    // A bytes field large enough that won't fit into the default block buffer.
    // 4.5 is to test the case where the total size of input is not aligned with DEFAULT_BLOCK_SIZE.
    final int bytesLength = DEFAULT_BLOCK_SIZE * 4 + (DEFAULT_BLOCK_SIZE / 2);
    byte[] bytes = new byte[bytesLength];
    for (int i = 0; i < bytesLength; i++) {
      bytes[i] = (byte) (i % 256);
    }
    output.writeByteArrayNoTag(bytes);
    output.flush();

    // Input data is split into multiple ByteBuffers so that a single bytes spans across them.
    // CodedInputStream with aliasing will decode it as a consequent rope by wrapping ByteBuffers.
    byte[] data = byteArrayStream.toByteArray();
    ArrayList<ByteBuffer> input = new ArrayList<>();
    for (int i = 0; i < data.length; i += DEFAULT_BLOCK_SIZE) {
      int rl = Math.min(DEFAULT_BLOCK_SIZE, data.length - i);
      ByteBuffer rb = ByteBuffer.allocateDirect(rl);
      rb.put(data, i, rl);
      rb.flip();
      input.add(rb);
    }
    final CodedInputStream inputStream = CodedInputStream.newInstance(input, true);
    inputStream.enableAliasing(true);

    ByteString result = inputStream.readBytes();
    for (int i = 0; i < bytesLength; i++) {
      assertThat(result.byteAt(i)).isEqualTo((byte) (i % 256));
    }
  }

  @Test
  public void testCompatibleTypes() throws Exception {
    long data = 0x100000000L;
    Int64Message message = Int64Message.newBuilder().setData(data).build();
    byte[] serialized = message.toByteArray();
    for (InputType inputType : InputType.values()) {
      CodedInputStream inputStream = inputType.newDecoder(serialized);

      // Test int64(long) is compatible with bool(boolean)
      BoolMessage msg2 = BoolMessage.parseFrom(inputStream);
      assertThat(msg2.getData()).isTrue();

      // Test int64(long) is compatible with int32(int)
      inputStream = inputType.newDecoder(serialized);
      Int32Message msg3 = Int32Message.parseFrom(inputStream);
      assertThat(msg3.getData()).isEqualTo((int) data);
    }
  }

  @Test
  public void testSkipInvalidVarint_FastPath() throws Exception {
    // Fast path: We have >= 10 bytes available. Ensure we properly recognize a non-ending varint.
    byte[] data = new byte[] {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0};
    for (InputType inputType : InputType.values()) {
      try {
        CodedInputStream input = inputType.newDecoder(data);
        input.skipField(WireFormat.makeTag(1, WireFormat.WIRETYPE_VARINT));
        assertWithMessage("%s: Should have thrown an exception.", inputType.name()).fail();
      } catch (InvalidProtocolBufferException e) {
        // Expected
      }
    }
  }

  @Test
  public void testSkipInvalidVarint_SlowPath() throws Exception {
    // Slow path: < 10 bytes available. Ensure we properly recognize a non-ending varint.
    byte[] data = new byte[] {-1, -1, -1, -1, -1, -1, -1, -1, -1};
    for (InputType inputType : InputType.values()) {
      try {
        CodedInputStream input = inputType.newDecoder(data);
        input.skipField(WireFormat.makeTag(1, WireFormat.WIRETYPE_VARINT));
        assertWithMessage("%s: Should have thrown an exception.", inputType.name()).fail();
      } catch (InvalidProtocolBufferException e) {
        // Expected
      }
    }
  }

  @Test
  public void testSkipPastEndOfByteArrayInput() throws Exception {
    try {
      CodedInputStream.newInstance(new ByteArrayInputStream(new byte[100])).skipRawBytes(101);
      assertWithMessage("Should have thrown an exception").fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected
    }
  }

  @Test
  public void testMaliciousInputStream() throws Exception {
    ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
    CodedOutputStream codedOutputStream = CodedOutputStream.newInstance(outputStream);
    codedOutputStream.writeByteArrayNoTag(new byte[] { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5 });
    codedOutputStream.flush();
    final List<byte[]> maliciousCapture = new ArrayList<>();
    InputStream inputStream = new ByteArrayInputStream(outputStream.toByteArray()) {
      @Override
      public synchronized int read(byte[] b, int off, int len) {
        maliciousCapture.add(b);
        return super.read(b, off, len);
      }
    };

    // test ByteString

    CodedInputStream codedInputStream = CodedInputStream.newInstance(inputStream, 1);
    ByteString byteString = codedInputStream.readBytes();
    assertThat(byteString.byteAt(0)).isEqualTo(0x0);
    maliciousCapture.get(1)[0] = 0x9;
    assertThat(byteString.byteAt(0)).isEqualTo(0x0);

    // test ByteBuffer

    inputStream.reset();
    maliciousCapture.clear();
    codedInputStream = CodedInputStream.newInstance(inputStream, 1);
    ByteBuffer byteBuffer = codedInputStream.readByteBuffer();
    assertThat(byteBuffer.get(0)).isEqualTo(0x0);
    maliciousCapture.get(1)[0] = 0x9;
    assertThat(byteBuffer.get(0)).isEqualTo(0x0);

    // test byte[]

    inputStream.reset();
    maliciousCapture.clear();
    codedInputStream = CodedInputStream.newInstance(inputStream, 1);
    byte[] byteArray = codedInputStream.readByteArray();
    assertThat(byteArray[0]).isEqualTo(0x0);
    maliciousCapture.get(1)[0] = 0x9;
    assertThat(byteArray[0]).isEqualTo(0x9); // MODIFICATION! Should we fix?

    // test rawBytes

    inputStream.reset();
    maliciousCapture.clear();
    codedInputStream = CodedInputStream.newInstance(inputStream, 1);
    int length = codedInputStream.readRawVarint32();
    byteArray = codedInputStream.readRawBytes(length);
    assertThat(byteArray[0]).isEqualTo(0x0);
    maliciousCapture.get(1)[0] = 0x9;
    assertThat(byteArray[0]).isEqualTo(0x9); // MODIFICATION! Should we fix?
  }

  @Test
  public void testInvalidInputYieldsInvalidProtocolBufferException_readTag() throws Exception {
    byte[] input = new byte[] {0x0a, (byte) 0xff, (byte) 0xff, (byte) 0xff, (byte) 0xff, 0x77};
    CodedInputStream inputStream = CodedInputStream.newInstance(input);
    try {
      inputStream.readTag();
      int size = inputStream.readRawVarint32();
      inputStream.pushLimit(size);
      inputStream.readTag();
      assertWithMessage("Should have thrown an exception").fail();
    } catch (InvalidProtocolBufferException ex) {
      // Expected.
    }
  }

  @Test
  public void testInvalidInputYieldsInvalidProtocolBufferException_readBytes() throws Exception {
    byte[] input =
        new byte[] {0x0a, (byte) 0xff, (byte) 0xff, (byte) 0xff, (byte) 0xff, 0x67, 0x1a, 0x1a};
    CodedInputStream inputStream = CodedInputStream.newInstance(input);
    try {
      inputStream.readTag();
      int size = inputStream.readRawVarint32();
      inputStream.pushLimit(size);
      inputStream.readBytes();
      assertWithMessage("Should have thrown an exception").fail();
    } catch (InvalidProtocolBufferException ex) {
      // Expected.
    }
  }
}
