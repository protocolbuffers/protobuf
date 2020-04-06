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

import java.io.IOException;
import java.util.List;
import java.util.Map;

/** A writer that performs serialization of protobuf message fields. */
@ExperimentalApi
interface Writer {

  /** The order in which the fields are written by a {@link Writer}. */
  enum FieldOrder {
    /** Fields are written in ascending order by field number. */
    ASCENDING,

    /** Fields are written in descending order by field number. */
    DESCENDING
  }

  /** Indicates the order in which the fields are written by this {@link Writer}. */
  FieldOrder fieldOrder();

  /** Writes a field of type {@link FieldType#SFIXED32}. */
  void writeSFixed32(int fieldNumber, int value) throws IOException;

  /** Writes a field of type {@link FieldType#INT64}. */
  void writeInt64(int fieldNumber, long value) throws IOException;

  /** Writes a field of type {@link FieldType#SFIXED64}. */
  void writeSFixed64(int fieldNumber, long value) throws IOException;

  /** Writes a field of type {@link FieldType#FLOAT}. */
  void writeFloat(int fieldNumber, float value) throws IOException;

  /** Writes a field of type {@link FieldType#DOUBLE}. */
  void writeDouble(int fieldNumber, double value) throws IOException;

  /** Writes a field of type {@link FieldType#ENUM}. */
  void writeEnum(int fieldNumber, int value) throws IOException;

  /** Writes a field of type {@link FieldType#UINT64}. */
  void writeUInt64(int fieldNumber, long value) throws IOException;

  /** Writes a field of type {@link FieldType#INT32}. */
  void writeInt32(int fieldNumber, int value) throws IOException;

  /** Writes a field of type {@link FieldType#FIXED64}. */
  void writeFixed64(int fieldNumber, long value) throws IOException;

  /** Writes a field of type {@link FieldType#FIXED32}. */
  void writeFixed32(int fieldNumber, int value) throws IOException;

  /** Writes a field of type {@link FieldType#BOOL}. */
  void writeBool(int fieldNumber, boolean value) throws IOException;

  /** Writes a field of type {@link FieldType#STRING}. */
  void writeString(int fieldNumber, String value) throws IOException;

  /** Writes a field of type {@link FieldType#BYTES}. */
  void writeBytes(int fieldNumber, ByteString value) throws IOException;

  /** Writes a field of type {@link FieldType#UINT32}. */
  void writeUInt32(int fieldNumber, int value) throws IOException;

  /** Writes a field of type {@link FieldType#SINT32}. */
  void writeSInt32(int fieldNumber, int value) throws IOException;

  /** Writes a field of type {@link FieldType#SINT64}. */
  void writeSInt64(int fieldNumber, long value) throws IOException;

  /** Writes a field of type {@link FieldType#MESSAGE}. */
  void writeMessage(int fieldNumber, Object value) throws IOException;

  /** Writes a field of type {@link FieldType#MESSAGE}. */
  void writeMessage(int fieldNumber, Object value, Schema schema) throws IOException;

  /**
   * Writes a field of type {@link FieldType#GROUP}.
   *
   * @deprecated groups fields are deprecated.
   */
  @Deprecated
  void writeGroup(int fieldNumber, Object value) throws IOException;

  /**
   * Writes a field of type {@link FieldType#GROUP}.
   *
   * @deprecated groups fields are deprecated.
   */
  @Deprecated
  void writeGroup(int fieldNumber, Object value, Schema schema) throws IOException;

  /**
   * Writes a single start group tag.
   *
   * @deprecated groups fields are deprecated.
   */
  @Deprecated
  void writeStartGroup(int fieldNumber) throws IOException;

  /**
   * Writes a single end group tag.
   *
   * @deprecated groups fields are deprecated.
   */
  @Deprecated
  void writeEndGroup(int fieldNumber) throws IOException;

  /** Writes a list field of type {@link FieldType#INT32}. */
  void writeInt32List(int fieldNumber, List<Integer> value, boolean packed) throws IOException;

  /** Writes a list field of type {@link FieldType#FIXED32}. */
  void writeFixed32List(int fieldNumber, List<Integer> value, boolean packed) throws IOException;

  /** Writes a list field of type {@link FieldType#INT64}. */
  void writeInt64List(int fieldNumber, List<Long> value, boolean packed) throws IOException;

  /** Writes a list field of type {@link FieldType#UINT64}. */
  void writeUInt64List(int fieldNumber, List<Long> value, boolean packed) throws IOException;

  /** Writes a list field of type {@link FieldType#FIXED64}. */
  void writeFixed64List(int fieldNumber, List<Long> value, boolean packed) throws IOException;

  /** Writes a list field of type {@link FieldType#FLOAT}. */
  void writeFloatList(int fieldNumber, List<Float> value, boolean packed) throws IOException;

  /** Writes a list field of type {@link FieldType#DOUBLE}. */
  void writeDoubleList(int fieldNumber, List<Double> value, boolean packed) throws IOException;

  /** Writes a list field of type {@link FieldType#ENUM}. */
  void writeEnumList(int fieldNumber, List<Integer> value, boolean packed) throws IOException;

  /** Writes a list field of type {@link FieldType#BOOL}. */
  void writeBoolList(int fieldNumber, List<Boolean> value, boolean packed) throws IOException;

  /** Writes a list field of type {@link FieldType#STRING}. */
  void writeStringList(int fieldNumber, List<String> value) throws IOException;

  /** Writes a list field of type {@link FieldType#BYTES}. */
  void writeBytesList(int fieldNumber, List<ByteString> value) throws IOException;

  /** Writes a list field of type {@link FieldType#UINT32}. */
  void writeUInt32List(int fieldNumber, List<Integer> value, boolean packed) throws IOException;

  /** Writes a list field of type {@link FieldType#SFIXED32}. */
  void writeSFixed32List(int fieldNumber, List<Integer> value, boolean packed) throws IOException;

  /** Writes a list field of type {@link FieldType#SFIXED64}. */
  void writeSFixed64List(int fieldNumber, List<Long> value, boolean packed) throws IOException;

  /** Writes a list field of type {@link FieldType#SINT32}. */
  void writeSInt32List(int fieldNumber, List<Integer> value, boolean packed) throws IOException;

  /** Writes a list field of type {@link FieldType#SINT64}. */
  void writeSInt64List(int fieldNumber, List<Long> value, boolean packed) throws IOException;

  /** Writes a list field of type {@link FieldType#MESSAGE}. */
  void writeMessageList(int fieldNumber, List<?> value) throws IOException;

  /** Writes a list field of type {@link FieldType#MESSAGE}. */
  void writeMessageList(int fieldNumber, List<?> value, Schema schema) throws IOException;

  /**
   * Writes a list field of type {@link FieldType#GROUP}.
   *
   * @deprecated groups fields are deprecated.
   */
  @Deprecated
  void writeGroupList(int fieldNumber, List<?> value) throws IOException;

  /**
   * Writes a list field of type {@link FieldType#GROUP}.
   *
   * @deprecated groups fields are deprecated.
   */
  @Deprecated
  void writeGroupList(int fieldNumber, List<?> value, Schema schema) throws IOException;

  /**
   * Writes a message field in {@code MessageSet} wire-format.
   *
   * @param value A message instance or an opaque {@link ByteString} for an unknown field.
   */
  void writeMessageSetItem(int fieldNumber, Object value) throws IOException;

  /** Writes a map field. */
  <K, V> void writeMap(int fieldNumber, MapEntryLite.Metadata<K, V> metadata, Map<K, V> map)
      throws IOException;
}
