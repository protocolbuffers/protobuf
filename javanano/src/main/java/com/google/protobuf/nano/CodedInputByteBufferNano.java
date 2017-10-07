// Protocol Buffers - Google's data interchange format
// Copyright 2013 Google Inc.  All rights reserved.
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

package com.google.protobuf.nano;

import java.io.IOException;

/**
 * Reads and decodes protocol message fields.
 *
 * This class contains two kinds of methods:  methods that read specific
 * protocol message constructs and field types (e.g. {@link #readTag()} and
 * {@link #readInt32()}) and methods that read low-level values (e.g.
 * {@link #readRawVarint32()} and {@link #readRawBytes}).  If you are reading
 * encoded protocol messages, you should use the former methods, but if you are
 * reading some other format of your own design, use the latter.
 *
 * @author kenton@google.com Kenton Varda
 */
public final class CodedInputByteBufferNano {
  /**
   * Create a new CodedInputStream wrapping the given byte array.
   */
  public static CodedInputByteBufferNano newInstance(final byte[] buf) {
    return newInstance(buf, 0, buf.length);
  }

  /**
   * Create a new CodedInputStream wrapping the given byte array slice.
   */
  public static CodedInputByteBufferNano newInstance(final byte[] buf, final int off,
                                             final int len) {
    return new CodedInputByteBufferNano(buf, off, len);
  }

  // -----------------------------------------------------------------

  /**
   * Attempt to read a field tag, returning zero if we have reached EOF.
   * Protocol message parsers use this to read tags, since a protocol message
   * may legally end wherever a tag occurs, and zero is not a valid tag number.
   */
  public int readTag() throws IOException {
    if (isAtEnd()) {
      lastTag = 0;
      return 0;
    }

    lastTag = readRawVarint32();
    if (lastTag == 0) {
      // If we actually read zero, that's not a valid tag.
      throw InvalidProtocolBufferNanoException.invalidTag();
    }
    return lastTag;
  }

  /**
   * Verifies that the last call to readTag() returned the given tag value.
   * This is used to verify that a nested group ended with the correct
   * end tag.
   *
   * @throws InvalidProtocolBufferNanoException {@code value} does not match the
   *                                        last tag.
   */
  public void checkLastTagWas(final int value)
                              throws InvalidProtocolBufferNanoException {
    if (lastTag != value) {
      throw InvalidProtocolBufferNanoException.invalidEndTag();
    }
  }

  /**
   * Reads and discards a single field, given its tag value.
   *
   * @return {@code false} if the tag is an endgroup tag, in which case
   *         nothing is skipped.  Otherwise, returns {@code true}.
   */
  public boolean skipField(final int tag) throws IOException {
    switch (WireFormatNano.getTagWireType(tag)) {
      case WireFormatNano.WIRETYPE_VARINT:
        readInt32();
        return true;
      case WireFormatNano.WIRETYPE_FIXED64:
        readRawLittleEndian64();
        return true;
      case WireFormatNano.WIRETYPE_LENGTH_DELIMITED:
        skipRawBytes(readRawVarint32());
        return true;
      case WireFormatNano.WIRETYPE_START_GROUP:
        skipMessage();
        checkLastTagWas(
          WireFormatNano.makeTag(WireFormatNano.getTagFieldNumber(tag),
                             WireFormatNano.WIRETYPE_END_GROUP));
        return true;
      case WireFormatNano.WIRETYPE_END_GROUP:
        return false;
      case WireFormatNano.WIRETYPE_FIXED32:
        readRawLittleEndian32();
        return true;
      default:
        throw InvalidProtocolBufferNanoException.invalidWireType();
    }
  }

  /**
   * Reads and discards an entire message.  This will read either until EOF
   * or until an endgroup tag, whichever comes first.
   */
  public void skipMessage() throws IOException {
    while (true) {
      final int tag = readTag();
      if (tag == 0 || !skipField(tag)) {
        return;
      }
    }
  }

  // -----------------------------------------------------------------

  /** Read a {@code double} field value from the stream. */
  public double readDouble() throws IOException {
    return Double.longBitsToDouble(readRawLittleEndian64());
  }

