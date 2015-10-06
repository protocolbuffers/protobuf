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

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * A protobuf {@link Encoder} that does little to no buffering for fields. It's not fully
 * zero-copy in that it does maintain an internal buffer for small operations. Large fields,
 * however, are written directly through.
 */
public final class ZeroCopyEncoder implements Encoder {
  // TODO(nmittler): Consider using an allocator for buffers to allow pooling.
  // TODO(nmittler): Need experimentation with good values.
  private static final int SMALL_BUFFER = 1024;
  private static final double COPY_THRESHOLD = 0.75;

  /**
   * Java 1.8 defines Integer.BYTES, but we need this for older versions.
   */
  private static final int INTEGER_BYTES = Integer.SIZE / 8;
  private static final int LONG_BYTES = Long.SIZE / 8;

  private final Handler handler;
  private final int bufferSize;

  /**
   * Internal buffer used to avoid writing small chunks of data. This is guaranteed to be
   * backed by an array.
   */
  private ByteBuffer buffer;

  /**
   * A target to receive individual writes from this marshaller.
   */
  public interface Handler {
    /**
     * Handler for encoded data. It is assumed that this data will not change.
     */
    void onDataEncoded(byte[] b, int offset, int length) throws IOException;

    /**
     * Handler for an encoded, read-only {@link ByteBuffer}. It is assumed that this data
     * will not change.
     */
    void onDataEncoded(ByteBuffer data) throws IOException;
  }

  public ZeroCopyEncoder(Handler handler) {
    this(handler, SMALL_BUFFER);
  }

  public ZeroCopyEncoder(Handler handler, int bufferSize) {
    if (handler == null) {
      throw new NullPointerException("writer");
    }

    this.handler = handler;
    this.bufferSize = Math.max(bufferSize, SMALL_BUFFER);
  }

  @Override
  public void writeDouble(int fieldNumber, double value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
    writeDoubleNoTag(value);
  }

  @Override
  public void writeFloat(int fieldNumber, float value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
    writeFloatNoTag(value);
  }

  @Override
  public void writeUInt64(int fieldNumber, long value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeUInt64NoTag(value);
  }

  @Override
  public void writeInt64(int fieldNumber, long value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeInt64NoTag(value);
  }

  @Override
  public void writeInt32(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeInt32NoTag(value);
  }

  @Override
  public void writeFixed64(int fieldNumber, long value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
    writeFixed64NoTag(value);
  }

  @Override
  public void writeFixed32(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
    writeFixed32NoTag(value);
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
  public void writeBytes(int fieldNumber, ByteString value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    writeBytesNoTag(value);
  }

  @Override
  public void writeByteArray(int fieldNumber, byte[] value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    writeByteArrayNoTag(value);
  }

  @Override
  public void writeByteArray(int fieldNumber, byte[] value, int offset, int length) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    writeByteArrayNoTag(value, offset, length);
  }

  @Override
  public void writeUInt32(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeUInt32NoTag(value);
  }

  @Override
  public void writeEnum(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeEnumNoTag(value);
  }

  @Override
  public void writeSFixed32(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
    writeSFixed32NoTag(value);
  }

  @Override
  public void writeSFixed64(int fieldNumber, long value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
    writeSFixed64NoTag(value);
  }

  @Override
  public void writeSInt32(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeSInt32NoTag(value);
  }

  @Override
  public void writeSInt64(int fieldNumber, long value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeSInt64NoTag(value);
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
    writeRawByte(value ? 1 : 0);
  }

  @Override
  public void writeStringNoTag(String value) throws IOException {
    //byte[] bytes = value.getBytes("UTF-8");
    //writeRawBytes(bytes);
    // UTF-8 byte length of the string is at least its UTF-16 code unit length (value.length()),
    // and at most 3 times of it. We take advantage of this in both branches below.
    final int maxEncodedSize = value.length() * Utf8.MAX_BYTES_PER_CHAR;
    final int maxLengthVarIntSize = computeRawVarint32Size(maxEncodedSize);
    final int maxRequiredSize = maxEncodedSize + maxLengthVarIntSize;

    int capacity = capacity();
    if (capacity >= maxRequiredSize) {
      // Optimize for the case where we know this length results in a constant varint length as this
      // saves a pass for measuring the length of the string.
      final int minLengthVarIntSize = computeRawVarint32Size(value.length());

      getOrCreateBuffer();
      int oldPosition = buffer.position();
      if (minLengthVarIntSize == maxLengthVarIntSize) {
        int position = oldPosition + minLengthVarIntSize;
        int newPosition = Utf8.encode(value, getOrCreateBuffer().array(),
                buffer.arrayOffset() + position,
                capacity - position);
        // Since this class is stateful and tracks the position, we rewind and store the state,
        // prepend the length, then reset it back to the end of the string.
        buffer.position(oldPosition);
        int length = newPosition - oldPosition - minLengthVarIntSize;
        writeRawVarint32(length);
        buffer.position(newPosition);
      } else {
        int length = Utf8.encodedLength(value);
        writeRawVarint32(length);
        int position = Utf8.encode(value, getOrCreateBuffer().array(),
                buffer.arrayOffset() + buffer.position(),
                capacity - buffer.position());
        buffer.position(position);
      }
    } else {
      // There is not enough space to onDataEncoded the entire string to the buffer.

      // Calculate the encoded length of the string and onDataEncoded it to the buffer.
      int encodedLength = Utf8.encodedLength(value);
      writeRawVarint32(encodedLength);

      // Write encoded chunks and flush as needed until the entire encoded string is written.
      int charPos = 0;
      while (charPos < value.length()) {
        capacity = capacity();
        int charsToWrite = Math.min(value.length() - charPos, capacity / Utf8.MAX_BYTES_PER_CHAR);
        if (charsToWrite == 0) {
          // The buffer is full, flush it and try again.
          flush();
          continue;
        }

        getOrCreateBuffer();

        // Write a chunk of the string to buffer.
        String chunk = value.substring(charPos, charPos + charsToWrite);
        int position = Utf8.encode(chunk, buffer.array(),
                buffer.arrayOffset() + buffer.position(),
                bufferSize - buffer.position());
        buffer.position(position);
        charPos += charsToWrite;
      }
    }
  }

