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

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
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
  public static CodedInputStream newInstance(final InputStream input) {
    return new CodedInputStream(input);
  }

  /**
   * Create a new CodedInputStream wrapping the given byte array.
   */
  public static CodedInputStream newInstance(final byte[] buf) {
    return newInstance(buf, 0, buf.length);
  }

  /**
   * Create a new CodedInputStream wrapping the given byte array slice.
   */
  public static CodedInputStream newInstance(final byte[] buf, final int off,
                                             final int len) {
    CodedInputStream result = new CodedInputStream(buf, off, len);
    try {
      // Some uses of CodedInputStream can be more efficient if they know
      // exactly how many bytes are available.  By pushing the end point of the
      // buffer as a limit, we allow them to get this information via
      // getBytesUntilLimit().  Pushing a limit that we know is at the end of
      // the stream can never hurt, since we can never past that point anyway.
      result.pushLimit(len);
    } catch (InvalidProtocolBufferException ex) {
      // The only reason pushLimit() might throw an exception here is if len
      // is negative. Normally pushLimit()'s parameter comes directly off the
      // wire, so it's important to catch exceptions in case of corrupt or
      // malicious data. However, in this case, we expect that len is not a
      // user-supplied value, so we can assume that it being negative indicates
      // a programming error. Therefore, throwing an unchecked exception is
      // appropriate.
      throw new IllegalArgumentException(ex);
    }
    return result;
  }

  /**
   * Create a new CodedInputStream wrapping the given ByteBuffer. The data
   * starting from the ByteBuffer's current position to its limit will be read.
   * The returned CodedInputStream may or may not share the underlying data
   * in the ByteBuffer, therefore the ByteBuffer cannot be changed while the
   * CodedInputStream is in use.
   * Note that the ByteBuffer's position won't be changed by this function.
   * Concurrent calls with the same ByteBuffer object are safe if no other
   * thread is trying to alter the ByteBuffer's status.
   */
  public static CodedInputStream newInstance(ByteBuffer buf) {
    if (buf.hasArray()) {
      return newInstance(buf.array(), buf.arrayOffset() + buf.position(),
          buf.remaining());
    } else {
      ByteBuffer temp = buf.duplicate();
      byte[] buffer = new byte[temp.remaining()];
      temp.get(buffer);
      return newInstance(buffer);
    }
  }

  /**
   * Create a new CodedInputStream wrapping a LiteralByteString.
   */
  static CodedInputStream newInstance(LiteralByteString byteString) {
    CodedInputStream result = new CodedInputStream(byteString);
    try {
      // Some uses of CodedInputStream can be more efficient if they know
      // exactly how many bytes are available.  By pushing the end point of the
      // buffer as a limit, we allow them to get this information via
      // getBytesUntilLimit().  Pushing a limit that we know is at the end of
      // the stream can never hurt, since we can never past that point anyway.
      result.pushLimit(byteString.size());
    } catch (InvalidProtocolBufferException ex) {
      // The only reason pushLimit() might throw an exception here is if len
      // is negative. Normally pushLimit()'s parameter comes directly off the
      // wire, so it's important to catch exceptions in case of corrupt or
      // malicious data. However, in this case, we expect that len is not a
      // user-supplied value, so we can assume that it being negative indicates
      // a programming error. Therefore, throwing an unchecked exception is
      // appropriate.
      throw new IllegalArgumentException(ex);
    }
    return result;
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
    if (WireFormat.getTagFieldNumber(lastTag) == 0) {
      // If we actually read zero (or any tag number corresponding to field
      // number zero), that's not a valid tag.
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
  public void checkLastTagWas(final int value)
                              throws InvalidProtocolBufferException {
    if (lastTag != value) {
      throw InvalidProtocolBufferException.invalidEndTag();
    }
  }

  public int getLastTag() {
    return lastTag;
  }

  /**
   * Reads and discards a single field, given its tag value.
   *
   * @return {@code false} if the tag is an endgroup tag, in which case
   *         nothing is skipped.  Otherwise, returns {@code true}.
   */
  public boolean skipField(final int tag) throws IOException {
    switch (WireFormat.getTagWireType(tag)) {
      case WireFormat.WIRETYPE_VARINT:
        skipRawVarint();
        return true;
      case WireFormat.WIRETYPE_FIXED64:
        skipRawBytes(8);
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
        skipRawBytes(4);
        return true;
      default:
        throw InvalidProtocolBufferException.invalidWireType();
    }
  }

  /**
   * Reads a single field and writes it to output in wire format,
   * given its tag value.
   *
   * @return {@code false} if the tag is an endgroup tag, in which case
   *         nothing is skipped.  Otherwise, returns {@code true}.
   */
  public boolean skipField(final int tag, final CodedOutputStream output)
      throws IOException {
    switch (WireFormat.getTagWireType(tag)) {
      case WireFormat.WIRETYPE_VARINT: {
        long value = readInt64();
        output.writeRawVarint32(tag);
        output.writeUInt64NoTag(value);
        return true;
      }
      case WireFormat.WIRETYPE_FIXED64: {
        long value = readRawLittleEndian64();
        output.writeRawVarint32(tag);
        output.writeFixed64NoTag(value);
        return true;
      }
      case WireFormat.WIRETYPE_LENGTH_DELIMITED: {
        ByteString value = readBytes();
        output.writeRawVarint32(tag);
        output.writeBytesNoTag(value);
        return true;
      }
      case WireFormat.WIRETYPE_START_GROUP: {
        output.writeRawVarint32(tag);
        skipMessage(output);
        int endtag = WireFormat.makeTag(WireFormat.getTagFieldNumber(tag),
                                        WireFormat.WIRETYPE_END_GROUP);
        checkLastTagWas(endtag);
        output.writeRawVarint32(endtag);
        return true;
      }
      case WireFormat.WIRETYPE_END_GROUP: {
        return false;
      }
      case WireFormat.WIRETYPE_FIXED32: {
        int value = readRawLittleEndian32();
        output.writeRawVarint32(tag);
        output.writeFixed32NoTag(value);
        return true;
      }
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
      final int tag = readTag();
      if (tag == 0 || !skipField(tag)) {
        return;
      }
    }
  }

  /**
   * Reads an entire message and writes it to output in wire format.
   * This will read either until EOF or until an endgroup tag,
   * whichever comes first.
   */
  public void skipMessage(CodedOutputStream output) throws IOException {
    while (true) {
      final int tag = readTag();
      if (tag == 0 || !skipField(tag, output)) {
        return;
      }
    }
  }

  /**
   * Collects the bytes skipped and returns the data in a ByteBuffer.
   */
  private class SkippedDataSink implements RefillCallback {
    private int lastPos = bufferPos;
    private ByteArrayOutputStream byteArrayStream;

    @Override
    public void onRefill() {
      if (byteArrayStream == null) {
        byteArrayStream = new ByteArrayOutputStream();
      }
      byteArrayStream.write(buffer, lastPos, bufferPos - lastPos);
      lastPos = 0;
    }

    /**
     * Gets skipped data in a ByteBuffer. This method should only be
     * called once.
     */
    ByteBuffer getSkippedData() {
      if (byteArrayStream == null) {
        return ByteBuffer.wrap(buffer, lastPos, bufferPos - lastPos);
      } else {
        byteArrayStream.write(buffer, lastPos, bufferPos);
        return ByteBuffer.wrap(byteArrayStream.toByteArray());
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
    return readRawVarint64() != 0;
  }

  /**
   * Read a {@code string} field value from the stream.
   * If the stream contains malformed UTF-8,
   * replace the offending bytes with the standard UTF-8 replacement character.
   */
  public String readString() throws IOException {
    final int size = readRawVarint32();
    if (size <= (bufferSize - bufferPos) && size > 0) {
      // Fast path:  We already have the bytes in a contiguous buffer, so
      //   just copy directly from it.
      final String result = new String(buffer, bufferPos, size, "UTF-8");
      bufferPos += size;
      return result;
    } else if (size == 0) {
      return "";
    } else {
      // Slow path:  Build a byte array first then copy it.
      return new String(readRawBytesSlowPath(size), "UTF-8");
    }
  }

  /**
   * Read a {@code string} field value from the stream.
   * If the stream contains malformed UTF-8,
   * throw exception {@link InvalidProtocolBufferException}.
   */
  public String readStringRequireUtf8() throws IOException {
    final int size = readRawVarint32();
    final byte[] bytes;
    int pos = bufferPos;
    if (size <= (bufferSize - pos) && size > 0) {
      // Fast path:  We already have the bytes in a contiguous buffer, so
      //   just copy directly from it.
      bytes = buffer;
      bufferPos = pos + size;
    } else if (size == 0) {
      return "";
    } else {
      // Slow path:  Build a byte array first then copy it.
      bytes = readRawBytesSlowPath(size);
      pos = 0;
    }
    // TODO(martinrb): We could save a pass by validating while decoding.
    if (!Utf8.isValidUtf8(bytes, pos, pos + size)) {
      throw InvalidProtocolBufferException.invalidUtf8();
    }
    return new String(bytes, pos, size, "UTF-8");
  }

  /** Read a {@code group} field value from the stream. */
  public void readGroup(final int fieldNumber,
                        final MessageLite.Builder builder,
                        final ExtensionRegistryLite extensionRegistry)
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


  /** Read a {@code group} field value from the stream. */
  public <T extends MessageLite> T readGroup(
      final int fieldNumber,
      final Parser<T> parser,
      final ExtensionRegistryLite extensionRegistry)
      throws IOException {
    if (recursionDepth >= recursionLimit) {
      throw InvalidProtocolBufferException.recursionLimitExceeded();
    }
    ++recursionDepth;
    T result = parser.parsePartialFrom(this, extensionRegistry);
    checkLastTagWas(
      WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP));
    --recursionDepth;
    return result;
  }

  /**
   * Reads a {@code group} field value from the stream and merges it into the
   * given {@link UnknownFieldSet}.
   *
   * @deprecated UnknownFieldSet.Builder now implements MessageLite.Builder, so
   *             you can just call {@link #readGroup}.
   */
  @Deprecated
  public void readUnknownGroup(final int fieldNumber,
                               final MessageLite.Builder builder)
      throws IOException {
    // We know that UnknownFieldSet will ignore any ExtensionRegistry so it
    // is safe to pass null here.  (We can't call
    // ExtensionRegistry.getEmptyRegistry() because that would make this
    // class depend on ExtensionRegistry, which is not part of the lite
    // library.)
    readGroup(fieldNumber, builder, null);
  }

  /** Read an embedded message field value from the stream. */
  public void readMessage(final MessageLite.Builder builder,
                          final ExtensionRegistryLite extensionRegistry)
      throws IOException {
    final int length = readRawVarint32();
    if (recursionDepth >= recursionLimit) {
      throw InvalidProtocolBufferException.recursionLimitExceeded();
    }
    final int oldLimit = pushLimit(length);
    ++recursionDepth;
    builder.mergeFrom(this, extensionRegistry);
    checkLastTagWas(0);
    --recursionDepth;
    popLimit(oldLimit);
  }


  /** Read an embedded message field value from the stream. */
  public <T extends MessageLite> T readMessage(
      final Parser<T> parser,
      final ExtensionRegistryLite extensionRegistry)
      throws IOException {
    int length = readRawVarint32();
    if (recursionDepth >= recursionLimit) {
      throw InvalidProtocolBufferException.recursionLimitExceeded();
    }
    final int oldLimit = pushLimit(length);
    ++recursionDepth;
    T result = parser.parsePartialFrom(this, extensionRegistry);
    checkLastTagWas(0);
    --recursionDepth;
    popLimit(oldLimit);
    return result;
  }

  /** Read a {@code bytes} field value from the stream. */
  public ByteString readBytes() throws IOException {
    final int size = readRawVarint32();
    if (size <= (bufferSize - bufferPos) && size > 0) {
      // Fast path:  We already have the bytes in a contiguous buffer, so
      //   just copy directly from it.
      final ByteString result = bufferIsImmutable && enableAliasing
          ? new BoundedByteString(buffer, bufferPos, size)
          : ByteString.copyFrom(buffer, bufferPos, size);
      bufferPos += size;
      return result;
    } else if (size == 0) {
      return ByteString.EMPTY;
    } else {
      // Slow path:  Build a byte array first then copy it.
      return new LiteralByteString(readRawBytesSlowPath(size));
    }
  }

  /** Read a {@code bytes} field value from the stream. */
  public byte[] readByteArray() throws IOException {
    final int size = readRawVarint32();
    if (size <= (bufferSize - bufferPos) && size > 0) {
      // Fast path: We already have the bytes in a contiguous buffer, so
      // just copy directly from it.
      final byte[] result =
          Arrays.copyOfRange(buffer, bufferPos, bufferPos + size);
      bufferPos += size;
      return result;
    } else {
      // Slow path: Build a byte array first then copy it.
      return readRawBytesSlowPath(size);
    }
  }

  /** Read a {@code bytes} field value from the stream. */
  public ByteBuffer readByteBuffer() throws IOException {
    final int size = readRawVarint32();
    if (size <= (bufferSize - bufferPos) && size > 0) {
      // Fast path: We already have the bytes in a contiguous buffer.
      // When aliasing is enabled, we can return a ByteBuffer pointing directly
      // into the underlying byte array without copy if the CodedInputStream is
      // constructed from a byte array. If aliasing is disabled or the input is
      // from an InputStream or ByteString, we have to make a copy of the bytes.
      ByteBuffer result = input == null && !bufferIsImmutable && enableAliasing
          ? ByteBuffer.wrap(buffer, bufferPos, size).slice()
          : ByteBuffer.wrap(Arrays.copyOfRange(
              buffer, bufferPos, bufferPos + size));
      bufferPos += size;
      return result;
    } else if (size == 0) {
      return Internal.EMPTY_BYTE_BUFFER;
    } else {
      // Slow path: Build a byte array first then copy it.
      return ByteBuffer.wrap(readRawBytesSlowPath(size));
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
    // See implementation notes for readRawVarint64
 fastpath: {
      int pos = bufferPos;

      if (bufferSize == pos) {
        break fastpath;
      }

      final byte[] buffer = this.buffer;
      int x;
      if ((x = buffer[pos++]) >= 0) {
        bufferPos = pos;
        return x;
      } else if (bufferSize - pos < 9) {
        break fastpath;
      } else if ((x ^= (buffer[pos++] << 7)) < 0L) {
        x ^= (~0L << 7);
      } else if ((x ^= (buffer[pos++] << 14)) >= 0L) {
        x ^= (~0L << 7) ^ (~0L << 14);
      } else if ((x ^= (buffer[pos++] << 21)) < 0L) {
        x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21);
      } else {
        int y = buffer[pos++];
        x ^= y << 28;
        x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28);
        if (y < 0 &&
            buffer[pos++] < 0 &&
            buffer[pos++] < 0 &&
            buffer[pos++] < 0 &&
            buffer[pos++] < 0 &&
            buffer[pos++] < 0) {
          break fastpath;  // Will throw malformedVarint()
        }
      }
      bufferPos = pos;
      return x;
    }
    return (int) readRawVarint64SlowPath();
  }

  private void skipRawVarint() throws IOException {
    if (bufferSize - bufferPos >= 10) {
      final byte[] buffer = this.buffer;
      int pos = bufferPos;
      for (int i = 0; i < 10; i++) {
        if (buffer[pos++] >= 0) {
          bufferPos = pos;
          return;
        }
      }
    }
    skipRawVarintSlowPath();
  }

  private void skipRawVarintSlowPath() throws IOException {
    for (int i = 0; i < 10; i++) {
      if (readRawByte() >= 0) {
        return;
      }
    }
    throw InvalidProtocolBufferException.malformedVarint();
  }

  /**
   * Reads a varint from the input one byte at a time, so that it does not
   * read any bytes after the end of the varint.  If you simply wrapped the
   * stream in a CodedInputStream and used {@link #readRawVarint32(InputStream)}
   * then you would probably end up reading past the end of the varint since
   * CodedInputStream buffers its input.
   */
  static int readRawVarint32(final InputStream input) throws IOException {
    final int firstByte = input.read();
    if (firstByte == -1) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
    return readRawVarint32(firstByte, input);
  }

  /**
   * Like {@link #readRawVarint32(InputStream)}, but expects that the caller
   * has already read one byte.  This allows the caller to determine if EOF
   * has been reached before attempting to read.
   */
  public static int readRawVarint32(
      final int firstByte, final InputStream input) throws IOException {
    if ((firstByte & 0x80) == 0) {
      return firstByte;
    }

    int result = firstByte & 0x7f;
    int offset = 7;
    for (; offset < 32; offset += 7) {
      final int b = input.read();
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
      final int b = input.read();
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
    // Implementation notes:
    //
    // Optimized for one-byte values, expected to be common.
    // The particular code below was selected from various candidates
    // empirically, by winning VarintBenchmark.
    //
    // Sign extension of (signed) Java bytes is usually a nuisance, but
    // we exploit it here to more easily obtain the sign of bytes read.
    // Instead of cleaning up the sign extension bits by masking eagerly,
    // we delay until we find the final (positive) byte, when we clear all
    // accumulated bits with one xor.  We depend on javac to constant fold.
 fastpath: {
      int pos = bufferPos;

      if (bufferSize == pos) {
        break fastpath;
      }

      final byte[] buffer = this.buffer;
      long x;
      int y;
      if ((y = buffer[pos++]) >= 0) {
        bufferPos = pos;
        return y;
      } else if (bufferSize - pos < 9) {
        break fastpath;
      } else if ((x = y ^ (buffer[pos++] << 7)) < 0L) {
        x ^= (~0L << 7);
      } else if ((x ^= (buffer[pos++] << 14)) >= 0L) {
        x ^= (~0L << 7) ^ (~0L << 14);
      } else if ((x ^= (buffer[pos++] << 21)) < 0L) {
        x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21);
      } else if ((x ^= ((long) buffer[pos++] << 28)) >= 0L) {
        x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28);
      } else if ((x ^= ((long) buffer[pos++] << 35)) < 0L) {
        x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28) ^ (~0L << 35);
      } else if ((x ^= ((long) buffer[pos++] << 42)) >= 0L) {
        x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28) ^ (~0L << 35) ^ (~0L << 42);
      } else if ((x ^= ((long) buffer[pos++] << 49)) < 0L) {
        x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28) ^ (~0L << 35) ^ (~0L << 42)
            ^ (~0L << 49);
      } else {
        x ^= ((long) buffer[pos++] << 56);
        x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28) ^ (~0L << 35) ^ (~0L << 42)
            ^ (~0L << 49) ^ (~0L << 56);
        if (x < 0L) {
          if (buffer[pos++] < 0L) {
            break fastpath;  // Will throw malformedVarint()
          }
        }
      }
      bufferPos = pos;
      return x;
    }
    return readRawVarint64SlowPath();
  }

  /** Variant of readRawVarint64 for when uncomfortably close to the limit. */
  /* Visible for testing */
  long readRawVarint64SlowPath() throws IOException {
    long result = 0;
    for (int shift = 0; shift < 64; shift += 7) {
      final byte b = readRawByte();
      result |= (long) (b & 0x7F) << shift;
      if ((b & 0x80) == 0) {
        return result;
      }
    }
    throw InvalidProtocolBufferException.malformedVarint();
  }

  /** Read a 32-bit little-endian integer from the stream. */
  public int readRawLittleEndian32() throws IOException {
    int pos = bufferPos;

    // hand-inlined ensureAvailable(4);
    if (bufferSize - pos < 4) {
      refillBuffer(4);
      pos = bufferPos;
    }

    final byte[] buffer = this.buffer;
    bufferPos = pos + 4;
    return (((buffer[pos]     & 0xff))       |
            ((buffer[pos + 1] & 0xff) <<  8) |
            ((buffer[pos + 2] & 0xff) << 16) |
            ((buffer[pos + 3] & 0xff) << 24));
  }

  /** Read a 64-bit little-endian integer from the stream. */
  public long readRawLittleEndian64() throws IOException {
    int pos = bufferPos;

    // hand-inlined ensureAvailable(8);
    if (bufferSize - pos < 8) {
      refillBuffer(8);
      pos = bufferPos;
    }

    final byte[] buffer = this.buffer;
    bufferPos = pos + 8;
    return ((((long) buffer[pos]     & 0xffL))       |
            (((long) buffer[pos + 1] & 0xffL) <<  8) |
            (((long) buffer[pos + 2] & 0xffL) << 16) |
            (((long) buffer[pos + 3] & 0xffL) << 24) |
            (((long) buffer[pos + 4] & 0xffL) << 32) |
            (((long) buffer[pos + 5] & 0xffL) << 40) |
            (((long) buffer[pos + 6] & 0xffL) << 48) |
            (((long) buffer[pos + 7] & 0xffL) << 56));
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
  private final boolean bufferIsImmutable;
  private int bufferSize;
  private int bufferSizeAfterLimit;
  private int bufferPos;
  private final InputStream input;
  private int lastTag;
  private boolean enableAliasing = false;

  /**
   * The total number of bytes read before the current buffer.  The total
   * bytes read up to the current position can be computed as
   * {@code totalBytesRetired + bufferPos}.  This value may be negative if
   * reading started in the middle of the current buffer (e.g. if the
   * constructor that takes a byte array and an offset was used).
   */
  private int totalBytesRetired;

  /** The absolute position of the end of the current message. */
  private int currentLimit = Integer.MAX_VALUE;

  /** See setRecursionLimit() */
  private int recursionDepth;
  private int recursionLimit = DEFAULT_RECURSION_LIMIT;

  /** See setSizeLimit() */
  private int sizeLimit = DEFAULT_SIZE_LIMIT;

  private static final int DEFAULT_RECURSION_LIMIT = 64;
  private static final int DEFAULT_SIZE_LIMIT = 64 << 20;  // 64MB
  private static final int BUFFER_SIZE = 4096;

  private CodedInputStream(final byte[] buffer, final int off, final int len) {
    this.buffer = buffer;
    bufferSize = off + len;
    bufferPos = off;
    totalBytesRetired = -off;
    input = null;
    bufferIsImmutable = false;
  }

  private CodedInputStream(final InputStream input) {
    buffer = new byte[BUFFER_SIZE];
    bufferSize = 0;
    bufferPos = 0;
    totalBytesRetired = 0;
    this.input = input;
    bufferIsImmutable = false;
  }

  private CodedInputStream(final LiteralByteString byteString) {
    buffer = byteString.bytes;
    bufferPos = byteString.getOffsetIntoBytes();
    bufferSize = bufferPos + byteString.size();
    totalBytesRetired = -bufferPos;
    input = null;
    bufferIsImmutable = true;
  }

  public void enableAliasing(boolean enabled) {
    this.enableAliasing = enabled;
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
   * when constructed around a raw byte array (nor with
   * {@link ByteString#newCodedInput}).
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
    totalBytesRetired = -bufferPos;
  }

  /**
   * Sets {@code currentLimit} to (current position) + {@code byteLimit}.  This
   * is called when descending into a length-delimited embedded message.
   *
   * <p>Note that {@code pushLimit()} does NOT affect how many bytes the
   * {@code CodedInputStream} reads from an underlying {@code InputStream} when
   * refreshing its buffer.  If you need to prevent reading past a certain
   * point in the underlying {@code InputStream} (e.g. because you expect it to
   * contain more data after the end of the message which you need to handle
   * differently) then you must place a wrapper around your {@code InputStream}
   * which limits the amount of data that can be read from it.
   *
   * @return the old limit.
   */
  public int pushLimit(int byteLimit) throws InvalidProtocolBufferException {
    if (byteLimit < 0) {
      throw InvalidProtocolBufferException.negativeSize();
    }
    byteLimit += totalBytesRetired + bufferPos;
    final int oldLimit = currentLimit;
    if (byteLimit > oldLimit) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
    currentLimit = byteLimit;

    recomputeBufferSizeAfterLimit();

    return oldLimit;
  }

  private void recomputeBufferSizeAfterLimit() {
    bufferSize += bufferSizeAfterLimit;
    final int bufferEnd = totalBytesRetired + bufferSize;
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

    final int currentAbsolutePosition = totalBytesRetired + bufferPos;
    return currentLimit - currentAbsolutePosition;
  }

  /**
   * Returns true if the stream has reached the end of the input.  This is the
   * case if either the end of the underlying input source has been reached or
   * if the stream has reached a limit created using {@link #pushLimit(int)}.
   */
  public boolean isAtEnd() throws IOException {
    return bufferPos == bufferSize && !tryRefillBuffer(1);
  }

  /**
   * The total bytes read up to the current position. Calling
   * {@link #resetSizeCounter()} resets this value to zero.
   */
  public int getTotalBytesRead() {
      return totalBytesRetired + bufferPos;
  }

  private interface RefillCallback {
    void onRefill();
  }

  private RefillCallback refillCallback = null;

  /**
   * Ensures that at least {@code n} bytes are available in the buffer, reading
   * more bytes from the input if necessary to make it so.  Caller must ensure
   * that the requested space is less than BUFFER_SIZE.
   *
   * @throws InvalidProtocolBufferException The end of the stream or the current
   *                                        limit was reached.
   */
  private void ensureAvailable(int n) throws IOException {
    if (bufferSize - bufferPos < n) {
      refillBuffer(n);
    }
  }

  /**
   * Reads more bytes from the input, making at least {@code n} bytes available
   * in the buffer.  Caller must ensure that the requested space is not yet
   * available, and that the requested space is less than BUFFER_SIZE.
   *
   * @throws InvalidProtocolBufferException The end of the stream or the current
   *                                        limit was reached.
   */
  private void refillBuffer(int n) throws IOException {
    if (!tryRefillBuffer(n)) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
  }

  /**
   * Tries to read more bytes from the input, making at least {@code n} bytes
   * available in the buffer.  Caller must ensure that the requested space is
   * not yet available, and that the requested space is less than BUFFER_SIZE.
   *
   * @return {@code true} if the bytes could be made available; {@code false}
   *         if the end of the stream or the current limit was reached.
   */
  private boolean tryRefillBuffer(int n) throws IOException {
    if (bufferPos + n <= bufferSize) {
      throw new IllegalStateException(
          "refillBuffer() called when " + n +
          " bytes were already available in buffer");
    }

    if (totalBytesRetired + bufferPos + n > currentLimit) {
      // Oops, we hit a limit.
      return false;
    }

    if (refillCallback != null) {
      refillCallback.onRefill();
    }

    if (input != null) {
      int pos = bufferPos;
      if (pos > 0) {
        if (bufferSize > pos) {
          System.arraycopy(buffer, pos, buffer, 0, bufferSize - pos);
        }
        totalBytesRetired += pos;
        bufferSize -= pos;
        bufferPos = 0;
      }

      int bytesRead = input.read(buffer, bufferSize, buffer.length - bufferSize);
      if (bytesRead == 0 || bytesRead < -1 || bytesRead > buffer.length) {
        throw new IllegalStateException(
            "InputStream#read(byte[]) returned invalid result: " + bytesRead +
            "\nThe InputStream implementation is buggy.");
      }
      if (bytesRead > 0) {
        bufferSize += bytesRead;
        // Integer-overflow-conscious check against sizeLimit
        if (totalBytesRetired + n - sizeLimit > 0) {
          throw InvalidProtocolBufferException.sizeLimitExceeded();
        }
        recomputeBufferSizeAfterLimit();
        return (bufferSize >= n) ? true : tryRefillBuffer(n);
      }
    }

    return false;
  }

  /**
   * Read one byte from the input.
   *
   * @throws InvalidProtocolBufferException The end of the stream or the current
   *                                        limit was reached.
   */
  public byte readRawByte() throws IOException {
    if (bufferPos == bufferSize) {
      refillBuffer(1);
    }
    return buffer[bufferPos++];
  }

  /**
   * Read a fixed size of bytes from the input.
   *
   * @throws InvalidProtocolBufferException The end of the stream or the current
   *                                        limit was reached.
   */
  public byte[] readRawBytes(final int size) throws IOException {
    final int pos = bufferPos;
    if (size <= (bufferSize - pos) && size > 0) {
      bufferPos = pos + size;
      return Arrays.copyOfRange(buffer, pos, pos + size);
    } else {
      return readRawBytesSlowPath(size);
    }
  }

  /**
   * Exactly like readRawBytes, but caller must have already checked the fast
   * path: (size <= (bufferSize - pos) && size > 0)
   */
  private byte[] readRawBytesSlowPath(final int size) throws IOException {
    if (size <= 0) {
      if (size == 0) {
        return Internal.EMPTY_BYTE_ARRAY;
      } else {
        throw InvalidProtocolBufferException.negativeSize();
      }
    }

    if (totalBytesRetired + bufferPos + size > currentLimit) {
      // Read to the end of the stream anyway.
      skipRawBytes(currentLimit - totalBytesRetired - bufferPos);
      // Then fail.
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    if (size < BUFFER_SIZE) {
      // Reading more bytes than are in the buffer, but not an excessive number
      // of bytes.  We can safely allocate the resulting array ahead of time.

      // First copy what we have.
      final byte[] bytes = new byte[size];
      int pos = bufferSize - bufferPos;
      System.arraycopy(buffer, bufferPos, bytes, 0, pos);
      bufferPos = bufferSize;

      // We want to refill the buffer and then copy from the buffer into our
      // byte array rather than reading directly into our byte array because
      // the input may be unbuffered.
      ensureAvailable(size - pos);
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
      final int originalBufferPos = bufferPos;
      final int originalBufferSize = bufferSize;

      // Mark the current buffer consumed.
      totalBytesRetired += bufferSize;
      bufferPos = 0;
      bufferSize = 0;

      // Read all the rest of the bytes we need.
      int sizeLeft = size - (originalBufferSize - originalBufferPos);
      final List<byte[]> chunks = new ArrayList<byte[]>();

      while (sizeLeft > 0) {
        final byte[] chunk = new byte[Math.min(sizeLeft, BUFFER_SIZE)];
        int pos = 0;
        while (pos < chunk.length) {
          final int n = (input == null) ? -1 :
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
      final byte[] bytes = new byte[size];

      // Start by copying the leftover bytes from this.buffer.
      int pos = originalBufferSize - originalBufferPos;
      System.arraycopy(buffer, originalBufferPos, bytes, 0, pos);

      // And now all the chunks.
      for (final byte[] chunk : chunks) {
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
  public void skipRawBytes(final int size) throws IOException {
    if (size <= (bufferSize - bufferPos) && size >= 0) {
      // We have all the bytes we need already.
      bufferPos += size;
    } else {
      skipRawBytesSlowPath(size);
    }
  }

  /**
   * Exactly like skipRawBytes, but caller must have already checked the fast
   * path: (size <= (bufferSize - pos) && size >= 0)
   */
  private void skipRawBytesSlowPath(final int size) throws IOException {
    if (size < 0) {
      throw InvalidProtocolBufferException.negativeSize();
    }

    if (totalBytesRetired + bufferPos + size > currentLimit) {
      // Read to the end of the stream anyway.
      skipRawBytes(currentLimit - totalBytesRetired - bufferPos);
      // Then fail.
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    // Skipping more bytes than are in the buffer.  First skip what we have.
    int pos = bufferSize - bufferPos;
    bufferPos = bufferSize;

    // Keep refilling the buffer until we get to the point we wanted to skip to.
    // This has the side effect of ensuring the limits are updated correctly.
    refillBuffer(1);
    while (size - pos > bufferSize) {
      pos += bufferSize;
      bufferPos = bufferSize;
      refillBuffer(1);
    }

    bufferPos = size - pos;
  }
}