  /** Read a {@code float} field value from the stream. */
  public float readFloat() throws IOException {
    return Float.intBitsToFloat(readRawLittleEndian32());
  }

  /** Read a {@code uint64} field value from the stream. */
  public long readUInt64() throws IOException {
    return readRawVarint64();
  }

  /** Read an {@code int64} field value from the stream. */
  public long readInt64() throws IOException {
    return readRawVarint64();
  }

  /** Read an {@code int32} field value from the stream. */
  public int readInt32() throws IOException {
    return readRawVarint32();
  }

  /** Read a {@code fixed64} field value from the stream. */
  public long readFixed64() throws IOException {
    return readRawLittleEndian64();
  }

  /** Read a {@code fixed32} field value from the stream. */
  public int readFixed32() throws IOException {
    return readRawLittleEndian32();
  }

  /** Read a {@code bool} field value from the stream. */
  public boolean readBool() throws IOException {
    return readRawVarint32() != 0;
  }

  /** Read a {@code string} field value from the stream. */
  public String readString() throws IOException {
    final int size = readRawVarint32();
    if (size <= (bufferSize - bufferPos) && size > 0) {
      // Fast path:  We already have the bytes in a contiguous buffer, so
      //   just copy directly from it.
      final String result = new String(buffer, bufferPos, size, InternalNano.UTF_8);
      bufferPos += size;
      return result;
    } else {
      // Slow path:  Build a byte array first then copy it.
      return new String(readRawBytes(size), InternalNano.UTF_8);
    }
  }

  /** Read a {@code group} field value from the stream. */
  public void readGroup(final MessageNano msg, final int fieldNumber)
      throws IOException {
    if (recursionDepth >= recursionLimit) {
      throw InvalidProtocolBufferNanoException.recursionLimitExceeded();
    }
    ++recursionDepth;
    msg.mergeFrom(this);
    checkLastTagWas(
      WireFormatNano.makeTag(fieldNumber, WireFormatNano.WIRETYPE_END_GROUP));
    --recursionDepth;
  }

  public void readMessage(final MessageNano msg)
      throws IOException {
    final int length = readRawVarint32();
    if (recursionDepth >= recursionLimit) {
      throw InvalidProtocolBufferNanoException.recursionLimitExceeded();
    }
    final int oldLimit = pushLimit(length);
    ++recursionDepth;
    msg.mergeFrom(this);
    checkLastTagWas(0);
    --recursionDepth;
    popLimit(oldLimit);
  }

  /** Read a {@code bytes} field value from the stream. */
  public byte[] readBytes() throws IOException {
    final int size = readRawVarint32();
    if (size <= (bufferSize - bufferPos) && size > 0) {
      // Fast path:  We already have the bytes in a contiguous buffer, so
      //   just copy directly from it.
      final byte[] result = new byte[size];
      System.arraycopy(buffer, bufferPos, result, 0, size);
      bufferPos += size;
      return result;
    } else if (size == 0) {
      return WireFormatNano.EMPTY_BYTES;
    } else {
      // Slow path:  Build a byte array first then copy it.
      return readRawBytes(size);
    }
  }

  /** Read a {@code uint32} field value from the stream. */
  public int readUInt32() throws IOException {
    return readRawVarint32();
  }

  /**
   * Read an enum field value from the stream.  Caller is responsible
   * for converting the numeric value to an actual enum.
   */
  public int readEnum() throws IOException {
    return readRawVarint32();
  }

  /** Read an {@code sfixed32} field value from the stream. */
  public int readSFixed32() throws IOException {
    return readRawLittleEndian32();
  }

  /** Read an {@code sfixed64} field value from the stream. */
  public long readSFixed64() throws IOException {
    return readRawLittleEndian64();
  }

  /** Read an {@code sint32} field value from the stream. */
  public int readSInt32() throws IOException {
    return decodeZigZag32(readRawVarint32());
  }

  /** Read an {@code sint64} field value from the stream. */
  public long readSInt64() throws IOException {
    return decodeZigZag64(readRawVarint64());
  }

  // =================================================================

