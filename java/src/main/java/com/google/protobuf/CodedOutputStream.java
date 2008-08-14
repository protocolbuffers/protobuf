// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.protobuf;

import java.io.OutputStream;
import java.io.IOException;

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
public final class CodedOutputStream {
  private final byte[] buffer;
  private final int limit;
  private int position;

  private final OutputStream output;

  /**
   * The buffer size used in {@link #newInstance(java.io.OutputStream)}.
   */
  public static final int DEFAULT_BUFFER_SIZE = 4096;

  private CodedOutputStream(byte[] buffer, int offset, int length) {
    this.output = null;
    this.buffer = buffer;
    this.position = offset;
    this.limit = offset + length;
  }

  private CodedOutputStream(OutputStream output, byte[] buffer) {
    this.output = output;
    this.buffer = buffer;
    this.position = 0;
    this.limit = buffer.length;
  }

  /**
   * Create a new {@code CodedOutputStream} wrapping the given
   * {@code OutputStream}.
   */
  public static CodedOutputStream newInstance(OutputStream output) {
    return newInstance(output, DEFAULT_BUFFER_SIZE);
  }

  /**
   * Create a new {@code CodedOutputStream} wrapping the given
   * {@code OutputStream} with a given buffer size.
   */
  public static CodedOutputStream newInstance(OutputStream output,
      int bufferSize) {
    return new CodedOutputStream(output, new byte[bufferSize]);
  }

  /**
   * Create a new {@code CodedOutputStream} that writes directly to the given
   * byte array.  If more bytes are written than fit in the array,
   * {@link OutOfSpaceException} will be thrown.  Writing directly to a flat
   * array is faster than writing to an {@code OutputStream}.  See also
   * {@link ByteString#newCodedBuilder}.
   */
  public static CodedOutputStream newInstance(byte[] flatArray) {
    return newInstance(flatArray, 0, flatArray.length);
  }

  /**
   * Create a new {@code CodedOutputStream} that writes directly to the given
   * byte array slice.  If more bytes are written than fit in the slice,
   * {@link OutOfSpaceException} will be thrown.  Writing directly to a flat
   * array is faster than writing to an {@code OutputStream}.  See also
   * {@link ByteString#newCodedBuilder}.
   */
  public static CodedOutputStream newInstance(byte[] flatArray, int offset,
      int length) {
    return new CodedOutputStream(flatArray, offset, length);
  }

  // -----------------------------------------------------------------

  /** Write a {@code double} field, including tag, to the stream. */
  public void writeDouble(int fieldNumber, double value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
    writeRawLittleEndian64(Double.doubleToRawLongBits(value));
  }

  /** Write a {@code float} field, including tag, to the stream. */
  public void writeFloat(int fieldNumber, float value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
    writeRawLittleEndian32(Float.floatToRawIntBits(value));
  }

  /** Write a {@code uint64} field, including tag, to the stream. */
  public void writeUInt64(int fieldNumber, long value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeRawVarint64(value);
  }

  /** Write an {@code int64} field, including tag, to the stream. */
  public void writeInt64(int fieldNumber, long value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeRawVarint64(value);
  }

  /** Write an {@code int32} field, including tag, to the stream. */
  public void writeInt32(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    if (value >= 0) {
      writeRawVarint32(value);
    } else {
      // Must sign-extend.
      writeRawVarint64(value);
    }
  }

  /** Write a {@code fixed64} field, including tag, to the stream. */
  public void writeFixed64(int fieldNumber, long value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
    writeRawLittleEndian64(value);
  }

  /** Write a {@code fixed32} field, including tag, to the stream. */
  public void writeFixed32(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
    writeRawLittleEndian32(value);
  }

  /** Write a {@code bool} field, including tag, to the stream. */
  public void writeBool(int fieldNumber, boolean value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeRawByte(value ? 1 : 0);
  }

  /** Write a {@code string} field, including tag, to the stream. */
  public void writeString(int fieldNumber, String value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    // Unfortunately there does not appear to be any way to tell Java to encode
    // UTF-8 directly into our buffer, so we have to let it create its own byte
    // array and then copy.
    byte[] bytes = value.getBytes("UTF-8");
    writeRawVarint32(bytes.length);
    writeRawBytes(bytes);
  }

