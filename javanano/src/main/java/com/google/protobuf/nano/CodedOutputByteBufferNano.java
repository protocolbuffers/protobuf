// Protocol Buffers - Google's data interchange format
// Copyright 2013 Google Inc.  All rights reserved.
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

package com.google.protobuf.nano;

import java.io.IOException;
import java.io.UnsupportedEncodingException;

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
public final class CodedOutputByteBufferNano {
  private final byte[] buffer;
  private final int limit;
  private int position;

  private CodedOutputByteBufferNano(final byte[] buffer, final int offset,
                            final int length) {
    this.buffer = buffer;
    position = offset;
    limit = offset + length;
  }

  /**
   * Create a new {@code CodedOutputStream} that writes directly to the given
   * byte array.  If more bytes are written than fit in the array,
   * {@link OutOfSpaceException} will be thrown.  Writing directly to a flat
   * array is faster than writing to an {@code OutputStream}.
   */
  public static CodedOutputByteBufferNano newInstance(final byte[] flatArray) {
    return newInstance(flatArray, 0, flatArray.length);
  }

  /**
   * Create a new {@code CodedOutputStream} that writes directly to the given
   * byte array slice.  If more bytes are written than fit in the slice,
   * {@link OutOfSpaceException} will be thrown.  Writing directly to a flat
   * array is faster than writing to an {@code OutputStream}.
   */
  public static CodedOutputByteBufferNano newInstance(final byte[] flatArray,
                                              final int offset,
                                              final int length) {
    return new CodedOutputByteBufferNano(flatArray, offset, length);
  }

  // -----------------------------------------------------------------

  /** Write a {@code double} field, including tag, to the stream. */
  public void writeDouble(final int fieldNumber, final double value)
                          throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_FIXED64);
    writeDoubleNoTag(value);
  }

  /** Write a {@code float} field, including tag, to the stream. */
  public void writeFloat(final int fieldNumber, final float value)
                         throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_FIXED32);
    writeFloatNoTag(value);
  }

  /** Write a {@code uint64} field, including tag, to the stream. */
  public void writeUInt64(final int fieldNumber, final long value)
                          throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_VARINT);
    writeUInt64NoTag(value);
  }

  /** Write an {@code int64} field, including tag, to the stream. */
  public void writeInt64(final int fieldNumber, final long value)
                         throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_VARINT);
    writeInt64NoTag(value);
  }

  /** Write an {@code int32} field, including tag, to the stream. */
  public void writeInt32(final int fieldNumber, final int value)
                         throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_VARINT);
    writeInt32NoTag(value);
  }

  /** Write a {@code fixed64} field, including tag, to the stream. */
  public void writeFixed64(final int fieldNumber, final long value)
                           throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_FIXED64);
    writeFixed64NoTag(value);
  }

  /** Write a {@code fixed32} field, including tag, to the stream. */
  public void writeFixed32(final int fieldNumber, final int value)
                           throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_FIXED32);
    writeFixed32NoTag(value);
  }

  /** Write a {@code bool} field, including tag, to the stream. */
  public void writeBool(final int fieldNumber, final boolean value)
                        throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_VARINT);
    writeBoolNoTag(value);
  }

  /** Write a {@code string} field, including tag, to the stream. */
  public void writeString(final int fieldNumber, final String value)
                          throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_LENGTH_DELIMITED);
    writeStringNoTag(value);
  }

  /** Write a {@code group} field, including tag, to the stream. */
  public void writeGroup(final int fieldNumber, final MessageNano value)
                         throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_START_GROUP);
    writeGroupNoTag(value);
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_END_GROUP);
  }

  /** Write an embedded message field, including tag, to the stream. */
  public void writeMessage(final int fieldNumber, final MessageNano value)
                           throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_LENGTH_DELIMITED);
    writeMessageNoTag(value);
  }

  /** Write a {@code bytes} field, including tag, to the stream. */
  public void writeBytes(final int fieldNumber, final byte[] value)
                         throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_LENGTH_DELIMITED);
    writeBytesNoTag(value);
  }

  /** Write a {@code uint32} field, including tag, to the stream. */
  public void writeUInt32(final int fieldNumber, final int value)
                          throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_VARINT);
    writeUInt32NoTag(value);
  }

  /**
   * Write an enum field, including tag, to the stream.  Caller is responsible
   * for converting the enum value to its numeric value.
   */
  public void writeEnum(final int fieldNumber, final int value)
                        throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_VARINT);
    writeEnumNoTag(value);
  }

  /** Write an {@code sfixed32} field, including tag, to the stream. */
  public void writeSFixed32(final int fieldNumber, final int value)
                            throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_FIXED32);
    writeSFixed32NoTag(value);
  }

  /** Write an {@code sfixed64} field, including tag, to the stream. */
  public void writeSFixed64(final int fieldNumber, final long value)
                            throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_FIXED64);
    writeSFixed64NoTag(value);
  }

  /** Write an {@code sint32} field, including tag, to the stream. */
  public void writeSInt32(final int fieldNumber, final int value)
                          throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_VARINT);
    writeSInt32NoTag(value);
  }

  /** Write an {@code sint64} field, including tag, to the stream. */
  public void writeSInt64(final int fieldNumber, final long value)
                          throws IOException {
    writeTag(fieldNumber, WireFormatNano.WIRETYPE_VARINT);
    writeSInt64NoTag(value);
  }

  /**
   * Write a MessageSet extension field to the stream.  For historical reasons,
   * the wire format differs from normal fields.
   */
