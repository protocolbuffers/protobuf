// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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

import static java.lang.Math.min;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CoderResult;

/**
 * A protobuf {@link Encoder} that does little to no buffering for fields. It's not fully zero-copy
 * in that it does maintain an internal buffer for small operations. Large fields, however, are
 * written directly through to the {@link ZeroCopyEncoder.Handler}.
 */
public final class ZeroCopyEncoder implements Encoder {
  /**
   * Default internal buffer size used if not specified.
   */
  private static final int DEFAULT_BUFFER_SIZE = 4096;

  private static final int MAX_VARINT_SIZE = 10;

  /**
   * Java 1.8 defines Integer.BYTES, but we need this for older versions.
   */
  private static final int INTEGER_BYTES = Integer.SIZE / 8;
  private static final int LONG_BYTES = Long.SIZE / 8;

  private final Handler handler;

  /**
   * Internal buffer used to avoid writing small chunks of data. This is guaranteed to be backed by
   * an array.
   */
  private final ByteBuffer buffer;

  public ZeroCopyEncoder(Handler handler) {
    this(handler, DEFAULT_BUFFER_SIZE);
  }

  public ZeroCopyEncoder(Handler handler, int bufferSize) {
    this(handler, allocateBuffer(bufferSize));
  }

  private static ByteBuffer allocateBuffer(int bufferSize) {
    if (bufferSize <= 0) {
      throw new IllegalArgumentException("bufferSize must be > 0");
    }
    return ByteBuffer.allocateDirect(bufferSize);
  }

  public ZeroCopyEncoder(Handler handler, ByteBuffer buffer) {
    if (handler == null) {
      throw new NullPointerException("writer");
    }
    if (buffer == null) {
      throw new NullPointerException("buffer");
    }
    if (!buffer.hasRemaining()) {
      throw new IllegalArgumentException("buffer has no capacity");
    }
    if (buffer.isReadOnly()) {
      throw new IllegalArgumentException("buffer is readonly");
    }

    this.handler = handler;
    this.buffer = buffer.slice();

    // Use little endian for the putInt and putLong methods.
    buffer.order(ByteOrder.LITTLE_ENDIAN);
  }

  @Override
  public void writeTag(int fieldNumber, int wireType) throws IOException {
    ensureCapacity(MAX_VARINT_SIZE);
    writeRawVarint32(WireFormat.makeTag(fieldNumber, wireType));
  }

  @Override
  public void writeInt32(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeInt32NoTag(value);
  }

  @Override
  public void writeUInt32(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeUInt32NoTag(value);
  }

  @Override
  public void writeSInt32(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeSInt32NoTag(value);
  }

  @Override
  public void writeFixed32(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
    writeFixed32NoTag(value);
  }

  @Override
  public void writeSFixed32(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
    writeSFixed32NoTag(value);
  }

  @Override
  public void writeInt64(int fieldNumber, long value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeInt64NoTag(value);
  }

  @Override
  public void writeUInt64(int fieldNumber, long value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeUInt64NoTag(value);
  }

  @Override
  public void writeSInt64(int fieldNumber, long value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeSInt64NoTag(value);
  }

  @Override
  public void writeFixed64(int fieldNumber, long value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
    writeFixed64NoTag(value);
  }

  @Override
  public void writeSFixed64(int fieldNumber, long value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
    writeSFixed64NoTag(value);
  }

  @Override
  public void writeFloat(int fieldNumber, float value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
    writeFloatNoTag(value);
  }

  @Override
  public void writeDouble(int fieldNumber, double value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
    writeDoubleNoTag(value);
  }

  @Override
  public void writeBool(int fieldNumber, boolean value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeBoolNoTag(value);
  }

  @Override
  public void writeString(int fieldNumber, String value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    writeStringNoTag(value);
  }