  /** Write a {@code group} field, including tag, to the stream. */
  public void writeGroup(int fieldNumber, Message value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_START_GROUP);
    value.writeTo(this);
    writeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP);
  }

  /** Write a group represented by an {@link UnknownFieldSet}. */
  public void writeUnknownGroup(int fieldNumber, UnknownFieldSet value)
      throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_START_GROUP);
    value.writeTo(this);
    writeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP);
  }

  /** Write an embedded message field, including tag, to the stream. */
  public void writeMessage(int fieldNumber, Message value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    writeRawVarint32(value.getSerializedSize());
    value.writeTo(this);
  }

  /** Write a {@code bytes} field, including tag, to the stream. */
  public void writeBytes(int fieldNumber, ByteString value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    byte[] bytes = value.toByteArray();
    writeRawVarint32(bytes.length);
    writeRawBytes(bytes);
  }

  /** Write a {@code uint32} field, including tag, to the stream. */
  public void writeUInt32(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeRawVarint32(value);
  }

  /**
   * Write an enum field, including tag, to the stream.  Caller is responsible
   * for converting the enum value to its numeric value.
   */
  public void writeEnum(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeRawVarint32(value);
  }

  /** Write an {@code sfixed32} field, including tag, to the stream. */
  public void writeSFixed32(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED32);
    writeRawLittleEndian32(value);
  }

  /** Write an {@code sfixed64} field, including tag, to the stream. */
  public void writeSFixed64(int fieldNumber, long value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_FIXED64);
    writeRawLittleEndian64(value);
  }

  /** Write an {@code sint32} field, including tag, to the stream. */
  public void writeSInt32(int fieldNumber, int value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeRawVarint32(encodeZigZag32(value));
  }

  /** Write an {@code sint64} field, including tag, to the stream. */
  public void writeSInt64(int fieldNumber, long value) throws IOException {
    writeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
    writeRawVarint64(encodeZigZag64(value));
  }

  /**
   * Write a MessageSet extension field to the stream.  For historical reasons,
   * the wire format differs from normal fields.
   */
  public void writeMessageSetExtension(int fieldNumber, Message value)
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
  public void writeRawMessageSetExtension(int fieldNumber, ByteString value)
                                          throws IOException {
    writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_START_GROUP);
    writeUInt32(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber);
    writeBytes(WireFormat.MESSAGE_SET_MESSAGE, value);
    writeTag(WireFormat.MESSAGE_SET_ITEM, WireFormat.WIRETYPE_END_GROUP);
  }

  /**
   * Write a field of arbitrary type, including tag, to the stream.
   *
   * @param type   The field's type.
   * @param number The field's number.
   * @param value  Object representing the field's value.  Must be of the exact
   *               type which would be returned by
   *               {@link Message#getField(Descriptors.FieldDescriptor)} for
   *               this field.
   */
  public void writeField(Descriptors.FieldDescriptor.Type type,
                         int number, Object value) throws IOException {
    switch (type) {
      case DOUBLE  : writeDouble  (number, (Double    )value); break;
      case FLOAT   : writeFloat   (number, (Float     )value); break;
      case INT64   : writeInt64   (number, (Long      )value); break;
      case UINT64  : writeUInt64  (number, (Long      )value); break;
      case INT32   : writeInt32   (number, (Integer   )value); break;
      case FIXED64 : writeFixed64 (number, (Long      )value); break;
      case FIXED32 : writeFixed32 (number, (Integer   )value); break;
      case BOOL    : writeBool    (number, (Boolean   )value); break;
      case STRING  : writeString  (number, (String    )value); break;
      case GROUP   : writeGroup   (number, (Message   )value); break;
      case MESSAGE : writeMessage (number, (Message   )value); break;
      case BYTES   : writeBytes   (number, (ByteString)value); break;
      case UINT32  : writeUInt32  (number, (Integer   )value); break;
      case SFIXED32: writeSFixed32(number, (Integer   )value); break;
      case SFIXED64: writeSFixed64(number, (Long      )value); break;
      case SINT32  : writeSInt32  (number, (Integer   )value); break;
      case SINT64  : writeSInt64  (number, (Long      )value); break;

      case ENUM:
        writeEnum(number, ((Descriptors.EnumValueDescriptor)value).getNumber());
        break;
    }
  }

  // =================================================================

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code double} field, including tag.
   */
  public static int computeDoubleSize(int fieldNumber, double value) {
    return computeTagSize(fieldNumber) + LITTLE_ENDIAN_64_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code float} field, including tag.
   */
  public static int computeFloatSize(int fieldNumber, float value) {
    return computeTagSize(fieldNumber) + LITTLE_ENDIAN_32_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code uint64} field, including tag.
   */
  public static int computeUInt64Size(int fieldNumber, long value) {
    return computeTagSize(fieldNumber) + computeRawVarint64Size(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code int64} field, including tag.
   */
  public static int computeInt64Size(int fieldNumber, long value) {
    return computeTagSize(fieldNumber) + computeRawVarint64Size(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code int32} field, including tag.
   */
  public static int computeInt32Size(int fieldNumber, int value) {
    if (value >= 0) {
      return computeTagSize(fieldNumber) + computeRawVarint32Size(value);
    } else {
      // Must sign-extend.
      return computeTagSize(fieldNumber) + 10;
    }
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code fixed64} field, including tag.
   */
  public static int computeFixed64Size(int fieldNumber, long value) {
    return computeTagSize(fieldNumber) + LITTLE_ENDIAN_64_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code fixed32} field, including tag.
   */
  public static int computeFixed32Size(int fieldNumber, int value) {
    return computeTagSize(fieldNumber) + LITTLE_ENDIAN_32_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bool} field, including tag.
   */
  public static int computeBoolSize(int fieldNumber, boolean value) {
    return computeTagSize(fieldNumber) + 1;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code string} field, including tag.
   */
  public static int computeStringSize(int fieldNumber, String value) {
    try {
      byte[] bytes = value.getBytes("UTF-8");
      return computeTagSize(fieldNumber) +
             computeRawVarint32Size(bytes.length) +
             bytes.length;
    } catch (java.io.UnsupportedEncodingException e) {
      throw new RuntimeException("UTF-8 not supported.", e);
    }
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code group} field, including tag.
   */
  public static int computeGroupSize(int fieldNumber, Message value) {
    return computeTagSize(fieldNumber) * 2 + value.getSerializedSize();
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code group} field represented by an {@code UnknownFieldSet}, including
   * tag.
   */
  public static int computeUnknownGroupSize(int fieldNumber,
                                            UnknownFieldSet value) {
    return computeTagSize(fieldNumber) * 2 + value.getSerializedSize();
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * embedded message field, including tag.
   */
  public static int computeMessageSize(int fieldNumber, Message value) {
    int size = value.getSerializedSize();
    return computeTagSize(fieldNumber) + computeRawVarint32Size(size) + size;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bytes} field, including tag.
   */
  public static int computeBytesSize(int fieldNumber, ByteString value) {
    return computeTagSize(fieldNumber) +
           computeRawVarint32Size(value.size()) +
           value.size();
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code uint32} field, including tag.
   */
  public static int computeUInt32Size(int fieldNumber, int value) {
    return computeTagSize(fieldNumber) + computeRawVarint32Size(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * enum field, including tag.  Caller is responsible for converting the
   * enum value to its numeric value.
   */
  public static int computeEnumSize(int fieldNumber, int value) {
    return computeTagSize(fieldNumber) + computeRawVarint32Size(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sfixed32} field, including tag.
   */
  public static int computeSFixed32Size(int fieldNumber, int value) {
    return computeTagSize(fieldNumber) + LITTLE_ENDIAN_32_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sfixed64} field, including tag.
   */
  public static int computeSFixed64Size(int fieldNumber, long value) {
    return computeTagSize(fieldNumber) + LITTLE_ENDIAN_64_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sint32} field, including tag.
   */
  public static int computeSInt32Size(int fieldNumber, int value) {
    return computeTagSize(fieldNumber) +
           computeRawVarint32Size(encodeZigZag32(value));
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sint64} field, including tag.
   */
  public static int computeSInt64Size(int fieldNumber, long value) {
    return computeTagSize(fieldNumber) +
           computeRawVarint64Size(encodeZigZag64(value));
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * MessageSet extension to the stream.  For historical reasons,
   * the wire format differs from normal fields.
   */
  public static int computeMessageSetExtensionSize(
      int fieldNumber, Message value) {
    return computeTagSize(WireFormat.MESSAGE_SET_ITEM) * 2 +
           computeUInt32Size(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber) +
           computeMessageSize(WireFormat.MESSAGE_SET_MESSAGE, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * unparsed MessageSet extension field to the stream.  For
   * historical reasons, the wire format differs from normal fields.
   */
  public static int computeRawMessageSetExtensionSize(
      int fieldNumber, ByteString value) {
    return computeTagSize(WireFormat.MESSAGE_SET_ITEM) * 2 +
           computeUInt32Size(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber) +
           computeBytesSize(WireFormat.MESSAGE_SET_MESSAGE, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * field of arbitrary type, including tag, to the stream.
   *
   * @param type   The field's type.
   * @param number The field's number.
   * @param value  Object representing the field's value.  Must be of the exact
   *               type which would be returned by
   *               {@link Message#getField(Descriptors.FieldDescriptor)} for
   *               this field.
   */
  public static int computeFieldSize(
      Descriptors.FieldDescriptor.Type type,
      int number, Object value) {
    switch (type) {
      case DOUBLE  : return computeDoubleSize  (number, (Double    )value);
      case FLOAT   : return computeFloatSize   (number, (Float     )value);
      case INT64   : return computeInt64Size   (number, (Long      )value);
      case UINT64  : return computeUInt64Size  (number, (Long      )value);
      case INT32   : return computeInt32Size   (number, (Integer   )value);
      case FIXED64 : return computeFixed64Size (number, (Long      )value);
      case FIXED32 : return computeFixed32Size (number, (Integer   )value);
      case BOOL    : return computeBoolSize    (number, (Boolean   )value);
      case STRING  : return computeStringSize  (number, (String    )value);
      case GROUP   : return computeGroupSize   (number, (Message   )value);
      case MESSAGE : return computeMessageSize (number, (Message   )value);
      case BYTES   : return computeBytesSize   (number, (ByteString)value);
      case UINT32  : return computeUInt32Size  (number, (Integer   )value);
      case SFIXED32: return computeSFixed32Size(number, (Integer   )value);
      case SFIXED64: return computeSFixed64Size(number, (Long      )value);
      case SINT32  : return computeSInt32Size  (number, (Integer   )value);
      case SINT64  : return computeSInt64Size  (number, (Long      )value);

      case ENUM:
        return computeEnumSize(number,
          ((Descriptors.EnumValueDescriptor)value).getNumber());
    }

    throw new RuntimeException(
      "There is no way to get here, but the compiler thinks otherwise.");
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
    OutOfSpaceException() {
      super("CodedOutputStream was writing to a flat byte array and ran " +
            "out of space.");
    }
  }

  /** Write a single byte. */
  public void writeRawByte(byte value) throws IOException {
    if (position == limit) {
      refreshBuffer();
    }

    buffer[position++] = value;
  }

  /** Write a single byte, represented by an integer value. */
  public void writeRawByte(int value) throws IOException {
    writeRawByte((byte) value);
  }

  /** Write an array of bytes. */
  public void writeRawBytes(byte[] value) throws IOException {
    writeRawBytes(value, 0, value.length);
  }

  /** Write part of an array of bytes. */
  public void writeRawBytes(byte[] value, int offset, int length)
                            throws IOException {
    if (limit - position >= length) {
      // We have room in the current buffer.
      System.arraycopy(value, offset, buffer, position, length);
      position += length;
    } else {
      // Write extends past current buffer.  Fill the rest of this buffer and
      // flush.
      int bytesWritten = limit - position;
      System.arraycopy(value, offset, buffer, position, bytesWritten);
      offset += bytesWritten;
      length -= bytesWritten;
      position = limit;
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
    }
  }

  /** Encode and write a tag. */
  public void writeTag(int fieldNumber, int wireType) throws IOException {
    writeRawVarint32(WireFormat.makeTag(fieldNumber, wireType));
  }

  /** Compute the number of bytes that would be needed to encode a tag. */
  public static int computeTagSize(int fieldNumber) {
    return computeRawVarint32Size(WireFormat.makeTag(fieldNumber, 0));
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
  public static int computeRawVarint32Size(int value) {
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
  public static int computeRawVarint64Size(long value) {
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
  public void writeRawLittleEndian32(int value) throws IOException {
    writeRawByte((value      ) & 0xFF);
    writeRawByte((value >>  8) & 0xFF);
    writeRawByte((value >> 16) & 0xFF);
    writeRawByte((value >> 24) & 0xFF);
  }

  public static final int LITTLE_ENDIAN_32_SIZE = 4;

  /** Write a little-endian 64-bit integer. */
  public void writeRawLittleEndian64(long value) throws IOException {
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
  public static int encodeZigZag32(int n) {
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
  public static long encodeZigZag64(long n) {
    // Note:  the right-shift must be arithmetic
    return (n << 1) ^ (n >> 63);
  }
}