  /**
   * Read a raw Varint from the stream.  If larger than 32 bits, discard the
   * upper bits.
   */
  public int readRawVarint32() throws IOException {
    byte tmp = readRawByte();
    if (tmp >= 0) {
      return tmp;
    }
    int result = tmp & 0x7f;
    if ((tmp = readRawByte()) >= 0) {
      result |= tmp << 7;
    } else {
      result |= (tmp & 0x7f) << 7;
      if ((tmp = readRawByte()) >= 0) {
        result |= tmp << 14;
      } else {
        result |= (tmp & 0x7f) << 14;
        if ((tmp = readRawByte()) >= 0) {
          result |= tmp << 21;
        } else {
          result |= (tmp & 0x7f) << 21;
          result |= (tmp = readRawByte()) << 28;
          if (tmp < 0) {
            // Discard upper 32 bits.
            for (int i = 0; i < 5; i++) {
              if (readRawByte() >= 0) {
                return result;
              }
            }
            throw InvalidProtocolBufferNanoException.malformedVarint();
          }
        }
      }
    }
    return result;
  }

  /** Read a raw Varint from the stream. */
  public long readRawVarint64() throws IOException {
    int shift = 0;
    long result = 0;
    while (shift < 64) {
      final byte b = readRawByte();
      result |= (long)(b & 0x7F) << shift;
      if ((b & 0x80) == 0) {
        return result;
      }
      shift += 7;
    }
    throw InvalidProtocolBufferNanoException.malformedVarint();
  }

  /** Read a 32-bit little-endian integer from the stream. */
  public int readRawLittleEndian32() throws IOException {
    final byte b1 = readRawByte();
    final byte b2 = readRawByte();
    final byte b3 = readRawByte();
    final byte b4 = readRawByte();
    return ((b1 & 0xff)      ) |
           ((b2 & 0xff) <<  8) |
           ((b3 & 0xff) << 16) |
           ((b4 & 0xff) << 24);
  }

  /** Read a 64-bit little-endian integer from the stream. */
  public long readRawLittleEndian64() throws IOException {
    final byte b1 = readRawByte();
    final byte b2 = readRawByte();
    final byte b3 = readRawByte();
    final byte b4 = readRawByte();
    final byte b5 = readRawByte();
    final byte b6 = readRawByte();
    final byte b7 = readRawByte();
    final byte b8 = readRawByte();
    return (((long)b1 & 0xff)      ) |
           (((long)b2 & 0xff) <<  8) |
           (((long)b3 & 0xff) << 16) |
           (((long)b4 & 0xff) << 24) |
           (((long)b5 & 0xff) << 32) |
           (((long)b6 & 0xff) << 40) |
           (((long)b7 & 0xff) << 48) |
           (((long)b8 & 0xff) << 56);
  }

  /**
   * Decode a ZigZag-encoded 32-bit value.  ZigZag encodes signed integers
   * into values that can be efficiently encoded with varint.  (Otherwise,
   * negative values must be sign-extended to 64 bits to be varint encoded,
   * thus always taking 10 bytes on the wire.)
   *
   * @param n An unsigned 32-bit integer, stored in a signed int because
   *          Java has no explicit unsigned support.
   * @return A signed 32-bit integer.
   */
  public static int decodeZigZag32(final int n) {
    return (n >>> 1) ^ -(n & 1);
  }

  /**
   * Decode a ZigZag-encoded 64-bit value.  ZigZag encodes signed integers
   * into values that can be efficiently encoded with varint.  (Otherwise,
   * negative values must be sign-extended to 64 bits to be varint encoded,
   * thus always taking 10 bytes on the wire.)
   *
   * @param n An unsigned 64-bit integer, stored in a signed int because
   *          Java has no explicit unsigned support.
   * @return A signed 64-bit integer.
   */
  public static long decodeZigZag64(final long n) {
    return (n >>> 1) ^ -(n & 1);
  }

  // -----------------------------------------------------------------

  private final byte[] buffer;
  private int bufferStart;
  private int bufferSize;
  private int bufferSizeAfterLimit;
  private int bufferPos;
  private int lastTag;

  /** The absolute position of the end of the current message. */
  private int currentLimit = Integer.MAX_VALUE;

  /** See setRecursionLimit() */
  private int recursionDepth;
  private int recursionLimit = DEFAULT_RECURSION_LIMIT;

  /** See setSizeLimit() */
  private int sizeLimit = DEFAULT_SIZE_LIMIT;