//  public void writeMessageSetExtension(final int fieldNumber,
//                                       final MessageMicro value)
//                                       throws IOException {
//    writeTag(WireFormatMicro.MESSAGE_SET_ITEM, WireFormatMicro.WIRETYPE_START_GROUP);
//    writeUInt32(WireFormatMicro.MESSAGE_SET_TYPE_ID, fieldNumber);
//    writeMessage(WireFormatMicro.MESSAGE_SET_MESSAGE, value);
//    writeTag(WireFormatMicro.MESSAGE_SET_ITEM, WireFormatMicro.WIRETYPE_END_GROUP);
//  }

  /**
   * Write an unparsed MessageSet extension field to the stream.  For
   * historical reasons, the wire format differs from normal fields.
   */
//  public void writeRawMessageSetExtension(final int fieldNumber,
//                                          final ByteStringMicro value)
//                                          throws IOException {
//    writeTag(WireFormatMicro.MESSAGE_SET_ITEM, WireFormatMicro.WIRETYPE_START_GROUP);
//    writeUInt32(WireFormatMicro.MESSAGE_SET_TYPE_ID, fieldNumber);
//    writeBytes(WireFormatMicro.MESSAGE_SET_MESSAGE, value);
//    writeTag(WireFormatMicro.MESSAGE_SET_ITEM, WireFormatMicro.WIRETYPE_END_GROUP);
//  }

  // -----------------------------------------------------------------

  /** Write a {@code double} field to the stream. */
  public void writeDoubleNoTag(final double value) throws IOException {
    writeRawLittleEndian64(Double.doubleToLongBits(value));
  }

  /** Write a {@code float} field to the stream. */
  public void writeFloatNoTag(final float value) throws IOException {
    writeRawLittleEndian32(Float.floatToIntBits(value));
  }

  /** Write a {@code uint64} field to the stream. */
  public void writeUInt64NoTag(final long value) throws IOException {
    writeRawVarint64(value);
  }

  /** Write an {@code int64} field to the stream. */
  public void writeInt64NoTag(final long value) throws IOException {
    writeRawVarint64(value);
  }

  /** Write an {@code int32} field to the stream. */
  public void writeInt32NoTag(final int value) throws IOException {
    if (value >= 0) {
      writeRawVarint32(value);
    } else {
      // Must sign-extend.
      writeRawVarint64(value);
    }
  }

  /** Write a {@code fixed64} field to the stream. */
  public void writeFixed64NoTag(final long value) throws IOException {
    writeRawLittleEndian64(value);
  }

  /** Write a {@code fixed32} field to the stream. */
  public void writeFixed32NoTag(final int value) throws IOException {
    writeRawLittleEndian32(value);
  }

  /** Write a {@code bool} field to the stream. */
  public void writeBoolNoTag(final boolean value) throws IOException {
    writeRawByte(value ? 1 : 0);
  }

  /** Write a {@code string} field to the stream. */
  public void writeStringNoTag(final String value) throws IOException {
    // Unfortunately there does not appear to be any way to tell Java to encode
    // UTF-8 directly into our buffer, so we have to let it create its own byte
    // array and then copy.
    final byte[] bytes = value.getBytes("UTF-8");
    writeRawVarint32(bytes.length);
    writeRawBytes(bytes);
  }

  /** Write a {@code group} field to the stream. */
  public void writeGroupNoTag(final MessageNano value) throws IOException {
    value.writeTo(this);
  }

  /** Write an embedded message field to the stream. */
  public void writeMessageNoTag(final MessageNano value) throws IOException {
    writeRawVarint32(value.getCachedSize());
    value.writeTo(this);
  }

  /** Write a {@code bytes} field to the stream. */
  public void writeBytesNoTag(final byte[] value) throws IOException {
    writeRawVarint32(value.length);
    writeRawBytes(value);
  }

  /** Write a {@code uint32} field to the stream. */
  public void writeUInt32NoTag(final int value) throws IOException {
    writeRawVarint32(value);
  }

  /**
   * Write an enum field to the stream.  Caller is responsible
   * for converting the enum value to its numeric value.
   */
  public void writeEnumNoTag(final int value) throws IOException {
    writeRawVarint32(value);
  }

  /** Write an {@code sfixed32} field to the stream. */
  public void writeSFixed32NoTag(final int value) throws IOException {
    writeRawLittleEndian32(value);
  }

  /** Write an {@code sfixed64} field to the stream. */
  public void writeSFixed64NoTag(final long value) throws IOException {
    writeRawLittleEndian64(value);
  }

  /** Write an {@code sint32} field to the stream. */
  public void writeSInt32NoTag(final int value) throws IOException {
    writeRawVarint32(encodeZigZag32(value));
  }

  /** Write an {@code sint64} field to the stream. */
  public void writeSInt64NoTag(final long value) throws IOException {
    writeRawVarint64(encodeZigZag64(value));
  }

  // =================================================================

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code double} field, including tag.
   */
  public static int computeDoubleSize(final int fieldNumber,
                                      final double value) {
    return computeTagSize(fieldNumber) + computeDoubleSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code float} field, including tag.
   */
  public static int computeFloatSize(final int fieldNumber, final float value) {
    return computeTagSize(fieldNumber) + computeFloatSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code uint64} field, including tag.
   */
  public static int computeUInt64Size(final int fieldNumber, final long value) {
    return computeTagSize(fieldNumber) + computeUInt64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code int64} field, including tag.
   */
  public static int computeInt64Size(final int fieldNumber, final long value) {
    return computeTagSize(fieldNumber) + computeInt64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code int32} field, including tag.
   */
  public static int computeInt32Size(final int fieldNumber, final int value) {
    return computeTagSize(fieldNumber) + computeInt32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code fixed64} field, including tag.
   */
  public static int computeFixed64Size(final int fieldNumber,
                                       final long value) {
    return computeTagSize(fieldNumber) + computeFixed64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code fixed32} field, including tag.
   */
  public static int computeFixed32Size(final int fieldNumber,
                                       final int value) {
    return computeTagSize(fieldNumber) + computeFixed32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bool} field, including tag.
   */
  public static int computeBoolSize(final int fieldNumber,
                                    final boolean value) {
    return computeTagSize(fieldNumber) + computeBoolSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code string} field, including tag.
   */
  public static int computeStringSize(final int fieldNumber,
                                      final String value) {
    return computeTagSize(fieldNumber) + computeStringSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code group} field, including tag.
   */
  public static int computeGroupSize(final int fieldNumber,
                                     final MessageNano value) {
    return computeTagSize(fieldNumber) * 2 + computeGroupSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * embedded message field, including tag.
   */
  public static int computeMessageSize(final int fieldNumber,
                                       final MessageNano value) {
    return computeTagSize(fieldNumber) + computeMessageSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bytes} field, including tag.
   */
  public static int computeBytesSize(final int fieldNumber,
                                     final byte[] value) {
    return computeTagSize(fieldNumber) + computeBytesSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code uint32} field, including tag.
   */
  public static int computeUInt32Size(final int fieldNumber, final int value) {
    return computeTagSize(fieldNumber) + computeUInt32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * enum field, including tag.  Caller is responsible for converting the
   * enum value to its numeric value.
   */
  public static int computeEnumSize(final int fieldNumber, final int value) {
    return computeTagSize(fieldNumber) + computeEnumSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sfixed32} field, including tag.
   */
  public static int computeSFixed32Size(final int fieldNumber,
                                        final int value) {
    return computeTagSize(fieldNumber) + computeSFixed32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sfixed64} field, including tag.
   */
  public static int computeSFixed64Size(final int fieldNumber,
                                        final long value) {
    return computeTagSize(fieldNumber) + computeSFixed64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sint32} field, including tag.
   */
  public static int computeSInt32Size(final int fieldNumber, final int value) {
    return computeTagSize(fieldNumber) + computeSInt32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sint64} field, including tag.
   */
  public static int computeSInt64Size(final int fieldNumber, final long value) {
    return computeTagSize(fieldNumber) + computeSInt64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * MessageSet extension to the stream.  For historical reasons,
   * the wire format differs from normal fields.
   */
//  public static int computeMessageSetExtensionSize(
//      final int fieldNumber, final MessageMicro value) {
//    return computeTagSize(WireFormatMicro.MESSAGE_SET_ITEM) * 2 +
//           computeUInt32Size(WireFormatMicro.MESSAGE_SET_TYPE_ID, fieldNumber) +
//           computeMessageSize(WireFormatMicro.MESSAGE_SET_MESSAGE, value);
//  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * unparsed MessageSet extension field to the stream.  For
   * historical reasons, the wire format differs from normal fields.
   */
//  public static int computeRawMessageSetExtensionSize(
//      final int fieldNumber, final ByteStringMicro value) {
//    return computeTagSize(WireFormatMicro.MESSAGE_SET_ITEM) * 2 +
//           computeUInt32Size(WireFormatMicro.MESSAGE_SET_TYPE_ID, fieldNumber) +
//           computeBytesSize(WireFormatMicro.MESSAGE_SET_MESSAGE, value);
//  }

  // -----------------------------------------------------------------

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code double} field, including tag.
   */
  public static int computeDoubleSizeNoTag(final double value) {
    return LITTLE_ENDIAN_64_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code float} field, including tag.
   */
  public static int computeFloatSizeNoTag(final float value) {
    return LITTLE_ENDIAN_32_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code uint64} field, including tag.
   */
  public static int computeUInt64SizeNoTag(final long value) {
    return computeRawVarint64Size(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code int64} field, including tag.
   */
  public static int computeInt64SizeNoTag(final long value) {
    return computeRawVarint64Size(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code int32} field, including tag.
   */
  public static int computeInt32SizeNoTag(final int value) {
    if (value >= 0) {
      return computeRawVarint32Size(value);
    } else {
      // Must sign-extend.
      return 10;
    }
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code fixed64} field.
   */
  public static int computeFixed64SizeNoTag(final long value) {
    return LITTLE_ENDIAN_64_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code fixed32} field.
   */
  public static int computeFixed32SizeNoTag(final int value) {
    return LITTLE_ENDIAN_32_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bool} field.
   */
  public static int computeBoolSizeNoTag(final boolean value) {
    return 1;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code string} field.
   */
  public static int computeStringSizeNoTag(final String value) {
    try {
      final byte[] bytes = value.getBytes("UTF-8");
      return computeRawVarint32Size(bytes.length) +
             bytes.length;
    } catch (UnsupportedEncodingException e) {
      throw new RuntimeException("UTF-8 not supported.");
    }
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code group} field.
   */
  public static int computeGroupSizeNoTag(final MessageNano value) {
    return value.getSerializedSize();
  }

  /**
   * Compute the number of bytes that would be needed to encode an embedded
   * message field.
   */
  public static int computeMessageSizeNoTag(final MessageNano value) {
    final int size = value.getSerializedSize();
    return computeRawVarint32Size(size) + size;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bytes} field.
   */
  public static int computeBytesSizeNoTag(final byte[] value) {
    return computeRawVarint32Size(value.length) + value.length;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code uint32} field.
   */
  public static int computeUInt32SizeNoTag(final int value) {
    return computeRawVarint32Size(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an enum field.
   * Caller is responsible for converting the enum value to its numeric value.
   */
  public static int computeEnumSizeNoTag(final int value) {
    return computeRawVarint32Size(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sfixed32} field.
   */
  public static int computeSFixed32SizeNoTag(final int value) {
    return LITTLE_ENDIAN_32_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sfixed64} field.
   */
  public static int computeSFixed64SizeNoTag(final long value) {
    return LITTLE_ENDIAN_64_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sint32} field.
   */
  public static int computeSInt32SizeNoTag(final int value) {
    return computeRawVarint32Size(encodeZigZag32(value));
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sint64} field.
   */
  public static int computeSInt64SizeNoTag(final long value) {
    return computeRawVarint64Size(encodeZigZag64(value));
  }

  // =================================================================

  /**
   * If writing to a flat array, return the space left in the array.
   * Otherwise, throws {@code UnsupportedOperationException}.
   */
  public int spaceLeft() {
    return limit - position;
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

    OutOfSpaceException(int position, int limit) {
      super("CodedOutputStream was writing to a flat byte array and ran " +
            "out of space (pos " + position + " limit " + limit + ").");
    }
  }

  /** Write a single byte. */
  public void writeRawByte(final byte value) throws IOException {
    if (position == limit) {
      // We're writing to a single buffer.
      throw new OutOfSpaceException(position, limit);
    }

    buffer[position++] = value;
  }

  /** Write a single byte, represented by an integer value. */
  public void writeRawByte(final int value) throws IOException {
    writeRawByte((byte) value);
  }

  /** Write an array of bytes. */
  public void writeRawBytes(final byte[] value) throws IOException {
    writeRawBytes(value, 0, value.length);
  }

  /** Write part of an array of bytes. */
  public void writeRawBytes(final byte[] value, int offset, int length)
                            throws IOException {
    if (limit - position >= length) {
      // We have room in the current buffer.
      System.arraycopy(value, offset, buffer, position, length);
      position += length;
    } else {
      // We're writing to a single buffer.
      throw new OutOfSpaceException(position, limit);
    }
  }

  /** Encode and write a tag. */
  public void writeTag(final int fieldNumber, final int wireType)
                       throws IOException {
    writeRawVarint32(WireFormatNano.makeTag(fieldNumber, wireType));
  }

  /** Compute the number of bytes that would be needed to encode a tag. */
  public static int computeTagSize(final int fieldNumber) {
    return computeRawVarint32Size(WireFormatNano.makeTag(fieldNumber, 0));
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
   */
  public static int computeRawVarint32Size(final int value) {
    if ((value & (0xffffffff <<  7)) == 0) return 1;
    if ((value & (0xffffffff << 14)) == 0) return 2;
    if ((value & (0xffffffff << 21)) == 0) return 3;
    if ((value & (0xffffffff << 28)) == 0) return 4;
    return 5;
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

  /** Compute the number of bytes that would be needed to encode a varint. */
  public static int computeRawVarint64Size(final long value) {
    if ((value & (0xffffffffffffffffL <<  7)) == 0) return 1;
    if ((value & (0xffffffffffffffffL << 14)) == 0) return 2;
    if ((value & (0xffffffffffffffffL << 21)) == 0) return 3;
    if ((value & (0xffffffffffffffffL << 28)) == 0) return 4;
    if ((value & (0xffffffffffffffffL << 35)) == 0) return 5;
    if ((value & (0xffffffffffffffffL << 42)) == 0) return 6;
    if ((value & (0xffffffffffffffffL << 49)) == 0) return 7;
    if ((value & (0xffffffffffffffffL << 56)) == 0) return 8;
    if ((value & (0xffffffffffffffffL << 63)) == 0) return 9;
    return 10;
  }

  /** Write a little-endian 32-bit integer. */
  public void writeRawLittleEndian32(final int value) throws IOException {
    writeRawByte((value      ) & 0xFF);
    writeRawByte((value >>  8) & 0xFF);
    writeRawByte((value >> 16) & 0xFF);
    writeRawByte((value >> 24) & 0xFF);
  }

  public static final int LITTLE_ENDIAN_32_SIZE = 4;

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

  public static final int LITTLE_ENDIAN_64_SIZE = 8;

  /**
   * Encode a ZigZag-encoded 32-bit value.  ZigZag encodes signed integers
   * into values that can be efficiently encoded with varint.  (Otherwise,
   * negative values must be sign-extended to 64 bits to be varint encoded,
   * thus always taking 10 bytes on the wire.)
   *
   * @param n A signed 32-bit integer.
   * @return An unsigned 32-bit integer, stored in a signed int because
   *         Java has no explicit unsigned support.
   */
  public static int encodeZigZag32(final int n) {
    // Note:  the right-shift must be arithmetic
    return (n << 1) ^ (n >> 31);
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
   */
  public static long encodeZigZag64(final long n) {
    // Note:  the right-shift must be arithmetic
    return (n << 1) ^ (n >> 63);
  }
}
