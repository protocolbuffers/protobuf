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

/**
 * An encoder for protobuf fields.
 */
public interface Encoder {
  /** Write a {@code double} field, including tag, to the stream. */
  void writeDouble(final int fieldNumber, final double value) throws IOException;

  /** Write a {@code float} field, including tag, to the stream. */
  void writeFloat(final int fieldNumber, final float value) throws IOException;

  /** Write a {@code uint64} field, including tag, to the stream. */
  void writeUInt64(final int fieldNumber, final long value) throws IOException;

  /** Write an {@code int64} field, including tag, to the stream. */
  void writeInt64(final int fieldNumber, final long value) throws IOException;

  /** Write an {@code int32} field, including tag, to the stream. */
  void writeInt32(final int fieldNumber, final int value) throws IOException;

  /** Write a {@code fixed64} field, including tag, to the stream. */
  void writeFixed64(final int fieldNumber, final long value) throws IOException;

  /** Write a {@code fixed32} field, including tag, to the stream. */
  void writeFixed32(final int fieldNumber, final int value) throws IOException;

  /** Write a {@code bool} field, including tag, to the stream. */
  void writeBool(final int fieldNumber, final boolean value) throws IOException;

  /** Write a {@code string} field, including tag, to the stream. */
  void writeString(final int fieldNumber, final String value) throws IOException;

  /** Write a {@code group} field, including tag, to the stream. */
  void writeGroup(final int fieldNumber, final MessageLite value) throws IOException;

  /** Write an embedded message field, including tag, to the stream. */
  void writeMessage(final int fieldNumber, final MessageLite value) throws IOException;


  /** Write a {@code bytes} field, including tag, to the stream. */
  void writeBytes(final int fieldNumber, final ByteString value) throws IOException;

  /** Write a {@code bytes} field, including tag, to the stream. */
  void writeByteArray(final int fieldNumber, final byte[] value) throws IOException;

  /** Write a {@code bytes} field, including tag, to the stream. */
  void writeByteArray(final int fieldNumber,
                             final byte[] value,
                             final int offset,
                             final int length) throws IOException;

  /** Write a {@code uint32} field, including tag, to the stream. */
  void writeUInt32(final int fieldNumber, final int value) throws IOException;

  /**
   * Write an enum field, including tag, to the stream.  Caller is responsible
   * for converting the enum value to its numeric value.
   */
  void writeEnum(final int fieldNumber, final int value) throws IOException;

  /** Write an {@code sfixed32} field, including tag, to the stream. */
  void writeSFixed32(final int fieldNumber, final int value) throws IOException;

  /** Write an {@code sfixed64} field, including tag, to the stream. */
  void writeSFixed64(final int fieldNumber, final long value) throws IOException;

  /** Write an {@code sint32} field, including tag, to the stream. */
  void writeSInt32(final int fieldNumber, final int value) throws IOException;

  /** Write an {@code sint64} field, including tag, to the stream. */
  void writeSInt64(final int fieldNumber, final long value) throws IOException;

  /**
   * Write a MessageSet extension field to the stream.  For historical reasons,
   * the wire format differs from normal fields.
   */
  void writeMessageSetExtension(final int fieldNumber,
                                       final MessageLite value)
      throws IOException;

  /**
   * Write an unparsed MessageSet extension field to the stream.  For
   * historical reasons, the wire format differs from normal fields.
   */
  void writeRawMessageSetExtension(final int fieldNumber,
                                          final ByteString value)
      throws IOException;

  // -----------------------------------------------------------------

  /** Write a {@code double} field to the stream. */
  void writeDoubleNoTag(final double value) throws IOException;

  /** Write a {@code float} field to the stream. */
  void writeFloatNoTag(final float value) throws IOException;

  /** Write a {@code uint64} field to the stream. */
  void writeUInt64NoTag(final long value) throws IOException;

  /** Write an {@code int64} field to the stream. */
  void writeInt64NoTag(final long value) throws IOException;