  /**
   * Compute the number of bytes that would be needed to encode a varint. {@code value} is treated
   * as unsigned, so it won't be sign-extended if negative.
   */
  private static int computeRawVarint32Size(final int value) {
    if ((value & (0xffffffff << 7)) == 0) return 1;
    if ((value & (0xffffffff << 14)) == 0) return 2;
    if ((value & (0xffffffff << 21)) == 0) return 3;
    if ((value & (0xffffffff << 28)) == 0) return 4;
    return 5;
  }

  @Override
  public void writeGroupNoTag(MessageLite value) throws IOException {
    value.writeTo(this);
  }

  @Override
  public void writeMessageNoTag(MessageLite value) throws IOException {
    writeRawVarint32(value.getSerializedSize());
    value.writeTo(this);
  }

  @Override
  public void writeBytesNoTag(ByteString value) throws IOException {
    writeRawVarint32(value.size());
    writeRawBytes(value);
  }

  @Override
  public void writeByteArrayNoTag(byte[] value) throws IOException {
    writeRawVarint32(value.length);
    writeRawBytes(value);
  }

  @Override
  public void writeByteArrayNoTag(byte[] value, int offset, int length) throws IOException {
    writeRawVarint32(length);
    writeRawBytes(value, offset, length);
  }

  @Override
  public void writeUInt32NoTag(int value) throws IOException {
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
    writeRawVarint32(encodeZigZag32(value));
  }

  @Override
  public void writeSInt64NoTag(long value) throws IOException {
    writeRawVarint64(encodeZigZag64(value));
  }

  @Override
  public void flush() throws IOException {
    if (position() == 0) {
      // Nothing to flush.
      return;
    }

    buffer.flip();
    int limit = buffer.limit();
    if (limit < SMALL_BUFFER || (bufferSize / limit) < COPY_THRESHOLD) {
      // Either not much data to onDataEncoded or the buffer is not very full ... copy the buffer.
      byte[] bytes = new byte[limit];
      buffer.get(bytes);
      buffer.position(0);
      handler.onDataEncoded(bytes, 0, limit);
    } else {
      // Use the current buffer and create a new buffer on the next onDataEncoded.
      ByteBuffer buf = buffer;
      buffer = null;
      handler.onDataEncoded(buf);
    }
  }

  @Override
  public void writeRawByte(byte value) throws IOException {
    if (capacity() == 0) {
      flush();
    }

    getOrCreateBuffer().put(value);
  }

  @Override
  public void writeRawByte(int value) throws IOException {
    writeRawByte((byte) value);
  }

  @Override
  public void writeRawBytes(ByteString value) throws IOException {
    writeRawBytes(value, 0, value.size());
  }

  @Override
  public void writeRawBytes(byte[] value) throws IOException {
    writeRawBytes(value, 0, value.length);
  }

  @Override
  public void writeRawBytes(ByteBuffer value) throws IOException {
    if (!value.hasRemaining()) {
      // Nothing to onDataEncoded.
      return;
    }

    if (value.remaining() > SMALL_BUFFER) {
      // It's a large buffer, just flush any previous data and onDataEncoded the buffer directly.
      flush();

      // Write a read-only view of this buffer. The new buffer has its own position and limit.
      handler.onDataEncoded(value.asReadOnlyBuffer());

      // Indicate that the buffer has been fully read.
      value.position(value.limit());
      return;
    }

    // Copy the value to the internal buffer, flushing as necessary.
    while (value.hasRemaining()) {
      int capacity = capacity();
      if (capacity == 0) {
        // The buffer is full, flush it and try again.
        flush();
        continue;
      }

      // Limit the value buffer by the number of bytes that can be written now.
      int bytesToWrite = Math.min(value.remaining(), capacity);
      int valueLimit = value.limit();
      value.limit(value.position() + bytesToWrite);

      getOrCreateBuffer().put(value);

      // Restore the actual limit to the input value.
      value.limit(valueLimit);
    }
  }

