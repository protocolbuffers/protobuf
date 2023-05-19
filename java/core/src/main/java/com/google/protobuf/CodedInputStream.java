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

import static com.google.protobuf.Internal.EMPTY_BYTE_ARRAY;
import static com.google.protobuf.Internal.EMPTY_BYTE_BUFFER;
import static com.google.protobuf.Internal.UTF_8;
import static com.google.protobuf.Internal.checkNotNull;
import static com.google.protobuf.WireFormat.FIXED32_SIZE;
import static com.google.protobuf.WireFormat.FIXED64_SIZE;
import static com.google.protobuf.WireFormat.MAX_VARINT_SIZE;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;

/**
 * Reads and decodes protocol message fields.
 *
 * <p>This class contains two kinds of methods: methods that read specific protocol message
 * constructs and field types (e.g. {@link #readTag()} and {@link #readInt32()}) and methods that
 * read low-level values (e.g. {@link #readRawVarint32()} and {@link #readRawBytes}). If you are
 * reading encoded protocol messages, you should use the former methods, but if you are reading some
 * other format of your own design, use the latter.
 *
 * @author kenton@google.com Kenton Varda
 */
public abstract class CodedInputStream {
  private static final int DEFAULT_BUFFER_SIZE = 4096;
  // Integer.MAX_VALUE == 0x7FFFFFF == INT_MAX from limits.h
  private static final int DEFAULT_SIZE_LIMIT = Integer.MAX_VALUE;
  private static volatile int defaultRecursionLimit = 100;

  /** Visible for subclasses. See setRecursionLimit() */
  int recursionDepth;

  int recursionLimit = defaultRecursionLimit;

  /** Visible for subclasses. See setSizeLimit() */
  int sizeLimit = DEFAULT_SIZE_LIMIT;

  /** Used to adapt to the experimental {@link Reader} interface. */
  CodedInputStreamReader wrapper;

  /** Create a new CodedInputStream wrapping the given InputStream. */
  public static CodedInputStream newInstance(final InputStream input) {
    return newInstance(input, DEFAULT_BUFFER_SIZE);
  }

  /** Create a new CodedInputStream wrapping the given InputStream, with a specified buffer size. */
  public static CodedInputStream newInstance(final InputStream input, int bufferSize) {
    if (bufferSize <= 0) {
      throw new IllegalArgumentException("bufferSize must be > 0");
    }
    if (input == null) {
      // Ideally we would throw here. This is done for backward compatibility.
      return newInstance(EMPTY_BYTE_ARRAY);
    }
    return new StreamDecoder(input, bufferSize);
  }

  /** Create a new CodedInputStream wrapping the given {@code Iterable <ByteBuffer>}. */
  public static CodedInputStream newInstance(final Iterable<ByteBuffer> input) {
    if (!UnsafeDirectNioDecoder.isSupported()) {
      return newInstance(new IterableByteBufferInputStream(input));
    }
    return newInstance(input, false);
  }

  /** Create a new CodedInputStream wrapping the given {@code Iterable <ByteBuffer>}. */
  static CodedInputStream newInstance(
      final Iterable<ByteBuffer> bufs, final boolean bufferIsImmutable) {
    // flag is to check the type of input's ByteBuffers.
    // flag equals 1: all ByteBuffers have array.
    // flag equals 2: all ByteBuffers are direct ByteBuffers.
    // flag equals 3: some ByteBuffers are direct and some have array.
    // flag greater than 3: other cases.
    int flag = 0;
    // Total size of the input
    int totalSize = 0;
    for (ByteBuffer buf : bufs) {
      totalSize += buf.remaining();
      if (buf.hasArray()) {
        flag |= 1;
      } else if (buf.isDirect()) {
        flag |= 2;
      } else {
        flag |= 4;
      }
    }
    if (flag == 2) {
      return new IterableDirectByteBufferDecoder(bufs, totalSize, bufferIsImmutable);
    } else {
      // TODO(yilunchong): add another decoders to deal case 1 and 3.
      return newInstance(new IterableByteBufferInputStream(bufs));
    }
  }

  /** Create a new CodedInputStream wrapping the given byte array. */
  public static CodedInputStream newInstance(final byte[] buf) {
    return newInstance(buf, 0, buf.length);
  }

  /** Create a new CodedInputStream wrapping the given byte array slice. */
  public static CodedInputStream newInstance(final byte[] buf, final int off, final int len) {
    return newInstance(buf, off, len, /* bufferIsImmutable= */ false);
  }

