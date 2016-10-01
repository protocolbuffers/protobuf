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

import java.util.List;

/** A writer that performs serialization of protobuf message fields. */
@ExperimentalApi
public interface Writer {
  /** Writes a field of type {@link FieldType#SFIXED32}. */
  void writeSFixed32(int fieldNumber, int value);

  /** Writes a field of type {@link FieldType#INT64}. */
  void writeInt64(int fieldNumber, long value);

  /** Writes a field of type {@link FieldType#SFIXED64}. */
  void writeSFixed64(int fieldNumber, long value);

  /** Writes a field of type {@link FieldType#FLOAT}. */
  void writeFloat(int fieldNumber, float value);

  /** Writes a field of type {@link FieldType#DOUBLE}. */
  void writeDouble(int fieldNumber, double value);

  /** Writes a field of type {@link FieldType#ENUM}. */
  void writeEnum(int fieldNumber, int value);

  /** Writes a field of type {@link FieldType#UINT64}. */
  void writeUInt64(int fieldNumber, long value);

  /** Writes a field of type {@link FieldType#INT32}. */
  void writeInt32(int fieldNumber, int value);

  /** Writes a field of type {@link FieldType#FIXED64}. */
  void writeFixed64(int fieldNumber, long value);

  /** Writes a field of type {@link FieldType#FIXED32}. */
  void writeFixed32(int fieldNumber, int value);

  /** Writes a field of type {@link FieldType#BOOL}. */
  void writeBool(int fieldNumber, boolean value);

  /** Writes a field of type {@link FieldType#STRING}. */
  void writeString(int fieldNumber, String value);

  /** Writes a field of type {@link FieldType#BYTES}. */
  void writeBytes(int fieldNumber, ByteString value);

  /** Writes a field of type {@link FieldType#UINT32}. */
  void writeUInt32(int fieldNumber, int value);

  /** Writes a field of type {@link FieldType#SINT32}. */
  void writeSInt32(int fieldNumber, int value);

  /** Writes a field of type {@link FieldType#SINT64}. */
  void writeSInt64(int fieldNumber, long value);

  /** Writes a field of type {@link FieldType#MESSAGE}. */
  void writeMessage(int fieldNumber, Object value);

  /**
   * Writes a field of type {@link FieldType#GROUP}.
   *
   * @deprecated groups fields are deprecated.
   */
  @Deprecated
  void writeGroup(int fieldNumber, Object value);

  /** Writes a list field of type {@link FieldType#INT32}. */
  void writeInt32List(int fieldNumber, List<Integer> value, boolean packed);

  /** Writes a list field of type {@link FieldType#FIXED32}. */
  void writeFixed32List(int fieldNumber, List<Integer> value, boolean packed);

  /** Writes a list field of type {@link FieldType#INT64}. */
  void writeInt64List(int fieldNumber, List<Long> value, boolean packed);

  /** Writes a list field of type {@link FieldType#UINT64}. */
  void writeUInt64List(int fieldNumber, List<Long> value, boolean packed);

  /** Writes a list field of type {@link FieldType#FIXED64}. */
  void writeFixed64List(int fieldNumber, List<Long> value, boolean packed);

  /** Writes a list field of type {@link FieldType#FLOAT}. */
  void writeFloatList(int fieldNumber, List<Float> value, boolean packed);

  /** Writes a list field of type {@link FieldType#DOUBLE}. */
  void writeDoubleList(int fieldNumber, List<Double> value, boolean packed);

  /** Writes a list field of type {@link FieldType#ENUM}. */
  void writeEnumList(int fieldNumber, List<Integer> value, boolean packed);

  /** Writes a list field of type {@link FieldType#BOOL}. */
  void writeBoolList(int fieldNumber, List<Boolean> value, boolean packed);

  /** Writes a list field of type {@link FieldType#STRING}. */
  void writeStringList(int fieldNumber, List<String> value);

  /** Writes a list field of type {@link FieldType#BYTES}. */
  void writeBytesList(int fieldNumber, List<ByteString> value);

  /** Writes a list field of type {@link FieldType#UINT32}. */
  void writeUInt32List(int fieldNumber, List<Integer> value, boolean packed);

  /** Writes a list field of type {@link FieldType#SFIXED32}. */
  void writeSFixed32List(int fieldNumber, List<Integer> value, boolean packed);

  /** Writes a list field of type {@link FieldType#SFIXED64}. */
  void writeSFixed64List(int fieldNumber, List<Long> value, boolean packed);

  /** Writes a list field of type {@link FieldType#SINT32}. */
  void writeSInt32List(int fieldNumber, List<Integer> value, boolean packed);

  /** Writes a list field of type {@link FieldType#SINT64}. */
  void writeSInt64List(int fieldNumber, List<Long> value, boolean packed);

  /** Writes a list field of type {@link FieldType#MESSAGE}. */
  void writeMessageList(int fieldNumber, List<?> value);

  /**
   * Writes a list field of type {@link FieldType#GROUP}.
   *
   * @deprecated groups fields are deprecated.
   */
  @Deprecated
  void writeGroupList(int fieldNumber, List<?> value);
}
