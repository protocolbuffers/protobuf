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

/**
 * An encoder for protobuf fields.
 */
public interface Encoder {

  /** Encode and onDataEncoded a tag. */
  void writeTag(int fieldNumber, int wireType) throws IOException;

  /** Write an {@code int32} field, including tag, to the stream. */
  void writeInt32(int fieldNumber, int value) throws IOException;

  /** Write a {@code uint32} field, including tag, to the stream. */
  void writeUInt32(int fieldNumber, int value) throws IOException;

  /** Write an {@code sint32} field, including tag, to the stream. */
  void writeSInt32(int fieldNumber, int value) throws IOException;

  /** Write a {@code fixed32} field, including tag, to the stream. */
  void writeFixed32(int fieldNumber, int value) throws IOException;

  /** Write an {@code sfixed32} field, including tag, to the stream. */
  void writeSFixed32(int fieldNumber, int value) throws IOException;

  /** Write an {@code int64} field, including tag, to the stream. */
  void writeInt64(int fieldNumber, long value) throws IOException;

  /** Write a {@code uint64} field, including tag, to the stream. */
  void writeUInt64(int fieldNumber, long value) throws IOException;

  /** Write an {@code sint64} field, including tag, to the stream. */
  void writeSInt64(int fieldNumber, long value) throws IOException;

  /** Write a {@code fixed64} field, including tag, to the stream. */
  void writeFixed64(int fieldNumber, long value) throws IOException;

  /** Write an {@code sfixed64} field, including tag, to the stream. */
  void writeSFixed64(int fieldNumber, long value) throws IOException;

  /** Write a {@code float} field, including tag, to the stream. */
  void writeFloat(int fieldNumber, float value) throws IOException;

  /** Write a {@code double} field, including tag, to the stream. */
  void writeDouble(int fieldNumber, double value) throws IOException;

  /** Write a {@code bool} field, including tag, to the stream. */
  void writeBool(int fieldNumber, boolean value) throws IOException;

  /** Write a {@code string} field, including tag, to the stream. */
  void writeString(int fieldNumber, String value) throws IOException;

  /** Write a {@code group} field, including tag, to the stream. */
  void writeGroup(int fieldNumber, MessageLite value) throws IOException;

  /** Write an embedded message field, including tag, to the stream. */
  void writeMessage(int fieldNumber, MessageLite value) throws IOException;

  /**
   * Write an enum field, including tag, to the stream.  Caller is responsible
   * for converting the enum value to its numeric value.
   */
  void writeEnum(int fieldNumber, int value) throws IOException;

  /**
   * Write a MessageSet extension field to the stream.  For historical reasons,
   * the wire format differs from normal fields.
   */
  void writeMessageSetExtension(int fieldNumber, MessageLite value) throws IOException;

  /**
   * Write an unparsed MessageSet extension field to the stream.  For
   * historical reasons, the wire format differs from normal fields.
   */
  void writeRawMessageSetExtension(int fieldNumber, ByteString value) throws IOException;

  /** Write a {@code bytes} field, including tag, to the stream. */
  void writeBytes(int fieldNumber, ByteString value) throws IOException;

  /** Write a {@code bytes} field, including tag, to the stream. */
  void writeByteArray(int fieldNumber, byte[] value, int offset, int length) throws IOException;

  // -----------------------------------------------------------------

  /** Write an {@code int32} field to the stream. */
  void writeInt32NoTag(int value) throws IOException;

  /** Write a {@code uint32} field to the stream. */
  void writeUInt32NoTag(int value) throws IOException;

  /** Write an {@code sint32} field to the stream. */
  void writeSInt32NoTag(int value) throws IOException;

  /** Write a {@code fixed32} field to the stream. */
  void writeFixed32NoTag(int value) throws IOException;

  /** Write an {@code sfixed32} field to the stream. */
  void writeSFixed32NoTag(int value) throws IOException;

  /** Write an {@code int64} field to the stream. */
  void writeInt64NoTag(long value) throws IOException;

  /** Write a {@code uint64} field to the stream. */
  void writeUInt64NoTag(long value) throws IOException;

  /** Write an {@code sint64} field to the stream. */
  void writeSInt64NoTag(long value) throws IOException;

  /** Write a {@code fixed64} field to the stream. */
  void writeFixed64NoTag(long value) throws IOException;

  /** Write an {@code sfixed64} field to the stream. */
  void writeSFixed64NoTag(long value) throws IOException;

  /** Write a {@code float} field to the stream. */
  void writeFloatNoTag(float value) throws IOException;

  /** Write a {@code double} field to the stream. */
  void writeDoubleNoTag(double value) throws IOException;

  /** Write a {@code bool} field to the stream. */
  void writeBoolNoTag(boolean value) throws IOException;

  /** Write a {@code string} field to the stream. */
  // TODO(dweis): Document behavior on ill-formed UTF-16 input.
  void writeStringNoTag(String value) throws IOException;

  /** Write a {@code group} field to the stream. */
  void writeGroupNoTag(MessageLite value) throws IOException;

  /** Write an embedded message field to the stream. */
  void writeMessageNoTag(MessageLite value) throws IOException;

  /**
   * Write an enum field to the stream.  Caller is responsible
   * for converting the enum value to its numeric value.
   */
  void writeEnumNoTag(int value) throws IOException;

  /** Write a {@code bytes} field to the stream. */
  void writeBytesNoTag(ByteString value) throws IOException;

  /** Write a {@code bytes} field to the stream. */
  void writeByteArrayNoTag(byte[] value, int offset, int length) throws IOException;

  /**
   * Flushes the stream and forces any buffered bytes to be written.  This
   * does not flush the underlying OutputStream.
   */
  void flush() throws IOException;
}