  @Override
  public void writeRawBytes(byte[] value, int offset, int length) throws IOException {
    if (length == 0) {
      return;
    }

    if (length > SMALL_BUFFER) {
      // It's a large buffer, just flush any previous data and onDataEncoded the buffer directly.
      flush();

      // Write a read-only view of this buffer. The new buffer has its own position and limit.
      handler.onDataEncoded(value, offset, length);
      return;
    }

    while(offset < length) {
      int capacity = capacity();
      if (capacity == 0) {
        // The buffer is full, flush it and try again.
        flush();
        continue;
      }

      // Determine the number of bytes that can be written before another flush is required.
      int remaining = length - offset;
      int bytesToWrite = Math.min(remaining, capacity);

      getOrCreateBuffer().put(value, offset, bytesToWrite);
      offset += bytesToWrite;
    }
  }

  @Override
  public void writeRawBytes(ByteString value, int offset, int length) throws IOException {
    if (length == 0) {
      return;
    }

    if (value instanceof LiteralByteString) {
      LiteralByteString lbs = (LiteralByteString) value;
      writeRawBytes(lbs.bytes, lbs.getOffsetIntoBytes(), lbs.size());
    } else if (value instanceof UnsafeByteString) {
      writeRawBytes(value.asReadOnlyByteBuffer());
    } else {
      for(ByteBuffer buf : value.asReadOnlyByteBufferList()) {
        writeRawBytes(buf);
      }
    }
  }

  @Override
  public void writeTag(int fieldNumber, int wireType) throws IOException {
    writeRawVarint32(WireFormat.makeTag(fieldNumber, wireType));
  }

  @Override
  public void writeRawVarint32(int value) throws IOException {
    while (true) {
      if ((value & ~0x7F) == 0) {
        writeRawByte(value);
        return;
      } else {
        writeRawByte((value & 0x7F) | 0x80);
        value >>>= 7;
      }
    }
  }

  @Override
  public void writeRawVarint64(long value) throws IOException {
    while (true) {
      if ((value & ~0x7FL) == 0) {
        writeRawByte((int) value);
        return;
      } else {
        writeRawByte(((int) value & 0x7F) | 0x80);
        value >>>= 7;
      }
    }
  }

  @Override
  public void writeRawLittleEndian32(int value) throws IOException {
    if(capacity() < INTEGER_BYTES) {
      flush();
    }
    getOrCreateBuffer();
    buffer.order(ByteOrder.LITTLE_ENDIAN);
    buffer.putInt(value);
    buffer.order(ByteOrder.LITTLE_ENDIAN);
  }

  @Override
  public void writeRawLittleEndian64(long value) throws IOException {
    if(capacity() < LONG_BYTES) {
      flush();
    }
    getOrCreateBuffer();
    buffer.order(ByteOrder.LITTLE_ENDIAN);
    buffer.putLong(value);
    buffer.order(ByteOrder.LITTLE_ENDIAN);
  }

  private int position() {
    return buffer == null ? 0 : buffer.position();
  }

  private int capacity() {
    return bufferSize - position();
  }

  private ByteBuffer getOrCreateBuffer() {
    if (buffer == null) {
      // Guarantee that it has a backing array.
      buffer = ByteBuffer.wrap(new byte[bufferSize]);
      buffer.order(ByteOrder.BIG_ENDIAN);
    }
    return buffer;
  }

  /**
   * Encode a ZigZag-encoded 32-bit value.  ZigZag encodes signed integers into values that can be
   * efficiently encoded with varint.  (Otherwise, negative values must be sign-extended to 64 bits
   * to be varint encoded, thus always taking 10 bytes on the wire.)
   *
   * @param n A signed 32-bit integer.
   * @return An unsigned 32-bit integer, stored in a signed int because Java has no explicit
   * unsigned support.
   */
  private static int encodeZigZag32(final int n) {
    // Note:  the right-shift must be arithmetic
    return (n << 1) ^ (n >> 31);
  }

  /**
   * Encode a ZigZag-encoded 64-bit value.  ZigZag encodes signed integers into values that can be
   * efficiently encoded with varint.  (Otherwise, negative values must be sign-extended to 64 bits
   * to be varint encoded, thus always taking 10 bytes on the wire.)
   *
   * @param n A signed 64-bit integer.
   * @return An unsigned 64-bit integer, stored in a signed int because Java has no explicit
   * unsigned support.
   */
  public static long encodeZigZag64(final long n) {
    // Note:  the right-shift must be arithmetic
    return (n << 1) ^ (n >> 63);
  }
}
