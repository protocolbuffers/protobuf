// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.WireFormat.FIXED32_SIZE;
import static com.google.protobuf.WireFormat.FIXED64_SIZE;
import static com.google.protobuf.WireFormat.MAX_VARINT32_SIZE;
import static com.google.protobuf.WireFormat.MAX_VARINT_SIZE;
import static java.lang.Math.max;

import com.google.protobuf.Utf8.UnpairedSurrogateException;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.BufferOverflowException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Locale;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Encodes and writes protocol message fields.
 *
 * <p>This class contains two kinds of methods: methods that write specific protocol message
 * constructs and field types (e.g. {@link #writeTag} and {@link #writeInt32}) and methods that
 * write low-level values (e.g. {@link #writeRawVarint32} and {@link #writeRawBytes}). If you are
 * writing encoded protocol messages, you should use the former methods, but if you are writing some
 * other format of your own design, use the latter.
 *
 * <p>This class is totally unsynchronized.
 */
public abstract class CodedOutputStream extends ByteOutput {
  private static final Logger logger = Logger.getLogger(CodedOutputStream.class.getName());
  private static final boolean HAS_UNSAFE_ARRAY_OPERATIONS = UnsafeUtil.hasUnsafeArrayOperations();

  /** Used to adapt to the experimental {@link Writer} interface. */
  CodedOutputStreamWriter wrapper;

  /**
   * @deprecated Use {@link #computeFixed32SizeNoTag(int)} instead.
   */
  @Deprecated public static final int LITTLE_ENDIAN_32_SIZE = FIXED32_SIZE;

  /** The buffer size used in {@link #newInstance(OutputStream)}. */
  public static final int DEFAULT_BUFFER_SIZE = 4096;

  /**
   * Returns the buffer size to efficiently write dataLength bytes to this CodedOutputStream. Used
   * by AbstractMessageLite.
   *
   * @return the buffer size to efficiently write dataLength bytes to this CodedOutputStream.
   */
  static int computePreferredBufferSize(int dataLength) {
    if (dataLength > DEFAULT_BUFFER_SIZE) {
      return DEFAULT_BUFFER_SIZE;
    }
    return dataLength;
  }

  /**
   * Create a new {@code CodedOutputStream} wrapping the given {@code OutputStream}.
   *
   * <p>NOTE: The provided {@link OutputStream} <strong>MUST NOT</strong> retain access or modify
   * the provided byte arrays. Doing so may result in corrupted data, which would be difficult to
   * debug.
   */
  public static CodedOutputStream newInstance(final OutputStream output) {
    return newInstance(output, DEFAULT_BUFFER_SIZE);
  }

  /**
   * Create a new {@code CodedOutputStream} wrapping the given {@code OutputStream} with a given
   * buffer size.
   *
   * <p>NOTE: The provided {@link OutputStream} <strong>MUST NOT</strong> retain access or modify
   * the provided byte arrays. Doing so may result in corrupted data, which would be difficult to
   * debug.
   */
  public static CodedOutputStream newInstance(final OutputStream output, final int bufferSize) {
    return new OutputStreamEncoder(output, bufferSize);
  }

  /**
   * Create a new {@code CodedOutputStream} that writes directly to the given byte array. If more
   * bytes are written than fit in the array, {@link OutOfSpaceException} will be thrown. Writing
   * directly to a flat array is faster than writing to an {@code OutputStream}. See also {@link
   * ByteString#newCodedBuilder}.
   */
  public static CodedOutputStream newInstance(final byte[] flatArray) {
    return newInstance(flatArray, 0, flatArray.length);
  }

  /**
   * Create a new {@code CodedOutputStream} that writes directly to the given byte array slice. If
   * more bytes are written than fit in the slice, {@link OutOfSpaceException} will be thrown.
   * Writing directly to a flat array is faster than writing to an {@code OutputStream}. See also
   * {@link ByteString#newCodedBuilder}.
   */
  public static CodedOutputStream newInstance(
      final byte[] flatArray, final int offset, final int length) {
    return new ArrayEncoder(flatArray, offset, length);
  }

  /** Create a new {@code CodedOutputStream} that writes to the given {@link ByteBuffer}. */
  public static CodedOutputStream newInstance(ByteBuffer buffer) {
    if (buffer.hasArray()) {
      return new HeapNioEncoder(buffer);
    }
    if (buffer.isDirect() && !buffer.isReadOnly()) {
      return UnsafeDirectNioEncoder.isSupported()
          ? newUnsafeInstance(buffer)
          : newSafeInstance(buffer);
    }
    throw new IllegalArgumentException("ByteBuffer is read-only");
  }

  /** For testing purposes only. */
  static CodedOutputStream newUnsafeInstance(ByteBuffer buffer) {
    return new UnsafeDirectNioEncoder(buffer);
  }

  /** For testing purposes only. */
  static CodedOutputStream newSafeInstance(ByteBuffer buffer) {
    return new SafeDirectNioEncoder(buffer);
  }

  /**
   * Configures serialization to be deterministic.
   *
   * <p>The deterministic serialization guarantees that for a given binary, equal (defined by the
   * {@code equals()} methods in protos) messages will always be serialized to the same bytes. This
   * implies:
   *
   * <ul>
   *   <li>repeated serialization of a message will return the same bytes
   *   <li>different processes of the same binary (which may be executing on different machines)
   *       will serialize equal messages to the same bytes.
   * </ul>
   *
   * <p>Note the deterministic serialization is NOT canonical across languages; it is also unstable
   * across different builds with schema changes due to unknown fields. Users who need canonical
   * serialization, e.g. persistent storage in a canonical form, fingerprinting, etc, should define
   * their own canonicalization specification and implement the serializer using reflection APIs
   * rather than relying on this API.
   *
   * <p>Once set, the serializer will: (Note this is an implementation detail and may subject to
   * change in the future)
   *
   * <ul>
   *   <li>sort map entries by keys in lexicographical order or numerical order. Note: For string
   *       keys, the order is based on comparing the Unicode value of each character in the strings.
   *       The order may be different from the deterministic serialization in other languages where
   *       maps are sorted on the lexicographical order of the UTF8 encoded keys.
   * </ul>
   */
  public void useDeterministicSerialization() {
    serializationDeterministic = true;
  }

  boolean isSerializationDeterministic() {
    return serializationDeterministic;
  }

  private boolean serializationDeterministic;

  /**
   * Create a new {@code CodedOutputStream} that writes to the given {@link ByteBuffer}.
   *
   * @deprecated the size parameter is no longer used since use of an internal buffer is useless
   *     (and wasteful) when writing to a {@link ByteBuffer}. Use {@link #newInstance(ByteBuffer)}
   *     instead.
   */
  @Deprecated
  public static CodedOutputStream newInstance(
      ByteBuffer byteBuffer, @SuppressWarnings("unused") int unused) {
    return newInstance(byteBuffer);
  }

  /**
   * Create a new {@code CodedOutputStream} that writes to the provided {@link ByteOutput}.
   *
   * <p>NOTE: The {@link ByteOutput} <strong>MUST NOT</strong> modify the provided buffers. Doing so
   * may result in corrupted data, which would be difficult to debug.
   *
   * @param byteOutput the output target for encoded bytes.
   * @param bufferSize the size of the internal scratch buffer to be used for string encoding.
   *     Setting this to {@code 0} will disable buffering, requiring an allocation for each encoded
   *     string.
   */
  static CodedOutputStream newInstance(ByteOutput byteOutput, int bufferSize) {
    if (bufferSize < 0) {
      throw new IllegalArgumentException("bufferSize must be positive");
    }

    return new ByteOutputEncoder(byteOutput, bufferSize);
  }

  // Disallow construction outside of this class.
  private CodedOutputStream() {}

  // -----------------------------------------------------------------

  /** Encode and write a tag. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeTag(int fieldNumber, int wireType) throws IOException;

  /** Write an {@code int32} field, including tag, to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeInt32(int fieldNumber, int value) throws IOException;

  /** Write a {@code uint32} field, including tag, to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeUInt32(int fieldNumber, int value) throws IOException;

  /** Write a {@code sint32} field, including tag, to the stream. */
  public final void writeSInt32(final int fieldNumber, final int value) throws IOException {
    writeUInt32(fieldNumber, encodeZigZag32(value));
  }

  /** Write a {@code fixed32} field, including tag, to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeFixed32(int fieldNumber, int value) throws IOException;

  /** Write an {@code sfixed32} field, including tag, to the stream. */
  public final void writeSFixed32(final int fieldNumber, final int value) throws IOException {
    writeFixed32(fieldNumber, value);
  }

  /** Write an {@code int64} field, including tag, to the stream. */
  public final void writeInt64(final int fieldNumber, final long value) throws IOException {
    writeUInt64(fieldNumber, value);
  }

  /** Write a {@code uint64} field, including tag, to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeUInt64(int fieldNumber, long value) throws IOException;

  /** Write an {@code sint64} field, including tag, to the stream. */
  public final void writeSInt64(final int fieldNumber, final long value) throws IOException {
    writeUInt64(fieldNumber, encodeZigZag64(value));
  }

  /** Write a {@code fixed64} field, including tag, to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeFixed64(int fieldNumber, long value) throws IOException;

  /** Write an {@code sfixed64} field, including tag, to the stream. */
  public final void writeSFixed64(final int fieldNumber, final long value) throws IOException {
    writeFixed64(fieldNumber, value);
  }

  /** Write a {@code float} field, including tag, to the stream. */
  public final void writeFloat(final int fieldNumber, final float value) throws IOException {
    writeFixed32(fieldNumber, Float.floatToRawIntBits(value));
  }

  /** Write a {@code double} field, including tag, to the stream. */
  public final void writeDouble(final int fieldNumber, final double value) throws IOException {
    writeFixed64(fieldNumber, Double.doubleToRawLongBits(value));
  }

  /** Write a {@code bool} field, including tag, to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeBool(int fieldNumber, boolean value) throws IOException;

  /**
   * Write an enum field, including tag, to the stream. The provided value is the numeric value used
   * to represent the enum value on the wire (not the enum ordinal value).
   */
  public final void writeEnum(final int fieldNumber, final int value) throws IOException {
    writeInt32(fieldNumber, value);
  }

  /** Write a {@code string} field, including tag, to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeString(int fieldNumber, String value) throws IOException;

  /** Write a {@code bytes} field, including tag, to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeBytes(int fieldNumber, ByteString value) throws IOException;

  /** Write a {@code bytes} field, including tag, to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeByteArray(int fieldNumber, byte[] value) throws IOException;

  /** Write a {@code bytes} field, including tag, to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeByteArray(int fieldNumber, byte[] value, int offset, int length)
      throws IOException;

  /**
   * Write a {@code bytes} field, including tag, to the stream. This method will write all content
   * of the ByteBuffer regardless of the current position and limit (i.e., the number of bytes to be
   * written is value.capacity(), not value.remaining()). Furthermore, this method doesn't alter the
   * state of the passed-in ByteBuffer. Its position, limit, mark, etc. will remain unchanged. If
   * you only want to write the remaining bytes of a ByteBuffer, you can call {@code
   * writeByteBuffer(fieldNumber, byteBuffer.slice())}.
   */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeByteBuffer(int fieldNumber, ByteBuffer value) throws IOException;

  /** Write a single byte. */
  public final void writeRawByte(final byte value) throws IOException {
    write(value);
  }

  /** Write a single byte, represented by an integer value. */
  public final void writeRawByte(final int value) throws IOException {
    write((byte) value);
  }

  /** Write an array of bytes. */
  public final void writeRawBytes(final byte[] value) throws IOException {
    write(value, 0, value.length);
  }

  /** Write part of an array of bytes. */
  public final void writeRawBytes(final byte[] value, int offset, int length) throws IOException {
    write(value, offset, length);
  }

  /** Write a byte string. */
  public final void writeRawBytes(final ByteString value) throws IOException {
    value.writeTo(this);
  }

  /**
   * Write a ByteBuffer. This method will write all content of the ByteBuffer regardless of the
   * current position and limit (i.e., the number of bytes to be written is value.capacity(), not
   * value.remaining()). Furthermore, this method doesn't alter the state of the passed-in
   * ByteBuffer. Its position, limit, mark, etc. will remain unchanged. If you only want to write
   * the remaining bytes of a ByteBuffer, you can call {@code writeRawBytes(byteBuffer.slice())}.
   */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeRawBytes(final ByteBuffer value) throws IOException;

  /** Write an embedded message field, including tag, to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeMessage(final int fieldNumber, final MessageLite value)
      throws IOException;

  /** Write an embedded message field, including tag, to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  abstract void writeMessage(final int fieldNumber, final MessageLite value, Schema schema)
      throws IOException;

  /**
   * Write a MessageSet extension field to the stream. For historical reasons, the wire format
   * differs from normal fields.
   */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeMessageSetExtension(final int fieldNumber, final MessageLite value)
      throws IOException;

  /**
   * Write an unparsed MessageSet extension field to the stream. For historical reasons, the wire
   * format differs from normal fields.
   */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeRawMessageSetExtension(final int fieldNumber, final ByteString value)
      throws IOException;

  // -----------------------------------------------------------------

  /** Write an {@code int32} field to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeInt32NoTag(final int value) throws IOException;

  /** Write a {@code uint32} field to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeUInt32NoTag(int value) throws IOException;

  /** Write a {@code sint32} field to the stream. */
  public final void writeSInt32NoTag(final int value) throws IOException {
    writeUInt32NoTag(encodeZigZag32(value));
  }

  /** Write a {@code fixed32} field to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeFixed32NoTag(int value) throws IOException;

  /** Write a {@code sfixed32} field to the stream. */
  public final void writeSFixed32NoTag(final int value) throws IOException {
    writeFixed32NoTag(value);
  }

  /** Write an {@code int64} field to the stream. */
  public final void writeInt64NoTag(final long value) throws IOException {
    writeUInt64NoTag(value);
  }

  /** Write a {@code uint64} field to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeUInt64NoTag(long value) throws IOException;

  /** Write a {@code sint64} field to the stream. */
  public final void writeSInt64NoTag(final long value) throws IOException {
    writeUInt64NoTag(encodeZigZag64(value));
  }

  /** Write a {@code fixed64} field to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeFixed64NoTag(long value) throws IOException;

  /** Write a {@code sfixed64} field to the stream. */
  public final void writeSFixed64NoTag(final long value) throws IOException {
    writeFixed64NoTag(value);
  }

  /** Write a {@code float} field to the stream. */
  public final void writeFloatNoTag(final float value) throws IOException {
    writeFixed32NoTag(Float.floatToRawIntBits(value));
  }

  /** Write a {@code double} field to the stream. */
  public final void writeDoubleNoTag(final double value) throws IOException {
    writeFixed64NoTag(Double.doubleToRawLongBits(value));
  }

  /** Write a {@code bool} field to the stream. */
  public final void writeBoolNoTag(final boolean value) throws IOException {
    write((byte) (value ? 1 : 0));
  }

  /**
   * Write an enum field to the stream. The provided value is the numeric value used to represent
   * the enum value on the wire (not the enum ordinal value).
   */
  public final void writeEnumNoTag(final int value) throws IOException {
    writeInt32NoTag(value);
  }

  /** Write a {@code string} field to the stream. */
  // TODO: Document behavior on ill-formed UTF-16 input.
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeStringNoTag(String value) throws IOException;

  /** Write a {@code bytes} field to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeBytesNoTag(final ByteString value) throws IOException;

  /** Write a {@code bytes} field to the stream. */
  public final void writeByteArrayNoTag(final byte[] value) throws IOException {
    writeByteArrayNoTag(value, 0, value.length);
  }

  /** Write an embedded message field to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  public abstract void writeMessageNoTag(final MessageLite value) throws IOException;

  /** Write an embedded message field to the stream. */
  // Abstract to avoid overhead of additional virtual method calls.
  abstract void writeMessageNoTag(final MessageLite value, Schema schema) throws IOException;

  // =================================================================

  @ExperimentalApi
  @Override
  public abstract void write(byte value) throws IOException;

  @ExperimentalApi
  @Override
  public abstract void write(byte[] value, int offset, int length) throws IOException;

  @ExperimentalApi
  @Override
  public abstract void writeLazy(byte[] value, int offset, int length) throws IOException;

  @Override
  public abstract void write(ByteBuffer value) throws IOException;

  @ExperimentalApi
  @Override
  public abstract void writeLazy(ByteBuffer value) throws IOException;

  // =================================================================
  // =================================================================

  /**
   * Compute the number of bytes that would be needed to encode an {@code int32} field, including
   * tag.
   */
  public static int computeInt32Size(final int fieldNumber, final int value) {
    return computeTagSize(fieldNumber) + computeInt32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code uint32} field, including
   * tag.
   */
  public static int computeUInt32Size(final int fieldNumber, final int value) {
    return computeTagSize(fieldNumber) + computeUInt32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an {@code sint32} field, including
   * tag.
   */
  public static int computeSInt32Size(final int fieldNumber, final int value) {
    return computeTagSize(fieldNumber) + computeSInt32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code fixed32} field, including
   * tag.
   */
  public static int computeFixed32Size(final int fieldNumber, final int value) {
    return computeTagSize(fieldNumber) + computeFixed32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an {@code sfixed32} field, including
   * tag.
   */
  public static int computeSFixed32Size(final int fieldNumber, final int value) {
    return computeTagSize(fieldNumber) + computeSFixed32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an {@code int64} field, including
   * tag.
   */
  public static int computeInt64Size(final int fieldNumber, final long value) {
    return computeTagSize(fieldNumber) + computeInt64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code uint64} field, including
   * tag.
   */
  public static int computeUInt64Size(final int fieldNumber, final long value) {
    return computeTagSize(fieldNumber) + computeUInt64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an {@code sint64} field, including
   * tag.
   */
  public static int computeSInt64Size(final int fieldNumber, final long value) {
    return computeTagSize(fieldNumber) + computeSInt64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code fixed64} field, including
   * tag.
   */
  public static int computeFixed64Size(final int fieldNumber, final long value) {
    return computeTagSize(fieldNumber) + computeFixed64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an {@code sfixed64} field, including
   * tag.
   */
  public static int computeSFixed64Size(final int fieldNumber, final long value) {
    return computeTagSize(fieldNumber) + computeSFixed64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code float} field, including
   * tag.
   */
  public static int computeFloatSize(final int fieldNumber, final float value) {
    return computeTagSize(fieldNumber) + computeFloatSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code double} field, including
   * tag.
   */
  public static int computeDoubleSize(final int fieldNumber, final double value) {
    return computeTagSize(fieldNumber) + computeDoubleSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code bool} field, including tag.
   */
  public static int computeBoolSize(final int fieldNumber, final boolean value) {
    return computeTagSize(fieldNumber) + computeBoolSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an enum field, including tag. The
   * provided value is the numeric value used to represent the enum value on the wire (not the enum
   * ordinal value).
   */
  public static int computeEnumSize(final int fieldNumber, final int value) {
    return computeTagSize(fieldNumber) + computeEnumSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code string} field, including
   * tag.
   */
  public static int computeStringSize(final int fieldNumber, final String value) {
    return computeTagSize(fieldNumber) + computeStringSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code bytes} field, including
   * tag.
   */
  public static int computeBytesSize(final int fieldNumber, final ByteString value) {
    return computeTagSize(fieldNumber) + computeBytesSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code bytes} field, including
   * tag.
   */
  public static int computeByteArraySize(final int fieldNumber, final byte[] value) {
    return computeTagSize(fieldNumber) + computeByteArraySizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code bytes} field, including
   * tag.
   */
  public static int computeByteBufferSize(final int fieldNumber, final ByteBuffer value) {
    return computeTagSize(fieldNumber) + computeByteBufferSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an embedded message in lazy field,
   * including tag.
   */
  public static int computeLazyFieldSize(final int fieldNumber, final LazyFieldLite value) {
    return computeTagSize(fieldNumber) + computeLazyFieldSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an embedded message field, including
   * tag.
   */
  public static int computeMessageSize(final int fieldNumber, final MessageLite value) {
    return computeTagSize(fieldNumber) + computeMessageSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an embedded message field, including
   * tag.
   */
  static int computeMessageSize(
      final int fieldNumber, final MessageLite value, final Schema schema) {
    return computeTagSize(fieldNumber) + computeMessageSizeNoTag(value, schema);
  }

  /**
   * Compute the number of bytes that would be needed to encode a MessageSet extension to the
   * stream. For historical reasons, the wire format differs from normal fields.
   */
  public static int computeMessageSetExtensionSize(final int fieldNumber, final MessageLite value) {
    return computeTagSize(WireFormat.MESSAGE_SET_ITEM) * 2
        + computeUInt32Size(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber)
        + computeMessageSize(WireFormat.MESSAGE_SET_MESSAGE, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an unparsed MessageSet extension
   * field to the stream. For historical reasons, the wire format differs from normal fields.
   */
  public static int computeRawMessageSetExtensionSize(
      final int fieldNumber, final ByteString value) {
    return computeTagSize(WireFormat.MESSAGE_SET_ITEM) * 2
        + computeUInt32Size(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber)
        + computeBytesSize(WireFormat.MESSAGE_SET_MESSAGE, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a lazily parsed MessageSet extension
   * field to the stream. For historical reasons, the wire format differs from normal fields.
   */
  public static int computeLazyFieldMessageSetExtensionSize(
      final int fieldNumber, final LazyFieldLite value) {
    return computeTagSize(WireFormat.MESSAGE_SET_ITEM) * 2
        + computeUInt32Size(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber)
        + computeLazyFieldSize(WireFormat.MESSAGE_SET_MESSAGE, value);
  }

  // -----------------------------------------------------------------

  /** Compute the number of bytes that would be needed to encode a tag. */
  public static int computeTagSize(final int fieldNumber) {
    return computeUInt32SizeNoTag(WireFormat.makeTag(fieldNumber, 0));
  }

  /**
   * Compute the number of bytes that would be needed to encode an {@code int32} field, excluding
   * tag.
   */
  public static int computeInt32SizeNoTag(final int value) {
    return computeUInt64SizeNoTag((long) value);
  }

  /** Compute the number of bytes that would be needed to encode a {@code uint32} field. */
  public static int computeUInt32SizeNoTag(final int value) {
    /*
    This code is ported from the C++ varint implementation.
    Implementation notes:

    To calculate varint size, we want to count the number of 7 bit chunks required. Rather than using
    division by 7 to accomplish this, we use multiplication by 9/64. This has a number of important
    properties:
     * It's roughly 1/7.111111. This makes the 0 bits set case have the same value as the 7 bits set
       case, so offsetting by 1 gives us the correct value we want for integers up to 448 bits.
     * Multiplying by 9 is special. x * 9 = x << 3 + x, and so this multiplication can be done by a
       single shifted add on arm (add w0, w0, w0, lsl #3), or a single lea instruction
       (leal (%rax,%rax,8), %eax)) on x86.
     * Dividing by 64 is a 6 bit right shift.

    An explicit non-sign-extended right shift is used instead of the more obvious '/ 64' because
    that actually produces worse code on android arm64 at time of authoring because of sign
    extension. Rather than
        lsr w0, w0, #6
    It would emit:
        add w16, w0, #0x3f (63)
        cmp w0, #0x0 (0)
        csel w0, w16, w0, lt
        asr w0, w0, #6

    Summarized:
    floor(((Integer.SIZE - clz) / 7.1111) + 1
    ((Integer.SIZE - clz) * 9) / 64 + 1
    (((Integer.SIZE - clz) * 9) >>> 6) + 1
    ((Integer.SIZE - clz) * 9 + (1 << 6)) >>> 6
    (Integer.SIZE * 9 + (1 << 6) - clz * 9) >>> 6
    (352 - clz * 9) >>> 6
    on arm:
    (352 - clz - (clz << 3)) >>> 6
    on x86:
    (352 - lea(clz, clz, 8)) >>> 6

    If you make changes here, please validate their compiled output on different architectures and
    runtimes.
    */
    int clz = Integer.numberOfLeadingZeros(value);
    return ((Integer.SIZE * 9 + (1 << 6)) - (clz * 9)) >>> 6;
  }

  /** Compute the number of bytes that would be needed to encode an {@code sint32} field. */
  public static int computeSInt32SizeNoTag(final int value) {
    return computeUInt32SizeNoTag(encodeZigZag32(value));
  }

  /** Compute the number of bytes that would be needed to encode a {@code fixed32} field. */
  public static int computeFixed32SizeNoTag(@SuppressWarnings("unused") final int unused) {
    return FIXED32_SIZE;
  }

  /** Compute the number of bytes that would be needed to encode an {@code sfixed32} field. */
  public static int computeSFixed32SizeNoTag(@SuppressWarnings("unused") final int unused) {
    return FIXED32_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode an {@code int64} field, including
   * tag.
   */
  public static int computeInt64SizeNoTag(final long value) {
    return computeUInt64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code uint64} field, including
   * tag.
   */
  public static int computeUInt64SizeNoTag(long value) {
    int clz = Long.numberOfLeadingZeros(value);
    // See computeUInt32SizeNoTag for explanation
    return ((Long.SIZE * 9 + (1 << 6)) - (clz * 9)) >>> 6;
  }

  /** Compute the number of bytes that would be needed to encode an {@code sint64} field. */
  public static int computeSInt64SizeNoTag(final long value) {
    return computeUInt64SizeNoTag(encodeZigZag64(value));
  }

  /** Compute the number of bytes that would be needed to encode a {@code fixed64} field. */
  public static int computeFixed64SizeNoTag(@SuppressWarnings("unused") final long unused) {
    return FIXED64_SIZE;
  }

  /** Compute the number of bytes that would be needed to encode an {@code sfixed64} field. */
  public static int computeSFixed64SizeNoTag(@SuppressWarnings("unused") final long unused) {
    return FIXED64_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code float} field, including
   * tag.
   */
  public static int computeFloatSizeNoTag(@SuppressWarnings("unused") final float unused) {
    return FIXED32_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code double} field, including
   * tag.
   */
  public static int computeDoubleSizeNoTag(@SuppressWarnings("unused") final double unused) {
    return FIXED64_SIZE;
  }

  /** Compute the number of bytes that would be needed to encode a {@code bool} field. */
  public static int computeBoolSizeNoTag(@SuppressWarnings("unused") final boolean unused) {
    return 1;
  }

  /**
   * Compute the number of bytes that would be needed to encode an enum field. The provided value is
   * the numeric value used to represent the enum value on the wire (not the enum ordinal value).
   */
  public static int computeEnumSizeNoTag(final int value) {
    return computeInt32SizeNoTag(value);
  }

  /** Compute the number of bytes that would be needed to encode a {@code string} field. */
  public static int computeStringSizeNoTag(final String value) {
    int length;
    try {
      length = Utf8.encodedLength(value);
    } catch (UnpairedSurrogateException e) {
      // TODO: Consider using nio Charset methods instead.
      final byte[] bytes = value.getBytes(Internal.UTF_8);
      length = bytes.length;
    }

    return computeLengthDelimitedFieldSize(length);
  }

  /**
   * Compute the number of bytes that would be needed to encode an embedded message stored in lazy
   * field.
   */
  public static int computeLazyFieldSizeNoTag(final LazyFieldLite value) {
    return computeLengthDelimitedFieldSize(value.getSerializedSize());
  }

  /** Compute the number of bytes that would be needed to encode a {@code bytes} field. */
  public static int computeBytesSizeNoTag(final ByteString value) {
    return computeLengthDelimitedFieldSize(value.size());
  }

  /** Compute the number of bytes that would be needed to encode a {@code bytes} field. */
  public static int computeByteArraySizeNoTag(final byte[] value) {
    return computeLengthDelimitedFieldSize(value.length);
  }

  /** Compute the number of bytes that would be needed to encode a {@code bytes} field. */
  public static int computeByteBufferSizeNoTag(final ByteBuffer value) {
    return computeLengthDelimitedFieldSize(value.capacity());
  }

  /** Compute the number of bytes that would be needed to encode an embedded message field. */
  public static int computeMessageSizeNoTag(final MessageLite value) {
    return computeLengthDelimitedFieldSize(value.getSerializedSize());
  }

  /** Compute the number of bytes that would be needed to encode an embedded message field. */
  static int computeMessageSizeNoTag(final MessageLite value, final Schema schema) {
    return computeLengthDelimitedFieldSize(((AbstractMessageLite) value).getSerializedSize(schema));
  }

  static int computeLengthDelimitedFieldSize(int fieldLength) {
    return computeUInt32SizeNoTag(fieldLength) + fieldLength;
  }

  /**
   * Encode a ZigZag-encoded 32-bit value. ZigZag encodes signed integers into values that can be
   * efficiently encoded with varint. (Otherwise, negative values must be sign-extended to 64 bits
   * to be varint encoded, thus always taking 10 bytes on the wire.)
   *
   * @param n A signed 32-bit integer.
   * @return An unsigned 32-bit integer, stored in a signed int because Java has no explicit
   *     unsigned support.
   */
  public static int encodeZigZag32(final int n) {
    // Note:  the right-shift must be arithmetic
    return (n << 1) ^ (n >> 31);
  }

  /**
   * Encode a ZigZag-encoded 64-bit value. ZigZag encodes signed integers into values that can be
   * efficiently encoded with varint. (Otherwise, negative values must be sign-extended to 64 bits
   * to be varint encoded, thus always taking 10 bytes on the wire.)
   *
   * @param n A signed 64-bit integer.
   * @return An unsigned 64-bit integer, stored in a signed int because Java has no explicit
   *     unsigned support.
   */
  public static long encodeZigZag64(final long n) {
    // Note:  the right-shift must be arithmetic
    return (n << 1) ^ (n >> 63);
  }

  // =================================================================

  /**
   * Flushes the stream and forces any buffered bytes to be written. This does not flush the
   * underlying OutputStream.
   */
  public abstract void flush() throws IOException;

  /**
   * If writing to a flat array, return the space left in the array. Otherwise, throws {@code
   * UnsupportedOperationException}.
   */
  public abstract int spaceLeft();

  /**
   * Verifies that {@link #spaceLeft()} returns zero. It's common to create a byte array that is
   * exactly big enough to hold a message, then write to it with a {@code CodedOutputStream}.
   * Calling {@code checkNoSpaceLeft()} after writing verifies that the message was actually as big
   * as expected, which can help catch bugs.
   */
  public final void checkNoSpaceLeft() {
    if (spaceLeft() != 0) {
      throw new IllegalStateException("Did not write as much data as expected.");
    }
  }

  /**
   * If you create a CodedOutputStream around a simple flat array, you must not attempt to write
   * more bytes than the array has space. Otherwise, this exception will be thrown.
   */
  public static class OutOfSpaceException extends IOException {
    private static final long serialVersionUID = -6947486886997889499L;

    private static final String MESSAGE =
        "CodedOutputStream was writing to a flat byte array and ran out of space.";

    OutOfSpaceException() {
      super(MESSAGE);
    }

    OutOfSpaceException(String explanationMessage) {
      super(MESSAGE + ": " + explanationMessage);
    }

    OutOfSpaceException(Throwable cause) {
      super(MESSAGE, cause);
    }

    OutOfSpaceException(String explanationMessage, Throwable cause) {
      super(MESSAGE + ": " + explanationMessage, cause);
    }

    OutOfSpaceException(int position, int limit, int length) {
      this(position, limit, length, null);
    }

    OutOfSpaceException(int position, int limit, int length, Throwable cause) {
      this((long) position, (long) limit, length, cause);
    }

    OutOfSpaceException(long position, long limit, int length) {
      this(position, limit, length, null);
    }

    OutOfSpaceException(long position, long limit, int length, Throwable cause) {
      this(String.format(Locale.US, "Pos: %d, limit: %d, len: %d", position, limit, length), cause);
    }
  }

  /**
   * Get the total number of bytes successfully written to this stream. The returned value is not
   * guaranteed to be accurate if exceptions have been found in the middle of writing.
   */
  public abstract int getTotalBytesWritten();

  // =================================================================

  /** Write a {@code bytes} field to the stream. Visible for testing. */
  abstract void writeByteArrayNoTag(final byte[] value, final int offset, final int length)
      throws IOException;

  final void inefficientWriteStringNoTag(String value, UnpairedSurrogateException cause)
      throws IOException {
    logger.log(
        Level.WARNING,
        "Converting ill-formed UTF-16. Your Protocol Buffer will not round trip correctly!",
        cause);

    // Unfortunately there does not appear to be any way to tell Java to encode
    // UTF-8 directly into our buffer, so we have to let it create its own byte
    // array and then copy.
    // TODO: Consider using nio Charset methods instead.
    final byte[] bytes = value.getBytes(Internal.UTF_8);
    try {
      writeUInt32NoTag(bytes.length);
      writeLazy(bytes, 0, bytes.length);
    } catch (IndexOutOfBoundsException e) {
      throw new OutOfSpaceException(e);
    }
  }

  // =================================================================

  /**
   * Write a {@code group} field, including tag, to the stream.
   *
   * @deprecated groups are deprecated.
   */
  @Deprecated
  public final void writeGroup(final int fieldNumber, final MessageLite value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_START_GROUP);
    writeGroupNoTag(value);
    writeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP);
  }

  /**
   * Write a {@code group} field, including tag, to the stream.
   *
   * @deprecated groups are deprecated.
   */
  @Deprecated
  final void writeGroup(final int fieldNumber, final MessageLite value, Schema schema)
      throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_START_GROUP);
    writeGroupNoTag(value, schema);
    writeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP);
  }

  /**
   * Write a {@code group} field to the stream.
   *
   * @deprecated groups are deprecated.
   */
  @Deprecated
  public final void writeGroupNoTag(final MessageLite value) throws IOException {
    value.writeTo(this);
  }

  /**
   * Write a {@code group} field to the stream.
   *
   * @deprecated groups are deprecated.
   */
  @Deprecated
  final void writeGroupNoTag(final MessageLite value, Schema schema) throws IOException {
    schema.writeTo(value, wrapper);
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code group} field, including
   * tag.
   *
   * @deprecated groups are deprecated.
   */
  @Deprecated
  public static int computeGroupSize(final int fieldNumber, final MessageLite value) {
    return computeTagSize(fieldNumber) * 2 + value.getSerializedSize();
  }

  /**
   * Compute the number of bytes that would be needed to encode a {@code group} field, including
   * tag.
   *
   * @deprecated groups are deprecated.
   */
  @Deprecated
  static int computeGroupSize(final int fieldNumber, final MessageLite value, Schema schema) {
    return computeTagSize(fieldNumber) * 2 + computeGroupSizeNoTag(value, schema);
  }

  /** Compute the number of bytes that would be needed to encode a {@code group} field. */
  @Deprecated
  @InlineMe(replacement = "value.getSerializedSize()")
  public static int computeGroupSizeNoTag(final MessageLite value) {
    return value.getSerializedSize();
  }

  /** Compute the number of bytes that would be needed to encode a {@code group} field. */
  @Deprecated
  static int computeGroupSizeNoTag(final MessageLite value, Schema schema) {
    return ((AbstractMessageLite) value).getSerializedSize(schema);
  }

  /**
   * Encode and write a varint. {@code value} is treated as unsigned, so it won't be sign-extended
   * if negative.
   *
   * @deprecated use {@link #writeUInt32NoTag} instead.
   */
  @Deprecated
  @InlineMe(replacement = "this.writeUInt32NoTag(value)")
  public final void writeRawVarint32(int value) throws IOException {
    writeUInt32NoTag(value);
  }

  /**
   * Encode and write a varint.
   *
   * @deprecated use {@link #writeUInt64NoTag} instead.
   */
  @Deprecated
  @InlineMe(replacement = "this.writeUInt64NoTag(value)")
  public final void writeRawVarint64(long value) throws IOException {
    writeUInt64NoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a varint. {@code value} is treated
   * as unsigned, so it won't be sign-extended if negative.
   *
   * @deprecated use {@link #computeUInt32SizeNoTag(int)} instead.
   */
  @Deprecated
  @InlineMe(
      replacement = "CodedOutputStream.computeUInt32SizeNoTag(value)",
      imports = "com.google.protobuf.CodedOutputStream")
  public static int computeRawVarint32Size(final int value) {
    return computeUInt32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a varint.
   *
   * @deprecated use {@link #computeUInt64SizeNoTag(long)} instead.
   */
  @Deprecated
  @InlineMe(
      replacement = "CodedOutputStream.computeUInt64SizeNoTag(value)",
      imports = "com.google.protobuf.CodedOutputStream")
  public static int computeRawVarint64Size(long value) {
    return computeUInt64SizeNoTag(value);
  }

  /**
   * Write a little-endian 32-bit integer.
   *
   * @deprecated Use {@link #writeFixed32NoTag} instead.
   */
  @Deprecated
  @InlineMe(replacement = "this.writeFixed32NoTag(value)")
  public final void writeRawLittleEndian32(final int value) throws IOException {
    writeFixed32NoTag(value);
  }

  /**
   * Write a little-endian 64-bit integer.
   *
   * @deprecated Use {@link #writeFixed64NoTag} instead.
   */
  @Deprecated
  @InlineMe(replacement = "this.writeFixed64NoTag(value)")
  public final void writeRawLittleEndian64(final long value) throws IOException {
    writeFixed64NoTag(value);
  }

  // =================================================================

  /** A {@link CodedOutputStream} that writes directly to a byte array. */
  private static class ArrayEncoder extends CodedOutputStream {
    private final byte[] buffer;
    private final int offset;
    private final int limit;
    private int position;

    ArrayEncoder(byte[] buffer, int offset, int length) {
      if (buffer == null) {
        throw new NullPointerException("buffer");
      }
      if ((offset | length | (buffer.length - (offset + length))) < 0) {
        throw new IllegalArgumentException(
            String.format(
                Locale.US,
                "Array range is invalid. Buffer.length=%d, offset=%d, length=%d",
                buffer.length,
                offset,
                length));
      }
      this.buffer = buffer;
      this.offset = offset;
      position = offset;
      limit = offset + length;
    }

    @Override
    public final void writeTag(final int fieldNumber, final int wireType) throws IOException {
      writeUInt32NoTag(WireFormat.makeTag(fieldNumber, wireType));
    }

    @Override
    public final void writeInt32(final int fieldNumber, final int value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      writeInt32NoTag(value);
    }

    @Override
    public final void writeUInt32(final int fieldNumber, final int value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      writeUInt32NoTag(value);
    }

    @Override
    public final void writeFixed32(final int fieldNumber, final int value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
      writeFixed32NoTag(value);
    }

    @Override
    public final void writeUInt64(final int fieldNumber, final long value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      writeUInt64NoTag(value);
    }

    @Override
    public final void writeFixed64(final int fieldNumber, final long value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
      writeFixed64NoTag(value);
    }

    @Override
    public final void writeBool(final int fieldNumber, final boolean value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      write((byte) (value ? 1 : 0));
    }

    @Override
    public final void writeString(final int fieldNumber, final String value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeStringNoTag(value);
    }

    @Override
    public final void writeBytes(final int fieldNumber, final ByteString value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeBytesNoTag(value);
    }

    @Override
    public final void writeByteArray(final int fieldNumber, final byte[] value) throws IOException {
      writeByteArray(fieldNumber, value, 0, value.length);
    }

    @Override
    public final void writeByteArray(
        final int fieldNumber, final byte[] value, final int offset, final int length)
        throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeByteArrayNoTag(value, offset, length);
    }

    @Override
    public final void writeByteBuffer(final int fieldNumber, final ByteBuffer value)
        throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeUInt32NoTag(value.capacity());
      writeRawBytes(value);
    }

    @Override
    public final void writeBytesNoTag(final ByteString value) throws IOException {
      writeUInt32NoTag(value.size());
      value.writeTo(this);
    }

    @Override
    public final void writeByteArrayNoTag(final byte[] value, int offset, int length)
        throws IOException {
      writeUInt32NoTag(length);
      write(value, offset, length);
    }

    @Override
    public final void writeRawBytes(final ByteBuffer value) throws IOException {
      if (value.hasArray()) {
        write(value.array(), value.arrayOffset(), value.capacity());
      } else {
        ByteBuffer duplicated = value.duplicate();
        Java8Compatibility.clear(duplicated);
        write(duplicated);
      }
    }

    @Override
    public final void writeMessage(final int fieldNumber, final MessageLite value)
        throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeMessageNoTag(value);
    }

    @Override
    final void writeMessage(final int fieldNumber, final MessageLite value, Schema schema)
        throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeUInt32NoTag(((AbstractMessageLite) value).getSerializedSize(schema));
      schema.writeTo(value, wrapper);
    }

    @Override
    public final void writeMessageSetExtension(final int fieldNumber, final MessageLite value)
        throws IOException {
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_START_GROUP);
      writeUInt32(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber);
      writeMessage(WireFormat.MESSAGE_SET_MESSAGE, value);
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_END_GROUP);
    }

    @Override
    public final void writeRawMessageSetExtension(final int fieldNumber, final ByteString value)
        throws IOException {
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_START_GROUP);
      writeUInt32(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber);
      writeBytes(WireFormat.MESSAGE_SET_MESSAGE, value);
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_END_GROUP);
    }

    @Override
    public final void writeMessageNoTag(final MessageLite value) throws IOException {
      writeUInt32NoTag(value.getSerializedSize());
      value.writeTo(this);
    }

    @Override
    final void writeMessageNoTag(final MessageLite value, Schema schema) throws IOException {
      writeUInt32NoTag(((AbstractMessageLite) value).getSerializedSize(schema));
      schema.writeTo(value, wrapper);
    }

    @Override
    public final void write(byte value) throws IOException {
      int position = this.position;
      try {
        buffer[position++] = value;
      } catch (IndexOutOfBoundsException e) {
        throw new OutOfSpaceException(position, limit, 1, e);
      }
      this.position = position; // Only update position if we stayed within the array bounds.
    }

    @Override
    public final void writeInt32NoTag(int value) throws IOException {
      if (value >= 0) {
        writeUInt32NoTag(value);
      } else {
        // Must sign-extend.
        writeUInt64NoTag(value);
      }
    }

    @Override
    public final void writeUInt32NoTag(int value) throws IOException {
      int position = this.position; // Perf: hoist field to register to avoid load/stores.
      try {
        while (true) {
          if ((value & ~0x7F) == 0) {
            buffer[position++] = (byte) value;
            break;
          } else {
            buffer[position++] = (byte) (value | 0x80);
            value >>>= 7;
          }
        }
      } catch (IndexOutOfBoundsException e) {
        throw new OutOfSpaceException(position, limit, 1, e);
      }
      this.position = position; // Only update position if we stayed within the array bounds.
    }

    @Override
    public final void writeFixed32NoTag(int value) throws IOException {
      int position = this.position; // Perf: hoist field to register to avoid load/stores.
      try {
        buffer[position] = (byte) value;
        buffer[position + 1] = (byte) (value >> 8);
        buffer[position + 2] = (byte) (value >> 16);
        buffer[position + 3] = (byte) (value >> 24);
      } catch (IndexOutOfBoundsException e) {
        throw new OutOfSpaceException(position, limit, FIXED32_SIZE, e);
      }
      // Only update position if we stayed within the array bounds.
      this.position = position + FIXED32_SIZE;
    }

    @Override
    public final void writeUInt64NoTag(long value) throws IOException {
      int position = this.position; // Perf: hoist field to register to avoid load/stores.
      if (HAS_UNSAFE_ARRAY_OPERATIONS && spaceLeft() >= MAX_VARINT_SIZE) {
        while (true) {
          if ((value & ~0x7FL) == 0) {
            UnsafeUtil.putByte(buffer, position++, (byte) value);
            break;
          } else {
            UnsafeUtil.putByte(buffer, position++, (byte) ((int) value | 0x80));
            value >>>= 7;
          }
        }
      } else {
        try {
          while (true) {
            if ((value & ~0x7FL) == 0) {
              buffer[position++] = (byte) value;
              break;
            } else {
              buffer[position++] = (byte) ((int) value | 0x80);
              value >>>= 7;
            }
          }
        } catch (IndexOutOfBoundsException e) {
          throw new OutOfSpaceException(position, limit, 1, e);
        }
      }
      this.position = position; // Only update position if we stayed within the array bounds.
    }

    @Override
    public final void writeFixed64NoTag(long value) throws IOException {
      int position = this.position; // Perf: hoist field to register to avoid load/stores.
      try {
        buffer[position] = (byte) value;
        buffer[position + 1] = (byte) (value >> 8);
        buffer[position + 2] = (byte) (value >> 16);
        buffer[position + 3] = (byte) (value >> 24);
        buffer[position + 4] = (byte) (value >> 32);
        buffer[position + 5] = (byte) (value >> 40);
        buffer[position + 6] = (byte) (value >> 48);
        buffer[position + 7] = (byte) (value >> 56);
      } catch (IndexOutOfBoundsException e) {
        throw new OutOfSpaceException(position, limit, FIXED64_SIZE, e);
      }
      // Only update position if we stayed within the array bounds.
      this.position = position + FIXED64_SIZE;
    }

    @Override
    public final void write(byte[] value, int offset, int length) throws IOException {
      try {
        System.arraycopy(value, offset, buffer, position, length);
      } catch (IndexOutOfBoundsException e) {
        throw new OutOfSpaceException(position, limit, length, e);
      }
      position += length;
    }

    @Override
    public final void writeLazy(byte[] value, int offset, int length) throws IOException {
      write(value, offset, length);
    }

    @Override
    public final void write(ByteBuffer value) throws IOException {
      final int length = value.remaining();
      try {
        value.get(buffer, position, length);
        position += length;
      } catch (IndexOutOfBoundsException e) {
        throw new OutOfSpaceException(position, limit, length, e);
      }
    }

    @Override
    public final void writeLazy(ByteBuffer value) throws IOException {
      write(value);
    }

    @Override
    public final void writeStringNoTag(String value) throws IOException {
      final int oldPosition = position;
      try {
        // UTF-8 byte length of the string is at least its UTF-16 code unit length (value.length()),
        // and at most 3 times of it. We take advantage of this in both branches below.
        final int maxLength = value.length() * Utf8.MAX_BYTES_PER_CHAR;
        final int maxLengthVarIntSize = computeUInt32SizeNoTag(maxLength);
        final int minLengthVarIntSize = computeUInt32SizeNoTag(value.length());
        if (minLengthVarIntSize == maxLengthVarIntSize) {
          position = oldPosition + minLengthVarIntSize;
          int newPosition = Utf8.encode(value, buffer, position, spaceLeft());
          // Since this class is stateful and tracks the position, we rewind and store the state,
          // prepend the length, then reset it back to the end of the string.
          position = oldPosition;
          int length = newPosition - oldPosition - minLengthVarIntSize;
          writeUInt32NoTag(length);
          position = newPosition;
        } else {
          int length = Utf8.encodedLength(value);
          writeUInt32NoTag(length);
          position = Utf8.encode(value, buffer, position, spaceLeft());
        }
      } catch (UnpairedSurrogateException e) {
        // Roll back the change - we fall back to inefficient path.
        position = oldPosition;

        // TODO: We should throw an IOException here instead.
        inefficientWriteStringNoTag(value, e);
      } catch (IndexOutOfBoundsException e) {
        throw new OutOfSpaceException(e);
      }
    }

    @Override
    public void flush() {
      // Do nothing.
    }

    @Override
    public final int spaceLeft() {
      return limit - position;
    }

    @Override
    public final int getTotalBytesWritten() {
      return position - offset;
    }
  }

  /**
   * A {@link CodedOutputStream} that writes directly to a heap {@link ByteBuffer}. Writes are done
   * directly to the underlying array. The buffer position is only updated after a flush.
   */
  private static final class HeapNioEncoder extends ArrayEncoder {
    private final ByteBuffer byteBuffer;
    private int initialPosition;

    HeapNioEncoder(ByteBuffer byteBuffer) {
      super(
          byteBuffer.array(),
          byteBuffer.arrayOffset() + byteBuffer.position(),
          byteBuffer.remaining());
      this.byteBuffer = byteBuffer;
      this.initialPosition = byteBuffer.position();
    }

    @Override
    public void flush() {
      // Update the position on the buffer.
      Java8Compatibility.position(byteBuffer, initialPosition + getTotalBytesWritten());
    }
  }

  /**
   * A {@link CodedOutputStream} that writes directly to a direct {@link ByteBuffer}, using only
   * safe operations..
   */
  private static final class SafeDirectNioEncoder extends CodedOutputStream {
    private final ByteBuffer originalBuffer;
    private final ByteBuffer buffer;
    private final int initialPosition;

    SafeDirectNioEncoder(ByteBuffer buffer) {
      this.originalBuffer = buffer;
      this.buffer = buffer.duplicate().order(ByteOrder.LITTLE_ENDIAN);
      initialPosition = buffer.position();
    }

    @Override
    public void writeTag(final int fieldNumber, final int wireType) throws IOException {
      writeUInt32NoTag(WireFormat.makeTag(fieldNumber, wireType));
    }

    @Override
    public void writeInt32(final int fieldNumber, final int value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      writeInt32NoTag(value);
    }

    @Override
    public void writeUInt32(final int fieldNumber, final int value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      writeUInt32NoTag(value);
    }

    @Override
    public void writeFixed32(final int fieldNumber, final int value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
      writeFixed32NoTag(value);
    }

    @Override
    public void writeUInt64(final int fieldNumber, final long value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      writeUInt64NoTag(value);
    }

    @Override
    public void writeFixed64(final int fieldNumber, final long value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
      writeFixed64NoTag(value);
    }

    @Override
    public void writeBool(final int fieldNumber, final boolean value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      write((byte) (value ? 1 : 0));
    }

    @Override
    public void writeString(final int fieldNumber, final String value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeStringNoTag(value);
    }

    @Override
    public void writeBytes(final int fieldNumber, final ByteString value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeBytesNoTag(value);
    }

    @Override
    public void writeByteArray(final int fieldNumber, final byte[] value) throws IOException {
      writeByteArray(fieldNumber, value, 0, value.length);
    }

    @Override
    public void writeByteArray(
        final int fieldNumber, final byte[] value, final int offset, final int length)
        throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeByteArrayNoTag(value, offset, length);
    }

    @Override
    public void writeByteBuffer(final int fieldNumber, final ByteBuffer value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeUInt32NoTag(value.capacity());
      writeRawBytes(value);
    }

    @Override
    public void writeMessage(final int fieldNumber, final MessageLite value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeMessageNoTag(value);
    }

    @Override
    void writeMessage(final int fieldNumber, final MessageLite value, Schema schema)
        throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeMessageNoTag(value, schema);
    }

    @Override
    public void writeMessageSetExtension(final int fieldNumber, final MessageLite value)
        throws IOException {
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_START_GROUP);
      writeUInt32(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber);
      writeMessage(WireFormat.MESSAGE_SET_MESSAGE, value);
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_END_GROUP);
    }

    @Override
    public void writeRawMessageSetExtension(final int fieldNumber, final ByteString value)
        throws IOException {
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_START_GROUP);
      writeUInt32(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber);
      writeBytes(WireFormat.MESSAGE_SET_MESSAGE, value);
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_END_GROUP);
    }

    @Override
    public void writeMessageNoTag(final MessageLite value) throws IOException {
      writeUInt32NoTag(value.getSerializedSize());
      value.writeTo(this);
    }

    @Override
    void writeMessageNoTag(final MessageLite value, Schema schema) throws IOException {
      writeUInt32NoTag(((AbstractMessageLite) value).getSerializedSize(schema));
      schema.writeTo(value, wrapper);
    }

    @Override
    public void write(byte value) throws IOException {
      try {
        buffer.put(value);
      } catch (BufferOverflowException e) {
        throw new OutOfSpaceException(buffer.position(), buffer.limit(), 1, e);
      }
    }

    @Override
    public void writeBytesNoTag(final ByteString value) throws IOException {
      writeUInt32NoTag(value.size());
      value.writeTo(this);
    }

    @Override
    public void writeByteArrayNoTag(final byte[] value, int offset, int length) throws IOException {
      writeUInt32NoTag(length);
      write(value, offset, length);
    }

    @Override
    public void writeRawBytes(final ByteBuffer value) throws IOException {
      if (value.hasArray()) {
        write(value.array(), value.arrayOffset(), value.capacity());
      } else {
        ByteBuffer duplicated = value.duplicate();
        Java8Compatibility.clear(duplicated);
        write(duplicated);
      }
    }

    @Override
    public void writeInt32NoTag(int value) throws IOException {
      if (value >= 0) {
        writeUInt32NoTag(value);
      } else {
        // Must sign-extend.
        writeUInt64NoTag(value);
      }
    }

    @Override
    public void writeUInt32NoTag(int value) throws IOException {
      try {
        while (true) {
          if ((value & ~0x7F) == 0) {
            buffer.put((byte) value);
            return;
          } else {
            buffer.put((byte) (value | 0x80));
            value >>>= 7;
          }
        }
      } catch (BufferOverflowException e) {
        throw new OutOfSpaceException(e);
      }
    }

    @Override
    public void writeFixed32NoTag(int value) throws IOException {
      try {
        buffer.putInt(value);
      } catch (BufferOverflowException e) {
        throw new OutOfSpaceException(buffer.position(), buffer.limit(), FIXED32_SIZE, e);
      }
    }

    @Override
    public void writeUInt64NoTag(long value) throws IOException {
      try {
        while (true) {
          if ((value & ~0x7FL) == 0) {
            buffer.put((byte) value);
            return;
          } else {
            buffer.put((byte) ((int) value | 0x80));
            value >>>= 7;
          }
        }
      } catch (BufferOverflowException e) {
        throw new OutOfSpaceException(e);
      }
    }

    @Override
    public void writeFixed64NoTag(long value) throws IOException {
      try {
        buffer.putLong(value);
      } catch (BufferOverflowException e) {
        throw new OutOfSpaceException(buffer.position(), buffer.limit(), FIXED64_SIZE, e);
      }
    }

    @Override
    public void write(byte[] value, int offset, int length) throws IOException {
      try {
        buffer.put(value, offset, length);
      } catch (IndexOutOfBoundsException e) {
        throw new OutOfSpaceException(e);
      } catch (BufferOverflowException e) {
        throw new OutOfSpaceException(e);
      }
    }

    @Override
    public void writeLazy(byte[] value, int offset, int length) throws IOException {
      write(value, offset, length);
    }

    @Override
    public void write(ByteBuffer value) throws IOException {
      try {
        buffer.put(value);
      } catch (BufferOverflowException e) {
        throw new OutOfSpaceException(e);
      }
    }

    @Override
    public void writeLazy(ByteBuffer value) throws IOException {
      write(value);
    }

    @Override
    public void writeStringNoTag(String value) throws IOException {
      final int startPos = buffer.position();
      try {
        // UTF-8 byte length of the string is at least its UTF-16 code unit length (value.length()),
        // and at most 3 times of it. We take advantage of this in both branches below.
        final int maxEncodedSize = value.length() * Utf8.MAX_BYTES_PER_CHAR;
        final int maxLengthVarIntSize = computeUInt32SizeNoTag(maxEncodedSize);
        final int minLengthVarIntSize = computeUInt32SizeNoTag(value.length());
        if (minLengthVarIntSize == maxLengthVarIntSize) {
          // Save the current position and increment past the length field. We'll come back
          // and write the length field after the encoding is complete.
          final int startOfBytes = buffer.position() + minLengthVarIntSize;
          Java8Compatibility.position(buffer, startOfBytes);

          // Encode the string.
          encode(value);

          // Now go back to the beginning and write the length.
          int endOfBytes = buffer.position();
          Java8Compatibility.position(buffer, startPos);
          writeUInt32NoTag(endOfBytes - startOfBytes);

          // Reposition the buffer past the written data.
          Java8Compatibility.position(buffer, endOfBytes);
        } else {
          final int length = Utf8.encodedLength(value);
          writeUInt32NoTag(length);
          encode(value);
        }
      } catch (UnpairedSurrogateException e) {
        // Roll back the change and convert to an IOException.
        Java8Compatibility.position(buffer, startPos);

        // TODO: We should throw an IOException here instead.
        inefficientWriteStringNoTag(value, e);
      } catch (IllegalArgumentException e) {
        // Thrown by buffer.position() if out of range.
        throw new OutOfSpaceException(e);
      }
    }

    @Override
    public void flush() {
      // Update the position of the original buffer.
      Java8Compatibility.position(originalBuffer, buffer.position());
    }

    @Override
    public int spaceLeft() {
      return buffer.remaining();
    }

    @Override
    public int getTotalBytesWritten() {
      return buffer.position() - initialPosition;
    }

    private void encode(String value) throws IOException {
      try {
        Utf8.encodeUtf8(value, buffer);
      } catch (IndexOutOfBoundsException e) {
        throw new OutOfSpaceException(e);
      }
    }
  }

  /**
   * A {@link CodedOutputStream} that writes directly to a direct {@link ByteBuffer} using {@code
   * sun.misc.Unsafe}.
   */
  private static final class UnsafeDirectNioEncoder extends CodedOutputStream {
    private final ByteBuffer originalBuffer;
    private final ByteBuffer buffer;
    private final long address;
    private final long initialPosition;
    private final long limit;
    private final long oneVarintLimit;
    private long position;

    UnsafeDirectNioEncoder(ByteBuffer buffer) {
      this.originalBuffer = buffer;
      this.buffer = buffer.duplicate().order(ByteOrder.LITTLE_ENDIAN);
      address = UnsafeUtil.addressOffset(buffer);
      initialPosition = address + buffer.position();
      limit = address + buffer.limit();
      oneVarintLimit = limit - MAX_VARINT_SIZE;
      position = initialPosition;
    }

    static boolean isSupported() {
      return UnsafeUtil.hasUnsafeByteBufferOperations();
    }

    @Override
    public void writeTag(int fieldNumber, int wireType) throws IOException {
      writeUInt32NoTag(WireFormat.makeTag(fieldNumber, wireType));
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
    public void writeFixed32(int fieldNumber, int value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
      writeFixed32NoTag(value);
    }

    @Override
    public void writeUInt64(int fieldNumber, long value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      writeUInt64NoTag(value);
    }

    @Override
    public void writeFixed64(int fieldNumber, long value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
      writeFixed64NoTag(value);
    }

    @Override
    public void writeBool(int fieldNumber, boolean value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      write((byte) (value ? 1 : 0));
    }

    @Override
    public void writeString(int fieldNumber, String value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeStringNoTag(value);
    }

    @Override
    public void writeBytes(int fieldNumber, ByteString value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeBytesNoTag(value);
    }

    @Override
    public void writeByteArray(int fieldNumber, byte[] value) throws IOException {
      writeByteArray(fieldNumber, value, 0, value.length);
    }

    @Override
    public void writeByteArray(int fieldNumber, byte[] value, int offset, int length)
        throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeByteArrayNoTag(value, offset, length);
    }

    @Override
    public void writeByteBuffer(int fieldNumber, ByteBuffer value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeUInt32NoTag(value.capacity());
      writeRawBytes(value);
    }

    @Override
    public void writeMessage(int fieldNumber, MessageLite value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeMessageNoTag(value);
    }

    @Override
    void writeMessage(int fieldNumber, MessageLite value, Schema schema) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeMessageNoTag(value, schema);
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
    public void writeMessageNoTag(MessageLite value) throws IOException {
      writeUInt32NoTag(value.getSerializedSize());
      value.writeTo(this);
    }

    @Override
    void writeMessageNoTag(MessageLite value, Schema schema) throws IOException {
      writeUInt32NoTag(((AbstractMessageLite) value).getSerializedSize(schema));
      schema.writeTo(value, wrapper);
    }

    @Override
    public void write(byte value) throws IOException {
      if (position >= limit) {
        throw new OutOfSpaceException(position, limit, 1);
      }
      UnsafeUtil.putByte(position++, value);
    }

    @Override
    public void writeBytesNoTag(ByteString value) throws IOException {
      writeUInt32NoTag(value.size());
      value.writeTo(this);
    }

    @Override
    public void writeByteArrayNoTag(byte[] value, int offset, int length) throws IOException {
      writeUInt32NoTag(length);
      write(value, offset, length);
    }

    @Override
    public void writeRawBytes(ByteBuffer value) throws IOException {
      if (value.hasArray()) {
        write(value.array(), value.arrayOffset(), value.capacity());
      } else {
        ByteBuffer duplicated = value.duplicate();
        Java8Compatibility.clear(duplicated);
        write(duplicated);
      }
    }

    @Override
    public void writeInt32NoTag(int value) throws IOException {
      if (value >= 0) {
        writeUInt32NoTag(value);
      } else {
        // Must sign-extend.
        writeUInt64NoTag(value);
      }
    }

    @Override
    public void writeUInt32NoTag(int value) throws IOException {
      long position = this.position; // Perf: hoist field to register to avoid load/stores.
      if (position <= oneVarintLimit) {
        // Optimization to avoid bounds checks on each iteration.
        while (true) {
          if ((value & ~0x7F) == 0) {
            UnsafeUtil.putByte(position++, (byte) value);
            break;
          } else {
            UnsafeUtil.putByte(position++, (byte) (value | 0x80));
            value >>>= 7;
          }
        }
      } else {
        while (true) {
          if (position >= limit) {
            throw new OutOfSpaceException(
                String.format("Pos: %d, limit: %d, len: %d", position, limit, 1));
          }
          if ((value & ~0x7F) == 0) {
            UnsafeUtil.putByte(position++, (byte) value);
            break;
          } else {
            UnsafeUtil.putByte(position++, (byte) (value | 0x80));
            value >>>= 7;
          }
        }
      }
      this.position = position; // Only update position if we stayed within the array bounds.
    }

    @Override
    public void writeFixed32NoTag(int value) throws IOException {
      try {
        buffer.putInt(bufferPos(position), value);
      } catch (IndexOutOfBoundsException e) {
        throw new OutOfSpaceException(position, limit, FIXED32_SIZE, e);
      }
      position += FIXED32_SIZE;
    }

    @Override
    public void writeUInt64NoTag(long value) throws IOException {
      long position = this.position; // Perf: hoist field to register to avoid load/stores.
      if (position <= oneVarintLimit) {
        // Optimization to avoid bounds checks on each iteration.
        while (true) {
          if ((value & ~0x7FL) == 0) {
            UnsafeUtil.putByte(position++, (byte) value);
            break;
          } else {
            UnsafeUtil.putByte(position++, (byte) ((int) value | 0x80));
            value >>>= 7;
          }
        }
      } else {
        while (true) {
          if (position >= limit) {
            throw new OutOfSpaceException(position, limit, 1);
          }
          if ((value & ~0x7FL) == 0) {
            UnsafeUtil.putByte(position++, (byte) value);
            break;
          } else {
            UnsafeUtil.putByte(position++, (byte) ((int) value | 0x80));
            value >>>= 7;
          }
        }
      }
      this.position = position; // Only update position if we stayed within the array bounds.
    }

    @Override
    public void writeFixed64NoTag(long value) throws IOException {
      try {
        buffer.putLong(bufferPos(position), value);
      } catch (IndexOutOfBoundsException e) {
        throw new OutOfSpaceException(position, limit, FIXED64_SIZE, e);
      }
      position += FIXED64_SIZE;
    }

    @Override
    public void write(byte[] value, int offset, int length) throws IOException {
      if (value == null
          || offset < 0
          || length < 0
          || (value.length - length) < offset
          || (limit - length) < position) {
        if (value == null) {
          throw new NullPointerException("value");
        }
        throw new OutOfSpaceException(position, limit, length);
      }

      UnsafeUtil.copyMemory(value, offset, position, length);
      position += length;
    }

    @Override
    public void writeLazy(byte[] value, int offset, int length) throws IOException {
      write(value, offset, length);
    }

    @Override
    public void write(ByteBuffer value) throws IOException {
      try {
        int length = value.remaining();
        repositionBuffer(position);
        buffer.put(value);
        position += length;
      } catch (BufferOverflowException e) {
        throw new OutOfSpaceException(e);
      }
    }

    @Override
    public void writeLazy(ByteBuffer value) throws IOException {
      write(value);
    }

    @Override
    public void writeStringNoTag(String value) throws IOException {
      long prevPos = position;
      try {
        // UTF-8 byte length of the string is at least its UTF-16 code unit length (value.length()),
        // and at most 3 times of it. We take advantage of this in both branches below.
        int maxEncodedSize = value.length() * Utf8.MAX_BYTES_PER_CHAR;
        int maxLengthVarIntSize = computeUInt32SizeNoTag(maxEncodedSize);
        int minLengthVarIntSize = computeUInt32SizeNoTag(value.length());
        if (minLengthVarIntSize == maxLengthVarIntSize) {
          // Save the current position and increment past the length field. We'll come back
          // and write the length field after the encoding is complete.
          int stringStart = bufferPos(position) + minLengthVarIntSize;
          Java8Compatibility.position(buffer, stringStart);

          // Encode the string.
          Utf8.encodeUtf8(value, buffer);

          // Write the length and advance the position.
          int length = buffer.position() - stringStart;
          writeUInt32NoTag(length);
          position += length;
        } else {
          // Calculate and write the encoded length.
          int length = Utf8.encodedLength(value);
          writeUInt32NoTag(length);

          // Write the string and advance the position.
          repositionBuffer(position);
          Utf8.encodeUtf8(value, buffer);
          position += length;
        }
      } catch (UnpairedSurrogateException e) {
        // Roll back the change and convert to an IOException.
        position = prevPos;
        repositionBuffer(position);

        // TODO: We should throw an IOException here instead.
        inefficientWriteStringNoTag(value, e);
      } catch (IllegalArgumentException e) {
        // Thrown by buffer.position() if out of range.
        throw new OutOfSpaceException(e);
      } catch (IndexOutOfBoundsException e) {
        throw new OutOfSpaceException(e);
      }
    }

    @Override
    public void flush() {
      // Update the position of the original buffer.
      Java8Compatibility.position(originalBuffer, bufferPos(position));
    }

    @Override
    public int spaceLeft() {
      return (int) (limit - position);
    }

    @Override
    public int getTotalBytesWritten() {
      return (int) (position - initialPosition);
    }

    private void repositionBuffer(long pos) {
      Java8Compatibility.position(buffer, bufferPos(pos));
    }

    private int bufferPos(long pos) {
      return (int) (pos - address);
    }
  }

  /** Abstract base class for buffered encoders. */
  private abstract static class AbstractBufferedEncoder extends CodedOutputStream {
    final byte[] buffer;
    final int limit;
    int position;
    int totalBytesWritten;

    AbstractBufferedEncoder(int bufferSize) {
      if (bufferSize < 0) {
        throw new IllegalArgumentException("bufferSize must be >= 0");
      }
      // As an optimization, we require that the buffer be able to store at least 2
      // varints so that we can buffer any integer write (tag + value). This reduces the
      // number of range checks for a single write to 1 (i.e. if there is not enough space
      // to buffer the tag+value, flush and then buffer it).
      this.buffer = new byte[max(bufferSize, MAX_VARINT_SIZE * 2)];
      this.limit = buffer.length;
    }

    @Override
    public final int spaceLeft() {
      throw new UnsupportedOperationException(
          "spaceLeft() can only be called on CodedOutputStreams that are "
              + "writing to a flat array or ByteBuffer.");
    }

    @Override
    public final int getTotalBytesWritten() {
      return totalBytesWritten;
    }

    /**
     * This method does not perform bounds checking on the array. Checking array bounds is the
     * responsibility of the caller.
     */
    final void buffer(byte value) {
      int position = this.position;
      buffer[position] = value;
      // Android optimisation: 1 fewer instruction codegen vs buffer[position++].
      this.position = position + 1;
      totalBytesWritten++;
    }

    /**
     * This method does not perform bounds checking on the array. Checking array bounds is the
     * responsibility of the caller.
     */
    final void bufferTag(final int fieldNumber, final int wireType) {
      bufferUInt32NoTag(WireFormat.makeTag(fieldNumber, wireType));
    }

    /**
     * This method does not perform bounds checking on the array. Checking array bounds is the
     * responsibility of the caller.
     */
    final void bufferInt32NoTag(final int value) {
      if (value >= 0) {
        bufferUInt32NoTag(value);
      } else {
        // Must sign-extend.
        bufferUInt64NoTag(value);
      }
    }

    /**
     * This method does not perform bounds checking on the array. Checking array bounds is the
     * responsibility of the caller.
     */
    final void bufferUInt32NoTag(int value) {
      if (HAS_UNSAFE_ARRAY_OPERATIONS) {
        final long originalPos = position;
        while (true) {
          if ((value & ~0x7F) == 0) {
            UnsafeUtil.putByte(buffer, position++, (byte) value);
            break;
          } else {
            UnsafeUtil.putByte(buffer, position++, (byte) (value | 0x80));
            value >>>= 7;
          }
        }
        int delta = (int) (position - originalPos);
        totalBytesWritten += delta;
      } else {
        while (true) {
          if ((value & ~0x7F) == 0) {
            buffer[position++] = (byte) value;
            totalBytesWritten++;
            return;
          } else {
            buffer[position++] = (byte) (value | 0x80);
            totalBytesWritten++;
            value >>>= 7;
          }
        }
      }
    }

    /**
     * This method does not perform bounds checking on the array. Checking array bounds is the
     * responsibility of the caller.
     */
    final void bufferUInt64NoTag(long value) {
      if (HAS_UNSAFE_ARRAY_OPERATIONS) {
        final long originalPos = position;
        while (true) {
          if ((value & ~0x7FL) == 0) {
            UnsafeUtil.putByte(buffer, position++, (byte) value);
            break;
          } else {
            UnsafeUtil.putByte(buffer, position++, (byte) ((int) value | 0x80));
            value >>>= 7;
          }
        }
        int delta = (int) (position - originalPos);
        totalBytesWritten += delta;
      } else {
        while (true) {
          if ((value & ~0x7FL) == 0) {
            buffer[position++] = (byte) value;
            totalBytesWritten++;
            return;
          } else {
            buffer[position++] = (byte) ((int) value | 0x80);
            totalBytesWritten++;
            value >>>= 7;
          }
        }
      }
    }

    /**
     * This method does not perform bounds checking on the array. Checking array bounds is the
     * responsibility of the caller.
     */
    final void bufferFixed32NoTag(int value) {
      int position = this.position; // Perf: hoist field to register to avoid load/stores.
      buffer[position++] = (byte) value;
      buffer[position++] = (byte) (value >> 8);
      buffer[position++] = (byte) (value >> 16);
      buffer[position++] = (byte) (value >> 24);
      this.position = position;
      totalBytesWritten += FIXED32_SIZE;
    }

    /**
     * This method does not perform bounds checking on the array. Checking array bounds is the
     * responsibility of the caller.
     */
    final void bufferFixed64NoTag(long value) {
      int position = this.position; // Perf: hoist field to register to avoid load/stores.
      buffer[position++] = (byte) value;
      buffer[position++] = (byte) (value >> 8);
      buffer[position++] = (byte) (value >> 16);
      buffer[position++] = (byte) (value >> 24);
      buffer[position++] = (byte) (value >> 32);
      buffer[position++] = (byte) (value >> 40);
      buffer[position++] = (byte) (value >> 48);
      buffer[position++] = (byte) (value >> 56);
      this.position = position;
      totalBytesWritten += FIXED64_SIZE;
    }
  }

  /**
   * A {@link CodedOutputStream} that decorates a {@link ByteOutput}. It internal buffer only to
   * support string encoding operations. All other writes are just passed through to the {@link
   * ByteOutput}.
   */
  private static final class ByteOutputEncoder extends AbstractBufferedEncoder {
    private final ByteOutput out;

    ByteOutputEncoder(ByteOutput out, int bufferSize) {
      super(bufferSize);
      if (out == null) {
        throw new NullPointerException("out");
      }
      this.out = out;
    }

    @Override
    public void writeTag(final int fieldNumber, final int wireType) throws IOException {
      writeUInt32NoTag(WireFormat.makeTag(fieldNumber, wireType));
    }

    @Override
    public void writeInt32(final int fieldNumber, final int value) throws IOException {
      flushIfNotAvailable(MAX_VARINT_SIZE * 2);
      bufferTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      bufferInt32NoTag(value);
    }

    @Override
    public void writeUInt32(final int fieldNumber, final int value) throws IOException {
      flushIfNotAvailable(MAX_VARINT_SIZE * 2);
      bufferTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      bufferUInt32NoTag(value);
    }

    @Override
    public void writeFixed32(final int fieldNumber, final int value) throws IOException {
      flushIfNotAvailable(MAX_VARINT_SIZE + FIXED32_SIZE);
      bufferTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
      bufferFixed32NoTag(value);
    }

    @Override
    public void writeUInt64(final int fieldNumber, final long value) throws IOException {
      flushIfNotAvailable(MAX_VARINT_SIZE * 2);
      bufferTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      bufferUInt64NoTag(value);
    }

    @Override
    public void writeFixed64(final int fieldNumber, final long value) throws IOException {
      flushIfNotAvailable(MAX_VARINT_SIZE + FIXED64_SIZE);
      bufferTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
      bufferFixed64NoTag(value);
    }

    @Override
    public void writeBool(final int fieldNumber, final boolean value) throws IOException {
      flushIfNotAvailable(MAX_VARINT_SIZE + 1);
      bufferTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      buffer((byte) (value ? 1 : 0));
    }

    @Override
    public void writeString(final int fieldNumber, final String value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeStringNoTag(value);
    }

    @Override
    public void writeBytes(final int fieldNumber, final ByteString value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeBytesNoTag(value);
    }

    @Override
    public void writeByteArray(final int fieldNumber, final byte[] value) throws IOException {
      writeByteArray(fieldNumber, value, 0, value.length);
    }

    @Override
    public void writeByteArray(
        final int fieldNumber, final byte[] value, final int offset, final int length)
        throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeByteArrayNoTag(value, offset, length);
    }

    @Override
    public void writeByteBuffer(final int fieldNumber, final ByteBuffer value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeUInt32NoTag(value.capacity());
      writeRawBytes(value);
    }

    @Override
    public void writeBytesNoTag(final ByteString value) throws IOException {
      writeUInt32NoTag(value.size());
      value.writeTo(this);
    }

    @Override
    public void writeByteArrayNoTag(final byte[] value, int offset, int length) throws IOException {
      writeUInt32NoTag(length);
      write(value, offset, length);
    }

    @Override
    public void writeRawBytes(final ByteBuffer value) throws IOException {
      if (value.hasArray()) {
        write(value.array(), value.arrayOffset(), value.capacity());
      } else {
        ByteBuffer duplicated = value.duplicate();
        Java8Compatibility.clear(duplicated);
        write(duplicated);
      }
    }

    @Override
    public void writeMessage(final int fieldNumber, final MessageLite value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeMessageNoTag(value);
    }

    @Override
    void writeMessage(final int fieldNumber, final MessageLite value, Schema schema)
        throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeMessageNoTag(value, schema);
    }

    @Override
    public void writeMessageSetExtension(final int fieldNumber, final MessageLite value)
        throws IOException {
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_START_GROUP);
      writeUInt32(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber);
      writeMessage(WireFormat.MESSAGE_SET_MESSAGE, value);
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_END_GROUP);
    }

    @Override
    public void writeRawMessageSetExtension(final int fieldNumber, final ByteString value)
        throws IOException {
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_START_GROUP);
      writeUInt32(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber);
      writeBytes(WireFormat.MESSAGE_SET_MESSAGE, value);
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_END_GROUP);
    }

    @Override
    public void writeMessageNoTag(final MessageLite value) throws IOException {
      writeUInt32NoTag(value.getSerializedSize());
      value.writeTo(this);
    }

    @Override
    void writeMessageNoTag(final MessageLite value, Schema schema) throws IOException {
      writeUInt32NoTag(((AbstractMessageLite) value).getSerializedSize(schema));
      schema.writeTo(value, wrapper);
    }

    @Override
    public void write(byte value) throws IOException {
      if (position == limit) {
        doFlush();
      }

      buffer(value);
    }

    @Override
    public void writeInt32NoTag(int value) throws IOException {
      if (value >= 0) {
        writeUInt32NoTag(value);
      } else {
        // Must sign-extend.
        writeUInt64NoTag(value);
      }
    }

    @Override
    public void writeUInt32NoTag(int value) throws IOException {
      flushIfNotAvailable(MAX_VARINT32_SIZE);
      bufferUInt32NoTag(value);
    }

    @Override
    public void writeFixed32NoTag(final int value) throws IOException {
      flushIfNotAvailable(FIXED32_SIZE);
      bufferFixed32NoTag(value);
    }

    @Override
    public void writeUInt64NoTag(long value) throws IOException {
      flushIfNotAvailable(MAX_VARINT_SIZE);
      bufferUInt64NoTag(value);
    }

    @Override
    public void writeFixed64NoTag(final long value) throws IOException {
      flushIfNotAvailable(FIXED64_SIZE);
      bufferFixed64NoTag(value);
    }

    @Override
    public void writeStringNoTag(String value) throws IOException {
      // UTF-8 byte length of the string is at least its UTF-16 code unit length (value.length()),
      // and at most 3 times of it. We take advantage of this in both branches below.
      final int maxLength = value.length() * Utf8.MAX_BYTES_PER_CHAR;
      final int maxLengthVarIntSize = computeUInt32SizeNoTag(maxLength);

      // If we are streaming and the potential length is too big to fit in our buffer, we take the
      // slower path.
      if (maxLengthVarIntSize + maxLength > limit) {
        // Allocate a byte[] that we know can fit the string and encode into it. String.getBytes()
        // does the same internally and then does *another copy* to return a byte[] of exactly the
        // right size. We can skip that copy and just writeRawBytes up to the actualLength of the
        // UTF-8 encoded bytes.
        final byte[] encodedBytes = new byte[maxLength];
        int actualLength = Utf8.encode(value, encodedBytes, 0, maxLength);
        writeUInt32NoTag(actualLength);
        writeLazy(encodedBytes, 0, actualLength);
        return;
      }

      // Fast path: we have enough space available in our buffer for the string...
      if (maxLengthVarIntSize + maxLength > limit - position) {
        // Flush to free up space.
        doFlush();
      }

      final int oldPosition = position;
      try {
        // Optimize for the case where we know this length results in a constant varint length as
        // this saves a pass for measuring the length of the string.
        final int minLengthVarIntSize = computeUInt32SizeNoTag(value.length());

        if (minLengthVarIntSize == maxLengthVarIntSize) {
          position = oldPosition + minLengthVarIntSize;
          int newPosition = Utf8.encode(value, buffer, position, limit - position);
          // Since this class is stateful and tracks the position, we rewind and store the state,
          // prepend the length, then reset it back to the end of the string.
          position = oldPosition;
          int length = newPosition - oldPosition - minLengthVarIntSize;
          bufferUInt32NoTag(length);
          position = newPosition;
          totalBytesWritten += length;
        } else {
          int length = Utf8.encodedLength(value);
          bufferUInt32NoTag(length);
          position = Utf8.encode(value, buffer, position, length);
          totalBytesWritten += length;
        }
      } catch (UnpairedSurrogateException e) {
        // Roll back the change and convert to an IOException.
        totalBytesWritten -= position - oldPosition;
        position = oldPosition;

        // TODO: We should throw an IOException here instead.
        inefficientWriteStringNoTag(value, e);
      } catch (IndexOutOfBoundsException e) {
        throw new OutOfSpaceException(e);
      }
    }

    @Override
    public void flush() throws IOException {
      if (position > 0) {
        // Flush the buffer.
        doFlush();
      }
    }

    @Override
    public void write(byte[] value, int offset, int length) throws IOException {
      flush();
      out.write(value, offset, length);
      totalBytesWritten += length;
    }

    @Override
    public void writeLazy(byte[] value, int offset, int length) throws IOException {
      flush();
      out.writeLazy(value, offset, length);
      totalBytesWritten += length;
    }

    @Override
    public void write(ByteBuffer value) throws IOException {
      flush();
      int length = value.remaining();
      out.write(value);
      totalBytesWritten += length;
    }

    @Override
    public void writeLazy(ByteBuffer value) throws IOException {
      flush();
      int length = value.remaining();
      out.writeLazy(value);
      totalBytesWritten += length;
    }

    private void flushIfNotAvailable(int requiredSize) throws IOException {
      if (limit - position < requiredSize) {
        doFlush();
      }
    }

    private void doFlush() throws IOException {
      out.write(buffer, 0, position);
      position = 0;
    }
  }

  /**
   * An {@link CodedOutputStream} that decorates an {@link OutputStream}. It performs internal
   * buffering to optimize writes to the {@link OutputStream}.
   */
  private static final class OutputStreamEncoder extends AbstractBufferedEncoder {
    private final OutputStream out;

    OutputStreamEncoder(OutputStream out, int bufferSize) {
      super(bufferSize);
      if (out == null) {
        throw new NullPointerException("out");
      }
      this.out = out;
    }

    @Override
    public void writeTag(final int fieldNumber, final int wireType) throws IOException {
      writeUInt32NoTag(WireFormat.makeTag(fieldNumber, wireType));
    }

    @Override
    public void writeInt32(final int fieldNumber, final int value) throws IOException {
      flushIfNotAvailable(MAX_VARINT_SIZE * 2);
      bufferTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      bufferInt32NoTag(value);
    }

    @Override
    public void writeUInt32(final int fieldNumber, final int value) throws IOException {
      flushIfNotAvailable(MAX_VARINT_SIZE * 2);
      bufferTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      bufferUInt32NoTag(value);
    }

    @Override
    public void writeFixed32(final int fieldNumber, final int value) throws IOException {
      flushIfNotAvailable(MAX_VARINT_SIZE + FIXED32_SIZE);
      bufferTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
      bufferFixed32NoTag(value);
    }

    @Override
    public void writeUInt64(final int fieldNumber, final long value) throws IOException {
      flushIfNotAvailable(MAX_VARINT_SIZE * 2);
      bufferTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      bufferUInt64NoTag(value);
    }

    @Override
    public void writeFixed64(final int fieldNumber, final long value) throws IOException {
      flushIfNotAvailable(MAX_VARINT_SIZE + FIXED64_SIZE);
      bufferTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
      bufferFixed64NoTag(value);
    }

    @Override
    public void writeBool(final int fieldNumber, final boolean value) throws IOException {
      flushIfNotAvailable(MAX_VARINT_SIZE + 1);
      bufferTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
      buffer((byte) (value ? 1 : 0));
    }

    @Override
    public void writeString(final int fieldNumber, final String value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeStringNoTag(value);
    }

    @Override
    public void writeBytes(final int fieldNumber, final ByteString value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeBytesNoTag(value);
    }

    @Override
    public void writeByteArray(final int fieldNumber, final byte[] value) throws IOException {
      writeByteArray(fieldNumber, value, 0, value.length);
    }

    @Override
    public void writeByteArray(
        final int fieldNumber, final byte[] value, final int offset, final int length)
        throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeByteArrayNoTag(value, offset, length);
    }

    @Override
    public void writeByteBuffer(final int fieldNumber, final ByteBuffer value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeUInt32NoTag(value.capacity());
      writeRawBytes(value);
    }

    @Override
    public void writeBytesNoTag(final ByteString value) throws IOException {
      writeUInt32NoTag(value.size());
      value.writeTo(this);
    }

    @Override
    public void writeByteArrayNoTag(final byte[] value, int offset, int length) throws IOException {
      writeUInt32NoTag(length);
      write(value, offset, length);
    }

    @Override
    public void writeRawBytes(final ByteBuffer value) throws IOException {
      if (value.hasArray()) {
        write(value.array(), value.arrayOffset(), value.capacity());
      } else {
        ByteBuffer duplicated = value.duplicate();
        Java8Compatibility.clear(duplicated);
        write(duplicated);
      }
    }

    @Override
    public void writeMessage(final int fieldNumber, final MessageLite value) throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeMessageNoTag(value);
    }

    @Override
    void writeMessage(final int fieldNumber, final MessageLite value, Schema schema)
        throws IOException {
      writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      writeMessageNoTag(value, schema);
    }

    @Override
    public void writeMessageSetExtension(final int fieldNumber, final MessageLite value)
        throws IOException {
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_START_GROUP);
      writeUInt32(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber);
      writeMessage(WireFormat.MESSAGE_SET_MESSAGE, value);
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_END_GROUP);
    }

    @Override
    public void writeRawMessageSetExtension(final int fieldNumber, final ByteString value)
        throws IOException {
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_START_GROUP);
      writeUInt32(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber);
      writeBytes(WireFormat.MESSAGE_SET_MESSAGE, value);
      writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_END_GROUP);
    }

    @Override
    public void writeMessageNoTag(final MessageLite value) throws IOException {
      writeUInt32NoTag(value.getSerializedSize());
      value.writeTo(this);
    }

    @Override
    void writeMessageNoTag(final MessageLite value, Schema schema) throws IOException {
      writeUInt32NoTag(((AbstractMessageLite) value).getSerializedSize(schema));
      schema.writeTo(value, wrapper);
    }

    @Override
    public void write(byte value) throws IOException {
      if (position == limit) {
        doFlush();
      }

      buffer(value);
    }

    @Override
    public void writeInt32NoTag(int value) throws IOException {
      if (value >= 0) {
        writeUInt32NoTag(value);
      } else {
        // Must sign-extend.
        writeUInt64NoTag(value);
      }
    }

    @Override
    public void writeUInt32NoTag(int value) throws IOException {
      flushIfNotAvailable(MAX_VARINT32_SIZE);
      bufferUInt32NoTag(value);
    }

    @Override
    public void writeFixed32NoTag(final int value) throws IOException {
      flushIfNotAvailable(FIXED32_SIZE);
      bufferFixed32NoTag(value);
    }

    @Override
    public void writeUInt64NoTag(long value) throws IOException {
      flushIfNotAvailable(MAX_VARINT_SIZE);
      bufferUInt64NoTag(value);
    }

    @Override
    public void writeFixed64NoTag(final long value) throws IOException {
      flushIfNotAvailable(FIXED64_SIZE);
      bufferFixed64NoTag(value);
    }

    @Override
    public void writeStringNoTag(String value) throws IOException {
      try {
        // UTF-8 byte length of the string is at least its UTF-16 code unit length (value.length()),
        // and at most 3 times of it. We take advantage of this in both branches below.
        final int maxLength = value.length() * Utf8.MAX_BYTES_PER_CHAR;
        final int maxLengthVarIntSize = computeUInt32SizeNoTag(maxLength);

        // If we are streaming and the potential length is too big to fit in our buffer, we take the
        // slower path.
        if (maxLengthVarIntSize + maxLength > limit) {
          // Allocate a byte[] that we know can fit the string and encode into it. String.getBytes()
          // does the same internally and then does *another copy* to return a byte[] of exactly the
          // right size. We can skip that copy and just writeRawBytes up to the actualLength of the
          // UTF-8 encoded bytes.
          final byte[] encodedBytes = new byte[maxLength];
          int actualLength = Utf8.encode(value, encodedBytes, 0, maxLength);
          writeUInt32NoTag(actualLength);
          writeLazy(encodedBytes, 0, actualLength);
          return;
        }

        // Fast path: we have enough space available in our buffer for the string...
        if (maxLengthVarIntSize + maxLength > limit - position) {
          // Flush to free up space.
          doFlush();
        }

        // Optimize for the case where we know this length results in a constant varint length as
        // this saves a pass for measuring the length of the string.
        final int minLengthVarIntSize = computeUInt32SizeNoTag(value.length());
        int oldPosition = position;
        final int length;
        try {
          if (minLengthVarIntSize == maxLengthVarIntSize) {
            position = oldPosition + minLengthVarIntSize;
            int newPosition = Utf8.encode(value, buffer, position, limit - position);
            // Since this class is stateful and tracks the position, we rewind and store the
            // state, prepend the length, then reset it back to the end of the string.
            position = oldPosition;
            length = newPosition - oldPosition - minLengthVarIntSize;
            bufferUInt32NoTag(length);
            position = newPosition;
          } else {
            length = Utf8.encodedLength(value);
            bufferUInt32NoTag(length);
            position = Utf8.encode(value, buffer, position, length);
          }
          totalBytesWritten += length;
        } catch (UnpairedSurrogateException e) {
          // Be extra careful and restore the original position for retrying the write with the
          // less efficient path.
          totalBytesWritten -= position - oldPosition;
          position = oldPosition;
          throw e;
        } catch (ArrayIndexOutOfBoundsException e) {
          throw new OutOfSpaceException(e);
        }
      } catch (UnpairedSurrogateException e) {
        inefficientWriteStringNoTag(value, e);
      }
    }

    @Override
    public void flush() throws IOException {
      if (position > 0) {
        // Flush the buffer.
        doFlush();
      }
    }

    @Override
    public void write(byte[] value, int offset, int length) throws IOException {
      if (limit - position >= length) {
        // We have room in the current buffer.
        System.arraycopy(value, offset, buffer, position, length);
        position += length;
        totalBytesWritten += length;
      } else {
        // Write extends past current buffer.  Fill the rest of this buffer and
        // flush.
        final int bytesWritten = limit - position;
        System.arraycopy(value, offset, buffer, position, bytesWritten);
        offset += bytesWritten;
        length -= bytesWritten;
        position = limit;
        totalBytesWritten += bytesWritten;
        doFlush();

        // Now deal with the rest.
        // Since we have an output stream, this is our buffer
        // and buffer offset == 0
        if (length <= limit) {
          // Fits in new buffer.
          System.arraycopy(value, offset, buffer, 0, length);
          position = length;
        } else {
          // Write is very big.  Let's do it all at once.
          out.write(value, offset, length);
        }
        totalBytesWritten += length;
      }
    }

    @Override
    public void writeLazy(byte[] value, int offset, int length) throws IOException {
      write(value, offset, length);
    }

    @Override
    public void write(ByteBuffer value) throws IOException {
      int length = value.remaining();
      if (limit - position >= length) {
        // We have room in the current buffer.
        value.get(buffer, position, length);
        position += length;
        totalBytesWritten += length;
      } else {
        // Write extends past current buffer.  Fill the rest of this buffer and
        // flush.
        final int bytesWritten = limit - position;
        value.get(buffer, position, bytesWritten);
        length -= bytesWritten;
        position = limit;
        totalBytesWritten += bytesWritten;
        doFlush();

        // Now deal with the rest.
        // Since we have an output stream, this is our buffer
        // and buffer offset == 0
        while (length > limit) {
          // Copy data into the buffer before writing it to OutputStream.
          value.get(buffer, 0, limit);
          out.write(buffer, 0, limit);
          length -= limit;
          totalBytesWritten += limit;
        }
        value.get(buffer, 0, length);
        position = length;
        totalBytesWritten += length;
      }
    }

    @Override
    public void writeLazy(ByteBuffer value) throws IOException {
      write(value);
    }

    private void flushIfNotAvailable(int requiredSize) throws IOException {
      if (limit - position < requiredSize) {
        doFlush();
      }
    }

    private void doFlush() throws IOException {
      out.write(buffer, 0, position);
      position = 0;
    }
  }
}