  @Override
  public void writeGroup(int fieldNumber, MessageLite value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_START_GROUP);
    writeGroupNoTag(value);
    writeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP);
  }

  @Override
  public void writeMessage(int fieldNumber, MessageLite value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    writeMessageNoTag(value);
  }

  @Override
  public void writeEnum(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeEnumNoTag(value);
  }

  @Override
  public void writeMessageSetExtension(int fieldNumber, MessageLite value) throws IOException {
    writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_START_GROUP);
    writeUInt32(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber);
    writeMessage(WireFormat.MESSAGE_SET_MESSAGE, value);
    writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_END_GROUP);
  }

  @Override
  public void writeRawMessageSetExtension(int fieldNumber, ByteString value) throws IOException {
    writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_START_GROUP);
    writeUInt32(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber);
    writeBytes(WireFormat.MESSAGE_SET_MESSAGE, value);
    writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_END_GROUP);
  }

  @Override
  public void writeBytes(int fieldNumber, ByteString value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    writeBytesNoTag(value);
  }

  @Override
  public void writeByteArray(int fieldNumber, byte[] value, int offset, int length)
          throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    writeByteArrayNoTag(value, offset, length);
  }

  @Override
  public void writeDoubleNoTag(double value) throws IOException {
    writeRawLittleEndian64(Double.doubleToRawLongBits(value));
  }

  @Override
  public void writeFloatNoTag(float value) throws IOException {
    writeRawLittleEndian32(Float.floatToRawIntBits(value));
  }

  @Override
  public void writeUInt64NoTag(long value) throws IOException {
    writeRawVarint64(value);
  }

  @Override
  public void writeInt64NoTag(long value) throws IOException {
    writeRawVarint64(value);
  }

  @Override
  public void writeInt32NoTag(int value) throws IOException {
    ensureCapacity(MAX_VARINT_SIZE);
    if (value >= 0) {
      writeRawVarint32(value);
    } else {
      // Must sign-extend.
      writeRawVarint64(value);
    }
  }

  @Override
  public void writeFixed64NoTag(long value) throws IOException {
    writeRawLittleEndian64(value);
  }

  @Override
  public void writeFixed32NoTag(int value) throws IOException {
    writeRawLittleEndian32(value);
  }

  @Override
  public void writeBoolNoTag(boolean value) throws IOException {
    ensureCapacity(1);
    buffer.put((byte) (value ? 1 : 0));
  }

  @Override
  public void writeStringNoTag(String value) throws IOException {
    // UTF-8 byte length of the string is at least its UTF-16 code unit length (value.length()),
    // and at most 3 times of it. We take advantage of this in both branches below.
    int maxEncodedSize = value.length() * Utf8.MAX_BYTES_PER_CHAR;
    final int maxLengthVarIntSize = WireFormat.computeRawVarint32Size(maxEncodedSize);
    final int maxRequiredSize = maxEncodedSize + maxLengthVarIntSize;

    int capacity = buffer.remaining();
    if (capacity >= maxRequiredSize) {
      // Optimize for the case where we know this length results in a constant varint length as this
      // saves a pass for measuring the length of the string.
      int minLengthVarIntSize = WireFormat.computeRawVarint32Size(value.length());
      if (minLengthVarIntSize == maxLengthVarIntSize) {
        // Save the current position and increment past the length field. We'll come back
        // and write the length field after the encoding is complete.
        int startPos = buffer.position();
        buffer.position(buffer.position() + minLengthVarIntSize);

        // Encode the string.
        encodeString(value);

        // Now go back to the beginning and write the length.
        int endPos = buffer.position();
        buffer.position(startPos);
        writeRawVarint32(endPos - startPos);

        // Reposition the buffer past the written data.
        buffer.position(endPos);
      } else {
        int length = Utf8.encodedLength(value);
        writeRawVarint32(length);
        encodeString(value);
      }
    } else {
      // There is not enough space to write the entire String. Iterate, writing parts of the
      // string and flushing as necessary.
      int length = Utf8.encodedLength(value);
      writeRawVarint32(length);
      int srcIx = 0;

      while (srcIx < value.length()) {
        int numChars = buffer.remaining() / Utf8.MAX_BYTES_PER_CHAR;
        if (numChars == 0) {
          // Buffer is full, flush it.
          flush();
          numChars = buffer.remaining() / Utf8.MAX_BYTES_PER_CHAR;
        }

        encodeString(value.substring(srcIx, srcIx + numChars));
        srcIx += numChars;
      }
    }
  }

  private void encodeString(String value) {
    try {
      // Encode the string.
      CoderResult coderResult = PlatformDependent.encode(value, buffer);
      if (!coderResult.isUnderflow()) {
        coderResult.throwException();
      }
    } catch (CharacterCodingException e) {
      throw new RuntimeException(e);
    }
  }

  @Override
  public void writeGroupNoTag(MessageLite value) throws IOException {
    value.writeTo(this);
  }

  @Override
  public void writeMessageNoTag(MessageLite value) throws IOException {
    ensureCapacity(MAX_VARINT_SIZE);
    writeRawVarint32(value.getSerializedSize());
    value.writeTo(this);
  }

  @Override
  public void writeBytesNoTag(ByteString value) throws IOException {
    ensureCapacity(MAX_VARINT_SIZE);
    writeRawVarint32(value.size());
    writeRawBytes(value, 0, value.size());
  }

  @Override
  public void writeByteArrayNoTag(byte[] value, int offset, int length) throws IOException {
    ensureCapacity(MAX_VARINT_SIZE);
    writeRawVarint32(length);
    writeRawBytes(value, offset, length);
  }

  @Override
  public void writeUInt32NoTag(int value) throws IOException {
    ensureCapacity(MAX_VARINT_SIZE);
    writeRawVarint32(value);
  }

  @Override
  public void writeEnumNoTag(int value) throws IOException {
    writeInt32NoTag(value);
  }

  @Override
  public void writeSFixed32NoTag(int value) throws IOException {
    writeRawLittleEndian32(value);
  }

  @Override
  public void writeSFixed64NoTag(long value) throws IOException {
    writeRawLittleEndian64(value);
  }

  @Override
  public void writeSInt32NoTag(int value) throws IOException {
    ensureCapacity(MAX_VARINT_SIZE);
    writeRawVarint32(WireFormat.encodeZigZag32(value));
  }

  @Override
  public void writeSInt64NoTag(long value) throws IOException {
    ensureCapacity(MAX_VARINT_SIZE);
    writeRawVarint64(WireFormat.encodeZigZag64(value));
  }

  @Override
  public void flush() throws IOException {
    if (buffer.position() == 0) {
      // Nothing to flush.
      return;
    }

    // Write the data to the handler and require that it be copied.
    buffer.flip();
    handler.onDataEncoded(buffer, true);

    // Clear the buffer.
    buffer.clear();
  }

  private void writeRawBytes(byte[] value, int offset, int length) throws IOException {
    if (length == 0) {
      return;
    }

    if (length > buffer.capacity()) {
      // It's a large buffer, just flush any previous data and onDataEncoded the buffer directly.
      flush();

      // Write a read-only view of this buffer. The new buffer has its own position and limit.
      handler.onDataEncoded(value, offset, length, false);
      return;
    }

    int capacity = buffer.remaining();
    if (capacity >= length) {
      // The entire value can be buffered.
      buffer.put(value, offset, length);
      return;
    }

    // It's a small buffer that won't completely fit into the buffer. Write it to the buffer,
    // flushing as necessary.
    int end = offset + length;
    while (offset < end) {
      if (capacity == 0) {
        // The buffer is full, flush it.
        flush();
        capacity = buffer.remaining();
      }

      // Determine the number of bytes that can be written before another flush is required.
      int bytesToWrite = min(end - offset, capacity);

      buffer.put(value, offset, bytesToWrite);
      offset += bytesToWrite;
      capacity -= bytesToWrite;
    }
  }

  private void writeRawBytes(ByteString value, int offset, int length) throws IOException {
    if (length > buffer.capacity()) {
      // It's a large buffer. Flush any currently buffered data and write out the value
      // to the handler directly.
      flush();

      if (value instanceof LiteralByteString) {
        LiteralByteString lbs = (LiteralByteString) value;
        handler.onDataEncoded(lbs.bytes, lbs.getOffsetIntoBytes() + offset, length, false);
      } else if (value instanceof ByteBufferByteString) {
        ByteBuffer buf = ((ByteBufferByteString) value).buffer().asReadOnlyBuffer();
        buf.position(offset);
        buf.limit(offset + length);
        handler.onDataEncoded(buf, false);
      } else {
        for (ByteBuffer buf : value.substring(offset, offset + length).asReadOnlyByteBufferList()) {
          handler.onDataEncoded(buf, false);
        }
      }
      return;
    }

    int capacity = buffer.remaining();

    // If the buffer will hold the entire value, just add it to the buffer.
    if (capacity >= length) {
      value.copyTo(buffer, offset, length);
      return;
    }

    // Write enough bytes to fill the buffer.
    value.copyTo(buffer, offset, capacity);
    offset += capacity;
    length -= capacity;

    // Flush the content of the buffer to the handler.
    flush();

    // We now have space for the remaining data.
    value.copyTo(buffer, offset, length);
    buffer.position(buffer.position() + length);
  }

  private void writeRawVarint32(int value) {
    while (true) {
      if ((value & ~0x7F) == 0) {
        buffer.put((byte) value);
        return;
      }

      buffer.put((byte) ((value & 0x7F) | 0x80));
      value >>>= 7;
    }
  }

  private void writeRawVarint64(long value) throws IOException {
    while (true) {
      if ((value & ~0x7FL) == 0) {
        buffer.put((byte) value);
        return;
      }

      buffer.put((byte) ((value & 0x7F) | 0x80));
      value >>>= 7;
    }
  }

  private void writeRawLittleEndian32(int value) throws IOException {
    ensureCapacity(INTEGER_BYTES);
    buffer.putInt(value);
  }

  private void writeRawLittleEndian64(long value) throws IOException {
    ensureCapacity(LONG_BYTES);
    buffer.putLong(value);
  }

  private void ensureCapacity(int bytes) throws IOException {
    if (buffer.remaining() < bytes) {
      flush();
    }
  }
}