  private static final int DEFAULT_RECURSION_LIMIT = 64;
  private static final int DEFAULT_SIZE_LIMIT = 64 << 20;  // 64MB

  private CodedInputByteBufferNano(final byte[] buffer, final int off, final int len) {
    this.buffer = buffer;
    bufferStart = off;
    bufferSize = off + len;
    bufferPos = off;
  }

  /**
   * Set the maximum message recursion depth.  In order to prevent malicious
   * messages from causing stack overflows, {@code CodedInputStream} limits
   * how deeply messages may be nested.  The default limit is 64.
   *
   * @return the old limit.
   */
  public int setRecursionLimit(final int limit) {
    if (limit < 0) {
      throw new IllegalArgumentException(
        "Recursion limit cannot be negative: " + limit);
    }
    final int oldLimit = recursionLimit;
    recursionLimit = limit;
    return oldLimit;
  }

  /**
   * Set the maximum message size.  In order to prevent malicious
   * messages from exhausting memory or causing integer overflows,
   * {@code CodedInputStream} limits how large a message may be.
   * The default limit is 64MB.  You should set this limit as small
   * as you can without harming your app's functionality.  Note that
   * size limits only apply when reading from an {@code InputStream}, not
   * when constructed around a raw byte array.
   * <p>
   * If you want to read several messages from a single CodedInputStream, you
   * could call {@link #resetSizeCounter()} after each one to avoid hitting the
   * size limit.
   *
   * @return the old limit.
   */
  public int setSizeLimit(final int limit) {
    if (limit < 0) {
      throw new IllegalArgumentException(
        "Size limit cannot be negative: " + limit);
    }
    final int oldLimit = sizeLimit;
    sizeLimit = limit;
    return oldLimit;
  }

  /**
   * Resets the current size counter to zero (see {@link #setSizeLimit(int)}).
   */
  public void resetSizeCounter() {
  }

  /**
   * Sets {@code currentLimit} to (current position) + {@code byteLimit}.  This
   * is called when descending into a length-delimited embedded message.
   *
   * @return the old limit.
   */
  public int pushLimit(int byteLimit) throws InvalidProtocolBufferNanoException {
    if (byteLimit < 0) {
      throw InvalidProtocolBufferNanoException.negativeSize();
    }
    byteLimit += bufferPos;
    final int oldLimit = currentLimit;
    if (byteLimit > oldLimit) {
      throw InvalidProtocolBufferNanoException.truncatedMessage();
    }
    currentLimit = byteLimit;

    recomputeBufferSizeAfterLimit();

    return oldLimit;
  }

  private void recomputeBufferSizeAfterLimit() {
    bufferSize += bufferSizeAfterLimit;
    final int bufferEnd = bufferSize;
    if (bufferEnd > currentLimit) {
      // Limit is in current buffer.
      bufferSizeAfterLimit = bufferEnd - currentLimit;
      bufferSize -= bufferSizeAfterLimit;
    } else {
      bufferSizeAfterLimit = 0;
    }
  }

  /**
   * Discards the current limit, returning to the previous limit.
   *
   * @param oldLimit The old limit, as returned by {@code pushLimit}.
   */
  public void popLimit(final int oldLimit) {
    currentLimit = oldLimit;
    recomputeBufferSizeAfterLimit();
  }

  /**
   * Returns the number of bytes to be read before the current limit.
   * If no limit is set, returns -1.
   */
  public int getBytesUntilLimit() {
    if (currentLimit == Integer.MAX_VALUE) {
      return -1;
    }

    final int currentAbsolutePosition = bufferPos;
    return currentLimit - currentAbsolutePosition;
  }

  /**
   * Returns true if the stream has reached the end of the input.  This is the
   * case if either the end of the underlying input source has been reached or
   * if the stream has reached a limit created using {@link #pushLimit(int)}.
   */
  public boolean isAtEnd() {
    return bufferPos == bufferSize;
  }

  /**
   * Get current position in buffer relative to beginning offset.
   */
  public int getPosition() {
    return bufferPos - bufferStart;
  }