  /** Create a new CodedInputStream wrapping the given byte array slice. */
  static CodedInputStream newInstance(
      final byte[] buf, final int off, final int len, final boolean bufferIsImmutable) {
    ArrayDecoder result = new ArrayDecoder(buf, off, len, bufferIsImmutable);
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
   * Create a new CodedInputStream wrapping the given ByteBuffer. The data starting from the
   * ByteBuffer's current position to its limit will be read. The returned CodedInputStream may or
   * may not share the underlying data in the ByteBuffer, therefore the ByteBuffer cannot be changed
   * while the CodedInputStream is in use. Note that the ByteBuffer's position won't be changed by
   * this function. Concurrent calls with the same ByteBuffer object are safe if no other thread is
   * trying to alter the ByteBuffer's status.
   */
  public static CodedInputStream newInstance(ByteBuffer buf) {
    return newInstance(buf, /* bufferIsImmutable= */ false);
  }

  /** Create a new CodedInputStream wrapping the given buffer. */
  static CodedInputStream newInstance(ByteBuffer buf, boolean bufferIsImmutable) {
    if (buf.hasArray()) {
      return newInstance(
          buf.array(), buf.arrayOffset() + buf.position(), buf.remaining(), bufferIsImmutable);
    }

    if (buf.isDirect() && UnsafeDirectNioDecoder.isSupported()) {
      return new UnsafeDirectNioDecoder(buf, bufferIsImmutable);
    }

    // The buffer is non-direct and does not expose the underlying array. Using the ByteBuffer API
    // to access individual bytes is very slow, so just copy the buffer to an array.
    // TODO(nathanmittler): Re-evaluate with Java 9
    byte[] buffer = new byte[buf.remaining()];
    buf.duplicate().get(buffer);
    return newInstance(buffer, 0, buffer.length, true);
  }

  public void checkRecursionLimit() throws InvalidProtocolBufferException {
    if (recursionDepth >= recursionLimit) {
      throw InvalidProtocolBufferException.recursionLimitExceeded();
    }
  }
  /** Disable construction/inheritance outside of this class. */
  private CodedInputStream() {}

  // -----------------------------------------------------------------

  /**
   * Attempt to read a field tag, returning zero if we have reached EOF. Protocol message parsers
   * use this to read tags, since a protocol message may legally end wherever a tag occurs, and zero
   * is not a valid tag number.
   */
  public abstract int readTag() throws IOException;

  /**
   * Verifies that the last call to readTag() returned the given tag value. This is used to verify
   * that a nested group ended with the correct end tag.
   *
   * @throws InvalidProtocolBufferException {@code value} does not match the last tag.
   */
  public abstract void checkLastTagWas(final int value) throws InvalidProtocolBufferException;

  public abstract int getLastTag();

  /**
   * Reads and discards a single field, given its tag value.
   *
   * @return {@code false} if the tag is an endgroup tag, in which case nothing is skipped.
   *     Otherwise, returns {@code true}.
   */
  public abstract boolean skipField(final int tag) throws IOException;

  /**
   * Reads a single field and writes it to output in wire format, given its tag value.
   *
   * @return {@code false} if the tag is an endgroup tag, in which case nothing is skipped.
   *     Otherwise, returns {@code true}.
   * @deprecated use {@code UnknownFieldSet} or {@code UnknownFieldSetLite} to skip to an output
   *     stream.
   */
  @Deprecated
  public abstract boolean skipField(final int tag, final CodedOutputStream output)
      throws IOException;

  /**
   * Reads and discards an entire message. This will read either until EOF or until an endgroup tag,
   * whichever comes first.
   */
  public abstract void skipMessage() throws IOException;

  /**
   * Reads an entire message and writes it to output in wire format. This will read either until EOF
   * or until an endgroup tag, whichever comes first.
   */
  public abstract void skipMessage(CodedOutputStream output) throws IOException;

  // -----------------------------------------------------------------

  /** Read a {@code double} field value from the stream. */
  public abstract double readDouble() throws IOException;

  /** Read a {@code float} field value from the stream. */
  public abstract float readFloat() throws IOException;

  /** Read a {@code uint64} field value from the stream. */
  public abstract long readUInt64() throws IOException;

  /** Read an {@code int64} field value from the stream. */
  public abstract long readInt64() throws IOException;

  /** Read an {@code int32} field value from the stream. */
  public abstract int readInt32() throws IOException;

  /** Read a {@code fixed64} field value from the stream. */
  public abstract long readFixed64() throws IOException;

  /** Read a {@code fixed32} field value from the stream. */
  public abstract int readFixed32() throws IOException;

  /** Read a {@code bool} field value from the stream. */
  public abstract boolean readBool() throws IOException;

  /**
   * Read a {@code string} field value from the stream. If the stream contains malformed UTF-8,
   * replace the offending bytes with the standard UTF-8 replacement character.
   */
  public abstract String readString() throws IOException;

  /**
   * Read a {@code string} field value from the stream. If the stream contains malformed UTF-8,
   * throw exception {@link InvalidProtocolBufferException}.
   */
  public abstract String readStringRequireUtf8() throws IOException;

  /** Read a {@code group} field value from the stream. */
  public abstract void readGroup(
      final int fieldNumber,
      final MessageLite.Builder builder,
      final ExtensionRegistryLite extensionRegistry)
      throws IOException;

  /** Read a {@code group} field value from the stream. */
  public abstract <T extends MessageLite> T readGroup(
      final int fieldNumber, final Parser<T> parser, final ExtensionRegistryLite extensionRegistry)
      throws IOException;

  /**
   * Reads a {@code group} field value from the stream and merges it into the given {@link
   * UnknownFieldSet}.
   *
   * @deprecated UnknownFieldSet.Builder now implements MessageLite.Builder, so you can just call
   *     {@link #readGroup}.
   */
  @Deprecated
  public abstract void readUnknownGroup(final int fieldNumber, final MessageLite.Builder builder)
      throws IOException;

  /** Read an embedded message field value from the stream. */
  public abstract void readMessage(
      final MessageLite.Builder builder, final ExtensionRegistryLite extensionRegistry)
      throws IOException;

  /** Read an embedded message field value from the stream. */
  public abstract <T extends MessageLite> T readMessage(
      final Parser<T> parser, final ExtensionRegistryLite extensionRegistry) throws IOException;

  /** Read a {@code bytes} field value from the stream. */
  public abstract ByteString readBytes() throws IOException;

  /** Read a {@code bytes} field value from the stream. */
  public abstract byte[] readByteArray() throws IOException;

  /** Read a {@code bytes} field value from the stream. */
  public abstract ByteBuffer readByteBuffer() throws IOException;

  /** Read a {@code uint32} field value from the stream. */
  public abstract int readUInt32() throws IOException;

  /**
   * Read an enum field value from the stream. Caller is responsible for converting the numeric
   * value to an actual enum.
   */
  public abstract int readEnum() throws IOException;

  /** Read an {@code sfixed32} field value from the stream. */
  public abstract int readSFixed32() throws IOException;

  /** Read an {@code sfixed64} field value from the stream. */
  public abstract long readSFixed64() throws IOException;

  /** Read an {@code sint32} field value from the stream. */
  public abstract int readSInt32() throws IOException;

  /** Read an {@code sint64} field value from the stream. */
  public abstract long readSInt64() throws IOException;

  // =================================================================

  /** Read a raw Varint from the stream. If larger than 32 bits, discard the upper bits. */
  public abstract int readRawVarint32() throws IOException;

  /** Read a raw Varint from the stream. */
  public abstract long readRawVarint64() throws IOException;

  /** Variant of readRawVarint64 for when uncomfortably close to the limit. */
  /* Visible for testing */
  abstract long readRawVarint64SlowPath() throws IOException;

  /** Read a 32-bit little-endian integer from the stream. */
  public abstract int readRawLittleEndian32() throws IOException;

  /** Read a 64-bit little-endian integer from the stream. */
  public abstract long readRawLittleEndian64() throws IOException;

  // -----------------------------------------------------------------

  /**
   * Enables {@link ByteString} aliasing of the underlying buffer, trading off on buffer pinning for
   * data copies. Only valid for buffer-backed streams.
   */
  public abstract void enableAliasing(boolean enabled);

  /**
   * Set the maximum message recursion depth. In order to prevent malicious messages from causing
   * stack overflows, {@code CodedInputStream} limits how deeply messages may be nested. The default
   * limit is 100.
   *
   * @return the old limit.
   */
  public final int setRecursionLimit(final int limit) {
    if (limit < 0) {
      throw new IllegalArgumentException("Recursion limit cannot be negative: " + limit);
    }
    final int oldLimit = recursionLimit;
    recursionLimit = limit;
    return oldLimit;
  }

  /**
   * Only valid for {@link InputStream}-backed streams.
   *
   * <p>Set the maximum message size. In order to prevent malicious messages from exhausting memory
   * or causing integer overflows, {@code CodedInputStream} limits how large a message may be. The
   * default limit is {@code Integer.MAX_VALUE}. You should set this limit as small as you can
   * without harming your app's functionality. Note that size limits only apply when reading from an
   * {@code InputStream}, not when constructed around a raw byte array.
   *
   * <p>If you want to read several messages from a single CodedInputStream, you could call {@link
   * #resetSizeCounter()} after each one to avoid hitting the size limit.
   *
   * @return the old limit.
   */
  public final int setSizeLimit(final int limit) {
    if (limit < 0) {
      throw new IllegalArgumentException("Size limit cannot be negative: " + limit);
    }
    final int oldLimit = sizeLimit;
    sizeLimit = limit;
    return oldLimit;
  }

  private boolean shouldDiscardUnknownFields = false;

  /**
   * Sets this {@code CodedInputStream} to discard unknown fields. Only applies to full runtime
   * messages; lite messages will always preserve unknowns.
   *
   * <p>Note calling this function alone will have NO immediate effect on the underlying input data.
   * The unknown fields will be discarded during parsing. This affects both Proto2 and Proto3 full
   * runtime.
   */
  final void discardUnknownFields() {
    shouldDiscardUnknownFields = true;
  }

  /**
   * Reverts the unknown fields preservation behavior for Proto2 and Proto3 full runtime to their
   * default.
   */
  final void unsetDiscardUnknownFields() {
    shouldDiscardUnknownFields = false;
  }

  /**
   * Whether unknown fields in this input stream should be discarded during parsing into full
   * runtime messages.
   */
  final boolean shouldDiscardUnknownFields() {
    return shouldDiscardUnknownFields;
  }

  /**
   * Resets the current size counter to zero (see {@link #setSizeLimit(int)}). Only valid for {@link
   * InputStream}-backed streams.
   */
  public abstract void resetSizeCounter();

  /**
   * Sets {@code currentLimit} to (current position) + {@code byteLimit}. This is called when
   * descending into a length-delimited embedded message.
   *
   * <p>Note that {@code pushLimit()} does NOT affect how many bytes the {@code CodedInputStream}
   * reads from an underlying {@code InputStream} when refreshing its buffer. If you need to prevent
   * reading past a certain point in the underlying {@code InputStream} (e.g. because you expect it
   * to contain more data after the end of the message which you need to handle differently) then
   * you must place a wrapper around your {@code InputStream} which limits the amount of data that
   * can be read from it.
   *
   * @return the old limit.
   */
  public abstract int pushLimit(int byteLimit) throws InvalidProtocolBufferException;

  /**
   * Discards the current limit, returning to the previous limit.
   *
   * @param oldLimit The old limit, as returned by {@code pushLimit}.
   */
  public abstract void popLimit(final int oldLimit);

  /**
   * Returns the number of bytes to be read before the current limit. If no limit is set, returns
   * -1.
   */
  public abstract int getBytesUntilLimit();

  /**
   * Returns true if the stream has reached the end of the input. This is the case if either the end
   * of the underlying input source has been reached or if the stream has reached a limit created
   * using {@link #pushLimit(int)}. This function may get blocked when using StreamDecoder as it
   * invokes {@link StreamDecoder#tryRefillBuffer(int)} in this function which will try to read
   * bytes from input.
   */
  public abstract boolean isAtEnd() throws IOException;

  /**
   * The total bytes read up to the current position. Calling {@link #resetSizeCounter()} resets
   * this value to zero.
   */
  public abstract int getTotalBytesRead();

  /**
   * Read one byte from the input.
   *
   * @throws InvalidProtocolBufferException The end of the stream or the current limit was reached.
   */
  public abstract byte readRawByte() throws IOException;

  /**
   * Read a fixed size of bytes from the input.
   *
   * @throws InvalidProtocolBufferException The end of the stream or the current limit was reached.
   */
  public abstract byte[] readRawBytes(final int size) throws IOException;

  /**
   * Reads and discards {@code size} bytes.
   *
   * @throws InvalidProtocolBufferException The end of the stream or the current limit was reached.
   */
  public abstract void skipRawBytes(final int size) throws IOException;

  /**
   * Decode a ZigZag-encoded 32-bit value. ZigZag encodes signed integers into values that can be
   * efficiently encoded with varint. (Otherwise, negative values must be sign-extended to 64 bits
   * to be varint encoded, thus always taking 10 bytes on the wire.)
   *
   * @param n An unsigned 32-bit integer, stored in a signed int because Java has no explicit
   *     unsigned support.
   * @return A signed 32-bit integer.
   */
  public static int decodeZigZag32(final int n) {
    return (n >>> 1) ^ -(n & 1);
  }

  /**
   * Decode a ZigZag-encoded 64-bit value. ZigZag encodes signed integers into values that can be
   * efficiently encoded with varint. (Otherwise, negative values must be sign-extended to 64 bits
   * to be varint encoded, thus always taking 10 bytes on the wire.)
   *
   * @param n An unsigned 64-bit integer, stored in a signed int because Java has no explicit
   *     unsigned support.
   * @return A signed 64-bit integer.
   */
  public static long decodeZigZag64(final long n) {
    return (n >>> 1) ^ -(n & 1);
  }

  /**
   * Like {@link #readRawVarint32(InputStream)}, but expects that the caller has already read one
   * byte. This allows the caller to determine if EOF has been reached before attempting to read.
   */
  public static int readRawVarint32(final int firstByte, final InputStream input)
      throws IOException {
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

  /**
   * Reads a varint from the input one byte at a time, so that it does not read any bytes after the
   * end of the varint. If you simply wrapped the stream in a CodedInputStream and used {@link
   * #readRawVarint32(InputStream)} then you would probably end up reading past the end of the
   * varint since CodedInputStream buffers its input.
   */
  static int readRawVarint32(final InputStream input) throws IOException {
    final int firstByte = input.read();
    if (firstByte == -1) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
    return readRawVarint32(firstByte, input);
  }

  /** A {@link CodedInputStream} implementation that uses a backing array as the input. */
  private static final class ArrayDecoder extends CodedInputStream {
    private final byte[] buffer;
    private final boolean immutable;
    private int limit;
    private int bufferSizeAfterLimit;
    private int pos;
    private int startPos;
    private int lastTag;
    private boolean enableAliasing;

    /** The absolute position of the end of the current message. */
    private int currentLimit = Integer.MAX_VALUE;

    private ArrayDecoder(final byte[] buffer, final int offset, final int len, boolean immutable) {
      this.buffer = buffer;
      limit = offset + len;
      pos = offset;
      startPos = pos;
      this.immutable = immutable;
    }

    @Override
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

    @Override
    public void checkLastTagWas(final int value) throws InvalidProtocolBufferException {
      if (lastTag != value) {
        throw InvalidProtocolBufferException.invalidEndTag();
      }
    }

    @Override
    public int getLastTag() {
      return lastTag;
    }

    @Override
    public boolean skipField(final int tag) throws IOException {
      switch (WireFormat.getTagWireType(tag)) {
        case WireFormat.WIRETYPE_VARINT:
          skipRawVarint();
          return true;
        case WireFormat.WIRETYPE_FIXED64:
          skipRawBytes(FIXED64_SIZE);
          return true;
        case WireFormat.WIRETYPE_LENGTH_DELIMITED:
          skipRawBytes(readRawVarint32());
          return true;
        case WireFormat.WIRETYPE_START_GROUP:
          skipMessage();
          checkLastTagWas(
              WireFormat.makeTag(WireFormat.getTagFieldNumber(tag), WireFormat.WIRETYPE_END_GROUP));
          return true;
        case WireFormat.WIRETYPE_END_GROUP:
          return false;
        case WireFormat.WIRETYPE_FIXED32:
          skipRawBytes(FIXED32_SIZE);
          return true;
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    }

    @Override
    public boolean skipField(final int tag, final CodedOutputStream output) throws IOException {
      switch (WireFormat.getTagWireType(tag)) {
        case WireFormat.WIRETYPE_VARINT:
          {
            long value = readInt64();
            output.writeUInt32NoTag(tag);
            output.writeUInt64NoTag(value);
            return true;
          }
        case WireFormat.WIRETYPE_FIXED64:
          {
            long value = readRawLittleEndian64();
            output.writeUInt32NoTag(tag);
            output.writeFixed64NoTag(value);
            return true;
          }
        case WireFormat.WIRETYPE_LENGTH_DELIMITED:
          {
            ByteString value = readBytes();
            output.writeUInt32NoTag(tag);
            output.writeBytesNoTag(value);
            return true;
          }
        case WireFormat.WIRETYPE_START_GROUP:
          {
            output.writeUInt32NoTag(tag);
            skipMessage(output);
            int endtag =
                WireFormat.makeTag(
                    WireFormat.getTagFieldNumber(tag), WireFormat.WIRETYPE_END_GROUP);
            checkLastTagWas(endtag);
            output.writeUInt32NoTag(endtag);
            return true;
          }
        case WireFormat.WIRETYPE_END_GROUP:
          {
            return false;
          }
        case WireFormat.WIRETYPE_FIXED32:
          {
            int value = readRawLittleEndian32();
            output.writeUInt32NoTag(tag);
            output.writeFixed32NoTag(value);
            return true;
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    }

    @Override
    public void skipMessage() throws IOException {
      while (true) {
        final int tag = readTag();
        if (tag == 0 || !skipField(tag)) {
          return;
        }
      }
    }

    @Override
    public void skipMessage(CodedOutputStream output) throws IOException {
      while (true) {
        final int tag = readTag();
        if (tag == 0 || !skipField(tag, output)) {
          return;
        }
      }
    }

    // -----------------------------------------------------------------

    @Override
    public double readDouble() throws IOException {
      return Double.longBitsToDouble(readRawLittleEndian64());
    }

    @Override
    public float readFloat() throws IOException {
      return Float.intBitsToFloat(readRawLittleEndian32());
    }

    @Override
    public long readUInt64() throws IOException {
      return readRawVarint64();
    }

    @Override
    public long readInt64() throws IOException {
      return readRawVarint64();
    }

    @Override
    public int readInt32() throws IOException {
      return readRawVarint32();
    }

    @Override
    public long readFixed64() throws IOException {
      return readRawLittleEndian64();
    }

    @Override
    public int readFixed32() throws IOException {
      return readRawLittleEndian32();
    }

    @Override
    public boolean readBool() throws IOException {
      return readRawVarint64() != 0;
    }

    @Override
    public String readString() throws IOException {
      final int size = readRawVarint32();
      if (size > 0 && size <= (limit - pos)) {
        // Fast path:  We already have the bytes in a contiguous buffer, so
        //   just copy directly from it.
        final String result = new String(buffer, pos, size, UTF_8);
        pos += size;
        return result;
      }

      if (size == 0) {
        return "";
      }
      if (size < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    @Override
    public String readStringRequireUtf8() throws IOException {
      final int size = readRawVarint32();
      if (size > 0 && size <= (limit - pos)) {
        String result = Utf8.decodeUtf8(buffer, pos, size);
        pos += size;
        return result;
      }

      if (size == 0) {
        return "";
      }
      if (size <= 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    @Override
    public void readGroup(
        final int fieldNumber,
        final MessageLite.Builder builder,
        final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      checkRecursionLimit();
      ++recursionDepth;
      builder.mergeFrom(this, extensionRegistry);
      checkLastTagWas(WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP));
      --recursionDepth;
    }

    @Override
    public <T extends MessageLite> T readGroup(
        final int fieldNumber,
        final Parser<T> parser,
        final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      checkRecursionLimit();
      ++recursionDepth;
      T result = parser.parsePartialFrom(this, extensionRegistry);
      checkLastTagWas(WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP));
      --recursionDepth;
      return result;
    }

    @Deprecated
    @Override
    public void readUnknownGroup(final int fieldNumber, final MessageLite.Builder builder)
        throws IOException {
      readGroup(fieldNumber, builder, ExtensionRegistryLite.getEmptyRegistry());
    }

    @Override
    public void readMessage(
        final MessageLite.Builder builder, final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      final int length = readRawVarint32();
      checkRecursionLimit();
      final int oldLimit = pushLimit(length);
      ++recursionDepth;
      builder.mergeFrom(this, extensionRegistry);
      checkLastTagWas(0);
      --recursionDepth;
      if (getBytesUntilLimit() != 0) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      popLimit(oldLimit);
    }

    @Override
    public <T extends MessageLite> T readMessage(
        final Parser<T> parser, final ExtensionRegistryLite extensionRegistry) throws IOException {
      int length = readRawVarint32();
      checkRecursionLimit();
      final int oldLimit = pushLimit(length);
      ++recursionDepth;
      T result = parser.parsePartialFrom(this, extensionRegistry);
      checkLastTagWas(0);
      --recursionDepth;
      if (getBytesUntilLimit() != 0) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      popLimit(oldLimit);
      return result;
    }

    @Override
    public ByteString readBytes() throws IOException {
      final int size = readRawVarint32();
      if (size > 0 && size <= (limit - pos)) {
        // Fast path:  We already have the bytes in a contiguous buffer, so
        //   just copy directly from it.
        final ByteString result =
            immutable && enableAliasing
                ? ByteString.wrap(buffer, pos, size)
                : ByteString.copyFrom(buffer, pos, size);
        pos += size;
        return result;
      }
      if (size == 0) {
        return ByteString.EMPTY;
      }
      // Slow path:  Build a byte array first then copy it.
      return ByteString.wrap(readRawBytes(size));
    }

    @Override
    public byte[] readByteArray() throws IOException {
      final int size = readRawVarint32();
      return readRawBytes(size);
    }

    @Override
    public ByteBuffer readByteBuffer() throws IOException {
      final int size = readRawVarint32();
      if (size > 0 && size <= (limit - pos)) {
        // Fast path: We already have the bytes in a contiguous buffer.
        // When aliasing is enabled, we can return a ByteBuffer pointing directly
        // into the underlying byte array without copy if the CodedInputStream is
        // constructed from a byte array. If aliasing is disabled or the input is
        // from an InputStream or ByteString, we have to make a copy of the bytes.
        ByteBuffer result =
            !immutable && enableAliasing
                ? ByteBuffer.wrap(buffer, pos, size).slice()
                : ByteBuffer.wrap(Arrays.copyOfRange(buffer, pos, pos + size));
        pos += size;
        // TODO(nathanmittler): Investigate making the ByteBuffer be made read-only
        return result;
      }

      if (size == 0) {
        return EMPTY_BYTE_BUFFER;
      }
      if (size < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    @Override
    public int readUInt32() throws IOException {
      return readRawVarint32();
    }

    @Override
    public int readEnum() throws IOException {
      return readRawVarint32();
    }

    @Override
    public int readSFixed32() throws IOException {
      return readRawLittleEndian32();
    }

    @Override
    public long readSFixed64() throws IOException {
      return readRawLittleEndian64();
    }

    @Override
    public int readSInt32() throws IOException {
      return decodeZigZag32(readRawVarint32());
    }

    @Override
    public long readSInt64() throws IOException {
      return decodeZigZag64(readRawVarint64());
    }

    // =================================================================

    @Override
    public int readRawVarint32() throws IOException {
      // See implementation notes for readRawVarint64
      fastpath:
      {
        int tempPos = pos;

        if (limit == tempPos) {
          break fastpath;
        }

        final byte[] buffer = this.buffer;
        int x;
        if ((x = buffer[tempPos++]) >= 0) {
          pos = tempPos;
          return x;
        } else if (limit - tempPos < 9) {
          break fastpath;
        } else if ((x ^= (buffer[tempPos++] << 7)) < 0) {
          x ^= (~0 << 7);
        } else if ((x ^= (buffer[tempPos++] << 14)) >= 0) {
          x ^= (~0 << 7) ^ (~0 << 14);
        } else if ((x ^= (buffer[tempPos++] << 21)) < 0) {
          x ^= (~0 << 7) ^ (~0 << 14) ^ (~0 << 21);
        } else {
          int y = buffer[tempPos++];
          x ^= y << 28;
          x ^= (~0 << 7) ^ (~0 << 14) ^ (~0 << 21) ^ (~0 << 28);
          if (y < 0
              && buffer[tempPos++] < 0
              && buffer[tempPos++] < 0
              && buffer[tempPos++] < 0
              && buffer[tempPos++] < 0
              && buffer[tempPos++] < 0) {
            break fastpath; // Will throw malformedVarint()
          }
        }
        pos = tempPos;
        return x;
      }
      return (int) readRawVarint64SlowPath();
    }

    private void skipRawVarint() throws IOException {
      if (limit - pos >= MAX_VARINT_SIZE) {
        skipRawVarintFastPath();
      } else {
        skipRawVarintSlowPath();
      }
    }

    private void skipRawVarintFastPath() throws IOException {
      for (int i = 0; i < MAX_VARINT_SIZE; i++) {
        if (buffer[pos++] >= 0) {
          return;
        }
      }
      throw InvalidProtocolBufferException.malformedVarint();
    }

    private void skipRawVarintSlowPath() throws IOException {
      for (int i = 0; i < MAX_VARINT_SIZE; i++) {
        if (readRawByte() >= 0) {
          return;
        }
      }
      throw InvalidProtocolBufferException.malformedVarint();
    }

    @Override
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
      fastpath:
      {
        int tempPos = pos;

        if (limit == tempPos) {
          break fastpath;
        }

        final byte[] buffer = this.buffer;
        long x;
        int y;
        if ((y = buffer[tempPos++]) >= 0) {
          pos = tempPos;
          return y;
        } else if (limit - tempPos < 9) {
          break fastpath;
        } else if ((y ^= (buffer[tempPos++] << 7)) < 0) {
          x = y ^ (~0 << 7);
        } else if ((y ^= (buffer[tempPos++] << 14)) >= 0) {
          x = y ^ ((~0 << 7) ^ (~0 << 14));
        } else if ((y ^= (buffer[tempPos++] << 21)) < 0) {
          x = y ^ ((~0 << 7) ^ (~0 << 14) ^ (~0 << 21));
        } else if ((x = y ^ ((long) buffer[tempPos++] << 28)) >= 0L) {
          x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28);
        } else if ((x ^= ((long) buffer[tempPos++] << 35)) < 0L) {
          x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28) ^ (~0L << 35);
        } else if ((x ^= ((long) buffer[tempPos++] << 42)) >= 0L) {
          x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28) ^ (~0L << 35) ^ (~0L << 42);
        } else if ((x ^= ((long) buffer[tempPos++] << 49)) < 0L) {
          x ^=
              (~0L << 7)
                  ^ (~0L << 14)
                  ^ (~0L << 21)
                  ^ (~0L << 28)
                  ^ (~0L << 35)
                  ^ (~0L << 42)
                  ^ (~0L << 49);
        } else {
          x ^= ((long) buffer[tempPos++] << 56);
          x ^=
              (~0L << 7)
                  ^ (~0L << 14)
                  ^ (~0L << 21)
                  ^ (~0L << 28)
                  ^ (~0L << 35)
                  ^ (~0L << 42)
                  ^ (~0L << 49)
                  ^ (~0L << 56);
          if (x < 0L) {
            if (buffer[tempPos++] < 0L) {
              break fastpath; // Will throw malformedVarint()
            }
          }
        }
        pos = tempPos;
        return x;
      }
      return readRawVarint64SlowPath();
    }

    @Override
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

    @Override
    public int readRawLittleEndian32() throws IOException {
      int tempPos = pos;

      if (limit - tempPos < FIXED32_SIZE) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }

      final byte[] buffer = this.buffer;
      pos = tempPos + FIXED32_SIZE;
      return ((buffer[tempPos] & 0xff)
          | ((buffer[tempPos + 1] & 0xff) << 8)
          | ((buffer[tempPos + 2] & 0xff) << 16)
          | ((buffer[tempPos + 3] & 0xff) << 24));
    }

    @Override
    public long readRawLittleEndian64() throws IOException {
      int tempPos = pos;

      if (limit - tempPos < FIXED64_SIZE) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }

      final byte[] buffer = this.buffer;
      pos = tempPos + FIXED64_SIZE;
      return ((buffer[tempPos] & 0xffL)
          | ((buffer[tempPos + 1] & 0xffL) << 8)
          | ((buffer[tempPos + 2] & 0xffL) << 16)
          | ((buffer[tempPos + 3] & 0xffL) << 24)
          | ((buffer[tempPos + 4] & 0xffL) << 32)
          | ((buffer[tempPos + 5] & 0xffL) << 40)
          | ((buffer[tempPos + 6] & 0xffL) << 48)
          | ((buffer[tempPos + 7] & 0xffL) << 56));
    }

    @Override
    public void enableAliasing(boolean enabled) {
      this.enableAliasing = enabled;
    }

    @Override
    public void resetSizeCounter() {
      startPos = pos;
    }

    @Override
    public int pushLimit(int byteLimit) throws InvalidProtocolBufferException {
      if (byteLimit < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      byteLimit += getTotalBytesRead();
      if (byteLimit < 0) {
        throw InvalidProtocolBufferException.parseFailure();
      }
      final int oldLimit = currentLimit;
      if (byteLimit > oldLimit) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      currentLimit = byteLimit;

      recomputeBufferSizeAfterLimit();

      return oldLimit;
    }

    private void recomputeBufferSizeAfterLimit() {
      limit += bufferSizeAfterLimit;
      final int bufferEnd = limit - startPos;
      if (bufferEnd > currentLimit) {
        // Limit is in current buffer.
        bufferSizeAfterLimit = bufferEnd - currentLimit;
        limit -= bufferSizeAfterLimit;
      } else {
        bufferSizeAfterLimit = 0;
      }
    }

    @Override
    public void popLimit(final int oldLimit) {
      currentLimit = oldLimit;
      recomputeBufferSizeAfterLimit();
    }

    @Override
    public int getBytesUntilLimit() {
      if (currentLimit == Integer.MAX_VALUE) {
        return -1;
      }

      return currentLimit - getTotalBytesRead();
    }

    @Override
    public boolean isAtEnd() throws IOException {
      return pos == limit;
    }

    @Override
    public int getTotalBytesRead() {
      return pos - startPos;
    }

    @Override
    public byte readRawByte() throws IOException {
      if (pos == limit) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      return buffer[pos++];
    }

    @Override
    public byte[] readRawBytes(final int length) throws IOException {
      if (length > 0 && length <= (limit - pos)) {
        final int tempPos = pos;
        pos += length;
        return Arrays.copyOfRange(buffer, tempPos, pos);
      }

      if (length <= 0) {
        if (length == 0) {
          return Internal.EMPTY_BYTE_ARRAY;
        } else {
          throw InvalidProtocolBufferException.negativeSize();
        }
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    @Override
    public void skipRawBytes(final int length) throws IOException {
      if (length >= 0 && length <= (limit - pos)) {
        // We have all the bytes we need already.
        pos += length;
        return;
      }

      if (length < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }
  }

  /**
   * A {@link CodedInputStream} implementation that uses a backing direct ByteBuffer as the input.
   * Requires the use of {@code sun.misc.Unsafe} to perform fast reads on the buffer.
   */
  private static final class UnsafeDirectNioDecoder extends CodedInputStream {
    /** The direct buffer that is backing this stream. */
    private final ByteBuffer buffer;

    /**
     * If {@code true}, indicates that the buffer is backing a {@link ByteString} and is therefore
     * considered to be an immutable input source.
     */
    private final boolean immutable;

    /** The unsafe address of the content of {@link #buffer}. */
    private final long address;

    /** The unsafe address of the current read limit of the buffer. */
    private long limit;

    /** The unsafe address of the current read position of the buffer. */
    private long pos;

    /** The unsafe address of the starting read position. */
    private long startPos;

    /** The amount of available data in the buffer beyond {@link #limit}. */
    private int bufferSizeAfterLimit;

    /** The last tag that was read from this stream. */
    private int lastTag;

    /**
     * If {@code true}, indicates that calls to read {@link ByteString} or {@code byte[]}
     * <strong>may</strong> return slices of the underlying buffer, rather than copies.
     */
    private boolean enableAliasing;

    /** The absolute position of the end of the current message. */
    private int currentLimit = Integer.MAX_VALUE;

    static boolean isSupported() {
      return UnsafeUtil.hasUnsafeByteBufferOperations();
    }

    private UnsafeDirectNioDecoder(ByteBuffer buffer, boolean immutable) {
      this.buffer = buffer;
      address = UnsafeUtil.addressOffset(buffer);
      limit = address + buffer.limit();
      pos = address + buffer.position();
      startPos = pos;
      this.immutable = immutable;
    }

    @Override
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

    @Override
    public void checkLastTagWas(final int value) throws InvalidProtocolBufferException {
      if (lastTag != value) {
        throw InvalidProtocolBufferException.invalidEndTag();
      }
    }

    @Override
    public int getLastTag() {
      return lastTag;
    }

    @Override
    public boolean skipField(final int tag) throws IOException {
      switch (WireFormat.getTagWireType(tag)) {
        case WireFormat.WIRETYPE_VARINT:
          skipRawVarint();
          return true;
        case WireFormat.WIRETYPE_FIXED64:
          skipRawBytes(FIXED64_SIZE);
          return true;
        case WireFormat.WIRETYPE_LENGTH_DELIMITED:
          skipRawBytes(readRawVarint32());
          return true;
        case WireFormat.WIRETYPE_START_GROUP:
          skipMessage();
          checkLastTagWas(
              WireFormat.makeTag(WireFormat.getTagFieldNumber(tag), WireFormat.WIRETYPE_END_GROUP));
          return true;
        case WireFormat.WIRETYPE_END_GROUP:
          return false;
        case WireFormat.WIRETYPE_FIXED32:
          skipRawBytes(FIXED32_SIZE);
          return true;
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    }

    @Override
    public boolean skipField(final int tag, final CodedOutputStream output) throws IOException {
      switch (WireFormat.getTagWireType(tag)) {
        case WireFormat.WIRETYPE_VARINT:
          {
            long value = readInt64();
            output.writeUInt32NoTag(tag);
            output.writeUInt64NoTag(value);
            return true;
          }
        case WireFormat.WIRETYPE_FIXED64:
          {
            long value = readRawLittleEndian64();
            output.writeUInt32NoTag(tag);
            output.writeFixed64NoTag(value);
            return true;
          }
        case WireFormat.WIRETYPE_LENGTH_DELIMITED:
          {
            ByteString value = readBytes();
            output.writeUInt32NoTag(tag);
            output.writeBytesNoTag(value);
            return true;
          }
        case WireFormat.WIRETYPE_START_GROUP:
          {
            output.writeUInt32NoTag(tag);
            skipMessage(output);
            int endtag =
                WireFormat.makeTag(
                    WireFormat.getTagFieldNumber(tag), WireFormat.WIRETYPE_END_GROUP);
            checkLastTagWas(endtag);
            output.writeUInt32NoTag(endtag);
            return true;
          }
        case WireFormat.WIRETYPE_END_GROUP:
          {
            return false;
          }
        case WireFormat.WIRETYPE_FIXED32:
          {
            int value = readRawLittleEndian32();
            output.writeUInt32NoTag(tag);
            output.writeFixed32NoTag(value);
            return true;
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    }

    @Override
    public void skipMessage() throws IOException {
      while (true) {
        final int tag = readTag();
        if (tag == 0 || !skipField(tag)) {
          return;
        }
      }
    }

    @Override
    public void skipMessage(CodedOutputStream output) throws IOException {
      while (true) {
        final int tag = readTag();
        if (tag == 0 || !skipField(tag, output)) {
          return;
        }
      }
    }

    // -----------------------------------------------------------------

    @Override
    public double readDouble() throws IOException {
      return Double.longBitsToDouble(readRawLittleEndian64());
    }

    @Override
    public float readFloat() throws IOException {
      return Float.intBitsToFloat(readRawLittleEndian32());
    }

    @Override
    public long readUInt64() throws IOException {
      return readRawVarint64();
    }

    @Override
    public long readInt64() throws IOException {
      return readRawVarint64();
    }

    @Override
    public int readInt32() throws IOException {
      return readRawVarint32();
    }

    @Override
    public long readFixed64() throws IOException {
      return readRawLittleEndian64();
    }

    @Override
    public int readFixed32() throws IOException {
      return readRawLittleEndian32();
    }

    @Override
    public boolean readBool() throws IOException {
      return readRawVarint64() != 0;
    }

    @Override
    public String readString() throws IOException {
      final int size = readRawVarint32();
      if (size > 0 && size <= remaining()) {
        // TODO(nathanmittler): Is there a way to avoid this copy?
        // TODO(anuraaga): It might be possible to share the optimized loop with
        // readStringRequireUtf8 by implementing Java replacement logic there.
        // The same as readBytes' logic
        byte[] bytes = new byte[size];
        UnsafeUtil.copyMemory(pos, bytes, 0, size);
        String result = new String(bytes, UTF_8);
        pos += size;
        return result;
      }

      if (size == 0) {
        return "";
      }
      if (size < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    @Override
    public String readStringRequireUtf8() throws IOException {
      final int size = readRawVarint32();
      if (size > 0 && size <= remaining()) {
        final int bufferPos = bufferPos(pos);
        String result = Utf8.decodeUtf8(buffer, bufferPos, size);
        pos += size;
        return result;
      }

      if (size == 0) {
        return "";
      }
      if (size <= 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    @Override
    public void readGroup(
        final int fieldNumber,
        final MessageLite.Builder builder,
        final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      checkRecursionLimit();
      ++recursionDepth;
      builder.mergeFrom(this, extensionRegistry);
      checkLastTagWas(WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP));
      --recursionDepth;
    }

    @Override
    public <T extends MessageLite> T readGroup(
        final int fieldNumber,
        final Parser<T> parser,
        final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      checkRecursionLimit();
      ++recursionDepth;
      T result = parser.parsePartialFrom(this, extensionRegistry);
      checkLastTagWas(WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP));
      --recursionDepth;
      return result;
    }

    @Deprecated
    @Override
    public void readUnknownGroup(final int fieldNumber, final MessageLite.Builder builder)
        throws IOException {
      readGroup(fieldNumber, builder, ExtensionRegistryLite.getEmptyRegistry());
    }

    @Override
    public void readMessage(
        final MessageLite.Builder builder, final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      final int length = readRawVarint32();
      checkRecursionLimit();
      final int oldLimit = pushLimit(length);
      ++recursionDepth;
      builder.mergeFrom(this, extensionRegistry);
      checkLastTagWas(0);
      --recursionDepth;
      if (getBytesUntilLimit() != 0) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      popLimit(oldLimit);
    }

    @Override
    public <T extends MessageLite> T readMessage(
        final Parser<T> parser, final ExtensionRegistryLite extensionRegistry) throws IOException {
      int length = readRawVarint32();
      checkRecursionLimit();
      final int oldLimit = pushLimit(length);
      ++recursionDepth;
      T result = parser.parsePartialFrom(this, extensionRegistry);
      checkLastTagWas(0);
      --recursionDepth;
      if (getBytesUntilLimit() != 0) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      popLimit(oldLimit);
      return result;
    }

    @Override
    public ByteString readBytes() throws IOException {
      final int size = readRawVarint32();
      if (size > 0 && size <= remaining()) {
        if (immutable && enableAliasing) {
          final ByteBuffer result = slice(pos, pos + size);
          pos += size;
          return ByteString.wrap(result);
        } else {
          // Use UnsafeUtil to copy the memory to bytes instead of using ByteBuffer ways.
          byte[] bytes = new byte[size];
          UnsafeUtil.copyMemory(pos, bytes, 0, size);
          pos += size;
          return ByteString.wrap(bytes);
        }
      }

      if (size == 0) {
        return ByteString.EMPTY;
      }
      if (size < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    @Override
    public byte[] readByteArray() throws IOException {
      return readRawBytes(readRawVarint32());
    }

    @Override
    public ByteBuffer readByteBuffer() throws IOException {
      final int size = readRawVarint32();
      if (size > 0 && size <= remaining()) {
        // "Immutable" implies that buffer is backing a ByteString.
        // Disallow slicing in this case to prevent the caller from modifying the contents
        // of the ByteString.
        if (!immutable && enableAliasing) {
          final ByteBuffer result = slice(pos, pos + size);
          pos += size;
          return result;
        } else {
          // The same as readBytes' logic
          byte[] bytes = new byte[size];
          UnsafeUtil.copyMemory(pos, bytes, 0, size);
          pos += size;
          return ByteBuffer.wrap(bytes);
        }
        // TODO(nathanmittler): Investigate making the ByteBuffer be made read-only
      }

      if (size == 0) {
        return EMPTY_BYTE_BUFFER;
      }
      if (size < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    @Override
    public int readUInt32() throws IOException {
      return readRawVarint32();
    }

    @Override
    public int readEnum() throws IOException {
      return readRawVarint32();
    }

    @Override
    public int readSFixed32() throws IOException {
      return readRawLittleEndian32();
    }

    @Override
    public long readSFixed64() throws IOException {
      return readRawLittleEndian64();
    }

    @Override
    public int readSInt32() throws IOException {
      return decodeZigZag32(readRawVarint32());
    }

    @Override
    public long readSInt64() throws IOException {
      return decodeZigZag64(readRawVarint64());
    }

    // =================================================================

    @Override
    public int readRawVarint32() throws IOException {
      // See implementation notes for readRawVarint64
      fastpath:
      {
        long tempPos = pos;

        if (limit == tempPos) {
          break fastpath;
        }

        int x;
        if ((x = UnsafeUtil.getByte(tempPos++)) >= 0) {
          pos = tempPos;
          return x;
        } else if (limit - tempPos < 9) {
          break fastpath;
        } else if ((x ^= (UnsafeUtil.getByte(tempPos++) << 7)) < 0) {
          x ^= (~0 << 7);
        } else if ((x ^= (UnsafeUtil.getByte(tempPos++) << 14)) >= 0) {
          x ^= (~0 << 7) ^ (~0 << 14);
        } else if ((x ^= (UnsafeUtil.getByte(tempPos++) << 21)) < 0) {
          x ^= (~0 << 7) ^ (~0 << 14) ^ (~0 << 21);
        } else {
          int y = UnsafeUtil.getByte(tempPos++);
          x ^= y << 28;
          x ^= (~0 << 7) ^ (~0 << 14) ^ (~0 << 21) ^ (~0 << 28);
          if (y < 0
              && UnsafeUtil.getByte(tempPos++) < 0
              && UnsafeUtil.getByte(tempPos++) < 0
              && UnsafeUtil.getByte(tempPos++) < 0
              && UnsafeUtil.getByte(tempPos++) < 0
              && UnsafeUtil.getByte(tempPos++) < 0) {
            break fastpath; // Will throw malformedVarint()
          }
        }
        pos = tempPos;
        return x;
      }
      return (int) readRawVarint64SlowPath();
    }

    private void skipRawVarint() throws IOException {
      if (remaining() >= MAX_VARINT_SIZE) {
        skipRawVarintFastPath();
      } else {
        skipRawVarintSlowPath();
      }
    }

    private void skipRawVarintFastPath() throws IOException {
      for (int i = 0; i < MAX_VARINT_SIZE; i++) {
        if (UnsafeUtil.getByte(pos++) >= 0) {
          return;
        }
      }
      throw InvalidProtocolBufferException.malformedVarint();
    }

    private void skipRawVarintSlowPath() throws IOException {
      for (int i = 0; i < MAX_VARINT_SIZE; i++) {
        if (readRawByte() >= 0) {
          return;
        }
      }
      throw InvalidProtocolBufferException.malformedVarint();
    }

    @Override
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
      fastpath:
      {
        long tempPos = pos;

        if (limit == tempPos) {
          break fastpath;
        }

        long x;
        int y;
        if ((y = UnsafeUtil.getByte(tempPos++)) >= 0) {
          pos = tempPos;
          return y;
        } else if (limit - tempPos < 9) {
          break fastpath;
        } else if ((y ^= (UnsafeUtil.getByte(tempPos++) << 7)) < 0) {
          x = y ^ (~0 << 7);
        } else if ((y ^= (UnsafeUtil.getByte(tempPos++) << 14)) >= 0) {
          x = y ^ ((~0 << 7) ^ (~0 << 14));
        } else if ((y ^= (UnsafeUtil.getByte(tempPos++) << 21)) < 0) {
          x = y ^ ((~0 << 7) ^ (~0 << 14) ^ (~0 << 21));
        } else if ((x = y ^ ((long) UnsafeUtil.getByte(tempPos++) << 28)) >= 0L) {
          x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28);
        } else if ((x ^= ((long) UnsafeUtil.getByte(tempPos++) << 35)) < 0L) {
          x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28) ^ (~0L << 35);
        } else if ((x ^= ((long) UnsafeUtil.getByte(tempPos++) << 42)) >= 0L) {
          x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28) ^ (~0L << 35) ^ (~0L << 42);
        } else if ((x ^= ((long) UnsafeUtil.getByte(tempPos++) << 49)) < 0L) {
          x ^=
              (~0L << 7)
                  ^ (~0L << 14)
                  ^ (~0L << 21)
                  ^ (~0L << 28)
                  ^ (~0L << 35)
                  ^ (~0L << 42)
                  ^ (~0L << 49);
        } else {
          x ^= ((long) UnsafeUtil.getByte(tempPos++) << 56);
          x ^=
              (~0L << 7)
                  ^ (~0L << 14)
                  ^ (~0L << 21)
                  ^ (~0L << 28)
                  ^ (~0L << 35)
                  ^ (~0L << 42)
                  ^ (~0L << 49)
                  ^ (~0L << 56);
          if (x < 0L) {
            if (UnsafeUtil.getByte(tempPos++) < 0L) {
              break fastpath; // Will throw malformedVarint()
            }
          }
        }
        pos = tempPos;
        return x;
      }
      return readRawVarint64SlowPath();
    }

    @Override
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

    @Override
    public int readRawLittleEndian32() throws IOException {
      long tempPos = pos;

      if (limit - tempPos < FIXED32_SIZE) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }

      pos = tempPos + FIXED32_SIZE;
      return ((UnsafeUtil.getByte(tempPos) & 0xff)
          | ((UnsafeUtil.getByte(tempPos + 1) & 0xff) << 8)
          | ((UnsafeUtil.getByte(tempPos + 2) & 0xff) << 16)
          | ((UnsafeUtil.getByte(tempPos + 3) & 0xff) << 24));
    }

    @Override
    public long readRawLittleEndian64() throws IOException {
      long tempPos = pos;

      if (limit - tempPos < FIXED64_SIZE) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }

      pos = tempPos + FIXED64_SIZE;
      return ((UnsafeUtil.getByte(tempPos) & 0xffL)
          | ((UnsafeUtil.getByte(tempPos + 1) & 0xffL) << 8)
          | ((UnsafeUtil.getByte(tempPos + 2) & 0xffL) << 16)
          | ((UnsafeUtil.getByte(tempPos + 3) & 0xffL) << 24)
          | ((UnsafeUtil.getByte(tempPos + 4) & 0xffL) << 32)
          | ((UnsafeUtil.getByte(tempPos + 5) & 0xffL) << 40)
          | ((UnsafeUtil.getByte(tempPos + 6) & 0xffL) << 48)
          | ((UnsafeUtil.getByte(tempPos + 7) & 0xffL) << 56));
    }

    @Override
    public void enableAliasing(boolean enabled) {
      this.enableAliasing = enabled;
    }

    @Override
    public void resetSizeCounter() {
      startPos = pos;
    }

    @Override
    public int pushLimit(int byteLimit) throws InvalidProtocolBufferException {
      if (byteLimit < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      byteLimit += getTotalBytesRead();
      final int oldLimit = currentLimit;
      if (byteLimit > oldLimit) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      currentLimit = byteLimit;

      recomputeBufferSizeAfterLimit();

      return oldLimit;
    }

    @Override
    public void popLimit(final int oldLimit) {
      currentLimit = oldLimit;
      recomputeBufferSizeAfterLimit();
    }

    @Override
    public int getBytesUntilLimit() {
      if (currentLimit == Integer.MAX_VALUE) {
        return -1;
      }

      return currentLimit - getTotalBytesRead();
    }

    @Override
    public boolean isAtEnd() throws IOException {
      return pos == limit;
    }

    @Override
    public int getTotalBytesRead() {
      return (int) (pos - startPos);
    }

    @Override
    public byte readRawByte() throws IOException {
      if (pos == limit) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      return UnsafeUtil.getByte(pos++);
    }

    @Override
    public byte[] readRawBytes(final int length) throws IOException {
      if (length >= 0 && length <= remaining()) {
        byte[] bytes = new byte[length];
        slice(pos, pos + length).get(bytes);
        pos += length;
        return bytes;
      }

      if (length <= 0) {
        if (length == 0) {
          return EMPTY_BYTE_ARRAY;
        } else {
          throw InvalidProtocolBufferException.negativeSize();
        }
      }

      throw InvalidProtocolBufferException.truncatedMessage();
    }

    @Override
    public void skipRawBytes(final int length) throws IOException {
      if (length >= 0 && length <= remaining()) {
        // We have all the bytes we need already.
        pos += length;
        return;
      }

      if (length < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    private void recomputeBufferSizeAfterLimit() {
      limit += bufferSizeAfterLimit;
      final int bufferEnd = (int) (limit - startPos);
      if (bufferEnd > currentLimit) {
        // Limit is in current buffer.
        bufferSizeAfterLimit = bufferEnd - currentLimit;
        limit -= bufferSizeAfterLimit;
      } else {
        bufferSizeAfterLimit = 0;
      }
    }

    private int remaining() {
      return (int) (limit - pos);
    }

    private int bufferPos(long pos) {
      return (int) (pos - address);
    }

    private ByteBuffer slice(long begin, long end) throws IOException {
      int prevPos = buffer.position();
      int prevLimit = buffer.limit();
      // View ByteBuffer as Buffer to avoid cross-Java version issues.
      // See https://issues.apache.org/jira/browse/MRESOLVER-85
      Buffer asBuffer = buffer;
      try {
        asBuffer.position(bufferPos(begin));
        asBuffer.limit(bufferPos(end));
        return buffer.slice();
      } catch (IllegalArgumentException e) {
        InvalidProtocolBufferException ex = InvalidProtocolBufferException.truncatedMessage();
        ex.initCause(e);
        throw ex;
      } finally {
        asBuffer.position(prevPos);
        asBuffer.limit(prevLimit);
      }
    }
  }

  /**
   * Implementation of {@link CodedInputStream} that uses an {@link InputStream} as the data source.
   */
  private static final class StreamDecoder extends CodedInputStream {
    private final InputStream input;
    private final byte[] buffer;
    /** bufferSize represents how many bytes are currently filled in the buffer */
    private int bufferSize;

    private int bufferSizeAfterLimit;
    private int pos;
    private int lastTag;

    /**
     * The total number of bytes read before the current buffer. The total bytes read up to the
     * current position can be computed as {@code totalBytesRetired + pos}. This value may be
     * negative if reading started in the middle of the current buffer (e.g. if the constructor that
     * takes a byte array and an offset was used).
     */
    private int totalBytesRetired;

    /** The absolute position of the end of the current message. */
    private int currentLimit = Integer.MAX_VALUE;

    private StreamDecoder(final InputStream input, int bufferSize) {
      checkNotNull(input, "input");
      this.input = input;
      this.buffer = new byte[bufferSize];
      this.bufferSize = 0;
      pos = 0;
      totalBytesRetired = 0;
    }

    /*
     * The following wrapper methods exist so that InvalidProtocolBufferExceptions thrown by the
     * InputStream can be differentiated from ones thrown by CodedInputStream itself. Each call to
     * an InputStream method that can throw IOException must be wrapped like this. We do this
     * because we sometimes need to modify IPBE instances after they are thrown far away from where
     * they are thrown (ex. to add unfinished messages) and we use this signal elsewhere in the
     * exception catch chain to know when to perform these operations directly or to wrap the
     * exception in their own IPBE so the extra information can be communicated without trampling
     * downstream information.
     */
    private static int read(InputStream input, byte[] data, int offset, int length)
        throws IOException {
      try {
        return input.read(data, offset, length);
      } catch (InvalidProtocolBufferException e) {
        e.setThrownFromInputStream();
        throw e;
      }
    }

    private static long skip(InputStream input, long length) throws IOException {
      try {
        return input.skip(length);
      } catch (InvalidProtocolBufferException e) {
        e.setThrownFromInputStream();
        throw e;
      }
    }

    private static int available(InputStream input) throws IOException {
      try {
        return input.available();
      } catch (InvalidProtocolBufferException e) {
        e.setThrownFromInputStream();
        throw e;
      }
    }

    @Override
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

    @Override
    public void checkLastTagWas(final int value) throws InvalidProtocolBufferException {
      if (lastTag != value) {
        throw InvalidProtocolBufferException.invalidEndTag();
      }
    }

    @Override
    public int getLastTag() {
      return lastTag;
    }

    @Override
    public boolean skipField(final int tag) throws IOException {
      switch (WireFormat.getTagWireType(tag)) {
        case WireFormat.WIRETYPE_VARINT:
          skipRawVarint();
          return true;
        case WireFormat.WIRETYPE_FIXED64:
          skipRawBytes(FIXED64_SIZE);
          return true;
        case WireFormat.WIRETYPE_LENGTH_DELIMITED:
          skipRawBytes(readRawVarint32());
          return true;
        case WireFormat.WIRETYPE_START_GROUP:
          skipMessage();
          checkLastTagWas(
              WireFormat.makeTag(WireFormat.getTagFieldNumber(tag), WireFormat.WIRETYPE_END_GROUP));
          return true;
        case WireFormat.WIRETYPE_END_GROUP:
          return false;
        case WireFormat.WIRETYPE_FIXED32:
          skipRawBytes(FIXED32_SIZE);
          return true;
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    }

    @Override
    public boolean skipField(final int tag, final CodedOutputStream output) throws IOException {
      switch (WireFormat.getTagWireType(tag)) {
        case WireFormat.WIRETYPE_VARINT:
          {
            long value = readInt64();
            output.writeUInt32NoTag(tag);
            output.writeUInt64NoTag(value);
            return true;
          }
        case WireFormat.WIRETYPE_FIXED64:
          {
            long value = readRawLittleEndian64();
            output.writeUInt32NoTag(tag);
            output.writeFixed64NoTag(value);
            return true;
          }
        case WireFormat.WIRETYPE_LENGTH_DELIMITED:
          {
            ByteString value = readBytes();
            output.writeUInt32NoTag(tag);
            output.writeBytesNoTag(value);
            return true;
          }
        case WireFormat.WIRETYPE_START_GROUP:
          {
            output.writeUInt32NoTag(tag);
            skipMessage(output);
            int endtag =
                WireFormat.makeTag(
                    WireFormat.getTagFieldNumber(tag), WireFormat.WIRETYPE_END_GROUP);
            checkLastTagWas(endtag);
            output.writeUInt32NoTag(endtag);
            return true;
          }
        case WireFormat.WIRETYPE_END_GROUP:
          {
            return false;
          }
        case WireFormat.WIRETYPE_FIXED32:
          {
            int value = readRawLittleEndian32();
            output.writeUInt32NoTag(tag);
            output.writeFixed32NoTag(value);
            return true;
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    }

    @Override
    public void skipMessage() throws IOException {
      while (true) {
        final int tag = readTag();
        if (tag == 0 || !skipField(tag)) {
          return;
        }
      }
    }

    @Override
    public void skipMessage(CodedOutputStream output) throws IOException {
      while (true) {
        final int tag = readTag();
        if (tag == 0 || !skipField(tag, output)) {
          return;
        }
      }
    }

    /** Collects the bytes skipped and returns the data in a ByteBuffer. */
    private class SkippedDataSink implements RefillCallback {
      private int lastPos = pos;
      private ByteArrayOutputStream byteArrayStream;

      @Override
      public void onRefill() {
        if (byteArrayStream == null) {
          byteArrayStream = new ByteArrayOutputStream();
        }
        byteArrayStream.write(buffer, lastPos, pos - lastPos);
        lastPos = 0;
      }

      /** Gets skipped data in a ByteBuffer. This method should only be called once. */
      ByteBuffer getSkippedData() {
        if (byteArrayStream == null) {
          return ByteBuffer.wrap(buffer, lastPos, pos - lastPos);
        } else {
          byteArrayStream.write(buffer, lastPos, pos);
          return ByteBuffer.wrap(byteArrayStream.toByteArray());
        }
      }
    }

    // -----------------------------------------------------------------

    @Override
    public double readDouble() throws IOException {
      return Double.longBitsToDouble(readRawLittleEndian64());
    }

    @Override
    public float readFloat() throws IOException {
      return Float.intBitsToFloat(readRawLittleEndian32());
    }

    @Override
    public long readUInt64() throws IOException {
      return readRawVarint64();
    }

    @Override
    public long readInt64() throws IOException {
      return readRawVarint64();
    }

    @Override
    public int readInt32() throws IOException {
      return readRawVarint32();
    }

    @Override
    public long readFixed64() throws IOException {
      return readRawLittleEndian64();
    }

    @Override
    public int readFixed32() throws IOException {
      return readRawLittleEndian32();
    }

    @Override
    public boolean readBool() throws IOException {
      return readRawVarint64() != 0;
    }

    @Override
    public String readString() throws IOException {
      final int size = readRawVarint32();
      if (size > 0 && size <= (bufferSize - pos)) {
        // Fast path:  We already have the bytes in a contiguous buffer, so
        //   just copy directly from it.
        final String result = new String(buffer, pos, size, UTF_8);
        pos += size;
        return result;
      }
      if (size == 0) {
        return "";
      }
      if (size <= bufferSize) {
        refillBuffer(size);
        String result = new String(buffer, pos, size, UTF_8);
        pos += size;
        return result;
      }
      // Slow path:  Build a byte array first then copy it.
      return new String(readRawBytesSlowPath(size, /* ensureNoLeakedReferences= */ false), UTF_8);
    }

    @Override
    public String readStringRequireUtf8() throws IOException {
      final int size = readRawVarint32();
      final byte[] bytes;
      final int oldPos = pos;
      final int tempPos;
      if (size <= (bufferSize - oldPos) && size > 0) {
        // Fast path:  We already have the bytes in a contiguous buffer, so
        //   just copy directly from it.
        bytes = buffer;
        pos = oldPos + size;
        tempPos = oldPos;
      } else if (size == 0) {
        return "";
      } else if (size <= bufferSize) {
        refillBuffer(size);
        bytes = buffer;
        tempPos = 0;
        pos = tempPos + size;
      } else {
        // Slow path:  Build a byte array first then copy it.
        bytes = readRawBytesSlowPath(size, /* ensureNoLeakedReferences= */ false);
        tempPos = 0;
      }
      return Utf8.decodeUtf8(bytes, tempPos, size);
    }

    @Override
    public void readGroup(
        final int fieldNumber,
        final MessageLite.Builder builder,
        final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      checkRecursionLimit();
      ++recursionDepth;
      builder.mergeFrom(this, extensionRegistry);
      checkLastTagWas(WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP));
      --recursionDepth;
    }

    @Override
    public <T extends MessageLite> T readGroup(
        final int fieldNumber,
        final Parser<T> parser,
        final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      checkRecursionLimit();
      ++recursionDepth;
      T result = parser.parsePartialFrom(this, extensionRegistry);
      checkLastTagWas(WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP));
      --recursionDepth;
      return result;
    }

    @Deprecated
    @Override
    public void readUnknownGroup(final int fieldNumber, final MessageLite.Builder builder)
        throws IOException {
      readGroup(fieldNumber, builder, ExtensionRegistryLite.getEmptyRegistry());
    }

    @Override
    public void readMessage(
        final MessageLite.Builder builder, final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      final int length = readRawVarint32();
      checkRecursionLimit();
      final int oldLimit = pushLimit(length);
      ++recursionDepth;
      builder.mergeFrom(this, extensionRegistry);
      checkLastTagWas(0);
      --recursionDepth;
      if (getBytesUntilLimit() != 0) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      popLimit(oldLimit);
    }

    @Override
    public <T extends MessageLite> T readMessage(
        final Parser<T> parser, final ExtensionRegistryLite extensionRegistry) throws IOException {
      int length = readRawVarint32();
      checkRecursionLimit();
      final int oldLimit = pushLimit(length);
      ++recursionDepth;
      T result = parser.parsePartialFrom(this, extensionRegistry);
      checkLastTagWas(0);
      --recursionDepth;
      if (getBytesUntilLimit() != 0) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      popLimit(oldLimit);
      return result;
    }

    @Override
    public ByteString readBytes() throws IOException {
      final int size = readRawVarint32();
      if (size <= (bufferSize - pos) && size > 0) {
        // Fast path:  We already have the bytes in a contiguous buffer, so
        //   just copy directly from it.
        final ByteString result = ByteString.copyFrom(buffer, pos, size);
        pos += size;
        return result;
      }
      if (size == 0) {
        return ByteString.EMPTY;
      }
      return readBytesSlowPath(size);
    }

    @Override
    public byte[] readByteArray() throws IOException {
      final int size = readRawVarint32();
      if (size <= (bufferSize - pos) && size > 0) {
        // Fast path: We already have the bytes in a contiguous buffer, so
        // just copy directly from it.
        final byte[] result = Arrays.copyOfRange(buffer, pos, pos + size);
        pos += size;
        return result;
      } else {
        // Slow path: Build a byte array first then copy it.
        // TODO(dweis): Do we want to protect from malicious input streams here?
        return readRawBytesSlowPath(size, /* ensureNoLeakedReferences= */ false);
      }
    }

    @Override
    public ByteBuffer readByteBuffer() throws IOException {
      final int size = readRawVarint32();
      if (size <= (bufferSize - pos) && size > 0) {
        // Fast path: We already have the bytes in a contiguous buffer.
        ByteBuffer result = ByteBuffer.wrap(Arrays.copyOfRange(buffer, pos, pos + size));
        pos += size;
        return result;
      }
      if (size == 0) {
        return Internal.EMPTY_BYTE_BUFFER;
      }
      // Slow path: Build a byte array first then copy it.
      
      // We must copy as the byte array was handed off to the InputStream and a malicious
      // implementation could retain a reference.
      return ByteBuffer.wrap(readRawBytesSlowPath(size, /* ensureNoLeakedReferences= */ true));
    }

    @Override
    public int readUInt32() throws IOException {
      return readRawVarint32();
    }

    @Override
    public int readEnum() throws IOException {
      return readRawVarint32();
    }

    @Override
    public int readSFixed32() throws IOException {
      return readRawLittleEndian32();
    }

    @Override
    public long readSFixed64() throws IOException {
      return readRawLittleEndian64();
    }

    @Override
    public int readSInt32() throws IOException {
      return decodeZigZag32(readRawVarint32());
    }

    @Override
    public long readSInt64() throws IOException {
      return decodeZigZag64(readRawVarint64());
    }

    // =================================================================

    @Override
    public int readRawVarint32() throws IOException {
      // See implementation notes for readRawVarint64
      fastpath:
      {
        int tempPos = pos;

        if (bufferSize == tempPos) {
          break fastpath;
        }

        final byte[] buffer = this.buffer;
        int x;
        if ((x = buffer[tempPos++]) >= 0) {
          pos = tempPos;
          return x;
        } else if (bufferSize - tempPos < 9) {
          break fastpath;
        } else if ((x ^= (buffer[tempPos++] << 7)) < 0) {
          x ^= (~0 << 7);
        } else if ((x ^= (buffer[tempPos++] << 14)) >= 0) {
          x ^= (~0 << 7) ^ (~0 << 14);
        } else if ((x ^= (buffer[tempPos++] << 21)) < 0) {
          x ^= (~0 << 7) ^ (~0 << 14) ^ (~0 << 21);
        } else {
          int y = buffer[tempPos++];
          x ^= y << 28;
          x ^= (~0 << 7) ^ (~0 << 14) ^ (~0 << 21) ^ (~0 << 28);
          if (y < 0
              && buffer[tempPos++] < 0
              && buffer[tempPos++] < 0
              && buffer[tempPos++] < 0
              && buffer[tempPos++] < 0
              && buffer[tempPos++] < 0) {
            break fastpath; // Will throw malformedVarint()
          }
        }
        pos = tempPos;
        return x;
      }
      return (int) readRawVarint64SlowPath();
    }

    private void skipRawVarint() throws IOException {
      if (bufferSize - pos >= MAX_VARINT_SIZE) {
        skipRawVarintFastPath();
      } else {
        skipRawVarintSlowPath();
      }
    }

    private void skipRawVarintFastPath() throws IOException {
      for (int i = 0; i < MAX_VARINT_SIZE; i++) {
        if (buffer[pos++] >= 0) {
          return;
        }
      }
      throw InvalidProtocolBufferException.malformedVarint();
    }

    private void skipRawVarintSlowPath() throws IOException {
      for (int i = 0; i < MAX_VARINT_SIZE; i++) {
        if (readRawByte() >= 0) {
          return;
        }
      }
      throw InvalidProtocolBufferException.malformedVarint();
    }

    @Override
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
      fastpath:
      {
        int tempPos = pos;

        if (bufferSize == tempPos) {
          break fastpath;
        }

        final byte[] buffer = this.buffer;
        long x;
        int y;
        if ((y = buffer[tempPos++]) >= 0) {
          pos = tempPos;
          return y;
        } else if (bufferSize - tempPos < 9) {
          break fastpath;
        } else if ((y ^= (buffer[tempPos++] << 7)) < 0) {
          x = y ^ (~0 << 7);
        } else if ((y ^= (buffer[tempPos++] << 14)) >= 0) {
          x = y ^ ((~0 << 7) ^ (~0 << 14));
        } else if ((y ^= (buffer[tempPos++] << 21)) < 0) {
          x = y ^ ((~0 << 7) ^ (~0 << 14) ^ (~0 << 21));
        } else if ((x = y ^ ((long) buffer[tempPos++] << 28)) >= 0L) {
          x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28);
        } else if ((x ^= ((long) buffer[tempPos++] << 35)) < 0L) {
          x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28) ^ (~0L << 35);
        } else if ((x ^= ((long) buffer[tempPos++] << 42)) >= 0L) {
          x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28) ^ (~0L << 35) ^ (~0L << 42);
        } else if ((x ^= ((long) buffer[tempPos++] << 49)) < 0L) {
          x ^=
              (~0L << 7)
                  ^ (~0L << 14)
                  ^ (~0L << 21)
                  ^ (~0L << 28)
                  ^ (~0L << 35)
                  ^ (~0L << 42)
                  ^ (~0L << 49);
        } else {
          x ^= ((long) buffer[tempPos++] << 56);
          x ^=
              (~0L << 7)
                  ^ (~0L << 14)
                  ^ (~0L << 21)
                  ^ (~0L << 28)
                  ^ (~0L << 35)
                  ^ (~0L << 42)
                  ^ (~0L << 49)
                  ^ (~0L << 56);
          if (x < 0L) {
            if (buffer[tempPos++] < 0L) {
              break fastpath; // Will throw malformedVarint()
            }
          }
        }
        pos = tempPos;
        return x;
      }
      return readRawVarint64SlowPath();
    }

    @Override
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

    @Override
    public int readRawLittleEndian32() throws IOException {
      int tempPos = pos;

      if (bufferSize - tempPos < FIXED32_SIZE) {
        refillBuffer(FIXED32_SIZE);
        tempPos = pos;
      }

      final byte[] buffer = this.buffer;
      pos = tempPos + FIXED32_SIZE;
      return ((buffer[tempPos] & 0xff)
          | ((buffer[tempPos + 1] & 0xff) << 8)
          | ((buffer[tempPos + 2] & 0xff) << 16)
          | ((buffer[tempPos + 3] & 0xff) << 24));
    }

    @Override
    public long readRawLittleEndian64() throws IOException {
      int tempPos = pos;

      if (bufferSize - tempPos < FIXED64_SIZE) {
        refillBuffer(FIXED64_SIZE);
        tempPos = pos;
      }

      final byte[] buffer = this.buffer;
      pos = tempPos + FIXED64_SIZE;
      return (((buffer[tempPos] & 0xffL))
          | ((buffer[tempPos + 1] & 0xffL) << 8)
          | ((buffer[tempPos + 2] & 0xffL) << 16)
          | ((buffer[tempPos + 3] & 0xffL) << 24)
          | ((buffer[tempPos + 4] & 0xffL) << 32)
          | ((buffer[tempPos + 5] & 0xffL) << 40)
          | ((buffer[tempPos + 6] & 0xffL) << 48)
          | ((buffer[tempPos + 7] & 0xffL) << 56));
    }

    // -----------------------------------------------------------------

    @Override
    public void enableAliasing(boolean enabled) {
      // TODO(nathanmittler): Ideally we should throw here. Do nothing for backward compatibility.
    }

    @Override
    public void resetSizeCounter() {
      totalBytesRetired = -pos;
    }

    @Override
    public int pushLimit(int byteLimit) throws InvalidProtocolBufferException {
      if (byteLimit < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      byteLimit += totalBytesRetired + pos;
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

    @Override
    public void popLimit(final int oldLimit) {
      currentLimit = oldLimit;
      recomputeBufferSizeAfterLimit();
    }

    @Override
    public int getBytesUntilLimit() {
      if (currentLimit == Integer.MAX_VALUE) {
        return -1;
      }

      final int currentAbsolutePosition = totalBytesRetired + pos;
      return currentLimit - currentAbsolutePosition;
    }

    @Override
    public boolean isAtEnd() throws IOException {
      return pos == bufferSize && !tryRefillBuffer(1);
    }

    @Override
    public int getTotalBytesRead() {
      return totalBytesRetired + pos;
    }

    private interface RefillCallback {
      void onRefill();
    }

    private RefillCallback refillCallback = null;

    /**
     * Reads more bytes from the input, making at least {@code n} bytes available in the buffer.
     * Caller must ensure that the requested space is not yet available, and that the requested
     * space is less than BUFFER_SIZE.
     *
     * @throws InvalidProtocolBufferException The end of the stream or the current limit was
     *     reached.
     */
    private void refillBuffer(int n) throws IOException {
      if (!tryRefillBuffer(n)) {
        // We have to distinguish the exception between sizeLimitExceeded and truncatedMessage. So
        // we just throw an sizeLimitExceeded exception here if it exceeds the sizeLimit
        if (n > sizeLimit - totalBytesRetired - pos) {
          throw InvalidProtocolBufferException.sizeLimitExceeded();
        } else {
          throw InvalidProtocolBufferException.truncatedMessage();
        }
      }
    }

    /**
     * Tries to read more bytes from the input, making at least {@code n} bytes available in the
     * buffer. Caller must ensure that the requested space is not yet available, and that the
     * requested space is less than BUFFER_SIZE.
     *
     * @return {@code true} If the bytes could be made available; {@code false} 1. Current at the
     *     end of the stream 2. The current limit was reached 3. The total size limit was reached
     */
    private boolean tryRefillBuffer(int n) throws IOException {
      if (pos + n <= bufferSize) {
        throw new IllegalStateException(
            "refillBuffer() called when " + n + " bytes were already available in buffer");
      }

      // Check whether the size of total message needs to read is bigger than the size limit.
      // We shouldn't throw an exception here as isAtEnd() function needs to get this function's
      // return as the result.
      if (n > sizeLimit - totalBytesRetired - pos) {
        return false;
      }

      // Shouldn't throw the exception here either.
      if (totalBytesRetired + pos + n > currentLimit) {
        // Oops, we hit a limit.
        return false;
      }

      if (refillCallback != null) {
        refillCallback.onRefill();
      }

      int tempPos = pos;
      if (tempPos > 0) {
        if (bufferSize > tempPos) {
          System.arraycopy(buffer, tempPos, buffer, 0, bufferSize - tempPos);
        }
        totalBytesRetired += tempPos;
        bufferSize -= tempPos;
        pos = 0;
      }

      // Here we should refill the buffer as many bytes as possible.
      int bytesRead =
          read(
              input,
              buffer,
              bufferSize,
              Math.min(
                  //  the size of allocated but unused bytes in the buffer
                  buffer.length - bufferSize,
                  //  do not exceed the total bytes limit
                  sizeLimit - totalBytesRetired - bufferSize));
      if (bytesRead == 0 || bytesRead < -1 || bytesRead > buffer.length) {
        throw new IllegalStateException(
            input.getClass()
                + "#read(byte[]) returned invalid result: "
                + bytesRead
                + "\nThe InputStream implementation is buggy.");
      }
      if (bytesRead > 0) {
        bufferSize += bytesRead;
        recomputeBufferSizeAfterLimit();
        return (bufferSize >= n) ? true : tryRefillBuffer(n);
      }

      return false;
    }

    @Override
    public byte readRawByte() throws IOException {
      if (pos == bufferSize) {
        refillBuffer(1);
      }
      return buffer[pos++];
    }

    @Override
    public byte[] readRawBytes(final int size) throws IOException {
      final int tempPos = pos;
      if (size <= (bufferSize - tempPos) && size > 0) {
        pos = tempPos + size;
        return Arrays.copyOfRange(buffer, tempPos, tempPos + size);
      } else {
        // TODO(dweis): Do we want to protect from malicious input streams here?
        return readRawBytesSlowPath(size, /* ensureNoLeakedReferences= */ false);
      }
    }

    /**
     * Exactly like readRawBytes, but caller must have already checked the fast path: (size <=
     * (bufferSize - pos) && size > 0)
     * 
     * If ensureNoLeakedReferences is true, the value is guaranteed to have not escaped to
     * untrusted code.
     */
    private byte[] readRawBytesSlowPath(
        final int size, boolean ensureNoLeakedReferences) throws IOException {
      // Attempt to read the data in one byte array when it's safe to do.
      byte[] result = readRawBytesSlowPathOneChunk(size);
      if (result != null) {
        return ensureNoLeakedReferences ? result.clone() : result;
      }

      final int originalBufferPos = pos;
      final int bufferedBytes = bufferSize - pos;

      // Mark the current buffer consumed.
      totalBytesRetired += bufferSize;
      pos = 0;
      bufferSize = 0;

      // Determine the number of bytes we need to read from the input stream.
      int sizeLeft = size - bufferedBytes;

      // The size is very large. For security reasons we read them in small
      // chunks.
      List<byte[]> chunks = readRawBytesSlowPathRemainingChunks(sizeLeft);

      // OK, got everything.  Now concatenate it all into one buffer.
      final byte[] bytes = new byte[size];

      // Start by copying the leftover bytes from this.buffer.
      System.arraycopy(buffer, originalBufferPos, bytes, 0, bufferedBytes);

      // And now all the chunks.
      int tempPos = bufferedBytes;
      for (final byte[] chunk : chunks) {
        System.arraycopy(chunk, 0, bytes, tempPos, chunk.length);
        tempPos += chunk.length;
      }

      // Done.
      return bytes;
    }

    /**
     * Attempts to read the data in one byte array when it's safe to do. Returns null if the size to
     * read is too large and needs to be allocated in smaller chunks for security reasons.
     *
     * <p>Returns a byte[] that may have escaped to user code via InputStream APIs.
     */
    private byte[] readRawBytesSlowPathOneChunk(final int size) throws IOException {
      if (size == 0) {
        return Internal.EMPTY_BYTE_ARRAY;
      }
      if (size < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }

      // Integer-overflow-conscious check that the message size so far has not exceeded sizeLimit.
      int currentMessageSize = totalBytesRetired + pos + size;
      if (currentMessageSize - sizeLimit > 0) {
        throw InvalidProtocolBufferException.sizeLimitExceeded();
      }

      // Verify that the message size so far has not exceeded currentLimit.
      if (currentMessageSize > currentLimit) {
        // Read to the end of the stream anyway.
        skipRawBytes(currentLimit - totalBytesRetired - pos);
        throw InvalidProtocolBufferException.truncatedMessage();
      }

      final int bufferedBytes = bufferSize - pos;
      // Determine the number of bytes we need to read from the input stream.
      int sizeLeft = size - bufferedBytes;
      // TODO(nathanmittler): Consider using a value larger than DEFAULT_BUFFER_SIZE.
      if (sizeLeft < DEFAULT_BUFFER_SIZE || sizeLeft <= available(input)) {
        // Either the bytes we need are known to be available, or the required buffer is
        // within an allowed threshold - go ahead and allocate the buffer now.
        final byte[] bytes = new byte[size];

        // Copy all of the buffered bytes to the result buffer.
        System.arraycopy(buffer, pos, bytes, 0, bufferedBytes);
        totalBytesRetired += bufferSize;
        pos = 0;
        bufferSize = 0;

        // Fill the remaining bytes from the input stream.
        int tempPos = bufferedBytes;
        while (tempPos < bytes.length) {
          int n = read(input, bytes, tempPos, size - tempPos);
          if (n == -1) {
            throw InvalidProtocolBufferException.truncatedMessage();
          }
          totalBytesRetired += n;
          tempPos += n;
        }

        return bytes;
      }

      return null;
    }

    /**
     * Reads the remaining data in small chunks from the input stream.
     * 
     * Returns a byte[] that may have escaped to user code via InputStream APIs.
     */
    private List<byte[]> readRawBytesSlowPathRemainingChunks(int sizeLeft) throws IOException {
      // The size is very large.  For security reasons, we can't allocate the
      // entire byte array yet.  The size comes directly from the input, so a
      // maliciously-crafted message could provide a bogus very large size in
      // order to trick the app into allocating a lot of memory.  We avoid this
      // by allocating and reading only a small chunk at a time, so that the
      // malicious message must actually *be* extremely large to cause
      // problems.  Meanwhile, we limit the allowed size of a message elsewhere.
      final List<byte[]> chunks = new ArrayList<>();

      while (sizeLeft > 0) {
        // TODO(nathanmittler): Consider using a value larger than DEFAULT_BUFFER_SIZE.
        final byte[] chunk = new byte[Math.min(sizeLeft, DEFAULT_BUFFER_SIZE)];
        int tempPos = 0;
        while (tempPos < chunk.length) {
          final int n = input.read(chunk, tempPos, chunk.length - tempPos);
          if (n == -1) {
            throw InvalidProtocolBufferException.truncatedMessage();
          }
          totalBytesRetired += n;
          tempPos += n;
        }
        sizeLeft -= chunk.length;
        chunks.add(chunk);
      }

      return chunks;
    }

    /**
     * Like readBytes, but caller must have already checked the fast path: (size <= (bufferSize -
     * pos) && size > 0 || size == 0)
     */
    private ByteString readBytesSlowPath(final int size) throws IOException {
      final byte[] result = readRawBytesSlowPathOneChunk(size);
      if (result != null) {
        // We must copy as the byte array was handed off to the InputStream and a malicious
        // implementation could retain a reference.
        return ByteString.copyFrom(result);
      }

      final int originalBufferPos = pos;
      final int bufferedBytes = bufferSize - pos;

      // Mark the current buffer consumed.
      totalBytesRetired += bufferSize;
      pos = 0;
      bufferSize = 0;

      // Determine the number of bytes we need to read from the input stream.
      int sizeLeft = size - bufferedBytes;

      // The size is very large. For security reasons we read them in small
      // chunks.
      List<byte[]> chunks = readRawBytesSlowPathRemainingChunks(sizeLeft);

      // OK, got everything.  Now concatenate it all into one buffer.
      final byte[] bytes = new byte[size];

      // Start by copying the leftover bytes from this.buffer.
      System.arraycopy(buffer, originalBufferPos, bytes, 0, bufferedBytes);

      // And now all the chunks.
      int tempPos = bufferedBytes;
      for (final byte[] chunk : chunks) {
        System.arraycopy(chunk, 0, bytes, tempPos, chunk.length);
        tempPos += chunk.length;
      }
      
      return ByteString.wrap(bytes);
    }

    @Override
    public void skipRawBytes(final int size) throws IOException {
      if (size <= (bufferSize - pos) && size >= 0) {
        // We have all the bytes we need already.
        pos += size;
      } else {
        skipRawBytesSlowPath(size);
      }
    }

    /**
     * Exactly like skipRawBytes, but caller must have already checked the fast path: (size <=
     * (bufferSize - pos) && size >= 0)
     */
    private void skipRawBytesSlowPath(final int size) throws IOException {
      if (size < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }

      if (totalBytesRetired + pos + size > currentLimit) {
        // Read to the end of the stream anyway.
        skipRawBytes(currentLimit - totalBytesRetired - pos);
        // Then fail.
        throw InvalidProtocolBufferException.truncatedMessage();
      }

      int totalSkipped = 0;
      if (refillCallback == null) {
        // Skipping more bytes than are in the buffer.  First skip what we have.
        totalBytesRetired += pos;
        totalSkipped = bufferSize - pos;
        bufferSize = 0;
        pos = 0;

        try {
          while (totalSkipped < size) {
            int toSkip = size - totalSkipped;
            long skipped = skip(input, toSkip);
            if (skipped < 0 || skipped > toSkip) {
              throw new IllegalStateException(
                  input.getClass()
                      + "#skip returned invalid result: "
                      + skipped
                      + "\nThe InputStream implementation is buggy.");
            } else if (skipped == 0) {
              // The API contract of skip() permits an inputstream to skip zero bytes for any reason
              // it wants. In particular, ByteArrayInputStream will just return zero over and over
              // when it's at the end of its input. In order to actually confirm that we've hit the
              // end of input, we need to issue a read call via the other path.
              break;
            }
            totalSkipped += (int) skipped;
          }
        } finally {
          totalBytesRetired += totalSkipped;
          recomputeBufferSizeAfterLimit();
        }
      }
      if (totalSkipped < size) {
        // Skipping more bytes than are in the buffer.  First skip what we have.
        int tempPos = bufferSize - pos;
        pos = bufferSize;

        // Keep refilling the buffer until we get to the point we wanted to skip to.
        // This has the side effect of ensuring the limits are updated correctly.
        refillBuffer(1);
        while (size - tempPos > bufferSize) {
          tempPos += bufferSize;
          pos = bufferSize;
          refillBuffer(1);
        }

        pos = size - tempPos;
      }
    }
  }

  /**
   * Implementation of {@link CodedInputStream} that uses an {@link Iterable <ByteBuffer>} as the
   * data source. Requires the use of {@code sun.misc.Unsafe} to perform fast reads on the buffer.
   */
  private static final class IterableDirectByteBufferDecoder extends CodedInputStream {
    /** The object that need to decode. */
    private final Iterable<ByteBuffer> input;
    /** The {@link Iterator} with type {@link ByteBuffer} of {@code input} */
    private final Iterator<ByteBuffer> iterator;
    /** The current ByteBuffer; */
    private ByteBuffer currentByteBuffer;
    /**
     * If {@code true}, indicates that all the buffers are backing a {@link ByteString} and are
     * therefore considered to be an immutable input source.
     */
    private final boolean immutable;
    /**
     * If {@code true}, indicates that calls to read {@link ByteString} or {@code byte[]}
     * <strong>may</strong> return slices of the underlying buffer, rather than copies.
     */
    private boolean enableAliasing;
    /** The global total message length limit */
    private int totalBufferSize;
    /** The amount of available data in the input beyond {@link #currentLimit}. */
    private int bufferSizeAfterCurrentLimit;
    /** The absolute position of the end of the current message. */
    private int currentLimit = Integer.MAX_VALUE;
    /** The last tag that was read from this stream. */
    private int lastTag;
    /** Total Bytes have been Read from the {@link Iterable} {@link ByteBuffer} */
    private int totalBytesRead;
    /** The start position offset of the whole message, used as to reset the totalBytesRead */
    private int startOffset;
    /** The current position for current ByteBuffer */
    private long currentByteBufferPos;

    private long currentByteBufferStartPos;
    /**
     * If the current ByteBuffer is unsafe-direct based, currentAddress is the start address of this
     * ByteBuffer; otherwise should be zero.
     */
    private long currentAddress;
    /** The limit position for current ByteBuffer */
    private long currentByteBufferLimit;

    /**
     * The constructor of {@code Iterable<ByteBuffer>} decoder.
     *
     * @param inputBufs The input data.
     * @param size The total size of the input data.
     * @param immutableFlag whether the input data is immutable.
     */
    private IterableDirectByteBufferDecoder(
        Iterable<ByteBuffer> inputBufs, int size, boolean immutableFlag) {
      totalBufferSize = size;
      input = inputBufs;
      iterator = input.iterator();
      immutable = immutableFlag;
      startOffset = totalBytesRead = 0;
      if (size == 0) {
        currentByteBuffer = EMPTY_BYTE_BUFFER;
        currentByteBufferPos = 0;
        currentByteBufferStartPos = 0;
        currentByteBufferLimit = 0;
        currentAddress = 0;
      } else {
        tryGetNextByteBuffer();
      }
    }

    /** To get the next ByteBuffer from {@code input}, and then update the parameters */
    private void getNextByteBuffer() throws InvalidProtocolBufferException {
      if (!iterator.hasNext()) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      tryGetNextByteBuffer();
    }

    private void tryGetNextByteBuffer() {
      currentByteBuffer = iterator.next();
      totalBytesRead += (int) (currentByteBufferPos - currentByteBufferStartPos);
      currentByteBufferPos = currentByteBuffer.position();
      currentByteBufferStartPos = currentByteBufferPos;
      currentByteBufferLimit = currentByteBuffer.limit();
      currentAddress = UnsafeUtil.addressOffset(currentByteBuffer);
      currentByteBufferPos += currentAddress;
      currentByteBufferStartPos += currentAddress;
      currentByteBufferLimit += currentAddress;
    }

    @Override
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

    @Override
    public void checkLastTagWas(final int value) throws InvalidProtocolBufferException {
      if (lastTag != value) {
        throw InvalidProtocolBufferException.invalidEndTag();
      }
    }

    @Override
    public int getLastTag() {
      return lastTag;
    }

    @Override
    public boolean skipField(final int tag) throws IOException {
      switch (WireFormat.getTagWireType(tag)) {
        case WireFormat.WIRETYPE_VARINT:
          skipRawVarint();
          return true;
        case WireFormat.WIRETYPE_FIXED64:
          skipRawBytes(FIXED64_SIZE);
          return true;
        case WireFormat.WIRETYPE_LENGTH_DELIMITED:
          skipRawBytes(readRawVarint32());
          return true;
        case WireFormat.WIRETYPE_START_GROUP:
          skipMessage();
          checkLastTagWas(
              WireFormat.makeTag(WireFormat.getTagFieldNumber(tag), WireFormat.WIRETYPE_END_GROUP));
          return true;
        case WireFormat.WIRETYPE_END_GROUP:
          return false;
        case WireFormat.WIRETYPE_FIXED32:
          skipRawBytes(FIXED32_SIZE);
          return true;
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    }

    @Override
    public boolean skipField(final int tag, final CodedOutputStream output) throws IOException {
      switch (WireFormat.getTagWireType(tag)) {
        case WireFormat.WIRETYPE_VARINT:
          {
            long value = readInt64();
            output.writeUInt32NoTag(tag);
            output.writeUInt64NoTag(value);
            return true;
          }
        case WireFormat.WIRETYPE_FIXED64:
          {
            long value = readRawLittleEndian64();
            output.writeUInt32NoTag(tag);
            output.writeFixed64NoTag(value);
            return true;
          }
        case WireFormat.WIRETYPE_LENGTH_DELIMITED:
          {
            ByteString value = readBytes();
            output.writeUInt32NoTag(tag);
            output.writeBytesNoTag(value);
            return true;
          }
        case WireFormat.WIRETYPE_START_GROUP:
          {
            output.writeUInt32NoTag(tag);
            skipMessage(output);
            int endtag =
                WireFormat.makeTag(
                    WireFormat.getTagFieldNumber(tag), WireFormat.WIRETYPE_END_GROUP);
            checkLastTagWas(endtag);
            output.writeUInt32NoTag(endtag);
            return true;
          }
        case WireFormat.WIRETYPE_END_GROUP:
          {
            return false;
          }
        case WireFormat.WIRETYPE_FIXED32:
          {
            int value = readRawLittleEndian32();
            output.writeUInt32NoTag(tag);
            output.writeFixed32NoTag(value);
            return true;
          }
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    }

    @Override
    public void skipMessage() throws IOException {
      while (true) {
        final int tag = readTag();
        if (tag == 0 || !skipField(tag)) {
          return;
        }
      }
    }

    @Override
    public void skipMessage(CodedOutputStream output) throws IOException {
      while (true) {
        final int tag = readTag();
        if (tag == 0 || !skipField(tag, output)) {
          return;
        }
      }
    }

    // -----------------------------------------------------------------

    @Override
    public double readDouble() throws IOException {
      return Double.longBitsToDouble(readRawLittleEndian64());
    }

    @Override
    public float readFloat() throws IOException {
      return Float.intBitsToFloat(readRawLittleEndian32());
    }

    @Override
    public long readUInt64() throws IOException {
      return readRawVarint64();
    }

    @Override
    public long readInt64() throws IOException {
      return readRawVarint64();
    }

    @Override
    public int readInt32() throws IOException {
      return readRawVarint32();
    }

    @Override
    public long readFixed64() throws IOException {
      return readRawLittleEndian64();
    }

    @Override
    public int readFixed32() throws IOException {
      return readRawLittleEndian32();
    }

    @Override
    public boolean readBool() throws IOException {
      return readRawVarint64() != 0;
    }

    @Override
    public String readString() throws IOException {
      final int size = readRawVarint32();
      if (size > 0 && size <= currentByteBufferLimit - currentByteBufferPos) {
        byte[] bytes = new byte[size];
        UnsafeUtil.copyMemory(currentByteBufferPos, bytes, 0, size);
        String result = new String(bytes, UTF_8);
        currentByteBufferPos += size;
        return result;
      } else if (size > 0 && size <= remaining()) {
        // TODO(yilunchong): To use an underlying bytes[] instead of allocating a new bytes[]
        byte[] bytes = new byte[size];
        readRawBytesTo(bytes, 0, size);
        String result = new String(bytes, UTF_8);
        return result;
      }

      if (size == 0) {
        return "";
      }
      if (size < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    @Override
    public String readStringRequireUtf8() throws IOException {
      final int size = readRawVarint32();
      if (size > 0 && size <= currentByteBufferLimit - currentByteBufferPos) {
        final int bufferPos = (int) (currentByteBufferPos - currentByteBufferStartPos);
        String result = Utf8.decodeUtf8(currentByteBuffer, bufferPos, size);
        currentByteBufferPos += size;
        return result;
      }
      if (size >= 0 && size <= remaining()) {
        byte[] bytes = new byte[size];
        readRawBytesTo(bytes, 0, size);
        return Utf8.decodeUtf8(bytes, 0, size);
      }

      if (size == 0) {
        return "";
      }
      if (size <= 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    @Override
    public void readGroup(
        final int fieldNumber,
        final MessageLite.Builder builder,
        final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      checkRecursionLimit();
      ++recursionDepth;
      builder.mergeFrom(this, extensionRegistry);
      checkLastTagWas(WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP));
      --recursionDepth;
    }

    @Override
    public <T extends MessageLite> T readGroup(
        final int fieldNumber,
        final Parser<T> parser,
        final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      checkRecursionLimit();
      ++recursionDepth;
      T result = parser.parsePartialFrom(this, extensionRegistry);
      checkLastTagWas(WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP));
      --recursionDepth;
      return result;
    }

    @Deprecated
    @Override
    public void readUnknownGroup(final int fieldNumber, final MessageLite.Builder builder)
        throws IOException {
      readGroup(fieldNumber, builder, ExtensionRegistryLite.getEmptyRegistry());
    }

    @Override
    public void readMessage(
        final MessageLite.Builder builder, final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      final int length = readRawVarint32();
      checkRecursionLimit();
      final int oldLimit = pushLimit(length);
      ++recursionDepth;
      builder.mergeFrom(this, extensionRegistry);
      checkLastTagWas(0);
      --recursionDepth;
      if (getBytesUntilLimit() != 0) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      popLimit(oldLimit);
    }

    @Override
    public <T extends MessageLite> T readMessage(
        final Parser<T> parser, final ExtensionRegistryLite extensionRegistry) throws IOException {
      int length = readRawVarint32();
      checkRecursionLimit();
      final int oldLimit = pushLimit(length);
      ++recursionDepth;
      T result = parser.parsePartialFrom(this, extensionRegistry);
      checkLastTagWas(0);
      --recursionDepth;
      if (getBytesUntilLimit() != 0) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      popLimit(oldLimit);
      return result;
    }

    @Override
    public ByteString readBytes() throws IOException {
      final int size = readRawVarint32();
      if (size > 0 && size <= currentByteBufferLimit - currentByteBufferPos) {
        if (immutable && enableAliasing) {
          final int idx = (int) (currentByteBufferPos - currentAddress);
          final ByteString result = ByteString.wrap(slice(idx, idx + size));
          currentByteBufferPos += size;
          return result;
        } else {
          byte[] bytes = new byte[size];
          UnsafeUtil.copyMemory(currentByteBufferPos, bytes, 0, size);
          currentByteBufferPos += size;
          return ByteString.wrap(bytes);
        }
      } else if (size > 0 && size <= remaining()) {
        if (immutable && enableAliasing) {
          ArrayList<ByteString> byteStrings = new ArrayList<>();
          int l = size;
          while (l > 0) {
            if (currentRemaining() == 0) {
              getNextByteBuffer();
            }
            int bytesToCopy = Math.min(l, (int) currentRemaining());
            int idx = (int) (currentByteBufferPos - currentAddress);
            byteStrings.add(ByteString.wrap(slice(idx, idx + bytesToCopy)));
            l -= bytesToCopy;
            currentByteBufferPos += bytesToCopy;
          }
          return ByteString.copyFrom(byteStrings);
        } else {
          byte[] temp = new byte[size];
          readRawBytesTo(temp, 0, size);
          return ByteString.wrap(temp);
        }
      }

      if (size == 0) {
        return ByteString.EMPTY;
      }
      if (size < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    @Override
    public byte[] readByteArray() throws IOException {
      return readRawBytes(readRawVarint32());
    }

    @Override
    public ByteBuffer readByteBuffer() throws IOException {
      final int size = readRawVarint32();
      if (size > 0 && size <= currentRemaining()) {
        if (!immutable && enableAliasing) {
          currentByteBufferPos += size;
          return slice(
              (int) (currentByteBufferPos - currentAddress - size),
              (int) (currentByteBufferPos - currentAddress));
        } else {
          byte[] bytes = new byte[size];
          UnsafeUtil.copyMemory(currentByteBufferPos, bytes, 0, size);
          currentByteBufferPos += size;
          return ByteBuffer.wrap(bytes);
        }
      } else if (size > 0 && size <= remaining()) {
        byte[] temp = new byte[size];
        readRawBytesTo(temp, 0, size);
        return ByteBuffer.wrap(temp);
      }

      if (size == 0) {
        return EMPTY_BYTE_BUFFER;
      }
      if (size < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    @Override
    public int readUInt32() throws IOException {
      return readRawVarint32();
    }

    @Override
    public int readEnum() throws IOException {
      return readRawVarint32();
    }

    @Override
    public int readSFixed32() throws IOException {
      return readRawLittleEndian32();
    }

    @Override
    public long readSFixed64() throws IOException {
      return readRawLittleEndian64();
    }

    @Override
    public int readSInt32() throws IOException {
      return decodeZigZag32(readRawVarint32());
    }

    @Override
    public long readSInt64() throws IOException {
      return decodeZigZag64(readRawVarint64());
    }

    @Override
    public int readRawVarint32() throws IOException {
      fastpath:
      {
        long tempPos = currentByteBufferPos;

        if (currentByteBufferLimit == currentByteBufferPos) {
          break fastpath;
        }

        int x;
        if ((x = UnsafeUtil.getByte(tempPos++)) >= 0) {
          currentByteBufferPos++;
          return x;
        } else if (currentByteBufferLimit - currentByteBufferPos < 10) {
          break fastpath;
        } else if ((x ^= (UnsafeUtil.getByte(tempPos++) << 7)) < 0) {
          x ^= (~0 << 7);
        } else if ((x ^= (UnsafeUtil.getByte(tempPos++) << 14)) >= 0) {
          x ^= (~0 << 7) ^ (~0 << 14);
        } else if ((x ^= (UnsafeUtil.getByte(tempPos++) << 21)) < 0) {
          x ^= (~0 << 7) ^ (~0 << 14) ^ (~0 << 21);
        } else {
          int y = UnsafeUtil.getByte(tempPos++);
          x ^= y << 28;
          x ^= (~0 << 7) ^ (~0 << 14) ^ (~0 << 21) ^ (~0 << 28);
          if (y < 0
              && UnsafeUtil.getByte(tempPos++) < 0
              && UnsafeUtil.getByte(tempPos++) < 0
              && UnsafeUtil.getByte(tempPos++) < 0
              && UnsafeUtil.getByte(tempPos++) < 0
              && UnsafeUtil.getByte(tempPos++) < 0) {
            break fastpath; // Will throw malformedVarint()
          }
        }
        currentByteBufferPos = tempPos;
        return x;
      }
      return (int) readRawVarint64SlowPath();
    }

    @Override
    public long readRawVarint64() throws IOException {
      fastpath:
      {
        long tempPos = currentByteBufferPos;

        if (currentByteBufferLimit == currentByteBufferPos) {
          break fastpath;
        }

        long x;
        int y;
        if ((y = UnsafeUtil.getByte(tempPos++)) >= 0) {
          currentByteBufferPos++;
          return y;
        } else if (currentByteBufferLimit - currentByteBufferPos < 10) {
          break fastpath;
        } else if ((y ^= (UnsafeUtil.getByte(tempPos++) << 7)) < 0) {
          x = y ^ (~0 << 7);
        } else if ((y ^= (UnsafeUtil.getByte(tempPos++) << 14)) >= 0) {
          x = y ^ ((~0 << 7) ^ (~0 << 14));
        } else if ((y ^= (UnsafeUtil.getByte(tempPos++) << 21)) < 0) {
          x = y ^ ((~0 << 7) ^ (~0 << 14) ^ (~0 << 21));
        } else if ((x = y ^ ((long) UnsafeUtil.getByte(tempPos++) << 28)) >= 0L) {
          x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28);
        } else if ((x ^= ((long) UnsafeUtil.getByte(tempPos++) << 35)) < 0L) {
          x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28) ^ (~0L << 35);
        } else if ((x ^= ((long) UnsafeUtil.getByte(tempPos++) << 42)) >= 0L) {
          x ^= (~0L << 7) ^ (~0L << 14) ^ (~0L << 21) ^ (~0L << 28) ^ (~0L << 35) ^ (~0L << 42);
        } else if ((x ^= ((long) UnsafeUtil.getByte(tempPos++) << 49)) < 0L) {
          x ^=
              (~0L << 7)
                  ^ (~0L << 14)
                  ^ (~0L << 21)
                  ^ (~0L << 28)
                  ^ (~0L << 35)
                  ^ (~0L << 42)
                  ^ (~0L << 49);
        } else {
          x ^= ((long) UnsafeUtil.getByte(tempPos++) << 56);
          x ^=
              (~0L << 7)
                  ^ (~0L << 14)
                  ^ (~0L << 21)
                  ^ (~0L << 28)
                  ^ (~0L << 35)
                  ^ (~0L << 42)
                  ^ (~0L << 49)
                  ^ (~0L << 56);
          if (x < 0L) {
            if (UnsafeUtil.getByte(tempPos++) < 0L) {
              break fastpath; // Will throw malformedVarint()
            }
          }
        }
        currentByteBufferPos = tempPos;
        return x;
      }
      return readRawVarint64SlowPath();
    }

    @Override
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

    @Override
    public int readRawLittleEndian32() throws IOException {
      if (currentRemaining() >= FIXED32_SIZE) {
        long tempPos = currentByteBufferPos;
        currentByteBufferPos += FIXED32_SIZE;
        return ((UnsafeUtil.getByte(tempPos) & 0xff)
            | ((UnsafeUtil.getByte(tempPos + 1) & 0xff) << 8)
            | ((UnsafeUtil.getByte(tempPos + 2) & 0xff) << 16)
            | ((UnsafeUtil.getByte(tempPos + 3) & 0xff) << 24));
      }
      return ((readRawByte() & 0xff)
          | ((readRawByte() & 0xff) << 8)
          | ((readRawByte() & 0xff) << 16)
          | ((readRawByte() & 0xff) << 24));
    }

    @Override
    public long readRawLittleEndian64() throws IOException {
      if (currentRemaining() >= FIXED64_SIZE) {
        long tempPos = currentByteBufferPos;
        currentByteBufferPos += FIXED64_SIZE;
        return ((UnsafeUtil.getByte(tempPos) & 0xffL)
            | ((UnsafeUtil.getByte(tempPos + 1) & 0xffL) << 8)
            | ((UnsafeUtil.getByte(tempPos + 2) & 0xffL) << 16)
            | ((UnsafeUtil.getByte(tempPos + 3) & 0xffL) << 24)
            | ((UnsafeUtil.getByte(tempPos + 4) & 0xffL) << 32)
            | ((UnsafeUtil.getByte(tempPos + 5) & 0xffL) << 40)
            | ((UnsafeUtil.getByte(tempPos + 6) & 0xffL) << 48)
            | ((UnsafeUtil.getByte(tempPos + 7) & 0xffL) << 56));
      }
      return ((readRawByte() & 0xffL)
          | ((readRawByte() & 0xffL) << 8)
          | ((readRawByte() & 0xffL) << 16)
          | ((readRawByte() & 0xffL) << 24)
          | ((readRawByte() & 0xffL) << 32)
          | ((readRawByte() & 0xffL) << 40)
          | ((readRawByte() & 0xffL) << 48)
          | ((readRawByte() & 0xffL) << 56));
    }

    @Override
    public void enableAliasing(boolean enabled) {
      this.enableAliasing = enabled;
    }

    @Override
    public void resetSizeCounter() {
      startOffset = (int) (totalBytesRead + currentByteBufferPos - currentByteBufferStartPos);
    }

    @Override
    public int pushLimit(int byteLimit) throws InvalidProtocolBufferException {
      if (byteLimit < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      byteLimit += getTotalBytesRead();
      final int oldLimit = currentLimit;
      if (byteLimit > oldLimit) {
        throw InvalidProtocolBufferException.truncatedMessage();
      }
      currentLimit = byteLimit;

      recomputeBufferSizeAfterLimit();

      return oldLimit;
    }

    private void recomputeBufferSizeAfterLimit() {
      totalBufferSize += bufferSizeAfterCurrentLimit;
      final int bufferEnd = totalBufferSize - startOffset;
      if (bufferEnd > currentLimit) {
        // Limit is in current buffer.
        bufferSizeAfterCurrentLimit = bufferEnd - currentLimit;
        totalBufferSize -= bufferSizeAfterCurrentLimit;
      } else {
        bufferSizeAfterCurrentLimit = 0;
      }
    }

    @Override
    public void popLimit(final int oldLimit) {
      currentLimit = oldLimit;
      recomputeBufferSizeAfterLimit();
    }

    @Override
    public int getBytesUntilLimit() {
      if (currentLimit == Integer.MAX_VALUE) {
        return -1;
      }

      return currentLimit - getTotalBytesRead();
    }

    @Override
    public boolean isAtEnd() throws IOException {
      return totalBytesRead + currentByteBufferPos - currentByteBufferStartPos == totalBufferSize;
    }

    @Override
    public int getTotalBytesRead() {
      return (int)
          (totalBytesRead - startOffset + currentByteBufferPos - currentByteBufferStartPos);
    }

    @Override
    public byte readRawByte() throws IOException {
      if (currentRemaining() == 0) {
        getNextByteBuffer();
      }
      return UnsafeUtil.getByte(currentByteBufferPos++);
    }

    @Override
    public byte[] readRawBytes(final int length) throws IOException {
      if (length >= 0 && length <= currentRemaining()) {
        byte[] bytes = new byte[length];
        UnsafeUtil.copyMemory(currentByteBufferPos, bytes, 0, length);
        currentByteBufferPos += length;
        return bytes;
      }
      if (length >= 0 && length <= remaining()) {
        byte[] bytes = new byte[length];
        readRawBytesTo(bytes, 0, length);
        return bytes;
      }

      if (length <= 0) {
        if (length == 0) {
          return EMPTY_BYTE_ARRAY;
        } else {
          throw InvalidProtocolBufferException.negativeSize();
        }
      }

      throw InvalidProtocolBufferException.truncatedMessage();
    }

    /**
     * Try to get raw bytes from {@code input} with the size of {@code length} and copy to {@code
     * bytes} array. If the size is bigger than the number of remaining bytes in the input, then
     * throw {@code truncatedMessage} exception.
     */
    private void readRawBytesTo(byte[] bytes, int offset, final int length) throws IOException {
      if (length >= 0 && length <= remaining()) {
        int l = length;
        while (l > 0) {
          if (currentRemaining() == 0) {
            getNextByteBuffer();
          }
          int bytesToCopy = Math.min(l, (int) currentRemaining());
          UnsafeUtil.copyMemory(currentByteBufferPos, bytes, length - l + offset, bytesToCopy);
          l -= bytesToCopy;
          currentByteBufferPos += bytesToCopy;
        }
        return;
      }

      if (length <= 0) {
        if (length == 0) {
          return;
        } else {
          throw InvalidProtocolBufferException.negativeSize();
        }
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    @Override
    public void skipRawBytes(final int length) throws IOException {
      if (length >= 0
          && length
              <= (totalBufferSize
                  - totalBytesRead
                  - currentByteBufferPos
                  + currentByteBufferStartPos)) {
        // We have all the bytes we need already.
        int l = length;
        while (l > 0) {
          if (currentRemaining() == 0) {
            getNextByteBuffer();
          }
          int rl = Math.min(l, (int) currentRemaining());
          l -= rl;
          currentByteBufferPos += rl;
        }
        return;
      }

      if (length < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      }
      throw InvalidProtocolBufferException.truncatedMessage();
    }

    // TODO: optimize to fastpath
    private void skipRawVarint() throws IOException {
      for (int i = 0; i < MAX_VARINT_SIZE; i++) {
        if (readRawByte() >= 0) {
          return;
        }
      }
      throw InvalidProtocolBufferException.malformedVarint();
    }

    /**
     * Try to get the number of remaining bytes in {@code input}.
     *
     * @return the number of remaining bytes in {@code input}.
     */
    private int remaining() {
      return (int)
          (totalBufferSize - totalBytesRead - currentByteBufferPos + currentByteBufferStartPos);
    }

    /**
     * Try to get the number of remaining bytes in {@code currentByteBuffer}.
     *
     * @return the number of remaining bytes in {@code currentByteBuffer}
     */
    private long currentRemaining() {
      return (currentByteBufferLimit - currentByteBufferPos);
    }

    private ByteBuffer slice(int begin, int end) throws IOException {
      int prevPos = currentByteBuffer.position();
      int prevLimit = currentByteBuffer.limit();
      // View ByteBuffer as Buffer to avoid cross-Java version issues.
      // See https://issues.apache.org/jira/browse/MRESOLVER-85
      Buffer asBuffer = currentByteBuffer;
      try {
        asBuffer.position(begin);
        asBuffer.limit(end);
        return currentByteBuffer.slice();
      } catch (IllegalArgumentException e) {
        throw InvalidProtocolBufferException.truncatedMessage();
      } finally {
        asBuffer.position(prevPos);
        asBuffer.limit(prevLimit);
      }
    }
  }
}
