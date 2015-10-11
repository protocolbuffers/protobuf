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

import com.google.protobuf.Utf8.UnpairedSurrogateException;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Encodes and writes protocol message fields.
 *
 * <p>This class contains two kinds of methods:  methods that write specific
 * protocol message constructs and field types (e.g. {@link #writeTag} and
 * {@link #writeInt32}) and methods that write low-level values (e.g.
 * {@link #writeRawVarint32} and {@link #writeRawBytes}).  If you are
 * writing encoded protocol messages, you should use the former methods, but if
 * you are writing some other format of your own design, use the latter.
 *
 * <p>This class is totally unsynchronized.
 *
 * @author kneton@google.com Kenton Varda
 */
public final class CodedOutputStream implements Encoder {
  
  private static final Logger logger = Logger.getLogger(CodedOutputStream.class.getName());

  // TODO(dweis): Consider migrating to a ByteBuffer.
  private final byte[] buffer;
  private final int limit;
  private int position;
  private int totalBytesWritten = 0;

  private final OutputStream output;

  /**
   * The buffer size used in {@link #newInstance(OutputStream)}.
   */
  public static final int DEFAULT_BUFFER_SIZE = 4096;

  /**
   * Returns the buffer size to efficiently write dataLength bytes to this
   * CodedOutputStream. Used by AbstractMessageLite.
   *
   * @return the buffer size to efficiently write dataLength bytes to this
   *         CodedOutputStream.
   */
  static int computePreferredBufferSize(int dataLength) {
    if (dataLength > DEFAULT_BUFFER_SIZE) return DEFAULT_BUFFER_SIZE;
    return dataLength;
  }

  private CodedOutputStream(final byte[] buffer, final int offset,
                            final int length) {
    output = null;
    this.buffer = buffer;
    position = offset;
    limit = offset + length;
  }

  private CodedOutputStream(final OutputStream output, final byte[] buffer) {
    this.output = output;
    this.buffer = buffer;
    position = 0;
    limit = buffer.length;
  }

  /**
   * Create a new {@code CodedOutputStream} wrapping the given
   * {@code OutputStream}.
   */
  public static CodedOutputStream newInstance(final OutputStream output) {
    return newInstance(output, DEFAULT_BUFFER_SIZE);
  }

  /**
   * Create a new {@code CodedOutputStream} wrapping the given
   * {@code OutputStream} with a given buffer size.
   */
  public static CodedOutputStream newInstance(final OutputStream output,
      final int bufferSize) {
    return new CodedOutputStream(output, new byte[bufferSize]);
  }

  /**
   * Create a new {@code CodedOutputStream} that writes directly to the given
   * byte array.  If more bytes are written than fit in the array,
   * {@link OutOfSpaceException} will be thrown.  Writing directly to a flat
   * array is faster than writing to an {@code OutputStream}.  See also
   * {@link ByteString#newCodedBuilder}.
   */
  public static CodedOutputStream newInstance(final byte[] flatArray) {
    return newInstance(flatArray, 0, flatArray.length);
  }

  /**
   * Create a new {@code CodedOutputStream} that writes directly to the given
   * byte array slice.  If more bytes are written than fit in the slice,
   * {@link OutOfSpaceException} will be thrown.  Writing directly to a flat
   * array is faster than writing to an {@code OutputStream}.  See also
   * {@link ByteString#newCodedBuilder}.
   */
  public static CodedOutputStream newInstance(final byte[] flatArray,
                                              final int offset,
                                              final int length) {
    return new CodedOutputStream(flatArray, offset, length);
  }

  /**
   * Create a new {@code CodedOutputStream} that writes to the given ByteBuffer.
   */
  public static CodedOutputStream newInstance(ByteBuffer byteBuffer) {
    return newInstance(byteBuffer, DEFAULT_BUFFER_SIZE);
  }

  /**
   * Create a new {@code CodedOutputStream} that writes to the given ByteBuffer.
   */
  public static CodedOutputStream newInstance(ByteBuffer byteBuffer,
      int bufferSize) {
    return newInstance(new ByteBufferOutputStream(byteBuffer), bufferSize);
  }

  private static class ByteBufferOutputStream extends OutputStream {
    private final ByteBuffer byteBuffer;
    public ByteBufferOutputStream(ByteBuffer byteBuffer) {
      this.byteBuffer = byteBuffer;
    }

    @Override
    public void write(int b) throws IOException {
      byteBuffer.put((byte) b);
    }

    @Override
    public void write(byte[] data, int offset, int length) throws IOException {
      byteBuffer.put(data, offset, length);
    }
  }

  // -----------------------------------------------------------------

  /** Write a {@code double} field, including tag, to the stream. */
  @Override
  public void writeDouble(final int fieldNumber, final double value)
                          throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
    writeDoubleNoTag(value);
  }

  /** Write a {@code float} field, including tag, to the stream. */
  @Override
  public void writeFloat(final int fieldNumber, final float value)
                         throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
    writeFloatNoTag(value);
  }

  /** Write a {@code uint64} field, including tag, to the stream. */
  @Override
  public void writeUInt64(final int fieldNumber, final long value)
                          throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeUInt64NoTag(value);
  }

  /** Write an {@code int64} field, including tag, to the stream. */
  @Override
  public void writeInt64(final int fieldNumber, final long value)
                         throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeInt64NoTag(value);
  }

  /** Write an {@code int32} field, including tag, to the stream. */
  @Override
  public void writeInt32(final int fieldNumber, final int value)
                         throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeInt32NoTag(value);
  }

  /** Write a {@code fixed64} field, including tag, to the stream. */
  @Override
  public void writeFixed64(final int fieldNumber, final long value)
                           throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
    writeFixed64NoTag(value);
  }

  /** Write a {@code fixed32} field, including tag, to the stream. */
  @Override
  public void writeFixed32(final int fieldNumber, final int value)
                           throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
    writeFixed32NoTag(value);
  }

  /** Write a {@code bool} field, including tag, to the stream. */
  @Override
  public void writeBool(final int fieldNumber, final boolean value)
                        throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeBoolNoTag(value);
  }

  /** Write a {@code string} field, including tag, to the stream. */
  @Override
  public void writeString(final int fieldNumber, final String value)
                          throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    writeStringNoTag(value);
  }

  /** Write a {@code group} field, including tag, to the stream. */
  @Override
  public void writeGroup(final int fieldNumber, final MessageLite value)
                         throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_START_GROUP);
    writeGroupNoTag(value);
    writeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP);
  }


  /**
   * Write a group represented by an {@link UnknownFieldSet}.
   *
   * @deprecated UnknownFieldSet now implements MessageLite, so you can just
   *             call {@link #writeGroup}.
   */
  @Deprecated
  public void writeUnknownGroup(final int fieldNumber,
                                final MessageLite value)
                                throws IOException {
    writeGroup(fieldNumber, value);
  }

  /** Write an embedded message field, including tag, to the stream. */
  @Override
  public void writeMessage(final int fieldNumber, final MessageLite value)
                           throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    writeMessageNoTag(value);
  }


  /** Write a {@code bytes} field, including tag, to the stream. */
  @Override
  public void writeBytes(final int fieldNumber, final ByteString value)
                         throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    writeBytesNoTag(value);
  }

  /** Write a {@code bytes} field, including tag, to the stream. */
  public void writeByteArray(final int fieldNumber, final byte[] value)
                             throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    writeByteArrayNoTag(value);
  }

  /** Write a {@code bytes} field, including tag, to the stream. */
  @Override
  public void writeByteArray(final int fieldNumber,
                             final byte[] value,
                             final int offset,
                             final int length)
                             throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    writeByteArrayNoTag(value, offset, length);
  }

  /**
   * Write a {@code bytes} field, including tag, to the stream.
   * This method will write all content of the ByteBuffer regardless of the
   * current position and limit (i.e., the number of bytes to be written is
   * value.capacity(), not value.remaining()). Furthermore, this method doesn't
   * alter the state of the passed-in ByteBuffer. Its position, limit, mark,
   * etc. will remain unchanged. If you only want to write the remaining bytes
   * of a ByteBuffer, you can call
   * {@code writeByteBuffer(fieldNumber, byteBuffer.slice())}.
   */
  public void writeByteBuffer(final int fieldNumber, final ByteBuffer value)
      throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    writeByteBufferNoTag(value);
  }

  /** Write a {@code uint32} field, including tag, to the stream. */
  @Override
  public void writeUInt32(final int fieldNumber, final int value)
                          throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeUInt32NoTag(value);
  }

  /**
   * Write an enum field, including tag, to the stream.  Caller is responsible
   * for converting the enum value to its numeric value.
   */
  @Override
  public void writeEnum(final int fieldNumber, final int value)
                        throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeEnumNoTag(value);
  }

  /** Write an {@code sfixed32} field, including tag, to the stream. */
  @Override
  public void writeSFixed32(final int fieldNumber, final int value)
                            throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
    writeSFixed32NoTag(value);
  }

  /** Write an {@code sfixed64} field, including tag, to the stream. */
  @Override
  public void writeSFixed64(final int fieldNumber, final long value)
                            throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
    writeSFixed64NoTag(value);
  }

  /** Write an {@code sint32} field, including tag, to the stream. */
  @Override
  public void writeSInt32(final int fieldNumber, final int value)
                          throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeSInt32NoTag(value);
  }

  /** Write an {@code sint64} field, including tag, to the stream. */
  @Override
  public void writeSInt64(final int fieldNumber, final long value)
                          throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeSInt64NoTag(value);
  }

  /**
   * Write a MessageSet extension field to the stream.  For historical reasons,
   * the wire format differs from normal fields.
   */
  @Override
  public void writeMessageSetExtension(final int fieldNumber,
                                       final MessageLite value)
                                       throws IOException {
    writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_START_GROUP);
    writeUInt32(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber);
    writeMessage(WireFormat.MESSAGE_SET_MESSAGE, value);
    writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_END_GROUP);
  }

  /**
   * Write an unparsed MessageSet extension field to the stream.  For
   * historical reasons, the wire format differs from normal fields.
   */
  @Override
  public void writeRawMessageSetExtension(final int fieldNumber,
                                          final ByteString value)
                                          throws IOException {
    writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_START_GROUP);
    writeUInt32(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber);
    writeBytes(WireFormat.MESSAGE_SET_MESSAGE, value);
    writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_END_GROUP);
  }

  // -----------------------------------------------------------------

  /** Write a {@code double} field to the stream. */
  @Override
  public void writeDoubleNoTag(final double value) throws IOException {
    writeRawLittleEndian64(Double.doubleToRawLongBits(value));
  }

  /** Write a {@code float} field to the stream. */
  @Override
  public void writeFloatNoTag(final float value) throws IOException {
    writeRawLittleEndian32(Float.floatToRawIntBits(value));
  }

  /** Write a {@code uint64} field to the stream. */
  @Override
  public void writeUInt64NoTag(final long value) throws IOException {
    writeRawVarint64(value);
  }

  /** Write an {@code int64} field to the stream. */
  @Override
  public void writeInt64NoTag(final long value) throws IOException {
    writeRawVarint64(value);
  }

  /** Write an {@code int32} field to the stream. */
  @Override
  public void writeInt32NoTag(final int value) throws IOException {
    if (value >= 0) {
      writeRawVarint32(value);
    } else {
      // Must sign-extend.
      writeRawVarint64(value);
    }
  }

  /** Write a {@code fixed64} field to the stream. */
  @Override
  public void writeFixed64NoTag(final long value) throws IOException {
    writeRawLittleEndian64(value);
  }

  /** Write a {@code fixed32} field to the stream. */
  @Override
  public void writeFixed32NoTag(final int value) throws IOException {
    writeRawLittleEndian32(value);
  }

  /** Write a {@code bool} field to the stream. */
  @Override
  public void writeBoolNoTag(final boolean value) throws IOException {
    writeRawByte(value ? 1 : 0);
  }

  /** Write a {@code string} field to the stream. */
  // TODO(dweis): Document behavior on ill-formed UTF-16 input.
  @Override
  public void writeStringNoTag(final String value) throws IOException {
    try {
      efficientWriteStringNoTag(value);
    } catch (UnpairedSurrogateException e) {
      logger.log(Level.WARNING, 
          "Converting ill-formed UTF-16. Your Protocol Buffer will not round trip correctly!", e);
      inefficientWriteStringNoTag(value);
    }
  }

  /** Write a {@code string} field to the stream. */
  private void inefficientWriteStringNoTag(final String value) throws IOException {
    // Unfortunately there does not appear to be any way to tell Java to encode
    // UTF-8 directly into our buffer, so we have to let it create its own byte
    // array and then copy.
    // TODO(dweis): Consider using nio Charset methods instead.
    final byte[] bytes = value.getBytes(Internal.UTF_8);
    writeRawVarint32(bytes.length);
    writeRawBytes(bytes);
  }

  /**
   * Write a {@code string} field to the stream efficiently. If the {@code string} is malformed,
   * this method rolls back its changes and throws an {@link UnpairedSurrogateException} with the
   * intent that the caller will catch and retry with {@link #inefficientWriteStringNoTag(String)}.
   * 
   * @param value the string to write to the stream
   * 
   * @throws UnpairedSurrogateException when {@code value} is ill-formed UTF-16. 
   */
  private void efficientWriteStringNoTag(final String value) throws IOException {
    // UTF-8 byte length of the string is at least its UTF-16 code unit length (value.length()),
    // and at most 3 times of it. We take advantage of this in both branches below.
    final int maxLength = value.length() * Utf8.MAX_BYTES_PER_CHAR;
    final int maxLengthVarIntSize = WireFormat.computeRawVarint32Size(maxLength);

    // If we are streaming and the potential length is too big to fit in our buffer, we take the
    // slower path. Otherwise, we're good to try the fast path.
    if (output != null && maxLengthVarIntSize + maxLength > limit - position) {
      // Allocate a byte[] that we know can fit the string and encode into it. String.getBytes()
      // does the same internally and then does *another copy* to return a byte[] of exactly the
      // right size. We can skip that copy and just writeRawBytes up to the actualLength of the
      // UTF-8 encoded bytes.
      final byte[] encodedBytes = new byte[maxLength];
      int actualLength = Utf8.encode(value, encodedBytes, 0, maxLength);
      writeRawVarint32(actualLength);
      writeRawBytes(encodedBytes, 0, actualLength);
    } else {
      // Optimize for the case where we know this length results in a constant varint length as this
      // saves a pass for measuring the length of the string.
      final int minLengthVarIntSize = WireFormat.computeRawVarint32Size(value.length());
      int oldPosition = position;
      final int length;
      try {
        if (minLengthVarIntSize == maxLengthVarIntSize) {
          position = oldPosition + minLengthVarIntSize;
          int newPosition = Utf8.encode(value, buffer, position, limit - position);
          // Since this class is stateful and tracks the position, we rewind and store the state,
          // prepend the length, then reset it back to the end of the string.
          position = oldPosition;
          length = newPosition - oldPosition - minLengthVarIntSize;
          writeRawVarint32(length);
          position = newPosition;
        } else {
          length = Utf8.encodedLength(value);
          writeRawVarint32(length);
          position = Utf8.encode(value, buffer, position, limit - position);
        }
      } catch (UnpairedSurrogateException e) {
        // Be extra careful and restore the original position for retrying the write with the less
        // efficient path.
        position = oldPosition;
        throw e;
      } catch (ArrayIndexOutOfBoundsException e) {
        throw new OutOfSpaceException(e);
      }
      totalBytesWritten += length;
    }
  }

  /** Write a {@code group} field to the stream. */
  @Override
  public void writeGroupNoTag(final MessageLite value) throws IOException {
    value.writeTo(this);
  }


  /**
   * Write a group represented by an {@link UnknownFieldSet}.
   *
   * @deprecated UnknownFieldSet now implements MessageLite, so you can just
   *             call {@link #writeGroupNoTag}.
   */
  @Deprecated
  public void writeUnknownGroupNoTag(final MessageLite value)
      throws IOException {
    writeGroupNoTag(value);
  }

  /** Write an embedded message field to the stream. */
  @Override
  public void writeMessageNoTag(final MessageLite value) throws IOException {
    writeRawVarint32(value.getSerializedSize());
    value.writeTo(this);
  }


  /** Write a {@code bytes} field to the stream. */
  @Override
  public void writeBytesNoTag(final ByteString value) throws IOException {
    writeRawVarint32(value.size());
    writeRawBytes(value);
  }

  /** Write a {@code bytes} field to the stream. */
  public void writeByteArrayNoTag(final byte[] value) throws IOException {
    writeRawVarint32(value.length);
    writeRawBytes(value);
  }

  /** Write a {@code bytes} field to the stream. */
  @Override
  public void writeByteArrayNoTag(final byte[] value,
                                  final int offset,
                                  final int length) throws IOException {
    writeRawVarint32(length);
    writeRawBytes(value, offset, length);
  }

  /**
   * Write a {@code bytes} field to the stream.  This method will write all
   * content of the ByteBuffer regardless of the current position and limit
   * (i.e., the number of bytes to be written is value.capacity(), not
   * value.remaining()). Furthermore, this method doesn't alter the state of
   * the passed-in ByteBuffer. Its position, limit, mark, etc. will remain
   * unchanged. If you only want to write the remaining bytes of a ByteBuffer,
   * you can call {@code writeByteBufferNoTag(byteBuffer.slice())}.
   */
  public void writeByteBufferNoTag(final ByteBuffer value) throws IOException {
    writeRawVarint32(value.capacity());
    writeRawBytes(value);
  }

  /** Write a {@code uint32} field to the stream. */
  @Override
  public void writeUInt32NoTag(final int value) throws IOException {
    writeRawVarint32(value);
  }

  /**
   * Write an enum field to the stream.  Caller is responsible
   * for converting the enum value to its numeric value.
   */
  @Override
  public void writeEnumNoTag(final int value) throws IOException {
    writeInt32NoTag(value);
  }

  /** Write an {@code sfixed32} field to the stream. */
  @Override
  public void writeSFixed32NoTag(final int value) throws IOException {
    writeRawLittleEndian32(value);
  }

  /** Write an {@code sfixed64} field to the stream. */
  @Override
  public void writeSFixed64NoTag(final long value) throws IOException {
    writeRawLittleEndian64(value);
  }

  /** Write an {@code sint32} field to the stream. */
  @Override
  public void writeSInt32NoTag(final int value) throws IOException {
    writeRawVarint32(WireFormat.encodeZigZag32(value));
  }

  /** Write an {@code sint64} field to the stream. */
  @Override
  public void writeSInt64NoTag(final long value) throws IOException {
    writeRawVarint64(WireFormat.encodeZigZag64(value));
  }

  // =================================================================

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code double} field, including tag.
   * @deprecated Use {@link WireFormat#computeDoubleSize(int, double)} instead.
   */
  @Deprecated
  public static int computeDoubleSize(final int fieldNumber,
                                      final double value) {
    return WireFormat.computeDoubleSize(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code float} field, including tag.
   * @deprecated Use {@link WireFormat#computeFloatSize(int, float)} instead.
   */
  @Deprecated
  public static int computeFloatSize(final int fieldNumber, final float value) {
    return WireFormat.computeFloatSize(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code uint64} field, including tag.
   * @deprecated Use {@link WireFormat#computeUInt64Size(int, long)} instead.
   */
  @Deprecated
  public static int computeUInt64Size(final int fieldNumber, final long value) {
    return WireFormat.computeUInt64Size(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code int64} field, including tag.
   * @deprecated Use {@link WireFormat#computeInt64Size(int, long)} instead.
   */
  @Deprecated
  public static int computeInt64Size(final int fieldNumber, final long value) {
    return WireFormat.computeInt64Size(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code int32} field, including tag.
   * @deprecated Use {@link WireFormat#computeInt32Size(int, int)} instead.
   */
  @Deprecated
  public static int computeInt32Size(final int fieldNumber, final int value) {
    return WireFormat.computeInt32Size(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code fixed64} field, including tag.
   * @deprecated Use {@link WireFormat#computeFixed64Size(int, long)} instead.
   */
  @Deprecated
  public static int computeFixed64Size(final int fieldNumber,
                                       final long value) {
    return WireFormat.computeFixed64Size(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code fixed32} field, including tag.
   * @deprecated Use {@link WireFormat#computeFixed32Size(int, int)} instead.
   */
  @Deprecated
  public static int computeFixed32Size(final int fieldNumber,
                                       final int value) {
    return WireFormat.computeFixed32Size(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bool} field, including tag.
   * @deprecated Use {@link WireFormat#computeBoolSize(int, boolean)} instead.
   */
  @Deprecated
  public static int computeBoolSize(final int fieldNumber,
                                    final boolean value) {
    return WireFormat.computeBoolSize(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code string} field, including tag.
   * @deprecated Use {@link WireFormat#computeStringSize(int, String)} instead.
   */
  @Deprecated
  public static int computeStringSize(final int fieldNumber,
                                      final String value) {
    return WireFormat.computeStringSize(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code group} field, including tag.
   * @deprecated Use {@link WireFormat#computeGroupSize(int, MessageLite)} instead.
   */
  @Deprecated
  public static int computeGroupSize(final int fieldNumber,
                                     final MessageLite value) {
    return WireFormat.computeGroupSize(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code group} field represented by an {@code UnknownFieldSet}, including
   * tag.
   *
   * @deprecated UnknownFieldSet now implements MessageLite, so you can just
   *             call {@link #computeGroupSize}.
   */
  @Deprecated
  public static int computeUnknownGroupSize(final int fieldNumber,
                                            final MessageLite value) {
    return WireFormat.computeGroupSize(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * embedded message field, including tag.
   * @deprecated Use {@link WireFormat#computeMessageSize(int, MessageLite)} instead.
   */
  @Deprecated
  public static int computeMessageSize(final int fieldNumber,
                                       final MessageLite value) {
    return WireFormat.computeMessageSize(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bytes} field, including tag.
   * @deprecated Use {@link WireFormat#computeBytesSize(int, ByteString)} instead.
   */
  @Deprecated
  public static int computeBytesSize(final int fieldNumber,
                                     final ByteString value) {
    return WireFormat.computeBytesSize(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bytes} field, including tag.
   * @deprecated Use {@link WireFormat#computeByteArraySize(int, byte[])} instead.
   */
  @Deprecated
  public static int computeByteArraySize(final int fieldNumber,
                                         final byte[] value) {
    return WireFormat.computeTagSize(fieldNumber) + computeByteArraySizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bytes} field, including tag.
   */
  public static int computeByteBufferSize(final int fieldNumber,
                                         final ByteBuffer value) {
    return WireFormat.computeTagSize(fieldNumber) + computeByteBufferSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * embedded message in lazy field, including tag.
   * @deprecated Use {@link WireFormat#computeLazyFieldSize(int, LazyFieldLite)} instead.
   */
  @Deprecated
  public static int computeLazyFieldSize(final int fieldNumber,
                                         final LazyFieldLite value) {
    return WireFormat.computeLazyFieldSize(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code uint32} field, including tag.
   * @deprecated Use {@link WireFormat#computeUInt32Size(int, int)} instead.
   */
  @Deprecated
  public static int computeUInt32Size(final int fieldNumber, final int value) {
    return WireFormat.computeUInt32Size(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * enum field, including tag.  Caller is responsible for converting the
   * enum value to its numeric value.
   * @deprecated Use {@link WireFormat#computeEnumSize(int, int)} instead.
   */
  @Deprecated
  public static int computeEnumSize(final int fieldNumber, final int value) {
    return WireFormat.computeEnumSize(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sfixed32} field, including tag.
   * @deprecated Use {@link WireFormat#computeSFixed32Size(int, int)} instead.
   */
  @Deprecated
  public static int computeSFixed32Size(final int fieldNumber,
                                        final int value) {
    return WireFormat.computeSFixed32Size(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sfixed64} field, including tag.
   * @deprecated Use {@link WireFormat#computeSFixed64Size(int, long)} instead.
   */
  @Deprecated
  public static int computeSFixed64Size(final int fieldNumber,
                                        final long value) {
    return WireFormat.computeSFixed64Size(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sint32} field, including tag.
   * @deprecated Use {@link WireFormat#computeSInt32Size(int, int)} instead.
   */
  @Deprecated
  public static int computeSInt32Size(final int fieldNumber, final int value) {
    return WireFormat.computeSInt32Size(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sint64} field, including tag.
   * @deprecated Use {@link WireFormat#computeSInt64Size(int, long)} instead.
   */
  @Deprecated
  public static int computeSInt64Size(final int fieldNumber, final long value) {
    return WireFormat.computeSInt64Size(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * MessageSet extension to the stream.  For historical reasons,
   * the wire format differs from normal fields.
   * @deprecated Use {@link WireFormat#computeMessageSetExtensionSize(int, MessageLite)} instead.
   */
  @Deprecated
  public static int computeMessageSetExtensionSize(
      final int fieldNumber, final MessageLite value) {
    return WireFormat.computeMessageSetExtensionSize(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * unparsed MessageSet extension field to the stream.  For
   * historical reasons, the wire format differs from normal fields.
   * @deprecated Use {@link WireFormat#computeRawMessageSetExtensionSize(int, ByteString)} instead.
   */
  @Deprecated
  public static int computeRawMessageSetExtensionSize(
      final int fieldNumber, final ByteString value) {
    return WireFormat.computeRawMessageSetExtensionSize(fieldNumber, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * lazily parsed MessageSet extension field to the stream.  For
   * historical reasons, the wire format differs from normal fields.
   * @deprecated Use {@link WireFormat#computeLazyFieldMessageSetExtensionSize(int, LazyFieldLite)}
   * instead.
   */
  @Deprecated
  public static int computeLazyFieldMessageSetExtensionSize(
      final int fieldNumber, final LazyFieldLite value) {
    return WireFormat.computeLazyFieldMessageSetExtensionSize(fieldNumber, value);
  }

  // -----------------------------------------------------------------

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code double} field, including tag.
   * @deprecated Use {@link WireFormat#computeDoubleSizeNoTag(double)} instead.
   */
  @Deprecated
  public static int computeDoubleSizeNoTag(final double value) {
    return WireFormat.LITTLE_ENDIAN_64_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code float} field, including tag.
   * @deprecated Use {@link WireFormat#computeFloatSizeNoTag(float)} instead.
   */
  @Deprecated
  public static int computeFloatSizeNoTag(@SuppressWarnings("unused") final float value) {
    return WireFormat.LITTLE_ENDIAN_32_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code uint64} field, including tag.
   * @deprecated Use {@link WireFormat#computeUInt64SizeNoTag(long)} instead.
   */
  @Deprecated
  public static int computeUInt64SizeNoTag(final long value) {
    return WireFormat.computeUInt64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code int64} field, including tag.
   * @deprecated Use {@link WireFormat#computeInt64SizeNoTag(long)} instead.
   */
  @Deprecated
  public static int computeInt64SizeNoTag(final long value) {
    return WireFormat.computeInt64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code int32} field, including tag.
   * @deprecated Use {@link WireFormat#computeInt32SizeNoTag(int)} instead.
   */
  @Deprecated
  public static int computeInt32SizeNoTag(final int value) {
    return WireFormat.computeInt32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code fixed64} field.
   * @deprecated Use {@link WireFormat#computeFixed64SizeNoTag(long)} instead.
   */
  @Deprecated
  public static int computeFixed64SizeNoTag(final long value) {
    return WireFormat.computeFixed64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code fixed32} field.
   * @deprecated Use {@link WireFormat#computeFixed32SizeNoTag(int)} instead.
   */
  @Deprecated
  public static int computeFixed32SizeNoTag(final int value) {
    return WireFormat.computeFixed32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bool} field.
   * @deprecated Use {@link WireFormat#computeBoolSizeNoTag(boolean)} instead.
   */
  @Deprecated
  public static int computeBoolSizeNoTag(final boolean value) {
    return WireFormat.computeBoolSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code string} field.
   * @deprecated Use {@link WireFormat#computeStringSizeNoTag(String)} instead.
   */
  @Deprecated
  public static int computeStringSizeNoTag(final String value) {
    return WireFormat.computeStringSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code group} field.
   * @deprecated Use {@link WireFormat#computeGroupSizeNoTag(MessageLite)} instead.
   */
  @Deprecated
  public static int computeGroupSizeNoTag(final MessageLite value) {
    return WireFormat.computeGroupSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code group} field represented by an {@code UnknownFieldSet}, including
   * tag.
   *
   * @deprecated UnknownFieldSet now implements MessageLite, so you can just
   *             call {@link WireFormat#computeGroupSizeNoTag}.
   */
  @Deprecated
  public static int computeUnknownGroupSizeNoTag(final MessageLite value) {
    return WireFormat.computeGroupSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an embedded
   * message field.
   * @deprecated Use {@link WireFormat#computeMessageSizeNoTag(MessageLite)} instead.
   */
  @Deprecated
  public static int computeMessageSizeNoTag(final MessageLite value) {
    return WireFormat.computeMessageSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an embedded
   * message stored in lazy field.
   * @deprecated Use {@link WireFormat#computeLazyFieldSizeNoTag(LazyFieldLite)} instead.
   */
  @Deprecated
  public static int computeLazyFieldSizeNoTag(final LazyFieldLite value) {
    return WireFormat.computeLazyFieldSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bytes} field.
   * @deprecated Use {@link WireFormat#computeBytesSizeNoTag(ByteString)} instead.
   */
  @Deprecated
  public static int computeBytesSizeNoTag(final ByteString value) {
    return WireFormat.computeBytesSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bytes} field.
   * @deprecated Use {@link WireFormat#computeByteArraySizeNoTag(byte[])} instead.
   */
  @Deprecated
  public static int computeByteArraySizeNoTag(final byte[] value) {
    return WireFormat.computeRawVarint32Size(value.length) + value.length;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bytes} field.
   */
  public static int computeByteBufferSizeNoTag(final ByteBuffer value) {
    return WireFormat.computeRawVarint32Size(value.capacity()) + value.capacity();
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code uint32} field.
   * @deprecated Use {@link WireFormat#computeUInt32SizeNoTag(int)} instead.
   */
  @Deprecated
  public static int computeUInt32SizeNoTag(final int value) {
    return WireFormat.computeUInt32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an enum field.
   * Caller is responsible for converting the enum value to its numeric value.
   * @deprecated Use {@link WireFormat#computeEnumSizeNoTag(int)} instead.
   */
  @Deprecated
  public static int computeEnumSizeNoTag(final int value) {
    return WireFormat.computeEnumSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sfixed32} field.
   * @deprecated Use {@link WireFormat#computeSFixed32SizeNoTag(int)} instead.
   */
  @Deprecated
  public static int computeSFixed32SizeNoTag(final int value) {
    return WireFormat.computeSFixed32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sfixed64} field.
   * @deprecated Use {@link WireFormat#computeSFixed64SizeNoTag(long)} instead.
   */
  @Deprecated
  public static int computeSFixed64SizeNoTag(final long value) {
    return WireFormat.computeSFixed64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sint32} field.
   * @deprecated Use {@link WireFormat#computeSInt32SizeNoTag(int)} instead.
   */
  @Deprecated
  public static int computeSInt32SizeNoTag(final int value) {
    return WireFormat.computeSInt32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sint64} field.
   * @deprecated Use {@link WireFormat#computeSInt64SizeNoTag(long)} instead.
   */
  @Deprecated
  public static int computeSInt64SizeNoTag(final long value) {
    return WireFormat.computeSInt64SizeNoTag(value);
  }

  // =================================================================

  /**
   * Internal helper that writes the current buffer to the output. The
   * buffer position is reset to its initial value when this returns.
   */
  private void refreshBuffer() throws IOException {
    if (output == null) {
      // We're writing to a single buffer.
      throw new OutOfSpaceException();
    }

    // Since we have an output stream, this is our buffer
    // and buffer offset == 0
    output.write(buffer, 0, position);
    position = 0;
  }

  /**
   * Flushes the stream and forces any buffered bytes to be written.  This
   * does not flush the underlying OutputStream.
   */
  @Override
  public void flush() throws IOException {
    if (output != null) {
      refreshBuffer();
    }
  }

  /**
   * If writing to a flat array, return the space left in the array.
   * Otherwise, throws {@code UnsupportedOperationException}.
   */
  public int spaceLeft() {
    if (output == null) {
      return limit - position;
    } else {
      throw new UnsupportedOperationException(
        "spaceLeft() can only be called on CodedOutputStreams that are " +
        "writing to a flat array.");
    }
  }

  /**
   * Verifies that {@link #spaceLeft()} returns zero.  It's common to create
   * a byte array that is exactly big enough to hold a message, then write to
   * it with a {@code CodedOutputStream}.  Calling {@code checkNoSpaceLeft()}
   * after writing verifies that the message was actually as big as expected,
   * which can help catch bugs.
   */
  public void checkNoSpaceLeft() {
    if (spaceLeft() != 0) {
      throw new IllegalStateException(
        "Did not write as much data as expected.");
    }
  }

  /**
   * If you create a CodedOutputStream around a simple flat array, you must
   * not attempt to write more bytes than the array has space.  Otherwise,
   * this exception will be thrown.
   */
  public static class OutOfSpaceException extends IOException {
    private static final long serialVersionUID = -6947486886997889499L;

    private static final String MESSAGE =
        "CodedOutputStream was writing to a flat byte array and ran out of space.";

    OutOfSpaceException() {
      super(MESSAGE);
    }

    OutOfSpaceException(Throwable cause) {
      super(MESSAGE, cause);
    }
  }

  /**
   * Get the total number of bytes successfully written to this stream.  The
   * returned value is not guaranteed to be accurate if exceptions have been
   * found in the middle of writing.
   */
  public int getTotalBytesWritten() {
    return totalBytesWritten;
  }

  /** Write a single byte. */
  public void writeRawByte(final byte value) throws IOException {
    if (position == limit) {
      refreshBuffer();
    }

    buffer[position++] = value;
    ++totalBytesWritten;
  }

  /** Write a single byte, represented by an integer value. */
  public void writeRawByte(final int value) throws IOException {
    writeRawByte((byte) value);
  }

  /** Write a byte string. */
  public void writeRawBytes(final ByteString value) throws IOException {
    writeRawBytes(value, 0, value.size());
  }

  /** Write an array of bytes. */
  public void writeRawBytes(final byte[] value) throws IOException {
    writeRawBytes(value, 0, value.length);
  }

  /**
   * Write a ByteBuffer. This method will write all content of the ByteBuffer
   * regardless of the current position and limit (i.e., the number of bytes
   * to be written is value.capacity(), not value.remaining()). Furthermore,
   * this method doesn't alter the state of the passed-in ByteBuffer. Its
   * position, limit, mark, etc. will remain unchanged. If you only want to
   * write the remaining bytes of a ByteBuffer, you can call
   * {@code writeRawBytes(byteBuffer.slice())}.
   */
  public void writeRawBytes(final ByteBuffer value) throws IOException {
    if (value.hasArray()) {
      writeRawBytes(value.array(), value.arrayOffset(), value.capacity());
    } else {
      ByteBuffer duplicated = value.duplicate();
      duplicated.clear();
      writeRawBytesInternal(duplicated);
    }
  }

  /** Write a ByteBuffer that isn't backed by an array. */
  private void writeRawBytesInternal(final ByteBuffer value)
      throws IOException {
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
      refreshBuffer();

      // Now deal with the rest.
      // Since we have an output stream, this is our buffer
      // and buffer offset == 0
      while (length > limit) {
        // Copy data into the buffer before writing it to OutputStream.
        // TODO(xiaofeng): Introduce ZeroCopyOutputStream to avoid this copy.
        value.get(buffer, 0, limit);
        output.write(buffer, 0, limit);
        length -= limit;
        totalBytesWritten += limit;
      }
      value.get(buffer, 0, length);
      position = length;
      totalBytesWritten += length;
    }
  }

  /** Write part of an array of bytes. */
  public void writeRawBytes(final byte[] value, int offset, int length)
                            throws IOException {
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
      refreshBuffer();

      // Now deal with the rest.
      // Since we have an output stream, this is our buffer
      // and buffer offset == 0
      if (length <= limit) {
        // Fits in new buffer.
        System.arraycopy(value, offset, buffer, 0, length);
        position = length;
      } else {
        // Write is very big.  Let's do it all at once.
        output.write(value, offset, length);
      }
      totalBytesWritten += length;
    }
  }

  /** Write part of a byte string. */
  public void writeRawBytes(final ByteString value, int offset, int length)
                            throws IOException {
    if (limit - position >= length) {
      // We have room in the current buffer.
      value.copyTo(buffer, offset, position, length);
      position += length;
      totalBytesWritten += length;
    } else {
      // Write extends past current buffer.  Fill the rest of this buffer and
      // flush.
      final int bytesWritten = limit - position;
      value.copyTo(buffer, offset, position, bytesWritten);
      offset += bytesWritten;
      length -= bytesWritten;
      position = limit;
      totalBytesWritten += bytesWritten;
      refreshBuffer();

      // Now deal with the rest.
      // Since we have an output stream, this is our buffer
      // and buffer offset == 0
      if (length <= limit) {
        // Fits in new buffer.
        value.copyTo(buffer, offset, 0, length);
        position = length;
      } else {
        value.writeTo(output, offset, length);
      }
      totalBytesWritten += length;
    }
  }

  /** Encode and write a tag. */
  @Override
  public void writeTag(final int fieldNumber, final int wireType)
                       throws IOException {
    writeRawVarint32(WireFormat.makeTag(fieldNumber, wireType));
  }

  /**
   * Compute the number of bytes that would be needed to encode a tag.
   * @deprecated Use {@link WireFormat#computeTagSize(int)} instead.
   */
  @Deprecated
  public static int computeTagSize(final int fieldNumber) {
    return WireFormat.computeTagSize(fieldNumber);
  }

  /**
   * Encode and write a varint.  {@code value} is treated as
   * unsigned, so it won't be sign-extended if negative.
   */
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

  /**
   * Compute the number of bytes that would be needed to encode a varint.
   * {@code value} is treated as unsigned, so it won't be sign-extended if
   * negative.
   * @deprecated Use {@link WireFormat#computeRawVarint32Size(int)} instead.
   */
  @Deprecated
  public static int computeRawVarint32Size(final int value) {
    return WireFormat.computeRawVarint32Size(value);
  }

  /** Encode and write a varint. */
  public void writeRawVarint64(long value) throws IOException {
    while (true) {
      if ((value & ~0x7FL) == 0) {
        writeRawByte((int)value);
        return;
      } else {
        writeRawByte(((int)value & 0x7F) | 0x80);
        value >>>= 7;
      }
    }
  }

  /**
   * Compute the number of bytes that would be needed to encode a varint.
   * @deprecated Use {@link WireFormat#computeRawVarint64Size(long)} instead.
   */
  @Deprecated
  public static int computeRawVarint64Size(final long value) {
    return WireFormat.computeRawVarint64Size(value);
  }

  /** Write a little-endian 32-bit integer. */
  public void writeRawLittleEndian32(final int value) throws IOException {
    writeRawByte((value      ) & 0xFF);
    writeRawByte((value >>  8) & 0xFF);
    writeRawByte((value >> 16) & 0xFF);
    writeRawByte((value >> 24) & 0xFF);
  }

  /**
   * @deprecated Use {@link WireFormat#LITTLE_ENDIAN_32_SIZE} instead.
   */
  @Deprecated
  public static final int LITTLE_ENDIAN_32_SIZE = WireFormat.LITTLE_ENDIAN_32_SIZE;

  /** Write a little-endian 64-bit integer. */
  public void writeRawLittleEndian64(final long value) throws IOException {
    writeRawByte((int)(value      ) & 0xFF);
    writeRawByte((int)(value >>  8) & 0xFF);
    writeRawByte((int)(value >> 16) & 0xFF);
    writeRawByte((int)(value >> 24) & 0xFF);
    writeRawByte((int)(value >> 32) & 0xFF);
    writeRawByte((int)(value >> 40) & 0xFF);
    writeRawByte((int)(value >> 48) & 0xFF);
    writeRawByte((int)(value >> 56) & 0xFF);
  }

  /**
   * @deprecated Use {@link WireFormat#LITTLE_ENDIAN_64_SIZE} instead.
   */
  @Deprecated
  public static final int LITTLE_ENDIAN_64_SIZE = WireFormat.LITTLE_ENDIAN_64_SIZE;

  /**
   * Encode a ZigZag-encoded 32-bit value.  ZigZag encodes signed integers
   * into values that can be efficiently encoded with varint.  (Otherwise,
   * negative values must be sign-extended to 64 bits to be varint encoded,
   * thus always taking 10 bytes on the wire.)
   *
   * @param n A signed 32-bit integer.
   * @return An unsigned 32-bit integer, stored in a signed int because
   *         Java has no explicit unsigned support.
   * @deprecated Use {@link WireFormat#encodeZigZag32(int)} instead.
   */
  @Deprecated
  public static int encodeZigZag32(final int n) {
    return WireFormat.encodeZigZag32(n);
  }

  /**
   * Encode a ZigZag-encoded 64-bit value.  ZigZag encodes signed integers
   * into values that can be efficiently encoded with varint.  (Otherwise,
   * negative values must be sign-extended to 64 bits to be varint encoded,
   * thus always taking 10 bytes on the wire.)
   *
   * @param n A signed 64-bit integer.
   * @return An unsigned 64-bit integer, stored in a signed int because
   *         Java has no explicit unsigned support.
   * @deprecated Use {@link WireFormat#encodeZigZag64(long)} instead.
   */
  @Deprecated
  public static long encodeZigZag64(final long n) {
    return WireFormat.encodeZigZag64(n);
  }
}