  /**
   * Retrieves a subset of data in the buffer. The returned array is not backed by the original
   * buffer array.
   *
   * @param offset the position (relative to the buffer start position) to start at.
   * @param length the number of bytes to retrieve.
   */
  public byte[] getData(int offset, int length) {
    if (length == 0) {
      return WireFormatNano.EMPTY_BYTES;
    }
    byte[] copy = new byte[length];
    int start = bufferStart + offset;
    System.arraycopy(buffer, start, copy, 0, length);
    return copy;
  }

  /**
   * Rewind to previous position. Cannot go forward.
   */
  public void rewindToPosition(int position) {
    if (position > bufferPos - bufferStart) {
      throw new IllegalArgumentException(
              "Position " + position + " is beyond current " + (bufferPos - bufferStart));
    }
    if (position < 0) {
      throw new IllegalArgumentException("Bad position " + position);
    }
    bufferPos = bufferStart + position;
  }

  /**
   * Read one byte from the input.
   *
   * @throws InvalidProtocolBufferNanoException The end of the stream or the current
   *                                        limit was reached.
   */
  public byte readRawByte() throws IOException {
    if (bufferPos == bufferSize) {
      throw InvalidProtocolBufferNanoException.truncatedMessage();
    }
    return buffer[bufferPos++];
  }

  /**
   * Read a fixed size of bytes from the input.
   *
   * @throws InvalidProtocolBufferNanoException The end of the stream or the current
   *                                        limit was reached.
   */
  public byte[] readRawBytes(final int size) throws IOException {
    if (size < 0) {
      throw InvalidProtocolBufferNanoException.negativeSize();
    }

    if (bufferPos + size > currentLimit) {
      // Read to the end of the stream anyway.
      skipRawBytes(currentLimit - bufferPos);
      // Then fail.
      throw InvalidProtocolBufferNanoException.truncatedMessage();
    }

    if (size <= bufferSize - bufferPos) {
      // We have all the bytes we need already.
      final byte[] bytes = new byte[size];
      System.arraycopy(buffer, bufferPos, bytes, 0, size);
      bufferPos += size;
      return bytes;
    } else {
      throw InvalidProtocolBufferNanoException.truncatedMessage();
    }
  }

  /**
   * Reads and discards {@code size} bytes.
   *
   * @throws InvalidProtocolBufferNanoException The end of the stream or the current
   *                                        limit was reached.
   */
  public void skipRawBytes(final int size) throws IOException {
    if (size < 0) {
      throw InvalidProtocolBufferNanoException.negativeSize();
    }

    if (bufferPos + size > currentLimit) {
      // Read to the end of the stream anyway.
      skipRawBytes(currentLimit - bufferPos);
      // Then fail.
      throw InvalidProtocolBufferNanoException.truncatedMessage();
    }

    if (size <= bufferSize - bufferPos) {
      // We have all the bytes we need already.
      bufferPos += size;
    } else {
      throw InvalidProtocolBufferNanoException.truncatedMessage();
    }
  }

  // Read a primitive type.
  Object readPrimitiveField(int type) throws IOException {
    switch (type) {
      case InternalNano.TYPE_DOUBLE:
          return readDouble();
      case InternalNano.TYPE_FLOAT:
          return readFloat();
      case InternalNano.TYPE_INT64:
          return readInt64();
      case InternalNano.TYPE_UINT64:
          return readUInt64();
      case InternalNano.TYPE_INT32:
          return readInt32();
      case InternalNano.TYPE_FIXED64:
          return readFixed64();
      case InternalNano.TYPE_FIXED32:
          return readFixed32();
      case InternalNano.TYPE_BOOL:
          return readBool();
      case InternalNano.TYPE_STRING:
          return readString();
      case InternalNano.TYPE_BYTES:
          return readBytes();
      case InternalNano.TYPE_UINT32:
          return readUInt32();
      case InternalNano.TYPE_ENUM:
          return readEnum();
      case InternalNano.TYPE_SFIXED32:
          return readSFixed32();
      case InternalNano.TYPE_SFIXED64:
          return readSFixed64();
      case InternalNano.TYPE_SINT32:
          return readSInt32();
      case InternalNano.TYPE_SINT64:
          return readSInt64();
      default:
          throw new IllegalArgumentException("Unknown type " + type);
    }
  }
}
