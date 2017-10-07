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
import java.nio.BufferOverflowException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.ReadOnlyBufferException;

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
  /* max bytes per java UTF-16 char in UTF-8 */
  private static final int MAX_UTF8_EXPANSION = 3;
  private final ByteBuffer buffer;

  private CodedOutputByteBufferNano(final byte[] buffer, final int offset,
                            final int length) {
    this(ByteBuffer.wrap(buffer, offset, length));
  }

  private CodedOutputByteBufferNano(final ByteBuffer buffer) {
    this.buffer = buffer;
    this.buffer.order(ByteOrder.LITTLE_ENDIAN);
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
    // UTF-8 byte length of the string is at least its UTF-16 code unit length (value.length()),
    // and at most 3 times of it. Optimize for the case where we know this length results in a
    // constant varint length - saves measuring length of the string.
    try {
      final int minLengthVarIntSize = computeRawVarint32Size(value.length());
      final int maxLengthVarIntSize = computeRawVarint32Size(value.length() * MAX_UTF8_EXPANSION);
      if (minLengthVarIntSize == maxLengthVarIntSize) {
        int oldPosition = buffer.position();
        // Buffer.position, when passed a position that is past its limit, throws
        // IllegalArgumentException, and this class is documented to throw
        // OutOfSpaceException instead.
        if (buffer.remaining() < minLengthVarIntSize) {
          throw new OutOfSpaceException(oldPosition + minLengthVarIntSize, buffer.limit());
        }
        buffer.position(oldPosition + minLengthVarIntSize);
        encode(value, buffer);
        int newPosition = buffer.position();
        buffer.position(oldPosition);
        writeRawVarint32(newPosition - oldPosition - minLengthVarIntSize);
        buffer.position(newPosition);
      } else {
        writeRawVarint32(encodedLength(value));
        encode(value, buffer);
      }
    } catch (BufferOverflowException e) {
      final OutOfSpaceException outOfSpaceException = new OutOfSpaceException(buffer.position(),
          buffer.limit());
      outOfSpaceException.initCause(e);
      throw outOfSpaceException;
    }
  }

  // These UTF-8 handling methods are copied from Guava's Utf8 class.
  /**
   * Returns the number of bytes in the UTF-8-encoded form of {@code sequence}. For a string,
   * this method is equivalent to {@code string.getBytes(UTF_8).length}, but is more efficient in
   * both time and space.
   *
   * @throws IllegalArgumentException if {@code sequence} contains ill-formed UTF-16 (unpaired
   *     surrogates)
   */
  private static int encodedLength(CharSequence sequence) {
    // Warning to maintainers: this implementation is highly optimized.
    int utf16Length = sequence.length();
    int utf8Length = utf16Length;
    int i = 0;

    // This loop optimizes for pure ASCII.
    while (i < utf16Length && sequence.charAt(i) < 0x80) {
      i++;
    }

    // This loop optimizes for chars less than 0x800.
    for (; i < utf16Length; i++) {
      char c = sequence.charAt(i);
      if (c < 0x800) {
        utf8Length += ((0x7f - c) >>> 31);  // branch free!
      } else {
        utf8Length += encodedLengthGeneral(sequence, i);
        break;
      }
    }

    if (utf8Length < utf16Length) {
      // Necessary and sufficient condition for overflow because of maximum 3x expansion
      throw new IllegalArgumentException("UTF-8 length does not fit in int: "
              + (utf8Length + (1L << 32)));
    }
    return utf8Length;
  }

  private static int encodedLengthGeneral(CharSequence sequence, int start) {
    int utf16Length = sequence.length();
    int utf8Length = 0;
    for (int i = start; i < utf16Length; i++) {
      char c = sequence.charAt(i);
      if (c < 0x800) {
        utf8Length += (0x7f - c) >>> 31; // branch free!
      } else {
        utf8Length += 2;
        // jdk7+: if (Character.isSurrogate(c)) {
        if (Character.MIN_SURROGATE <= c && c <= Character.MAX_SURROGATE) {
          // Check that we have a well-formed surrogate pair.
          int cp = Character.codePointAt(sequence, i);
          if (cp < Character.MIN_SUPPLEMENTARY_CODE_POINT) {
            throw new IllegalArgumentException("Unpaired surrogate at index " + i);
          }
          i++;
        }
      }
    }
    return utf8Length;
  }

  /**
   * Encodes {@code sequence} into UTF-8, in {@code byteBuffer}. For a string, this method is
   * equivalent to {@code buffer.put(string.getBytes(UTF_8))}, but is more efficient in both time
   * and space. Bytes are written starting at the current position. This method requires paired
   * surrogates, and therefore does not support chunking.
   *
   * <p>To ensure sufficient space in the output buffer, either call {@link #encodedLength} to
   * compute the exact amount needed, or leave room for {@code 3 * sequence.length()}, which is the
   * largest possible number of bytes that any input can be encoded to.
   *
   * @throws IllegalArgumentException if {@code sequence} contains ill-formed UTF-16 (unpaired
   *     surrogates)
   * @throws BufferOverflowException if {@code sequence} encoded in UTF-8 does not fit in
   *     {@code byteBuffer}'s remaining space.
   * @throws ReadOnlyBufferException if {@code byteBuffer} is a read-only buffer.
   */
  private static void encode(CharSequence sequence, ByteBuffer byteBuffer) {
    if (byteBuffer.isReadOnly()) {
      throw new ReadOnlyBufferException();
    } else if (byteBuffer.hasArray()) {
      try {
        int encoded = encode(sequence,
                byteBuffer.array(),
                byteBuffer.arrayOffset() + byteBuffer.position(),
                byteBuffer.remaining());
        byteBuffer.position(encoded - byteBuffer.arrayOffset());
      } catch (ArrayIndexOutOfBoundsException e) {
        BufferOverflowException boe = new BufferOverflowException();
        boe.initCause(e);
        throw boe;
      }
    } else {
      encodeDirect(sequence, byteBuffer);
    }
  }

  private static void encodeDirect(CharSequence sequence, ByteBuffer byteBuffer) {
    int utf16Length = sequence.length();
    for (int i = 0; i < utf16Length; i++) {
      final char c = sequence.charAt(i);
      if (c < 0x80) { // ASCII
        byteBuffer.put((byte) c);
      } else if (c < 0x800) { // 11 bits, two UTF-8 bytes
        byteBuffer.put((byte) ((0xF << 6) | (c >>> 6)));
        byteBuffer.put((byte) (0x80 | (0x3F & c)));
      } else if (c < Character.MIN_SURROGATE || Character.MAX_SURROGATE < c) {
        // Maximium single-char code point is 0xFFFF, 16 bits, three UTF-8 bytes
        byteBuffer.put((byte) ((0xF << 5) | (c >>> 12)));
        byteBuffer.put((byte) (0x80 | (0x3F & (c >>> 6))));
        byteBuffer.put((byte) (0x80 | (0x3F & c)));
      } else {
        final char low;
        if (i + 1 == sequence.length()
                || !Character.isSurrogatePair(c, (low = sequence.charAt(++i)))) {
          throw new IllegalArgumentException("Unpaired surrogate at index " + (i - 1));
        }
        int codePoint = Character.toCodePoint(c, low);
        byteBuffer.put((byte) ((0xF << 4) | (codePoint >>> 18)));
        byteBuffer.put((byte) (0x80 | (0x3F & (codePoint >>> 12))));
        byteBuffer.put((byte) (0x80 | (0x3F & (codePoint >>> 6))));
        byteBuffer.put((byte) (0x80 | (0x3F & codePoint)));
      }
    }
  }

  private static int encode(CharSequence sequence, byte[] bytes, int offset, int length) {
    int utf16Length = sequence.length();
    int j = offset;
    int i = 0;
    int limit = offset + length;
    // Designed to take advantage of
    // https://wikis.oracle.com/display/HotSpotInternals/RangeCheckElimination
    for (char c; i < utf16Length && i + j < limit && (c = sequence.charAt(i)) < 0x80; i++) {
      bytes[j + i] = (byte) c;
    }
    if (i == utf16Length) {
      return j + utf16Length;
    }
    j += i;
    for (char c; i < utf16Length; i++) {
      c = sequence.charAt(i);
      if (c < 0x80 && j < limit) {
        bytes[j++] = (byte) c;
      } else if (c < 0x800 && j <= limit - 2) { // 11 bits, two UTF-8 bytes
        bytes[j++] = (byte) ((0xF << 6) | (c >>> 6));
        bytes[j++] = (byte) (0x80 | (0x3F & c));
      } else if ((c < Character.MIN_SURROGATE || Character.MAX_SURROGATE < c) && j <= limit - 3) {
        // Maximum single-char code point is 0xFFFF, 16 bits, three UTF-8 bytes
        bytes[j++] = (byte) ((0xF << 5) | (c >>> 12));
        bytes[j++] = (byte) (0x80 | (0x3F & (c >>> 6)));
        bytes[j++] = (byte) (0x80 | (0x3F & c));
      } else if (j <= limit - 4) {
        // Minimum code point represented by a surrogate pair is 0x10000, 17 bits, four UTF-8 bytes
        final char low;
        if (i + 1 == sequence.length()
                || !Character.isSurrogatePair(c, (low = sequence.charAt(++i)))) {
          throw new IllegalArgumentException("Unpaired surrogate at index " + (i - 1));
        }
        int codePoint = Character.toCodePoint(c, low);
        bytes[j++] = (byte) ((0xF << 4) | (codePoint >>> 18));
        bytes[j++] = (byte) (0x80 | (0x3F & (codePoint >>> 12)));
        bytes[j++] = (byte) (0x80 | (0x3F & (codePoint >>> 6)));
        bytes[j++] = (byte) (0x80 | (0x3F & codePoint));
      } else {
        throw new ArrayIndexOutOfBoundsException("Failed writing " + c + " at index " + j);
      }
    }
    return j;
  }

  // End guava UTF-8 methods


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
    final int length = encodedLength(value);
    return computeRawVarint32Size(length) + length;
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
    return buffer.remaining();
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
   * Returns the position within the internal buffer.
   */
  public int position() {
    return buffer.position();
  }

  /**
   * Resets the position within the internal buffer to zero.
   *
   * @see #position
   * @see #spaceLeft
   */
  public void reset() {
    buffer.clear();
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
    if (!buffer.hasRemaining()) {
      // We're writing to a single buffer.
      throw new OutOfSpaceException(buffer.position(), buffer.limit());
    }

    buffer.put(value);
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
    if (buffer.remaining() >= length) {
      buffer.put(value, offset, length);
    } else {
      // We're writing to a single buffer.
      throw new OutOfSpaceException(buffer.position(), buffer.limit());
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
    if (buffer.remaining() < 4) {
      throw new OutOfSpaceException(buffer.position(), buffer.limit());
    }
    buffer.putInt(value);
  }

  public static final int LITTLE_ENDIAN_32_SIZE = 4;

  /** Write a little-endian 64-bit integer. */
  public void writeRawLittleEndian64(final long value) throws IOException {
    if (buffer.remaining() < 8) {
      throw new OutOfSpaceException(buffer.position(), buffer.limit());
    }
    buffer.putLong(value);
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

  static int computeFieldSize(int number, int type, Object object) {
    switch (type) {
      case InternalNano.TYPE_BOOL:
        return computeBoolSize(number, (Boolean) object);
      case InternalNano.TYPE_BYTES:
        return computeBytesSize(number, (byte[]) object);
      case InternalNano.TYPE_STRING:
        return computeStringSize(number, (String) object);
      case InternalNano.TYPE_FLOAT:
        return computeFloatSize(number, (Float) object);
      case InternalNano.TYPE_DOUBLE:
        return computeDoubleSize(number, (Double) object);
      case InternalNano.TYPE_ENUM:
        return computeEnumSize(number, (Integer) object);
      case InternalNano.TYPE_FIXED32:
        return computeFixed32Size(number, (Integer) object);
      case InternalNano.TYPE_INT32:
        return computeInt32Size(number, (Integer) object);
      case InternalNano.TYPE_UINT32:
        return computeUInt32Size(number, (Integer) object);
      case InternalNano.TYPE_SINT32:
        return computeSInt32Size(number, (Integer) object);
      case InternalNano.TYPE_SFIXED32:
        return computeSFixed32Size(number, (Integer) object);
      case InternalNano.TYPE_INT64:
        return computeInt64Size(number, (Long) object);
      case InternalNano.TYPE_UINT64:
        return computeUInt64Size(number, (Long) object);
      case InternalNano.TYPE_SINT64:
        return computeSInt64Size(number, (Long) object);
      case InternalNano.TYPE_FIXED64:
        return computeFixed64Size(number, (Long) object);
      case InternalNano.TYPE_SFIXED64:
        return computeSFixed64Size(number, (Long) object);
      case InternalNano.TYPE_MESSAGE:
        return computeMessageSize(number, (MessageNano) object);
      case InternalNano.TYPE_GROUP:
        return computeGroupSize(number, (MessageNano) object);
      default:
        throw new IllegalArgumentException("Unknown type: " + type);
    }
  }

  void writeField(int number, int type, Object value)
      throws IOException {
    switch (type) {
      case InternalNano.TYPE_DOUBLE:
        Double doubleValue = (Double) value;
        writeDouble(number, doubleValue);
        break;
      case InternalNano.TYPE_FLOAT:
        Float floatValue = (Float) value;
        writeFloat(number, floatValue);
        break;
      case InternalNano.TYPE_INT64:
        Long int64Value = (Long) value;
        writeInt64(number, int64Value);
        break;
      case InternalNano.TYPE_UINT64:
        Long uint64Value = (Long) value;
        writeUInt64(number, uint64Value);
        break;
      case InternalNano.TYPE_INT32:
        Integer int32Value = (Integer) value;
        writeInt32(number, int32Value);
        break;
      case InternalNano.TYPE_FIXED64:
        Long fixed64Value = (Long) value;
        writeFixed64(number, fixed64Value);
        break;
      case InternalNano.TYPE_FIXED32:
        Integer fixed32Value = (Integer) value;
        writeFixed32(number, fixed32Value);
        break;
      case InternalNano.TYPE_BOOL:
        Boolean boolValue = (Boolean) value;
        writeBool(number, boolValue);
        break;
      case InternalNano.TYPE_STRING:
        String stringValue = (String) value;
        writeString(number, stringValue);
        break;
      case InternalNano.TYPE_BYTES:
        byte[] bytesValue = (byte[]) value;
        writeBytes(number, bytesValue);
        break;
      case InternalNano.TYPE_UINT32:
        Integer uint32Value = (Integer) value;
        writeUInt32(number, uint32Value);
        break;
      case InternalNano.TYPE_ENUM:
        Integer enumValue = (Integer) value;
        writeEnum(number, enumValue);
        break;
      case InternalNano.TYPE_SFIXED32:
        Integer sfixed32Value = (Integer) value;
        writeSFixed32(number, sfixed32Value);
        break;
      case InternalNano.TYPE_SFIXED64:
        Long sfixed64Value = (Long) value;
        writeSFixed64(number, sfixed64Value);
        break;
      case InternalNano.TYPE_SINT32:
        Integer sint32Value = (Integer) value;
        writeSInt32(number, sint32Value);
        break;
      case InternalNano.TYPE_SINT64:
        Long sint64Value = (Long) value;
        writeSInt64(number, sint64Value);
        break;
      case InternalNano.TYPE_MESSAGE:
        MessageNano messageValue = (MessageNano) value;
        writeMessage(number, messageValue);
        break;
      case InternalNano.TYPE_GROUP:
        MessageNano groupValue = (MessageNano) value;
        writeGroup(number, groupValue);
        break;
      default:
        throw new IOException("Unknown type: " + type);
    }
  }

}