  /** Write an {@code int32} field to the stream. */
  void writeInt32NoTag(final int value) throws IOException;

  /** Write a {@code fixed64} field to the stream. */
  void writeFixed64NoTag(final long value) throws IOException;

  /** Write a {@code fixed32} field to the stream. */
  void writeFixed32NoTag(final int value) throws IOException;

  /** Write a {@code bool} field to the stream. */
  void writeBoolNoTag(final boolean value) throws IOException;

  /** Write a {@code string} field to the stream. */
  // TODO(dweis): Document behavior on ill-formed UTF-16 input.
  void writeStringNoTag(final String value) throws IOException;

  /** Write a {@code group} field to the stream. */
  void writeGroupNoTag(final MessageLite value) throws IOException;

  /** Write an embedded message field to the stream. */
  void writeMessageNoTag(final MessageLite value) throws IOException;


  /** Write a {@code bytes} field to the stream. */
  void writeBytesNoTag(final ByteString value) throws IOException;

  /** Write a {@code bytes} field to the stream. */
  void writeByteArrayNoTag(final byte[] value) throws IOException;

  /** Write a {@code bytes} field to the stream. */
  void writeByteArrayNoTag(final byte[] value,
                                  final int offset,
                                  final int length) throws IOException;

  /** Write a {@code uint32} field to the stream. */
  void writeUInt32NoTag(final int value) throws IOException;

  /**
   * Write an enum field to the stream.  Caller is responsible
   * for converting the enum value to its numeric value.
   */
  void writeEnumNoTag(final int value) throws IOException;

  /** Write an {@code sfixed32} field to the stream. */
  void writeSFixed32NoTag(final int value) throws IOException;

  /** Write an {@code sfixed64} field to the stream. */
  void writeSFixed64NoTag(final long value) throws IOException;

  /** Write an {@code sint32} field to the stream. */
  void writeSInt32NoTag(final int value) throws IOException;

  /** Write an {@code sint64} field to the stream. */
  void writeSInt64NoTag(final long value) throws IOException;

  /**
   * Flushes the stream and forces any buffered bytes to be written.  This
   * does not flush the underlying OutputStream.
   */
  void flush() throws IOException;

  /** Write a single byte. */
  void writeRawByte(final byte value) throws IOException;

  /** Write a single byte, represented by an integer value. */
  void writeRawByte(final int value) throws IOException;

  /** Write a byte string. */
  void writeRawBytes(final ByteString value) throws IOException;

  /** Write an array of bytes. */
  void writeRawBytes(final byte[] value) throws IOException;

  /**
   * Write a ByteBuffer. This method will onDataEncoded all content of the ByteBuffer
   * regardless of the current position and limit (i.e., the number of bytes
   * to be written is value.capacity(), not value.remaining()). Furthermore,
   * this method doesn't alter the state of the passed-in ByteBuffer. Its
   * position, limit, mark, etc. will remain unchanged. If you only want to
   * onDataEncoded the remaining bytes of a ByteBuffer, you can call
   * {@code writeRawBytes(byteBuffer.slice())}.
   */
  void writeRawBytes(final ByteBuffer value) throws IOException;

  /** Write part of an array of bytes. */
  void writeRawBytes(final byte[] value, int offset, int length)
      throws IOException;

  /** Write part of a byte string. */
  void writeRawBytes(final ByteString value, int offset, int length)
      throws IOException;

  /** Encode and onDataEncoded a tag. */
  void writeTag(final int fieldNumber, final int wireType)
      throws IOException;

  /**
   * Encode and onDataEncoded a varint.  {@code value} is treated as
   * unsigned, so it won't be sign-extended if negative.
   */
  void writeRawVarint32(int value) throws IOException;

  /** Encode and onDataEncoded a varint. */
  void writeRawVarint64(long value) throws IOException;

  /** Write a little-endian 32-bit integer. */
  void writeRawLittleEndian32(final int value) throws IOException;

  /** Write a little-endian 64-bit integer. */
  void writeRawLittleEndian64(final long value) throws IOException;
}
