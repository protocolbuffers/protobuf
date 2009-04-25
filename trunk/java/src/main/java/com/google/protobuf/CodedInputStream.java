// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

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
public final class CodedInputStream {
  /**
   * Create a new CodedInputStream wrapping the given InputStream.
   */
  public static CodedInputStream newInstance(InputStream input) {
    return new CodedInputStream(input);
  }

  /**
   * Create a new CodedInputStream wrapping the given byte array.
   */
  public static CodedInputStream newInstance(byte[] buf) {
    return newInstance(buf, 0, buf.length);
  }

  /**
   * Create a new CodedInputStream wrapping the given byte array slice.
   */
  public static CodedInputStream newInstance(byte[] buf, int off, int len) {
    return new CodedInputStream(buf, off, len);
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
      throw InvalidProtocolBufferException.invalidTag();
    }
    return lastTag;
  }

  /**
   * Verifies that the last call to readTag() returned the given tag value.
   * This is used to verify that a nested group ended with the correct
   * end tag.
   *
   * @throws InvalidProtocolBufferException {@code value} does not match the
   *                                        last tag.
   */
  public void checkLastTagWas(int value) throws InvalidProtocolBufferException {
    if (lastTag != value) {
      throw InvalidProtocolBufferException.invalidEndTag();
    }
  }

  /**
   * Reads and discards a single field, given its tag value.
   *
   * @return {@code false} if the tag is an endgroup tag, in which case
   *         nothing is skipped.  Otherwise, returns {@code true}.
   */
  public boolean skipField(int tag) throws IOException {
    switch (WireFormat.getTagWireType(tag)) {
      case WireFormat.WIRETYPE_VARINT:
        readInt32();
        return true;
      case WireFormat.WIRETYPE_FIXED64:
        readRawLittleEndian64();
        return true;
      case WireFormat.WIRETYPE_LENGTH_DELIMITED:
        skipRawBytes(readRawVarint32());
        return true;
      case WireFormat.WIRETYPE_START_GROUP:
        skipMessage();
        checkLastTagWas(
          WireFormat.makeTag(WireFormat.getTagFieldNumber(tag),
                             WireFormat.WIRETYPE_END_GROUP));
        return true;
      case WireFormat.WIRETYPE_END_GROUP:
        return false;
      case WireFormat.WIRETYPE_FIXED32:
        readRawLittleEndian32();
        return true;
      default:
        throw InvalidProtocolBufferException.invalidWireType();
    }
  }

  /**
   * Reads and discards an entire message.  This will read either until EOF
   * or until an endgroup tag, whichever comes first.
   */
  public void skipMessage() throws IOException {
    while (true) {
      int tag = readTag();
      if (tag == 0 || !skipField(tag)) return;
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
    int size = readRawVarint32();
    if (size <= (bufferSize - bufferPos) && size > 0) {
      // Fast path:  We already have the bytes in a contiguous buffer, so
      //   just copy directly from it.
      String result = new String(buffer, bufferPos, size, "UTF-8");
      bufferPos += size;
      return result;
    } else {
      // Slow path:  Build a byte array first then copy it.
      return new String(readRawBytes(size), "UTF-8");
    }
  }

  /** Read a {@code group} field value from the stream. */
  public void readGroup(int fieldNumber, Message.Builder builder,
                        ExtensionRegistry extensionRegistry)
      throws IOException {
    if (recursionDepth >= recursionLimit) {
      throw InvalidProtocolBufferException.recursionLimitExceeded();
    }
    ++recursionDepth;
    builder.mergeFrom(this, extensionRegistry);
    checkLastTagWas(
      WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP));
    --recursionDepth;
  }

  /**
   * Reads a {@code group} field value from the stream and merges it into the
   * given {@link UnknownFieldSet}.
   */
  public void readUnknownGroup(int fieldNumber, UnknownFieldSet.Builder builder)
      throws IOException {
    if (recursionDepth >= recursionLimit) {
      throw InvalidProtocolBufferException.recursionLimitExceeded();
    }
    ++recursionDepth;
    builder.mergeFrom(this);
    checkLastTagWas(
      WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP));
    --recursionDepth;
  }

  /** Read an embedded message field value from the stream. */
  public void readMessage(Message.Builder builder,
                          ExtensionRegistry extensionRegistry)
      throws IOException {
    int length = readRawVarint32();
    if (recursionDepth >= recursionLimit) {
      throw InvalidProtocolBufferException.recursionLimitExceeded();
    }
    int oldLimit = pushLimit(length);
    ++recursionDepth;
    builder.mergeFrom(this, extensionRegistry);
    checkLastTagWas(0);
    --recursionDepth;
    popLimit(oldLimit);
  }

  /** Read a {@code bytes} field value from the stream. */
  public ByteString readBytes() throws IOException {
    int size = readRawVarint32();
    if (size < bufferSize - bufferPos && size > 0) {
      // Fast path:  We already have the bytes in a contiguous buffer, so
      //   just copy directly from it.
      ByteString result = ByteString.copyFrom(buffer, bufferPos, size);
      bufferPos += size;
      return result;
    } else {
      // Slow path:  Build a byte array first then copy it.
      return ByteString.copyFrom(readRawBytes(size));
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

  /**
   * Read a field of any primitive type.  Enums, groups, and embedded
   * messages are not handled by this method.
   *
   * @param type Declared type of the field.
   * @return An object representing the field's value, of the exact
   *         type which would be returned by
   *         {@link Message#getField(Descriptors.FieldDescriptor)} for
   *         this field.
   */
  public Object readPrimitiveField(
      Descriptors.FieldDescriptor.Type type) throws IOException {
    switch (type) {
      case DOUBLE  : return readDouble  ();
      case FLOAT   : return readFloat   ();
      case INT64   : return readInt64   ();
      case UINT64  : return readUInt64  ();
      case INT32   : return readInt32   ();
      case FIXED64 : return readFixed64 ();
      case FIXED32 : return readFixed32 ();
      case BOOL    : return readBool    ();
      case STRING  : return readString  ();
      case BYTES   : return readBytes   ();
      case UINT32  : return readUInt32  ();
      case SFIXED32: return readSFixed32();
      case SFIXED64: return readSFixed64();
      case SINT32  : return readSInt32  ();
      case SINT64  : return readSInt64  ();

      case GROUP:
        throw new IllegalArgumentException(
          "readPrimitiveField() cannot handle nested groups.");
      case MESSAGE:
        throw new IllegalArgumentException(
          "readPrimitiveField() cannot handle embedded messages.");
      case ENUM:
        // We don't hanlde enums because we don't know what to do if the
        // value is not recognized.
        throw new IllegalArgumentException(
          "readPrimitiveField() cannot handle enums.");
    }

    throw new RuntimeException(
      "There is no way to get here, but the compiler thinks otherwise.");
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
              if (readRawByte() >= 0) return result;
            }
            throw InvalidProtocolBufferException.malformedVarint();
          }
        }
      }
    }
    return result;
  }

  /**
   * Reads a varint from the input one byte at a time, so that it does not
   * read any bytes after the end of the varint.  If you simply wrapped the
   * stream in a CodedInputStream and used {@link #readRawVarint32(InputStream)}
   * then you would probably end up reading past the end of the varint since
   * CodedInputStream buffers its input.
   */
  static int readRawVarint32(InputStream input) throws IOException {
    int result = 0;
    int offset = 0;
    for (; offset < 32; offset += 7) {
      int b = input.read();
      if (b == -1) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      result |= (b & 0x7f) << offset;
      if ((b & 0x80) == 0) {
        return result;
      }
    }
    // Keep reading up to 64 bits.
    for (; offset < 64; offset += 7) {
      int b = input.read();
      if (b == -1) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      if ((b & 0x80) == 0) {
        return result;
      }
    }
    throw InvalidProtocolBufferException.malformedVarint();
  }

  /** Read a raw Varint from the stream. */
  public long readRawVarint64() throws IOException {
    int shift = 0;
    long result = 0;
    while (shift < 64) {
      byte b = readRawByte();
      result |= (long)(b & 0x7F) << shift;
      if ((b & 0x80) == 0) return result;
      shift += 7;
    }
    throw InvalidProtocolBufferException.malformedVarint();
  }

  /** Read a 32-bit little-endian integer from the stream. */
  public int readRawLittleEndian32() throws IOException {
    byte b1 = readRawByte();
    byte b2 = readRawByte();
    byte b3 = readRawByte();
    byte b4 = readRawByte();
    return (((int)b1 & 0xff)      ) |
           (((int)b2 & 0xff) <<  8) |
           (((int)b3 & 0xff) << 16) |
           (((int)b4 & 0xff) << 24);
  }

  /** Read a 64-bit little-endian integer from the stream. */
  public long readRawLittleEndian64() throws IOException {
    byte b1 = readRawByte();
    byte b2 = readRawByte();
    byte b3 = readRawByte();
    byte b4 = readRawByte();
    byte b5 = readRawByte();
    byte b6 = readRawByte();
    byte b7 = readRawByte();
    byte b8 = readRawByte();
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
  public static int decodeZigZag32(int n) {
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
  public static long decodeZigZag64(long n) {
    return (n >>> 1) ^ -(n & 1);
  }

  // -----------------------------------------------------------------

  private byte[] buffer;
  private int bufferSize;
  private int bufferSizeAfterLimit = 0;
  private int bufferPos;
  private InputStream input;
  private int lastTag = 0;

  /**
   * The total number of bytes read before the current buffer.  The total
   * bytes read up to the current position can be computed as
   * {@code totalBytesRetired + bufferPos}.
   */
  private int totalBytesRetired = 0;

  /** The absolute position of the end of the current message. */
  private int currentLimit = Integer.MAX_VALUE;

  /** See setRecursionLimit() */
  private int recursionDepth = 0;
  private int recursionLimit = DEFAULT_RECURSION_LIMIT;

  /** See setSizeLimit() */
  private int sizeLimit = DEFAULT_SIZE_LIMIT;

  private static final int DEFAULT_RECURSION_LIMIT = 64;
  private static final int DEFAULT_SIZE_LIMIT = 64 << 20;  // 64MB
  private static final int BUFFER_SIZE = 4096;

  private CodedInputStream(byte[] buffer, int off, int len) {
    this.buffer = buffer;
    this.bufferSize = off + len;
    this.bufferPos = off;
    this.input = null;
  }

  private CodedInputStream(InputStream input) {
    this.buffer = new byte[BUFFER_SIZE];
    this.bufferSize = 0;
    this.bufferPos = 0;
    this.input = input;
  }

  /**
   * Set the maximum message recursion depth.  In order to prevent malicious
   * messages from causing stack overflows, {@code CodedInputStream} limits
   * how deeply messages may be nested.  The default limit is 64.
   *
   * @return the old limit.
   */
  public int setRecursionLimit(int limit) {
    if (limit < 0) {
      throw new IllegalArgumentException(
        "Recursion limit cannot be negative: " + limit);
    }
    int oldLimit = recursionLimit;
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
   * when constructed around a raw byte array (nor with
   * {@link ByteString#newCodedInput}).
   * <p>
   * If you want to read several messages from a single CodedInputStream, you
   * could call {@link #resetSizeCounter()} after each one to avoid hitting the
   * size limit.
   *
   * @return the old limit.
   */
  public int setSizeLimit(int limit) {
    if (limit < 0) {
      throw new IllegalArgumentException(
        "Size limit cannot be negative: " + limit);
    }
    int oldLimit = sizeLimit;
    sizeLimit = limit;
    return oldLimit;
  }

  /**
   * Resets the current size counter to zero (see {@link #setSizeLimit(int)}).
   */
  public void resetSizeCounter() {
    totalBytesRetired = 0;
  }

  /**
   * Sets {@code currentLimit} to (current position) + {@code byteLimit}.  This
   * is called when descending into a length-delimited embedded message.
   *
   * @return the old limit.
   */
  public int pushLimit(int byteLimit) throws InvalidProtocolBufferException {
    if (byteLimit < 0) {
      throw InvalidProtocolBufferException.negativeSize();
    }
    byteLimit += totalBytesRetired + bufferPos;
    int oldLimit = currentLimit;
    if (byteLimit > oldLimit) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
    currentLimit = byteLimit;

    recomputeBufferSizeAfterLimit();

    return oldLimit;
  }

  private void recomputeBufferSizeAfterLimit() {
    bufferSize += bufferSizeAfterLimit;
    int bufferEnd = totalBytesRetired + bufferSize;
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
  public void popLimit(int oldLimit) {
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

    int currentAbsolutePosition = totalBytesRetired + bufferPos;
    return currentLimit - currentAbsolutePosition;
  }

  /**
   * Returns true if the stream has reached the end of the input.  This is the
   * case if either the end of the underlying input source has been reached or
   * if the stream has reached a limit created using {@link #pushLimit(int)}.
   */
  public boolean isAtEnd() throws IOException {
    return bufferPos == bufferSize && !refillBuffer(false);
  }

  /**
   * Called with {@code this.buffer} is empty to read more bytes from the
   * input.  If {@code mustSucceed} is true, refillBuffer() gurantees that
   * either there will be at least one byte in the buffer when it returns
   * or it will throw an exception.  If {@code mustSucceed} is false,
   * refillBuffer() returns false if no more bytes were available.
   */
  private boolean refillBuffer(boolean mustSucceed) throws IOException {
    if (bufferPos < bufferSize) {
      throw new IllegalStateException(
        "refillBuffer() called when buffer wasn't empty.");
    }

    if (totalBytesRetired + bufferSize == currentLimit) {
      // Oops, we hit a limit.
      if (mustSucceed) {
        throw InvalidProtocolBufferException.truncatedMessage();
      } else {
        return false;
      }
    }

    totalBytesRetired += bufferSize;

    bufferPos = 0;
    bufferSize = (input == null) ? -1 : input.read(buffer);
    if (bufferSize == 0 || bufferSize < -1) {
      throw new IllegalStateException(
          "InputStream#read(byte[]) returned invalid result: " + bufferSize +
          "\nThe InputStream implementation is buggy.");
    }
    if (bufferSize == -1) {
      bufferSize = 0;
      if (mustSucceed) {
        throw InvalidProtocolBufferException.truncatedMessage();
      } else {
        return false;
      }
    } else {
      recomputeBufferSizeAfterLimit();
      int totalBytesRead =
        totalBytesRetired + bufferSize + bufferSizeAfterLimit;
      if (totalBytesRead > sizeLimit || totalBytesRead < 0) {
        throw InvalidProtocolBufferException.sizeLimitExceeded();
      }
      return true;
    }
  }

  /**
   * Read one byte from the input.
   *
   * @throws InvalidProtocolBufferException The end of the stream or the current
   *                                        limit was reached.
   */
  public byte readRawByte() throws IOException {
    if (bufferPos == bufferSize) {
      refillBuffer(true);
    }
    return buffer[bufferPos++];
  }

  /**
   * Read a fixed size of bytes from the input.
   *
   * @throws InvalidProtocolBufferException The end of the stream or the current
   *                                        limit was reached.
   */
  public byte[] readRawBytes(int size) throws IOException {
    if (size < 0) {
      throw InvalidProtocolBufferException.negativeSize();
    }

    if (totalBytesRetired + bufferPos + size > currentLimit) {
      // Read to the end of the stream anyway.
      skipRawBytes(currentLimit - totalBytesRetired - bufferPos);
      // Then fail.
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    if (size <= bufferSize - bufferPos) {
      // We have all the bytes we need already.
      byte[] bytes = new byte[size];
      System.arraycopy(buffer, bufferPos, bytes, 0, size);
      bufferPos += size;
      return bytes;
    } else if (size < BUFFER_SIZE) {
      // Reading more bytes than are in the buffer, but not an excessive number
      // of bytes.  We can safely allocate the resulting array ahead of time.

      // First copy what we have.
      byte[] bytes = new byte[size];
      int pos = bufferSize - bufferPos;
      System.arraycopy(buffer, bufferPos, bytes, 0, pos);
      bufferPos = bufferSize;

      // We want to use refillBuffer() and then copy from the buffer into our
      // byte array rather than reading directly into our byte array because
      // the input may be unbuffered.
      refillBuffer(true);

      while (size - pos > bufferSize) {
        System.arraycopy(buffer, 0, bytes, pos, bufferSize);
        pos += bufferSize;
        bufferPos = bufferSize;
        refillBuffer(true);
      }

      System.arraycopy(buffer, 0, bytes, pos, size - pos);
      bufferPos = size - pos;

      return bytes;
    } else {
      // The size is very large.  For security reasons, we can't allocate the
      // entire byte array yet.  The size comes directly from the input, so a
      // maliciously-crafted message could provide a bogus very large size in
      // order to trick the app into allocating a lot of memory.  We avoid this
      // by allocating and reading only a small chunk at a time, so that the
      // malicious message must actually *be* extremely large to cause
      // problems.  Meanwhile, we limit the allowed size of a message elsewhere.

      // Remember the buffer markers since we'll have to copy the bytes out of
      // it later.
      int originalBufferPos = bufferPos;
      int originalBufferSize = bufferSize;

      // Mark the current buffer consumed.
      totalBytesRetired += bufferSize;
      bufferPos = 0;
      bufferSize = 0;

      // Read all the rest of the bytes we need.
      int sizeLeft = size - (originalBufferSize - originalBufferPos);
      List<byte[]> chunks = new ArrayList<byte[]>();

      while (sizeLeft > 0) {
        byte[] chunk = new byte[Math.min(sizeLeft, BUFFER_SIZE)];
        int pos = 0;
        while (pos < chunk.length) {
          int n = (input == null) ? -1 :
            input.read(chunk, pos, chunk.length - pos);
          if (n == -1) {
            throw InvalidProtocolBufferException.truncatedMessage();
          }
          totalBytesRetired += n;
          pos += n;
        }
        sizeLeft -= chunk.length;
        chunks.add(chunk);
      }

      // OK, got everything.  Now concatenate it all into one buffer.
      byte[] bytes = new byte[size];

      // Start by copying the leftover bytes from this.buffer.
      int pos = originalBufferSize - originalBufferPos;
      System.arraycopy(buffer, originalBufferPos, bytes, 0, pos);

      // And now all the chunks.
      for (byte[] chunk : chunks) {
        System.arraycopy(chunk, 0, bytes, pos, chunk.length);
        pos += chunk.length;
      }

      // Done.
      return bytes;
    }
  }

  /**
   * Reads and discards {@code size} bytes.
   *
   * @throws InvalidProtocolBufferException The end of the stream or the current
   *                                        limit was reached.
   */
  public void skipRawBytes(int size) throws IOException {
    if (size < 0) {
      throw InvalidProtocolBufferException.negativeSize();
    }

    if (totalBytesRetired + bufferPos + size > currentLimit) {
      // Read to the end of the stream anyway.
      skipRawBytes(currentLimit - totalBytesRetired - bufferPos);
      // Then fail.
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    if (size <= bufferSize - bufferPos) {
      // We have all the bytes we need already.
      bufferPos += size;
    } else {
      // Skipping more bytes than are in the buffer.  First skip what we have.
      int pos = bufferSize - bufferPos;
      totalBytesRetired += pos;
      bufferPos = 0;
      bufferSize = 0;

      // Then skip directly from the InputStream for the rest.
      while (pos < size) {
        int n = (input == null) ? -1 : (int) input.skip(size - pos);
        if (n <= 0) {
          throw InvalidProtocolBufferException.truncatedMessage();
        }
        pos += n;
        totalBytesRetired += n;
      }
    }
  }
}
